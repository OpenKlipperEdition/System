# eMMC/SD/SDIO 接口

## 模块功能介绍

DWC_MSHC是一种可高度配置和可编程的高性能移动存储主控制器。DWC_MSHC其数据传输的总接口为AXI。EMMC 是英文 Embedded Multi-Media Card（嵌入式多媒体卡）的缩写，X2000支持EMMC协议Electrical Standard（5.1）。

X2000使用的ap6256芯片。 AP6256是基于BCM4345C5方案的集成wifi和bluetooth的功能模块，它包括用于WiFi的SDIO接口，和用于蓝牙的UART/PCM接口。

### GPIO功能描述:

####  EMMC


|  |  |  |
| --- | --- | --- |
| **Name** | **I/O** | **Funtion** |
| **MSC0_CLK** | GPIO-PD17 | FUNTION0 |
| **MSC0_CMD** | GPIO-PD18 | FUNTION0 |
| **MSC0_DATA** | GPIO-(PD19~PD26) | FUNTION0 |

####  SDIO

|  |  |  |
| --- | --- | --- |
| **Name** | **I/O** | **Funtion** |
| **SDIO** | GPIO-(PD08~PD13) | FUNTION0 |
| **WL_REG_ON** | GPIO-PD0 |  |
| **WL_WAKE_HOST** | GPIO-PD1 |  |

#### SD

|  |  |  |
| --- | --- | --- |
| **Name** | **I/O** | **Funtion** |
| **MSC0_CLK** | GPIO-PE00 | FUNTION0 |
| **MSC0_CMD** | GPIO-PE01 | FUNTION0 |
| **MSC0_DATA** | GPIO-(PE02~PE05) | FUNTION0 |

### MSC控制器命名对应关系：

|  |  |  |  |  |  |
| --- | --- | --- | --- | --- | --- |
| **控制器** | **Base** | **bootrom** | **kernel** | **pm-spec** | **hardware-pcd** |
| **控制器0** | 13450000 | msc0 | msc0 | msc0 | msc0 |
| **控制器1** | 13460000 | msc1 | msc1 | sdio | sdio |
| **控制器2** | 13490000 | Msc2 | msc2 | msc1 | msc1 |

## 驱动源码位置

驱动源码位于：

