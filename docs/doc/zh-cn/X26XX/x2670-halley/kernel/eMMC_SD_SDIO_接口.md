# eMMC/SD/SDIO 接口

## 模块功能介绍

DWC_MSHC是一种可高度配置和可编程的高性能移动存储主控制器。DWC_MSHC其数据传输的总接口为AXI。EMMC 是英文 Embedded Multi-Media Card（嵌入式多媒体卡）的缩写。

X26xx Halley平台默认使用的ap6212芯片。 AP6212是基于BCM4340A1方案的集成wifi和bluetooth的功能模块，它包括用于WiFi的SDIO接口，和用于蓝牙的UART/PCM接口。

目前X26xx Halley平台仅X2600E Halley V1.0开发板支持EMMC设备。

## 板级差异

### SDIO

####  x2660 & x2670

|  |  |  |
| --- | --- | --- |
| **Name** | **I/O** | **Funtion** |
| **SDIO(MSC0)** | GPIO-(PD00~PD05) | FUNTION0 |
| **WL_REG_ON** | GPIO-PC00 |  |
| **WL_WAKE_HOST** | GPIO-PD12 |  |

####  x2600e

|  |  |  |
| --- | --- | --- |
| **Name** | **I/O** | **Funtion** |
| **SDIO(MSC1)** | GPIO-(PC25~PC30) | FUNTION0 |
| **WL_REG_ON** | GPIO-PD13 |  |
| **WL_WAKE_HOST** | GPIO-PD12 |  |

###  SD/EMMC

|  |  |  |
| --- | --- | --- |
| **Name** | **I/O** | **Funtion** |
| **MSC0_CLK** | GPIO-PD00 | FUNTION0 |
| **MSC0_CMD** | GPIO-PD01 | FUNTION0 |
| **MSC0_DATA** | GPIO-(PD02~PD05)(PE01~PE04) | FUNTION0 |

### MSC控制器命名对应关系：

|  |  |  |  |  |  |  |  |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **控制器** | **Base** | **bootrom** | **kernel** | **pm-spec** | **x2660 hardware-pcb** | **x2670 hardware-pcb** | **x2600e hardware-pcb** |
| **控制器0** | 13450000 | msc0 | msc0 | msc0 | sdio | sdio | emmc |
| **控制器1** | 13460000 | msc1 | msc1 | msc1 | none | none |sdio |

## 驱动源码位置

驱动源码位于：

