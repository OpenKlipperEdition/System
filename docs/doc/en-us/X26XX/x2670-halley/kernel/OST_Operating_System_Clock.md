# OST operating system clock

## Module Function Introduction

OST is the operating system clock, divided into global ost and core ost. Global ost is a 64-bit counter clock used for operating system timing functions. The core ost is a 32-bit clock that each core has and is used to generate time slices for the operating system.

## Drive source code location

Location of driver source code:

***module_drivers/drivers/clocksource/***

## Device tree configuration

Location of device tree:

***module_drivers/dts/x2600.dtsi***

Device tree description:

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

### Default configuration of device tree

The default compilation will produce ost devices.

### Device tree custom configuration

## Kernel compilation configuration

The kernel configuration **CLKSRC_INGENIC_CORE_OST** is configured as follows:

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

### Default compile configuration of kernel

The default configuration of ost driver in kernel

### Kernel custom compile configuration

## Device Node Generation

```
/sys/devices/system/clocksource
```

global ost node

```
/sys/devices/system/clockevents
```

core ost node

## Application Instructions

None
