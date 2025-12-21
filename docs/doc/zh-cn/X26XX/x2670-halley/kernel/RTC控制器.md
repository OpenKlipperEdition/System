# RTC控制器

## 模块功能介绍

实时时钟（RTC）单元可以在芯片主电源打开或主电源关闭但RTC电源仍然打开的情况下运行。在这种情况下，RTC电源域仅消耗几微瓦的功率。

## 驱动位置

驱动源码所在位置

***module_drivers/drivers/rtc/rtc-ingenic-v1.c***

## 设备树配置

设备树所在位置：

kernel内核(version <= 5.10)dts文件路径：

***module_driver/dts/x2600.dtsi***

kernel内核(version > 5.10)dts文件路径：

***module_driver/dts/x2600/x2600.dtsi***

RTC控制器描述：

```c
rtc: rtc@0x10003000 {

 compatible = "ingenic,rtc";

 reg = <0x10003000 0x4000>;

 interrupt-parent = <&core_intc>;

 interrupts = <IRQ_RTC>;

 status = "ok";

}; 
```

### 设备树默认配置

设备树默认编译不会产生RTC设备。

### 设备树自定义配置

## 内核编译配置

内核配置RTC_DRV_INGENIC，配置说明如下：

```
Symbol: RTC_DRV_INGENIC_V1 [=n]

 Type : tristate

 Prompt: INGENIC RTC V1

 Location:

 -> Ingenic device-drivers Configurations

 -> [RTC] drivers

 Defined at module_drivers/drivers/rtc/Kconfig:23 
```

### 内核默认编译配置

内核默认未配置RTC驱动，配置界面如下：

![](assets/RTC控制器.0.png)

![](assets/RTC控制器.1.png)

### 内核自定义编译配置

用户可根据实际需求添加该驱动的配置。

## 内核模块差异

无

## 设备节点生成

驱动加载成功后生成以下节点：

```
/dev/rtc0
```

## 应用程序使用说明

1. 设置系统的日期和时间

```
# date -s "2023-08-02 16:30"
```

2. 将系统时间同步到rtc

```
# hwclock -w
```

3. 读取rtc的时间

```
# hwclock -r
Wed Aug  2 16:30:10 2023  0.000000 seconds
```
