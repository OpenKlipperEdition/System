# PWM模块驱动接口

## 模块功能介绍

Pulse width modulation (pwm)模块设计分为16个独立的通道，可以互不影响的进行独立工作，通过使用输入的时钟源来产生方波。

## 驱动源码位置

驱动源码所在位置：

***module_drivers/drivers/pwm/pwm-ingenic-v3.c***

## 设备树配置

设备树所在位置：

kernel内核(version <= 5.10)dts文件路径：
***module_driver/dts/x2600.dtsi***
kernel内核(version > 5.10)dts文件路径：
***module_driver/dts/x2600/x2600.dtsi***

1. 定义相关PWM IO Function

```c
pwm_pins: pwm-pins{

    pwm0_pb: pwm0_pb {

        ingenic,pinmux = <&gpb 12 12>;

        ingenic,pinmux-funcsel = <PINCTL_FUNCTION2>;

    };

....

....

....

    pwm15_pc: pwm15_pc {

        ingenic,pinmux = <&gpc 14 14>;

        ingenic,pinmux-funcsel = <PINCTL_FUNCTION0>;

    };

    pwm_trigger: pwm-trigger{

        ingenic,pinmux = <&gpc 20 20>;

        ingenic,pinmux-funcsel = <PINCTL_FUNCTION1>;

    };

 }; 
```

2. 定义PWM控制器功能

```c
pwm: pwm@0x13610000 {

    compatible = "ingenic,x2600-pwm";

    #pwm-cells = <2>;

    reg = <0x13610000 0x10000>;

    interrupt-parent = <&core_intc>;

    pwm-num = <16>;

    first-channel = <0>;

    interrupts = <IRQ_PWM0>;

    dmas = <&pdma1 INGENIC_DMA_AHB_MCU_TYPE(INGENIC_DMA_REQ_PWM0_TX)>,

    <&pdma1 INGENIC_DMA_AHB_MCU_TYPE(INGENIC_DMA_REQ_PWM1_TX)>,

    <&pdma1 INGENIC_DMA_AHB_MCU_TYPE(INGENIC_DMA_REQ_PWM2_TX)>,

    <&pdma1 INGENIC_DMA_AHB_MCU_TYPE(INGENIC_DMA_REQ_PWM3_TX)>,

    <&pdma1 INGENIC_DMA_AHB_MCU_TYPE(INGENIC_DMA_REQ_PWM4_TX)>,

    <&pdma1 INGENIC_DMA_AHB_MCU_TYPE(INGENIC_DMA_REQ_PWM5_TX)>,

    <&pdma1 INGENIC_DMA_AHB_MCU_TYPE(INGENIC_DMA_REQ_PWM6_TX)>,

    <&pdma1 INGENIC_DMA_AHB_MCU_TYPE(INGENIC_DMA_REQ_PWM7_TX)>,

    <&pdma1 INGENIC_DMA_AHB_MCU_TYPE(INGENIC_DMA_REQ_PWM8_TX)>,

    <&pdma1 INGENIC_DMA_AHB_MCU_TYPE(INGENIC_DMA_REQ_PWM9_TX)>,

    <&pdma1 INGENIC_DMA_AHB_MCU_TYPE(INGENIC_DMA_REQ_PWM10_TX)>,

    <&pdma1 INGENIC_DMA_AHB_MCU_TYPE(INGENIC_DMA_REQ_PWM11_TX)>,

    <&pdma1 INGENIC_DMA_AHB_MCU_TYPE(INGENIC_DMA_REQ_PWM12_TX)>,

    <&pdma1 INGENIC_DMA_AHB_MCU_TYPE(INGENIC_DMA_REQ_PWM13_TX)>,

    <&pdma1 INGENIC_DMA_AHB_MCU_TYPE(INGENIC_DMA_REQ_PWM14_TX)>,

    <&pdma1 INGENIC_DMA_AHB_MCU_TYPE(INGENIC_DMA_REQ_PWM15_TX)>;

    dma-names = "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15";

};
```

### 设备树默认配置

以X2660 halley v1.0为例,内核板级配置文件中,内核默认在开发板适配的LCD设备树中开启pwm设备，例如在X2660_HALLEY_MIPI_LCD_FW050.dtsi中

