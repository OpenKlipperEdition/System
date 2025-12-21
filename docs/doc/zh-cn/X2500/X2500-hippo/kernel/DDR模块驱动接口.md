# DDR模块

## 模块功能介绍

DDRC（DDR控制器）是一个通用IP，它提供了LPDDR2和LPDDR3存储器的接口。DDRC IP专为SOC使用而设计，可配置、可扩展以满足各种SOC的要求。

## 驱动源码

驱动源码所在位置：

arch/mips/xburst2/common/cpu_ddr_test

├── ddr_bandwidth_monitor.c

├── ddr_bus_protection.c

├── ddr_change_freq.c

## 设备树配置

DDR模块无设备树配置。

## 内核编译配置

Symbol: XBURST2_CPU_TEST [=y] 

Type : boolean 

Prompt: xburst2 cpu ddr test 

 Location: 

 -> Machine selection 

(1) -> SOC Type Selection 

 Defined at arch/mips/xburst2/Kconfig:139

 Depends on: MACH_XBURST2 [=y] 

## 设备节点生成

驱动加载成功后生成以下debugfs文件节点，需要手动挂载debugfs：

***mount -t debugfs none /mnt***

***/mnt***

├── ddr

├── ddr_config

├── ddrfreq

## 应用程序使用说明

### 带宽监测

####  Debug节点

***/mnt/ddr***

***ahb2_read_rate cpu_read_rate monitors periods***

***ahb2_write_rate cpu_write_rate output run***

#### 命令行及参数示意

1. monitors节点可配置、显示监测的通道，配置需按如下格式，最多可同时监测四通道

***#echo 6:3:1:0　> monitors***

***# cat monitors*** 

***monitor:0, chn:6, desc:cpu***

 ***read:3046, write:1126***

***monitor:1, chn:3, desc:dpu&cim***

 ***read:279604, write:0***

***monitor:2, chn:1, desc:vpu&isp***

 ***read:0, write:0***

***monitor:3, chn:0, desc:gmac&msc***

 ***read:0, write:0***

通道号与模块对应关系如下

***{0, "gmac&msc"},*** 

***{1, "vpu&isp"},*** 

***{3, "dpu&cim"},*** 

***{5, "ahb2&audio&apb"},***

***{6, "cpu"}*** 

2. Period可配置采样的频率，默认25000000hz

***# echo 20000000 > periods*** 

***# cat periods*** 

***20000000***

3. output配置开启、关闭打印log信息
4. run配置启动带宽监测，运行后会依据output配置打印log信息，如下所示：

***echo 1 > run***

***[60712.288103] cpu chn:6 write occupancy rate:0.036%***

***[60712.292990] cpu chn:6 read occupancy rate:0.097%***

***[60712.297757] dpu&cim chn:3 write occupancy rate:0.000%***

***[60712.303142] dpu&cim chn:3 read occupancy rate:8.947%***

***[60712.308263] vpu&isp chn:1 write occupancy rate:0.000%***

***[60712.313602] vpu&isp chn:1 read occupancy rate:0.000%***

***[60712.318725] gmac&msc chn:0 write occupancy rate:0.000%***

***[60712.324148] gmac&msc chn:0 read occupancy rate:0.000%***

### 时钟频率

####  Debug节点

***/mnt/ddrfreq***

***ddr_cur_rate rate***

#### 命令行及参数示意

1. ddr_cur_rate、rate节点分别显示ddr时钟频率，单位Hz、MHz。

***# cat ddr_cur_rate***

***500000000***

***# cat rate***

***500***

### 参数配置

####  Debug节点

***/mnt/ddr_config***

***ddr_ahb_protection ddr_driver_strength***

***ddr_apb_protection ddr_odt***

***ddr_transaction_priority ddr_port_priority***

***ddr_bandwidth limit***

#### 命令行及参数示意

1. ddr_ahb_protection、ddr_apb_protection节点分别配置ddr ahb、apb总线上的寄存器保护，开启总线保护时，会记录是否发生未许可的读写访问。

***# cat ddr_ahb_protection***

***disable***

***# echo 1 > ddr_ahb_protection***

***# cat ddr_ahb_protection***

***enable, un-protected register write not occurred***

2. ddr_driver_strength节点配置ddr驱动强度，范围0~31.

***# cat ddr_driver_strength***

***22 (range 0 ~ 31), 34.35(ohm)***

***# echo 31 > ddr_driver_strength*** 

***# cat ddr_driver_strength***

***31 (range 0 ~ 31), 20.91(ohm)***

3. ddr_transaction_priority节点配置ddr仲裁计数，可读写通道单独配置，格式如下，一般情况下计数越小，仲裁获得优先级越高。

***# echo 5:10:5 > ddr_transaction_priority***

***# echo 6:15 > ddr_transaction_priority***

***# cat ddr_transaction_priority***

***range is (0~15)***

***chn:0, desc:gmac&msc, write priority:8, read priority:8***

***chn:1, desc:vpu&isp, write priority:8, read priority:8***

***chn:3, desc:dpu&cim, write priority:8, read priority:8***

***chn:5, desc:ahb2&audio&apb, write priority:10, read priority:5***

***chn:6, desc:cpu, write priority:15, read priority:15***

4. ddr_port_priority节点配置ddr端口优先级，命令格式如下，1表示高优先级，0表示低优先级。

***# echo 6:1>*** ddr_port_priority

***# cat*** ddr_transaction_priority

range is (0~1)

***chn:6, desc:cpu，***priority：1

***chn:5, desc:ahb2&audio&apb，***priority：0

***chn:3, desc:dpu&cim，***priority：0

***chn:1, desc:vpu&isp，***priority：0

***chn:0, desc:gmac&msc，***priority：0

5. ddr_bandwidth_limit节点配置ddr各port带宽限制，可以读写通道单独配置，格式如下，配置值越低，带宽越小，输入-1可以关闭带宽限制。

***# echo 6:128 > ddr_bandwidth_limit*** 

***# echo 5:32:-1 > ddr_bandwidth_limit*** 

***# cat ddr_bandwidth_limit***

***range is (-1~255)***

***chn:0, desc:gmac&msc***

 ***write:64,disable***

 ***read:64.disable***

***chn:1, desc:vpu&isp***

 ***write:64,disable***

 ***read:64.disable***

***chn:3, desc:dpu&cim***

 ***write:64,disable***

 ***read:64.disable***

***chn:5, desc:ahb2&audio&apb***

 ***write:32,enable***

 ***read:64.disable***

***chn:6, desc:cpu***

 ***write:128,enable***

 ***read:128.enable***

