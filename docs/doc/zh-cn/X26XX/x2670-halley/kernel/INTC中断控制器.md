# INTC中断控制器

## 模块功能介绍

intc中断控制器控制处理器的中断源。64bit中断源可以独立控制各个中断的打开和屏蔽。

## 驱动位置

驱动源码所在位置：

```
module_drives/drivers/irqchip/irq-ingenic-chip.c

module_drivers/drivers/irqchip/irq-ingenic-cpu.c
```

## 设备树配置

设备树所在位置：

kernel内核(version <= 5.10)dts文件路径：

***module_driver/dts/x2600.dtsi***

kernel内核(version > 5.10)dts文件路径：

***module_driver/dts/x2600/x2600.dtsi***

INTC中断控制器描述：

```
cpuintc: interrupt-controller {
        
        #address-cells = <0>;
        
        #interrupt-cells = <1>;
        
        interrupt-controller;
        
        compatible = "ingenic,cpu-interrupt-controller";

};

core_intc: core-intc@0x12300000 {

        compatible = "ingenic,core-intc";
    
        reg = <0x12300000 0x1000>, <0x10001000 0x1000>;
    
        interrupt-controller;
    
        #interrupt-cells = <1>;
    
        cpu-intc-map = <0 0x000>, <1 0x100>;

        interrupt-parent = <&cpuintc>;
    
        interrupts = <CORE_INTC_IRQ>;
    
        interrupt-names ="intc";
};
```


### 设备树默认配置

默认编译会产生intc设备

### 设备树自定义配置

## 内核编译配置

内核配置IRQ_INGENIC_CPU，配置说明如下：

```
CONFIG_IRQ_INGENIC_CPU: 

 Support for ingenic cpu core irq handler.

 Symbol: IRQ_INGENIC_CPU [=y]

 Type : boolean

 Prompt: cpu interrupt driver

 Location: 

 -> Ingenic device-drivers Configurations

 -> [Interrupt] Drivers 

 Defined at module_drivers/drivers/irqchip/Kconfig:6 

 Depends on: SOC_X2000 [=n] || SOC_X2100 [=n] || SOC_X2500 [=n] || SOC_M300 [=n] || SOC_X2580 [=n] || SOC_X2600 [=y]

 Selects: IRQ_DOMAIN [=y]

 Selected by: SOC_X2000 [=n] && <choice> || SOC_X2100 [=n] && <choice> || SOC_M300 [=n] && <choice> || SOC_X2500 [=n] && <choice> || SOC_X2600 [=y] && <choice> || SOC_X2580 [=n] && <choice> 


CONFIG_INGENIC_INTC_CHIP: 

 Support for ingenic XBURST2 based SOCs, which intc is

 near cpu core,and each logic cpu has an intc. 

 Symbol: INGENIC_INTC_CHIP [=y]

 Type : boolean

 Prompt: intc v2 interrupt driver

 Location: 

 -> Ingenic device-drivers Configurations

 -> [Interrupt] Drivers 

 -> cpu interrupt driver (IRQ_INGENIC_CPU [=y])

 Defined at module_drivers/drivers/irqchip/Kconfig:13

 Depends on: IRQ_INGENIC_CPU [=y]

 Selects: IRQ_DOMAIN [=y] 

 Selected by: SOC_X2000 [=n] && <choice> || SOC_X2100 [=n] && <choice> || SOC_M300 [=n] && <choice> || SOC_X2500 [=n] && <choice> || SOC_X2600 [=y] && <choice> || SOC_X2580 [=n] && <choice>
```

### 内核默认编译配置

内核默认配置intc驱动。

### 内核自定义编译配置

## 设备节点生成

驱动注册成功后生成设备节点

**/sys/devices/platform/12300000.core-intc**

## 应用程序使用说明

查看中断：
```
\# cat proc/interrupts 

           CPU0       CPU1       

  2:      24437      33363   XBurst2          2  xburst2-intc

  3:      13091       6781   XBurst2          3  jz-mailbox

  4:       4826      23658   XBurst2          4  core_timerevent

  8:          0          0  XBurst2-irqchip   0  10020000.aic

  9:         30         37  XBurst2-irqchip   1  13500000.otg, 13500000.otg, dwc2_hsotg:usb1

 10:          1        104  XBurst2-irqchip   2  13540000.usb, dwc2_hsotg:usb2

 11:          0          0  XBurst2-irqchip   3  pdma

 12:          0          0  XBurst2-irqchip   4  pdmad

 15:      10433      32986  XBurst2-irqchip   7  13440000.sfc

 20:          0          0  XBurst2-irqchip  12  as-dmic-dma

 28:          0          0  XBurst2-irqchip  20  pwm-interrupt

 35:          0          0  XBurst2-irqchip  27  13300000.felix

 39:      13525         16  XBurst2-irqchip  31  lcdc-1

 55:        392        217  XBurst2-irqchip  47  uart0

 64:         11          3  XBurst2-irqchip  56  i2c3

 65:         40          0  XBurst2-irqchip  57  i2c2

 68:          0          0  XBurst2-irqchip  60  pdma

 69:          0          0  XBurst2-irqchip  61  pdmad

 70:          0          0  XBurst2-irqchip  62  jpegenc

 71:          0          0  XBurst2-irqchip  63  jpegdec

 72:          0          0      GPIO         14  bluetooth bthostwake

 73:          0          0      GPIO         15  bootsel1

 74:          4          0      GPIO         18  gt9xx

ERR:          0
```

