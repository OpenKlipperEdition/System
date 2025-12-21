# PDMA controller

## Module Function Introduction

The x26xx series chip has two programmable DMA controllers, pdma and dma_mcu.

The Programmable DMA Controller (PDMAC) is specifically designed for intelligent data transfer between on-chip peripherals (MSC, AIC, UART, etc.), external memory and memory-mapped external devices. It supports up to 32 independent DMA channels.

The programmable DMA controller (DMA_MCU) intelligently transfers data between on-chip peripherals (ADC, PWM, etc.), external memory and memory-mapped external devices. It supports up to 32 independent DMA channels.

## Drive source code location

Location of driver source code:

***module_drivers/drivers/dma/ingenic/***

```
├── ingenic_dma.c
├── ingenic_dma.h
├── Kconfig
└── Makefile
```

## Device tree configuration

Location of device tree:

***module_drivers/dts/x2600.dtsi***

PDMA controller description:

```c
pdma: dma@13420000 {

    compatible = "ingenic,x2600-pdma";

    reg = <0x13420000 0x10000>;

    interrupt-parent = <&core_intc>;

    interrupt-names = "pdma", "pdmad";

    interrupts = <IRQ_PDMA>, <IRQ_PDMAD>;

    ingenic,bus-ctrl = <INGENIC_DMA_CTRL_AHB2_BUS>;

    #dma-channels = <8>;

    #dma-cells = <1>;

    ingenic,reserved-chs = <0x3>;

    status ="disable";

};

pdma1: dma@13660000 {

    compatible = "ingenic,x2600-pdma";

    reg = <0x13660000 0x10000>;

    interrupt-parent = <&core_intc>;

    interrupt-names = "pdma", "pdmad";

    interrupts = <IRQ_MCU_PDMA>, <IRQ_MCU_PDMAD>;

    ingenic,bus-ctrl = <INGENIC_DMA_CTRL_AHB_MCU_BUS>;

    #dma-channels = <32>;

    #dma-cells = <1>;

    status ="disable";

    //ingenic,reserved-chs = <0x3>;

};
```

### Default configuration of device tree

The default compilation will produce pdma devices

```c
&pdma {

    status = "okay";

};

&pdma1 {

    status = "okay";

};
```

### Device tree custom configuration

Users can close PDMA devices according to actual needs, and configure this node as disable.

```
&pdma {

    status = "disable";

};

&pdma1 {

    status = "disable";

};
```

## Kernel compilation configuration

The kernel configuration for INGENIC_PDMAC is as follows:

```
Symbol: INGENIC_PDMAC [=y]

 Type : boolean

 Prompt: Ingenic programmable dma controller (Of Driver)

 Location:

 -> Device Drivers

 -> DMA Engine support (DMADEVICES [=y])

 Prompt: Ingenic programmable dma controller (Of Driver)

 Location:

 -> Ingenic device-drivers Configurations

 -> [DMA] Drivers

 Defined at drivers/dma/Kconfig:280

 Depends on: DMADEVICES [=y] && (MACH_XBURST [=n] || MACH_XBURST2 [=y])

 Selects: DMA_VIRTUAL_CHANNELS [=y] && DMA_ENGINE [=y] && DMA_VIRTUAL_CHANNELS [=y] && DMA_ENGINE [=y]

 Selected by: SND_ASOC_PDMA [=y] && SND_ASOC_INGENIC [=y] && SND_ASOC_INGENIC_AS_V1 [=y]
```

### Default compile configuration of kernel

The kernel default configuration is PDMA driver,

The configuration interface is as follows:

![](assets/PDMA控制器.0.png)

### Kernel custom compile configuration

Users can remove the configuration of this driver according to their actual needs.

Drive test configuration, the configuration is as follows:

```
Symbol: DMATEST [=y]

Type : tristate

Prompt: DMA Test client

Location:

-> Device Drivers

-> DMA Engine support (DMADEVICES [=y])

Defined at drivers/dma/Kconfig:561

Depends on: DMADEVICES [=y] && DMA_ENGINE [=y]
```

The configuration interface is as follows:

![](assets/PDMA控制器.1.png)

## Version Differences

The main difference between kernel4.4.94 and kernel5.10 is that kernel5.10 removed some APIs, and the API interface for requesting DMA channels in kernel4.4.94 was different from that of kernel5.10. Users can refer to the driver usage process described below.

## Device Node Generation

### Controller node

After successfully loading the driver, the following controller node is generated:

```
# cd /sys/devices/platform/ahb_mcu/13660000.dma/dma/

# ls

dma0chan0 dma0chan14 dma0chan2 dma0chan25 dma0chan30 dma0chan8

dma0chan1 dma0chan15 dma0chan20 dma0chan26 dma0chan31 dma0chan9

dma0chan10 dma0chan16 dma0chan21 dma0chan27 dma0chan4

dma0chan11 dma0chan17 dma0chan22 dma0chan28 dma0chan5

dma0chan12 dma0chan18 dma0chan23 dma0chan29 dma0chan6

dma0chan13 dma0chan19 dma0chan24 dma0chan3 dma0chan7
```

```
cd /sys/devices/platform/ahb2/13420000.dma/dma

# ls

dma1chan0 dma1chan14 dma1chan2 dma1chan25 dma1chan30 dma1chan8

dma1chan1 dma1chan15 dma1chan20 dma1chan26 dma1chan31 dma1chan9

dma1chan10 dma1chan16 dma1chan21 dma1chan27 dma1chan4

dma1chan11 dma1chan17 dma1chan22 dma1chan28 dma1chan5

dma1chan12 dma1chan18 dma1chan23 dma1chan29 dma1chan6

dma1chan13 dma1chan19 dma1chan24 dma1chan3 dma1chan7
```

### Test program nodes

If you configure a driver test, generate the following test nodes:

```
# cd /sys/module/dmatest/parameters/

# ls

channel     noverify    threads_per_chan    xor_sources

device      pq_sources  timeout

iterations  run         verbose

max_channels test_buf_size  wait
```

## Application Instructions

### dmatest test program

1. into a node of the test program

* dma_mcu test

```
# echo dma0chan31 > /sys/module/dmatest/parameters/channel

# echo 2000 > /sys/module/dmatest/parameters/timeout

# echo 1 > /sys/module/dmatest/parameters/iterations

# echo 1 > /sys/module/dmatest/parameters/run
```

* pdma test

```
# echo dma1chan7 > /sys/module/dmatest/parameters/channel

# echo 2000 > /sys/module/dmatest/parameters/timeout

# echo 1 > /sys/module/dmatest/parameters/iterations

# echo 1 > /sys/module/dmatest/parameters/run
```

* Test results

```
[ 148.309057] dmatest: Started 1 threads using dma0chan31

[ 148.309825] dmatest: dma0chan31-copy: summary 1 tests, 0 failures 1355 iops 10840 KB/s (0)

[ 366.888453] dmatest: Started 1 threads using dma1chan7

[ 366.888848] dmatest: dma1chan7-copy0: summary 1 tests, 0 failures 2777 iops 0 KB/s (0)
```

### SSI driver test

The PDMA and DMA_MCU drivers follow the kernel standard dmaengine driver framework, which can provide a DMA transmission channel as a provider for other drivers. Here we take the SSI driver as an example to introduce how to configure it.

#### Device tree configuration

```
spi0: spi0@0x10043000 {

    compatible = "ingenic,x2600-spi";

    reg = <0x10043000 0x1000>;

    interrupt-parent = <&core_intc>;

    interrupts = <IRQ_SSI0>;

    dmas = <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_SSI0_TX)>,

    <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_SSI0_RX)>;

    dma-names = "tx", "rx";

    #address-cells = <1>;

    #size-cells = <0>;

    status = "disable";

};
```

Among them, dmas and dma-names nodes are related to DMA configuration

```
dmas = <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_SSI0_TX)>,

<&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_SSI0_RX)>;

dma-names = "tx", "rx";
```

**dmas** indicates using the corresponding transmission channel provided by PDMA

**dma-names** is a flag used to request DMA channels in SSI drivers

#### Driver Use Process

Since it uses the kernel standard dmaengine driver framework, all consumer-side drivers use the same API and the usage process is basically the same.

* The process of using SSI driver in kernel4.4.94 is as follows:

1. apply for dma channel

```c
ingspi->txchan = dma_request_slave_channel_reason(dev, "tx");

ingspi->rxchan = dma_request_slave_channel_reason(dev, "rx");
```

2. Parameters for configuring DMA channels

```c
dmaengine_slave_config(txchan, &tx_config);

dmaengine_slave_config(rxchan, &rx_config);
```

3. Get Transmission Descriptor

```c
txdesc = dmaengine_prep_slave_sg(txchan, t->tx_sg.sgl, t->tx_sg.nents, DMA_DEV_TO_MEM, DMA_PREP_INTERRUPT | DMA_CTRL_ACK);

rxdesc = dmaengine_prep_slave_sg(rxchan, t->rx_sg.sgl, t->rx_sg.nents, DMA_DEV_TO_MEM, DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
```

4. Submit and start transfer

```c
dmaengine_submit(txdesc);

dmaengine_submit(rxdesc);

dma_async_issue_pending(rxchan);

dma_async_issue_pending(txchan);
```

* The process of using SSI drivers in kernel 5.10 is as follows:

1. apply for dma channel

```c
ingspi->txchan = dma_request_chan(dev, "tx");

ingspi->rxchan = dma_request_chan(dev, "rx");
```

2. Parameters for configuring DMA channels

```c
dmaengine_slave_config(txchan, &tx_config);

dmaengine_slave_config(rxchan, &rx_config);
```

3. Get Transmission Descriptor

```c
txdesc = dmaengine_prep_slave_sg(txchan, t->tx_sg.sgl, t->tx_sg.nents, DMA_DEV_TO_MEM, DMA_PREP_INTERRUPT | DMA_CTRL_ACK);

rxdesc = dmaengine_prep_slave_sg(rxchan, t->rx_sg.sgl, t->rx_sg.nents, DMA_DEV_TO_MEM, DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
```

4. Submit and start transfer

```c
dmaengine_submit(txdesc);

dmaengine_submit(rxdesc);

dma_async_issue_pending(rxchan);

dma_async_issue_pending(txch
```
