**君正®**

![](assets/10-Halley6_CIM适配新型号camera指导_v1.0.0.png)**君正®**


**Release history**



| Date       | Revision | Change        |
| ---------- | -------- | ------------- |
| 2024.09.10 | 1.0      | First release |


# 简介

本文档主要针对添加一个新型号 camera 到 kernel-x.x 标准内核平台进行说明。



# camera 适配前准备

## 硬件原理图

自定义的主板适配，需要主板原理图，平台公板适配，需要模组转接板原理图以及对应公板的原理图。

## init setting

适配前需确定camera setting，包含信息有：register init list，mclk，输出 raw 图位宽，输出格式，mipi clk/lanes 或 dvp vsync/hsync/pclk，分辨率，i2c 地址。



# 配置设备树

适配新型号camera时，可以仿照已有型号配置camera.dtsi，然后在板级.dts中include对应的 dtsi。

设备树所在位置：
halley6: module_drivers/dts/halley6_cameras
panda  : module_drivers/dts/panda_cameras


## mipi camera 配置

在halley6_cameras/RD_X1600_HALLEY6_MIPI_CAMERA.dtsi中，对cim控制器进行如下配置：其中，data-lanes字段根据接入的mipi sensor的data lane数量填充，例如：接入2 lane mipi sensor，data-lanes需填充两个参数，data-lanes = <0 1>，接入4 lane mipi sensor，data-lanes需填充四个参数，data-lanes = <0 1 3 4>；clk-lanes字段根据接入的mipi sensor的clock lane数量填充，例如：mipi sensor 连接一根clk lane，clk-lanes需填充一个参数，clk-lanes = <2>。

```c
&cim {
        status = "okay";
        port {
                cim_0: endpoint@0 {
                        remote-endpoint = <&sc031gs_ep0>;
                        data-lanes = <0 1>;
                        clk-lanes = <2>;
                };
        };
};
```

在halley6_cameras/RD_X1600_HALLEY6_MIPI_CAMERA.dtsi中，对camera sensor进行如下配置：根据原理图确定i2c，例如：i2c0_pa；根据camera datasheet确定i2c address，例如：0x30；确定mclk，reset，pwdn，vcc等gpio有效方式以及上电时序。

```c
&i2c0 {
        status = "okay";
        clock-frequency = <100000>;
        timeout = <1000>;
        pinctrl-names = "default";
        pinctrl-0 = <&i2c0_pa>;

        sc031gs_0:sc031gs@30 {
                status = "okay";
                compatible = "sc031gs";
                reg = <0x30>;
                pinctrl-names = "default";
                pinctrl-0 = <&cim_mipi_mclk_pc>;

                resetb-gpios = <&gpa 30 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;
                vcc-en-gpios = <&gpb 12 GPIO_ACTIVE_HIGH INGENIC_GPIO_NOBIAS>;

                port {
                        sc031gs_ep0:endpoint {
                                remote-endpoint = <&cim_0>;
                        };
                };
        };
};
```


## dvp camera 配置

在halley6_cameras/RD_X1600_HALLEY6_DVP_CAMERA.dtsi中，对cim控制器进行如下配置:根据datasheet 或波形测量确定hsync,vsync,pclk,data有效方式。

```c
&cim {
        status = "okay";
        port {
                cim_0: endpoint@0 {
                        remote-endpoint = <&sc031gs_ep0>;
                        bus-width = <8>;
                        data-shift = <0>;
                        bus-type = <5>;
                        hsync-active = <1>;
                        vsync-active = <0>;
                        data-active = <1>;
                        pclk-sample = <1>;
                };
        };
};
```

在halley6_cameras/RD_X1600_HALLEY6_DVP_CAMERA.dtsi中，对camera sensor进行如下配置：

```c
&i2c0 {
        status = "okay";
        clock-frequency = <100000>;
        timeout = <1000>;
        pinctrl-names = "default";
        pinctrl-0 = <&i2c0_pa>;

        sc031gs_0:sc031gs@30 {
                status = "okay";
                compatible = "sc031gs";
                reg = <0x30>;
                pinctrl-names = "default";
                pinctrl-0 = <&cim_mipi_mclk_pc>, <&cim_pa>;

                resetb-gpios = <&gpa 30 GPIO_ACTIVE_HIGH INGENIC_GPIO_NOBIAS>;
                vcc-en-gpios = <&gpb 12 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;

                port {
                        sc031gs_ep0:endpoint {
                                remote-endpoint = <&cim_0>;
                        };
                };
        };
};
```


## bt656 camera 配置

在wy827_bt656.dtsi中，对cim控制器进行如下配置：根据datasheet或波形测量确定pclk,data有效方式。

```c
&cim {
        status = "okay";
        port {
                cim_0: endpoint@0 {
                        remote-endpoint = <&wy827_ep0>;
                        bus-width = <8>;        /* Used data lines */
                        data-shift = <0>;       /* Lines 9:0 are used */

                        /* If hsync-active/vsync-active are missing,
                                embedded BT.656 sync is used */
                        pclk-sample = <0>;      /* Falling */
                        data-active = <1>;      /* Active high */
                };
        };
};

```

在wy827_bt656.dtsi中，对camera sensor进行如下配置：根据原理图确定i2c，例如：i2c3_pc；根据camera datasheet确定i2c address，例如：0x3d；确定mclk，reset，pwdn，vcc等gpio有效方式以及上电时序。

