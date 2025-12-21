
# TCU测试介绍

## 简介

本测试主要用来测试TCU是否正常工作

## 目录结构

```
.
├── ihal_config.h       # 自定义配置文件
├── Makefile            # 编译文件
├── README.md           # 说明文件
└── tcu_test.c          # 测试文件
```

## 测试自定义配置
在 ***ihal_config.h*** 文件中可以自定义配置TCU的参数，具体配置如下：
#define DEVNAME  "sys/devices/platform/apb/10002000.tcu/enable"
#define CONTROLLER_ADDR   0x10002000

注：x1600、x2000、x2500TCU控制器的地址相同，设备节点名字相同，所以在ihal_config.h里进行统一的宏定义。


#if defined  X2600E_HALLEY
#define TCU0_DEVNAME   "/sys/devices/platform/ahb_mcu/13630000.tcu0 enable"
#define TCU1_DEVNAME   "/sys/devices/platform/ahb_mcu/13640000.tcu1 enable"
#define TCU0_CONTROLLER_ADDR  0x13630000
#define TCU1_CONTROLLER_ADDR  0x13640000
注：1.x26xx系列有两个tcu，分别为tcu0和tcu1，所以需要对两个tcu控制器分别定义，在使用时需要进行区分。
2.在使用API时，先通过source build/envsetup.h选则对应的板级，然后在代码中给函数传对应的参数即可。

## 测试流程

以x2000为例，测试TCU的GENERAL_MODE,GATE_MODE,DIRECTION_MODE,QUADRATURE_MODE以及POS_MODE.(具体的测试方法参考
ingenic-linux-docs/zh-cn /X26XX/notes/tcu功能测试说明文档.md)

## 测试结果
测试结果符合每个模式的预期

## 注意事项
1.在tcu_teset.c中 我为每一种模式定义了一个宏，默认状态为GENERAL等于1,其他模式等于0，在使用时，想测试哪一种模式就把对应模式的DEBUG宏 改为1 其他模式的改为零(同一时刻最多只能存在一个1)。
例如我想测试POS_MODE 宏定义如下：
#define DEBUG_GENERAL_MODE 0 
#define DEBUG_GATE_MODE  0
#define DEBUG_DIRECTION_MODE  0
#define DEBUG_QUADRATURE_MODE  0
#define DEBUG_POS_MODE   1

2.进行测试之前要根据ingenic-linux-docs/zh-cn /X26XX/notes/tcu功能测试说明文档.md的描述进行配置。
3.因为GENERAL_MODE,GATE_MODE,DIRECTION_MODE的配置相同，所以这三种模式适用同一套代码，只需要给tcu_general_gate_direction_mode()函数的第三个参数传对应的模式即可。
4.CAPTURE_MODE、FILTER_MODE、TCUGpiotrigger这三种模式都需要对相应的驱动进行配置，所以在tcu_test.c中并没有体现，如果使用者有需要，可以根据上文提到的文档，自行配置自行编写代码。遇到需要对GPIO拉高拉低的操作，可以参考tcu_test.c中的方法，调用本库gpio.c中的函数来实现。
5.x2500仅有通用模式，所以当使用x2500系列芯片时，IHal_TcuEnable函数的第三个参数仅传GENERAL_MODE即可，传别的参数得到的结果无效。
6.封装TCU的api的核心思想就是读取TCU_COUNTn(n为所选择的tcu通道 n的范围为0~7)寄存器中的数据，所以在应用层我们需要对TCU_COUNTn寄存器的物理地址进行映射。将TCU控制器的地址作为映射的起始地址，映射大小为4096即可，将映射后的地址作为参数传递给IHal_TcuGetCount函数的第一个参数，该函数的返回值即为TCU的数据。本示例在tcu_test.c中给出了地址映射函数和取消地址映射函数的实现，使用者可直接使用也可根据需要自行修改。总之，IHal_TcuGetCountH函数的第一个参数需要的是TCU的物理地址映射得到的虚拟地址。