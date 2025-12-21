# I2C测试介绍

## 简介

本测试主要用来测试I2C通信是否正常。

## 目录结构

```
.
├── ihal_config.h       # 自定义配置文件
├── Makefile            # 编译文件
├── README.md           # 说明文件
└── i2c_test.c          # 测试文件
```

## 测试自定义配置
在 ***ihal_config.h*** 文件中可以自定义配置I2C的参数，具体配置如下：
#define  I2C0_DEV_NAME "/dev/i2c-0"
#define  I2C1_DEV_NAME "/dev/i2c-1"
#if defined X2000_HALLEY5
#define  I2C2_DEV_NAME "/dev/i2c-2"
#define  I2C3_DEV_NAME "/dev/i2c-3"
#define  I2C4_DEV_NAME "/dev/i2c-4"
#define  I2C4_DEV_NAME "/dev/i2c-5"
#elif defined X2500_HIPPO
#define  I2C2_DEV_NAME "/dev/i2c-2"
#define  I2C3_DEV_NAME "/dev/i2c-3"
#elif defined X2600E_HALLEY
#define  I2C2_DEV_NAME "/dev/i2c-2"
#define  I2C3_DEV_NAME "/dev/i2c-3"

#define  ADDRLENGTH_8BIT    8      //I2C外设存储地址长度
#define  ADDRLENGTH_16BIT   16
#define  ADDRLENGTH_32BIT   32
#define  ADDRLENGTH_64BIT   64

1.在这个文件中根据板级的不同定义了不同的I2C设备节点名，客户在使用过程中直接使用想用的I2C总线即可.
2.因为I2C总线上可以接数几种外设，每个外设的从机地址各不相同，所以没有将所有的从机地址列举出来，在使用过程中，要确定好对应总线上的外设的地址，将从机地址封装成一个宏(例如 #define I2C_DEV_ADDR)。
3.在这个文件中还定义了三个常用的地址长度，在使用I2C读写函数时，需要将其中一个作为函数的第三个参数，如遇到特殊长度的地址，用户根据ihal_config.h的格式自行定义。

## 测试流程
(以读写x2000 i2c-2总线上的EEPROM为例)
1. 初始化I2C消息结构体IHal_I2C_Msg_t,根据自己实际需求初始化消息结构体的各个成员。
2.调用IHal_I2C_Init函数初始化I2C相关配置。
3.创建两个线程，一个线程用于读取数据，一个函数用于写入数据，将初始化完成的IHal_I2C_Msg_t结构体通过arg参数传给每一个线程。
4.a线程调用IHal_I2C_Read_Data函数读取指定地址的数据。b线程调IHal_I2C_Write_Dat函数将数据写入指定地址。

## 测试结果
	a线程向0x03出写入数据0x01，0x02，0x03，b线程从0x03处读取数据，读到的值和a进程写进去的值一样，证明I2C读写数据没问题。


## 注意事项
1.测试用例以多线程为例，展示API的功能，用户可以根据需要自行设计。
2.设备节点路径已经在ihal_config.h中定义好了，需要使用哪一个I2C总线就使用对应的宏定义即可。(例如：I2C1总线的宏定义为 I2C1_DEV_NAME)将这个参数传给IHal_I2C_Init函数即可。
3.客户需要自己对我们定义好的IHal_I2C_Msg结构体进行初始化，下面对该结构体的成员进行说明。
typedef struct{
	IHAL_UINT8 	SlaveAddr; 	//I2C从设备地址
	IHAL_UINT32 ReadAddr;   //想要进行读操作的起始地址(或寄存器地址)
	IHAL_UINT8  AddrLength；//地址长度
	IHAL_UINT8* tx_buf;    	//发送数据缓冲区
	IHAL_INT32  tx_len;     //想要发送数据的长度
	IHAL_UINT8* rx_buf;     //接收数据缓冲区
	IHAL_INT32  rx_len;   	//想要接收数据的长度
}IHal_I2C_Msg_t; 客户根据业务需求灵活的对该结构体成员进行初始化。
4.tx_buf的前几个字节(和地址长度有关)应该存放要写的地址，在地址后面存放要写的数据。如果地址长度大于一个字节，应将地址拆分成一个一个字节，从高字节写到低字节，后面再跟要写的数据。例如 向0x02的起始地址写0x01，0x02,0x03 那么IHAL_UINT8 tx_buf[]={0x02,0x01,0x02,0x03}；向0x1234的地址写0xFF，那么IHAL_UINT8 tx_buf[]={0x12,0x34,0xFF};