```c
&i2c3 {
        status = "okay";
        clock-frequency = <100000>;
        timeout = <1000>;
        pinctrl-names = "default";
        pinctrl-0 = <&i2c3_pc>;

        wy827_0:wy827@0x3d {
                status = "ok";
                compatible = "bt656,wy827";
                reg = <0x3d>;
                pinctrl-names = "default";
                pinctrl-0 = <&cim_pa>;

                port {
                        wy827_ep0:endpoint {
                                remote-endpoint = <&cim_0>;
                        };
                };
        };
};

```


# 配置驱动

适配新型号camera时，可以仿照已有型号。以下将以sc031gs为例进一步分析如何配置驱动。

驱动所在位置：
module_drivers/drivers/media/i2c/ingenic-cim


## 获取 reset、pdwn、vcc 等 gpio 属性

```c
/* Request the reset GPIO deasserted */
priv->resetb_gpio = devm_gpiod_get_optional(&client->dev, "resetb", GPIOD_OUT_LOW);
if (!priv->resetb_gpio)
	dev_dbg(&client->dev, "resetb gpio is not assigned!\n");
else if (IS_ERR(priv->resetb_gpio))
	return PTR_ERR(priv->resetb_gpio);

/* Request the power down GPIO asserted */
priv->pwdn_gpio = devm_gpiod_get_optional(&client->dev, "pwdn", GPIOD_OUT_LOW);
if (!priv->pwdn_gpio)
	dev_dbg(&client->dev, "pwdn gpio is not assigned!\n");
else if (IS_ERR(priv->pwdn_gpio))
	return PTR_ERR(priv->pwdn_gpio);

/* Request the power down GPIO asserted */
priv->vcc_en_gpio = devm_gpiod_get_optional(&client->dev, "vcc-en", GPIOD_OUT_LOW);
if (!priv->vcc_en_gpio)
	dev_dbg(&client->dev, "vcc_en gpio is not assigned!\n");
else if (IS_ERR(priv->vcc_en_gpio))
	return PTR_ERR(priv->vcc_en_gpio);

```


## 配置mclk

```c
priv->clk = v4l2_clk_get(&client->dev, "div_cim");
if (IS_ERR(priv->clk))
	return -EPROBE_DEFER;

v4l2_clk_set_rate(priv->clk, 24000000);  /* mclk 24M */

```


## 配置camera chip_id

根据datasheet确定chip_id和寄存器。

```c
#define REG_CHIP_ID_HIGH        0x3107
#define REG_CHIP_ID_LOW         0x3108
#define CHIP_ID_HIGH            0x00
#define CHIP_ID_LOW             0x31

/*
 * check and show product ID and manufacturer ID
 */

retval_high = sc031gs_read_reg(client, REG_CHIP_ID_HIGH);
if (retval_high != CHIP_ID_HIGH) {
	dev_err(&client->dev, "read sensor %s chip_id high %x is error\n",
			client->name, retval_high);
	ret = -EINVAL;
	goto done;
}


retval_low = sc031gs_read_reg(client, REG_CHIP_ID_LOW);
if (retval_low != CHIP_ID_LOW) {
	dev_err(&client->dev, "read sensor %s chip_id low %x is error\n",
			client->name, retval_low);
	ret = -EINVAL;
	goto done;
}

```


## 添加camera初始化寄存器列表

该列表需要有模组厂商提供。

```c
struct regval_list {
        u16 reg_num;
        u16 value;
};

static const struct regval_list sc031gs_init_regs[] = {
        {0x0103, 0x01},
        {0x0100, 0x00},
        {0x36e9, 0x80},
        {0x36f9, 0x80},
        {0x3001, 0x00},
        {0x3000, 0x00},
        {0x300f, 0x0f},
        {………………, …………},
        {………………, …………},
        {SC031_REG_DELAY,10},
        ENDMARKER,
};

```

## camera参数配置

xxx_win_size结构体用于存储camera分辨率，输出格式，mclk频率，寄存器初始化等信息。


```c
#define  SC031GS_DEFAULT_WIDTH    640
#define  SC031GS_DEFAULT_HEIGHT   480

enum sc031gs_width {
        W_VGA   = SC031GS_DEFAULT_WIDTH,
};

enum sc031gs_height {
        H_VGA   = SC031GS_DEFAULT_HEIGHT,
};

struct sc031gs_win_size {
        char *name;
        enum sc031gs_width width;
        enum sc031gs_height height;
        const struct regval_list *regs;
        unsigned int mbus_code;
        struct sensor_info sensor_info;
};

static struct sc031gs_win_size sc031gs_supported_win_sizes[] = {
        {
                .name = "sc031gs",
                .width = W_VGA,
                .height = H_VGA,
                .regs = sc031gs_vga_regs,
                .mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,
                .sensor_info.mipi_clk = 800,
        },
};

```

| output format |                    mbus_code                         |
| --------------| ---------------------------------------------------- |
|     raw8      | MEDIA_BUS_FMT_SBGGR8_1X8   (序列:BGGR/GBRG/GRBG/RGGB)|
|     raw10     | MEDIA_BUS_FMT_SGBRG10_1X10 (序列:BGGR/GBRG/GRBG/RGGB)|
|     raw12     | MEDIA_BUS_FMT_SGRBG12_1X12 (序列:BGGR/GBRG/GRBG/RGGB)|
|     GREY      | MEDIA_BUS_FMT_Y8_1X8                                 |
|     YUV       | MEDIA_BUS_FMT_YVYU8_2X8    (序列:YUYV/YVYU/UYVY/VYUY)|




