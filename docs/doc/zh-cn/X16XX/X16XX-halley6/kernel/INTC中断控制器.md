# INTC中断控制器

## 模块功能介绍

intc中断控制器控制处理器的中断源。64bit中断源可以独立控制各个中断的打开和屏蔽。

## 驱动位置

驱动源码所在位置：

***module_drivers/drivers/irqchip/irq-ingenic.c***

## 设备树配置

设备树所在位置：

kernel内核(version <= 5.10)dts文件路径：

***module_drivers/dts/x1600.dtsi***

kernel内核(version > 5.10)dts文件路径：

***module_drivers/dts/x1600/x1600.dtsi***

INTC中断控制器描述：
```c
cpuintc: interrupt-controller {

 ……

};

core_intc: core-intc@0x12300000 {

 ……

};
```

### 设备树默认配置

默认编译会产生intc设备

### 设备树自定义配置

## 内核编译配置

内核配置IRQ_INGENIC_CPU，配置说明如下：
```
Symbol: IRQ_MIPS_CPU [=y] 

Type : boolean 

 Defined at drivers/irqchip/Kconfig:93 

 Selects: GENERIC_IRQ_CHIP [=y] && IRQ_DOMAIN [=y] 

 Selected by: SOC_EMMA2RH [=n] || SOC_PNX833X [=n] || SIBYTE_SB1250 [=n] || SIBYTE_BCM1120 [=n] || 

BYTE_BCM1125 [=n] || SIBYTE_BCM1125H [=n] || SIBYTE_BCM112X [=n] || SIBYTE_BCM1x80 [=n]  
```
```
Symbol: INGENIC_INTC [=y] 

Type : boolean 

Prompt: intc v1 interrupt 

 Location: 

 -> Ingenic device-drivers Configurations 

(1) -> [Interrupt] Drivers 

 Defined at module_drivers/drivers/irqchip/Kconfig:1 

 Depends on: SOC_X1600 [=y] 

 Selects: IRQ_DOMAIN [=y] 

 Selected by: SOC_X1000 [=n] && <choice> || SOC_X1800 [=n] && <choice> || SOC_X1021 [=n] && <choice> || SOC_X1520 [=n] && <choice> || SOC_X1630 [=n] && <choice> || SOC_X1600 [=y] && <choice>
```

### 内核默认编译配置

内核默认配置intc驱动。

### 内核自定义编译配置

## 设备节点生成

驱动注册成功后生成设备节点

***/sys/devices/platform/apb/10000000.clock-controller/subsystem/devices/10001000.core-intc***

## 应用程序使用说明

查看中断：
```bash
\#cat proc/interrupts 

CPU0 

 2: 28773 MIPS 2 ingenic cascade interrupt

 4: 28014 MIPS 4 ingenic-timerost

 9: 63 INTC 1 13500000.otg, 13500000.otg, dwc2_hsotg:usb1

 11: 540 INTC 3 pdma

 12: 5 INTC 4 pdmad

 15: 27990 INTC 7 13440000.sfc

 38: 0 INTC 30 13060000.cim

 44: 39 INTC 36 13460000.msc

 53: 112 INTC 45 uart2

 69: 1 INTC 61 i2c0

 72: 0 GPIO 31 WAKEUP

 73: 0 GPIO 27 bootsel0

 74: 0 GPIO 28 bootsel1

ERR: 0
```