**module_drivers/drivers/mmc/host/**

```
├──sdhci.c
├──sdhci-ingenic.c
├──ingenic_sdio.c
```

## 设备树配置

设备树所在位置：

kernel内核(version <= 5.10)dts文件路径：

***module_driver/dts/x2600.dtsi***

kernel内核(version > 5.10)dts文件路径：

***module_driver/dts/x2600/x2600.dtsi***

msc0控制器描述：

```c
msc0: msc@0x13450000 { 

    compatible = "ingenic,sdhci";

    reg = <0x13450000 0x10000>;

    interrupt-parent = <&core_intc>;

    interrupts = <IRQ_MSC0>;

    pinctrl-names ="default";
    
    pinctrl-0 = <&msc0_8bit_low>;

    pinctrl-1 = <&msc0_8bit_high>;
};
```

msc1控制器描述：

```c
msc1: msc@0x13460000 {

    compatible = "ingenic,sdhci";

    reg = <0x13460000 0x10000>;

    status = "disabled";

    interrupt-parent = <&core_intc>;

    interrupts = <IRQ_MSC1>;

    pinctrl-names ="default";

    pinctrl-0 = <&msc1_4bit>;

};
```

### 设备树默认配置

在板级设备树x2660_halley_v1.0.dts、x2660_halley_v1.1.dts和x2670_halley_v1.0.dts中，默认配置产生msc0设备，x2600e_halley_v1.0.dts中，默认配置产生msc0和msc1设备，描述如下：

**x2660_halley_v1.0.dts**

```c
&msc0 {

    status = "okay"; 

    pinctrl-names ="default","enable", "disable";

    pinctrl-0 = <&msc0_4bit>;

    pinctrl-1 = <&rtc32k_enable>;

    pinctrl-2 = <&rtc32k_disable>;

    cap-mmc-highspeed;

    max-frequency = <50000000>;

    bus-width = <4>;

    non-removable;

    voltage-ranges = <1800 3300>;

    ingenic,sdio_clk = <1>;

    keep-power-in-suspend;

    /* special property */

    ingenic,wp-gpios = <0>;

    ingneic,cd-gpios = <0>;

    ingenic,rst-gpios = <0>;

    ingenic,removal-manual;

    bcmdhd_wlan: bcmdhd_wlan {

        compatible = "android,bcmdhd_wlan";

        ingenic,sdio-irq = <&gpd 12 IRQ_TYPE_LEVEL_HIGH INGENIC_GPIO_NOBIAS>;

        ingenic,sdio-reset = <&gpc 0 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;

    };

};
```

**x2660_halley_v1.1.dts** 和 **x2670_halley_v1.0.dts** 配置相同

```c
&msc0 {

    status = "okay";

    /*mmc-hs200-1_8v;*/

    pinctrl-names ="default";

    pinctrl-0 = <&msc0_4bit>;

    cap-mmc-highspeed;

    max-frequency = <50000000>;

    bus-width = <4>;

    non-removable;

    voltage-ranges = <1800 3300>;

    ingenic,sdio_clk = <1>;

    keep-power-in-suspend;

    /* special property */

    ingenic,wp-gpios = <0>;

    ingneic,cd-gpios = <0>;

    ingenic,rst-gpios = <0>;

    ingenic,removal-manual;

    bcmdhd_wlan: bcmdhd_wlan {

        compatible = "android,bcmdhd_wlan";

        wl_vccio-supply = <&wl_vccio>;

        ingenic,sdio-irq = <&gpd 12 IRQ_TYPE_LEVEL_HIGH INGENIC_GPIO_NOBIAS>;

        ingenic,sdio-reset = <&gpc 0 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;

    };

};
```

**x2670_halley_v1.0.dts**

```c
&msc0 {
        status = "okay";
        pinctrl-names ="default";                                                                                                                                      
        pinctrl-0 = <&msc0_8bit_low>,<&msc0_8bit_high>;
        mmc-hs200-1_8v;
        non-removable;
        max-frequency = <200000000>;
        bus-width = <8>;
        voltage-ranges = <1800 3300>;

        ingenic,rst-gpios = <0>;
        ingenic,poc-v1.8;

};

