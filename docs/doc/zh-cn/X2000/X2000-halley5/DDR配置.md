### 2.2.2 DDR 配置

在板级文件中导入DDR配置文件，例如：在`x2000_halley5.h`中导入`x2000_ddr.h`。

```c
u-boot/include/configs/x2000_halley5.h

/* 导入X2000系列DDR配置文件 */
#include "X2000_ddr.h"
```

#### 2.2.2.1 DDR 配置文件

在DDR配置文件中包含该系列所有DDR参数和相关宏定义，配置如下：

```c
u-boot/include/configs/X2000_ddr.h

#ifndef __X2000_DDR__
#define __X2000_DDR__


#define CONFIG_DDR_INNOPHY
#define CONFIG_DDR_PARAMS_CREATOR
#define CONFIG_DDR_HOST_CC
#define CONFIG_DDR_CS0                      1    /* 1-connected, 0-disconnected */
#define CONFIG_DDR_CS1                      0    /* 1-connected, 0-disconnected */
#define CONFIG_DDR_DW32                     0    /* 1-32bit-width, 0-16bit-width */

#define CONFIG_DDR_TYPE_LPDDR2
#define CONFIG_DDR_TYPE_LPDDR3

#ifdef CONFIG_DDR_TYPE_LPDDR2

	/*X2000E M300*/
	#define CONFIG_LPDDR2_W97BV6MK
        #define CONFIG_LPDDR2_W97BV6MK_MEM_FREQ                 500000000

	/*X2100*/
	#define CONFIG_LPDDR2_M54D5121632A
        #define CONFIG_LPDDR2_M54D5121632A_MEM_FREQ             500000000

	/*X2100L*/
	#define CONFIG_LPDDR2_SCB4BL256160AFL19GI
        #define CONFIG_LPDDR2_SCB4BL256160AFL19GI_MEM_FREQ      500000000

        #define CONFIG_LPDDR2_KGD_CONFIG             0x1
        #define CONFIG_LPDDR2_KGD_MR3_DS             0x1
#endif

#ifdef CONFIG_DDR_TYPE_LPDDR3

	/*X2000*/
	#define CONFIG_LPDDR3_W63AH6NKB_BI

	/*X2000H*/
	#define CONFIG_LPDDR3_NK6CL256M16DKX_H1

        #define CONFIG_LPDDR3_KGD_CONFIG             0x1
        #define CONFIG_LPDDR3_KGD_MR3_DS             0x2
        #define CONFIG_LPDDR3_KGD_MR11_ODT           0x0
#endif

#define CONFIG_PHY_DRVODT_CONFIG             0x0
#define CONFIG_PHY_PU_DRV_CMD                0x8
#define CONFIG_PHY_PD_DRV_CMD                0x8
#define CONFIG_PHY_PU_DRV_CK                 0x8
#define CONFIG_PHY_PD_DRV_CK                 0x8
#define CONFIG_PHY_PU_DRV_DQ7_0              0x8
#define CONFIG_PHY_PD_DRV_DQ7_0              0x8
#define CONFIG_PHY_PU_DRV_DQ15_8             0x8
#define CONFIG_PHY_PD_DRV_DQ15_8             0x8
#define CONFIG_PHY_PU_ODT_DQ7_0              0x5
#define CONFIG_PHY_PD_ODT_DQ7_0              0x5
#define CONFIG_PHY_PU_ODT_DQ15_8             0x5
#define CONFIG_PHY_PD_ODT_DQ15_8             0x5

#define CONFIG_PHY_DESKEW_CONFIG             0x0
#define CONFIG_PHY_DESKEW_CMD                0x3
#define CONFIG_PHY_DESKEW_RX_DM0             0x3
#define CONFIG_PHY_DESKEW_TX_DM0             0x3
#define CONFIG_PHY_DESKEW_RX_DQ7_0           0x3
#define CONFIG_PHY_DESKEW_TX_DQ7_0           0x3
#define CONFIG_PHY_DESKEW_RX_DQS0            0x3
#define CONFIG_PHY_DESKEW_TX_DQS0            0x3
#define CONFIG_PHY_DESKEW_RX_DM1             0x3
#define CONFIG_PHY_DESKEW_TX_DM1             0x3
#define CONFIG_PHY_DESKEW_RX_DQ15_8          0x3
#define CONFIG_PHY_DESKEW_TX_DQ15_8          0x3
#define CONFIG_PHY_DESKEW_RX_DQS1            0x3
#define CONFIG_PHY_DESKEW_TX_DQS1            0x3

#define CONFIG_DDR_PHY_IMPEDANCE             40
#define CONFIG_DDR_PHY_ODT_IMPEDANCE         120

#define CONFIG_DDR_AUTO_SELF_REFRESH
#define CONFIG_DDR_AUTO_SELF_REFRESH_CNT     257

/*
#define CONFIG_DDR_CHIP_ODT
#define CONFIG_DDR_PHY_ODT
#define CONFIG_DDR_PHY_DQ_ODT
#define CONFIG_DDR_PHY_DQS_ODT
#define CONFIG_DDR_PHY_IMPED_PULLUP		0xe
#define CONFIG_DDR_PHY_IMPED_PULLDOWN           0xe
*/

/*
#define CONFIG_DDR_TEST_CPU
#define CONFIG_DDR_TEST
#define CONFIG_DDR_TEST_DATALINE
#define CONFIG_DDR_TEST_ADDRLINE
*/

/*#define CONFIG_DDR_DRVODT_DEBUG*/
/*#define CONFIG_DDRP_SOFTWARE_TRAINING*/

#endif

```
#### 2.2.2.2 DDR 参数文件

DDR参数文件中定义内存基础参数，可在颗粒手册中找到对应说明。

