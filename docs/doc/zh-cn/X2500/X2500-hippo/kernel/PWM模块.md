# PWM模块

## 模块功能介绍

X2500芯片没有单独的PWM硬件发生器，PWM功能的实现是通过TCU的输出通道来实现的。因此，在使用PWM功能时应当注意PWM的TCU通道是否冲突，对应关系PWMx--TCU_CHNx(x=0..7)。

## 驱动源码位置

驱动源码所在位置：

 module_drivers/drivers/pwm/pwm-ingenic.c

## 设备树配置

设备树所在位置：

module_drivers/dts/x2500-pinctrl.dtsi

module_drivers/dts/x2500.dtsi

1.定义相关PWM IO Function

module_drivers/dts/x2500-pinctrl.dtsi

pwm_pins: pwm-pins{

      pwm0_pc: pwm0_pc {

      ingenic,pinmux = <&gpc 2 2>;

      ingenic,pinmux-funcsel = <PINCTL_FUNCTION2>;

      };

....

      pwm7_pd: pwm7_pd {

        ingenic,pinmux = <&gpd 23 23>;

        ingenic,pinmux-funcsel = <PINCTL_FUNCTION2>;

      };

module_drivers/dts/x2500.dtsi

 pwmz:pwm { 

 compatible = "ingenic,pwm"; 

 #pwm-cells = <2>; 

 pinctrl-names = "default"; 

 pwm0:pwm@0 {

 ingenic,timer-parent = <&channel0>; 

 status = "okay";

 }; 

 pwm1:pwm@1 {

 ingenic,timer-parent = <&channel1>;

 status = "okay";

 };

 pwm2:pwm@2 {

 ingenic,timer-parent = <&channel2>;

 status = "okay";

 };

......

｝；

### 设备树默认配置

内核hippo_v12.dts板级配置文件中默认使用了pwm3_pb作为PWM输出功能

&pwmz {

 status = "okay" ;          

 pinctrl-names = "default";

 pinctrl-0 = <&pwm3_pb>;

};

### 设备树自定义配置

如果需要使用其他通道的GPIO 作为PWM，可以修改hippo_v12.dts文件,比如添加pwm1_pc

&pwm {

 status = "okay" ;          

 pinctrl-names = "default";

 pinctrl-0 = <&pwm3_pb>, <&pwm1_pc>;

};

## 内核编译配置

内核默认配置PWM驱动，配置界面如下：

![](assets/PWM模块.0.png)

## 设备节点生成

可以通过标准/sys/class/pwm查看pwm设备的具体情况，通过标准sysfs接口，进行pwm用户空间的导出和使用。

## 应用程序使用说明

1. 进入/sys/class/pwm/pwmchip0目录下：

\# ls

device export npwm power subsystem uevent unexport

2. 查看npwm可获取当前支持的pwm通道数，目前总共支持8个：

\# cat npwm 

8

3. 通过写0～7到export可导出对应的pwm进行使用，下面以pwm0为例：

\# echo 0 > export 

然后可以看到当前目录下多出了一个pwm0目录，其他通道类似：

\#ls

device npwm pwm0 uevent

export power subsystem unexport

4. 进入到pwm0目录，目前可以设置两项，一个是占空比duty_cycle，另外一个是周期period：

\# ls

duty_cycle enable period polarity power uevent

5. 比如，设置周期为1000000000，单位为纳秒：

\# echo 1000000000 > period 

[58044.902196] period=23809523

[58044.905080] duty=0 level_init=1 low=23809523

6. 设置占空比为500000000，单位为纳秒：

\# echo 500000000 > duty_cycle 

[58210.268663] period=23809523

[58210.271585] duty=11904761 level_init=1 low=11904762

7. 使能pwm0：

\# echo 1 > enable 

如果设置正确，此时会有对应的波形输出。

8. 如果不再使用可以关闭波形输出：

\# echo 0 > enable 

9. 最后也可以将pwm0导出关闭：

\# cd ..

\# ls

device npwm pwm0 uevent

export power subsystem unexport

\# echo 0 > unexport 

\# ls

device export npwm power subsystem uevent unexport

可以看到此时pwm0目录已经没有了。

