# TCU timer unit

## Module Function Introduction

The X26xx series chip has two TCU controllers, tcu0 and tcu1.

Each timer counter unit (TCU) chip design contains 8 TCU channels, which are numbered from 0 to 7. Each channel can have three clock input sources pwm(gpio0, gpio1), extal, and any combination of them can be used for functional use.

According to the specifications, there are seven functional modes that can be divided into:

Conventional mode: The counter counts on the rising edge or falling edge of the clock, and can also count at the same time.

Door mode: The counter counts when the door is 0.

Orthogonal mode: The counter counts due to orthogonal input.

Direction mode: The counter increases or decreases depending on the input signal.

POS mode: The counter starts counting from 0 due to rising or falling edges.

Capture mode: The counter counts cycles and outputs high-level time and cycle time.

Filter mode: This mode is used to remove unwanted GPIO inputs (e.g. noise).

GPIO storage mode: This mode stores a counter value when detecting level changes of GPIO.

|  |  |  |
| --- | --- | --- |
| TCU channel | GPIO0 |GPIO1 |
| ch0 | input0 | input0 |
| ch1 | input1 | input1 |
| ch2 | input2 | input2 |
| ch3 | input3 | input3 |
| ch4 | input4 | input4 |
| ch5 | input5 | input5 |
| ch6 | input6 | input6 |
| ch7 | input7 | input7 |

## driver source code

Location of driver source code

***module_drivers/drivers/mfd/ingenic-tcu.c***

## Device tree configuration

Location of device tree:

***module_drivers/dts/x2600.dtsi***

TCU Controller Statement:

```c
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

### Default configuration of device tree

The default compilation of device tree will not produce tcu devices.

### Device tree custom configuration

Users can enable TCU devices according to actual needs and configure this controller as OK.

```
&tcu0 {

    status = "okay";

};

&tcu1 {

    status = "okay";

};
```

## Kernel compilation configuration

The kernel configuration for MFD_INGENIC_TCU is as follows:

```
Symbol: MFD_INGENIC_TCU [=y]

Type : boolean

Prompt: [TCU] Ingenic tcu driver

 Location:

(1) -> Ingenic device-drivers Configurations

 Defined at module_drivers/drivers/mfd/Kconfig:43

 Depends on: SOC_X2000 [=n] || SOC_M300 [=n] || SOC_X2100 [=n] || SOC_X1600 [=y] || SOC_X2600 [=y] || SOC_X2580 [=n]

 Selects: MFD_CORE [=y] && GENERIC_IRQ_CHIP [=y]
```

### Default configuration of device tree

The kernel default configuration is TCU driver, and the configuration interface is as follows:

![](assets/TCU定时器单元.0.png)

### Kernel custom compile configuration

Users can remove the configuration of this driver according to actual needs.

## Device Node Generation

After successful loading of the driver, the following nodes are generated:

```
/sys/devices/platform/ahb_mcu/13630000.tcu0

# ls

disable enable power

driver modalias subsystem

driver_override of_node uevent


/sys/devices/platform/ahb_mcu/13640000.tcu1/

# ls

disable enable power

driver modalias subsystem

driver_override of_node uevent
```

## Application Instructions

1. The main functions used are disable and enable.

2. There is a configuration function in the driver, which defaults to **module_drivers/drivers/mfd/ingenic-tcu.c**

```c
/*This is a simple configuration test demo*/

static void ingenic_tcu_config_attr(int id,enum tcu_mode_sel mode_sel)

{

    int i = 0;

    g_tcu_chn[id].mode_sel = mode_sel;

    g_tcu_chn[id].irq_type = FULL_IRQ_MODE;

    ……

    for(i = 0; i < 20 ;i++)

        sws_pr_debug(&g_tcu_chn[id]);

}
```

Description: There is no specific application scenario yet. The system interface has not been left too much. This function can be quickly verified by combining with spec. More system configuration interfaces will be left later.

Before using DIRECTION_MODE, POS_MODE, CAPTURE_MODE and FILTER_MODE modes, please configure gpio0 or gpio1 of corresponding tcu channel.

When using QUADRATURE_MODE mode, please configure gpio0 and gpio1 of corresponding TCU channel as orthogonal signals.

3. The process of use is to write the corresponding channel into enable, and then start working

```
#

mode_sel:

0:GENERAL_MODE 1:GATE_MODE 2:DIRECTION_MODE 3:QUADRATURE_MODE

4:POS_MODE 5:CAPTURE_MODE 6:FILTER_MODE

####################example ####################

## echo channel_id mode_sel > enable

## echo channel_id > disable

channel: 00 disable

channel: 01 disable

channel: 02 disable

channel: 03 disable

channel: 04 disable

channel: 05 disable

channel: 06 disable

channel: 07 disable

# echo 0 0 > enable

[164.485676] mode_id = 0

...

...

[165.896941] ----------------------------------------count N0.20 ----------------------------------------------------

[165.896941]

[165.912304] -stop-----addr-b000201c-value-00000000 -----------

[165.918236] -mask-----addr-b0002030-value-00ff80fe -----------

[165.924181] -enable---addr-b0002010-value-00000001 -----------

[165.930120] -flag-----addr-b0002020-value-00010000 -----------

[165.936048] -Control--addr-b000204c-value-00010004 -----------

[165.941990] -full-----addr-b0002040-value-0000ffff -----------

[165.947925] -half- -addr-b0002044-value-00005000 -----------

[165.953874] -TCNT-----addr-b0002048-value-0000b5bf -----------

[165.959811] -CAP reg_base-------value-00000000 -----------

[165.965399] -CAP_VAL register---value-00000000 -----------
```

4. Write the corresponding channel into the disable and stop counting.

```
# echo 0 > disable

# cat disable

channel: 00 disable

channel: 01 disable

channel: 02 disable

channel: 03 disable

channel: 04 disable

channel: 05 disable

channel: 06 disable

channel: 07 disable
```

Explanation: TCU uses more functional modes, so there are not many interfaces left. However, all internal logic has been written and only needs to assign values to the ingenic_tcu_config_attr function according to different usage methods in the spec. Each macro is also defined well, which can be conveniently configured for use. Different usage scenarios please modify the corresponding configuration.
