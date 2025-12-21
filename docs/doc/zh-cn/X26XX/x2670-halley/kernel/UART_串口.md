# UART 串口

## 模块功能介绍

通用异步接收器/发送器（UART）串行端口。 共有四个UART：所有UART使用相同的编 程模型。 每个串行端口都可以在基于中断的模式或基于DMA的模式下运行。通用异步 接收器/发送器（UART）与16550行业标准兼容，可以用作符合红外数据协会（IrDA） 串行红外规范1.1的慢速红外异步接口。

## 驱动位置

驱动源码位置：

```
module_drivers/drivers/tty/serial/ingenic_uart.h  

module_drivers/drivers/tty/serial/ingenic_uart.c
```

## 设备树配置

设备树所在位置：

kernel内核(version <= 5.10)dts文件路径：

***module_driver/dts/x2600.dtsi***

kernel内核(version > 5.10)dts文件路径：

***module_driver/dts/x2600/x2600.dtsi***

UART控制器描述：

```c
uart0: serial@0x10030000 {

    compatible = "ingenic,8250-uart";

    reg = <0x10030000 0x100>;

    reg-shift = <2>;

    interrupt-parent = <&core_intc>;

    interrupts = <IRQ_UART0>;

    status = "disabled";

    dmas = <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_UART0_TX)>,

    <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_UART0_RX)>;

    dma-names = "tx", "rx";

    #address-cells = <1>;

    #size-cells = <0>;

};

uart1: serial@0x10031000 {

    compatible = "ingenic,8250-uart";

    reg = <0x10031000 0x100>;

    reg-shift = <2>;

    interrupt-parent = <&core_intc>;

    interrupts = <IRQ_UART1>;

    status = "disabled";

    dmas = <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_UART1_TX)>,

    <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_UART1_RX)>;

    dma-names = "tx", "rx";

};

uart2: serial@0x10032000 {

    compatible = "ingenic,8250-uart";

    reg = <0x10032000 0x100>;

    reg-shift = <2>;

    interrupt-parent = <&core_intc>;

    interrupts = <IRQ_UART2>;

    status = "disabled";

    dmas = <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_UART2_TX)>,

    <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_UART2_RX)>;

    dma-names = "tx", "rx";

};

uart3: serial@0x10033000 {

    compatible = "ingenic,8250-uart";

    reg = <0x10033000 0x100>;

    reg-shift = <2>;

    interrupt-parent = <&core_intc>;

    interrupts = <IRQ_UART3>;

    status = "disabled";

    dmas = <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_UART3_TX)>,

    <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_UART3_RX)>;

    dma-names = "tx", "rx";

};

uart4: serial@0x10034000 {

    compatible = "ingenic,8250-uart";

    reg = <0x10034000 0x100>;

    reg-shift = <2>;

    interrupt-parent = <&core_intc>;

    interrupts = <IRQ_UART4>;

    status = "disabled";

    dmas = <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_UART4_TX)>,

    <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_UART4_RX)>;

    dma-names = "tx", "rx";

};

uart5: serial@0x10035000 {

    compatible = "ingenic,8250-uart";

    reg = <0x10035000 0x100>;

    reg-shift = <2>;

    interrupt-parent = <&core_intc>;

    interrupts = <IRQ_UART5>;

    status = "disabled";

    dmas = <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_UART5_TX)>,

    <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_UART5_RX)>;

    dma-names = "tx", "rx";

};

uart6: serial@0x10036000 {

    compatible = "ingenic,8250-uart";

    reg = <0x10036000 0x100>;

    reg-shift = <2>;

    interrupt-parent = <&core_intc>;

    interrupts = <IRQ_UART6>;

    status = "disabled";

    dmas = <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_UART6_TX)>,

    <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_UART6_RX)>;

    dma-names = "tx", "rx";

};

uart7: serial@0x10037000 {

    compatible = "ingenic,8250-uart";

    reg = <0x10037000 0x100>;

    reg-shift = <2>;

    interrupt-parent = <&core_intc>;

    interrupts = <IRQ_UART7>;

    status = "disabled";

    dmas = <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_UART7_TX)>,

    <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_UART7_RX)>;

    dma-names = "tx", "rx";

};
```

### 设备树默认配置

在板级设备树中，默认编译会产生uart0，uart1设备，描述 如下：

```c
&uart0 {

    status = "okay";

    pinctrl-names = "default";

    pinctrl-0 = <&uart0_pe>;

};

&uart1 {

    status = "okay";

    pinctrl-names = "default";

    pinctrl-0 = <&uart1_pc>;

};
```

### 设备树自定义配置

用户可根据实际需求打开或关闭某个串口设备。

## 内核编译配置

内核配置SERIAL_INGENIC_UART，配置说明如下：

```
CONFIG_SERIAL_INGENIC_UART: 

 If you have a machine based on a xbrust mips soc you can 

 enable its onboard serial port by enabling this option. 

 Symbol: SERIAL_INGENIC_UART [=y] 

 Type : tristate 

 Defined at module_drivers/drivers/tty/serial/Kconfig 

 Prompt: ingenic serial port support 

 Location: 

 -> Ingenic device-drivers Configurations 

 -> [UART] Drivers 

 Selects: SERIAL_CORE [=y] 
```

### 内核默认编译配置

内核默认配置UART驱动，配置界面如下：

![](assets/UART_串口.0.png)

### 内核自定义编译配置

用户可根据实际需求去掉该驱动的配置。

## 模块内核差异

无

## 设备节点生成

驱动加载成功后生成相应的设备节点

```
# ls /dev/ttyS*

ttyS0 ttyS1
```
