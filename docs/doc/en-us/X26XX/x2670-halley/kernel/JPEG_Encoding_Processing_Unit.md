# JPEG encoding processing unit

## Module Function Introduction

The JPEG encoder is a complete standard-compliant JPEG/Motion JPEG hardware encoding unit.

Supported encoding input formats: ARGB, NV12, NV21, YUV422

## driver source code

Location of driver source code:

module_drivers/drivers/video/ingenic-jpeg

```
├── jpege_drv.c
├── jpege_drv.h
├── jpeg_dmabuf.c
├── jpeg_dmabuf.h
├── jpege_reg.h
├── Kconfig
└── Makefile
```

## Device tree configuration

Location of device tree:

***module_drivers/dts/x2600.dtsi***

Jpeg encoder description:

```c
jpege: jpege@0x13210000 {

    compatible = "ingenic,x2600-jpege";

    reg = <0x13210000 0x10000>;

    interrupt-parent = <&core_intc>;

    interrupts = <IRQ_JPEGE>;

    status = "disabled";

};
```

### Default configuration of device tree

The default build of the device tree will produce a jpeg encoder device:

```c
&jpege {

    status = "okay";

};
```

### Device tree custom configuration

Users can disable the jpeg encoder device according to actual needs and set this node as disabled.

```c
&jpege {

    status = "disable";

};
```

## Kernel compilation configuration

The kernel configuration for VIDEO_INGENIC_VCODEC is as follows:

The kernel configuration of Kernelx.x.x is as follows:

```
There is no help available for this option.

Symbol: JZ_JPEG [=y]

Type : boolean

Prompt: [JPEG] JZ jpeg unit

 Location:

 -> Ingenic device-drivers Configurations

 Defined at module_drivers/drivers/video/ingenic-jpeg/Kconfig:1

 Depends on: SOC_X2600 [=y]

CONFIG_INGENIC_JPEG_ENC:

Support for Ingenic jpeg-enc operations.

Symbol: INGENIC_JPEG_ENC [=y]

Type : tristate

Prompt: Ingenic jpeg encoder Driver

 Location:

 -> Ingenic device-drivers Configurations

 -> [JPEG] JZ jpeg unit (JZ_JPEG [=y])

 Defined at module_drivers/drivers/video/ingenic-jpeg/Kconfig:13

 Depends on: JZ_JPEG [=y]
```

### Default compile configuration of kernel

The kernel defaults to opening the jpeg encoder driver, and the configuration interface is as follows:

![](assets/JPEG编码处理单元.0.png)

### Kernel custom compile configuration

Users can disable the configuration of this driver according to actual needs.

## Kernel version differences

## Device Node Generation

After successful loading of the driver, the following nodes are generated:

```
/dev/jpegenc
```

jpeg encoder's device node

## Application Instructions

Using IMPP test demo: jpege_example

Parameter Description:

```
Options:
 -i                Coded source file.
 -w                Source file width.
 -h                Source file height.
 -o                output jpeg encode filename.(default file_jpegenc_out.jpg)
```

Run a test command, for example:

```
./jpege_example -i test.nv12 -w 1280 -h 720 -o output.jpg
```
