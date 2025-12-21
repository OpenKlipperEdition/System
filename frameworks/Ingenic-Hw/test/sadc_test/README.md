# SADC测试介绍
SADC提供的API, 封装的是通道的设备节点(/dev/ingenic_aux_(0~6))  
x1600, x2000, x2500是SADC模块  x26xx是IIO_SADC, 避免混淆2个模块  
## 简介
本测试SADC采集通道电压。利用稳压电源, 向aux0通道输入电压, sadc 进行采集转化
### 注意事项
```
1. 参考电压
   驱动默认配置，用户不用修改配置。
    x1600 VREF 3.3V
    x2000 VREF 1.8V
    x2500 VREF 1.8V
   注：VREF即：sadc通道的最大接受电压，输入电压或者其他信号(温度，湿度等)转换的电压避免超过VREF
2. 通道数目
    x1600 sadc 分辨率12位 4个通道 具体看原理图
    x2000 sadc 分辨率10位 6个通道 板级Extension Interface 具体看原理图
    x2500 sadc 分辨率12位 4个通道 板级Extension Interface 具体看原理图
3. 配置
x1600、x2000、x2500, 设备数和驱动配置相同
1.设备数：板级设备数存在, 无则添加
&sadc {
   status = "okay";
};
2.驱动：make menuconfig
   -->Ingenic device-drivers Configuration
      --> [SADC] Support for the Ingenic SADC core 选*
      --> [SADC] Support for the Ingenic SADC AUX  选*
```
## 目录结构
```
├── ihal_config.h           # 自定义配置文件
├── Makefile                # 编译文件
├── README.md               # 说明文件
└── sadc_test.c             # 测试文件
```
## 测试流程
### 输出功能
1. 初始化SADC通道     
2. 读取通道电压     
3. 读取10次取平均电压     
4. 反初始化SADC通道     
### 测试结果
aux0通道输入电压, 终端log打印sadc采集转化后的电压

