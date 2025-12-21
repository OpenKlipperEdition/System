# SFC controller

## Module Function Introduction

The SFC is a spi master device that controls the spi flash. Supports CPU mode and DMA mode transfers.

* Support CDT command transmission mode
* Support DMA descriptor chain data transmission mode
* Support hardware polling flash status
* Support Standard, Dual, Quad, Octal SPI and other transmission protocols

## Drive source code location

Location of driver source code:

***module_drivers/drivers/mtd/devices/ingenic_sfc_v2/***

```
├── fmw.h
├── ingenic_sfc_common.c
├── ingenic_sfc_common.h
├── ingenic_sfc_drv.c
├── ingenic_sfc_drv.h
├── ingenic_sfc_nand.c
├── ingenic_sfc_nor.c
├── ingenic_sfc_ops.c
├── Makefile
├── nand_device
│   ├── ato_nand.c
│   ├── dosilicon_nand.c
│   ├── fm_nand.c
│   ├── foresee_nand.c
│   ├── gd_nand.c
│   ├── issi_nand.c
│   ├── Makefile
│   ├── mxic_nand.c
│   ├── nand_common.c
│   ├── nand_common.h
│   ├── tc_nand.c
│   ├── toshiba_nand.c
│   ├── winbond_nand.c   x26
│   ├── xcsp_nand.c
│   ├── xtx_mid0b_nand.c
│   ├── xtx_mid2c_nand.c
│   ├── xtx_nand.c
│   ├── yhy_nand.c
│   ├── zb_nand.c
│   └── zetta_nand.c
├── nor_device
│   ├── Makefile
│   ├── nor_device.c
│   └── nor_device.h
├── sfc_flash.h
├── sfc.h
├── spinand_cmd.h
├── spinand.h
├── spinor_cmd.h
└── spinor.h
```

## Device tree configuration

Location of device tree:

Kernel DTS file directory:

***module_drivers/dts/x2600.dtsi***

Device tree description:

```c
sfc: sfc@0x13440000 {  

    compatible = "ingenic,x2600-sfc";

    reg = <0x13440000 0x10000>;

    interrupt-parent = <&core_intc>;

    interrupts = <IRQ_SFC>;
};
```

### Default configuration of device tree

The default compilation of device tree will produce sfc devices

```c
&sfc {

    status = "okay";

    pinctrl-names = "default";

    pinctrl-0 = <&sfc_pd>;

    ingenic,sfc-max-frequency = <200000000>;

    ingenic,sfc-init-frequency= <40000000>;

    ingenic,use_ofpart_info = /bits/ 8 <0>;

    ingenic,spiflash_param_offset = <0>;

    nandflash@1 {

        partitions {

            compatible = "fixed-partitions";

            #address-cells = <1>;

            #size-cells = <1>;

            /* spi nand flash partition */

            partition@0 {

                label = "uboot";

                reg = <0x0000000 0x100000>;

                /*read-only;*/

            };

            partition@100000 {

                label = "kernel";

                reg = <0x100000 0x800000>;

            };

            partition@900000 {

                label = "rootfs";

                reg = <0x900000 0xf700000>;

            };

        };

    };

};
```

### Device tree custom configuration

You can disable the node or perform the following configurations according to your actual needs:

|  |  |
| --- | --- |
| **Property name** | **Explanation** |
| **ingenic, sfc-init-frequency** | The clock frequency set by the SFC controller in the initialization stage is lower because it has not been configured with timing parameters. |
| **ingenic, sfc-max-frequency** | To configure sfc maximum frequency, it needs to be divided by 4. The actual online clock is (sfc-max-frequency/4) MHz. |
| **ingenic, use of part info** | By default, the MTD partition is passed in by the burning tool. When the node is set to 1, the device tree partition information is used.|
| **ingenic,spiflash_param_offset** | When using spi flash to store flash parameters and partition information, it is used to modify the offset address of the parameter stored in the spi flash. When the value is 0, the default offset is 0x5800. |

## Kernel compilation configuration

The kernel configuration for INGENIC_SFC is as follows:

```
Symbol: INGENIC_SFC [=y]

Type : tristate

Prompt: Ingenic series SFC driver

Location:

-> Ingenic device-drivers Configurations

-> [SFC] (SPI Nand/Nor Flash) Drivers

Defined at module_drivers/drivers/mtd/devices/Kconfig:1

Depends on: MACH_XBURST [=n] || MACH_XBURST2 [=y]
```

## Default compile configuration of kernel

By default, the kernel configures SFC drivers as follows

```
Ingenic device-drivers Configurations  --->

[SFC] (SPI Nand/Nor Flash) Drivers  --->

NAND:

<*> Ingenic series SFC driver

Select Ingenic series SFC driver version (Use ingenic sfc driver version 2) --->

the SFC external memory (nor or nand) (Support ingenic sfc-nand) --->

[ ] ingenic SN and MAC read write support.

NOR:

<*> Ingenic series SFC driver

 Select Ingenic series SFC driver version (Use ingenic sfc driver version 2) --->

 the SFC external memory (nor or nand) (Support ingenic sfc-nor) --->

[ ] Use SPI Nor Flash params built in kernel (NEW) --->
```

### Kernel custom compile configuration

1. NAND:

NAND reserved space function, which is used to store information such as SN number, MAC address and license. The size of the reserved area can be configured, with a default single area size of 1 MB.

This feature is not enabled by default.

```
Symbol: INGENIC_SFCNAND_FMW [=y]                                

Type  : bool                                                    

Defined at module_drivers/drivers/mtd/devices/Kconfig:44        

  Prompt: ingenic SN and MAC read write support.                

  Depends on: INGENIC_SFC [=y] && MTD_INGENIC_SFC_NANDFLASH [=y]

  Location:                                                    

    -> Ingenic device-drivers Configurations                    

      -> [SFC] (SPI Nand/Nor Flash) Drivers                    

        -> Ingenic series SFC driver (INGENIC_SFC [=y])  

<*> Ingenic series SFC driver

    Select Ingenic series SFC driver version (Use ingenic sfc driver version 2)  --->

    the SFC external memory (nor or nand) (Support ingenic sfc-nand)  --->        

[*] ingenic SN and MAC read write support.                                      

(1) SN space size (MB) (NEW)

(1) MAC space size (MB) (NEW)

(1) LICENSE space size (MB) (NEW)
```

2. NOR:

Enable NOR flash parameters included in the kernel. This function will use nor flash parameters in the kernel first.

(Support multi-selection, but only one flash with the same ID can be selected)

This feature is not enabled by default.

```
Symbol: INGENIC_BUILTIN_PARAMS [=y]                                    

Type  : bool                                                  

Defined at module_drivers/drivers/mtd/devices/Kconfig:70      

  Prompt: Use SPI Nor Flash params built in kernel            

  Depends on: INGENIC_SFC [=y] && MTD_INGENIC_SFC_NORFLASH [=y]

  Location:                                                    

    -> Ingenic device-drivers Configurations                  

      -> [SFC] (SPI Nand/Nor Flash) Drivers                    

        -> Ingenic series SFC driver (INGENIC_SFC [=y])  

--- Use SPI Nor Flash params built in kernel

[ ]   GD25Q127C 0xc84018 (NEW)              

[ ]   GD25Q256C 0xc84019 (NEW)              

[ ]   GD25S512MD 0xc84019 (NEW)    
```

## Device Node Generation

After the driver is loaded successfully, the following nodes are generated:

MTD character device node:

```
/dev/mtd*
```

MTD block device node:

```
/dev/mtdblock*
```

## Application Instructions

### flash read/write test

1. Execute script path: /testsuite/StorageMedia-Test.sh

```
#./testsuite/StorageMedia-Test.sh

* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

* * Please input which module you want test.

* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

* Enter 1 : Read/Write test in the nand/nor.

* Enter 2 : Read/Write test in the sdcard.

* Enter 3 : Read/Write test in the ddr.

* Enter 4 : Read/Write test between nand/nor and sdcard.

* Enter 5 : Read/Write test between nand/nor and ddr.

* Enter 6 : Read/Write test between ddr and sdcard.
```

2. Input 1

```
You select 1, nand/nor <-> nand/nor

***************************************************

* * Please choice which Size you want copy .

***************************************************

* Enter 1 : auto random Size

* Enter 2 : setting a fix Size
```

3. Input 1 or Input 2

Choice 1: Test random size data (need to calculate size according to remaining space of flash)

Choice 2: Test data of specified size (Kbytes)

4. For example, if you input 1, the flash read and write test will start. After continuous testing for a period of time, if there is no error printing or abnormal exit, it is normal.

```
You select 1, RANDOM_SIZE

NAND_COPY_TEST: random size: 179864
random file is /bigfile_n, bs=179864
difffile nTon
diff right

difffile nTon
diff right

rm /bigfile_n
rm /bigfile_back_n
rm /tmpfile
NAND_COPY_TEST: sleep 1s for recycle::::::::::::::::::::::::::::::::::::::::cnt=1

NAND_COPY_TEST: random size: 4570199
random file is /bigfile_n, bs=4570199
difffile nTon
diff right

difffile nTon
diff right

