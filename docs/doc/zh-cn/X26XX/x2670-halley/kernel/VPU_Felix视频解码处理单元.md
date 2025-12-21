# VPU Felix视频解码处理单元

## 模块功能介绍

felix是h264视频解码器，可将h264格式视频数据解码为nv12格式。

## 驱动源码

驱动源码所在位置：

module_drivers/drivers/media/platform/ingenic-vcodec/felix/
```
├── felix_drv.c
├── felix_drv.h
├── felix_ops.c
├── felix_ops.h
├── libh264
└── Makefile
```

## 设备树配置

设备树所在位置：

kernel内核(version <= 5.10)dts文件路径：

***module_driver/dts/x2600.dtsi***

kernel内核(version > 5.10)dts文件路径：

***module_driver/dts/x2600/x2600.dtsi***

felix控制器描述：

```c
felix: felix@0x13300000 { 

    compatible = "ingenic,x2600-felix";

    reg = <0x13300000 0x100000>;

    interrupt-parent = <&core_intc>;

    interrupts = <IRQ_FELIX>;

    status = "disabled";

};
```

### 设备树默认配置

设备树默认编译会产生felix设备：

```
&felix {

    status = "okay";

};
```

### 设备树自定义配置

用户可根据实际需求关闭felix设备，将该节点设置为disabled：

```
&felix {

    status = "disaled";

};
```

## 内核编译配置

内核配置VIDEO_INGENIC_VCODEC，配置说明如下：

编译Kernelx.x.x内核配置如下：

```
Symbol: VIDEO_INGENIC_VCODEC [=y]

Type : tristate

Prompt: V4L2 driver for ingenic Video Codec

 Location:

 -> Ingenic device-drivers Configurations

 -> [VPU] Drivers

 Defined at module_drivers/drivers/media/platform/ingenic-vcodec/Kconfig:1

 Depends on: (SOC_X2000 [=n] || SOC_M300 [=n] || SOC_X2100 [=n] || SOC_X2600 [=y]) && VIDEO_DEV [=y] && VIDEO_V4L2 [=y]

 Selects: V4L2_MEM2MEM_DEV [=y] && VIDEOBUF2_DMA_CONTIG_INGENIC [=y] && VIDEOBUF2_DMA_CONTIG [=y]
```

```
Symbol: INGENIC_FELIX [=y]

Type : tristate

Prompt: [DEC] ingenic video felix

 Location:

 -> Ingenic device-drivers Configurations

 -> [VPU] Drivers

 -> V4L2 driver for ingenic Video Codec (VIDEO_INGENIC_VCODEC [=y])

 Defined at module_drivers/drivers/media/platform/ingenic-vcodec/Kconfig:29

 Depends on: (SOC_X2000 [=n] || SOC_M300 [=n] || SOC_X2100 [=n] || SOC_X2600 [=y]) && VIDEO_INGENIC_VCODEC [=y]
```

### 内核默认编译配置

内核默认打开Felix驱动，配置界面如下：

![2023-05-04 09-27-44屏幕截图](assets/VPU_Felix视频解码处理单元.0.png)

### 内核自定义编译配置

用户可根据实际需求去掉该驱动的配置。

## 内核版本差异

## 设备节点生成

驱动加载成功后生成以下节点：

felix的设备节点：

```
/dev/video2
```

## 应用程序使用说明

### v4l2-h264dec

h264解码测试参考IMPP库中的示例程序
