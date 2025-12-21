# OST操作系统时钟

## 模块功能介绍

OST是操作系统时钟，分为global ost和core ost。global ost是一个64bit的计数时钟，用于操作系统计时功能。core ost是每个核有一个的32bit时钟，用于生成操作系统的时间片。

## 驱动位置

驱动源码所在位置：

***module_drives/drivers/clocksource/ingenic_core_ost.c***

## 设备树配置

设备树所在位置：

***module_drivers/dts/x2500.dtsi***

设备树描述：

core_ost: core-ost@0x12000000 {

 compatible = "ingenic,core-ost";

 reg = <0x12000000 0x10000>, /*Global ost*/

 　<0x12100000 0x10000>; /*Core ost*/

 interrupt-parent = <&cpuintc>;

 interrupt-names = "sys_ost";

 interrupts = <CORE_SYS_OST_IRQ>;

 cpu-ost-map = <0 0x000>,

 　<1 0x100>;

};

## 内核编译配置

内核默认配置此选项，开发者无需修改配置。

## 设备节点生成

global ost节点

/sys/devices/system/ clocksource

core ost节点

/sys/devices/system/clockevents

