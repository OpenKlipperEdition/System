# RTC controller

## Module Function Introduction

The RTC (real-time clock) unit can run with chip main power on or main power off but RTC power still on. In this case, the RTC power domain consumes only a few micro-watts of power.

## Drive source code location

Location of driver source code:

***module_drivers/drivers/rtc/rtc-ingenic-v1.c***

## Device tree configuration

Location of device tree:

***module_drivers/dts/x2600.dtsi***

RTC controller description:

```c
rtc: rtc@0x10003000 {

 compatible = "ingenic,rtc";

 reg = <0x10003000 0x4000>;

 interrupt-parent = <&core_intc>;

 interrupts = <IRQ_RTC>;

 status = "ok";

};
```

### Default configuration of device tree

The default build of the device tree will not produce an RTC device.

### Device tree custom configuration

## Kernel compilation configuration

The configuration of **RTC_DRV_INGENIC** in the kernel is as follows:

```
Symbol: RTC_DRV_INGENIC_V1 [=n]

 Type : tristate

 Prompt: INGENIC RTC V1

 Location:

 -> Ingenic device-drivers Configurations

 -> [RTC] drivers

 Defined at module_drivers/drivers/rtc/Kconfig:23
```

### Default compile configuration of kernel

The kernel by default does not configure the RTC driver, and the configuration interface is as follows:

### Kernel custom compile configuration

Users can add configurations of this driver according to actual needs.

## Kernel version differences

None

## Device Node Generation

After successful loading of the driver, the following nodes are generated:

```
/dev/rtc0
```

## Application Instructions

1. Set system date and time

```
# date -s "2023-08-02 16:30"
```

2. Synchronize system time to rtc

```
# hwclock -w
```

3. Read rtc time

```
# hwclock -r
Wed Aug  2 16:30:10 2023  0.000000 seconds
```