***module_drivers/drivers/mmc/host/***
```
├──sdhci.c
├──sdhci-ingenic.c
├──ingenic_sdio.c
```

## 设备树配置

设备树所在位置：

kernel内核(version <= 5.10)dts文件路径：

***module_driver/dts/x2000.dtsi***

kernel内核(version > 5.10)dts文件路径：

***module_driver/dts/x2000/x2000.dtsi***

msc0控制器描述：
```c
msc0: msc@0x13450000 { 

 compatible = "ingenic,sdhci";

 reg = <0x13450000 0x10000>;

 status = "disabled";

 interrupt-parent = <&core_intc>;

 interrupts = <IRQ_MSC0>;

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

};
```

msc2控制器描述：
```c
msc2: msc@0x13490000 {

 compatible = "ingenic,sdhci";

 reg = <0x13490000 0x10000>;

 status = "disabled";

 interrupt-parent = <&core_intc>;

 interrupts = <IRQ_MSC2>;

}; 
```

### 设备树默认配置

在板级设备树halley5_v30.dts中，默认配置产生msc1 msc2设备，描述如下：

#### EMMC
```c
&msc0 {

 status = "disable"; 

 /*mmc-hs200-1_8v;*/

 cap-mmc-highspeed;

 non-removable;

 max-frequency = <50000000>;

 bus-width = <4>;

 non-removable;

 voltage-ranges = <1800 3300>;

 /* special property */

 ingenic,wp-gpios = <0>;

 ingenic,cd-gpios = <0>;

 ingenic,rst-gpios = <0>;

};
```

####  SDIO
```c
&msc1 {

 status = "okay";

 pinctrl-names ="default","enable", "disable";

 pinctrl-0 = <&msc1_4bit>;

 pinctrl-1 = <&rtc32k_enable>;

 pinctrl-2 = <&rtc32k_disable>;

 sd-uhs-sdr104;

 max-frequency = <100000000>;

 bus-width = <4>;

 voltage-ranges = <1800 3300>;

 non-removable;

 ingenic,sdio_clk = <1>;

 keep-power-in-suspend;

 /* special property */

 ingenic,sdr-gpios = <0>;

 ingenic,wp-gpios = <0>;

 ingenic,removal-manual; /*removal-dontcare, removal-nonremovable, removal-removable, removal-manual*/

 bcmdhd_wlan: bcmdhd_wlan {

 compatible = "android,bcmdhd_wlan";

 ingenic,sdio-irq = <&gpd 0 IRQ_TYPE_LEVEL_HIGH INGENIC_GPIO_NOBIAS>;

 ingenic,sdio-reset = <&gpd 1 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;

 };

};
```

####  SD
```c
&msc2 {

 status = "okay";

 pinctrl-names ="default";

 pinctrl-0 = <&msc2_4bit>;

 sd-uhs-sdr104;

 max-frequency = <200000000>;

 /*cd-inverted;*/

 bus-width = <4>;

 voltage-ranges = <1800 3300>;

 cd-gpios = <&gpc 12 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;

 /* special property */

 ingenic,sdr-gpios = <&gpc 0 GPIO_ACTIVE_HIGH INGENIC_GPIO_NOBIAS>;

 ingenic,rst-gpios = <0>;

};
```

### 设备树自定义配置

####  EMMC

|  |  |
| --- | --- |
| **属性名称** | **说明** |
| **mmc-hs200-1_8v**| 配置emmc支持hs200传输模式 |
| **cap-mmc-highspeed**| 配置emmc支持highspeed传输模式 |
| **non-removable**| 配置emmc属性为不可移动卡 |
| **max-frequency**| 通过配置该参数来指定当前模式下最大频率 |
| **enable_cpm_rx_tuning**| 该选项用于设置手动调试MSC rx相位，默认控制器自动tuning |
| **enable_cpm_rx_tuning**| 选项用于设置手动调试MSC tx相位 |
| **bus-width**| 配置数据总线宽度，支持1bit、4bit、8bit |
| **voltage-ranges**| 指定电压范围 |
| **ingenic,wp-gpios**| 写保护 |
| **ingneic,cd-gpios**| 卡检查 |
| **ingenic,rst-gpios**| 硬件reset |

####  SDIO

|  |  |
| --- | --- |
| **属性名称** | **说明** |
| **sd-uhs-sdr104**| sdr104模式 |
| **max-frequency**| 通过配置该参数来指定当前模式下最大频率 |
| **bus-width**| 配置数据总线宽度，支持1bit、4bit、8bit |
| **voltage-ranges**| 指定电压范围 |
| **ingenic,sdio_clk**| 选项用于设置手动调试MSC tx相位 |
| **keep-power-in-suspend**| 休眠保持供电 |
| **ingenic,wp-gpios**| 写保护 |
| **ingneic,cd-gpios**| 卡检查 |
| **ingenic,rst-gpios**| 硬件reset |
| **ingenic,removal-manual**| removal-dontcare, removal-nonremovable, removal-removable, removal-manual |
| **ingenic,sdio-irq**| Wifi中断 |
| **ingenic,sdio-reset**| Wifi reset |

####  SD

|  |  |
| --- | --- |
| **属性名称** | **说明** |
| **sd-uhs-sdr104**| sdr104模式 |
| **max-frequency**| 通过配置该参数来指定当前模式下最大频率 |
| **cd-inverted**| 配置支持SD卡热插拔 |
| **bus-width**| 配置数据总线宽度，支持1bit、4bit、8bit |
| **voltage-ranges**| 指定电压范围 |
| **ingenic,sdr-gpios**| 通过GPIO配置SD卡外部电路3.3V到1.8V电压切换 |
| **ingenic,wp-gpios**| 配置CD pin用于SD卡检测 |
| **ingenic,rst-gpios**| 硬件reset |

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

 (/firmware/fw_bcm43456c5_ag.bin) Firmware path 

 (/firmware/nvram_ap6256.txt) NVRAM path 

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

###  EMMC

驱动加载成功：
```
[ 1.575803] mmc1: new ultra high speed SDR104 SDHC card at address aaaa 

