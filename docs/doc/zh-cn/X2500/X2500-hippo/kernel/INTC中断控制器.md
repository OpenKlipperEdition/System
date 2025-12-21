# INTC中断控制器

## 模块功能介绍

intc中断控制器控制处理器的中断源。64bit中断源可以独立控制各个中断的打开和屏蔽。

## 驱动位置

驱动源码所在位置：

module_drives/drivers/irqchip/irq-ingenic-chip.c

module_drivers/drivers/irqchip/irq-ingenic-cpu.c

## 设备树配置

设备树所在位置：

***module_drivers/dts/x2500.dtsi***

INTC中断控制器描述：

***cpuintc: interrupt-controller {***

 ***……***

***};***

***core_intc: core-intc@0x12300000 {***

 ***……***

***};***

## 内核编译配置

内核配置IRQ_INGENIC_CPU，配置说明如下：

***Symbol: IRQ_INGENIC_CPU [=y]*** 

***Type : boolean*** 

***Prompt: cpu interrupt driver*** 

***Location:*** 

 ***-> Ingenic device-drivers Configurations*** 

 ***-> [Interrupt] Drivers*** 

***Defined at module_drivers/drivers/irqchip/Kconfig:6*** 

***Depends on: SOC_X2000 [=n] || SOC_X2100 [=n] || SOC_X2500 [=y] || SOC_M300 [=n]*** 

***Selects: IRQ_DOMAIN [=y]*** 

***Selected by: SOC_X2000 [=n] && <choice> || SOC_X2100 [=n] && <choice> || SOC_M300 [=n] && <choice> || SOC_X2500 [=y] && <choice>*** 

***Symbol: INGENIC_INTC_CHIP [=y]*** 

***Type : boolean*** 

***Prompt: intc v2 interrupt driver*** 

***Location:*** 

 ***-> Ingenic device-drivers Configurations*** 

 ***-> [Interrupt] Drivers*** 

 ***-> cpu interrupt driver (IRQ_INGENIC_CPU [=y])*** 

***Defined at module_drivers/drivers/irqchip/Kconfig:13*** 

***Depends on: IRQ_INGENIC_CPU [=y]*** 

***Selects: IRQ_DOMAIN [=y]*** 

***Selected by: SOC_X2000 [=n] && <choice> || SOC_X2100 [=n] && <choice> || SOC_M300 [=n] && <choice> || SOC_X2500 [=y] && <choice>*** 

## 设备节点生成

驱动注册成功后生成设备节点

***/sys/devices/platform/apb/10000000.clock-controller/subsystem/devices/12300000.core-intc***

## 应用程序使用说明

查看中断：

***cat /proc/interrupts***

***CPU0 CPU1*** 

***2: 804742 730919 XBurst2 2 xburst2-intc***

***3: 51396 90444 XBurst2 3 jz-mailbox***

***4: 158406 343976 XBurst2 4 core_timerevent***

 ***8: 0 0 XBurst2-irqchip 0 as-dma***

 ***9: 34037 24092 XBurst2-irqchip 1 13500000.otg, 13500000.otg, dwc2_hsotg:usb1***

 ***11: 0 0 XBurst2-irqchip 3 pdma***

 ***12: 0 0 XBurst2-irqchip 4 pdmad***

***14: 0 0 XBurst2-irqchip 6 pwm-interrupt***

***15: 32684 24604 XBurst2-irqchip 7 13440000.sfc***

***26: 0 0 XBurst2-irqchip 18 13810000.vic***

***27: 25 136 XBurst2-irqchip 19 13710000.vic***

***28: 0 0 XBurst2-irqchip 20 13800000.isp***

***……***

***ERR: 0***

## 中断绑定

中断绑定即设置中断的CPU Affinity，让中断只在指定CPU核心上进行响应。

利用echo命令将CPU掩码写入/proc/irq /中断ID/smp_affinity文件中，即可实现修改某一中断的CPU亲和性。

例如：

