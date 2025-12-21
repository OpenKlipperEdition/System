# PWM模块驱动接口

## 模块功能介绍

Pulse width modulation (pwm)模块设计分为16个独立的通道，可以互不影响的进行独立工作，同时又分为两种工作模式，为cpu模式和DMA模式。cpu模式通过使用输入的时钟源来产生方波，而DMA模式的方波产生是通过fifo中的的数据来决定周期以及占空比，另外DMA模式必须按照4word进行对齐。

## 驱动源码位置

驱动源码所在位置：

***module_drivers/drivers/pwm/pwm-ingenic-v2.c***

## 设备树配置

设备树所在位置：

内核dts文件路径：

kernel内核(version <= 5.10)dts文件路径：

***module_driver/dts/x2000.dtsi***

kernel内核(version > 5.10)dts文件路径：

***module_driver/dts/x2000/x2000.dtsi***

1.定义相关PWM IO Function：
```c
pwm_pin: pwm-pin{

                pwm0_pc: pwm0_pc {

                        ingenic,pinmux = <&gpc 0 0>;

                        ingenic,pinmux-funcsel = <PINCTL_FUNCTION0>;

                };

....

....

....

               pwm15_pc: pwm15_pc {

                        ingenic,pinmux = <&gpc 15 15>;

                        ingenic,pinmux-funcsel = <PINCTL_FUNCTION0>;

                };
}
```

2.PWM控制器描述：
```c
pwm: pwm@0x134c0000 {

        compatible = "ingenic,x2000-pwm";

 #pwm-cells = <2>;

        reg = <0x134c0000 0x10000>;

        interrupt-parent = <&core_intc>;

        interrupts = <IRQ_PWM>;
		clock-frequency = <300000000>; /*设备时钟，配置时必须大于设备总线频率*/

};
```

### 设备树默认配置

内核halley5_v30.dts板级配置文件中默认使用了pwm1_pc作为PWM输出功能
```c
&pwm {

 pinctrl-names = "default";

 pinctrl-0 = <&pwm1_pc>;

 status = "okay";

};
```

### 设备树自定义配置

如果需要使用其他通道的GPIO 作为PWM，可以修改halley5_v30.dts文件,比如添加pwm0_pd
```c
&pwm {

 pinctrl-names = "default";

 pinctrl-0 = <&pwm1_pc>,<&pwm0_pd>;

 status = "okay";

};
```

## 内核编译配置

内核驱动编译选项使用PWM_INGENIC_V2控制，配置说明如下：
```
 Symbol: PWM_INGENIC_V2 [=y] 

 Type : tristate 

 Prompt: Ingenic PWM V2 support 

 Location: 

 -> Ingenic device-drivers Configurations 

 -> [PWM] drivers 

 Defined at module_drivers/drivers/pwm/Kconfig:13 

 Depends on:SOC_X2000 [=y] || SOC_M300 [=n] || SOC_X2100 [=n] 

 Selects: PWM [=y] 
```

### 内核默认编译配置

内核默认配置PWM驱动，配置界面如下：

![](assets/PWM模块驱动接口.0.png)

### 核自定义编译配置

如果产品不需要PWM功能，可以去掉该选项以减小内核大小。

## 设备节点生成

可以通过标准`/sys/class/pwm`查看pwm设备的具体情况，通过标准sysfs接口，进行pwm用户空间的导出和使用。

### 内核标准pwm框架的导出

1. 进入`/sys/class/pwm/pwmchip0`目录下：
```bash
# ls

device export npwm power subsystem uevent unexport
```

2. 查看npwm可获取当前支持的pwm通道数，目前总共支持16个：
```bash
# cat npwm

16
```

3. 通过写0～15到export可导出对应的pwm进行使用，下面以pwm0为例：
```bash
# echo 0 > export 
```

然后可以看到当前目录下多出了一个pwm0目录，其他通道类似：
```bash
# ls

device npwm pwm0 uevent

export power subsystem unexport
```

4. 进入到pwm0目录，目前可以设置两项，一个是占空比duty_cycle，另外一个是周期period：
```bash
# ls

duty_cycle enable period polarity power uevent
```

5. 比如，设置周期为1000000000，单位为纳秒：
```baah
# echo 1000000000 > period 

[58044.902196] period=23809523

[58044.905080] duty=0 level_init=1 low=23809523
```

6. 设置占空比为500000000，单位为纳秒：
```bash
# echo 500000000 > duty_cycle 

[58210.268663] period=23809523

[58210.271585] duty=11904761 level_init=1 low=11904762
```

7. 使能pwm0：
```bash
# echo 1 > enable 
```

如果设置正确，此时会有对应的波形输出。

8. 如果不再使用可以关闭波形输出：
```bash
# echo 0 > enable 
```

9. 最后也可以将pwm0导出关闭：
```bash
# cd ..

# ls

device npwm pwm0 uevent

export power subsystem unexport

# echo 0 > unexport 

# ls

device export npwm power subsystem uevent unexport
```

可以看到此时pwm0目录已经没有了。

## 应用程序使用说明

### 测试方法

1. 进入`sys/devices/platform/ahb2/134c0000.pwm/`目录下
```bash
# ls

channels free subsystem driver_override

power modalias pwm uevent

enable request config
```

2. 通过写0～15到request可导出对应的pwm进行使用，下面以pwm0为例：
```bash
# echo 0 > request
```

然后可以看到channels下0通道已被申请，其他通道类似：
```bash
# cat channels 

ch: 00 requested

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

ch: 12 unrequested

ch: 13 unrequested

ch: 14 unrequested

ch: 15 unrequested
```

3. 设置周期、占空比和选择pwm工作模式：
```bash
# echo 0 50000 100000 > config
```

其中传递的参数含义如“工作模式 占空比 周期”，其中第一个参数表示工作模式0为cpu模式，1为dma模式。

4. 使能pwm0：
```bash
# echo 1 > enable 
```

如果设置正确，此时会有对应的波形输出。

5. 如果不再使用echo 通道号 > free可以关闭波形输出：
```bash
# echo 0 > free 
```