[ 1.593001] mmcblk0: mmc1:aaaa SC16G 14.8 GiB 

[ 1.601920] Alternate GPT is invalid, using primary GPT. 

[ 1.607432] mmcblk0: p1 p2 p3 p4 p5 p6 p7
```

产生设备节点：
```
/dev/mmcblk0 /dev/mmcblk0p1~p7 /*对应emmc的分区*/
```

###  SDIO

驱动加载成功：
```
[ 1.124644] Dongle Host Driver, version 1.363.59.144.11 (r) [ 1.124644] Compiled from [ 1.125120] Register interface [wlan0] MAC: 00:90:4c:11:22:33 [ 1.125120] [ 1.125195] dhd_module_init: Exit err=0
```

###  SD

驱动加载成功：
```
[ 1.575803] mmc1: new ultra high speed SDR104 SDHC card at address aaaa

[ 1.593001] mmcblk0: mmc1:aaaa SC16G 14.8 GiB 

[ 1.601920] Alternate GPT is invalid, using primary GPT.

[ 1.607432] mmcblk0: p1 p2 p3 p4 p5 p6 p7
```

产生设备节点：
```
/dev/mmcblk0/dev/mmcblk0p1~p7 /*对应sd的分区*/
```

## 应用程序使用说明

### EMMC/SD测试方法

#### 写测试
```bash
# dd if=/dev/zero of=/dev/mmcblk0 bs=1M count=100 conv=fsync
```

#### 读测试
```bash
# sync; echo 3 > proc/sys/vm/drop_caches 

# time dd if=/dev/mmcblk0 of=/dev/null bs=1M count=100
```

### SDIO测试方法

#### 配置网络方法

1. /etc/wpa_supplicant
```bash
# cat /etc/wpa_supplicant.conf

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
```bash
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

/#udhcpc: started, v1.26.2

Successfully initialized wpa_supplicant

[ 16.320669] Connecting with 8c:0c:90:98:47:cd ssid "wifi ssid", len (5) channel=153

[ 16.367807] wl_bss_connect_done succeeded with 8c:0c:90:98:47:cd

[ 16.401524] wl_bss_connect_done succeeded with 8c:0c:90:98:47:cd

udhcpc: sending select for 10.4.30.146

adding dns 219.141.136.10

adding dns 202.106.0.20

3. 执行ping命令连网

/#ping www.baidu.com

PING www.baidu.com (220.181.38.150): 56 data bytes

64 bytes from 220.181.38.150: seq=0 ttl=53 time=14.347 ms

64 bytes from 220.181.38.150: seq=1 ttl=53 time=9.873 ms

64 bytes from 220.181.38.150: seq=2 ttl=53 time=12.889 ms
```

#### airkiss配网测试

AirKiss是微信硬件平台为Wi-Fi设备提供的微信配网、局域网发现和局域网通讯的技术。

1. 配网

注：以下操作必须在WIFI(外网)环境下配置

使用AirkissDubugger软件（手机端下载）配置wifi

