# SADC控制器

## 模块功能介绍

X26xx系列芯片内置12bit ADC，具有16个模拟通道，有三个可自定义的转换序列。支持单次，连续，扫描等多种转换模式。具有看门狗功能，能检测输入电压是否超过用户设置的阈值。

## 驱动源码位置

驱动源码所在位置：

***module_drivers/drivers/iio/adc/ingenic-adc.c***

## 设备树配置

设备树所在位置

kernel内核(version <= 5.10)dts文件路径：

***module_driver/dts/x2600.dtsi***

kernel内核(version > 5.10)dts文件路径：

***module_driver/dts/x2600/x2600.dtsi***

SADC控制器描述

```c
sadc: sadc@13650000 {

    compatible = "ingenic,x2600-sadc";

    reg = <0x13650000 0x10000>;

    interrupt-parent = <&core_intc>;

    interrupts = <IRQ_SADC>;

    interrupt-controller;

    dmas = <&pdma1 INGENIC_DMA_AHB_MCU_TYPE(INGENIC_DMA_REQ_SADC_SEQ1_RX)>,

    <&pdma1 INGENIC_DMA_AHB_MCU_TYPE(INGENIC_DMA_REQ_SADC_SEQ2_RX)>;

    dma-names = "seq1rx", "seq2rx";

    status = "disable";

};
```

### 设备树默认配置

默认编译不会产生SADC设备。

### 设备树自定义配置

用户可根据需求在板级设备树中添加sadc，并使能

```c
&sadc{                                                 
        status = "disable";
        pinctrl-names = "default";
        pinctrl-0 = <&aux0_pe>, <&aux1_pe>, <&aux2_pe>, <&aux3_pe>;
        seq1 = <0 1 2 3>;
        /* seq1的连续转换间隔时间 单位 ms*/
        seq1-cont-interval = <10>;
        ingenic,has_dma_support = <0>;
};

根据板级上的adc通道情况填充seq1内容。
```

## 内核编译配置

配置内核并编译sadc驱动： 

```
Symbol: IIO [=y]

Type : tristate

Prompt: Industrial I/O support

 Location:

 -> Device Drivers

 Defined at drivers/iio/Kconfig:5

 Selects: ANON_INODES [=y]

 Selected by: RTC_DRV_HID_SENSOR_TIME [=n] && RTC_CLASS [=y] && USB_HID [=y]


Symbol: INGENIC_ADC [=y]

Type : tristate

Prompt: [X2600 SADC] X2600 sadc iio driver.

 Location:

 -> Ingenic device-drivers Configurations

 -> [iio] drivers

 Defined at module_drivers/drivers/iio/adc/Kconfig:1

 Depends on: SOC_X2600 [=y]
```

### 内核默认编译配置

默认编译不会产生sadc设备。

### 内核自定义编译配置

用户可根据实际需求，添加该驱动的配置。

![2023-05-29 16-07-10屏幕截图](assets/SADC控制器.0.png)

## 模块内核差异

无

## 设备节点生成

驱动注册成功后生成设备节点

```
/dev/iio:device0
```

## 应用程序使用说明

### 测试方法

进入 /sys/bus/iio/devices/iio:device0 目录下： 

```
# cd /sys/bus/iio/devices/iio:device0

# ls
buffer           in_voltage2_raw  power            uevent
dev              in_voltage3_raw  scan_elements
in_voltage0_raw  name             subsystem
in_voltage1_raw  of_node          trigger
```

查看通道1电压值:

```
# cat in_voltage1_raw 
```

## ADC 电池检测

在此功能使用之前需要按照上述配置 sadc 控制器内容

### 功能介绍

​	通过 ADC 通道报告基本的电池数据，并且可以选择通过轮询 GPIO 线来指示电池是否已充满。电压将在电池端子之间测量，并且是强制性的，可选的电流/功率通道用于监测从电池流出的电流/功率，温度通道用于测量电池温度。

### 驱动源码位置

​	***drivers/power/supply/generic-adc-battery.c***

### 设备树配置

#### 设备树配置指导文档位置

​	***Documentation/devicetree/bindings/power/supply/ingenic,adc-battery.yaml***

​	***Documentation/devicetree/bindings/power/supply/adc-battery.yaml***

以 x2600e_halley_v1.0.dts 为例，用户根据实际电路进行修改

```c
	battery:battery {
			compatible = "generic-battery";
			voltage-min-design-microvolt = <3600000>;
			voltage-max-design-microvolt = <4200000>;
 	};

	generic-adc-battery{
		compatible = "adc-battery";
 		status = "okay";
		io-channels = <&sadc 0>, <&sadc 1>, <&sadc 2>;
		io-channel-names = "voltage", "current", "temperature";
		charged-gpios = <&gpe 15 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;

		// power-supplies = <&charger>;
		monitored-battery = <&battery>;
	};

```

### 内核编译配置

