# SPI测试介绍

## 简介
本测试主要用于控制SPI_Master(SPI总线控制器)收发数据

## 目录结构
```
.
├── ihal_config.h	# 自定义配置文件
├── Makefile		# 编译文件
├── README.md		# 说明文件
└── spi_test.c		# 测试文件
```
## 使用前配置

1.默认的板级设备树中是没有打开spi的，需要在设备树中打开spi控制器节点
***注意：如果spi是gpio的复用功能，一定要注意引脚是否冲突***
2.在menuconfig中选中spi控制器
***注意：如果编译不成功查看menuconfig中Device Drivers -> SPI SUPPORT是否选中***

## 测试流程
(本模块的测试用例需要将SPI的SISO和SOSI短接，模拟总线传输，用于测试SPIDEV是否正常)
1. 定义一个IHal_SPI_attr结构体
2. 根据需求初始化结构体，具体参数的功能在其头文件中有相关注释
3. 调用IHal_SPI_Init初始化SPI传输
4. 使用初始化得到的句柄进行相应的操作

***注：在实际使用中SPI控制器控制的SPI总线需要挂载从机设备，我们这里采用SOC自带的SPI_SLV控制器模拟SPI从设备，***
***具体的例子可以看SPI_SLV相关的测试用例(其本质都是按照从设备规定的协议进行数据收发)。实际使用中连接的其它从***
***机设备的使用方法只需要连接好SPI总线，然后SPIMaster按照从机协议读写数据即可***

***注：spi.h头文件中定义的IHal_SPI_Transfer可以同时发送和接收(即全双工)，其余两个收发函数为单独发送或接收，也***
***IHal_SPI_Transfer函数的send_data或receiv_data可以设置为空，也可以实现单独发送或接收。用户可以自由选择***

## 测试结果

接收数据和发送数据相同