2. 执行airkiss命令
```bash
# airkiss

[ 113.669340] dhd_open: Enter 842f1000

[ 113.677706] Dongle Host Driver, version 1.579.77.41.26 (r-20200429-2)

[ 113.695902] ======== PULL WL_REG_ON(-1) HIGH! ========

[ 114.122604] F1 signature read @0x18000000=0x15294345

[ 114.130573] F1 signature OK, socitype:0x1 chip:0x4345 rev:0x9 pkg:0x2

[ 114.137706] DHD: dongle ram size is set to 819200(orig 819200) at 0x198000

[ 114.153611] [dhd] dhd_conf_read_config : Ignore config file /firmware/config.txt

[ 114.161413] [dhd] dhd_conf_set_path_params : Final fw_path=/firmware/fw_bcm43456c5_ag.bin

[ 114.169851] [dhd] dhd_conf_set_path_params : Final nv_path=/firmware/nvram_ap6256.txt

[ 114.177960] [dhd] dhd_conf_set_path_params : Final clm_path=/firmware/clm_bcm43456c5_ag.blob

[ 114.186695] [dhd] dhd_conf_set_path_params : Final conf_path=/firmware/config.txt

[ 114.194942] dhd_os_open_image: /firmware/fw_bcm43456c5_ag.bin (579388 bytes) open success

[ 114.266737] dhd_os_open_image: /firmware/nvram_ap6256.txt (2440 bytes) open success

[ 114.283245] dhdsdio_write_vars: Download, Upload and compare of NVRAM succeeded.

[ 114.580570] [dhd-wlan0] wl_android_wifi_on : Success

Easy setup target library v4.0.0

state: 0 --> 1

state: 1 --> 3

state: 3 --> 5

ssid: JZ_SW /*软件端配置的网络账号*/

password: jz_sw%#!135 /*软件端配置的网络密码*/

/etc/wpa_supplicant.conf create successfully!

random: 0xc1

time elapsed: 0s

/#udhcpc: started, v1.31.1

Successfully initialized wpa_supplicant

[ 118.985525] [dhd-wlan0] wl_run_escan : LEGACY_SCAN sync ID: 0, bssidx: 0

wlan0: Trying to associate with c4:01:7c:78:c3:dd (SSID='JZ_SW' freq=5765 MHz)

[ 121.338272] [dhd-wlan0] wl_cfg80211_connect : Connecting with c4:01:7c:78:c3:dd ssid "JZ_SW", len (5), sec=wpa2psk/mfpn/tkipaes, channel=153

[ 121.402046] [dhd-wlan0] wl_ext_iapsta_event : [S] Link UP with c4:01:7c:78:c3:dd

[ 121.409697] [dhd-wlan0] wl_notify_connect_status : wl_bss_connect_done succeeded with c4:01:7c:78:c3:dd 

wlan0: Associated with c4:01:7c:78:c3:dd

wlan0: CTRL-EVENT-SUBNET-STATUS-UPDATE status=0

wlan0: WPA: Key negotiation completed with c4:01:7c:78:c3:dd [PTK=CCMP GTK=TKIP]

wlan0: CTRL-EV[ 121.451231] [dhd-wlan0] wl_notify_connect_status : wl_bss_connect_done succeeded with c4:01:7c:78:c3:dd vndr_oui: 00-90-4C 00-13-92 

ENT-CONNECTED - Connection to c4:01:7c:78:c3:dd completed [id=0 id_str=]

udhcpc: sending select for 10.10.30.203

udhcpc: lease of 10.10.30.203 obtained, lease time 86400

adding dns 192.168.1.2
```

3. 执行ping命令连接网络
```bash
# ping www.baidu.com

PING www.baidu.com (220.181.38.150): 56 data bytes

64 bytes from 220.181.38.150: seq=0 ttl=53 time=14.347 ms

64 bytes from 220.181.38.150: seq=1 ttl=53 time=9.873 ms
```

#### AP模式使用方法

1. AP模式下的配置文件（/etc/hostapd.conf）
```
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
```bash
# wifi_ap_mode_start.sh

wlan0: interface state UNINITIALIZED->ENABLED

wlan0: AP-ENABLED 
```

3. 执行ifconfig，查看连接信息
```bash
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

连接成功会打印如下信息:
```
[ 515.831824] [dhd-wlan0] wl_ext_iapsta_event : [A] connected device 00:26:c6:58:50:54

[ 515.839828] [dhd-wlan0] wl_notify_connect_status_ap : connected device 00:26:c6:58:50:54

[ 515.851919] [dhd] CFG80211-ERROR) wl_cfg80211_change_station : WLC_SCB_AUTHORIZE sta_flags_mask not set 

udhcpd: sending OFFER to 192.168.1.2

udhcpd: sending ACK to 192.168.1.2
```

5. 连接成功,Ping网络测试
```bash
# ping 192.168.1.2

PING 192.168.1.2 (192.168.1.2): 56 data bytes

64 bytes from 192.168.1.2: seq=0 ttl=64 time=8.148 ms

64 bytes from 192.168.1.2: seq=1 ttl=64 time=15.411 ms

64 bytes from 192.168.1.2: seq=2 ttl=64 time=12.505 ms

64 bytes from 192.168.1.2: seq=3 ttl=64 time=7.654 ms

64 bytes from 192.168.1.2: seq=4 ttl=64 time=7.505 ms
```
