# 1. Introduction to u-boot

## 1.1 Functions of u-boot

U-Boot is an open source boot program for embedded platforms. It can boot a variety of embedded operating systems including linux kernel, supports a variety of processor architectures such as mips, arm, powerpc, etc. It provides a variety of download methods such as serial port and Ethernet, provides NOR and NAND flash memory, environment variable management and other functions, and supports network protocol stack, JFFS2/EXT2/FAT file system, it also supports a variety of device drivers such as MMC/SD card, USB devices, LCD drivers, etc.

**Note**: In this text, "Nor" refers specifically to SPI Nor Flash, and "Nand" refers to SPI Nand Flash unless otherwise specified.

## 1.2 u-boot Boot Process

The boot process of u-boot on the X26xx Halley platform is illustrated below:

![](assets/x26xx-uboot.1.png) As seen in the diagram, there are two boot sequences:

1. **Bootrom -> SPL -> U-Boot -> Kernel -> RootFS**: This sequence requires U-Boot to load the kernel.
2. **Bootrom -> SPL -> Kernel -> RootFS**: In this sequence, U-Boot is not needed; the SPL directly loads the kernel.

## 1.3 u-boot Source Code Directory Introduction

|  |  |
| --- | --- |
| api | Contains the interface functions provided by uboot |
| arch | Architecture-related files |
| board | Board-related files |
| common | Common code, mainly various commands |
| disk | Disk partition related code |
| doc | Introduction documents |
| drivers | Driver-related code |
| examples | Example programs |
| fs | File system code, supporting common embedded file systems |
| include | Header files |
| lib | Common library files |
| nand_spl | NAND flash boot-related code |
| net | Network-related code |
| post | Power On Self Test, power-on self-test program |
| spl | SPL-related code |
| test | Test code |
| tools | Assistive tools for compiling and checking uboot target files |

# 2. u-boot Configuration

For the current X26xx Halley platform, the main files determining the u-boot configuration are:

1. **u-boot/boards.cfg**
2. **u-boot/include/configs/x2600_halley.h**
3. **u-boot/include/configs/x2600e_halley.h**
4. **u-boot/include/configs/x2670_halley.h**
5. **u-boot/include/configs/x2670m_hare.h**

Two files work together to determine the final configuration of the generated burning image.

## 2.1 boards.cfg file description

boards.cfg is the most basic configuration file in u-boot, used to distinguish different board configurations from different manufacturers. Each line in this file represents a specific board configuration, and the format is the same, as shown in the following table:

|  |  |  |  |  |  |  |
| --- | --- | --- | --- | --- | --- | --- |
| Target | ARCH | CPU | Board name | Vendor | SoC | Options |
| Target name | Architecture name | Processor name | Board name | Vendor name | Soc name | Additional options |

The explanation of each configuration item is as follows:

1. Target: As a flag for this type of configuration.
2. ARCH, CPU, Board name, Vendor, SoC: Mainly used for selecting code directories during Makefile compilation, such as many configuration files in u-boot/include/configs directory. Selecting x26xx_halley.h is determined by Board name.
3. Options: The main difference between each configuration. u-boot will add the "CONFIG_" prefix before each field in the additional options and export it as a global available macro definition. These macro definitions will further determine other configurations, currently mainly those in x26xx_halley.h.

### 2.1.1 default configuration

