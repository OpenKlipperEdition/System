/*
 * Nsiway NS2009 touchscreen controller driver
 *
 * Copyright (C) 2017 Icenowy Zheng <icenowy@aosc.xyz>
 *
 * Some codes are from silead.c, which is
 *   Copyright (C) 2014-2015 Intel Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/input/touchscreen.h>
#include <linux/i2c.h>
#include <linux/gpio/consumer.h>

/* polling interval in ms */
#define POLL_INTERVAL	30

/* this driver uses 12-bit readout */
#define MAX_12BIT	0xfff

#define NS2009_TS_NAME	"ns2009_ts"

#define NS2009_READ_X_LOW_POWER_12BIT	0xc0
#define NS2009_READ_Y_LOW_POWER_12BIT	0xd0
#define NS2009_READ_Z1_LOW_POWER_12BIT	0xe0
#define NS2009_READ_Z2_LOW_POWER_12BIT	0xf0

#define NS2009_DEF_X_FUZZ	32
#define NS2009_DEF_Y_FUZZ	16

/*
 * The chip have some error in z1 value when pen is up, so the data read out
 * is sometimes not accurately 0.
 * This value is based on experiements.
 */
#define NS2009_PEN_UP_Z1_ERR	80

struct ns2009_data {
	struct i2c_client		*client;
	struct input_dev		*input;

	struct touchscreen_properties	prop;

	bool				pen_down;

	/* ke-mainline-klipper touch mission: optional "pendown-gpios" DT
	 * property, matching stock's real (disassembly-proven) touch-detect
	 * signal - see the halley5_v30.dts ns2009@48 node for the full
	 * evidence trail. NULL (no property present) preserves the original
	 * generic upstream Z1-threshold-only behavior unchanged for any other
	 * board using this driver. */
	struct gpio_desc		*pendown_gpio;
};

static int ns2009_ts_read_data(struct ns2009_data *data, u8 cmd, u16 *val)
{
	u8 raw_data[2];
	int error;

	error = i2c_smbus_read_i2c_block_data(data->client, cmd, 2, raw_data);
	if (error < 0)
		return error;

	if (unlikely(raw_data[1] & 0xf))
		return -EINVAL;

	*val = (raw_data[0] << 4) | (raw_data[1] >> 4);

	return 0;
}

static int ns2009_ts_report(struct ns2009_data *data)
{
	u16 x, y, z1;
	int ret;

	/*
	 * NS2009 chip supports pressure measurement, but currently it needs
	 * more investigation, so we only use z1 axis to detect pen down
	 * here.
	 */
	ret = ns2009_ts_read_data(data, NS2009_READ_Z1_LOW_POWER_12BIT, &z1);
	if (ret) {
		/* Temporary diagnostic (ke-mainline-klipper touch mission):
		 * custom reports zero coordinate events on this exact board
		 * despite matching capabilities/ranges vs stock - need to see
		 * whether z1 reads are failing outright (as opposed to
		 * succeeding but never crossing the pen-down threshold).
		 * Rate-limited, bounded, read-only - no behavior change. */
		pr_err_ratelimited("ns2009 diag: z1 read failed, ret=%d\n", ret);
		return ret;
	}

	/* Temporary diagnostic: log every z1 sample near/at the threshold so
	 * a real touch's actual value is visible even if it never crosses
	 * NS2009_PEN_UP_Z1_ERR (80) - would otherwise be silently invisible,
	 * since only the pen-down/up transition (not the raw pressure
	 * reading itself) was ever logged anywhere. */
	pr_info_ratelimited("ns2009 diag: z1=%u threshold=%u pen_down=%d gpio=%d\n",
			     z1, NS2009_PEN_UP_Z1_ERR, data->pen_down,
			     data->pendown_gpio ? gpiod_get_value_cansleep(data->pendown_gpio) : -1);

	/* ke-mainline-klipper touch mission: when a pendown-gpios property is
	 * present, use it as the pen-down signal instead of the Z1 pressure
	 * threshold - proven live on this exact board that z1 always reads 0
	 * regardless of touch state, while stock's own driver never uses Z1
	 * at all (interrupt-driven off this exact GPIO instead). Boards
	 * without the property keep the original upstream Z1-only behavior
	 * unchanged. */
	if (data->pendown_gpio ?
	    gpiod_get_value_cansleep(data->pendown_gpio) :
	    (z1 >= NS2009_PEN_UP_Z1_ERR)) {
		ret = ns2009_ts_read_data(data, NS2009_READ_X_LOW_POWER_12BIT,
					  &x);
		if (ret)
			return ret;

		ret = ns2009_ts_read_data(data, NS2009_READ_Y_LOW_POWER_12BIT,
					  &y);
		if (ret)
			return ret;

		if (!data->pen_down) {
			input_report_key(data->input, BTN_TOUCH, 1);
			data->pen_down = true;
		}

		input_report_abs(data->input, ABS_X, x);
		input_report_abs(data->input, ABS_Y, y);
		input_sync(data->input);
	} else if (data->pen_down) {
		input_report_key(data->input, BTN_TOUCH, 0);
		input_sync(data->input);
		data->pen_down = false;
	}
	return 0;
}