```c
u-boot/include/ddr/chip-v2/LPDDR3_W63AH6NKB_BI.h
/*
 * =====================================================================================
 *
 *       Filename:  LPDDR3_W63AH6NKB_BI.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2020年09月21日 17时55分13秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *   Organization:
 *
 * =====================================================================================
 */

#ifndef __LPDDR3_W63AH6NKB_BI_H__
#define __LPDDR3_W63AH6NKB_BI_H__

#ifndef CONFIG_LPDDR3_W63AH6NKB_BI_MEM_FREQ
#define CONFIG_LPDDR3_W63AH6NKB_BI_MEM_FREQ CONFIG_SYS_MEM_FREQ
#endif

#if (CONFIG_DDR_SEL_PLL == MPLL)
#define CONFIG_SYS_PLL_FREQ CONFIG_SYS_MPLL_FREQ
#else
#define CONFIG_SYS_PLL_FREQ CONFIG_SYS_APLL_FREQ
#endif

#define CONFIG_DDR_DATA_RATE (CONFIG_LPDDR3_W63AH6NKB_BI_MEM_FREQ * 2)

#if((CONFIG_SYS_PLL_FREQ % CONFIG_LPDDR3_W63AH6NKB_BI_MEM_FREQ ) ||\
	(CONFIG_SYS_PLL_FREQ / CONFIG_LPDDR3_W63AH6NKB_BI_MEM_FREQ < 0) ||\
	(CONFIG_SYS_PLL_FREQ / CONFIG_LPDDR3_W63AH6NKB_BI_MEM_FREQ > 15))
#error DDR memoryclock division ratio should be an integer between 1 and 16, check CONFIG_SYS_MPLL_FREQ and CONFIG_LPDDR3_W63AH6NKB_BI_MEM_FREQ
#endif

#if	((CONFIG_DDR_DATA_RATE > 50000000) &&\
		(CONFIG_DDR_DATA_RATE <= 333000000))
#define CONFIG_DDR_RL	3
#define CONFIG_DDR_WL	1
#elif((CONFIG_DDR_DATA_RATE > 333000000) &&\
		(CONFIG_DDR_DATA_RATE <= 800000000))
#define CONFIG_DDR_RL	6
#define CONFIG_DDR_WL	3
#elif((CONFIG_DDR_DATA_RATE > 800000000) &&\
		(CONFIG_DDR_DATA_RATE <= 1066000000))
#define CONFIG_DDR_RL	8
#define CONFIG_DDR_WL	4
#elif((CONFIG_DDR_DATA_RATE > 1066000000) &&\
		(CONFIG_DDR_DATA_RATE <= 1200000000))
#define CONFIG_DDR_RL	9
#define CONFIG_DDR_WL	5
#elif((CONFIG_DDR_DATA_RATE > 1200000000) &&\
		(CONFIG_DDR_DATA_RATE <= 1333000000))
#define CONFIG_DDR_RL	10
#define CONFIG_DDR_WL	6
#elif((CONFIG_DDR_DATA_RATE > 1333000000) &&\
		(CONFIG_DDR_DATA_RATE <= 1466000000))
#define CONFIG_DDR_RL	11
#define CONFIG_DDR_WL	6
#elif((CONFIG_DDR_DATA_RATE > 1466000000) &&\
		(CONFIG_DDR_DATA_RATE <= 1600000000))
#define CONFIG_DDR_RL	12
#define CONFIG_DDR_WL	6
#else
#define CONFIG_DDR_RL	-1
#define CONFIG_DDR_WL	-1
#endif

#if(-1 == CONFIG_DDR_RL)
#error CONFIG_LPDDR3_W63AH6NKB_BI_MEM_FREQ "don't support, check data_rate range"
#endif


#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_KGD_CONFIG) && \
        defined(CONFIG_LPDDR3_KGD_CONFIG)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_KGD_CONFIG            CONFIG_LPDDR3_KGD_CONFIG
#endif
#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_KGD_MR3_DS) && \
        defined(CONFIG_LPDDR3_KGD_MR3_DS)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_KGD_MR3_DS            CONFIG_LPDDR3_KGD_MR3_DS
#endif
#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_KGD_MR11_ODT) && \
        defined(CONFIG_LPDDR3_KGD_MR11_ODT)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_KGD_MR11_ODT          CONFIG_LPDDR3_KGD_MR11_ODT
#endif

#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DRVODT_CONFIG) && \
        defined(CONFIG_PHY_DRVODT_CONFIG)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DRVODT_CONFIG     CONFIG_PHY_DRVODT_CONFIG
#endif
#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PU_DRV_CMD) && \
        defined(CONFIG_PHY_PU_DRV_CMD)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PU_DRV_CMD        CONFIG_PHY_PU_DRV_CMD
#endif
#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PD_DRV_CMD) && \
        defined(CONFIG_PHY_PD_DRV_CMD)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PD_DRV_CMD        CONFIG_PHY_PD_DRV_CMD
#endif
#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PU_DRV_CK) && \
        defined(CONFIG_PHY_PU_DRV_CK)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PU_DRV_CK         CONFIG_PHY_PU_DRV_CK
#endif
#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PD_DRV_CK) && \
        defined(CONFIG_PHY_PD_DRV_CK)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PD_DRV_CK         CONFIG_PHY_PD_DRV_CK
#endif
#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PU_DRV_DQ7_0) && \
        defined(CONFIG_PHY_PU_DRV_DQ7_0)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PU_DRV_DQ7_0      CONFIG_PHY_PU_DRV_DQ7_0
#endif
#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PD_DRV_DQ7_0) && \
        defined(CONFIG_PHY_PD_DRV_DQ7_0)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PD_DRV_DQ7_0      CONFIG_PHY_PD_DRV_DQ7_0
#endif
#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PU_DRV_DQ15_8) && \
        defined(CONFIG_PHY_PU_DRV_DQ15_8)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PU_DRV_DQ15_8     CONFIG_PHY_PU_DRV_DQ15_8
#endif
#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PD_DRV_DQ15_8) && \
        defined(CONFIG_PHY_PD_DRV_DQ15_8)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PD_DRV_DQ15_8     CONFIG_PHY_PD_DRV_DQ15_8
#endif
#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PU_ODT_DQ7_0) && \
        defined(CONFIG_PHY_PU_ODT_DQ7_0)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PU_ODT_DQ7_0      CONFIG_PHY_PU_ODT_DQ7_0
#endif
#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PD_ODT_DQ7_0) && \
        defined(CONFIG_PHY_PD_ODT_DQ7_0)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PD_ODT_DQ7_0      CONFIG_PHY_PD_ODT_DQ7_0
#endif
#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PU_ODT_DQ15_8) && \
        defined(CONFIG_PHY_PU_ODT_DQ15_8)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PU_ODT_DQ15_8     CONFIG_PHY_PU_ODT_DQ15_8
#endif
#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PD_ODT_DQ15_8) && \
        defined(CONFIG_PHY_PD_ODT_DQ15_8)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PD_ODT_DQ15_8     CONFIG_PHY_PD_ODT_DQ15_8
#endif

#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_CONFIG) && \
        defined(CONFIG_PHY_DESKEW_CONFIG)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_CONFIG     CONFIG_PHY_DESKEW_CONFIG
#endif
#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_CMD) && \
        defined(CONFIG_PHY_DESKEW_CMD)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_CMD        CONFIG_PHY_DESKEW_CMD
#endif
#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_RX_DM0) && \
        defined(CONFIG_PHY_DESKEW_RX_DM0)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_RX_DM0     CONFIG_PHY_DESKEW_RX_DM0
#endif
#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_TX_DM0) && \
        defined(CONFIG_PHY_DESKEW_TX_DM0)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_TX_DM0     CONFIG_PHY_DESKEW_TX_DM0
#endif
#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_RX_DQ7_0) && \
        defined(CONFIG_PHY_DESKEW_RX_DQ7_0)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_RX_DQ7_0   CONFIG_PHY_DESKEW_RX_DQ7_0
#endif
#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_TX_DQ7_0) && \
        defined(CONFIG_PHY_DESKEW_TX_DQ7_0)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_TX_DQ7_0   CONFIG_PHY_DESKEW_TX_DQ7_0
#endif
#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_RX_DQS0) && \
        defined(CONFIG_PHY_DESKEW_RX_DQS0)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_RX_DQS0    CONFIG_PHY_DESKEW_RX_DQS0
#endif
#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_TX_DQS0) && \
        defined(CONFIG_PHY_DESKEW_TX_DQS0)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_TX_DQS0    CONFIG_PHY_DESKEW_TX_DQS0
#endif
#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_RX_DM1) && \
        defined(CONFIG_PHY_DESKEW_RX_DM1)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_RX_DM1     CONFIG_PHY_DESKEW_RX_DM1
#endif
#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_TX_DM1) && \
        defined(CONFIG_PHY_DESKEW_TX_DM1)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_TX_DM1     CONFIG_PHY_DESKEW_TX_DM1
#endif
#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_RX_DQ15_8) && \
        defined(CONFIG_PHY_DESKEW_RX_DQ15_8)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_RX_DQ15_8  CONFIG_PHY_DESKEW_RX_DQ15_8
#endif
#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_TX_DQ15_8) && \
        defined(CONFIG_PHY_DESKEW_TX_DQ15_8)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_TX_DQ15_8  CONFIG_PHY_DESKEW_TX_DQ15_8
#endif
#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_RX_DQS1) && \
        defined(CONFIG_PHY_DESKEW_RX_DQS1)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_RX_DQS1    CONFIG_PHY_DESKEW_RX_DQS1
#endif
#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_TX_DQS1) && \
        defined(CONFIG_PHY_DESKEW_TX_DQS1)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_TX_DQS1    CONFIG_PHY_DESKEW_TX_DQS1
#endif

#if defined(CONFIG_DDR_DRIVER_STRENGTH)
#define CONFIG_LPDDR3_W63AH6NKB_BI_KGD_CONFIG                    0x1
#define CONFIG_LPDDR3_W63AH6NKB_BI_KGD_MR3_DS                    CONFIG_DDR_DRIVER_STRENGTH
#if !defined(CONFIG_LPDDR3_W63AH6NKB_BI_KGD_MR11_ODT)
        #define CONFIG_LPDDR3_W63AH6NKB_BI_KGD_MR11_ODT          0x0
#endif
#endif


static inline void LPDDR3_W63AH6NKB_BI_init(void *data)
{

	struct ddr_chip_info *c = (struct ddr_chip_info *)data;


	c->DDR_ROW  		= 13,
	c->DDR_ROW1 		= 13,
	c->DDR_COL  		= 10,
	c->DDR_COL1 		= 10,
	c->DDR_BANK8 		= 1,
	c->DDR_BL	   	= 8,
/*
0100b: RL = 6 / WL = 3 (≤400 MHz)
0110b: RL = 8 / WL = 4 (≤533 MHz)
0111b: RL = 9 / WL = 5 (≤600 MHz)
1000b: RL = 10 / WL = 6 (≤667 MHz, default)
1001b: RL = 11 / WL = 6 (≤733 MHz)
1010b: RL = 12 / WL = 6 (≤800 MHz)
1100b: RL = 14 / WL = 8 (≤933 MHz)
1110b: RL = 16 / WL = 8 (≤1066 MHz)
*/

	c->DDR_RL	   	= DDR__tck(CONFIG_DDR_RL),
	c->DDR_WL	   	= DDR__tck(CONFIG_DDR_WL),

	c->DDR_tMRW  		= DDR_SELECT_MAX__tCK_ps(10, 14 * 1000);
	c->DDR_tDQSCK 		= DDR__ps(2500);
	c->DDR_tDQSCKMAX 	= DDR__ps(5500);
	c->DDR_tRAS  		= DDR_SELECT_MAX__tCK_ps(3, 42 * 1000);
	c->DDR_tRTP  		= DDR_SELECT_MAX__tCK_ps(4, 7500);
	c->DDR_tRP   		= DDR_SELECT_MAX__tCK_ps(3, 21 * 1000);
	c->DDR_tRCD  		= DDR_SELECT_MAX__tCK_ps(3, 18 * 1000);
	c->DDR_tRC   		= c->DDR_tRAS + c->DDR_tRP;
	c->DDR_tRRD  		= DDR_SELECT_MAX__tCK_ps(2, 10 * 1000);
	c->DDR_tWR   		= DDR_SELECT_MAX__tCK_ps(4, 15 * 1000);
	c->DDR_tWTR  		= DDR_SELECT_MAX__tCK_ps(4, 7500);
	c->DDR_tCCD  		= DDR__tck(4);
	c->DDR_tFAW  		= DDR_SELECT_MAX__tCK_ps(8, 50 * 1000);

	c->DDR_tRFC  		= DDR__ns(130);
	c->DDR_tREFI 		= DDR__ns(3900);

	c->DDR_tCKE  		= DDR_SELECT_MAX__tCK_ps(3, 7500);
	c->DDR_tCKESR 		= DDR_SELECT_MAX__tCK_ps(3, 15 * 1000);
	c->DDR_tXSR  		= DDR_SELECT_MAX__tCK_ps(2, (c->DDR_tRFC + 10 * 1000));
	c->DDR_tXP  		= DDR_SELECT_MAX__tCK_ps(3, 7500);

#ifdef CONFIG_LPDDR3_W63AH6NKB_BI_KGD_CONFIG
        struct lpddr3_mr_config *mr_cfg    = &c->kgd_config.mr_config;
        c->kgd_config.use_kgd_config       = CONFIG_LPDDR3_W63AH6NKB_BI_KGD_CONFIG  ;
        mr_cfg->kgd_mr3_ds                 = CONFIG_LPDDR3_W63AH6NKB_BI_KGD_MR3_DS  ;
	mr_cfg->kgd_mr11_odt               = CONFIG_LPDDR3_W63AH6NKB_BI_KGD_MR11_ODT;
#endif
#ifdef CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DRVODT_CONFIG
        c->phy_drvodt.use_drvodt_config    = CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DRVODT_CONFIG;
        c->phy_drvodt.phy_pu_drv_cmd       = CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PU_DRV_CMD   ;
        c->phy_drvodt.phy_pd_drv_cmd       = CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PD_DRV_CMD   ;
        c->phy_drvodt.phy_pu_drv_ck        = CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PU_DRV_CK    ;
        c->phy_drvodt.phy_pd_drv_ck        = CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PD_DRV_CK    ;
        c->phy_drvodt.phy_pu_drv_dq7_0     = CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PU_DRV_DQ7_0 ;
        c->phy_drvodt.phy_pd_drv_dq7_0     = CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PD_DRV_DQ7_0 ;
        c->phy_drvodt.phy_pu_drv_dq15_8    = CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PU_DRV_DQ15_8;
        c->phy_drvodt.phy_pd_drv_dq15_8    = CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PD_DRV_DQ15_8;
        c->phy_drvodt.phy_pu_odt_dq7_0     = CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PU_ODT_DQ7_0 ;
        c->phy_drvodt.phy_pd_odt_dq7_0     = CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PD_ODT_DQ7_0 ;
        c->phy_drvodt.phy_pu_odt_dq15_8    = CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PU_ODT_DQ15_8;
        c->phy_drvodt.phy_pd_odt_dq15_8    = CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PD_ODT_DQ15_8;
#endif
#ifdef CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_CONFIG
        c->phy_deskew.use_deskew_config    = CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_CONFIG   ;
        c->phy_deskew.phy_deskew_cmd       = CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_CMD      ;
        c->phy_deskew.phy_deskew_rx_dm0    = CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_RX_DM0   ;
        c->phy_deskew.phy_deskew_tx_dm0    = CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_TX_DM0   ;
        c->phy_deskew.phy_deskew_rx_dq7_0  = CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_RX_DQ7_0 ;
        c->phy_deskew.phy_deskew_tx_dq7_0  = CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_TX_DQ7_0 ;
        c->phy_deskew.phy_deskew_rx_dqs0   = CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_RX_DQS0  ;
        c->phy_deskew.phy_deskew_tx_dqs0   = CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_TX_DQS0  ;
        c->phy_deskew.phy_deskew_rx_dm1    = CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_RX_DM1   ;
        c->phy_deskew.phy_deskew_tx_dm1    = CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_TX_DM1   ;
        c->phy_deskew.phy_deskew_rx_dq15_8 = CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_RX_DQ15_8;
        c->phy_deskew.phy_deskew_tx_dq15_8 = CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_TX_DQ15_8;
        c->phy_deskew.phy_deskew_rx_dqs1   = CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_RX_DQS1  ;
        c->phy_deskew.phy_deskew_tx_dqs1   = CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DESKEW_TX_DQS1  ;
#endif
}

#define LPDDR3_W63AH6NKB_BI {						\
	.name 	= "W63AH6NKB_BI",					\
	.id	= DDR_CHIP_ID(VENDOR_WINBOND, TYPE_LPDDR3, MEM_128M),	\
	.type	= LPDDR3,						\
	.size	= 128,							\
	.freq	= CONFIG_LPDDR3_W63AH6NKB_BI_MEM_FREQ,			\
	.init	= LPDDR3_W63AH6NKB_BI_init,				\
}


#endif

```
参数文件中KGD和PHY配置相关宏介绍：