|  |  |
| --- | --- |
| x2660_halley_uImage_sfc_nand | **This configuration supports booting the Linux kernel image uImage from nand flash** CONFIG_SPL_SFC_NAND: It includes SFC NAND flash related configurations, CONFIG_MTD_SFCNAND: SFC NAND flash driver compilation configurations,  CONFIG_SPL_PARAMS_FIXER: CPM frequency changing configurations. |
| x2660_halley_uImage_sfc_nor | **This configuration supports booting the Linux kernel image uImage from nor flash** CONFIG_SPL_SFC_NOR: It includes SFC NOR flash related configurations, CONFIG_ENV_IS_IN_SFC: environment variable storage related configurations, CONFIG_MTD_SFCNOR: SFC NOR flash driver compilation configurations, CONFIG_SPL_PARAMS_FIXER: CPM frequency changing configurations. |
| x2660_halley_xImage_sfc_nand_ota | **This configuration supports booting the Linux kernel image xImage from nand flash with OTA upgrade** CONFIG_SPL_SFC_NAND: It includes SFC NAND flash related configurations, CONFIG_MTD_SFCNAND: SFC NAND flash driver compilation configurations, CONFIG_SPL_OS_BOOT: SPL direct boot configurations for Linux kernel image, CONFIG_OTA_VERSION30: OTA upgrade related configurations. |
| x2660_halley_xImage_sfc_nand | **This configuration supports booting the Linux kernel image xImage from nand flash** CONFIG_SPL_SFC_NAND: It includes SFC NAND flash related configurations, CONFIG_MTD_SFCNAND: SFC NAND flash driver compilation configurations, CONFIG_SPL_OS_BOOT: SPL direct boot configurations for Linux kernel image, CONFIG_SPL_PARAMS_FIXER: CPM frequency changing configurations. |
| x2660_halley_xImage_sfc_nor | **This configuration supports booting the Linux kernel image xImage from nor flash** CONFIG_SPL_SFC_NOR: It includes SFC NOR flash related configurations, CONFIG_MTD_SFCNOR: SFC NOR flash driver compilation configurations, CONFIG_SPL_OS_BOOT: SPL direct boot configurations for Linux kernel image, CONFIG_ENV_IS_IN_SFC: environment variable storage related configurations, CONFIG_SPL_PARAMS_FIXER: CPM frequency changing configurations. |
| x2670_halley_uImage_sfc_nand | **This configuration supports booting the Linux kernel image uImage from nand flash** CONFIG_SPL_SFC_NAND: It includes SFC NAND flash related configurations, CONFIG_MTD_SFCNAND: SFC NAND flash driver compilation configurations, CONFIG_SPL_PARAMS_FIXER: CPM frequency changing configurations. |
| x2670_halley_uImage_sfc_nor | **This configuration supports booting the Linux kernel image uImage from nor flash** CONFIG_SPL_SFC_NOR: It includes SFC NOR flash related configurations, CONFIG_ENV_IS_IN_SFC: environment variable storage related configurations, CONFIG_MTD_SFCNOR: SFC NOR flash driver compilation configurations, CONFIG_SPL_PARAMS_FIXER: CPM frequency changing configurations. |
| x2670_halley_xImage_sfc_nand_ota | **This configuration supports booting the Linux kernel image xImage from nand flash with OTA upgrade** CONFIG_SPL_SFC_NAND: It includes SFC NAND flash related configurations, CONFIG_MTD_SFCNAND: SFC NAND flash driver compilation configurations, CONFIG_SPL_OS_BOOT: SPL direct boot configurations for Linux kernel image, CONFIG_OTA_VERSION30: OTA upgrade related configurations. |
| x2670_halley_xImage_sfc_nand | **This configuration supports booting the Linux kernel image xImage from nand flash** CONFIG_SPL_SFC_NAND: It includes SFC NAND flash related configurations, CONFIG_MTD_SFCNAND: SFC NAND flash driver compilation configurations, CONFIG_SPL_OS_BOOT: SPL direct boot configurations for Linux kernel image, CONFIG_SPL_PARAMS_FIXER: CPM frequency changing configurations. |
| x2670_halley_xImage_sfc_nor | **This configuration supports booting the Linux kernel image xImage from nor flash** CONFIG_SPL_SFC_NOR: It includes SFC NOR flash related configurations, CONFIG_MTD_SFCNOR: SFC NOR flash driver compilation configurations, CONFIG_SPL_OS_BOOT: SPL direct boot configurations for Linux kernel image, CONFIG_ENV_IS_IN_SFC: environment variable storage related configurations, CONFIG_SPL_PARAMS_FIXER: CPM frequency changing configurations. |
| x2670m_hare_uImage_sfc_nand | **This configuration supports booting the Linux kernel image uImage from nand flash** CONFIG_SPL_SFC_NAND: It includes SFC NAND flash related configurations, CONFIG_MTD_SFCNAND: SFC NAND flash driver compilation configurations, CONFIG_SPL_PARAMS_FIXER: CPM frequency changing configurations. |
| x2670m_hare_uImage_sfc_nor | **This configuration supports booting the Linux kernel image uImage from nor flash** CONFIG_SPL_SFC_NOR: It includes SFC NOR flash related configurations, CONFIG_ENV_IS_IN_SFC: environment variable storage related configurations, CONFIG_MTD_SFCNOR: SFC NOR flash driver compilation configurations, CONFIG_SPL_PARAMS_FIXER: CPM frequency changing configurations. |
| x2670m_hare_xImage_sfc_nand_ota | **This configuration supports booting the Linux kernel image xImage from nand flash with OTA upgrade** CONFIG_SPL_SFC_NAND: It includes SFC NAND flash related configurations, CONFIG_MTD_SFCNAND: SFC NAND flash driver compilation configurations, CONFIG_SPL_OS_BOOT: SPL direct boot configurations for Linux kernel image, CONFIG_OTA_VERSION30: OTA upgrade related configurations. |
| x2670m_hare_xImage_sfc_nand | **This configuration supports booting the Linux kernel image xImage from nand flash** CONFIG_SPL_SFC_NAND: It includes SFC NAND flash related configurations, CONFIG_MTD_SFCNAND: SFC NAND flash driver compilation configurations, CONFIG_SPL_OS_BOOT: SPL direct boot configurations for Linux kernel image, CONFIG_SPL_PARAMS_FIXER: CPM frequency changing configurations. |
| x2670m_hare_xImage_sfc_nor | **This configuration supports booting the Linux kernel image xImage from nor flash** CONFIG_SPL_SFC_NOR: It includes SFC NOR flash related configurations, CONFIG_MTD_SFCNOR: SFC NOR flash driver compilation configurations, CONFIG_SPL_OS_BOOT: SPL direct boot configurations for Linux kernel image, CONFIG_ENV_IS_IN_SFC: environment variable storage related configurations, CONFIG_SPL_PARAMS_FIXER: CPM frequency changing configurations. |
| x2600_halley_uImage_sfc_nand | **This configuration supports booting the Linux kernel image uImage from nand flash** CONFIG_SPL_SFC_NAND: It includes SFC NAND flash related configurations, CONFIG_MTD_SFCNAND: SFC NAND flash driver compilation configurations, CONFIG_SPL_PARAMS_FIXER: CPM frequency changing configurations. |
| x2600_halley_uImage_sfc_nor | **This configuration supports booting the Linux kernel image uImage from nor flash** CONFIG_SPL_SFC_NOR: It includes SFC NOR flash related configurations, CONFIG_ENV_IS_IN_SFC: environment variable storage related configurations, CONFIG_MTD_SFCNOR: SFC NOR flash driver compilation configurations, CONFIG_SPL_PARAMS_FIXER: CPM frequency changing configurations. |
| x2600_halley_uImage_msc0 | **This configuration supports booting the Linux kernel image uImage from emmc** CONFIG_SPL_JZMMC_SUPPORT: It includes SPL MMC support configurations, CONFIG_ENV_IS_IN_MMC: environment variable storage related configurations, CONFIG_GPT_CREATOR: MMC partition creator configurations, CONFIG_JZ_MMC_MSC0: JZ MMC MSC0 controller configurations, CONFIG_SPL_PARAMS_FIXER: CPM frequency changing configurations. |
| x2600_halley_xImage_sfc_nand_ota | **This configuration supports booting the Linux kernel image xImage from nand flash with OTA upgrade** CONFIG_SPL_SFC_NAND: It includes SFC NAND flash related configurations, CONFIG_MTD_SFCNAND: SFC NAND flash driver compilation configurations, CONFIG_SPL_OS_BOOT: SPL direct boot configurations for Linux kernel image, CONFIG_OTA_VERSION30: OTA upgrade related configurations. |
| x2600_halley_xImage_sfc_nand | **This configuration supports booting the Linux kernel image xImage from nand flash** CONFIG_SPL_SFC_NAND: It includes SFC NAND flash related configurations, CONFIG_MTD_SFCNAND: SFC NAND flash driver compilation configurations, CONFIG_SPL_OS_BOOT: SPL direct boot configurations for Linux kernel image, CONFIG_SPL_PARAMS_FIXER: CPM frequency changing configurations. |
| x2600_halley_xImage_sfc_nor | **This configuration supports booting the Linux kernel image xImage from nor flash** CONFIG_SPL_SFC_NOR: It includes SFC NOR flash related configurations, CONFIG_MTD_SFCNOR: SFC NOR flash driver compilation configurations, CONFIG_SPL_OS_BOOT: SPL direct boot configurations for Linux kernel image, CONFIG_ENV_IS_IN_SFC: environment variable storage related configurations, CONFIG_SPL_PARAMS_FIXER: CPM frequency changing configurations. |
| x2600_halley_xImage_msc0 | **This configuration supports booting the Linux kernel image xImage from emmc** CONFIG_SPL_JZMMC_SUPPOR: It includes SPL MMC support configurations, CONFIG_ENV_IS_IN_MMC: environment variable storage related configurations, CONFIG_GPT_CREATOR: MMC partition creator configurations, CONFIG_JZ_MMC_MSC0: JZ MMC MSC0 controller configurations, CONFIG_SPL_PARAMS_FIXER: CPM frequency changing configurations, CONFIG_SPL_OS_BOOT:  SPL direct boot configurations. |
| x2600e_halley_uImage_sfc_nand | **This configuration supports booting the Linux kernel image uImage from nand flash** CONFIG_SPL_SFC_NAND: It includes SFC NAND flash related configurations, CONFIG_MTD_SFCNAND: SFC NAND flash driver compilation configurations, CONFIG_SPL_PARAMS_FIXER: CPM frequency changing configurations. |
| x2600e_halley_uImage_sfc_nor | **This configuration supports booting the Linux kernel image uImage from nor flash** CONFIG_SPL_SFC_NOR: It includes SFC NOR flash related configurations, CONFIG_ENV_IS_IN_SFC: environment variable storage related configurations, CONFIG_MTD_SFCNOR: SFC NOR flash driver compilation configurations, CONFIG_SPL_PARAMS_FIXER: CPM frequency changing configurations. |
| x2600e_halley_uImage_msc0 | **This configuration supports booting the Linux kernel image uImage from emmc** CONFIG_SPL_JZMMC_SUPPORT: It includes SPL MMC support configurations, CONFIG_ENV_IS_IN_MMC: environment variable storage related configurations, CONFIG_GPT_CREATOR: MMC partition creator configurations, CONFIG_JZ_MMC_MSC0: JZ MMC MSC0 controller configurations, CONFIG_SPL_PARAMS_FIXER: CPM frequency changing configurations. |
| x2600e_halley_xImage_sfc_nand_ota | **This configuration supports booting the Linux kernel image xImage from nand flash with OTA upgrade** CONFIG_SPL_SFC_NAND: It includes SFC NAND flash related configurations, CONFIG_MTD_SFCNAND: SFC NAND flash driver compilation configurations, CONFIG_SPL_OS_BOOT: SPL direct boot configurations for Linux kernel image, CONFIG_OTA_VERSION30: OTA upgrade related configurations. |
| x2600e_halley_xImage_sfc_nand | **This configuration supports booting the Linux kernel image xImage from nand flash** CONFIG_SPL_SFC_NAND: It includes SFC NAND flash related configurations, CONFIG_MTD_SFCNAND: SFC NAND flash driver compilation configurations, CONFIG_SPL_OS_BOOT: SPL direct boot configurations for Linux kernel image, CONFIG_SPL_PARAMS_FIXER: CPM frequency changing configurations. |
| x2600e_halley_xImage_sfc_nor | **This configuration supports booting the Linux kernel image xImage from nor flash** CONFIG_SPL_SFC_NOR: It includes SFC NOR flash related configurations, CONFIG_MTD_SFCNOR: SFC NOR flash driver compilation configurations, CONFIG_SPL_OS_BOOT: SPL direct boot configurations for Linux kernel image, CONFIG_ENV_IS_IN_SFC: environment variable storage related configurations, CONFIG_SPL_PARAMS_FIXER: CPM frequency changing configurations. |
| x2600e_halley_xImage_msc0 | **This configuration supports booting the Linux kernel image xImage from emmc** CONFIG_SPL_JZMMC_SUPPOR: It includes SPL MMC support configurations, CONFIG_ENV_IS_IN_MMC: environment variable storage related configurations, CONFIG_GPT_CREATOR: MMC partition creator configurations, CONFIG_JZ_MMC_MSC0: JZ MMC MSC0 controller configurations, CONFIG_SPL_PARAMS_FIXER: CPM frequency changing configurations, CONFIG_SPL_OS_BOOT:  SPL direct boot configurations. |