***# cat interrupts*** 

 ***CPU0 CPU1*** 

 ***2: 11613 15978 XBurst2 2 xburst2-intc***

 ***3: 13054 11075 XBurst2 3 jz-mailbox***

 ***4: 11063 8091 XBurst2 4 core_timerevent***

 ***8: 0 0 XBurst2-irqchip 0 as-dma***

 ***9: 35 42 XBurst2-irqchip 1 13500000.otg, 13500000.otg, dwc2_hsotg:usb1***

 ***11: 0 0 XBurst2-irqchip 3 pdma***

 ***12: 0 0 XBurst2-irqchip 4 pdmad***

 ***13: 1 0 XBurst2-irqchip 5 mcu***

 ***14: 0 0 XBurst2-irqchip 6 pwm-interrupt***

 ***15: 10231 14732 XBurst2-irqchip 7 13440000.sfc***

 ***26: 0 0 XBurst2-irqchip 18 13810000.vic***

 ***27: 0 0 XBurst2-irqchip 19 13710000.vic***

 ***28: 0 0 XBurst2-irqchip 20 13800000.isp***

 ***29: 0 0 XBurst2-irqchip 21 13700000.isp***

 ***30: 120 0 XBurst2-irqchip 22 13470000.hash***

 ***35: 0 0 XBurst2-irqchip 27 ingenic-tcu-interrupt***

 ***40: 0 0 XBurst2-irqchip 32 rtc 1Hz and alarm***

 ***44: 1 1204 XBurst2-irqchip 36 mmc1***

 ***52: 1204 0 XBurst2-irqchip 44 uart3***

 ***56: 0 0 XBurst2-irqchip 48 mmc2***

 ***61: 0 0 XBurst2-irqchip 53 134a0000.mac***

 ***66: 4 0 XBurst2-irqchip 58 i2c3***

 ***69: 15 0 XBurst2-irqchip 61 i2c0***

 ***70: 0 0 XBurst2-irqchip 62 13200000.helix***

 ***71: 0 0 XBurst2-irqchip 63 13300000.felix***

 ***72: 0 0 GPIO 17 vbus_dete***

 ***73: 0 0 GPIO 27 id_dete***

 ***74: 0 0 ingenic-adc 0 ingenic-aux***

 ***75: 0 0 ingenic-adc 1 ingenic-aux***

 ***76: 0 0 ingenic-adc 2 ingenic-aux***

 ***77: 0 0 ingenic-adc 3 ingenic-aux***

 ***78: 0 0 ingenic-adc 4 ingenic-aux***

 ***79: 0 0 ingenic-adc 5 ingenic-aux***

 ***80: 0 0 GPIO 21 bluetooth bthostwake***

 ***81: 0 0 GPIO 12 13490000.msc cd***

 ***82: 0 0 GPIO 31 WAKEUP***

 ***83: 0 0 GPIO 25 bootsel0***

 ***84: 0 0 GPIO 26 bootsel1***

 ***85: 2 0 GPIO 9 gt9xx***

***ERR: 0***

例如第52号中断在cpu0上响应，通过以下修改可以实现只在cpu1上响应。

***echo 2 > smp_affinity***

再次查看interrupts。

***# cat interrupts*** 

 ***CPU0 CPU1*** 

 ***2: 12160 16479 XBurst2 2 xburst2-intc***

 ***3: 13128 11240 XBurst2 3 jz-mailbox***

 ***4: 11460 8493 XBurst2 4 core_timerevent***

 ***8: 0 0 XBurst2-irqchip 0 as-dma***

 ***9: 35 42 XBurst2-irqchip 1 13500000.otg, 13500000.otg, dwc2_hsotg:usb1***

 ***11: 0 0 XBurst2-irqchip 3 pdma***

 ***12: 0 0 XBurst2-irqchip 4 pdmad***

 ***13: 1 0 XBurst2-irqchip 5 mcu***

 ***14: 0 0 XBurst2-irqchip 6 pwm-interrupt***

 ***15: 10249 14732 XBurst2-irqchip 7 13440000.sfc***

 ***26: 0 0 XBurst2-irqchip 18 13810000.vic***

 ***27: 0 0 XBurst2-irqchip 19 13710000.vic***

 ***28: 0 0 XBurst2-irqchip 20 13800000.isp***

 ***29: 0 0 XBurst2-irqchip 21 13700000.isp***

 ***30: 120 0 XBurst2-irqchip 22 13470000.hash***

 ***35: 0 0 XBurst2-irqchip 27 ingenic-tcu-interrupt***

 ***40: 0 0 XBurst2-irqchip 32 rtc 1Hz and alarm***

 ***44: 1 1204 XBurst2-irqchip 36 mmc1***

 ***52: 1733 501 XBurst2-irqchip 44 uart3***

 ***56: 0 0 XBurst2-irqchip 48 mmc2***

 ***61: 0 0 XBurst2-irqchip 53 134a0000.mac***

 ***66: 4 0 XBurst2-irqchip 58 i2c3***

 ***69: 15 0 XBurst2-irqchip 61 i2c0***

 ***70: 0 0 XBurst2-irqchip 62 13200000.helix***

 ***71: 0 0 XBurst2-irqchip 63 13300000.felix***

 ***72: 0 0 GPIO 17 vbus_dete***

 ***73: 0 0 GPIO 27 id_dete***

 ***74: 0 0 ingenic-adc 0 ingenic-aux***

 ***75: 0 0 ingenic-adc 1 ingenic-aux***

 ***76: 0 0 ingenic-adc 2 ingenic-aux***

 ***77: 0 0 ingenic-adc 3 ingenic-aux***

 ***78: 0 0 ingenic-adc 4 ingenic-aux***

 ***79: 0 0 ingenic-adc 5 ingenic-aux***

 ***80: 0 0 GPIO 21 bluetooth bthostwake***

 ***81: 0 0 GPIO 12 13490000.msc cd***

 ***82: 0 0 GPIO 31 WAKEUP***

 ***83: 0 0 GPIO 25 bootsel0***

 ***84: 0 0 GPIO 26 bootsel1***

 ***85: 2 0 GPIO 9 gt9xx***

***ERR: 0***

连续cat查看发现其只在cpu1上响应。

