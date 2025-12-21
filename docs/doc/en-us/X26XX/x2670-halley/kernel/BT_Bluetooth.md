# BT Bluetooth

## Module Introduction

### AP6212 chip

The X26xx Halley platform uses the AP6212 chip. The AP6212 is an integrated Wi-Fi and Bluetooth function module based on the BCM4340A1 solution.

The Bluetooth module supports HCI UART interface and audio data PCM interface. The Bluetooth conforms to the Bluetooth standard specification 5.0.

### GPIO function description

bluetooth module

| **Name** | **I/O** | **Description** |
| **BT_REG_ON** | I | Powerup/down internal regulators used by BT section |
| **BT_WAKE** | I | HOST wake-up Bluetooth device |
| **BT_HOST_WAKE** | O | Bluetooth device to wake-upHOST |
| **UART_RTS_N** | O | Bluetooth UART interface |
| **UART_TXD** | O | Bluetooth UART interface |
| **UART_RXD** | I | Bluetooth UART interface |
| **UART_CTS_N** | I | Bluetooth UART interface |

## Drive source code location

Location of driver source code:

***kernel/module_drivers/drivers/misc/bt_power_bluesleep.c***

## Device tree configuration

### Default configuration of device tree

Taking x2670_halley v1.0 development board as an example, in the board-level device tree x2670_halley_v1.0.dts, default configurations are made as follows:

1. Configure UART for Bluetooth

```c
&uart1 {

	status = "okay";

	pinctrl-names = "default";

	pinctrl-0 = <&uart1_pc>;

};
```

2. Configure power management related gpio

```c
/*
 * gpd 14 bt wakeup host
 * gpd 13 host wakeup bt
 */

bt_power {

	compatible = "ingenic,bt_power";

	ingenic,reg-on-gpio = <&gpc 1 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;

	ingenic,wake-gpio = <&gpd 14 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;

	status = "okay";

};
```

### Device tree custom configuration

Users can enable related configurations according to actual needs.

## Kernel compilation configuration

The kernel configuration for BCM_4345C5_RFKILL is as follows:

```
Symbol: BCMDHD [=y]

Type : tristate

Prompt: Broadcom FullMAC wireless cards support

 Location:

 -> Ingenic device-drivers Configurations

 -> SDIO-WIFI drivers

 Defined at module_drivers/drivers/net/wireless/bcmdhd/Kconfig
 ```

 ```
 Symbol: BCM_4345C5_RFKILL [=y]

 Type  : tristate

 Prompt: Bluetooth power control driver for BCM-4345C5 module

  Location:
    -> Ingenic device-drivers Configurations

  Defined at module_drivers/drivers/misc/Kconfig:50

  Depends on: RFKILL [=y] && (MMC_SDHCI_INGENIC [=y] || INGENIC_MMC_MMC0 [=n] || INGENIC_MMC_MMC1 [=n])
 ```

### Default compile configuration of kernel

The kernel default configuration opens this module

### Kernel custom compile configuration

Users can turn off this module according to their actual needs.

## Kernel version differences

## Device Node Generation

After successful loading of the driver, the following nodes are generated:

1. UART Node

```
# ls dev/ttyS*

/dev/ttyS0 /dev/ttyS1 /dev/ttyS3
```

2. Power management related nodes

```
# cat /sys/class/rfkill/rfkill0/name

bluetooth
```

## Application Instructions

### Module power supply

Attention: The first time you turn on the system must `echo 1 sys/class/rfkill/rfkill0/state`, otherwise it may not be turned on successfully.

```
# echo 1 > sys/class/rfkill/rfkill0/state

[ 1402.277128] restore_pin_status is not defined
```

### Get Bluetooth module ID through serial port

* Test program: bcm_test.
* Ensures that bluetooth power supply is normal.
* Ensures UART interface communicates normally with bluetooth module.

```
# ./pretest/bcm_test -d /dev/ttyS0

=======Read Successfully! Chip Version : BCM4340A1
```

### Bluetooth UART test

Set up `bsa_server`

Test program: `bsa_server`

Steps:

1. Enter `adb shell`

```
ser@user-HP-Compaq-8200:~$ adb shell
```

2. Execute `bsa_server` to start service

```
# bsa_server -r 15 -p /lib/firmware/bluetooth/BCM43430A1.hcd -u /var/run/ -d /dev/ttyS1
```

* -r: specifies baudrate as 3M (UART supports a maximum of 3M)
* -p: specifies Bluetooth firmware path
* -u: specifies location to generate bt node
* -d: device

Note: bsa_server cannot be executed directly through serial port because it is too slow and will cause bsa_server to fail to execute normally; testing needs to be done by executing bsa_server through adb shell.

3. After the `bsa_server` service is successful, it will create a daemon process and generate two bt nodes.

```
# ls /var/run/bt*

/var/run/bt-avk-fifo		/var/run/bt-daemon-socket
```

### Based on bsa server development reference content

1. `bsa_server` development guide location

***external/bluetooth_demo/BSA_GATT_Guide-v03.pdf***

***external/bluetooth_demo/BSA_Simple_Guideline-v01.pdf***

2. demo source code directory

***external/bluetooth_demo/3rdparty***

3. Quickly compile app (refer to bsa_server development guide in detail)

* 1. Configure cross-compile tools.

```
# export PATH=prebuilts/toolchains/mips-gcc720-glibc226/bin:$PATH
```

* 2. Enter the build of the APP that needs to be compiled, for example app_manager

```
# cd packages/example/App/bluetooth_demo

# cd 3rdparty/embedded/bsa_examples/linux/app_manager/build
```

* 3. Configure Environment

```
# export MIPSGCC=mips-linux-gnu-gcc
```

* 4. Compile

```
# make CPU=mips clean

# make CPU=mips
```

* 5. Generate executable programs in the build directory

```
# ls mips/

app_manager obj/
```

## FAQ

### bsa_server does not support -lpm parameter

Low power mode is not supported.

### Common normal printing errors will not affect normal use

The 3D command is not supported.

![](assets/BT蓝牙.0.jpeg)

Check that low-power nodes do not exist