```c
&pwm {

    pinctrl-names = "default";

    pinctrl-0 = <&pwm10_pe>;

    status = "okay";

};
```

### 设备树自定义配置

如果需要使用其他通道的GPIO 作为PWM输出，可以修改**module_drivers/dts/x2660_halley_lcd/**下属于当前开发板所配置LCD的dtsi文件,以X2660_HALLEY_MIPI_LCD_FW050.dtsi为例,添加pwm12_pc：

```c
&pwm {

    pinctrl-names = "default";

    pinctrl-0 = <&pwm10_pe>,<&pwm12_pc>;

    status = "okay";

};
```

## 内核编译配置

内核驱动编译选项使用PWM_INGENIC_V3控制，配置说明如下：

```
Symbol: PWM_INGENIC_V3 [=y] 

Type : boolean 

Prompt: Ingenic V3 PWM support 

 Location: 

 -> Ingenic device-drivers Configurations 

 -> [PWM] drivers 

 Defined at module_drivers/drivers/pwm/Kconfig:26

 Depends on: SOC_X1600 [=y] || SOC_X2600 [=y] 

 Selects: PWM [=y] 
```

### 内核默认编译配置

内核默认编译PWM驱动

### 内核自定义编译配置

如果产品需要关闭PWM功能，可以去选择PWM编译选项，配置界面如下：

![](assets/PWM模块驱动接口.0.png)

## 设备节点生成

可以通过标准`/sys/class/pwm`查看pwm设备的具体情况，通过标准sysfs接口，进行pwm用户空间的导出和使用。

### 内核pwm标准框架的导出

用户在使用内核pwm标准框架时，可以在“sys/class/pwm/pwmchip0/”路径下操作，首先用户可以cat npwm文件查看支持的pwm通道数,以x2660 halley v1.0为例:

```
# ls

device export npwm power subsystem uevent unexport

# cat npwm 

16
```

X2660支持16个pwm通道。

用户可以向export文件写入需要的通道号，将会生成pwm*文件夹，文件夹下面会有操作对应pwm通道的文件。

```
# echo 12 > export 

# ls

device npwm pwm12 uevent

export power subsystem unexport

# cd pwm12/

# ls

duty_cycle enable period polarity power uevent
```

period：该文件是用来设置周期的，单位是ns。

duty_cycle：该文件是用来设置占空比的（正占空比），单位是ns。

polarity：该文件是用来设置极性的，即以低电平为开启状态，还是以高电平为开启状态，一般为 normal，也就是高电平为开启状态（inversed、normal）。

enable：该文件用来使能或关闭pwm输出。

在设置周期和占空比时，周期必须先于占空比设置，否则会报错。

在不需要使用pwm通道时，用户可以向unexport文件导入pwm通道号关闭pwm相关配置文件。

```
# ls

device npwm pwm12 uevent

export power subsystem unexport

# echo 12 > unexport 

# ls

device export npwm power subsystem uevent unexport 
```

## 应用程序使用说明

### 测试方法

1. 进入`sys/devices/platform/ahb_mcu/13610000.pwm/`目录下：

```
# ls

channels finish_level of_node sg_pwm_num

driver free period_ns subsystem

driver_override init_level power trigger

duty_ns modalias pwm uevent

enable mode request
```

2. 通过写0～15到request可导出对应的pwm进行使用，下面以pwm12为例：

```
# echo 12 > request
```

3. 然后可以看到channels下12通道已被申请，其他通道类似：

```
# cat channels 

ch: 00 unrequested

ch: 01 unrequested

ch: 02 unrequested

ch: 03 unrequested

ch: 04 unrequested

ch: 05 unrequested

ch: 06 unrequested

ch: 07 unrequested

ch: 08 unrequested

ch: 09 unrequested

ch: 10 unrequested

ch: 11 unrequested

ch: 12 requested

ch: 13 unrequested

ch: 14 unrequested

ch: 15 unrequested
```

4. 设置周期为100000，单位为纳秒：

```
# echo 100000 > period_ns
```

5. 设置占空比为50000，单位为纳秒：

```
# echo 50000 > duty_ns 
```

6. 使能pwm12：

```
# echo 1 > enable 
```

如果设置正确，此时会有对应的波形输出。

7. 如果不再使用echo 通道号 > free可以关闭波形输出：

```
# echo 0 > free 
```