| 宏定义 | 描述 |
| :----- | :----- |
| `CONFIG_{类型}_{KGD/PHY}_CONFIG` | `{类型}` 通用KGD或PHY配置是否使用。|
| `CONFIG_{类型}_{KGD/PHY}_{配置}` | `{类型}` 通用KGD或PHY配置项。 |
| `CONFIG_{型号}_{KGD/PHY}_CONFIG` | `{型号}` KGD或PHY配置是否使用。 |
| `CONFIG_{型号}_{KGD/PHY}_{配置}` | `{型号}` KGD或PHY配置项。 |

若需要单独调整某款DDR信号时`CONFIG_{型号}_{KGD/PHY}_XXXX`配置方式：

1. 继承通用配置`CONFIG_{类型}_{KGD/PHY}_XXXX`，例如参数文件中现有配置。
2. 直接定义`CONFIG_{型号}_{KGD/PHY}_XXXX`宏，例如在DDR配置文件或者板级配置文件定义。
3. 兼容旧宏定义，例如旧板级配置中`CONFIG_DDR_DLL_OFF`,`CONFIG_DDR_DRIVE_STRENGTH`等。


#### 2.2.2.3 DDR 寄存器配置生成

在u-boot/tools/ingenic-tools/ddr_creator_x2000目录下为DDR寄存器配置生成工具，例如MR寄存器配置如下：