rm /bigfile_n
rm /bigfile_back_n
rm /tmpfile
NAND_COPY_TEST: sleep 1s for recycle::::::::::::::::::::::::::::::::::::::::cnt=2

NAND_COPY_TEST: random size: 399134
random file is /bigfile_n, bs=399134
difffile nTon
diff right

difffile nTon
diff right

rm /bigfile_n
rm /bigfile_back_n
rm /tmpfile
NAND_COPY_TEST: sleep 1s for recycle::::::::::::::::::::::::::::::::::::::::cnt=3
```

## Add new flash parameters

#### Add NOR flash parameters

I. Add SPI NOR Flash parameters through cloner burning tool (recommended)

1. Open Config in Cloner, under the INFO menu, select Board as "x2660_sfc_nor_lpddr3_linux.cfg".

2. Under the SFC menu, select the secondary menu norinfo and choose ADD.

3. In the pop-up dialog box, fill in and save according to the parameters of spi nor flash.

Note: When the ID number in the burning tool is red, it means that there are multiple flash parameters with the same ID. You need to manually delete conflicting flash parameters

![](assets/SFC控制器.0.png)

II. Add NOR flash parameters in the kernel

If you need to add a custom flash, the process is as follows:

1. According to the flash model, add base params, cdt params and private params parameters. Please refer to as follows:

```
module_drivers/drivers/mtd/devices/ingenic_sfc_v2/nor_device/nor_device.c
```

2. Add config configuration, refer to as follows:

```
module_drivers/drivers/mtd/devices/Kconfig
```

#### Add NAND flash parameters

1. Add NAND flash manufacturer support

Location of NAND flash parameter configuration files for each manufacturer

```
module_drivers/drivers/mtd/devices/ingenic_sfc_v2/nand_device/
```

If there is a profile for this manufacturer, skip this step;

If it does not exist, you need to refer to the parameter configuration files of other manufacturers to write your own configuration files. For example:

```
cp gd_nand.c myflash_nand.c && vi myflash_nand.c
```

The flash parameter configuration file mainly consists of (such as gd_nand.c)

```c
/* 1. the number of flash supported models */

define GD_DEVICES_NUM 7

/* 2. basic parameters of flash */

static struct ingenic_sfcnand_base_param gd_param[GD_DEVICES_NUM] = {

[0] = { ...},

}

/* 3. associate the flash id with the parameters */

static struct device_id_struct device_id[GD_DEVICES_NUM] = {

DEVICE_ID_STRUCT(0xD1, "GD5F1GQ4UB",& gd_param[0]),

...

}

/* 4.flash obtains the default cdt parameter and updates the parameter according to the id */

static cdt_params_t *gd_get_cdt_params(struct sfc_flash *flash, uint8_t device_id) {

...

}

/* 5. to use the general get feature interface, you need to define the ecc processing interface */

static inline int deal_ecc_status(struct sfc_flash *flash, uint8_t device_id, uint8_t ecc_status) {

...

}

/* 6. Register the flash parameter */

static int __init gd_nand_init(void) {

...

}
```

2. Add nand flash model support

You need to refer to the flash manual and complete the following steps.

* 1. Add flash basic parameters.

```c
[6] = {pagesize = 2 * 1024, ... },
```

* 2. Establish association between flash and parameters

```c
DEVICE_ID_STRUCT(0xA1, "GD5F1GQ4RF", &gd_param[6]),
```

* 3. Update default parameters according to id

```c
case 0xA1:

gd_nand->cdt_params.standard_r.addr_nbyte = 3;

gd_nand->cdt_params.quad_r.addr_nbyte = 3;

break;
```

* 4. Add corresponding model processing in ECC interface handling

```c
switch(device_id) {

case 0xA1:

．．．

}
```

* 5. Add +1 to the total number of flash support

```c
#define GD_DEVICES_NUM 7
```

3. compare default parameters with flash manual and update parameters

Default command parameter file:

```
module_drivers/drivers/mtd/devices/ingenic_sfc_v2/nand_device/nand_common.h
```

Parameter interface description for production:

Control flash related:

```
CMD_INFO(_CMD, flash command, dummy bits, address length, transport protocol)
```

Read parameters in flash status.

```
ST_INFO(_ST, flash command, status offset bit, mask of status bit, expected value of status bit, transmission length, dummy bits)
```

For example:
1
|  |  |  |  |  |  |  |
| --- | --- | --- | --- | --- | --- | --- |
| **Command Name** | **Byte 1** | **Byte 2** | **Byte 3** | **Byte 4** | **Byte 5** | **Byte N** |
| **Read From Cache x 4** | 6BH | dummy (2) | A15-A8 | A7-A0 | dummy (2) | (D7-D0)x4 |

```c
/*dummy 8bit, address length 3, transmission protocol corresponding to sfc manual: TM_QI_QO_SPI*/
CMD_INFO(cdt_params.quad_r, SPINAND_CMD_RDCH_X4, 8, 2, TM_QI_QO_SPI);
```

|  |  |  |  |  |  |  |  |  |  |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Register |Addr |7|6|5|4|3|2|1|0|
|  |Status |C0H |Reserved |ECCS2 |ECCS1 |ECCS0 |P_FAIL |E_FAIL |WEL |OIP |

```c
 /*OIP offset bit is 0, status mask is 0x1, expected value is 0x0, transmission length is 1 byte, get feature dummy 0bit*/
 ST_INFO(cdt_params.oip, SPINAND_CMD_GET_FEATURE, 0, 0x1, 0x0, 1, 0);