&msc1 {
        status = "okay";
        /*mmc-hs200-1_8v;*/
        pinctrl-names ="default";
        pinctrl-0 = <&msc1_4bit1>;

        cap-mmc-highspeed;
        max-frequency = <50000000>;
        bus-width = <4>;
        non-removable;
        voltage-ranges = <1800 3300>;

        ingenic,sdio_clk = <1>;
        keep-power-in-suspend;

        /* special property */
        ingenic,wp-gpios = <0>;
        ingneic,cd-gpios = <0>;
        ingenic,rst-gpios = <0>;
        ingenic,removal-manual; /*removal-dontcare, removal-nonremovable, removal-removable, removal-manual*/

        bcmdhd_wlan: bcmdhd_wlan {
                 compatible = "android,bcmdhd_wlan";
                 wl_vccio-supply = <&wl_vccio>;
                 ingenic,sdio-irq = <&gpd 12 IRQ_TYPE_LEVEL_HIGH INGENIC_GPIO_NOBIAS>;
                 ingenic,sdio-reset = <&gpd 13 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;
        };
};
```

### 设备树自定义配置

####  EMMC

|  |  |
| --- | --- |
| **属性名称** | **说明** |
| **mmc-hs200-1_8v** | 配置emmc支持hs200传输模式 |
| **cap-mmc-highspeed** | 配置emmc支持highspeed传输模式 |
| **non-removable** | 配置emmc属性为不可移动卡 |
| **max-frequency** | 通过配置该参数来指定当前模式下最大频率 |
| **enable_cpm_rx_tuning** | 该选项用于设置手动调试MSC rx相位，默认控制器自动tuning |
| **enable_cpm_rx_tuning** | 选项用于设置手动调试MSC tx相位 |
| **bus-width** | 配置数据总线宽度，支持1bit、4bit、8bit |
| **voltage-ranges** | 指定电压范围 |
| **ingenic,wp-gpios** | 写保护 |
| **ingneic,cd-gpios** | 卡检查 |
| **ingenic,rst-gpios** | 硬件reset |

####  SDIO

|  |  |
| --- | --- |
| **属性名称** | **说明** |
| **sd-uhs-sdr104** | sdr104模式 |
| **max-frequency** | 通过配置该参数来指定当前模式下最大频率 |
| **bus-width** | 配置数据总线宽度，支持1bit、4bit、8bit |
| **voltage-ranges** | 指定电压范围 |
| **ingenic,sdio_clk** | 选项用于设置手动调试MSC tx相位 |
| **keep-power-in-suspend** | 休眠保持供电 |
| **ingenic,wp-gpios** | 写保护 |
| **ingneic,cd-gpios** | 卡检查 |
| **ingenic,rst-gpios** | 硬件reset |
| **ingenic,removal-manual** | removal-dontcare, removal-nonremovable, removal-removable, removal-manual |
| **ingenic,sdio-irq** | Wifi中断 |
| **ingenic,sdio-reset** | Wifi reset |

####  SD

|  |  |
| --- | --- |
| **属性名称** | **说明** |
| **sd-uhs-sdr104** | sdr104模式 |
| **max-frequency** | 通过配置该参数来指定当前模式下最大频率 |
| **cd-inverted** | 配置支持SD卡热插拔 |
| **bus-width** | 配置数据总线宽度，支持1bit、4bit、8bit |
| **voltage-ranges** | 指定电压范围 |
| **ingenic,sdr-gpios** | 通过GPIO配置SD卡外部电路3.3V到1.8V电压切换 |
| **ingenic,wp-gpios** | 配置CD pin用于SD卡检测 |
| **ingenic,rst-gpios** | 硬件reset |

## 内核编译配置

### 内核Block & wireless基础编译配置

#### Block基础编译配置

```
Device Drivers --->

 --- MMC/SD/SDIO card support

 <*> HW reset support for eMMC 

 <*> Simple HW reset support for MMC 

 <*> MMC block device driver 

 (16) Number of minors per block device 

 < > SDIO UART/GPS class support 

 < > MMC host test driver 

 *** MMC/SD/SDIO Host Controller Drivers *** 

 [ ] MMC host drivers debugging 

 -*- Secure Digital Host Controller Interface support 

 < > SDHCI platform and OF driver helper 

 < > MMC/SD/SDIO over SPI 

 < > Synopsys DesignWare Memory Card Interface 

 < > Ingenic JZ47xx SD/Multimedia Card Interface support 

 < > VUB300 USB to SDIO/SD/MMC Host Controller support 
```

#### Wireless基础编译配置

```
Device Drivers

--- Network device support 

[*] Network core driver support

[*] Wireless LAN --->
```

### EMMC/SD/SDIO控制器驱动配置

```
Symbol: MMC_SDHCI_INGENIC [=y] 

Type : tristate 

Prompt: Ingenic(XBurst2) MMC/SD Card Controller(MSC) support 

Location: 

-> Ingenic device-drivers Configurations 

-> [MSC] (eMMC/SD/SDIO) and Device Drivers 

Depends on: MMC [=y] 