static void ns2009_ts_poll(struct input_dev *input_dev)
{
	struct ns2009_data *data = input_get_drvdata(input_dev);
	int ret;

	ret = ns2009_ts_report(data);
	if (ret)
		dev_err(&input_dev->dev, "Poll touch data failed: %d\n", ret);
}

static void ns2009_ts_config_input_dev(struct ns2009_data *data)
{
	struct input_dev *input = data->input;

	input_set_abs_params(input, ABS_X, 0, MAX_12BIT, NS2009_DEF_X_FUZZ, 0);
	input_set_abs_params(input, ABS_Y, 0, MAX_12BIT, NS2009_DEF_Y_FUZZ, 0);
	touchscreen_parse_properties(input, false, &data->prop);

	input->name = NS2009_TS_NAME;
	input->phys = "input/ts";
	input->id.bustype = BUS_I2C;
	input_set_capability(input, EV_KEY, BTN_TOUCH);
}

static int ns2009_ts_request_polled_input_dev(struct ns2009_data *data)
{
	struct device *dev = &data->client->dev;
	int error;

	data->input = devm_input_allocate_device(dev);
	if (!data->input) {
		dev_err(dev, "Failed to allocate input device\n");
		return -ENOMEM;
	}
	input_set_drvdata(data->input, data);

	ns2009_ts_config_input_dev(data);

	error = input_setup_polling(data->input, ns2009_ts_poll);
	if (error) {
		dev_err(dev, "Failed to set up polling: %d\n", error);
		return error;
	}
	input_set_poll_interval(data->input, POLL_INTERVAL);

	error = input_register_device(data->input);
	if (error) {
		dev_err(dev, "Failed to register input device: %d\n", error);
		return error;
	}

	return 0;
}

static int ns2009_ts_probe(struct i2c_client *client)
{
	struct ns2009_data *data;
	struct device *dev = &client->dev;
	int error;

	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_I2C |
				     I2C_FUNC_SMBUS_READ_I2C_BLOCK |
				     I2C_FUNC_SMBUS_WRITE_I2C_BLOCK)) {
		dev_err(dev, "I2C functionality check failed\n");
		return -ENXIO;
	}

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	i2c_set_clientdata(client, data);
	data->client = client;

	/* ke-mainline-klipper touch mission: optional, absent on any board
	 * that doesn't declare "pendown-gpios" in its DT node - see the
	 * struct field comment and halley5_v30.dts for the full evidence
	 * trail behind this exact property on this exact board. */
	data->pendown_gpio = devm_gpiod_get_optional(dev, "pendown", GPIOD_IN);
	if (IS_ERR(data->pendown_gpio)) {
		dev_err(dev, "Failed to get pendown-gpios: %ld\n",
			PTR_ERR(data->pendown_gpio));
		return PTR_ERR(data->pendown_gpio);
	}

	error = ns2009_ts_request_polled_input_dev(data);
	if (error)
		return error;

	return 0;
};

static const struct i2c_device_id ns2009_ts_id[] = {
	{ "ns2009", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ns2009_ts_id);

#ifdef CONFIG_OF
static const struct of_device_id ns2009_ts_of_match[] = {
	{ .compatible = "nsiway,ns2009", },
	{ }
};
MODULE_DEVICE_TABLE(of, ns2009_ts_of_match);
#endif

static struct i2c_driver ns2009_ts_driver = {
	.probe = ns2009_ts_probe,
	.id_table = ns2009_ts_id,
	.driver = {
		.name = NS2009_TS_NAME,
		.of_match_table = of_match_ptr(ns2009_ts_of_match),
	},
};
module_i2c_driver(ns2009_ts_driver);

MODULE_AUTHOR("Icenowy Zheng <icenowy@aosc.xyz>");
MODULE_DESCRIPTION("Nsiway NS2009 touchscreen controller driver");
MODULE_LICENSE("GPL");