### 2.1.2 Custom Configuration

You can add or remove fields in each board-level configuration or even add a new one based on actual needs.

Below is an example of adding a new board-level configuration named **X2660_halley_uImage_msc0**:

1. Start a new line in the boards.cfg file, and according to the format requirements of this file, determine the first configuration item to be X2660_halley_uImage_msc0. The second to sixth configuration items can refer to existing board-level configurations, which are generally quite fixed;
2. The main focus is on the configuration of additional options. If you are not very familiar with the overall configuration of u-boot for the X26xx Halley platform, you can refer to existing configurations. For example, you can reference halley5_uImage_msc2 here, changing JZ_MMC_MSC2 to JZ_MMC_MSC0 to select the configuration related to MSC0, and keep the rest consistent;
3. If you want to modify the additional options more meticulously, it is recommended to read through this document thoroughly, especially the introduction to configurations in chapter two. After understanding the overall configuration of u-boot for the X26xx Halley platform, you will find it easier to make modifications.

## 2.2 x26xx_halley.h Configuration File Explanation

The x26xx_halley.h file is the configuration file for the core of the X26xx Halley platform within u-boot, primarily including the following content: 

* system clock configuration
* cache size configuration
* DDR configuration
* SFC/MSC/GMAC/LCD driver-related configurations
* built-in u-boot commands

### 2.2.1 System Clock Configuration

#### 2.2.1.1 default configuration

The current system clock configuration is as follows：
```c
#define CONFIG_SYS_APLL_FREQ 1200000000 /*If APLL not use mast be set 0*/
#define CONFIG_SYS_MPLL_FREQ 1800000000 /*If MPLL not use mast be set 0*/
#define CONFIG_SYS_EPLL_FREQ 300000000 /*If EPLL not use mast be set 0*/
#define CONFIG_CPU_SEL_PLL APLL
#define CONFIG_DDR_SEL_PLL MPLL
#define CONFIG_SYS_CPU_FREQ 1200000000
#define CONFIG_SYS_MEM_FREQ 900000000
#define CONFIG_SYS_AHB0_FREQ 300000000
#define CONFIG_SYS_AHB2_FREQ 300000000 /*APB = AHB2/2*/

```

#### 2.2.1.2 Custom configuration

1. PLL frequency configuration, which can be customized according to specific requirements.
```c
#define CONFIG_SYS_APLL_FREQ 1200000000 /*If APLL not use mast be set 0*/
#define CONFIG_SYS_MPLL_FREQ 1800000000 /*If MPLL not use mast be set 0*/
#define CONFIG_SYS_EPLL_FREQ 300000000 /*If EPLL not use mast be set 0*/
```
1. Selection of PLL for CPU and DDR is recommended to use default configurations.
```c
#define CONFIG_CPU_SEL_PLL APLL
#define CONFIG_DDR_SEL_PLL MPLL
```

1. Configuration of CPU and DDR frequencies, where the CPU frequency needs to be a multiple of the selected PLL frequency, with a maximum supported speed of 1.5GHz. Similarly, the DDR frequency must also be a multiple of the chosen PLL frequency.

```c
#define CONFIG_SYS_CPU_FREQ 1200000000
#define CONFIG_SYS_MEM_FREQ 900000000
```

1. Configuration of AHB frequency.
```c
#define CONFIG_SYS_AHB0_FREQ 300000000
#define CONFIG_SYS_AHB2_FREQ 300000000 /*APB = AHB2/2*/
```

1. Note: After making the modifications, it is necessary to first clean the uboot image, followed by recompiling it. For the cleaning procedure, refer to the corresponding content in sections 3.1 and 3.2.

### 2.2.2 DDR type configuration

#### 2.2.2.1 Default configuration

The current default DDR parameters are as follows:

```c
#define CONFIG_DDR_INNOPHY
#define CONFIG_DDR_PARAMS_CREATOR
#define CONFIG_DDR_HOST_CC
#define CONFIG_DDR_TYPE_DDR3
/* #define CONFIG_DDR_TYPE_LPDDR3 */
/* #define CONFIG_DDR_TYPE_LPDDR2 */
#define CONFIG_DDR_CS0 1 /* 1-connected, 0-disconnected */
#define CONFIG_DDR_CS1 0 /* 1-connected, 0-disconnected */
#define CONFIG_DDR_DW32 0 /* 1-32bit-width, 0-16bit-width */
/*#define CONFIG_DDR3_TSD34096M1333C9_E*/

#ifdef CONFIG_DDR_TYPE_LPDDR2
#define CONFIG_LPDDR2_FMT4D32UAB_25LI_FPGA
/* #define CONFIG_LPDDR2_AD210032F_AB_FPGA */
#endif

#ifdef CONFIG_DDR_TYPE_DDR3
/* #define CONFIG_DDR3_TSD34096M1333C9_E_FPG */
#define CONFIG_DDR3_W631GU6NG
#endif

#ifdef CONFIG_DDR_TYPE_LPDDR3
#define CONFIG_LPDDR3_MT52L256M32D1PF_FPGA
/* #define CONFIG_LPDDR3_AD310032C_AB_FPGA */
/* #define CONFIG_LPDDR3_W63AH6NBVABI_FPGA *//* size = 128M */
#endif

#define CONFIG_OPEN_KGD_DRIVER_STRENGTH
#ifdef CONFIG_OPEN_KGD_DRIVER_STRENGTH
#define CONFIG_DDR_DRIVER_OUT_STRENGTH
#define CONFIG_DDR_DRIVER_OUT_STRENGTH_1 0
#define CONFIG_DDR_DRIVER_OUT_STRENGTH_0 1
#endif

#define CONFIG_DDR_CHIP_ODT
#define CONFIG_DDR_CHIP_ODT_VAL
#ifdef CONFIG_DDR_CHIP_ODT_VAL

#define CONFIG_DDR_CHIP_ODT_VAL_RTT_NOM_9 0 /* RTT_Nom_9 is MR1 A9 bit */

#define CONFIG_DDR_CHIP_ODT_VAL_RTT_NOM_6 0 /* RTT_Nom_6 is MR1 A6 bit */

#define CONFIG_DDR_CHIP_ODT_VAL_RTT_NOM_2 1 /* RTT_Nom_2 is MR1 A2 bit */

#define CONFIG_DDR_CHIP_ODT_VAL_RTT_WR 0 /* RTT_WR is odt for KGD write of MR2*/

#endif

#define CONFIG_DDR_PHY_IMPEDANCE 40
#define CONFIG_DDR_PHY_ODT_IMPEDANCE 120

/* #define CONFIG_FPGA_TEST */

/*#define CONFIG_DDR_AUTO_REFRESH_TEST*/

#define CONFIG_DDR_AUTO_SELF_REFRESH
#define CONFIG_DDR_AUTO_SELF_REFRESH_CNT 257
```

### 2.2.3 SFC driver configuration

#### 2.2.3.1 Driver for SPL stage

Location of driver files：common/spl

```
├── spl_sfc_nand.c
├── spl_sfc_nand_v2.c
├── spl_sfc_nor.c
├── spl_sfc_nor_v2.c
```

#### 2.2.3.2 U-Boot stage driver files

**Driver location： drivers/mtd/devices/jz_sfc_v2**

```
├── jz_sfc_common.c 
├── jz_sfc_common.h 
├── jz_sfc_nand.c
├── jz_sfc_nor.c
├── jz_sfc_ops.c
├── Makefile
└── nand_device
    ├── ato_nand.c
    ├── dosilicon_nand.c
    ├── foresee_nand.c
    ├── gd_nand.c
    ├── mxic_nand.c
    ├── nand_common.c
    ├── nand_common.h
    ├── xtx_mid0b_nand.c
    ├── xtx_nand.c
    └── zetta_nand.c
```

#### 2.2.3.3 Configuration Instructions

1. GPIO pin function configuration for the SFC controller

```c
/* sfc gpio */
/* #define CONFIG_JZ_SFC_PD_4BIT */
/* #define CONFIG_JZ_SFC_PD_8BIT */
/* #define CONFIG_JZ_SFC_PD_8BIT_PULL */
#define CONFIG_JZ_SFC_PE
```

2. SFC NAND flash configuration. If **CONFIG_SPL_SFC_NAND** is defined, then the configurations related to NAND will be enabled:

```c
#define CONFIG_SFC_NAND_RATE 200000000 /* value <= 400000000(sfc 100Mhz)*/
#define CONFIG_SFC_QUAD
#define CONFIG_SPI_SPL_CHECK
#define CONFIG_SPIFLASH_PART_OFFSET 0x5800
#define CONFIG_SPI_NAND_BPP (2048 +64) /*Bytes Per Page*/
``````c
#define CONFIG_SPI_NAND_PPB (64) /*Page Per Block*/
#define CONFIG_JZ_SFC
#define CONFIG_CMD_SFCNAND
#define CONFIG_CMD_NAND
#define CONFIG_SYS_MAX_NAND_DEVICE 1
#define CONFIG_SYS_NAND_BASE 0xb3441000
#define CONFIG_SYS_MAXARGS 16

/*#define CONFIG_NAND_BUILTIN_PARAMS*/

/* sfc nand env config */
#define CONFIG_MTD_DEVICE
#define CONFIG_CMD_SAVEENV /* saveenv */
#define CONFIG_CMD_UBI
#define CONFIG_CMD_UBIFS
#define CONFIG_CMD_MTDPARTS
#define CONFIG_MTD_PARTITIONS
#define MTDIDS_DEFAULT "nand0:nand"
#define MTDPARTS_DEFAULT "mtdparts=nand:1M(boot),8M(kernel),40M(rootfs),- (data)"
#define CONFIG_SYS_NAND_BLOCK_SIZE (128 * 1024)
```

3. SFC NOR flash configuration. If **CONFIG_SPL_SFC_NOR** is defined, then the configurations related to NOR will be enabled：

```c
#ifdef CONFIG_SPL_SFC_NOR
#define CONFIG_JZ_SFC
#define CONFIG_CMD_SFC_NOR
#define CONFIG_JZ_SFC_NOR
#define CONFIG_SPI_SPL_CHECK
#define CONFIG_SFC_NOR_RATE 200000000 /* value <= 400000000(sfc 100Mhz)*/
#define CONFIG_SFC_QUAD
#define CONFIG_SPIFLASH_PART_OFFSET 0x5800
#define CONFIG_SPI_NORFLASH_PART_OFFSET 0x5874
#define CONFIG_NOR_MAJOR_VERSION_NUMBER 1
#define CONFIG_NOR_MINOR_VERSION_NUMBER 0
#define CONFIG_NOR_REVERSION_NUMBER 0
#define CONFIG_NOR_VERSION (CONFIG_NOR_MAJOR_VERSION_NUMBER | (CONFIG_NOR_MINOR_VERSION_NUMBER << 8) | (CONFIG_NOR_REVERSION_NUMBER <<16))

/*#define CONFIG_NOR_BUILTIN_PARAMS*/
#endif
```

#### 2.2.3.4 Default Configuration
1. When starting with NAND flash, the configuration X26xx_halley_uImage_sfc_nand and X26xx_xImage_sfc_nand in the boards.cfg file export **CONFIG_SPL_SFC_NAND**.
2. When starting with NOR flash, the configuration X26xx_halley_uImage_sfc_nor in the boards.cfg file exports **CONFIG_SPL_SFC_NOR**.

#### 2.2.3.5 Custom configuration

**1. sfc controller gpio pins can be selected according to the hardware connection**