Selects: MMC_SDHCI [=y] 
```

###  wifi驱动配置

```
Ingenic device-drivers Configurations

 SDIO-WIFI drivers 

 <*> Broadcom FullMAC wireless cards support 

 (/lib//firmware/fw_bcm43438a1.bin) Firmware path 

 (/lib/firmware/BCM43438_A1.cal) NVRAM path 

 Enable Chip Interface (SDIO bus interface support) ---> 

 Interrupt type (Out-of-Band Interrupt) ---> 
```

### 内核自定义编译配置

|  |  |
| --- | --- |
| **内核配置名称** | **说明** |
| **MMC_BLOCK_MINORS** | 每个块设备的最大分区数 |
| **BCMDHD_FW_PATH** | 配置wifi模块固件路径 |
| **BCMDHD_NVRAM_PATH** | 配置wifi模块固件路径 |

## 内核版本差异

无

## 设备节点生成

###  SDIO

驱动加载成功：

```
[ 1.124644] Dongle Host Driver, version 1.363.59.144.11 (r) 

[ 1.124644] Compiled from 

[ 1.125120] Register interface [wlan0] MAC: 00:90:4c:11:22:33 

[ 1.125195] dhd_module_init: Exit err=0
```

###  EMMC

驱动加载成功：

```
[    1.838410] mmc0: new HS200 MMC card at address 0001

[    1.847123] mmcblk0: mmc0:0001 Biwin  6.96 GiB 

[    1.859712] mmcblk0boot0: mmc0:0001 Biwin  partition 1 4.00 MiB

[    1.872637] mmcblk0boot1: mmc0:0001 Biwin  partition 2 4.00 MiB

[    1.884714] mmcblk0rpmb: mmc0:0001 Biwin  partition 3 4.00 MiB

[    1.891988]  mmcblk0: p1 p2 p3 p4 p5 p6 p7 p8

```

产生设备节点：

```
/dev/mmcblk0/dev/mmcblk0p1~p7 /*对应sd的分区*/
```

## 应用程序使用说明

###  EMMC/SD测试方法

####  写测试

```
# dd if=/dev/zero of=/dev/mmcblk0 bs=1M count=100 conv=fsync
```

####  读测试

```
\# sync; echo 3 > proc/sys/vm/drop_caches 

\# time dd if=/dev/mmcblk0 of=/dev/null bs=1M count=100
```

###  SDIO测试方法

####  配置网络方法

1. /etc/wpa_supplicant

```
\# cat /etc/wpa_supplicant.conf

ctrl_interface=/var/run/wpa_supplicant

update_config=1

country=GB"

network={

    ssid="Guest" /*连接WiFi账户*/

    psk="ingenic*123" /*连接Wifi密码*/

    bssid=

    priority=1

}
```

2. 执行wifi_up.sh

```
# wifi_up.sh 

[ 13.285811] dhd_open: Enter 843c4800

[ 13.290329] dhd_conf_read_config: Ignore config file /firmware/config.txt

[ 13.297356] Final fw_path=/firmware/fw_bcm43456c5_ag.bin

[ 13.302873] Final nv_path=/firmware/nvram_ap6256.txt

[ 13.308002] Final clm_path=/firmware/clm_bcmdhd.blob

[ 13.313152] Final conf_path=/firmware/config.txt

[ 13.317926] dhd_set_bus_params: set use_rxchain 0

[ 13.322804] dhd_set_bus_params: set txglomsize 36

[ 13.328047] dhd_os_open_image: /firmware/fw_bcm43456c5_ag.bin (579388 bytes) open success

[ 13.409630] dhd_os_open_image: /firmware/nvram_ap6256.txt (2440 bytes) open success

[ 13.516230] bcmsdh_oob_intr_register: HW_OOB enabled

[ 13.521376] bcmsdh_oob_intr_register OOB irq=72 flags=0x4

[ 13.556873] Firmware up: op_mode=0x0005, MAC=c0:84:7d:31:c8:c8

[ 13.640263] dhd_txglom_enable: enable 1

[ 13.748733] dhd_open: Exit ret=0

# udhcpc: started, v1.26.2

Successfully initialized wpa_supplicant

[ 16.320669] Connecting with 8c:0c:90:98:47:cd ssid "wifi ssid", len (5) channel=153

[ 16.367807] wl_bss_connect_done succeeded with 8c:0c:90:98:47:cd

[ 16.401524] wl_bss_connect_done succeeded with 8c:0c:90:98:47:cd

udhcpc: sending select for 10.4.30.146

adding dns 219.141.136.10

adding dns 202.106.0.20
```

3. 执行ping命令连网

```
# ping www.baidu.com

PING www.baidu.com (220.181.38.150): 56 data bytes

64 bytes from 220.181.38.150: seq=0 ttl=53 time=14.347 ms

64 bytes from 220.181.38.150: seq=1 ttl=53 time=9.873 ms

64 bytes from 220.181.38.150: seq=2 ttl=53 time=12.889 ms
```

####  AP模式使用方法

1. AP模式下的配置文件（/etc/hostapd.conf）

```
cat /etc/hostapd.conf

interface=wlan0

driver=nl80211

ssid=ingenic

channel=1

hw_mode=g

macaddr_acl=0

auth_algs=1

ignore_broadcast_ssid=0

#wpa=2

#wpa_passphrase=12345678

#wpa_key_mgmt=WPA-PSK

#wpa_pairwise=TKIP

#rsn_pairwise=CCMP 
```

2. 进入AP模式

```
# wifi_ap_mode_start.sh

wlan0: interface state UNINITIALIZED->ENABLED

wlan0: AP-ENABLED 
```

3. 执行ifconfig，查看连接信息

```
# ifconfig 

lo Link encap:Local Loopback 

inet addr:127.0.0.1 Mask:255.0.0.0

UP LOOPBACK RUNNING MTU:65536 Metric:1

RX packets:0 errors:0 dropped:0 overruns:0 frame:0

TX packets:0 errors:0 dropped:0 overruns:0 carrier:0

collisions:0 txqueuelen:1 

RX bytes:0 (0.0 B) TX bytes:0 (0.0 B)

wlan0 Link encap:Ethernet HWaddr C0:84:7D:6A:F1:CD 

inet addr:192.168.1.1 Bcast:192.168.1.255 Mask:255.255.255.0

UP BROADCAST RUNNING MULTICAST MTU:1500 Metric:1

RX packets:46 errors:0 dropped:21 overruns:0 frame:0

TX packets:3 errors:0 dropped:0 overruns:0 carrier:0

collisions:0 txqueuelen:1000 

RX bytes:6509 (6.3 KiB) TX bytes:744 (744.0 B)
```

4. 使用手机（或电脑）连接wifi

连接SSID为“ingenic”对应配置文件ssid=ingenic，密码可以使用wpa_passphrase　=　12345678指定(本测试未设置密码)

连接成功会打印如下信息：

```
[ 515.831824] [dhd-wlan0] wl_ext_iapsta_event : [A] connected device 00:26:c6:58:50:54

[ 515.839828] [dhd-wlan0] wl_notify_connect_status_ap : connected device 00:26:c6:58:50:54

[ 515.851919] [dhd] CFG80211-ERROR wl_cfg80211_change_station : WLC_SCB_AUTHORIZE sta_flags_mask not set 

udhcpd: sending OFFER to 192.168.1.2

udhcpd: sending ACK to 192.168.1.2
```

5. 连接成功,Ping网络测试

```
# ping 192.168.1.2

PING 192.168.1.2 (192.168.1.2): 56 data bytes

64 bytes from 192.168.1.2: seq=0 ttl=64 time=8.148 ms

64 bytes from 192.168.1.2: seq=1 ttl=64 time=15.411 ms

64 bytes from 192.168.1.2: seq=2 ttl=64 time=12.505 ms

64 bytes from 192.168.1.2: seq=3 ttl=64 time=7.654 ms

64 bytes from 192.168.1.2: seq=4 ttl=64 time=7.505 ms
```