## 中断绑定

中断绑定即设置中断的CPU Affinity，让中断只在指定CPU核心上进行响应。

利用echo命令将CPU掩码写入/proc/irq /中断ID/smp_affinity文件中，即可实现修改某一中断的CPU亲和性。

例如：
```
\# cat /proc/interrupts 

           CPU0       CPU1       

  2:      74468      87099   XBurst2          2  xburst2-intc

  3:      18503      17344   XBurst2          3  jz-mailbox

  4:      30174     147217   XBurst2          4  core_timerevent

  8:          0          0  XBurst2-irqchip   0  10020000.aic

  9:         30         37  XBurst2-irqchip   1  13500000.otg, 13500000.otg, dwc2_hsotg:usb1

 10:          1        104  XBurst2-irqchip   2  13540000.usb, dwc2_hsotg:usb2

 11:          0          0  XBurst2-irqchip   3  pdma

 12:          0          0  XBurst2-irqchip   4  pdmad

 15:      18340      37258  XBurst2-irqchip   7  13440000.sfc

 20:          0          0  XBurst2-irqchip  12  as-dmic-dma

 28:          0          0  XBurst2-irqchip  20  pwm-interrupt

 35:          0          0  XBurst2-irqchip  27  13300000.felix

 39:      55380      47342  XBurst2-irqchip  31  lcdc-1

 55:        661       2355  XBurst2-irqchip  47  uart0

 64:         11          3  XBurst2-irqchip  56  i2c3

 65:         40          0  XBurst2-irqchip  57  i2c2

 68:          0          0  XBurst2-irqchip  60  pdma

 69:          0          0  XBurst2-irqchip  61  pdmad

 70:          0          0  XBurst2-irqchip  62  jpegenc

 71:          0          0  XBurst2-irqchip  63  jpegdec

 72:          0          0      GPIO         14  bluetooth bthostwake

 73:          0          0      GPIO         15  bootsel1

 74:          4          0      GPIO         18  gt9xx

ERR:          0
```
例如第55号中断在cpu0上响应，通过以下修改可以实现只在cpu1上响应。

echo 2 > /proc/irq/55/smp_affinity

再次查看interrupts

```
\# cat /proc/interrupts 

           CPU0       CPU1       

  2:      74472      87153   XBurst2          2  xburst2-intc

  3:      18506      17349   XBurst2          3  jz-mailbox

  4:      30179     147223   XBurst2          4  core_timerevent

  8:          0          0  XBurst2-irqchip   0  10020000.aic

  9:         30         37  XBurst2-irqchip   1  13500000.otg, 13500000.otg, dwc2_hsotg:usb1

 10:          1        104  XBurst2-irqchip   2  13540000.usb, dwc2_hsotg:usb2

 11:          0          0  XBurst2-irqchip   3  pdma

 12:          0          0  XBurst2-irqchip   4  pdmad

 15:      18340      37258  XBurst2-irqchip   7  13440000.sfc

 20:          0          0  XBurst2-irqchip  12  as-dmic-dma

 28:          0          0  XBurst2-irqchip  20  pwm-interrupt

 35:          0          0  XBurst2-irqchip  27  13300000.felix

 39:      55384      47342  XBurst2-irqchip  31  lcdc-1

 55:        661       2409  XBurst2-irqchip  47  uart0

 64:         11          3  XBurst2-irqchip  56  i2c3

 65:         40          0  XBurst2-irqchip  57  i2c2

 68:          0          0  XBurst2-irqchip  60  pdma

 69:          0          0  XBurst2-irqchip  61  pdmad

 70:          0          0  XBurst2-irqchip  62  jpegenc

 71:          0          0  XBurst2-irqchip  63  jpegdec

 72:          0          0      GPIO         14  bluetooth bthostwake

 73:          0          0      GPIO         15  bootsel1

 74:          4          0      GPIO         18  gt9xx

ERR:          0
```
连续cat查看发现其只在cpu1上响应。