```c
/* sfc gpio */
#define CONFIG_JZ_SFC_PD
```

**2. nand flash configuration**

a. sfc controller speed, note that the final speed is the result of this macro divided by 4

```c
#define CONFIG_SFC_NAND_RATE 400000000 /* value <= 400000000(sfc 100Mhz)*/
```

b. Data transfer line width, define this macro as 4 lines, otherwise it is 1 line

```c
#define CONFIG_SFC_QUAD
```

c. Nand flash partition parameters are stored in the offset of nand flash

```c
#define CONFIG_SPIFLASH_PART_OFFSET 0x5800
```

d. Nand flash page size and block size, determined according to the specific nand flash model

```c
#define CONFIG_SPI_NAND_BPP (2048 +64) /*Bytes Per Page*/
#define CONFIG_SPI_NAND_PPB (64) /*Page Per Block*/
```

e. Whether to use the built-in partition parameters, if defined, use them; otherwise, do not use them.

```c
/*#define CONFIG_NAND_BUILTIN_PARAMS*/
```

**3. nor flash configuration**

a. SFC controller clock rate, note that the final rate is the result after the macro is divided by 4.

```c
#define CONFIG_SFC_NOR_RATE 400000000 /* value <= 400000000(sfc 100Mhz)*/
```

b. Data transfer line width, define this macro as 4 lines, otherwise it is 1 line.

```c
#define CONFIG_SFC_QUAD
```

c. The NOR Flash partition parameters are stored at the offset within the NOR Flash.

```c
#define CONFIG_SPIFLASH_PART_OFFSET 0x5800
```

d. The hardware parameters of NOR Flash are stored at an offset within the NOR Flash.

```c
#define CONFIG_SPI_NORFLASH_PART_OFFSET 0x5874
```

e. Whether to use the built-in partition parameters, if defined, use them; otherwise, do not use them.

```c
/*#define CONFIG_NOR_BUILTIN_PARAMS*/
```

**4. Note: After modification, you need to clean the uboot image and recompile it. You can refer to chapters 3.1 and 3.2 for cleaning operations.**

### 2.2.4 Serial Port Configuration

#### 2.2.4.1 Uboot Serial Port Configuration

```c
#define CONFIG_SYS_UART_INDEX       0
#define CONFIG_BAUDRATE         115200
```

"**CONFIG_SYS_UART_INDEX**" corresponds to the serial port number

"**CONFIG_BAUDRATE**" corresponds to the baud rate of the serial port

#### 2.2.4.2 bootargs Serial Port Configuration (kernel serial port)

```c
#define BOOTARGS_COMMON "console=ttyS0,115200 mem=128M@0x0" 
```

"**BOOTARGS_COMMON**" defines the common bootargs parameters. Among them, "**console**" corresponds to the serial port configuration during the kernel startup process.

### 2.2.5 u-boot built-in command configuration

#### 2.2.5.1 Default configuration

The current default support commands are as follows:

```c
/**
* Command configuration.
*/

#define CONFIG_CMD_BOOTD /* bootd */
#define CONFIG_CMD_CONSOLE /* coninfo */
#define CONFIG_CMD_DHCP /* DHCP support */
#define CONFIG_CMD_ECHO /* echo arguments */
#define CONFIG_CMD_EXT4 /* ext4 support */
#define CONFIG_CMD_FAT /* FAT support */
#define CONFIG_USE_XYZMODEM /* xyzModem */
#define CONFIG_CMD_LOAD /* serial load support */
#define CONFIG_CMD_LOADB /* loadb */
#define CONFIG_CMD_LOADS /* loads */
#define CONFIG_CMD_MEMORY /* md mm nm mw cp cmp crc base loop mtest */
#define CONFIG_CMD_MISC /* Misc functions like sleep etc*/
#define CONFIG_CMD_NET /* networking support */
#define CONFIG_CMD_PING
#define CONFIG_CMD_RUN /* run command in env variable */
#define CONFIG_CMD_SETGETDCR /* DCR support on 4xx */
#define CONFIG_CMD_SOURCE /* "source" command support */
#define CONFIG_CMD_GETTIME
#define CONFIG_CMD_GPIO
#define CONFIG_CMD_EXT2
#define CONFIG_CMD_EXT4
#define CONFIG_CMD_FAT
#define CONFIG_EFI_PARTITION
#define CONFIG_SOFT_BURNER
#define CONFIG_CMD_DDR_TEST /* DDR Test Command */
```

#### 2.2.5.2 Custom Configuration

Below, we will introduce how to customize commands in u-boot by creating a simple hello command as an example.

1. In the common directory, create a new file cmd_hello.c and enter the following code:

```c
#include<command.h>
#include<common.h>

static int do_hello(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
printf("hello world!\n");

return 0;
}

U_BOOT_CMD(
hello, 1, 0, do_hello,
"short description. This is a string.n",
"long description. This is a string.n"
);
```

In the above code, U_BOOT_CMD is a macro definition used for creating new commands in u-boot, with a total of 6 parameters, which are introduced as follows:

a. The first parameter: the name of the command
b. The second parameter: the number of arguments for the command
c. The third parameter: whether to repeat the execution of the command after pressing the enter key
d. The fourth parameter: the callback function corresponding to the command
e. The fifth parameter: the short help information for the command
f. The sixth parameter: the long help information for the command

2. Add the following compilation option in common/Makefile:

```c
COBJS-y += cmd_hello.o
```

3. Recompile u-boot.

### 2.2.6 Kernel Parameter Description

