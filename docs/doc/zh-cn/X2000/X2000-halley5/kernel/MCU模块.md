# MCU模块

## 模块功能介绍

MCU在PDMA中是一个迷你CPU，它兼容XBurst-1指令集架构（ISA），但不实现CACHE、MMU（内存管理单元）、DEBUG调试功能、FPU（浮点运算单元）和MXU（矩阵运算单元）。

内核使用remoteproc对mcu进行远程管理，能实现mcu固件的加载，启动/停止mcu，与mcu进行核间通信等功能

## 驱动位置

驱动源码所在位置：

```
module_drivers/drivers/remoteproc/mcu_remoteproc$ tree .

├── Makefile
├── ingenic_mcu.c  
├── ingenic_mcu.h  
└── mcu_msg  
    ├── Makefile  
    ├── assert.c  
    ├── assert.h  
    ├── bit_field.h  
    ├── mcu_firmware.h  
    ├── mcu_ioctl.h  
    ├── mcu_msg.c  
    ├── mcu_msg.h  
    ├── ring_mem.c  
    ├── ring_mem.h  
    └── utils  
        ├── clock.c  
        └── clock.h
```

## 设备树配置

设备树所在位置：

kernel内核(version <= 5.10)dts文件路径：

***module_driver/dts/x2000.dtsi***

kernel内核(version > 5.10)dts文件路径：

***module_driver/dts/x2000/x2000.dtsi***

PDMA控制器描述：

```c
mcu: mcu@0x13420000 {
        compatible = "ingenic,x2000-mcu";
        reg = <0x13420000 0x10000>;
        interrupt-parent = <&core_intc>;
        interrupt-names = "pdmam";
        interrupts = <IRQ_PDMAM>;
        ingenic,tcsm_size = <16384>;
};
```

### 设备树默认配置

默认编译会产生mcu设备

### 设备树自定义配置

用户可根据实际需求关闭mcu设备，可以将该节点配置为disable。

```c
&mcu {
 status = "disable";
};
```

## 内核编译配置

内核配置INGENIC_PDMAC，配置说明如下：

```
Symbol: INGENIC_MCU_RPROC [=y]
Type  : tristate
Defined at module_drivers/drivers/remoteproc/Kconfig:2
    Prompt: ingenic mcu remoteproc support
    Depends on: HAS_DMA [=y] && SOC_X2000 [=y]
    Location: 
        -> Ingenic device-drivers Configurations
(1)         -> [Remoteproc] drivers
Selects: REMOTEPROC [=y]                                   
```

### 内核默认编译配置

内核默认配置MCU驱动， 

配置界面如下：

![](assets/MCU模块.0.jpg)

### 内核自定义编译配置

用户可根据实际需求，去掉该驱动的配置。

## 版本差异

无

## 设备节点生成

### 控制器节点

驱动加载成功后生成以下控制器节点：

***/sys/devices/platform/ahb2/13420000.mcu***

### 核间通信节点

小核加载启动后，生成以下节点用于核间通信：

***/dev/mcu***

## 应用程序使用说明

### template-mcu

用于测试远程启停mcu

#### 源码位置

***libbare-cpu/projects/x2000-halley5/Templates/template-mcu***

#### 使用方法
1. 编译工程生成template.bin

2. 将生成的固件放在文件系统/lib/firmware目录下并改名为libmcu-bare.bin

3. 大核通过remoteproc状态控制启动小核

```bash
# echo 1 > /sys/devices/platform/ahb2/13420000.mcu/load_fw
```

4. 大核通过remoteproc状态控制关闭小核

```bash
#echo 1 > /sys/devices/platform/ahb2/13420000.mcu//shutdown
```

