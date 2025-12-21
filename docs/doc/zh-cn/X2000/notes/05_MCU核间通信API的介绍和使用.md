**核间通信API的介绍和使用**

# mcu核间通信实现原理

小核初始化时会设置两个环形缓冲区用于通信，分别命名为data_form_host和data_form_host(代码位于x2000_hal_mcu.c),并且将缓冲区地址放置在小核固件的头部结构中(代码位于start.S)．加载小核固件后，该头部结构位于大小核双方都可以访问的TCSM中，因此大核经过地址转换后也可以使用环形缓冲区．

大小核会分别注册一个中断，用于响应对方发送数据的通知．

加载并启动小核固件后，大核会注册一个misc decive作为通信节点，并通过设备文件/dev/mcu提供操作接口，用户层可通过系统调用使用该节点进行大核端的通信．

当大核向小核发送数据时，用户层将消息地址，长度，超时时间等信息组织成参数数组，使用 ioctl 系统调用传递发送命令码给设备驱动，驱动会将消息放入环形缓冲区，并给小核发送一个中断，小核接收到中断后读取消息．

当小核向大核发送数据时，调用发送接口，传参消息地址和长度，小核驱动会直接将消息放入环形缓冲区，并给大核发送一个中断，小核接收到中断后读取消息．

![](assets/MCU%E6%A0%B8%E9%97%B4%E9%80%9A%E4%BF%A1%E7%A4%BA%E6%84%8F%E5%9B%BE.jpg)

# 通信API介绍

## host端通信

驱动程序注册了一个 misc device 作为通信节点，并通过设备文件 /dev/mcu 提供接口。用户层使用系统调用该节点进行数据的收发，详细介绍如下：

驱动代码路径:kernel/module_drivers/drivers/remoteproc/mcu_remoteproc/mcu_msg

### 1. 打开设备文件

在进行任何通信之前，必须先打开设备文件以获得文件描述符。

```c
#include <fcntl.h>
#include <stdio.h>
#include "mcu_ioctl.h"  // 包含定义的头文件

int fd = open("/dev/mcu", O_RDWR);
if (fd < 0) {
    perror("Failed to open device file");
    return -1;
}
```

### 2. 发送数据

使用 ioctl 系统调用发送数据到目标处理器，并支持超时机制。

```c
#include <sys/ioctl.h>
#include <unistd.h>

// 设置写操作的数据
char msg[64];
sprintf(msg, "hello mcu!\n");
unsigned long arg[4] = {};                                
arg[0] = (unsigned long)msg; //消息地址
arg[1] = 64;　//消息长度
arg[2] = 500; //超时时间

// 执行 ioctl 写操作
ret = ioctl(fd, MCU_WRITE_DATA_TIMEOUT,arg);
if(ret == -1) {
    printf("mcu write data error!\n");
    return ret;
}

```

### 3. 接收数据

使用 ioctl 系统调用从目标处理器接收数据，并支持超时机制。

```c
char buf[64];;

// 设置读操作的数据
arg[0] = (unsigned long)buf; //接收buffer
arg[1] = 16; //接收长度
arg[2] = 2000; //超时时间，单位ms


// 执行 ioctl 读操作
ret = ioctl(fd, MCU_READ_DATA_TIMEOUT, arg);
if(ret == -1) {
    printf("mcu write data error!\n");
    return ret;
}

printf("Read from MCU: %s\n", (char *)buffer);
```

### 4. 关闭设备文件

完成所有通信后，关闭设备文件以释放资源。

```c
close(fd);
```

注意事项

    超时处理：在进行读写操作时，务必设置合理的超时时间，以防止程序陷入长时间等待状态。
    错误检查：每次系统调用后应检查返回值，确保操作成功。
    资源管理：确保每次打开设备文件后最终都调用 close 来释放资源。


## remote端通信API

本章节详细介绍了小核端核间通信的API。这些API提供了在mcu上发送和接收数据的功能，并支持通过软中断通知主机端进行处理。API包括初始化、消息发送、消息接收以及设置接收回调函数等功能。

驱动代码路径：libbare-cpu/drivers/drivers-x2000/src/x2000_hal_mcu.c

### 1. HAL_MCU_MsgInit

【函数原型】void HAL_MCU_MsgInit(void);

【功能描述】初始化核间通信模块，注册软中断处理程序，必须在使用任何通信功能之前调用此函数。。

【参数说明】无

【返回值】 无

### 2. HAL_MCU_SetRecvCallback

【函数原型】HAL_MCU_SetRecvCallback

【功能描述】设置接收消息的回调函数。

【参数说明】

| 参数 | 描述 |
| --- | --- |
|cb|指向回调函数的指针。每当有新消息到达时，软中断处理程序将调用此回调函数。|

