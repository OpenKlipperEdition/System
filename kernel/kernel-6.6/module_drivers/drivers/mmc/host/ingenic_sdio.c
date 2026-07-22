#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdio.h>
#include <linux/scatterlist.h>
#include <linux/dma-mapping.h>

#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/rtc.h>

#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>

#if (defined(CONFIG_INGENIC_MMC_MMC0) || defined(CONFIG_INGENIC_MMC_MMC1))
	#include "ingenic_mmc.h"
#else
	#include "sdhci-ingenic.h"
#endif

struct wifi_data {
	int sdio_index;
	struct gpio_desc *wifi_reset;
	struct gpio_desc *wifi_irq;
	struct pinctrl *pctrl;
	atomic_t rtc32k_ref;
	struct clk *clk;
};

#define MANUALLY_INSERT 2

struct wifi_data wifi_data;
static int rtc32k_init(struct device *dev, struct wifi_data *wdata)
{
	atomic_set(&wdata->rtc32k_ref, 0);
	wdata->pctrl = NULL;
	wdata->clk = NULL;
#ifdef CONFIG_RTC_DRV_PCF8563
	struct rtc_device *rtc_pdev = NULL;

	rtc_pdev = rtc_class_open(CONFIG_RTC_HCTOSYS_DEVICE);
	if (rtc_pdev) {
		printk("%s is loaded!!!\n", CONFIG_RTC_HCTOSYS_DEVICE);
	} else {
		printk("%s is not loaded!!!\n", CONFIG_RTC_HCTOSYS_DEVICE);
		return -1;
	}

	{
		struct of_phandle_args phandle;
		phandle.np = of_find_compatible_node(NULL, NULL, "nxp,pcf8563");
		if (phandle.np) {
			wdata->clk = of_clk_get_from_provider(&phandle);
		}
	}
#else
	wdata->pctrl = devm_pinctrl_get(dev);
#endif
	return 0;
}

void rtc32k_enable(void)
{

	if (atomic_inc_return(&wifi_data.rtc32k_ref) == 1) {
		if (wifi_data.clk) {
			printk("@@ rtc.pcf8563 %s @@\n", __func__);
			clk_prepare_enable(wifi_data.clk);
		}
		if (wifi_data.pctrl) {
			struct pinctrl_state *state = NULL;
			struct pinctrl *p = wifi_data.pctrl;
			state = pinctrl_lookup_state(p, "enable");
			if (!IS_ERR_OR_NULL(state)) {
				pinctrl_select_state(p, state);
			}
		}
	}
}
EXPORT_SYMBOL(rtc32k_enable);

void rtc32k_disable(void)
{
	if (atomic_dec_return(&wifi_data.rtc32k_ref) == 0) {
		if (wifi_data.clk) {
			printk("@@ rtc.pcf8563 %s @@\n", __func__);
			clk_disable_unprepare(wifi_data.clk);
		}
		if (wifi_data.pctrl) {
			struct pinctrl_state *state = NULL;
			struct pinctrl *p = wifi_data.pctrl;
			state = pinctrl_lookup_state(p, "disable");
			if (!IS_ERR_OR_NULL(state)) {
				pinctrl_select_state(p, state);
			}
		}
	}
}
EXPORT_SYMBOL(rtc32k_disable);

static const struct of_device_id wlan_ingenic_of_match[] = {
	{.compatible = "rtk,rtl8723ds_wlan"},
	{},
};

int ingenic_sdio_wlan_init(struct device *dev, int index)
{
	struct device_node *np = dev->of_node, *cnp;
	int ret = 0;

	for_each_child_of_node(np, cnp) {
		if (of_device_is_compatible(cnp, "rtk,rtl8723ds_wlan")) {
			printk("----rtk,rtl8723ds_wlan\n");
			wifi_data.wifi_reset = devm_gpiod_get_optional(dev, "ingenic,sdio-reset", GPIOD_OUT_HIGH);
			wifi_data.wifi_irq = devm_gpiod_get_optional(dev, "ingenic,sdio-irq", GPIOD_IN);
		}
	}

	ret = rtc32k_init(dev, &wifi_data);
	if (ret) {
		dev_err(dev, "Failed to init rtc32k clock!\n");
		return ret;
	}
	wifi_data.sdio_index = index;

	return 0;
}
EXPORT_SYMBOL(ingenic_sdio_wlan_init);