```c
u-boot/tools/ingenic-tools/ddr_creator_x2000/ddr3_params.c

static void fill_mr_params_ddr3(struct ddr_params *p, struct kgd_config *kgd_cfg)
{
        struct ddr3_mr_config *mr_cfg = &kgd_cfg->mr_config;
        int tmp;

        /* MRn registers */
        p->mr0.ddr3.BA = 0;
        p->mr1.ddr3.BA = 1;
        p->mr2.ddr3.BA = 2;
        p->mr3.ddr3.BA = 3;

        /* BL: 1 is on the fly,???? */
        if(p->bl == 4)
            p->mr0.ddr3.BL = 2;
        else if(p->bl == 8)
            p->mr0.ddr3.BL = 0;
        else{
            out_error("DDR_BL(%d) error,only support 4 or 8\n",p->bl);
            assert(1);
        }
        p->mr0.ddr3.BL = (8 - p->bl) / 2;

        BETWEEN(p->cl,5,14);
        if(p->cl < 11) {
            p->mr0.ddr3.CL_4_6 = (p->cl % 11) - 4;
            p->mr0.ddr3.CL_2 = p->cl / 11;
        }
        else if(p->cl == 11) {
            p->mr0.ddr3.CL_4_6 = 7;
            p->mr0.ddr3.CL_2 = 0;
        }
        else if(p->cl == 12) {
            p->mr0.ddr3.CL_4_6 = 0;
            p->mr0.ddr3.CL_2 = 1;
        }
        else if(p->cl == 13) {
            p->mr0.ddr3.CL_4_6 = 1;
            p->mr0.ddr3.CL_2 = 1;
        }
        else if(p->cl == 14) {
            p->mr0.ddr3.CL_4_6 = 2;
            p->mr0.ddr3.CL_2 = 1;
        }

        tmp = ps2cycle_ceil(p->private_params.ddr3_params.tWR, 1);
        switch(tmp)
        {
        case 5 ... 8:
            p->mr0.ddr3.WR = tmp - 4;
            break;
        case 9 ... 12:
            p->mr0.ddr3.WR = (tmp + 1) / 2;
            break;
        case 14:
            p->mr0.ddr3.WR = tmp / 2;
            break;
        case 15 ... 16:
            p->mr0.ddr3.WR = 0;
            break;
    #ifdef CONFIG_AD_SLT
        case 17 ... 22:
            p->mr0.ddr3.WR = 0;
            break;
    #endif
        default:
            out_error("tWR(%d) is error, valid value is between from 5 to 12.\n",
                p->private_params.ddr3_params.tWR);
            assert(1);
        }

	if (kgd_cfg->use_kgd_config) {
            p->mr0.ddr3.DR = mr_cfg->kgd_mr0_dll_rst & 1;
            p->mr0.ddr3.PD = mr_cfg->kgd_mr0_pd & 1;
            p->mr1.ddr3.DE = mr_cfg->kgd_mr1_dll_en & 1;

            p->mr1.ddr3.DIC5 = (mr_cfg->kgd_mr1_dic & (1 << 1)) >> 1;
            BETWEEN(p->mr1.ddr3.DIC5,0,1);
            p->mr1.ddr3.DIC1 = mr_cfg->kgd_mr1_dic & 1;
            BETWEEN(p->mr1.ddr3.DIC1,0,1);

            p->mr1.ddr3.RTT9 = (mr_cfg->kgd_mr1_rtt_nom & (1 << 2)) >> 2;
            BETWEEN(p->mr1.ddr3.RTT9,0,1);
            p->mr1.ddr3.RTT6 = (mr_cfg->kgd_mr1_rtt_nom & (1 << 1)) >> 1;
            BETWEEN(p->mr1.ddr3.RTT6,0,1);
            p->mr1.ddr3.RTT2 = mr_cfg->kgd_mr1_rtt_nom & 1;
            BETWEEN(p->mr1.ddr3.RTT2,0,1);

            p->mr2.ddr3.RTTWR = mr_cfg->kgd_mr2_rtt_wr & 3;
            BETWEEN(p->mr2.ddr3.RTTWR,0,3);
	} else {
            p->mr0.ddr3.DR = 1;
            p->mr0.ddr3.PD = 1;

            /* DLL 0:enable, 1:disable. */
            p->mr1.ddr3.DE = 0;

            /**********************
                DIC5   DIC1
                * 0     0 - RZQ/6.   *
                * 0     1 - RZQ/7.   * default
                * 1     0 - RZQ/3.   *
                * 1     1 - RZQ/4.   *
                **********************/
            p->mr1.ddr3.DIC5 = 0;
            p->mr1.ddr3.DIC1 = 1;

            /**********************
                * 000 - ODT disable. * default
                * 001 - RZQ/4.       *
                * 010 - RZQ/2.       *
                * 011 - RZQ/6.       *
                * 100 - RZQ/12.      *
                * 101 - RZQ/8.       *
                **********************/
            p->mr1.ddr3.RTT9 = 0;
            p->mr1.ddr3.RTT6 = 0;
            p->mr1.ddr3.RTT2 = 0;

            /*****************************
                A10    A9
                * 0     0 - rtt_wr disable. * default
                * 0     1 - RZQ/4.          *
                * 1     0 - RZQ/2.          *
                * 1     1 - reserved.       *
                *****************************/
            p->mr2.ddr3.RTTWR = 0;
	}

        tmp = -1;
        tmp = ps2cycle_ceil(p->private_params.ddr3_params.WL,1);
    #ifdef CONFIG_AD_SLT
        if(tmp < 5 || tmp > 10)
    #else
        if(tmp < 5 || tmp > 9)
    #endif
        {
            out_error("ddr frequancy too fast. %d\n",tmp);
            out_error(". %d\n",__ps_per_tck);
            assert(1);
        }
        p->mr2.ddr3.CWL = tmp - 5;
}

```

