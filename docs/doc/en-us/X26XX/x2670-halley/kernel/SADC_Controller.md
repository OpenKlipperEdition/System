# SADC controller

## Module Function Introduction

The X26xx series chip is equipped with a 12-bit ADC and has 16 analog channels, with three customizable conversion sequences. It supports multiple conversion modes such as single-shot, continuous, scanning, etc. It also features watchdog function to detect whether the input voltage exceeds the user-set threshold.

## Drive source code location

Location of driver source code:

***module_drivers/drivers/iio/adc/ingenic-adc.c***

## Device tree configuration

Location of device tree:

***module_drivers/dts/x2600.dtsi***

SADC Controller Description

```c
sadc: sadc@13650000 {

    compatible = "ingenic,x2600-sadc";

    reg = <0x13650000 0x10000>;

    interrupt-parent = <&core_intc>;

    interrupts = <IRQ_SADC>;

    interrupt-controller;

    dmas = <&pdma1 INGENIC_DMA_AHB_MCU_TYPE(INGENIC_DMA_REQ_SADC_SEQ1_RX)>,

    <&pdma1 INGENIC_DMA_AHB_MCU_TYPE(INGENIC_DMA_REQ_SADC_SEQ2_RX)>;

    dma-names = "seq1rx", "seq2rx";

    status = "disable";

};
```

### Default configuration of device tree

Default compilation will not produce SADC devices.

### Device tree custom configuration

Users can add and enable sadc in the board-level device tree according to their needs.

```c
&sadc{
        status = "disable";
        pinctrl-names = "default";
        pinctrl-0 = <&aux0_pe>, <&aux1_pe>, <&aux2_pe>, <&aux3_pe>;
        seq1 = <0 1 2 3>;
        /* seq1的连续转换间隔时间 单位 ms*/
        seq1-cont-interval = <10>;
        ingenic,has_dma_support = <0>;
};

Fill the seq1 content according to the adc channel situation at the development board.
```

## Kernel compilation configuration

Configure the kernel and compile the sadc driver:

```
Symbol: IIO [=y]

Type : tristate

Prompt: Industrial I/O support

 Location:

 -> Device Drivers

 Defined at drivers/iio/Kconfig:5

 Selects: ANON_INODES [=y]

 Selected by: RTC_DRV_HID_SENSOR_TIME [=n] && RTC_CLASS [=y] && USB_HID [=y]


Symbol: INGENIC_ADC [=y]

Type : tristate

Prompt: [X2600 SADC] X2600 sadc iio driver.

 Location:

 -> Ingenic device-drivers Configurations

 -> [iio] drivers

 Defined at module_drivers/drivers/iio/adc/Kconfig:1

 Depends on: SOC_X2600 [=y]
```

### Default compile configuration of kernel

The default compilation will not produce a sadc device.

### Kernel custom compile configuration

Users can add configurations of this driver according to actual needs.

![2023-05-29 16-07-10屏幕截图](assets/SADC控制器.0.png)

## Module kernel differences

## Device Node Generation

Generate device nodes after driver registration success

```
/dev/iio:device0
```

## Application Instructions

### Test methods

Enter the directory /sys/bus/iio/devices/iio:device0:

```
# cd /sys/bus/iio/devices/iio:device0

# ls
buffer           in_voltage2_raw  power            uevent
dev              in_voltage3_raw  scan_elements
in_voltage0_raw  name             subsystem
in_voltage1_raw  of_node          trigger
```

View channel 1 voltage value:

```
# cat in_voltage1_raw
```
