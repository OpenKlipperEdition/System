# Rotator图像旋转

## 模块功能介绍

介绍模块的基本功能，驱动实现的功能，驱动实现的架构和框架图。默认配置的功能，需要自己配置的功能等内容

* 输入格式支持：RGB888 ARGB88 RGB565 RGB555 ARGB1555 YUV422

* 输出格式支持： ARGB8888，RGB565，RGB555，YUV422

* 旋转角度：0°，90°，180°，270°，Horizontal mirror，Vertical mirror

* 不支持RGB和YUV格式互相转换，支持RGB格式互相转换。

## 驱动源码位置

驱动源码所在位置：

***module_drivers/drivers/media/platform/ingenic-rotate/***

## 设备树配置

设备树所在位置：

kernel内核(version <= 5.10)dts文件路径：

***module_driver/dts/x2000.dtsi***

kernel内核(version > 5.10)dts文件路径：

***module_driver/dts/x2000/x2000.dtsi***

ROTATE控制器描述：
```c
rotate : rotate@0x13070000 {

compatible="ingenic,x2000-rotate";

reg=<0x130700000 x10000>;

interrupt-parent=<&core_intc>;

interrupts=<IRQ_ROTATE>;

status="okay";

};
```

### 设备树默认配置

设备树默认编译会产生rotate设备。

### 设备树自定义配置

用户可根据实际需求关闭rotate设备，将该节点配置为disabled：
```c
&rotate {

status="disabled";

};
```

## 内核编译配置

内核配置VIDEO_INGENIC_ROTATE，配置说明如下：
```
Symbol: VIDEO_INGENIC_ROTATE [=y]

Type : tristate

Prompt: Ingenic rotate driver

 Location:

 -> Ingenic device-drivers Configurations

 -> [Rotator] Drivers

 Defined at module_drivers/drivers/media/platform/ingenic-rotate/Kconfig
```

## 内核模块差异

## 设备节点生成

驱动加载成功后生成以下节点：

***/dev/video0***

rotate的设备节点