```c
Symbol: IIO [=y]
Type : tristate
Prompt: Industrial I/O support
Location:
-> Device Drivers
Defined at drivers/iio/Kconfig:5
Selects: ANON_INODES [=y]
Selected by: RTC_DRV_HID_SENSOR_TIME [=n] && RTC_CLASS [=y] && USB_HID [=y]

Symbol: POWER_SUPPLY [=y]
Type  : bool
Defined at drivers/power/supply/Kconfig:2
Prompt: Power supply class support
Location:
-> Device Drivers
Selected by [n]:
  - AB8500_CORE [=n] && HAS_IOMEM [=y] && ABX500_CORE [=n] && MFD_DB8500_PRCMU [=n]
  - DRM_RADEON [=n] && HAS_IOMEM [=y] && DRM [=n] && PCI [=n] && MMU [=y] && (AGP [=n] || !AGP [=n])
  - DRM_AMDGPU [=n] && HAS_IOMEM [=y] && DRM [=n] && PCI [=n] && MMU [=y]
  - DRM_NOUVEAU [=n] && HAS_IOMEM [=y] && DRM [=n] && PCI [=n] && MMU [=y]
  - HID_BATTERY_STRENGTH [=n] && INPUT [=y] && HID [=y]
  - HID_ASUS [=n] && INPUT [=y] && HID [=y] && USB_HID [=n] && LEDS_CLASS [=n] && (ASUS_WMI [=n] || ASUS_WMI [=n]=n)
  - HID_LOGITECH_HIDPP [=n] && INPUT [=y] && HID [=y] && HID_LOGITECH [=n]
  - HID_SONY [=n] && INPUT [=y] && HID [=y] && USB_HID [=n] && NEW_LEDS [=n] && LEDS_CLASS [=n]
  - HID_STEAM [=n] && INPUT [=y] && HID [=y]
  - HID_WACOM [=n] && INPUT [=y] && HID [=y] && USB_HID [=n]
  - HID_WIIMOTE [=n] && INPUT [=y] && HID [=y] && LEDS_CLASS [=n]
  - USB_CONN_GPIO [=n] && USB_SUPPORT [=y] && GPIOLIB [=y]
  - APPLE_MFI_FASTCHARGE [=n] && USB_SUPPORT [=y] && USB [=y]
  - TYPEC_TCPM [=n] && USB_SUPPORT [=y] && TYPEC [=n] && USB [=y]
  - DELL_LAPTOP [=n] && X86 && X86_PLATFORM_DEVICES [=n] && DMI [=n] && BACKLIGHT_CLASS_DEVICE [=y] && (ACPI_VIDEO || ACPI_VIDEO=n) && (RFKILL [=y] || \
  RFKILL [=y]=n) && SERIO_I8042 [=n] && DELL_SMBIOS [=n]


Symbol: GENERIC_ADC_BATTERY [=y]
Type  : tristate
Defined at drivers/power/supply/Kconfig:47
Prompt: Generic battery support using IIO
Depends on: POWER_SUPPLY [=y] && IIO [=y]
Location:
-> Device Drivers
-> Power supply class support (POWER_SUPPLY [=y])
 
```

### 应用程序使用说明

进入 /sys/class/power_supply/generic-adc-battery/ 目录

```c
#cd  /sys/class/power_supply/generic-adc-battery/
#ls

	current_now  power        subsystem    type         voltage_now
	device       status       temp         uevent       wakeup4
```

查看当前电池电压值

```c
cat voltage_now
```

## ADC 按键

在此功能使用之前需要按照上述配置 sadc 控制器内容

### 功能介绍

ADC（模数转换器）按键功能通常用于将模拟信号转换为数字信号，从而检测按键的状态。

### 驱动源码位置

***drivers/input/keyboard/adc-keys.c***

### 设备树配置

#### 设备树配置指导文档位置

***Documentation/devicetree/bindings/input/adc-keys.txt***

```c
 adc-keys {
            compatible = "adc-keys";
            io-channels = <&sadc 1>;
            io-channel-names = "buttons";
            keyup-threshold-microvolt = <8191>;

            button-play {
                    label = "Play/Pause";
                    linux,code = <KEY_PLAYPAUSE>;
                    press-threshold-microvolt = <4096000>;
            };

            button-volume {
                    label = "Volume UP";
                    linux,code = <KEY_VOLUMEUP>;
                    press-threshold-microvolt = <7660000>;
            };
     		 button-next {
                    label = "Next";
                    linux,code = <KEY_VIDEO_NEXT>;
                    press-threshold-microvolt = <5170000>;
            };

            button-prev {
                    label = "Prev";
                    linux,code = <KEY_VIDEO_PREV>;
                    press-threshold-microvolt = <6143000>;
            };

    };

```

### 内核编译配置

```c
Symbol: IIO [=y]
Type : tristate
Prompt: Industrial I/O support
Location:
-> Device Drivers
Defined at drivers/iio/Kconfig:5
Selects: ANON_INODES [=y]
Selected by: RTC_DRV_HID_SENSOR_TIME [=n] && RTC_CLASS [=y] && USB_HID [=y]

Symbol: KEYBOARD_ADC [=y]
Type  : tristate
Defined at drivers/input/keyboard/Kconfig:16
Prompt: ADC Ladder ButtonsDepends on: !UML && INPUT [=y] && INPUT_KEYBOARD [=y] && IIO [=y]
Location:
-> Device Drivers
-> Input device support
-> Generic input layer (needed for keyboard, mouse, ...) (INPUT [=y])
-> Keyboards (INPUT_KEYBOARD [=y])
```

### 应用程序使用说明

进入 /sys/bus/iio/devices/iio:device0 目录下： 

```c
# cd /sys/bus/iio/devices/iio:device0

# ls
buffer           in_voltage2_raw  power            uevent
dev              in_voltage3_raw  scan_elements
in_voltage0_raw  name             subsystem
in_voltage1_raw  of_node          trigger
```

按下对应的按键并查看通道1电压值:

```c
# cat in_voltage1_raw 
```