int ingenic_bcmdhd_wlan_power_onoff(int flag)
{
	if (flag) {
		/* OpenKE (2026-07-22, FIRMWARE.md sec 50): WIFI_SEQ event markers -
		 * cheap, always-on (this whole path only runs on manual insert/
		 * remove, not per-command), matching the event timeline this
		 * project's own investigation notes track stock against. */
		printk("WIFI_SEQ: wlan power on requested, flag=%d\n", flag);
		printk("wlan power on:%d\n", flag);
		printk("WIFI_SEQ: RTC32K enable requested\n");
		rtc32k_enable();
		printk("WIFI_SEQ: RTC32K enable completed\n");
		printk("WIFI_SEQ: MSC1 clock enable requested\n");
		ingenic_mmc_clk_ctrl(wifi_data.sdio_index, 1);
		printk("WIFI_SEQ: MSC1 clock enable completed\n");
		if (flag == MANUALLY_INSERT) {
			printk("WIFI_SEQ: manual insert requested\n");
			ingenic_mmc_manual_detect(wifi_data.sdio_index, 1);
			printk("WIFI_SEQ: manual insert call returned\n");
		}
	} else {
		printk("WIFI_SEQ: wlan power off requested\n");
		printk("wlan power off:%d\n", flag);
		rtc32k_disable();
	}
	return 0;

}
EXPORT_SYMBOL(ingenic_bcmdhd_wlan_power_onoff);

/* OpenKE (2026-07-22, FIRMWARE.md sec 46): real, live dmesg from stock (this exact
 * board, this exact chip) shows the SDIO card is NEVER found by a generic eager
 * rescan at msc1-probe time at all - stock's own soc_msc.ko has msc1_rst=-1,
 * msc1_pwr=-1 (no power/reset gpio of its own whatsoever). ALL real power
 * sequencing happens inside cywdhd.ko's own direct gpio_direction_output() calls
 * (bypassing the devicetree mmc-pwrseq framework entirely), which THEN explicitly
 * calls this exact function with MANUALLY_INSERT - only then does the card ever
 * respond. Six real, independently-verified devicetree/driver fixes (correct pin,
 * correct polarity, freed the pin from uart4's pinmux, a real vmmc-supply
 * regulator model, the rtc32k clock genuinely enabled, a real clock-register
 * copy-paste bug fixed) all left mmc1 timing out identically on every generic
 * rescan attempt (confirmed via live dynamic-debug tracing: real CMD5/CMD55/CMD1
 * sent at the correct 100kHz, real hardware command timeouts every time) - despite
 * every individual gpio/clock being independently confirmed correct at the live
 * register level. This strongly suggests the generic non-removable+pwrseq
 * auto-rescan path is structurally the wrong trigger for this exact combination -
 * our own gpio sequencing (wlan_pwrseq + the wifi_bt_power regulator) is already
 * confirmed electrically correct, so reuse it as-is and just add the one thing
 * nothing in our build ever did: the explicit manual-insert call stock's own
 * cywdhd.ko always makes after its own gpio sequencing. late_initcall runs well
 * after msc1's own probe (and ingenic_sdio_wlan_init(), which sets
 * wifi_data.sdio_index) has already completed, so our existing pwrseq gpios
 * should already be settled by the time this fires. */
static int __init openke_wifi_manual_insert(void)
{
	printk("WIFI_SEQ: openke_wifi_manual_insert entry\n");
	if (wifi_data.sdio_index != 1) {
		printk("openke_wifi_manual_insert: unexpected sdio_index %d, skipping\n",
		       wifi_data.sdio_index);
		return 0;
	}
	printk("openke_wifi_manual_insert: triggering manual SDIO insert on mmc%d\n",
	       wifi_data.sdio_index);
	ingenic_bcmdhd_wlan_power_onoff(MANUALLY_INSERT);
	return 0;
}
late_initcall(openke_wifi_manual_insert);