```

The default command parameter is as follows:

```c
 /*
* cdt params
*/

#define CDT_PARAMS_INIT(cdt_params) {

/* read to cache */

CMD_INFO(cdt_params.r_to_cache, SPINAND_CMD_PARD, 0, 3, TM_STD_SPI);

/* standard read from cache */

CMD_INFO(cdt_params.standard_r, SPINAND_CMD_FRCH, 8, 2, TM_STD_SPI);

/* quad read from cache */

CMD_INFO(cdt_params.quad_r, SPINAND_CMD_RDCH_X4, 8, 2, TM_QI_QO_SPI);

/* standard write to cache */

CMD_INFO (SPINAND_CMD_PRO_LOAD, 0,2, TM_STD_SPI);

/* quad write to cache */

CMD_INFO(cdt_params.quad_w_cache, SPINAND_CMD_PRO_LOAD_X4, 0, 2, TM_QI_QO_SPI);

/* write exec */

CMD_INFO(cdt_params.w_exec, SPINAND_CMD_PRO_EN, 0,3, TM_STD_SPI);

/* block erase */

CMD_INFO(cdt_params. B _erase, SPINAND_CMD_ERASE_128K, 0,3, TM_STD_SPI);

/* write enable */

CMD_INFO(cdt_params.w_en, SPINAND_CMD_WREN, 0, 0, TM_STD_SPI);

/* get frature wait oip not busy */

ST_INFO(cdt_params.oip, SPINAND_CMD_GET_FEATURE, 0x 1, 0x 0, 1, 0);

}
```

Parameter update (such as gd_nand.c)

```c
static cdt_params_t *gd_get_cdt_params(struct sfc_flash *flash, uint8_t device_id) {

switch(device_id) {

...

case 0xA1:

gd_nand-> = 3; /* Address length updated to 3byte */

gd_nand-> = 3;/* Address length updated to 3byte */

break;

...

}
```

4. configure get_feature interface

Default location of get_feature interface:

```
module_drivers/drivers/mtd/devices/ingenic_sfc_v2/nand_device/nand_common.c
```

You can use the default get_feature interface. In special cases, you need to customize the ECC processing interface

```c
static inline int deal_ecc_status(struct sfc_flash *flash, uint8_t device_id, uint8_t ecc_status) {

    switch(device_id) {

        case 0xA1:

        case 0xB1 ... 0xB4:

        switch((ecc_status >> 4) & 0x7) {

            case 0x7:

            ret = -EBADMSG;

            break;

            case 0x6:

            ret = 0x8;

            break;

            case 0x5:

            ret = 0x7;

            break;

            default:

            ret = 0;

        }

        ...

        break;

    }

}
```

Customized get_feature interface:

1. Define your own get_feature interface in the configuration file of flash parameters
2. Registering self-defined interfaces to parameters

```c
int32_t my_get_feature(struct sfc_flash *flash, uint8_t flag) {

    ...

}

static int __init gd_nand_init(void) {

    ...

    /* use private get feature interface, please define it in this document */

    //gd_nand->ops.get_feature = NULL;

    gd_nand->ops.get_feature = my_get_feature();

    ...

}
```

## Caution

1. Nand Flash ECC solution

Use hardware ECC of nand flash based on MTD framework to build BBT in memory.

2.Currently Supported Parameter Positions

NAND: Under directory ***module_drivers/drivers/mtd/devices/ingenic_sfc_v2/nand_device/***

NOR: See the interface of the burning tool `norinfo`.

3. Kernel can use u-boot built-in parameters, please refer to uboot document for details.

It needs to rely on u-boot, and kernel uses default configuration.

4. Multi-Die SPI NOR Flash Support

spl: Multi-Die switching is not supported.

Parameters: When using flash parameters, they must be stored in Die0.

Start: Support reset flash switch to Die 0 when rebooting abnormally.
