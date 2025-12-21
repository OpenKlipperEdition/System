# PWM模块驱动接口

## 模块功能介绍

Pulse width modulation (pwm)模块设计分为8个独立的通道，可以互不影响的进行独立工作，同时又分为两种工作模式，为cpu模式和DMA模式。cpu模式通过使用输入的时钟源来产生方波，而DMA模式的方波产生是通过fifo中的的数据来决定周期以及占空比，另外DMA模式必须按照4word进行对齐。

## 驱动源码位置

驱动源码所在位置：

***module_drivers/drivers/pwm/pwm-ingenic-v3.c***

## 设备树配置

设备树所在位置：

module_drivers/dts/x1600-pinctrl.dtsi

1.定义相关PWM IO Function
```c
pwm_pin: pwm-pin{

                pwm0_pc: pwm0_pc {

                        ingenic,pinmux = <&gpc 0 0>;

                        ingenic,pinmux-funcsel = <PINCTL_FUNCTION0>;

                };

....

....

....

               pwm7_pb_21: pwm7_pb_21 {

                        ingenic,pinmux = <&gpb 21 21>;

                        ingenic,pinmux-funcsel = <PINCTL_FUNCTION2>;

                };
};
```

2.定义PWM控制器功能
```c

pwm: pwm@0x134c0000 { 

compatible = "ingenic,x1600-pwm";

#pwm-cells = <2>;

reg = <0x134c0000 0x10000>;

interrupt-parent = <&core_intc>;

interrupts = <IRQ_PWM>;

dmas = <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_PWM0_TX)>,

 <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_PWM1_TX)>,

 <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_PWM2_TX)>,

 <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_PWM3_TX)>,

 <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_PWM4_TX)>,

 <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_PWM5_TX)>,

 <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_PWM6_TX)>,

 <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_PWM7_TX)>;

dma-names = "0", "1", "2", "3", "4", "5", "6", "7";

};
```

### 设备树默认配置

内核halley6_v10.dts板级配置文件中默认使用了pwm0_pc作为PWM输出功能。

内核默认开启pwm设备。
```c
&pwm {

 pinctrl-names = "default";

 pinctrl-0 = <&pwm0_pc>;

 status = "disable";

};
```

### 设备树自定义配置

如果需要使用其他通道的GPIO 作为PWM输出，可以修改halley6_v10.dts文件,比如添加pwm2_pc
```c
&pwm {

 pinctrl-names = "default";

 pinctrl-0 = <&pwm0_pc>,<&pwm2_pc>;

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

(1) -> [PWM] drivers 

 Defined at module_drivers/drivers/pwm/Kconfig:26

 Depends on: SOC_X1600 [=y] 

 Selects: PWM [=y] 
```

### 内核默认编译配置

内核默认编译PWM驱动

### 内核自定义编译配置

如果产品需要关闭PWM功能，可以去选择PWM编译选项，配置界面如下：

![](assets/PWM模块驱动接口.0.png)

## 设备节点生成

可以通过标准/sys/class/pwm查看pwm设备的具体情况，通过标准sysfs接口，进行pwm用户空间的导出和使用。

### 内核pwm标准框架的导出

用户在使用内核pwm标准框架时，可以在“sys/class/pwm/pwmchip0/”路径下操作，首先用户可以cat npwm文件查看支持的pwm通道数。
```bash
\# ls

device export npwm power subsystem uevent unexport


\# cat npwm

8
```
x1600支持8个pwm通道。

用户可以向export文件写入需要的通道号，将会生成pwm*文件夹，文件夹下面会有操作对应pwm通道的文件。
```bash
\# echo 0 > export

\# ls

device npwm pwm0 uevent

export power subsystem unexport

\# cd pwm0/

\# ls

duty_cycle enable period polarity power uevent
```

period：该文件是用来设置周期的，单位是ns。

duty_cycle：该文件是用来设置占空比的（正占空比），单位是ns。

polarity：该文件是用来设置极性的，即以低电平为开启状态，还是以高电平为开启状态，一般为 normal，也就是高电平为开启状态（inversed、normal）。

enable：该文件用来使能或关闭pwm输出。

在设置周期和占空比时，周期必须先于占空比设置，否则会报错。

在不需要使用pwm通道时，用户可以向unexport文件导入pwm通道号关闭pwm相关配置文件。
```bash
\# ls

device npwm pwm0 uevent

export power subsystem unexport

\# echo 0 > unexport

\# ls

device export npwm power subsystem uevent unexport
```

## pwm放音

### 内核编译配置

使用pwm放音功能需要在内核中选配。
```
Symbol: INGENIC_PWM_AUDIO [=y] 

Type : boolean 

Prompt: [PWM-AUDIO] Ingenic Pwm Audio Driver 

Location: 

 -> Ingenic device-drivers Configurations Defined at module_drivers/drivers/char/Kconfig:8 Depends on: PWM_INGENIC_V3 [=y] 

Selects: PWM [=y] 
```

### 设备树配置

设备树需要打开。
```c
pwm_audio {

 status = "okay";

 compatible = "ingenic,x1600-pwm-audio";

 pwm-audio-mode = <2>;

 pwm-audio-channel = <0>;

 };
```

用户不需要的话可以设置为“disable”关闭。

### 应用程序编译

pwm放音功能使用字符设备驱动编写，用户可以在以下路径找到应用程序：

***packages/example/pwm_audio/***

在使用应用程序进行pwm放音测试时，需要准备好音频文件，然后用一下方法进行测试：
```bash
# ./pwm_audio_test FM876-summer.wav
```
## 应用程序使用说明

### 测试方法

1. 进入sys/devices/platform/ahb2/134c0000.pwm/目录下：
```bash
\# ls

channels finish_level of_node sg_pwm_num

driver free period_ns subsystem

driver_override init_level power trigger

duty_ns modalias pwm uevent

enable mode request
```

2. 通过写0～7到request可导出对应的pwm进行使用，下面以pwm0为例：
```bash
\# echo 0 > request
```

然后可以看到channels下0通道已被申请，其他通道类似：
```bash
\# cat channels

ch: 00 requested

ch: 01 unrequested

ch: 02 unrequested

ch: 03 unrequested

ch: 04 unrequested

ch: 05 unrequested

ch: 06 unrequested

ch: 07 unrequested
```

3. 目前可以设置占空比duty_ns，周期period_ns, 模式mode，可以使用cat mode命令查看模式：
```bash
\# cat mode

0 --> CPU mode, Continuous waveform.

1 --> DMA mode,Specified number of waveforms.

2 --> DMA mode,Continuous waveform.

current mode: 0
```

4. 若选择模式1，需要设置周期数量，其他模式不需要设置周期数量：
```bash
\# echo 1> mode

\# echo 1000000 > sg_pwm_num
```

5. 设置周期为1000000000，单位为纳秒：
```bash
\# echo 100000 > period_ns
```

6. 设置占空比为500000000，单位为纳秒：
```bash
\# echo 50000 > duty_ns
```

7. 使能pwm0：
```bash
\# echo 1 > enable
```

如果设置正确，此时会有对应的波形输出。

8. 如果不再使用echo 通道号 > free可以关闭波形输出：
```bash
\# echo 0 > free
```

