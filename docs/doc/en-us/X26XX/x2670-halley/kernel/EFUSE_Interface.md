# EFUSE interface

## Module Function Introduction

The EFUSE module provides interfaces for reading and writing data segments of efuses. You can view information about CPUs saved in efuses or write user-defined information.

## Drive source code location

Location of driver source code

***module_drivers/drivers/misc/ingenic_efuse_x2000.c***

**Note: The controller registers of x2600 and x2000 are consistent, using the same set of driver codes.**

## Device tree configuration

Location of device tree:

***module_drivers/dts/x2600.dtsi***

```c
efuse: efuse@0x13480000 {

    compatible = "ingenic,x2600-efuse";

    reg = <0x13480000 0x10000>;

    status = "okay";

};
```

### Default configuration of device tree

The default build of the device tree will not produce an EFUSE device.

### Device tree custom configuration

Users can enable efuse devices according to actual needs.

```c
&efuse {

    status = "okay";

    ingenic,efuse-en-gpio = <&gpe 6 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;

};
```

**Attention: The x26xx series development boards only have efuse-en-gpio in x2660 halley v1.0 and v1.1, while all other development boards are directly powered without configuring efuse-en-gpio**

## Kernel compilation configuration

Kernel configuration INGENIC_EFUSE_X2000, the configuration description is as follows:

**Note: The controller registers of x2600 and x2000 are consistent, using the same set of driver codes.**

```
 Symbol: INGENIC_EFUSE_X2000 [=y]

 Type : boolean

 Prompt: [Efuse] Ingenic Efuse X2000 Driver

 Location:

 -> Ingenic device-drivers Configurations

 Defined at module_drivers/drivers/misc/Kconfig:35

 Depends on: SOC_X2000 [=n] || SOC_X2100 [=n] || SOC_M300 [=n] || SOC_X2600 [=y]
```

### Default compile configuration of kernel

The configuration interface is as follows:

![](assets/EFUSE_接口.0.png)

### Kernel custom compile configuration

Users can add this module according to their actual needs. This module is not configured by default.

## Device Node Generation

After successful loading of the driver, the following files will be generated in the directory `/sys/devices/platform/ahb2/13480000.efuse/efuse_rw/`:

```
chipid    chipkey    custid0    custid1    custid2

hideblk   nku        prt        socinfo    trim0

trim1     trim2      userkey0   userkey1
```

Each generated file corresponds to a segment of efuse. For specific functions of each segment, please refer to the pm manual.

## Application Instructions

### Read data from each segment of efuse

```
cd /sys/devices/platform/ahb2/13480000.efuse/efuse_rw/

cat chipid

cat ......

cat socinfo
```

Among them, userkey, chidkey and nku are related to encryption and cannot be read in this way.

### Write data to each segment of efuse

When writing efuse, data should be written in hexadecimal string format without a 0x prefix. For example, write data 0x2080020800 to efuse.

Execute command

```
cd /sys/devices/platform/ahb2/13480000.efuse/efuse_rw/

echo 2080020800 > socinfo
```

Among them, userkey, chidkey and nku are related to encryption and cannot be read in this way.

Caution

Efuses can only be written once, and repeated writing may cause unpredictable errors in chips. Be careful when writing.
