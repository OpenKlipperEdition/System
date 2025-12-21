# PWM测试介绍

## 简介

本测试主要用于配置PWM，使之输出正确波形

## 目录结构

```
.
├── ihal_config.h     # 自定义配置文件
├── Makefile          # 编译文件
├── pwm_test.c        # cpu模式测试文件
├── pwm_test-dma.c    # dma模式测试文件
└── README.md		  # 说明文件

```
## 设备树配置
```
使用前需要根据需求在板级设备树中打开对应的通道如下:

板级设备树位置:kernel/kernel-版本/module_drivers/dts/

一、X26XX
&pwm {
	status = "okay";
	pinctrl-names = "default";
    pinctrl-0 = <&pwm1_pb>, <&pwm2_pb>, <&pwm3_pb>, <&pwm4_pb>;
};
二、X2500
&pwmz {
    status = "okay";
    pinctrl-names = "default";
    pinctrl-0 = <&pwm0_pc>, <&pwm1_pc>, <&pwm2_pc>, <&pwm3_pb>;
};
三、X2000
&pwm {
    status = "okay";
    pinctrl-names = "default";
    pinctrl-0 = <&pwm1_pc>, <&pwm2_pc>, <&pwm11_pc>, <&pwm6_pc>;
};
四、X1600
&pwm {
    status = "okay";
    pinctrl-names = "default";
    pinctrl-0 = <&pwm0_pc>, <&pwm2_pc>, <&pwm6_pb_9>, <&pwm7_pb_10>;
};

注：以上是修改设备树的流程，具体需要配置哪一通道可以参考kernel/kernel-版本/module_drivers/dts/XXXX-pinctrl.dtsi文件和硬件原理图
```
## 测试流程
```
1. 申请通道
2. 设置周期
3. 设置占空比
4. 根据需求设置pwm的初始电平
5. 使能通道
6. 释放通道
注：使用结束后要将通道释放，否则可能会造成内存泄漏
```

## 测试结果

相应的引脚会输出所配置的PWM方波
