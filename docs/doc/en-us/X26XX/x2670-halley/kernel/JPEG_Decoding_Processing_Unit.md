# JPEG encoding processing unit

## Module Function Introduction

The JPEG decoder is a complete standard-compliant JPEG/Motion JPEG hardware decoding unit.

Support decoding output format: ARGB, NV12, NV21, YUV422

## driver source code

Location of driver source code:

***module_drivers/drivers/video/ingenic-jpeg***

```
├── jpegd_drv.c
├── jpegd_drv.h
├── jpeg_dmabuf.c
├── jpeg_dmabuf.h
├── jpegd_reg.h
├── Kconfig
└── Makefile
```

## Device tree configuration

Location of device tree:

***module_drivers/dts/x2600.dtsi***

Jpeg encoder description:

```c
jpegd: jpegd@0x13200000 {

    compatible = "ingenic,x2600-jpegd";

    reg = <0x13200000 0x10000>;

    interrupt-parent = <&core_intc>;

    interrupts = <IRQ_JPEGD>;

    status = "disabled";

};
```

### Default configuration of device tree

The default build of the device tree will produce a jpeg encoder device:

```c
&jpegd {

    status = "okay";

};
```

### Device tree custom configuration

Users can disable the jpeg encoder device according to actual needs and set this node as disabled.

```c
&jpegd {

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


CONFIG_INGENIC_JPEG_DEC:

Support for Ingenic jpeg-dec operations.

Symbol: INGENIC_JPEG_DEC [=y]

Type  : tristate

Prompt: Ingenic jpeg decoder Driver

 Location:

  -> Ingenic device-drivers Configurations

  -> [JPEG] JZ jpeg unit (JZ_JPEG [=y])

  Defined at module_drivers/drivers/video/ingenic-jpeg/Kconfig:6

  Depends on: JZ_JPEG [=y]
```

### Default compile configuration of kernel

The kernel defaults to opening the jpeg decoder driver, and the configuration interface is as follows:

![](assets/JPEG编码处理单元.0.png)

### Kernel custom compile configuration

Users can disable the configuration of this driver according to actual needs.

## Kernel version differences

## Device Node Generation

After successful loading of the driver, the following nodes are generated:

```
/dev/jpegdec
```

jpeg decoder's device node

## Application Instructions

Using IMPP test demo: jpegd_example

Parameter Description:

```
jpegdec-example usage:
   -s or --pixel_size          The resolution size of the JPEG file. The format is WxH, for example: 1280x720.
   -i or --src_file            File name and path of the JPEG file.
   -o or --dst_file            File name and path of the NV12 file. The default is out_test.nv12.
```

Run a test command, for example:

```
./jpege_example -s 1280x720 -i test.jpg -o output.nv12
```
