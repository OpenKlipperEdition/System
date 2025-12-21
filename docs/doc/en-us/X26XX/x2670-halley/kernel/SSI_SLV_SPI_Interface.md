# SSI_SLV SPI interface

## Module Function Introduction

* SSI-SLV is a full-duplex synchronous serial slave device interface that can be connected to various external analog-to-digital (A/D) converters, audio and telecommunication decoders, as well as other protocols that use serial data transmission. It supports Motorola's Serial Peripheral Interface (SPI) protocol.

* GPIO Function Description

|  |  |  |
| --- | --- | --- |
| Name | I/O | Description |
| SSI_SLV_CLK | Input | Serialbit-rateclock |
| SSI_SLV_CE0 | Input | Firstslaveselectenable |
| SSI_SLV_DT | Output | Transmitdata(serialdataout) |
| SSI_SLV_DR | Input | Receivedata(serialdatain) |

* GPIO interface

|  |  |  |  |
| --- | --- | --- | --- |
| SSI_SLV | 0 | PB28-31 | FUNCTION2 |

## Drive source code location

Location of driver source code:

***module_drivers/drivers/spi/***

```
├── ingenic_slv.c
├── ingenic_slv.h
```

## Device tree configuration

Location of device tree:

***module_drivers/dts/x2600.dtsi***

***module_drivers/dts/x2660_halley_v1.0.dts***

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

### Default configuration of device tree

The device tree by default disables SPI_SLV controller devices because these pins are also used for LCD functionality.

### Device tree custom configuration

Users can open SPI controller devices according to actual needs, for example:

```c
&spi_slv0 {

    status = "okey";

    pinctrl-names = "default";

    pinctrl-0 = <&spi_slv_pa>;

    ingenic,has_dma_support = <1>;

};
```

## Kernel compilation configuration

The kernel configuration for INGENIC_SLV is as follows:

```
Symbol: INGENIC_SLV [=n]

Type : tristate

Prompt: Ingenic series SPI slave driver

 Location:

 -> Ingenic device-drivers Configurations

 -> [SPI] Master/Slave Drivers

 Defined at module_drivers/drivers/spi/Kconfig:18

 Depends on: MACH_XBURST [=y]

 Selects: SPI_BITBANG [=n]
```

### Default compile configuration of kernel

The kernel defaults to disabling SPI_SLV controller drivers.

### Kernel custom compile configuration

Users can open SPI controller compilation according to actual needs. The configuration interface is as follows:

## Device Node Generation

Drive loaded successfully log:

```
[ 0.298196] IINGENIC SSI slaver Controller ok!!
```

Generate device nodes:

```
/dev/spi_slv
```

## Application Instructions

### Test spi read and write

The kernel's ssi_slv driver as a slave device driver requires that the master device provide ce chip select and clock during testing.

#### Source code location

```
packages/example/spi_slv/spislaver_test.c
```

#### Test methods

1. Hardware connection, according to the schematic diagram, short-circuit the SSI_SLV_DT and SSI_SLV_DR pins. Connect SSI_SLV_CLK to SSI0_CLK and SSI_SLV_CE to SSI0_CE.

2. Configure the SSI master device driver.

Refer to Chapter 18 for SSI controllers.

3. Execute `./slv_test -l` to test loop back mode.

```
 ./slv_test -l

>>> bits_prt_word : 8

>>>tranfer mode : cpu

>>>len: 32

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

\# ./slvtest -help

./slvtest: invalid option -- 'h'

Usage: ./slvtest [-DsbdlHOLC3]

-p --path device to use (default /dev/spidev1.1)

-b --bpw bits per word

-l --loop loopback

-i Send data (e.g. "1234xdexad")

-d --dma use dma
```

4. Open a new terminal and execute ssi master device test program to provide clock and chip select.

```
#adb shell

#./spi_test -D /dev/spidev0.0 -O -H

#### Test Results

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

[18310.018394] @@@@@@ SLV LOOP TEST OK @@@@@
```