#### 测试log
小核启动输出如下内容，小核启动正常
```bash
# echo 1 > /sys/devices/platform/ahb2/13420000.mcu/load_fw 
[21801.998964] fw: c01ab000: 10000006 00000000 534f5452 f4000000  ........RTOS....
[21802.006563] fw: c01ab010: f40007b8 f400079c f4000784 40806000  .............`.@
[21802.014145] fw: c01ab020: 3c1af400 275a07b8 3c1bf400 277b2218  ...<..Z'...<."{'
[21802.021730] fw: c01ab030: 135b0005 00000000 af400000 275a0004  ..[.......@...Z'
[21802.029274] fw: c01ab040: 175bfffd 00000000 3c1df400 27bd2318  ..[........<.#.'
[21802.036858] fw: c01ab050: 3c1af400 275a0154 0340f809 00000000  ...<T.Z'..@.....
[21802.044432] fw: c01ab060: 00000000 00000000 00000000 00000000  ................
[21802.052011] fw: c01ab070: 00000000 00000000 00000000 00000000  ................
----- he[llo ---- mcu2 ---1-!
802.059801] MCU msg device regist OK!
# echo 1 > /sys/devices/platform/ahb2/13420000.mcu/shutdown
```

### mcu-msg

用于测试大小核核间通信

#### 源码位置

***libbare-cpu/projects/x2000-halley5/Example/mcu-msg***

#### 使用方法

1. 编译工程生成mcu-msg.bin

2. 将生成的固件放在文件系统/lib/firmware目录下并改名为libmcu-bare.bin

3. 大核通过remoteproc状态控制启动小核

```bash
# echo 1 > /sys/devices/platform/ahb2/13420000.mcu/load_fw
```
4. 编译工程中的xburst2_app测试用例，执行进行通信测试

#### 测试log

小核启动正常打印mcu startup~, 之后./mcu_msg_test执行通信测试，大核首先给小核发送信息hello muc!，小核接收并打印该信息，然后回消息hello host!给大核，大核打印该消息． 随后大核连续向小核发送消息25次，小核接收打印消息并回复，由于大核不读取小核发送的消息，小核会在发送buffer被填满后发生发送异常，报错mcu send data error.
```bash
# echo 1 > /sys/devices/platform/ahb2/13420000.mcu/load_fw   39.189586] fw: c01a7000: 10000006 00000000 534f5452 f4000000  ........RTOS....
[8Dload_fw 
[   39.202482] fw: c01a7010: f4000f00 f4000ee4 f4000ecc 40806000  .............`.@
[   39.211221] fw: c01a7020: 3c1af400 275a0f00 3c1bf400 277b2964  ...<..Z'...<d){'
[   39.218775] fw: c01a7030: 135b0005 00000000 af400000 275a0004  ..[.......@...Z'
[   39.226422] fw: c01a7040: 175bfffd 00000000 3c1df400 27bd2a68  ..[........<h*.'
[   39.233997] fw: c01a7050: 3c1af400 275a0310 0340f809 00000000  ...<..Z'..@.....
[   39.241621] fw: c01a7060: 00000000 00000000 00000000 00000000  ................
[   39.249165] fw: c01a7070: 00000000 00000000 00000000 00000000  ................
mcu [startup~
   39.257062] MCU msg device regist OK!
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

### mcu-systick

用于测试mcu系统时钟

#### 源码位置

***libbare-cpu/projects/x2000-halley5/Example/mcu-systick***

#### 使用方法

1. 编译工程生成template.bin

2. 将生成的固件放在文件系统/lib/firmware目录下并改名为libmcu-bare.bin

3. 大核通过remoteproc状态控制启动小核

```bash
#echo 1 > /sys/devices/platform/ahb2/13420000.mcu/load_fw
```

#### 测试log

小核按照测试用例所写，每秒输出一条打印

```bash
# echo 1 > /sys/devices/platform/ahb2/13420000.mcu/load_fw 
[  712.400911] fw: c01ab000: 10000006 00000000 534f5452 f4000000  ........RTOS....
[  712.408460] fw: c01ab010: f4000e54 f4000e2c f4000e14 40806000  T...,........`.@
[  712.416137] fw: c01ab020: 3c1af400 275a0e54 3c1bf400 277b28b4  ...<T.Z'...<.({'
[  712.423728] fw: c01ab030: 135b0005 00000000 af400000 275a0004  ..[.......@...Z'
[  712.431363] fw: c01ab040: 175bfffd 00000000 3c1df400 27bd29b8  ..[........<.).'
[  712.438912] fw: c01ab050: 3c1af400 275a0178 0340f809 00000000  ...<x.Z'..@.....
[  712.446534] fw: c01ab060: 00000000 00000000 00000000 00000000  ................
[  712.454092] fw: c01ab070: 00000000 00000000 00000000 00000000  ................
----- hello ---- [m cu ----!
 712.461993] MCU msg device regist OK!
# ----- hello ---- mcu ----!
----- hello ---- mcu ----!
----- hello ---- mcu ----!
----- hello ---- mcu ----!
----- hello ---- mcu ----!
----- hello ---- mcu ----!
----- hello ---- mcu ----!
......
```
