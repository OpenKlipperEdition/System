# OST操作系统时钟

## 模块功能介绍

OST是操作系统时钟，分为global ost和core ost。global ost是一个64bit的计数时钟，用于操作系统计时功能。core ost是每个核有一个的32bit时钟，用于生成操作系统的时间片。

## 驱动位置

驱动源码所在位置：

***module_drivers/drivers/clocksource/***

## 设备树配置

设备树所在位置：

kernel内核(version <= 5.10)dts文件路径：

***module_driver/dts/x2600.dtsi***

kernel内核(version > 5.10)dts文件路径：

***module_driver/dts/x2600/x2600.dtsi***

设备树描述：

```c
core_ost: core-ost@0x12000000 {

    compatible = "ingenic,core-ost";

    reg = <0x12000000 0x10000>, /*Global ost*/

    <0x12100000 0x10000>; /*Core ost*/

    interrupt-parent = <&cpuintc>;

    interrupt-names = "sys_ost";

    interrupts = <CORE_SYS_OST_IRQ>;

    cpu-ost-map = <0 0x000>,

    <1 0x100>,

    <2 0x200>,

    <3 0x300>; 

};
```

### 设备树默认配置

默认编译会产生ost设备。

### 设备树自定义配置

## 内核编译配置

内核配置CLKSRC_INGENIC_CORE_OST，配置说明如下：

```
Symbol: CLKSRC_INGENIC_CORE_OST [=y]

 Type : boolean

 Prompt: Core OST clocksource. 

 Location:

 -> Ingenic device-drivers Configurations

 -> [OST] clocksoure Drivers 

 Defined at module_drivers/drivers/clocksource/Kconfig:8

 Depends on: SOC_X2000 [=n] || SOC_M300 [=n] || SOC_X2100 [=n] || SOC_X2500 [=n] || SOC_X2600 [=y] || SOC_X2580 [=n] 

 Selects: CLKSRC_OF [=y]

 Selected by: SOC_X2000 [=n] && <choice> || SOC_X2100 [=n] && <choice> || SOC_M300 [=n] && <choice> || SOC_X2500 [=n] && <choice> || SOC_X2600 [=y] && <choice> || SOC_X2580 [=n] && <choice>
```

### 内核默认编译配置

内核默认配置ost驱动

### 内核自定义编译配置

## 设备节点生成

```
/sys/devices/system/clocksource
```

global ost节点

```
/sys/devices/system/clockevents
```

core ost节点

## 应用程序使用说明
无