#### 2.2.2.4 DDR 寄存器配置文件

Creator工具生成的DDR寄存器配置文件`ddr_reg_values.h`，配置如下：

```c
u-boot/include/generated/ddr_reg_values.h

/* Warnning: Please set ddr driver strength.*/

#ifndef __DDR_REG_VALUES_H__
#define __DDR_REG_VALUES_H__
#include <asm/ddr_innophy.h>
struct ddr_reg_value supported_ddr_reg_values[] = {
{
	.h.name		       = "W63AH6NKB_BI",
	.h.id		       = 0x00000041,
	.h.type		       = 0x00000003,
	.h.freq		       = 0x2cb41780,
	.DDRC_CFG_VALUE        = 0x82002a29,
	.DDRC_CTRL_VALUE       = 0x00003092,
	.DDRC_DLMR_VALUE       = 0x00000006,
	.DDRC_DDLP_VALUE       = 0x00000000,
	.DDRC_MMAP0_VALUE      = 0x000020f8,
	.DDRC_MMAP1_VALUE      = 0x00002800,
	.DDRC_REFCNT_VALUE     = 0x40b60081,
	.DDRC_TIMING1_VALUE    = 0x06110c06,
	.DDRC_TIMING2_VALUE    = 0x0a10060c,
	.DDRC_TIMING3_VALUE    = 0x030e0410,
	.DDRC_TIMING4_VALUE    = 0x26302008,
	.DDRC_TIMING5_VALUE    = 0x1b0c0066,
	.DDRC_AUTOSR_CNT_VALUE = 0x31000101,
	.DDRC_AUTOSR_EN_VALUE  = 0x00000001,
	.DDRC_HREGPRO_VALUE    = 0x00000001,
	.DDRC_PREGPRO_VALUE    = 0x00000001,
	.DDRC_CGUC0_VALUE      = 0x11111111,
	.DDRC_CGUC1_VALUE      = 0x00000113,
	.DDRP_MEMCFG_VALUE     = 0x00000012,
	.DDRP_CL_VALUE         = 0x0000000c,
	.DDRP_CWL_VALUE        = 0x00000006,
	.DDR_MR0_VALUE         = 0x00000000,
	.DDR_MR1_VALUE         = 0x00000143,
	.DDR_MR2_VALUE         = 0x0000021a,
	.DDR_MR3_VALUE         = 0x00000302,
	.DDR_MR10_VALUE        = 0x00000aab,
	.DDR_MR11_VALUE        = 0x00000b00,
	.DDR_MR63_VALUE        = 0x00003ffc,
	.DDR_CHIP_0_SIZE       = 0x08000000,
	.DDR_CHIP_1_SIZE       = 0x00000000,
	.REMMAP_ARRAY[0] = 0x030e0d0c,
	.REMMAP_ARRAY[1] = 0x07060504,
	.REMMAP_ARRAY[2] = 0x0b0a0908,
	.REMMAP_ARRAY[3] = 0x0f020100,
	.REMMAP_ARRAY[4] = 0x13121110,
	.phy_drvodt = {
                .use_drvodt_config    = 0x00,
                .phy_pu_drv_cmd       = 0x08,
                .phy_pd_drv_cmd       = 0x08,
                .phy_pu_drv_ck        = 0x08,
                .phy_pd_drv_ck        = 0x08,
                .phy_pu_drv_dq7_0     = 0x08,
                .phy_pd_drv_dq7_0     = 0x08,
                .phy_pu_drv_dq15_8    = 0x08,
                .phy_pd_drv_dq15_8    = 0x08,
                .phy_pu_odt_dq7_0     = 0x05,
                .phy_pd_odt_dq7_0     = 0x05,
                .phy_pu_odt_dq15_8    = 0x05,
                .phy_pd_odt_dq15_8    = 0x05,
        },
	.phy_deskew = {
                .use_deskew_config    = 0x00,
                .phy_deskew_cmd       = 0x03,
                .phy_deskew_rx_dm0    = 0x03,
                .phy_deskew_tx_dm0    = 0x03,
                .phy_deskew_rx_dq7_0  = 0x03,
                .phy_deskew_tx_dq7_0  = 0x03,
                .phy_deskew_rx_dqs0   = 0x03,
                .phy_deskew_tx_dqs0   = 0x03,
                .phy_deskew_rx_dm1    = 0x03,
                .phy_deskew_tx_dm1    = 0x03,
                .phy_deskew_rx_dq15_8 = 0x03,
                .phy_deskew_tx_dq15_8 = 0x03,
                .phy_deskew_rx_dqs1   = 0x03,
                .phy_deskew_tx_dqs1   = 0x03,
        },
},

...

```

#### 2.2.2.4 DDR 匹配参数

在DDR初始化时读取镜像128字节处`DDR ID`或者读取Efuse中`SOC ID`生成`DDR ID`与Creator生成的寄存器配置列表中id匹配，获取到芯片KGD相对应的DDR寄存器配置，匹配代码如下：

