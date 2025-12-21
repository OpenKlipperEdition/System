# SPI_SLV测试介绍

## 简介
本测试主要用于控制SPI_SLV(SPI从设备控制器)收发数据，使用测试可以测试SPI_SLV控制器是否正常，且使用SPI_Master和SPI_SLV通讯可以再次验证SPI_Master是否正常

## 目录结构
```
.
├── ihal_config.h	# 自定义配置文件
├── Makefile		# 编译文件
├── README.md		# 说明文件
└── spi_slv_test.c		# 测试文件
```
## 使用前配置

1. 关闭SPI_SLV的其他复用功能(防止GPIO冲突)
2. 根据SPI_Master的README打开SPI_Master
3. 在menuconfig中选中Ingenic series SPI slave driver 和Ingenic SoC SSI_SLV controller 0 for SPI SLV Host driver
```
Location:
-> Ingenic device-drivers Configurations
        -> [SPI] Master/Slave Drivers 
                -> Ingenic series SPI slave driver
```
(注:如果关闭了SPI_SLV引脚复用的其他功能，需要在menuconfig中关闭对应的驱动配置)
4. SPI_SLV通信的时候可能会因为通信速率问题导致数据不正确。这是因为ingenic的SPI_SLV有相应的时序要求(当cpoh=0时，需要加延时)。当SPI_Master使用DMA模式时，如果通信速率太小可能需要将延时增大。
***改变延时的方法：***
```
#ifdef SPI_SLV_TEST
		// 在内核中将这个函数的参数改变即可，延时时间为n x dev_clk period
        set_spi_frame_interval(ingspi, 0x4);
#endif
```

## 测试流程
(本模块的测试用例需要将SPI_Master和SPI_SLV链接，模拟SPI_SLV挂在到SPI总线上或SPI_Master总线上挂在从机设备，测试数据传输是否正常)
1. 定义一个IHal_SPI_SLV_attr结构体
2. 根据需求初始化结构体，具体参数的功能在其头文件中有定义
***(注：对于传输数据的长度，最大的单次传输长度为4096字节，如需更长的数据，请手动分发)***
3. 调用IHal_SPI_SLV_Init初始化传输
4. 使用初始化得到的句柄进行相应的操作
5. 在串口中运行spi_slv_test
6. 在adb shell中运行spi_test(这是SPI_Master的测试程序)

***注：使用本方法测试时，需要注意SPI_Master的相位极性设置以及传输数据大小要和SPI_SLV相同。在实际使用SPI控制器控制的SPI总线需要挂在从机设备，我们这里采用SPI_SLV控制器模拟SPI从机设备。***
