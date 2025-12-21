#ifndef __IHAL_GPIO_CONFIG_H__
#define __IHAL_GPIO_CONFIG_H__

#define  I2C0_DEV_NODE "/dev/i2c-0"
#define  I2C1_DEV_NODE "/dev/i2c-1"

#define  ADDRLENGTH_8BIT    8      //I2C外设存储地址的位数
#define  ADDRLENGTH_16BIT   16
#define  ADDRLENGTH_32BIT   32
#define  ADDRLENGTH_64BIT   64

#if defined X1600_HALLEY6
	#define  I2C2_DEV_NODE "/dev/i2c-2"
	#define  I2C3_DEV_NODE "/dev/i2c-3"
#elif defined X2000_HALLEY5
	#define  I2C2_DEV_NODE "/dev/i2c-2"
	#define  I2C3_DEV_NODE "/dev/i2c-3"
	#define  I2C4_DEV_NODE "/dev/i2c-4"
	#define  I2C5_DEV_NODE "/dev/i2c-5"
#elif defined X2500_HIPPO
	#define  I2C2_DEV_NODE "/dev/i2c-2"
	#define  I2C3_DEV_NODE "/dev/i2c-3"
#elif defined X2600E_HALLEY
	#define  I2C2_DEV_NODE "/dev/i2c-2"
	#define  I2C3_DEV_NODE "/dev/i2c-3"
#elif defined X2670_HALLEY
	#define  I2C2_DEV_NODE "/dev/i2c-2"
	#define  I2C3_DEV_NODE "/dev/i2c-3"
#elif defined X270M_HARE
	#define  I2C2_DEV_NODE "/dev/i2c-2"
	#define  I2C3_DEV_NODE "/dev/i2c-3"
#elif defined X2600_HALLEY7
	#define  I2C2_DEV_NODE "/dev/i2c-2"
	#define  I2C3_DEV_NODE "/dev/i2c-3"
#elif defined X2670_HARE
	#define  I2C2_DEV_NODE "/dev/i2c-2"
	#define  I2C3_DEV_NODE "/dev/i2c-3"


#endif
#endif