```c
u-boot/arch/mips/cpu/xburst2/ddr_innophy.c

#ifndef CONFIG_BURNER

__weak unsigned int check_socid(void)
{
        return -1;
}


int get_ddr_params_socid(void)
{
	int i;
	int found = 0;
	uint32_t ddrid = 0;
	uint32_t mask = ~(7 << 3);

	ddrid = check_socid();
	if ((int)ddrid < 0) {
		printf("Check socid return invalid ddr id %x\n",ddrid);
		return -1;
	}

	for(i = 0; i < ARRAY_SIZE(supported_ddr_reg_values); i++) {
		global_reg_value = &supported_ddr_reg_values[i];
		if((ddrid & mask) == (global_reg_value->h.id & mask)) {
			found = 1;
			break;
		}
	}

	if(found == 0) {
		printf("Check socid not match to %x\n",ddrid);
		return -1;
	}

	return 0;
}

void get_ddr_params_normal(void)
{
	int found = 0;
	int size = 0;
	int i;
	unsigned int burned_ddr_id = *(volatile unsigned int *)(CONFIG_SPL_TEXT_BASE + 128);
	uint32_t mask = ~(7 << 3);

	if((burned_ddr_id & 0xffff) != (burned_ddr_id >> 16)) {
		printf("invalid burned ddr id\n");
	}

	burned_ddr_id &= 0xffff;

	for(i = 0; i < ARRAY_SIZE(supported_ddr_reg_values); i++) {
		global_reg_value = &supported_ddr_reg_values[i];
		if((burned_ddr_id & mask) == (global_reg_value->h.id & mask)) {
			found = 1;
			break;
		}
	}

	if(found == 0) {
		printf("No match to %x\n",burned_ddr_id);
	}

}
#else
void get_ddr_params_burner(void)
{
	/* keep ddr_reg_value inc ddr_innophy.h
	 * with ddr_registers the same
	 * */
	global_reg_value = g_ddr_param;
    memset(&global_reg_value->phy_drvodt, 0, sizeof(struct phy_drvodt_config));
    memset(&global_reg_value->phy_deskew, 0, sizeof(struct phy_deskew_config));
}
#endif

void get_ddr_params(void)
{
#ifndef CONFIG_BURNER
	if(ARRAY_SIZE(supported_ddr_reg_values) == 1)
		global_reg_value = &supported_ddr_reg_values[0];
	else if (get_ddr_params_socid() < 0)
		get_ddr_params_normal();
#else
	get_ddr_params_burner();
#endif
	//dump_generated_reg(global_reg_value);

}

```

#### 2.2.2.5 DDR 测试

- SPL中DDR测试
  - 在DDR配置文件中开启DDR_TEST相关宏定义，例如：
    ```c
    u-boot/include/configs/X2000_ddr.h

    #define CONFIG_DDR_TEST_CPU
    #define CONFIG_DDR_TEST
    #define CONFIG_DDR_TEST_DATALINE
    #define CONFIG_DDR_TEST_ADDRLINE
    ```
  - 测试打印：
    ```c
    U-Boot SPL 2013.07-dirty(Dec042024-18:42:10)
    ERROR EPC bfc000a4
    Current Version:V2
    CPA_CPAPCR:0310086d
    CPM_CPMPCR:07c0484d
    CPM_CPEPCR:0310186d
    CPM_CPCCR:9a094410
    invalid ddr id 00000000
    DDR:W97BV6MK type is:LPDDR2
    -----ddr_readl(DDRP_INNOPHY_CALIB_DONE):00000003
    Now test the DDR
    ddr test address unremap case
    cache test the ddr ...
    cache test OK
    Now the DDR test over
    ```
  - 或者, 在DDR初始化后，添加DDR测试代码，例如:
    ```c
    u-boot/arch/mips/cpu/xburst2/x2000_v12/soc.c
    unsigned int addr = 0xa0000000;
    unsigned int size = 0x10000000;
    unsigned int data = 0xffffffff;
    int i = 0;
    while(1) {
              for (i=0; i<size; i+=4) {
                      *(volatile unsignedl int *)(addr + i) = data;
              }
              for (i = 0; i<size; i+=4) {
                      if (*(volatile unsigned int *)(addr + i) != data) {
                              printf("read 1 address %x data %x\n", i, *(volatile unsigned t *)(addr + i));
                              printf("read 2 address %x data %x\n", i, *(volatile unsigned t *)(addr + i));
                      }
              }
              data = ~data;
    }
    ```
  - 为了方便分析出错地址，关闭DDR REMAP功能，例如：
    ```c
    u-boot/arch/mips/cpu/xburst2/ddr_innophy.c
    // mem_remap();  //关闭DDR REMAP功能
    ```

  - 测试打印：
    ```c
    U-BootSPL2013.07-dirty(Dec042024-18:42:10)
    ERROREPCbfc000a4
    CurrentVersion:V2
    CPA_CPAPCR:0310086d
    CPM_CPMPCR:07c0484d
    CPM_CPEPCR:0310186d
    CPM_CPCCR:9a094410
    invalidddrid00000000
    DDR:W97BV6MKtypeis:LPDDR2
    -----ddr_readl(DDRP_INNOPHY_CALIB_DONE):00000003
    read 1 address 079000c4 data f7ffffff
    read 2 address 079000c4 data f7ffffff
    read 1 address 079000cc data f7ffffff
    read 2 address 079000cc data f7ffffff
    ```
  - 数据与SDRAM DQ之间映射关系，如下表：
    |Word Bits|31|30|29|28|27|26|25|24|23|22|21|20|19|18|17|16|15|14|13|12|11|10|9|8|7|6|5|3|2|1|0|
    |-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|
    |SDRAM DQ|15|14|13|12|11|10|9|8|7|6|5|4|3|2|1|0|15|14|13|12|11|10|9|8|7|6|5|4|3|2|1|0|
  - 地址映射关系，如下表：

    |Address Bits|31|30|29|28|27|26|25|24|23|22|21|20|19|18|17|16|15|14|13|12|11|10|9|8|7|6|4|3|2|1|0|
    |-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|
    |Original Bits| | | | |b2|b1|b0|r12|r11|r10|r9|r8|r7|r6|r5|r4|r3|r2|r1|r0|c10|c9|c8|c7||c5|c4|c3|c2|c1|c0|dw|
    |Remapped Bits| | | | |$\color{red}{r2}$|$\color{red}{r1}$|$\color{red}{r0}$|r12|r11|0|r9|r8|r7|r6|r5|r4|r3|$\color{red}{b2}$|$\color{red}{b1}$|$\color{red}{b0}$|c10|c9||c7|c6|c5|c4|c3|c2|c1|c0|dw|

    *注意：dw为data width，c为column，r为row，b为bank，remap将row低3位与bank 3位交换*

- Uboot中mtest命令

  - 在板级文件中添加DDR测试宏定义，例如：
    ```c
    u-boot/include/configs/x2000_halley5.h

    #define CONFIG_CMD_MEMTEST
    ```
  - 测试打印：
    ```
    U-Boot 2013.07-00009-g42a068fff-dirty (Dec 13 2024 - 16:49:15)

    Board: x2670_halley (Ingenic XBurst2 X2670 SoC)
    DRAM:  128 MiB
    Top of RAM usable for U-Boot at: 88000000
    Reserving 471k for U-Boot at: 87f88000
    Reserving 16388k for malloc() at: 86f67000
    Reserving 32 Bytes for Board Info at: 86f66fe0
    Reserving 128k for boot params() at: 87f68000
    Reserving 124 Bytes for Global Data at: 86f66f64
    Stack Pointer at: 86f66f48
    Now running in RAM - U-Boot at: 87f88000
    Enter 'CDT' mode.
    create CDT index: 0 ~ 6,  index number:7.
    params.magic : 0x726f6e   params.version : 0x2
    create CDT index: 6 ~ 37,  index number:32.
    nor flash quad mode is set, now use quad mode!
    *** Warning - bad CRC, using default environment

    In:    serial
    Out:   serial
    Err:   serial
    Net:   GMAC-9161
    Hit any key to stop autoboot:  0
    x2600_halley# mtest 0x80000000 0x86f60000
    Testing 80000000 ... 86f60000:
    Pattern 00000000  Writing...  Reading...Iteration:     89
    ```
    *注意：mtest命令参数中结束地址应小于 Stack Pointer at: $\color{red}{86f66f48}$*

