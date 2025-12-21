# INTC Interrupt Controller

## Module Function Introduction

The INTC interrupt controller controls the interrupt sources of the processor. The 64-bit interrupt source can independently control the opening and masking of each interrupt.

## Drive source code location

Location of driver source code:

***module_drives/drivers/irqchip/irq-ingenic-chip.c***

***module_drivers/drivers/irqchip/irq-ingenic-cpu.c***

## Device tree configuration

Location of device tree:

```c
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

### Default configuration of device tree

The default compilation will produce an intc device.

### Device tree custom configuration

## Kernel compilation configuration

The kernel configuration IRQ_INGENIC_CPU is configured as follows:

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

### Default compile configuration of kernel

The kernel defaults to the intc driver.

### Kernel custom compile configuration

## Device Node Generation

Generate device nodes after driver registration success.

```
/sys/devices/platform/12300000.core-intc
```

## Application Instructions

View Interrupts:

```
 \# cat proc/interrupts

CPU0 CPU1

2: 24437 33363 XBurst2 2 xburst2-intc

3: 13091 6781 XBurst2 3 jz-mailbox

4: 4826 23658 XBurst2 4 core_timerevent

8:0 0 XBurst2-irqchip 0 10020000.aic

9:30 37 XBurst2-irqchip 1 13500000.otg, 13500000.otg, dwc2_hsotg:usb1

10: 1 104 XBurst2-irqchip 2 13540000.usb, dwc2_hsotg:usb2

11: 0 0 XBurst2-irqchip 3 pdma

12: 0 0 XBurst2-irqchip 4 pdmad

15: 10433 32986 XBurst2-irqchip 7 13440000.sfc

20:0 0 XBurst2-irqchip 12 as-dmic-dma

28:0 0 XBurst2-irqchip 20 pwm-interrupt

35: 0 0 XBurst2-irqchip 27 13300000.felix

39: 13525 16 XBurst2-irqchip 31 lcdc-1

55: 392 217 XBurst2-irqchip 47 uart0

64: 11 3 XBurst2-irqchip 56 i2c3

65: 40 0 XBurst2-irqchip 57 i2c2

68: 0 0 XBurst2-irqchip 60 pdma

69: 0 0 XBurst2-irqchip 61 pdmad

70: 0 0 XBurst2-irqchip 62 jpegenc

71: 0 0 XBurst2-irqchip 63 jpegdec

72: 0 0 GPIO 14 bluetooth bthostwake

73: 0 0 GPIO 15 bootsel1

74: 4 0 GPIO 18 gt9xx

ERR: 0
```

## Interrupt Binding

Interrupt binding is to set CPU affinity for interrupts, so that interrupts only respond on specified CPU cores.

By using the echo command to write the CPU mask into the /proc/irq/interrupt ID/smp_affinity file, you can modify the affinity of a certain interrupt.

For example:

```
\# cat /proc/interrupts

CPU0 CPU1

2: 74468 87099 XBurst2 2 xburst2-intc

3: 18503 17344 XBurst2 3 jz-mailbox

4: 30174 147217 XBurst2 4 core_timerevent

8:0 0 XBurst2-irqchip 0 10020000.aic

9:30 37 XBurst2-irqchip 1 13500000.otg, 13500000.otg, dwc2_hsotg:usb1

10: 1 104 XBurst2-irqchip 2 13540000.usb, dwc2_hsotg:usb2

11: 0 0 XBurst2-irqchip 3 pdma

12: 0 0 XBurst2-irqchip 4 pdmad

15: 18340 37258 XBurst2-irqchip 7 13440000.sfc

20:0 0 XBurst2-irqchip 12 as-dmic-dma

28:0 0 XBurst2-irqchip 20 pwm-interrupt

35: 0 0 XBurst2-irqchip 27 13300000.felix

39: 55380 47342 XBurst2-irqchip 31 lcdc-1

55: 661 2355 XBurst2-irqchip 47 uart0

64: 11 3 XBurst2-irqchip 56 i2c3

65: 40 0 XBurst2-irqchip 57 i2c2

68: 0 0 XBurst2-irqchip 60 pdma

69: 0 0 XBurst2-irqchip 61 pdmad

70: 0 0 XBurst2-irqchip 62 jpegenc

71: 0 0 XBurst2-irqchip 63 jpegdec

72: 0 0 GPIO 14 bluetooth bthostwake

73: 0 0 GPIO 15 bootsel1

74: 4 0 GPIO 18 gt9xx

ERR: 0
```

For example, Interrupt 55 responds on CPU0. It can be modified to respond only on CPU1.

```
echo 2 > /proc/irq/55/smp_affinity
```

Check interrupts again

```
\# cat /proc/interrupts

CPU0 CPU1

2: 74472 87153 XBurst2 2 xburst2-intc

3: 18506 17349 XBurst2 3 jz-mailbox

4: 30179 147223 XBurst2 4 core_timerevent

8:0 0 XBurst2-irqchip 0 10020000.aic

9:30 37 XBurst2-irqchip 1 13500000.otg, 13500000.otg, dwc2_hsotg:usb1

10: 1 104 XBurst2-irqchip 2 13540000.usb, dwc2_hsotg:usb2

11: 0 0 XBurst2-irqchip 3 pdma

12: 0 0 XBurst2-irqchip 4 pdmad

15: 18340 37258 XBurst2-irqchip 7 13440000.sfc

20:0 0 XBurst2-irqchip 12 as-dmic-dma

28:0 0 XBurst2-irqchip 20 pwm-interrupt

35: 0 0 XBurst2-irqchip 27 13300000.felix

39: 55384 47342 XBurst2-irqchip 31 lcdc-1

55: 661 2409 XBurst2-irqchip 47 uart0

64: 11 3 XBurst2-irqchip 56 i2c3

65: 40 0 XBurst2-irqchip 57 i2c2

68: 0 0 XBurst2-irqchip 60 pdma

69: 0 0 XBurst2-irqchip 61 pdmad

70: 0 0 XBurst2-irqchip 62 jpegenc

71: 0 0 XBurst2-irqchip 63 jpegdec

72: 0 0 GPIO 14 bluetooth bthostwake

73: 0 0 GPIO 15 bootsel1

74: 4 0 GPIO 18 gt9xx

ERR: 0
```

Consecutive cat checks revealed that it only responds on cpu1.