【返回值】 无


### 3. HAL_MCU_MsgSend

【函数原型】int HAL_MCU_MsgSend(void *buf, unsigned int size);

【功能描述】向主机端发送消息，该函数会等待主机端空闲，并通过环形缓冲区逐段写入数据。发送完成后，会通知主机端有新的数据可用。

【参数说明】

| 参数 | 描述 |
| --- | --- |
| buf | 指向要发送的数据缓冲区的指针。 |
| size | 要发送的数据长度（字节数） |

【返回值】 
    成功：返回实际发送的字节数。
    失败：返回负数错误码（如 -EAGAIN 表示主机端忙或发送失败）。


### 4. HAL_MCU_MsgRecv

【函数原型】int HAL_MCU_MsgRecv(void *buf, unsigned int size);

【功能描述】从主机端接收消息, 该函数从环形缓冲区中逐段读取数据到指定的缓冲区中，直到读取到所有数据或达到最大长度。

【参数说明】

| 参数 | 描述 |
| --- | --- |
| buf | 指向接收数据缓冲区的指针 |
| size | 期望接收的数据长度（字节数） |

【返回值】 
    成功：返回实际接收到的字节数。
    失败：返回负数错误码（如 -EAGAIN 表示没有更多数据可读）。


# 核间通信应用实例

## host端软件实现

### menuconfig

```c
CONFIG_INGENIC_MCU_RPROC=y
```

### host端测试用例实现

host端测试用例代码源码位置：

```c
 libbare-cpu/projects/x2000-halley5/Examples/mcu-msg/xburst2_app
```

该测试用例首先打开通信端点，分别测试一发一收的情况和连续发送的情况，最后销毁通信端点．

## remote端软件实现

### 工程路径

```markdown
libbare-cpu/projects/x2000-halley5/Examples/mcu-msg
```

### remote端测试用例实现

测试用例首先调用HAL_MCU_MsgInit接口进行通信的初始化，然后调用HAL_MCU_SetRecvCallback接口设计接收消息的回调函数，在回调函数中接收并打印收到的消息，然后向host端发出消息

## 实验步骤

1.编译mcu工程生成mcu_msg.bin

2.将固件放在文件系统/lib/firmware目录下并改名为libmcu-bare.bin

3.大核通过remoteproc状态控制启动小核

```console
echo 1 > /sys/devices/platform/ahb2/13420000.mcu/load_fw
```

４.启动小核后出现如下打印，证明小核正常启动
```console
mcu startup~
```
出现如下打印证明通信设备节点注册成功
```console
MCU msg device regist OK!
```

此时会在/dev目录下生成mcu节点.


5.编译工程中的xburst2_app测试用例，执行进行测试，测试log如下
```console
# ./mcu_msg_test 
===mcu receive a message===
hello mcu!

          app receive msg:hello host!

===mcu receive a message===
hello mcu 0!
            ===mcu receive a message===
hello mcu 1!
            ===mcu receive a message===
hello mcu 2!
            ===mcu receive a message===
hello mcu 3!
            ===mcu receive a message===
hello mcu 4!
            ===mcu receive a message===
hello mcu 5!
            ===mcu receive a message===
hello mcu 6!
            ===mcu receive a message===
hello mcu 7!
            ===mcu receive a message===
hello mcu 8!
            ===mcu receive a message===
hello mcu 9!
            ===mcu receive a message===
hello mcu 10!
             ===mcu receive a message===
hello mcu 11!
             ===mcu receive a message===
hello mcu 12!
             ===mcu receive a message===
hello mcu 13!
             ===mcu receive a message===
hello mcu 14!
             ===mcu receive a message===
hello mcu 15!
             ===mcu receive a message===
hello mcu 16!
             mcu send data error!
===mcu receive a message===
hello mcu 17!
             mcu send data error!
===mcu receive a message===
hello mcu 18!
             mcu send data error!
===mcu receive a message===
hello mcu 19!
             mcu send data error!
===mcu receive a message===
hello mcu 20!
             mcu send data error!
===mcu receive a message===
hello mcu 21!
             mcu send data error!
===mcu receive a message===
hello mcu 22!
             mcu send data error!
===mcu receive a message===
hello mcu 23!
             mcu send data error!
===# mcu receive a message===
hello mcu 24!
             mcu send data error!
```

6.关闭小核结束测试
```console
echo 1 > /sys/devices/platform/ahb2/13420000.mcu/shutdown 
```

大核首先给小核发送信息hello muc!，小核接收并打印该信息，然后回消息hello host!给大核，大核打印该消息．
随后大核连续向小核发送消息25次，小核接收打印消息并回复，由于大核不读取小核发送的消息，小核会在发送buffer被填满后发生发送异常，报错mcu send data error.

 