Refer to [u-boot kernel parameter description](#_u-boot kernel parameter description)

# 3. u-boot Compilation

The compilation of u-boot is divided into two types, and the features of each type are as follows:

|  |  |
| --- | --- |
| **Compilation Method** | **Features** |
| **Top-level directory compilation** | Dependent on the board-level configuration of Ingenic SDK. Suitable for development based on Ingenic SDK. |
| **u-boot source directory compilation** | Directly follow the standard compilation method of u-boot, suitable for custom u-boot configuration and development methods that need to be separated from Ingenic SDK. |

## 3.1 Top-level directory compilation

### 3.1.1 Compile SFC Nand Boot Image

This boot image corresponds to the **X26xx_halley_uImage_sfc_nand** configuration.

1. Execute the following command in the top-level directory of the project to import the environment variables required for compiling the X26xx Halley platform:

```c
$ source build/envsetup.sh
```

2. Execute the `lunch` command to select the type of image you want to compile:

```c
$ lunch
```

* **a. In the case of the X2660 Halley v1.0 development board, choose between the following configurations as needed:**

```
X2660halley.v10_nand_4.4.94-eng

X2660halley.v10_nand_4.4.94_ota-eng

X2660halley.v10_nand_5.10-eng
```

* **b. In the case of the X2660 Halley v1.1 development board, choose between the following configurations as needed:**

```
X2660halley.v11_nand_4.4.94-eng

X2660halley.v11_nand_4.4.94_ota-eng

X2660halley.v11_nand_5.10-eng
```

* **c. In the case of the X2670 Halley v1.0 development board, choose between the following configurations as needed:**

```
X2670halley.v10_nand_4.4.94-eng

X2670halley.v10_nand_4.4.94_ota-eng

X2670halley.v10_nand_5.10-eng
```

* **d. In the case of the X2670M Hare v1.0 development board, select from the following configurations as needed:**

```
X2670Mhare.v10_nand_4.4.94-eng

X2670Mhare.v10_nand_4.4.94_ota-eng

X2670Mhare.v10_nand_5.10-eng
```

* **e. In the case of the X2600 Halley v1.0 development board, select from the following configurations as needed:**

```
X2600halley.v10_nand_4.4.94-eng

X2600halley.v10_nand_4.4.94_ota-eng

X2600halley.v10_nand_5.10-eng
```

* **f. In the case of the X2600E Halley v1.0 development board, select from the following configurations as needed:**

```
X2600Ehalley.v10_nand_4.4.94-eng

X2600Ehalley.v10_nand_4.4.94_ota-eng

X2600Ehalley.v10_nand_5.10-eng
```

3. Execute `make uboot-clean` to remove the intermediates from the last compilation:

```c
$ make uboot-clean
```

4. Execute `make uboot` to compile:

```c
$ make uboot
```

5. Wait for the compilation to complete, in the project out/product/X26xxhalley(or hare).v10(or v11)_nor_4.4.94(or 5.10)-eng/image directory will be generated under the uboot image file, the image can be burned directly.

### 3.1.2 Compile SFC Nor Boot Image

This boot image corresponds to the **X26xx_halley_uImage_sfc_nor** configuration.

1. Execute the following command in the top-level directory of the project to import the environment variables required to compile the X26xx_halley platform:

```c
$ source build/envsetup.sh
```

2. Execute the `lunch` command to select the type of image to be compiled:

```c
$ lunch
```

* **a. In the case of the X2660 Halley v1.0 development board, choose between the following configurations as needed:**

```
X2660halley.v10_nor_4.4.94-eng

X2660halley.v10_nor_5.10-eng
```

* **b. In the case of the X2660 Halley v1.1 development board, choose between the following configurations as needed:**

```
X2660halley.v11_nor_4.4.94-eng

X2660halley.v11_nor_5.10-eng
```

* **c. In the case of the X2670 Halley v1.0 development board, choose between the following configurations as needed:**

```
X2670halley.v10_nor_4.4.94-eng

X2670halley.v10_nor_5.10-eng
```

* **d. In the case of the X2670M Hare v1.0 development board, select from the following configurations as needed:**

```
X2670Mhare.v10_nor_4.4.94-eng

X2670Mhare.v10_nor_5.10-eng
```

* **e. In the case of the X2600 Halley v1.0 development board, select from the following configurations as needed:**

```
X2600halley.v10_nor_4.4.94-eng

X2600halley.v10_nor_5.10-eng
```

* **f. In the case of the X2600E Halley v1.0 development board, select from the following configurations as needed:**

```
X2600Ehalley.v10_nor_4.4.94-eng

X2600Ehalley.v10_nor_5.10-eng
```

3. Execute `make uboot-clean` to remove the intermediates from the last compilation:

```c
$ make uboot-clean
```

4. Execute `make uboot` to compile:

```c
$ make uboot
```

5. Wait for the compilation to complete, in the project out/product/X26xxhalley (or hare).v10 (or v11)_nand_4.4.94 (or 5.10)-eng/image directory will be generated uboot image file, the image can be burned directly.

### 3.1.3 Compile the EMMC boot image

This boot image corresponds to the **X26xx_halley_uImage_msc0** configuration.

1. In the top-level directory of the project, execute the following command to import the environment variables required for compiling the X26xx_halley platform：

```c
$ source build/envsetup.sh
```

2. Execute the `lunch` command to select the image type for compilation：

```c
$ lunch
```

* **a. In the case of the X2600 Halley v1.0 development board, select from the following configurations as needed:**

```
X2600halley.v10_msc_4.4.94-eng

X2600halley.v10_msc_5.10-eng
```

* **b. In the case of the X2600E Halley v1.0 development board, select from the following configurations as needed:**

```
X2600Ehalley.v10_msc_4.4.94-eng

X2600Ehalley.v10_msc_5.10-eng
```

3. Execute `make uboot-clean` to remove the intermediates from the last compilation:

```c
$ make uboot-clean
```

4. Execute `make uboot` to compile:

```c
$ make uboot
```

5. Wait for the compilation to complete, in the project out/product/X26xxhalley(h or hare).v10(or v11)_nand_4.4.94(or 5.10)-eng/image directory will be generated in the uboot image file, the image can be burned directly.

##  3.2 Compiling U-Boot Source Code

1. In the top-level directory of the project, execute the following command to import the environment variables required for compiling the X26xx_halley platform：

```c
$ source build/envsetup.sh
```

1. Execute the launch command to select the type of image you need to compile, here is just to import the cross-compilation tool environment, if you only compile u-boot you can choose:

```c
$ lunch
```

2. After that, go to the u-boot directory and execute `make distclean` to clear the intermediate products generated by the last compilation:

```c
$ make distclean
```

Execute `make xxxxxx -jN` command to compile, xxxxxx is the specific board configuration name, need to be compiled according to the boot image to decide, please refer to [2.1 boards.cfg file description](#_boards.cfg file description) for details. Currently, there are 4 types of board configurations supported, as shown in the following table:

|  |  |
| --- | --- |
| x2660_halley_uImage_sfc_nor | This configuration supports booting the Linux kernel image uImage from nor flash |
| X2660_halley_uImage_sfc_nand | This configuration supports booting the Linux kernel image uImage from nand flash |
| x2660_halley_xImage_sfc_nor | This configuration supports booting the Linux kernel image xImage from nor flash |
| x2660_halley_xImage_sfc_nand | This configuration supports booting the Linux kernel image xImage from nand flash |
| x2670_halley_uImage_sfc_nor | This configuration supports booting the Linux kernel image uImage from nor flash |
| X2670_halley_uImage_sfc_nand | This configuration supports booting the Linux kernel image uImage from nand flash |
| x2670_halley_xImage_sfc_nor | This configuration supports booting the Linux kernel image xImage from nor flash |
| x2670_halley_xImage_sfc_nand | This configuration supports booting the Linux kernel image xImage from nand flash |
| x2670m_hare_uImage_sfc_nor | This configuration supports booting the Linux kernel image uImage from nor flash |
| X2670m_hare_uImage_sfc_nand | This configuration supports booting the Linux kernel image uImage from nand flash |
| x2670m_hare_xImage_sfc_nor | This configuration supports booting the Linux kernel image xImage from nor flash |
| x2670m_hare_xImage_sfc_nand | This configuration supports booting the Linux kernel image xImage from nand flash |
| x2600_halley_uImage_sfc_nor | This configuration supports booting the Linux kernel image uImage from nor flash |
| X2600_halley_uImage_sfc_nand | This configuration supports booting the Linux kernel image uImage from nand flash |
| x2600_halley_uImage_msc0 | This configuration supports booting the Linux kernel image uImage from emmc |
| x2600_halley_xImage_sfc_nor | This configuration supports booting the Linux kernel image xImage from nor flash |
| x2600_halley_xImage_sfc_nand | This configuration supports booting the Linux kernel image xImage from nand flash |
| x2600_halley_xImage_msc0 | This configuration supports booting the Linux kernel image xImage from emmc |
| x2600e_halley_uImage_sfc_nor | This configuration supports booting the Linux kernel image uImage from nor flash |
| X2600e_halley_uImage_sfc_nand | This configuration supports booting the Linux kernel image uImage from nand flash |
| x2600e_halley_uImage_msc0 | This configuration supports booting the Linux kernel image uImage from emmc |
| x2600e_halley_xImage_sfc_nor | This configuration supports booting the Linux kernel image xImage from nor flash |
| x2600e_halley_xImage_sfc_nand | This configuration supports booting the Linux kernel image xImage from nand flash |
| x2600e_halley_xImage_msc0 | This configuration supports booting the Linux kernel image xImage from emmc |

For example, to compile the boot image that starts from nand flash and uImage, execute the following command:

```c
$ make X26xx_halley_uImage_sfc_nand -jN
```

Here, -jN is used for multi-threaded compilation, which can speed up the compilation process. The specific value of N depends on the PC you are using.

3. After compiling, the final image file u-boot-with-spl.bin will be generated in the top directory of the u-boot project. This file can be directly burned.

# 4. u-boot Application Interface with Kernel

## 4.1 Common u-boot Commands

After entering the u-boot command line mode, you can view the commands supported by the current u-boot by typing "help" or "?" and pressing Enter.

Taking the sfc nand flash startup as an example, the supported commands are as follows:

```c
X26xx_halley# help 
? - alias for 'help' 
base - print or set address offset 
boot - boot default, i.e., run 'bootcmd' 
boota - boot android system 
bootd - boot default, i.e., run 'bootcmd' 
bootm - boot application image from memory 
bootp - boot image via network using BOOTP/TFTP protocol
chpart - change active partition 
cmp - memory compare 
coninfo - print console devices and information 
cp - memory copy 
crc32 - checksum calculation 
dhcp - boot image via network using DHCP/TFTP protocol 
echo - echo args to console 
env - environment handling commands
ext2load- load binary file from a Ext2 filesystem
ext2ls - list files in a directory (default /)
ext4load- load binary file from a Ext4 filesystem
ext4ls - list files in a directory (default /)
fatinfo - print information about filesystem
fatload - load binary file from a dos filesystem
fatls - list files in a directory (default /)
gettime - get timer val elapsed,
go - start application at address 'addr'
gpio - input/set/clear/toggle gpio pins
help - print command description/usage
loadb - load binary file over serial line (kermit mode)
loads - load S-Record file over serial line
loady - load binary file over serial line (ymodem mode)
loop - infinite loop on address range
md - memory display
mm - memory modify (auto-incrementing address)
mtdparts- define flash/nand partitions
mw - memory write (fill)
nand - NAND sub-system
nboot - boot from NAND device
nm - memory modify (constant address)
ping - send ICMP ECHO_REQUEST to network host
printenv- print environment variables
reset - Perform RESET of the CPU
run - run commands in an environment variable
saveenv - save environment variables to persistent storage
setenv - set environment variables
sfcnand - sfcnand - SFC_NAND sub-system
sleep - delay execution for some time
softburn- Ingenic usb soft burn
source - run script from memory
tftpboot- boot image via network using TFTP protocol
ubi - ubi commands
ubifsload- load file from an UBIFS filesystem
ubifsls - list files in a directory
ubifsmount- mount UBIFS volume
ubifsumount- unmount UBIFS volume
version - print monitor, compiler and linker version
```

For each command, you can also execute "help command name" to get the specific usage instructions for that command, as shown below:

```c
X26xx_halley# help bootm
bootm - boot application image from memory
Usage:
bootm [addr [arg ...]]
- boot application image stored in memory
passing arguments 'arg ...'; when booting a Linux kernel,
'arg' can be the address of an initrd image
Sub-commands to do part of the bootm sequence. The sub-commands must be
issued in the order below (it's ok to not issue all sub-commands):
start [addr [arg ...]]
loados - load OS image
cmdline - OS specific command line processing/setup
bdt - OS specific bd_t processing
prep - OS specific prep before relocation or go
go - start OS
```

**Here are some commonly used commands:**

|  |  |
| --- | --- |
| printenv | Print the current environment variables |
| setenv | Set environment variables, format: setenv name value …， meaning setting the name variable to the value; if there is no such parameter, it means deleting the variable |
| saveenv | Save environment variables to storage devices |
| sleep | Delay execution, format: sleep N, which can delay N seconds of execution |
| bootm | Start the kernel image from a specified address in memory. Format: bootm addr, where the parameter is the address of the image in memory |
| md | Read memory data, format: md [.b, .w, .l] address [# of objects], where the first parameter represents the unit of data read at a time (b: 8 bits, w: 16 bits, l: 32 bits), the second parameter is the starting address of the memory to be read, and the third parameter indicates the number of data objects to be read from the address |
| nand erase | Erase nand flash, format: nand erase addr1 count, where the first parameter is the offset, and the second parameter is the number of bytes to be erased |
| nand write | Write memory data into nand flash, format: nand write addr offset count, where the first parameter is the base address for writing, the second parameter is the offset address, and the third parameter is the number of bytes to be written |
| nand read | Read data from nand flash into memory, format: nand read addr offset count, where the first parameter is the nand flash address to be read, the second parameter is the memory position offset, and the third parameter is the number of bytes to be read |
| cp | Copy data in memory, format: cp source target count, where the first parameter is the source address, the second parameter is the destination address, and the third parameter is the number of copies |
| cmp | Compare data in memory, format: cmp addr1 addr2 count, where the first parameter is the memory address one, the second parameter is the memory address two, and the third parameter is the comparison length (in bytes divided by 4, in units of WORDS) |
| crc32 | Calculate the checksum, format: crc32 address count [addr], where the first parameter is the starting address of the data to be checked, the second parameter is the number of data bytes to be checked, and the third parameter is the address where the checksum value is saved |

## 4.2 u-boot passes parameters to the kernel

Before the Linux kernel starts, it needs to know some necessary parameters about the current hardware, such as the available memory size, the serial device used, and the device and partition where the root file system is located. About these startup parameters, the Linux kernel has defined a series of rules to pass them to the bootloader that starts it. Luckily, u-boot has done most of the work for us. It provides an environment variable called `bootargs`, and we just need to set the necessary startup parameters according to the specified format.

Specifically, we can set the startup parameters in `x26xx_halley.h` by defining the macro `CONFIG_BOOTARGS`.

In `x26xx_halley.h`, there are corresponding startup parameters for each mounted file system when loading the kernel. They have some common content. For example, in `x2660_halley.h`, it is defined as follows:

```c
#define BOOTARGS_COMMON "console=ttyS0,115200 mem=128M@0x0 "
```

The specific explanation is as follows:

1. `console=ttyS0,115200` means that the serial device uses /dev/ttyS0, and the baud rate is 115200.
2. `mem=128M@0x0` means that the total available memory size is 128MB, and the starting address is 0x0.
 
**Please refer to the following for the unique part of the boot parameters.**

### 4.2.1 u-boot loading kernel and mounting jffs2 file system

Taking x2660 as an example, this boot parameter corresponds to the **x2660_halley_uImage_sfc_nor** configuration. The current boot parameters in x2660_halley.h are as follows:

```c
#define CONFIG_BOOTARGS 

BOOTARGS_COMMON "ip=off init=/linuxrc rootfstype=jffs2 root=/dev/mtdblock2 rw"
```

The specific explanation is as follows:

1. ip=off means that the network file system is not used.
2. init=/linuxrc means that the init process uses the linuxrc in the root directory.
3. rootfstype=jffs2 means that the root file system format is jffs2.
4. root=/dev/mtdblock2 means that the device storing the root file system is /dev/mtdblock2, corresponding to the partition with nor flash number 2.
5. rw means that the mounted file system can be read and written.

### 4.2.2 u-boot loading kernel and mounting ubifs file system

Taking x2660 as an example, this startup parameter corresponds to the **x2660_halley_uImage_sfc_nand** configuration, and the current startup parameters in x2660_halley.h are as follows:

```c
#define CONFIG_BOOTARGS 

BOOTARGS_COMMON "ip=off init=/linuxrc ubi.mtd=2 root=ubi0:rootfs ubi.mtd=3 rootfstype=ext4 rw"
```

The specific instructions are as follows:

1. ip=off means that the network file system is not used for startup.
2. init=/linuxrc means that the init process uses linuxrc in the root directory.
3. ubi.mtd=2 root=ubi0:rootfs ubi.mtd=3 rootfstype=ubifs rw indicates that the current nand flash partitions numbered 2 and 3 are using the ubifs filesystem, and that the root filesystem is mounted read-write on partition number 2.

### 4.2.3 u-boot load kernel mounting ext4 file system

Taking x2600e as an example, this boot parameter corresponds to the **x2600e_halley_uImage_msc0** configuration, which is currently in x2600e_halley.h with the following boot parameters:

```c
#define CONFIG_BOOTARGS 

BOOTARGS_COMMON " rootfstype=ext4 root=/dev/mmcblk0p7 rootdelay=3 rw"
```

The specific instructions are as follows:

1. rootfstype=ext4 means that the root filesystem format is ext4
2. root=/dev/mmcblk0p7 means that the device that holds the root filesystem is /dev/mmcblk0p7, which corresponds to partition number 7 of the first emmc or sd device on the current system.
3. rootdelay=3 means wait 3 seconds before mounting the root filesystem.
4. rw means the mounted file system is readable and writable.

## 4.3 u-boot loads kernel image

u-boot through tftp or fastboot way to load the kernel image can save the process of burning kernel image, directly load the kernel image into memory, using this way in the development process is very easy to debug the use.

### 4.3.1 uboot loads kernel image via fastboot command

The fastboot method is a command that uses the USB port to download the kernel image to memory and start it, and is only used for debugging. It does not support other functions of Android fastboot.

1. Install the fastboot application on the PC using the following command:

```c
$ sudo apt-get install android-tools-fastboot
```

2. Enter the uboot command line mode and enter the fastboot command:

```c
X26xx_halley# fastboot
```

3. Execute the following command in the path where the kernel image is stored on the PC:
```c
$ sudo fastboot boot uImage
```

4. After the command is successfully executed, the kernel image can be downloaded to memory and started.

### 4.3.2 Load kernel image via UART by U-Boot

The serial port method involves directly downloading the kernel image to memory via the serial port. Note that the transmission speed will be slower with this method, so please choose according to the actual situation.

1. Enter the u-boot command line mode and execute the following command:

```c
x26xx_halley# loady 0x80800000 115200
## Ready for binary (ymodem) download to 0x80800000 at 115200 bps...
```

2. First press and hold the ctrl key, then press and hold the a key, then release both keys at the same time, and finally press the s key, you can see the screen shown below:

![](assets/x26xx-uboot.2.png)

Here press the ↓ key to select the ymodem protocol and then enter;

3. Then the following screen will appear, here you can select the file you need to transfer, ↑↓ key to move, double tap space to enter the directory, single tap space to select the file, enter to confirm the selection.

We select the kernel image we want to transfer:

![](assets/x26xx-uboot.3.png)

4. After selecting and confirming the selection, the following screen will appear for file transfer:

![](assets/x26xx-uboot.4.png)

5. Wait for the transfer to complete, and then boot the kernel image written to memory at location 0x80800000 with the bootm command:

```c
$ bootm 0x80800000
```

# 5. Introduction to Ingenic tools

Ingenic tools is a collection of tools added to u-boot by Ingenic.

## 5.1 DDR parameter related

**Source code location: u-boot/tools/ingenic-tools/**

```
├── ddr_creator_x2000
│   ├── add_chip_info.c
│   ├── ddr3_params.c
│   ├── ddr_params_creator.c
│   ├── ddr_params_creator.h
│   ├── lpddr2_params.c
│   ├── lpddr3_params.c
│   ├── Makefile
│   ├── supported_ddr_chips.c
```

The main use here is to generate the appropriate DDR parameters and then write them to the specified location in the u-boot image as a way to differentiate.

##  5.2 MBR/GPT Partition Table

**Source code location: u-boot/tools/ingenic-tools/**
```
├── gpt.bin
├── gpt_creator.c
├── gpt_tab_to_c.sh
├── mbr_creator.c
├── mbr-of-gpt.bin
├── mk-gpt-xboot.sh
```

Both GPT and MBR are standards about hard disk partitioning.

MBR: Master Boot Record, records information about the hard disk itself as well as the size and location of each partition of the hard disk.

GPT: Globally Unique Identifier Partition Table, GUID partition table, is a newer standard for disk partition table structure derived from the UEFI standard. Compared with the MBR partition scheme, GPT provides a more flexible disk partitioning mechanism.

The function of the file here is to generate the corresponding partition data according to the standards of GPT and MBR.

## 5.3 nand/nor flash related

**Source code location: u-boot/tools/ingenic-tools/**

1. Save various nand flash hardware parameters for use by the sfc driver in spl

```
├── nand_device
```

2. Save nand flash and nor flash partition parameters, which are different from the partition parameters written by the burning tool, and will ultimately be read by the sfc driver in the kernel and generate the corresponding mtd partitions, which can be changed if necessary

```
├── sfc_builtin_params
│   ├── nand_device.c
│   ├── nand_device.h
│   ├── nor_device.c
│   └── nor_device.h
```

3. For writing the partition parameters of nand flash and nor flash into the specified location in the u-boot image

```
├── sfc_nand_builtin_params.c
├── sfc_nor_builtin_params.c
```

## 5.4 Others

**Source code location: u-boot/tools/ingenic-tools/**

1. Used for generating hardware timing parameters for the sfc controller

```
    sfc_timing_params.c
```

2. Used for generating the header data in front of spl when sfc booting, for checking before loading spl by bootom

```
    spi_checksum.c
```

3. Used for X26xx series chips during quick start-up, used for generating spl env parameters

```
    spl_params_fixer_x2600.c
```

# Reference documents

1. u-boot/README
2. kernel-4.4.94/Documentation/kernel-parameters.txt
