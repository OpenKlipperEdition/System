# IIO_SADC测试介绍
iio_sadc模块：x26xx芯片  
API基于以下访问2种方式封装, IIO子系统针对IIO DEVICE中各通道的访问方式主要提供两种,即sysfs方式访问、字符设备方式访问   
1. adc方式1:sysfs方式数据访问  （单通道采集）  
sysfs目录下通过IIO DEVICE提供的属性文件进行访问(如cat in_voltage0_raw)   
2. adc方式2:字符设备方式数据访问 (多通道扫描采集)  
针对IIO DEVICE的通道数据, 需要支持连续采集功能时, 则借助iio-trigger、iio-buffer实现数据的触发与存储功能, 同时应用程序可通过对应的iio buffer提供的字符设备文件, 访问存储的连续采集的数据.

## 简介
本测试SADC采集通道电压。利用稳压电源, 向通道输入电压, sadc 进行采集转化
### 注意事项
```
1. ADC 芯片
   x26xx参数: VREF:1.8V；12位ADC；16通道
   注：VREF即sadc通道的最大接受电压，输入电压或者其他信号(温度，湿度等)转换的电压避免超过VREF
2. 配置
----- x2600e -----
1.设备数：板级设备数存在, 无则添加  
&sadc {
     status = "okay";  
     pinctrl-names = "default";       
     pinctrl-0 = <&aux0_pe>, <&aux1_pe>;// 通道0、通道1
     seq1 = <0 1>;                      // seq1开启的通道和通道扫描顺序：通道0、通道1
     ingenic,has_dma_support = <0>;     // 0:不使用DMA  1:使用DMA
     seq1-cont-interval = <10>;         // seq1的连续转换间隔时间, 单位 ms
};
注意：pinctrl-0 = <&aux0_pe>,<&aux1_pe>; 避免引脚冲突。x2600e的aux0、aux1不冲突(空闲)
2.驱动：make menuconfig 确保 
    -->Device Drivers (前提)
       --> Industrial I/O support 选* (不用进入，后面的iio驱动开启会自动选择里面内容)
    -->Ingenic device-drivers Configuration
       -->[iio] drivers
            --> [X2600 SADC] X2600 sadc iio driver 选*
            --> [DEBUG] DEBUG sadc iio driver 选*

------ x2670 ------
1.设备数：板级设备数存在, 无则添加  
&sadc {
     status = "okay";  
     pinctrl-names = "default";       
     pinctrl-0 = <&aux0_pe>, <&aux1_pe>;// 通道0、通道1
     seq1 = <0 1>;                      // seq1开启的通道和通道扫描顺序：通道0、通道1
     ingenic,has_dma_support = <0>;     // 0:不使用DMA  1:使用DMA
     seq1-cont-interval = <10>;         // seq1的连续转换间隔时间, 单位 ms
};
注意：pinctrl-0 = <&aux0_pe>, <&aux1_pe>; 由于&aux0_pe、&aux1_pe引脚和i2c3冲突 ，测试时确保关闭i2c3即可  
&i2c3 {
     clock-frequency = <100000>;
     pinctrl-0 = <&i2c3_pe>;
     timeout = <1000>;
     pinctrl-names = "default";      //关闭
     status = "disable";
 
     rtc_pcf8563:rtc_pcf8563@0x51{
                 compatible = "nxp,pcf8563";
                 reg = <0x51>;
                 status = "disable"; //关闭
     };
 };

3.使用adc方式
1. adc方式1 单通道采集
    驱动本质：seq0模式、adc的cpu模式(不支持DMA模式). 在下面测试流程的adc方式1的步骤2, 每读取一次驱动颞部都会adc采集, cpu等待采集完成, 返回数据
    adc方式1只关注设备数seq1 = <0 1>; 内容代表开启的通道, 其他字段的无需关注, 但不可缺少。
2. adc方式2 多通道扫描采集
    驱动本质：seq1模式，支持DMA模式.iio框架的trigger模式,支持连续采集功能,trigger会触发adc开启扫描，数据存放在buff中
    seq1 = <0 1>;// 开启的通道和通道扫描顺序, 扫描的顺序，存放buff的数据顺序
    ingenic,has_dma_support = <0>;  // 0:不使用DMA  1:使用DMA   
    seq1-cont-interval = <10>;      // seq1的连续转换间隔时间, 单位 ms。一般无需修改
```
## 目录结构
```
├── ihal_config.h           # 自定义配置文件
├── Makefile                # 编译文件
├── README.md               # 说明文件
└── iio_sadc_test.c         # 测试文件

```
## 测试流程  
### adc方式1
1. 初始化SADC通道aux0  
2. 读取通道原始数据  
3. 反初始化SADC通道  
### adc方式2
1. ADC trigger初始化  
2. 使能通道aux0、aux1  
3. 设置序列buff长度  
4. ADC开始扫描采集  
5. ADC停止扫描采集  
6. 获取序列buff通道原始数据  
7. ADC trigger反初始化  
```
adc方式2 buff介绍
    如果设备数配置seq1 = <0 1 2 3 4>; 表示开启通道和扫描顺序0、1、2、3、4
    步骤2enable 通道0、1
    步骤3设置buff的长度length后, 实际的buff的大小 == length 乘以 enable的通道序列数数据。
    举例：设备数配置seq1 = <0 1 2 3 4>
        enable 通道0、1
        设置buff的长度length==2
        trigger开启采集一段时间，buff（fifo）填充满。buff的数据是{通道0 通道1 通道0 通道1}；
        步骤6的api屏蔽了buff序列,直接data1 == IHal_IIO_SADC_Trigger_Read_Sequence_Buff_Data, 代表拿走一次buff的序列，即拿走开头的通道0和通道1的数据。
        data1.channel_data[IIO_SADC_AUX0]）//获取aux0原始数据
        data1.channel_data[IIO_SADC_AUX1]）//获取aux1原始数据
    用户使用可根据实际情况，设置更大的buff的长度，buff会存放更多的数据
```
### 测试结果

通道输入电压, 终端log打印sadc采集转化后的原始数据(0-4095)