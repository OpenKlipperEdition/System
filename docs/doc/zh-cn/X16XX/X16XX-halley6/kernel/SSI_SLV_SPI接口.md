# SSI_SLV SPI接口

## 模块功能介绍

* SSI-SLV是一个全双工同步串行从设备接口，可以连接到多种外部模拟-数字（A/D）转换器、音频和电信编解码器以及其他使用串行传输数据的协议。支持SSI摩托罗拉的串行外设接口（SPI）协议。
* GPIO功能描述

|             |        |                             |
| ----------- | ------ | --------------------------- |
| Name        | I/O    | Description                 |
| SSI_SLV_CLK | Input  | Serialbit-rateclock         |
| SSI_SLV_CE0 | Input  | Firstslaveselectenable      |
| SSI_SLV_DT  | Output | Transmitdata(serialdataout) |
| SSI_SLV_DR  | Input  | Receivedata(serialdatain)   |

* GPIO接口
```
SSI_SLV 0 PA28-31 FUNCTION0
```

## 驱动源码位置

驱动源码所在位置：

***module_drivers/drivers/spi/***

```
├── ingenic_slv.c
├── ingenic_slv.h
```

## 设备树配置

设备树所在位置：

kernel内核(version <= 5.10)dts文件路径：

***module_drivers/dts/x1600.dtsi***

kernel内核(version > 5.10)dts文件路径：

***module_drivers/dts/x1600/x1600.dtsi***

SPI控制器描述：
```c
spi_slv0: slv@0x10045000 {

 compatible = "ingenic,slv";

 reg = <0x10045000 0x1000>;

 interrupt-parent = <&core_intc>;

 interrupts = <IRQ_SSI_SLV>;

 dmas = <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_SLV0_TX)>,

 <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_SLV0_RX)>;

 dma-names = "tx", "rx";

 #address-cells = <1>;

 #size-cells = <0>;

 status = "disabled";

};
```

### 设备树默认配置

设备树默认关闭SPI_SLV控制器设备。 

### 设备树自定义配置

用户可根据实际需求打开SPI控制器设备，例如：
```c
&spi_slv0 {

 status = "disable";

 pinctrl-names = "default";

 pinctrl-0 = <&spi_slv_pa>;

 ingenic,has_dma_support = <1>;

};
```

## 内核编译配置

内核配置INGENIC_SLV，配置说明如下：
```
Symbol: INGENIC_SLV [=n] 

Type : tristate 

Prompt: Ingenic series SPI slave driver 

 Location: 

 -> Ingenic device-drivers Configurations 

(1) -> [SPI] Master/Slave Drivers 

 Defined at module_drivers/drivers/spi/Kconfig:18

 Depends on: MACH_XBURST [=y] 

 Selects: SPI_BITBANG [=n] 
```

### 内核默认编译配置

内核默认关闭SPI_SLV控制器驱动

### 内核自定义编译配置

用户可根据实际需求打开SPI控制器编译，配置界面如下：

![](assets/SSI_SLV_SPI接口.0.png)

## 设备节点生成

驱动加载成功log
```bash
[ 0.298196] IINGENIC SSI slaver Controller ok!!
```
生成设备节点:

***/dev/spi_SlV***

## 应用程序使用说明

### 测试spi读写

内核的 ssi_slv 驱动程序作为从设备驱动，测试时需要主设备提供ce片选与时钟.

#### 源码位置

***packages/example/spi_slv/spislaver_test.c***

#### 测试方法

1. 将SSI_SLV_DT和SSI_SLV_DR引脚短接,SSI_SLV_CLK连接SSI0_CLK，SSI_SLV_CE连接SSI0_CE
```bash
\# ./spi_slvtest --help

./spi_slvtest: invalid option -- 'h'
Usage: ./spi_slvtest [-DsbdlHOLC3]
  -p --path   device to use (default /dev/spidev1.1)
  -b --bpw      bits per word 
  -l --loop     loopback
  -i            Send data (e.g. "1234\xde\xad")
  -d --dma      use dma
```

配置SSI主设备驱动参考18章SSI控制器。

2. 执行./spi_slvtest -l,测试loop back模式
```bash
./spi_slvtest -l

\> > > bits_prt_word : 8

\> > > tranfer mode : cpu

\> > > len: 32

[17880.956803] slv ready

[17880.962866] write data 0

[17880.966667] write data 1

[17880.969299] write data 2

[17880.972785] write data 3

[17880.975429] write data 4

[17880.978061] write data 5

[17880.981283] write data 6

[17880.983945] write data 7

[17880.986597] write data 8

[17880.989228] write data 9

[17880.992425] write data 10

[17880.995154] write data 11

[17880.997877] write data 12

[17881.000598] write data 13

[17881.003904] write data 14

[17881.006653] write data 15

[17881.009396] write data 16

[17881.012742] write data 17

[17881.015495] write data 18

[17881.018216] write data 19

[17881.021477] write data 20

[17881.024227] write data 21

[17881.026968] write data 22

[17881.029687] write data 23

[17881.033020] write data 24

[17881.035770] write data 25

[17881.038513] write data 26

[17881.041651] write data 27

[17881.044400] write data 28

[17881.047121] write data 29

[17881.049840] write data 30

[17881.053176] write data 31
```

3. 打开新终端,执行ssi主设备测试程序提供时钟与片选
```bash
\#adb shell

\#cd

\#./spi_test -D /dev/spidev0.0 -O -H
```

#### 测试结果
```bash
[18309.917179] write number: 32 ; reads number:32

[18309.922251] read_c[0] = 0

[18309.925002] read_c[1] = 1

[18309.927724] read_c[2] = 2

[18309.930444] read_c[3] = 3

[18309.933745] read_c[4] = 4

[18309.936498] read_c[5] = 5

[18309.939220] read_c[6] = 6

[18309.942455] read_c[7] = 7

[18309.945186] read_c[8] = 8

[18309.947886] read_c[9] = 9

[18309.950608] read_c[10] = 10

[18309.954085] read_c[11] = 11

[18309.957016] read_c[12] = 12

[18309.959918] read_c[13] = 13

[18309.963339] read_c[14] = 14

[18309.966249] read_c[15] = 15

[18309.969168] read_c[16] = 16

[18309.972594] read_c[17] = 17

[18309.975524] read_c[18] = 18

[18309.978446] read_c[19] = 19

[18309.981774] read_c[20] = 20

[18309.984703] read_c[21] = 21

[18309.987625] read_c[22] = 22

[18309.990526] read_c[23] = 23

[18309.993961] read_c[24] = 24

[18309.996891] read_c[25] = 25

[18309.999792] read_c[26] = 26

[18310.003169] read_c[27] = 27

[18310.006099] read_c[28] = 28

[18310.009000] read_c[29] = 29

[18310.012579] read_c[30] = 30

[18310.015492] read_c[31] = 31

[18310.018394] @@@@@@ SLV LOOP TEST OK @@@@@@
```
