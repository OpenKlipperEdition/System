# DTRNG module driver interface

## Module Function Introduction

True Random Number Generator, which provides true random number functions in encryption and decryption scenarios.

## Drive source code location

Location of driver source code:

***module_drivers/drivers/char/hw_random/ingenic-rng.c***

## Device tree configuration

Kernel DTS file path:

***module_driver/dts/x2600.dtsi***

Device tree description:

```c
dtrng: dtrng@0x10072000 {

    compatible = "ingenic,dtrng";

    reg = <0x10072000 0x100>;

    interrupt-parent = <&core_intc>;

    interrupts = <IRQ_DTRNG>;

    status = "disabled";

};
```

### Default configuration of device tree

The default compilation will produce a dtrng device.

```c
&dtrng {

 status = "okay";

};
```

### Device tree custom configuration

If customers need to close dtrng devices, they can configure these nodes as disabled.

```c
&dtrng {

 status = "disabled";

};
```

## Kernel compilation configuration

The kernel configuration for HW_RANDOM_INGENIC is as follows:

```
Symbol: HW_RANDOM_INGENIC [=y]

Type : tristate

Prompt: Ingenic HW Random Number Generator support

 Location:

 -> Device Drivers

 -> Character devices

 -> Hardware Random Number Generator Core support (HW_RANDOM [=y])

Prompt: [RANDOM] Ingenic HW Random Number Generator support

 Location:

 -> Ingenic device-drivers Configurations

 -> [RANDOM] drivers

 Defined at drivers/char/hw_random/Kconfig:384

 Depends on: HW_RANDOM [=y] && MIPS [=y]
```

### Default compile configuration of kernel

By default, the kernel configures the dtrng driver. The configuration interface is as follows:

![](assets/DTRNG模块驱动接口.0.png)

### Kernel custom compile configuration

The customer can remove the configuration of this driver according to actual situation.

## Device Node Generation

After the driver is loaded successfully, /dev/hwrng will be generated. Users can judge whether the driver has been loaded successfully according to this node.

## Application Instructions

Test application path:

***packages/example/security_utils/dtrng/$***

```
├── app_dtrng.c
└── CMakeLists.txt
```

Test method:

```
dtrng_test out_file
```

out_file is a specified file for generating random numbers, and the generated random number data will be stored in the out_file file.

Test results:

```
# ./dtrng_test out_file

random_num register value 0x49f794bc

# ./dtrng_test out_file

random_num register value 0xcaa60aaa

# ./dtrng_test out_file

random_num register value 0x0c6a7eba

# ./dtrng_test out_file

random_num register value 0xd208541c

# ./dtrng_test out_file

random_num register value 0x63f8e65b

# ./dtrng_test out_file

random_num register value 0xaf77fdbe

# cat out_file

��w�^x_x0019_��t��r��������@�I3�6��"�D#
```