- 文件系统中memtester程序

  memtester源码下载和编译不在本文档范围内，请自行搜索或者用编译好的程序push到文件系统中。
  ```
  # ./memtester 100M
  memtester version 4.6.0 (32-bit)
  Copyright (C) 2001-2020 Charles Cazabon.
  Licensed under the GNU General Public License version 2 (only).

  pagesize is 4096
  pagesizemask is 0xfffff000
  want 100MB (104857600 bytes)
  got  100MB (104857600 bytes), trying mlock ...locked.
  Loop 1:
    Stuck Address       : ok
    Random Value        : ok
    Compare XOR         : ok
    Compare SUB         : ok
    Compare MUL         : ok
    Compare DIV         : ok
    Compare OR          : ok
    Compare AND         : ok
    Sequential Increment: ok
    Solid Bits          : ok
    Block Sequential    : ok
    Checkerboard        : ok
    Bit Spread          : ok
    Bit Flip            : ok
    Walking Ones        : ok
    Walking Zeroes      : ok
  ```
  *注意：memtester \<mem>测试内存大小应小于free剩余内存大小。*

#### 2.2.2.6 DDR 调试
若DDR测试出现错误情况，尝试以下调试方式：

- 降低DDR频率

  DDR时钟源为MPLL，所以DDR降频时注意能否被MPLL整除，例如：

  ```
  u-boot/include/configs/x2000_halley5.h

  #define CONFIG_SYS_MPLL_FREQ		1500000000
  #define CONFIG_SYS_MEM_FREQ		100000000
  ```
  *注意：DDR频率可参考参数文件中CL&CWL配置中最小频率*

- KGD端ODT和DRV调试方法：

    1. 在DDR配置文件中定义了不同DDR类型的通用KGD配置，例如：
    ```c
    u-boot/include/configs/X2000_ddr.h

    #define CONFIG_LPDDR2_KGD_CONFIG             0x1
    #define CONFIG_LPDDR2_KGD_MR3_DS             0x1

    #define CONFIG_LPDDR3_KGD_CONFIG             0x1
    #define CONFIG_LPDDR3_KGD_MR3_DS             0x2
    #define CONFIG_LPDDR3_KGD_MR11_ODT           0x0
    ```

    2. 根据不同DDR型号，定义不同的KGD配置，例如：
    ```c
    u-boot/include/ddr/chip-v2/LPDDR3_W63AH6NKB_BI.h

    #define CONFIG_LPDDR3_W63AH6NKB_BI_KGD_CONFIG             0x1
    #define CONFIG_LPDDR3_W63AH6NKB_BI_KGD_MR3_DS             0x1
    #define CONFIG_LPDDR3_W63AH6NKB_BI_KGD_MR11_ODT           0x1
    ```


