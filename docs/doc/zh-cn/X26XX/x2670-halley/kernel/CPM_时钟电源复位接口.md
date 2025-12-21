# CPM 时钟电源复位接口

## 模块功能介绍

CPM模块用于管理时钟和电源。包括：时钟控制，pll控制，电源控制，复位控制。

CPM驱动主要负责系统和模块的时钟电源管理，包括倍频，分频，时钟开关,　时钟源选择。

## 驱动位置

kernel-4.4.94驱动源码所在位置：

***module_drivers/drivers/clk/ingenic-v2***

```
├── clk-bus.c
├── clk-bus.h
├── clk.c
├── clk-div.c
├── clk-div.h
├── clk.h
├── clk-m300.c
├── clk-pll.h
├── clk-pll-v0.c
├── clk-pll-v0.h
├── clk-pll-v1.c
├── clk-pll-v1.h
├── clk-pll-v2.c
├── clk-pll-v2.h
├── clk-rtc.c
├── clk-rtc.h
├── clk-x1000.c
├── clk-x1600.c
├── clk-x2000.c
├── clk-x2000-v12.c
├── clk-x2500.c
├── clk-x2580.c
├── clk-x2600.c
├── Kconfig
├── Makefile
├── power-gate.c
└── power-gate.h
```

kernel-5.10驱动源码所在位置：

***module_drivers/drivers/clk/ingenic-v2***

```
├── clk-bus.c
├── clk-bus.h
├── clk.c
├── clk-div.c
├── clk-div.h
├── clk.h
├── clk-m300.c
├── clk-pll.c
├── clk-pll.h
├── clk-pll-v1.c
├── clk-pll-v1.h
├── clk-pll-v2.c
├── clk-pll-v2.h
├── clk-x1600.c
├── clk-x2000.c
├── clk-x2500.c
├── clk-x2600.c
├── Kconfig
├── Makefile
├── power-gate.c
└── power-gate.h
```

kernel-6.6驱动源码所在位置：

***module_drivers/drivers/clk/ingenic-v2***

```
├── power-gate.h
├── power-gate.c
├── clk.h
├── clk.c
├── clk-x2600.c
├── clk-x2500.c
├── clk-x2000.c
├── clk-x1600.c
├── clk-pll.h
├── clk-pll.c
├── clk-pll-v2.h
├── clk-pll-v2.c
├── clk-pll-v1.h
├── clk-pll-v1.c
├── clk-m300.c
├── clk-div.h
├── clk-div.c
├── clk-bus.h
├── clk-bus.c
├── clk-ad100.c
├── Makefile
└── Kconfig
```



## 设备树配置

kernel内核(version <= 5.10)dts文件路径：

***module_driver/dts/x2600.dtsi***

kernel内核(version > 5.10)dts文件路径：

***module_driver/dts/x2600/x2600.dtsi***

设备树描述：

```c
clock: clock-controller@0x10000000 {

    compatible = "ingenic,x2600-clocks";

    reg = <0x10000000 0x100>;

    clocks = <&extclk>, <&rtcclk>;

    clock-names = "ext", "rtc_ext";

    #clock-cells = <1>;

    little-endian; 

};

extclk: extclk {

    compatible = "ingenic,fixed-clock";

    clock-output-names ="ext";

    #clock-cells = <0>;

    clock-frequency = <24000000>;

};

rtcclk: rtcclk {

    compatible = "ingenic,fixed-clock";

    clock-output-names ="rtc_ext";

    #clock-cells = <0>;

    clock-frequency = <32768>;

};
```

### 设备树默认配置

设备树默认编译会产生clock，extclk，rtcclk设备，配置extclk频率为24Mhz：

```c
extclk: extclk {

    clock-frequency = <24000000>;

};
```

### 设备树自定义配置

## 内核编译配置

### 内核默认编译配置

使用cpm驱动内核不需要添加特殊配置，只依赖soc的配置选项CONFIG_SOC_X2600，相关代码在：***module_drivers/drivers/clk/ingenic-v2/***

### 内核自定义编译配置

## 设备节点生成

1. 生成的设备节点在:

```c
/sys/devices/platform/10000000.clock-controller

mount -t debugfs none /mnt
```

2. 挂载debugfs到/mnt，/mnt/clk下面的节点可以查看各个时钟的频率和状态

```c
mount -t debugfs none /mnt
```

* 1. cat clk_summary可以查看所以时钟的父子关系，使能状态，频率等:

![](assets/CPM_时钟电源复位接口.0.png)

* 2. /mnt/clk下,每个文件夹代表一个时钟，其中mux_xxx控制时钟源选择，div_xxx控制时钟分频，　gate_xxx控制时钟的开关。pd_mem_xxx控制每个模块memory的电源开关状态。power_xxx控制模块的电源开关状态。

![](assets/CPM_时钟电源复位接口.1.png)

* 3. 进入到某一个时钟的文件夹内，cat clk_rate可以查看时钟频率

![](assets/CPM_时钟电源复位接口.2.png)

## 应用程序使用说明

### 时钟的使用

在SDK的**module_drivers/drivers/clk/ingenic-v2**路径下已经定义好了时钟的信息，在内核启动后，会将时钟信息以树的形式进行关联，用户在使用时可以通过以下方式（以tcu为例）。

在**module_drivers/dts/x2600.dtsi**中tcu节点的描述为：

```
tcu0: tcu0@0x13630000 {

    compatible = "ingenic,tcu";

    reg = <0x13630000 0x10000>;

    interrupt-parent = <&core_intc>;

    interrupt-names = "tcu_int0";

    interrupts = <IRQ_TCU0>;

    interrupt-controller;

    clocks = <&clock CLK_GATE_TCU0>;

    clock-names = "gate_tcu0";

    status = "disable";

};

tcu1: tcu1@0x13640000 {

    compatible = "ingenic,tcu";

    reg = <0x13640000 0x10000>;

    interrupt-parent = <&core_intc>;

    interrupt-names = "tcu_int1";

    interrupts = <IRQ_TCU1>;

    interrupt-controller;

    clocks = <&clock CLK_GATE_TCU1>;

    clock-names = "gate_tcu1";

    status = "disable";

};
```

其中“clocks”和“clock-name”为内核关键字，用于获取时钟信息使用。

tcu的probe函数会获取tcu的时钟信息，用于控制tcu设备时钟的开关。

```c
tcu->clk = of_clk_get(pdev->dev.of_node,0);

if(!tcu->clk){

    dev_err(&pdev->dev,"get clk from dts err\n");

    goto err_mfd_add; 

}
```

使能gate时钟:

```c
clk_prepare_enable(tcu->clk);
```
