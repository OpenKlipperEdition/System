# WDT watchdog

## Module Function Introduction

The watchdog timer is used to restore the processor when the processor is disturbed by faults such as noise and system errors. The watchdog timer can generate a reset signal. The watchdog uses the clock source provided by rtc, which can be divided into 1,4,16,64,256 and 1024 by software, and has a 16bit count register. In addition, it also supports half interrupt processing.

## Drive source code location

Location of driver source code:

***module_drivers/drivers/watchdog/ingenic_wtd.c***

## Device tree configuration

Location of device tree:

***module_drivers/dts/x2600.dtsi***

WDT Controller Description:

```c
watchdog: watchdog@0x13630000 {

    compatible = "ingenic,watchdog";

    reg = <0x13630000 0x1000>;

    interrupt-parent = <&core_intc>;

    interrupts = <IRQ_WDT>;

    status = "ok";

};
```

### Default configuration of device tree

Default compilation will produce WDT devices.

### Device tree custom configuration

Users can turn off WDT devices according to their needs and configure this node as disable.

```
&watchdog {

    status = "disable";

};
```

## Kernel compilation configuration

Kernel configuration **INGENIC_WDT**, the configuration description is as follows:

```
 Symbol: INGENIC_WDT [=y]

 Type : tristate

 Defined at module_drivers/drivers/watchdog/Kconfig

 Prompt: Ingenic ingenic SoC hardware watchdog
```

### Default compile configuration of kernel

The default configuration of WDT watchdog in the kernel is as follows:

![](assets/WDT看门狗.0.png)

### Kernel custom compile configuration

Users can remove the configuration of this driver according to their actual needs.

## Kernel version differences

None

## Device Node Generation

Generate device nodes after driver registration success:

```
/dev/watchdog0
```

## Application Instructions

In the SDK released, busybox will be configured with watchdog test application by default. The following is a description of the test application:

```
watchdog [-t N[ms]] [-T N[ms]] [-F] DEV

Periodically write to watchdog device DEV

Options:

-T N Reboot after N seconds if not reset (default 60)

-t N Reset every N seconds (default 30)

-F Run in foreground

Use 500ms to specify period in milliseconds
```

Execute the following command to test watchdog:

1. Set feeding cycle for dogs

```
watchdog -t 3 /dev/watchdog0 /*execute dog feeding operation ping every 3 seconds*/
```

2. Set a timeout restart

```
watchdog -t 5 -T 3 /dev/watchdog0 /*Reset after 3 seconds (feed dog every 5 seconds, reset if not fed for 3 seconds)*/
```

3. reboot restart

```
reboot (after executing "reboot" in command line, the system will restart immediately)
```