- PHY端ODR和DRV调试方法：

    1. 在ddr_innophy_x2000.c文件中添加调试宏定义`CONFIG_DDR_DRVODT_DEBUG`，例如：

      u-boot/arch/mips/cpu/xburst2/ddr_innophy_x2000.c
      #define CONFIG_DDR_DRVODT_DEBUG

    2. 编译uboot。

        参考 [u-boot编译](02_uboot开发手册.md#u-boot编译)。

    3. 编译烧录固件，并将其拷贝到烧录工具firmwares/{平台名称}/目录下，例如：

      make burner_x2000 -j
      cp spl/u-boot-spl.bin {烧录工具路径}/firmwares/x2000/spl.bin

    4. 烧录固件，并启动开发板，在uboot启动时调试自动运行，串口打印如下：

    ```
    U-Boot SPL 2013.07-00009-g42a068fff-dirty (Dec 13 2024 - 10:31:30)
    ERROR EPC bfc017f8
    CPA_CPAPCR:0640590d
    CPM_CPMPCR:0320490d
    CPM_CPEPCR:0190510d
    CPM_CPCCR:9a0b5510
    DDR: DDR3_W631GU6NG type is : DDR3
    drv is 0                     11111111111111111111111111111111
    drv is 1                     11111111111111111111111111111111
    drv is 2                     11111111111111111111111111111111
    drv is 3                     11111111111111111111111111111111
    drv is 4                     11111111111111111111111111111111
    drv is 5                     11111111111111111111111111111111
    drv is 6                     11111111111111111111111111111111
    drv is 7                     11111111111111111111111111111111
    drv is 8                     11111111111111111111111111111111
    drv is 9                     11111111111111111111111111111111
    drv is 10                    11111111111111111111111111111111
    drv is 11                    11111111111111111111111111111111
    drv is 12                    11111111111111111111111111111111
    drv is 13                    11111111111111111111111111111111
    drv is 14                    11111111111111111111111111111111
    drv is 15                    11111111111111111111111111111111
    drv is 16                    11111111111111111111111111111111
    drv is 17                    11111111111111111111111111111111
    drv is 18                    11111111111111111111111111111111
    drv is 19                    11111111111111111111111111111111
    drv is 20                    11111111111111111111111111111111
    drv is 21                    11111111111111111111111111111111
    drv is 22                    11111111111111111111111111111111
    drv is 23                    11111111111111111111111111111111
    drv is 24                    11111111111111111111111111111111
    drv is 25                    11111111111111111111111111111111
    drv is 26                    11111111111111111111111111111111
    drv is 27                    11111111111111111111111111111111
    drv is 28                    11111111111111111111111111111111
    drv is 29                    11111111111111111111111111111111
    drv is 30                    11111111111111111111111111111111
    drv is 31                    11111111111111111111111111111111
    U-Boot 2013.07-00007-g1156b6255-dirty (Dec 12 2024 - 18:00:12)
    Ingenic SoC Burner
    DRAM:  128 MiB
    Top of RAM usable for U-Boot at: 88000000
    Reserving 381k for U-Boot at: 87fa0000
    Reserving 16416k for malloc() at: 86f78000
    Reserving 32 Bytes for Board Info at: 86f77fe0
    Reserving 128k for boot params() at: 87f80000
    Reserving 124 Bytes for Global Data at: 86f77f64
    Stack Pointer at: 86f77f48
    Now running in RAM - U-Boot at: 87fa0000
    NAND:  0 MiB
    Enter 'CDT' mode.
    create CDT index: 0 ~ 6,  index number:7.
    MMC:   MSC: 0, MSC: 1
    Using default environment
    In:    serial
    Out:   serial
    Err:   serial
    USB_udc_probe
    jz_dwc2_udc_v1.1
    Hit any key to stop autoboot:  0
    usb_gadget_register_driver 87ffb35c
    burner# spi nor flash chip_id is : c84019
    create CDT index: 6 ~ 37,  index number:32.
    status register 1 = 0x0
    status register 2 = 0x2
    status register 3 = 0x0
    status register 1 = 0x0
    status register 2 = 0x0
    status register 3 = 0x0
    nor flash quad mode is set, now use quad mode!
    cloner->full_size = 30124
    the offset = 0
    the length = 10000
    SF: 262144 bytes @ 0x0 Erased: OK
    SF: 65536 bytes @ 0x0 write: OK
    SF: 65536 bytes @ 0x0 check: OK
    the offset = 10000
    the length = 10000
    SF: 65536 bytes @ 0x10000 write: OK
    SF: 65536 bytes @ 0x10000 check: OK
    the offset = 20000
    the length = 10000
    SF: 65536 bytes @ 0x20000 write: OK
    SF: 65536 bytes @ 0x20000 check: OK
    the offset = 30000
    the length = 124
    SF: 292 bytes @ 0x30000 write: OK
    SF: 292 bytes @ 0x30000 check: OK
    reset in 4ms
    U-Boot SPL 2013.07-00009-g42a068fff-dirty (Dec 13 2024 - 10:35:29)
    ERROR EPC 87fa0820
    CPA_CPAPCR:0300490d
    CPM_CPMPCR:04b0490d
    CPM_CPEPCR:0190510d
    CPM_CPCCR:9a0b5410
    DDR: X2600N-DDR3 type is : DDR3
    drv_value  is 00000000 odt_value is 00000000
    reset in 4ms
    U-Boot SPL 2013.07-00009-g42a068fff-dirty (Dec 13 2024 - 10:35:29)
    ERROR EPC 80001cdc
    CPA_CPAPCR:0300490d
    CPM_CPMPCR:04b0490d
    CPM_CPEPCR:0190510d
    CPM_CPCCR:9a0b5410
    DDR: X2600N-DDR3 type is : DDR3
    drv_value  is 00000000 odt_value is 00000000
    ---------------------------------------------------------err:81000000:00000000
    ---------------------------------------------------------err:81000040:00000000
    reset in 4ms
    ... //此处省略部分打印
    U-Boot SPL 2013.07-00009-g42a068fff-dirty (Dec 13 2024 - 10:35:29)
    ERROR EPC 80001cdc
    CPA_CPAPCR:0300490d
    CPM_CPMPCR:04b0490d
    CPM_CPEPCR:0190510d
    CPM_CPCCR:9a0b5410
    DDR: X2600N-DDR3 type is : DDR3
    drv_value  is 0000001f odt_value is 0000001f
    reset in 4ms
    U-Boot SPL 2013.07-00009-g42a068fff-dirty (Dec 13 2024 - 10:35:29)
    ERROR EPC 80001cdc
    CPA_CPAPCR:0300490d
    CPM_CPMPCR:04b0490d
    CPM_CPEPCR:0190510d
    CPM_CPCCR:9a0b5410
    DDR: X2600N-DDR3 type is : DDR3
    drv_value  is 0000001f odt_value is 0000001f
    drv is 0                     00000000000000000000000000000000
    drv is 1                     00000000000000000000000000000000
    drv is 2                     00000000000000000000000000000000
    drv is 3                     00000001000000000000001000010000
    drv is 4                     11111111111111111111111111111111
    drv is 5                     11111111111111111111111111111111
    drv is 6                     11111111111111111111111111111111
    drv is 7                     11111111111111111111111111111111
    drv is 8                     11111111111111111111111111111111
    drv is 9                     11111111111111111111111111111111
    drv is 10                    11111111111111111111111111111111
    drv is 11                    11111111111111111111111111111111
    drv is 12                    11111111111111111111111111111111
    drv is 13                    11111111111111111111111111111111
    drv is 14                    11111111111111111111111111111111
    drv is 15                    11111111111111111111111111111111
    drv is 16                    11111111111111111111111111111111
    drv is 17                    11111111111111111111111111111111
    drv is 18                    11111111111111111111111111111111
    drv is 19                    11111111111111111111111111111111
    drv is 20                    11111111111111111111111111111111
    drv is 21                    11111111111111111111111111111111
    drv is 22                    11111111111111111111111111111111
    drv is 23                    11111111111111111111111111111111
    drv is 24                    11111111111111111111111111111111
    drv is 25                    11111111111111111111111111111111
    drv is 26                    11111111111111111111111111111111
    drv is 27                    11111111111111111111111111111111
    drv is 28                    11111111111111111111111111111111
    drv is 29                    11111111111111111111111111111111
    drv is 30                    11111111111111111111111111111111
    drv is 31                    11111111111111111111111111111111
    GD25Q256DYIG 00c84019 00c84019
    ```

    5. 打印drv和odt遍历表中1为pass，0为fail, 根据打印结果，drv可选择4~31，odt可选择0~31。

    6. 检查DDR配置文件中通用配置是否符合要求，如若不符合，可在DDR参数文件中添加新配置。

    ```c
    u-boot/include/configs/X2000_ddr.h

    #define CONFIG_PHY_DRVODT_CONFIG             0x1
    #define CONFIG_PHY_PU_DRV_CMD                0x3
    #define CONFIG_PHY_PD_DRV_CMD                0x3
    #define CONFIG_PHY_PU_DRV_CK                 0x3
    #define CONFIG_PHY_PD_DRV_CK                 0x3
    #define CONFIG_PHY_PU_DRV_DQ7_0              0x3
    #define CONFIG_PHY_PD_DRV_DQ7_0              0x3
    #define CONFIG_PHY_PU_DRV_DQ15_8             0x3
    #define CONFIG_PHY_PD_DRV_DQ15_8             0x3
    #define CONFIG_PHY_PU_ODT_DQ7_0              0x5
    #define CONFIG_PHY_PD_ODT_DQ7_0              0x5
    #define CONFIG_PHY_PU_ODT_DQ15_8             0x5
    #define CONFIG_PHY_PD_ODT_DQ15_8             0x5
    ```

    ```c
    u-boot/include/ddr/chip-v2/LPDDR3_W63AH6NKB_BI.h

    #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_DRVODT_CONFIG             0x1
    #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PU_DRV_CMD                0x8
    #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PD_DRV_CMD                0x8
    #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PU_DRV_CK                 0x8
    #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PD_DRV_CK                 0x8
    #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PU_DRV_DQ7_0              0x8
    #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PD_DRV_DQ7_0              0x8
    #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PU_DRV_DQ15_8             0x8
    #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PD_DRV_DQ15_8             0x8
    #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PU_ODT_DQ7_0              0x8
    #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PD_ODT_DQ7_0              0x8
    #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PU_ODT_DQ15_8             0x8
    #define CONFIG_LPDDR3_W63AH6NKB_BI_PHY_PD_ODT_DQ15_8             0x8
    ```

    7. 配置完成后，重新编译uboot，烧录到板卡。
    8. 运行DDR测试程序，验证DDR稳定性。
