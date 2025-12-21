#  USB OTG 控制器接口

## 模块功能介绍

通用串行总线USB( Universal Serial Bus) 是一种新兴的并逐渐取代其他接口标准的数据通信方式，由 Intel、Compaq、Digital、IBM、Microsoft、NEC 及 Northern Telecom 等计算机公司和通信公司于 1995 年联合制定，并逐渐形成了行业标准。

USB 总线作为一种高速串行总线，其极高的传输速度可以满足高速数据传输的应用环境要求，且该总线还兼有供电简单（可总线供电）、安装配置便捷（支持即插即用和热插拔）、 扩展端口简易（通过集线器最多可扩展 127 个外设）、传输方式多样化（4 种传输模式），以及兼容良好（产品升级后向下兼容）等优点。

USB OTG控制器实现了USB主机和多种可同时访问的便携式外围设备之间进行串行数据交换。

x26xx系列芯片有两个usb控制器，支持usb功能。

## 驱动源码位置

驱动源码所在位置：

***drivers/usb/dwc2***

```
├── core.c
├── core.h
├── core_intr.c
├── debugfs.c
├── debug.h
├── gadget.c
├── hcd.c
├── hcd_ddma.c
├── hcd.h
├── hcd_intr.c
├── hcd_queue.c
├── hw.h
├── Kconfig
├── Makefile
├── pci.c
├── platform.c
```

## 设备树配置

设备树所在位置：

kernel内核dts文件路径：

kernel内核(version <= 5.10)dts文件路径：

***module_driver/dts/x2600.dtsi***

kernel内核(version > 5.10)dts文件路径：

***module_driver/dts/x2600/x2600.dtsi***

设备树描述：

usb 控制器配置：

```c
otg: otg@0x13500000 {
    compatible = "ingenic,x2600-dwc2-hsotg";
    reg = <0x13500000 0x40000>;
    interrupt-parent = <&core_intc>;
    interrupts = <IRQ_OTG>;
    clocks = <&clock CLK_GATE_OTG>;
    clock-names = "gate_otg";
    ingenic,usbphy=<&otg_phy>;

    g-rx-fifo-size = <1096>;
    g-np-tx-fifo-size = <512>;
    g-tx-fifo-size = <16 16 16 128 256 256 512 768>;
    status = "okay";
#ifdef CONFIG_USB_DWC2_EXT_ID_PIN
    external-id-pin;
#endif
#ifdef CONFIG_USB_DWC2_FORCE_FULL_SPEED
    force-full-speed;
#endif
};

usb: usb@0x13540000 {
    compatible = "ingenic,x2600-dwc2-hsotg";
    reg = <0x13540000 0x40000>;
    interrupt-parent = <&core_intc>;
    interrupts = <IRQ_USB>;
    clocks = <&clock CLK_GATE_USB>;
    clock-names = "gate_usb";
    ingenic,usbphy=<&usb_phy>;
    g-rx-fifo-size = <1096>;
    g-np-tx-fifo-size = <512>;
    g-tx-fifo-size = <16 16 16 128 256 256 512 768>;
    dr_mode = "host";
    status = "okay";
};
```

phy 控制器配置：

```c
otg_phy: otg_phy {
    #address-cells = <1>;
    #size-cells = <1>;
    compatible = "ingenic,usbphy0-x2600";
    reg = <0x10000000 0x1000 0x10078000 0x1000  0x13500000 0x40000>;
#ifdef CONFIG_USB_DWC2_EXT_VBUS_DETECT
    external-vbus-detect;
#endif
};

usb_phy: usb_phy {
    #address-cells = <1>;
    #size-cells = <1>;
    compatible = "ingenic,usbphy1-x2600";
    reg = <0x10000000 0x1000 0x10078400 0x1000>;
    external-vbus-detect;
    disable-sw-switch-id;
};
```

### 设备树配置说明

|  |  |  |  |  |  |
| --- | --- | --- | --- | --- | --- |
| 通路配置 | 控制器名称 | 控制器地址 | Phy名称 | Phy地址 | 功能 |
| 通路一 | otg | 0x13500000 | otg_phy | 0x10078000 | 通用usb功能，支持otg功能 |
| 通路二 | usb | 0x13540000 | usb_phy | 0x10078400 | 通用usb功能 |

### 设备树默认配置

设备树默认编译会产生otg 和otg_phy设备

kernel内核(version <= 5.10)

在**module_drivers/dts/x2600_halley_v1.0.dts** 下配置：

kernel内核(version > 5.10)

在**module_drivers/dts/x2600/halley7.dts** 下配置

```c
&otg {

    dr_mode = "otg"; // host,peripheral,otg

    device-using-dma = <1>;

    status = "okay";

};

&usb {

    dr_mode = "host"; // host,peripheral,otg

    device-using-dma = <1>;

    status = "okay";

};

&otg_phy {

    status = "okay";

};

&usb_phy {

    status = "okay";

};
```

### 设备树自定义配置

用户可根据实际需求关闭该节点，或进行以下配置：

|  |  |
| --- | --- |
| **属性名称** | **说明** |
| ingenic,id-dete-gpio | 配置usb 用于探测 id变化的GPIO  |
| ingenic,vbus-dete-gpio | 配置usb 用于插拔检测的GPIO  |
| Ingenic,drvvbus-gpio | 配置usb用于控制vbus的GPIO  |

## 内核编译配置

需要根据USB不同功能选择不同的配置, USB composite功能可以支持多个device功能同时使用。

USB 作为host:

|  |  |
| --- | --- |
| **支持功能** | **说明** |
| **mass storage** | 作为主机,可识别大容量存储设备 |
| **usb camera** | 作为主机,可识别USB摄像头设备 |
| **hid** | 作为主机,可以识别人机交互设备,如鼠标.. |
| **cdrom** | 作为主机,可以识别光盘驱动设备 |

USB 作为device：

|  |  |
| --- | --- |
| **支持功能** | **说明** |
| **mass storage** | usb作为大容量存储设备 |
| **adb (Android Debug Bridge)** | 用于USB debug调试功能 |
| **hid** | usb作为人机交互设备,如鼠标、键盘 |
| **uvc (usb video class)** | usb作为视频类设备 |
| **uac1.0 (usb audio class1.0)** | usb作为音频类设备 |
| **printer** | usb 作为打印机设备 |
| **rndis** | usb 作为网卡设备 |
| **serial** | usb 作为串口设备 |


### 内核默认编译配置

USB host 默认支持mass storage、usb camera、hid、rndis、cdrom。

USB device composite默认支持mass storage、adb、hid、uvc、rndis、printer、uac1.0、mtp可以同时使用。

表格 1 usb host 功能支持情况

|  |  |  |  |
| --- | --- | --- | --- |
| 功能类型 | Kernel-4.4.94-deconfig是否默认配置 | Kernel-5.10-defconfig是否默认配置 | X26xx halley板级是否支持该功能 |
| Mass storage | YES | YES | YES |
| Usb camera | YES | YES | YES |
| hid | YES | YES | YES |
| rndis | YES | YES | YES |
| cdrom | YES | YES | YES |

表格 2 usb device 功能支持情况

|  |  |  |  |
| --- | --- | --- | --- |
| 功能类型 | Kernel-4.4.94-defconfig是否默认配置 | Kernel-5.10-defconfig是否默认配置 | X26xxhalley板级是否支持该功能 |
| Mass storage | YES | YES | YES |
| adb | YES | YES | YES |
| hid | YES | YES | YES |
| uvc | NO | NO | NO |
| rndis | YES | YES | YES |
| printer | YES | YES | YES |
| Uac1.0 | YES | YES | YES |
| mtp | YES | YES | YES |
| cdrom | NO | NO | NO |

### 内核自定义编译配置

#### USB控制器模式配置

可通过内核配置或设备树选择配置USB 控制器为host only、device only或otg模式，配置方法如下：

通过内核配置USB控制器为host only模式：

```
Symbol: USB_DWC2_HOST [=n]

Type : boolean

Prompt: Host only mode

Location:

 -> Device Drivers

 -> USB support (USB_SUPPORT [=y])

 -> DesignWare USB2 DRD Core Support (USB_DWC2 [=y])

 -> DWC2 Mode Selection (<choice> [=y])

Defined at drivers/usb/dwc2/Kconfig:24

Depends on: <choice> && (USB [=y]=y || USB_DWC2 [=y]=m && USB [=y]
```

通过内核配置USB控制器为device only模式：

```
Symbol: USB_DWC2_PERIPHERAL [=n]

Type : boolean

Prompt: Gadget only mode

Location:

 -> Device Drivers

 -> USB support (USB_SUPPORT [=y])

 -> DesignWare USB2 DRD Core Support (USB_DWC2 [=y])

 -> DWC2 Mode Selection (<choice> [=y])

Defined at drivers/usb/dwc2/Kconfig:34

Depends on: <choice> && (USB_GADGET [=y]=y || USB_GADGET [=y]=USB_DWC2 [=y])
```

通过内核配置USB控制器为otg模式：

```
Symbol: USB_DWC2_DUAL_ROLE [=y]

Type : boolean

Prompt: Dual Role mode

Location:

 -> Device Drivers

 -> USB support (USB_SUPPORT [=y])

 -> DesignWare USB2 DRD Core Support (USB_DWC2 [=y])

 -> DWC2 Mode Selection (<choice> [=y])

Defined at drivers/usb/dwc2/Kconfig:43

Depends on: <choice> && (USB [=y]=y && USB_GADGET [=y]=y || USB_DWC2 [=y]=m && USB [=y] && USB_GADGET [=y]
```

或者可在设备树中配置USB控制器模式，以x2660_halley_v1.0.dts为例：

```c
&otg {

    dr_mode = "otg"; // host,peripheral,otg

    device-using-dma = <1>;

    status = "okay";

};

&usb {

    dr_mode = "host"; // host,peripheral,otg

    device-using-dma = <1>;

    status = "okay";

};
```

#### USB host内核配置

#####  USB host Mass Storage

(1) VFAT (Windows-95) fs support

```
Symbol: VFAT_FS [=y]

Type : tristate

Prompt: VFAT (Windows-95) fs support

Location:

-> File systems

-> DOS/FAT/NT Filesystems

Defined at fs/fat/Kconfig:60

Depends on: BLOCK [=y]

Selects: FAT_FS [=y]
```

(2) Codepage 437 (United States, Canada)

```
Symbol: NLS_CODEPAGE_437 [=y]

Type : tristate

Prompt: Codepage 437 (United States, Canada)

Location:

-> File systems

-> Native language support (NLS [=y])

Defined at fs/nls/Kconfig:39

Depends on: NLS [=y]
```

(3) NLS ISO 8859-1 (Latin 1; Western European Languages)

```
Symbol: NLS_ISO8859_1 [=y]

Type : tristate

Prompt: NLS ISO 8859-1 (Latin 1; Western European Languages)

Location:

-> File systems

-> Native language support (NLS [=y])

Defined at fs/nls/Kconfig:318

Depends on: NLS [=y]
```

(4) SCSI disk support

```
Symbol: BLK_DEV_SD [=y]

Type : tristate

Prompt: SCSI disk support

Location:

-> Device Drivers

-> SCSI device support

Defined at drivers/scsi/Kconfig:73

Depends on: SCSI [=y]
```

(5) USB Mass Storage support

```
Symbol: USB_STORAGE [=y]

Type : tristate

Prompt: USB Mass Storage support

Location:

-> Device Drivers

-> USB support (USB_SUPPORT [=y])

-> Support for Host-side USB (USB [=y])

Defined at drivers/usb/storage/Kconfig:8

Depends on: USB_SUPPORT [=y] && USB [=y] && SCSI [=y]
```

#####  USB host camera

```
Symbol: USB_VIDEO_CLASS [=y]

Type : tristate

Prompt: USB Video Class (UVC)

Location:

-> Device Drivers

-> Multimedia support (MEDIA_SUPPORT [=y])

-> Media USB Adapters (MEDIA_USB_SUPPORT [=y])

Defined at drivers/media/usb/uvc/Kconfig:1

Depends on: USB [=y] && MEDIA_SUPPORT [=y] && MEDIA_USB_SUPPORT [=y] && MEDIA_CAMERA_SUPPORT [=y] && VIDEO_V4L2 [=y]

Selects: VIDEOBUF2_VMALLOC [=n]
```

#####  USB host hid mouse

(1) USB HIDBP Mouse (simple Boot) support

```
Symbol: USB_MOUSE [=y]

Type : tristate

Prompt: USB HIDBP Mouse (simple Boot) support

Location:

-> Device Drivers

-> HID support

-> USB HID support

-> USB HID Boot Protocol module_drivers/drivers

Defined at drivers/hid/usbhid/Kconfig:66

Depends on: USB_HID [=n]!=y && EXPERT [=y] && USB [=y] && INPUT [=y]
```

(2) Mouse interface

```
Symbol: INPUT_MOUSEDEV [=y]

Type : tristate

Prompt: Mouse interface

Location:

-> Device Drivers

-> Input device support

-> Generic input layer (needed for keyboard, mouse, ...) (INPUT [=y])

Defined at drivers/input/Kconfig:95

Depends on: !UML && INPUT [=y]
```

#####  USB host rndis

(1) USB Network Adapters
```
Symbol: USB_USBNET [=y]

Type : tristate

Prompt: Multi-purpose USB Networking Framework

Location:

-> Device Drivers

-> Network device support (NETDEVICES [=y])

-> USB Network Adapters (USB_NET_DRIVERS [=y])

Defined at drivers/net/usb/Kconfig:121

Depends on: NETDEVICES [=y] && USB_NET_DRIVERS [=y]

Selects: MII [=y]

Selected by: USB_NET_RNDIS_WLAN [=n] && NETDEVICES [=y] && WLAN [=y] && USB [=y] && CFG80211 [=y]
```

(2) Multi-purpose USB Networking Framework
```
Symbol: USB_NET_RNDIS_HOST [=y]
Type  : tristate
Defined at drivers/net/usb/Kconfig:398
Prompt: Host for RNDIS and ActiveSync devices
Depends on: NETDEVICES [=y] && USB_NET_DRIVERS [=y] && USB_USBNET [=y]
Location:
-> Device Drivers
-> Network device support (NETDEVICES [=y])
-> USB Network Adapters (USB_NET_DRIVERS [=y])
-> Multi-purpose USB Networking Framework (USB_USBNET [=y])
Selects: USB_NET_CDCETHER [=y]
Selected by [n]:
- USB_NET_RNDIS_WLAN [=n] && NETDEVICES [=y] && WLAN [=n] && USB [=y] && CFG80211 [=y]
```

#####  USB host cdrom

(1)ISO 9660 CDROM file system support

```
Symbol: ISO9660_FS [=y]

Type : tristate

Prompt: ISO 9660 CDROM file system support

Location:

-> File systems

(1)-> CD-ROM/DVD Filesystems

Defined at fs/isofs/Kconfig:1

Depends on: BLOCK [=y]
```

(2) Microsoft Joliet CDROM extensions

```
Symbol: JOLIET [=y]

Type : boolean

Prompt: Microsoft Joliet CDROM extensions

Location:

-> File systems

-> CD-ROM/DVD Filesystems

-> ISO 9660 CDROM file system support (ISO9660_FS [=y])

Defined at fs/isofs/Kconfig:17

Depends on: BLOCK [=y] && ISO9660_FS [=y]

Selects: NLS [=y]
```

(3) SCSI CDROM support

```
Symbol: BLK_DEV_SR [=y]

Type : tristate

Prompt: SCSI CDROM support

Location:

-> Device Drivers

(1)-> SCSI device support

Defined at drivers/scsi/Kconfig:129

Depends on: SCSI [=y]
```

(4) SCSI generic support

```
Symbol: CHR_DEV_SG [=y]

Type : tristate

Prompt: SCSI generic support

Location:

-> Device Drivers

(1)-> SCSI device support

Defined at drivers/scsi/Kconfig:152

Depends on: SCSI [=y]
```

#### USB gadget内核配置

#####  USB device mass storage

```
Symbol: USB_CONFIGFS_MASS_STORAGE [=y]

Type : boolean

Prompt: Mass storage

Location:

-> Device Drivers

-> USB support (USB_SUPPORT [=y])

-> USB Gadget Support (USB_GADGET [=y])

-> USB Gadget Drivers (<choice> [=y])

-> USB functions configurable through configfs (USB_CONFIGFS [=y])

Defined at drivers/usb/gadget/Kconfig:338

Depends on: <choice> && USB_CONFIGFS [=y] && BLOCK [=y]

Selects: USB_F_MASS_STORAGE [=y]
```

##### USB device adb

```
Symbol: USB_CONFIGFS_F_FS [=y]

Type : boolean

Prompt: Function filesystem (FunctionFS)

Location:

-> Device Drivers

-> USB support (USB_SUPPORT [=y])

-> USB Gadget Support (USB_GADGET [=y])

-> USB Gadget Drivers (<choice> [=y])

-> USB functions configurable through configfs (USB_CONFIGFS [=y])

Defined at drivers/usb/gadget/Kconfig:362

Depends on: <choice> && USB_CONFIGFS [=y]

Selects: USB_F_FS [=y]
```

#####  USB device hid

```
Symbol: USB_CONFIGFS_F_HID [=y]

Type : boolean

Prompt: HID function

Location:

-> Device Drivers

-> USB support (USB_SUPPORT [=y])

-> USB Gadget Support (USB_GADGET [=y])

-> USB Gadget Drivers (<choice> [=y])

-> USB functions configurable through configfs (USB_CONFIGFS [=y])

Defined at drivers/usb/gadget/Kconfig:419

Depends on: <choice> && USB_CONFIGFS [=y]

Selects: USB_F_HID [=n]
```

#####  USB device ncm

```
Symbol: USB_CONFIGFS_NCM [=y]

Type  : bool

Defined at drivers/usb/gadget/Kconfig:279

Prompt: Network Control Model (CDC NCM)

Depends on: USB_SUPPORT [=y] && USB_GADGET [=y] && USB_CONFIGFS [=y] && NET [=y]

Location:

-> Device Drivers

-> USB support (USB_SUPPORT [=y])

-> USB Gadget Support (USB_GADGET [=y])

-> USB Gadget functions configurable through configfs (USB_CONFIGFS [=y])

Selects: USB_U_ETHER [=y] && USB_F_NCM [=n] && CRC32 [=y]
```

#####  USB device remote NDIS

```
Symbol: USB_CONFIGFS_RNDIS [=y]

Type : boolean

Prompt: RNDIS

Location:

-> Device Drivers

-> USB support (USB_SUPPORT [=y])

-> USB Gadget Support (USB_GADGET [=y])

-> USB Gadget Drivers (<choice> [=y])

-> USB functions configurable through configfs (USB_CONFIGFS [=y])

Defined at drivers/usb/gadget/Kconfig:297

Depends on: <choice> && USB_CONFIGFS [=y] && NET [=y]

Selects: USB_U_ETHER [=n] && USB_F_RNDIS [=n]
```

#####  USB device serial

(1) Generic serial bulk in/out

```
Symbol: USB_CONFIGFS_SERIAL [=y]

Type : boolean

Prompt: Generic serial bulk in/out

Location:

-> Device Drivers

-> USB support (USB_SUPPORT [=y])

-> USB Gadget Support (USB_GADGET [=y])

-> USB Gadget Drivers (<choice> [=y])

-> USB functions configurable through configfs (USB_CONFIGFS [=y])

Defined at drivers/usb/gadget/Kconfig:241

Depends on: <choice> && USB_CONFIGFS [=y] && TTY [=y]

Selects: USB_U_SERIAL [=n] && USB_F_SERIAL [=n]
```

(2) Abstract Control Model (CDC ACM)

```
Symbol: USB_CONFIGFS_ACM [=y]

Type : boolean

Prompt: Abstract Control Model (CDC ACM)

Location:

-> Device Drivers

-> USB support (USB_SUPPORT [=y])

-> USB Gadget Support (USB_GADGET [=y])

-> USB Gadget Drivers (<choice> [=y])

-> USB functions configurable through configfs (USB_CONFIGFS [=y])

Defined at drivers/usb/gadget/Kconfig:250

Depends on: <choice> && USB_CONFIGFS [=y] && TTY [=y]

Selects: USB_U_SERIAL [=n] && USB_F_ACM [=n]
```

(3) Object Exchange Model (CDC OBEX)

```
Symbol: USB_CONFIGFS_OBEX [=y]

Type : boolean

Prompt: Object Exchange Model (CDC OBEX)

Location:

-> Device Drivers

-> USB support (USB_SUPPORT [=y])

-> USB Gadget Support (USB_GADGET [=y])

-> USB Gadget Drivers (<choice> [=y])

-> USB functions configurable through configfs (USB_CONFIGFS [=y])

Defined at drivers/usb/gadget/Kconfig:260

Depends on: <choice> && USB_CONFIGFS [=y] && TTY [=y]

Selects: USB_U_SERIAL [=n] && USB_F_OBEX [=n]
```

#####  USB device printer

```
Symbol: USB_CONFIGFS_F_PRINTER [=y]

Type : boolean

Prompt: Printer function

Location:

-> Device Drivers

-> USB support (USB_SUPPORT [=y])

-> USB Gadget Support (USB_GADGET [=y])

-> USB Gadget Drivers (<choice> [=y])

-> USB functions configurable through configfs (USB_CONFIGFS [=y])

Defined at drivers/usb/gadget/Kconfig:469

Depends on: <choice> && USB_CONFIGFS [=y]

Selects: USB_F_PRINTER [=y]
```

#####  USB device uac1.0

```
Symbol: USB_CONFIGFS_F_UAC1 [=y]

Type : boolean

Prompt: Audio Class 1.0

Location:

-> Device Drivers

-> USB support (USB_SUPPORT [=y])

-> USB Gadget Support (USB_GADGET [=y])

-> USB Gadget Drivers (<choice> [=y])

-> USB functions configurable through configfs (USB_CONFIGFS [=y])

Defined at drivers/usb/gadget/Kconfig:402

Depends on: <choice> && USB_CONFIGFS [=y] && SND [=y]

Selects: USB_LIBCOMPOSITE [=y] && SND_PCM [=y] && USB_F_UAC1 [=y]
```

#### USB legacy内核配置

#####  USB device serial

```
Symbol: USB_G_SERIAL [=y]

Type : tristate

Prompt: Serial Gadget (with CDC ACM and CDC OBEX support)

Location:

-> Device Drivers

-> USB support (USB_SUPPORT [=y])

-> USB Gadget Support (USB_GADGET [=y])

-> USB Gadget Drivers (<choice> [=y])

Defined at drivers/usb/gadget/legacy/Kconfig:260

Depends on: <choice> && TTY [=y]

Selects: USB_U_SERIAL [=y] && USB_F_ACM [=y] && USB_F_SERIAL [=y] && USB_F_OBEX [=y] && USB_LIBCOMPOSITE [=y]
```

##### **USB device uvc**

```
Symbol: USB_G_WEBCAM [=y]

Type : tristate

Prompt: USB Webcam Gadget

Location:

-> Device Drivers

-> USB support (USB_SUPPORT [=y])

-> USB Gadget Support (USB_GADGET [=y])

-> USB Gadget Drivers (<choice> [=y])

Defined at drivers/usb/gadget/legacy/Kconfig:471

Depends on: <choice> && VIDEO_DEV [=y]

Selects: USB_LIBCOMPOSITE [=y] && VIDEOBUF2_VMALLOC [=y] && USB_F_UVC [=y] && USB_DWC2_HIGHWIDTH_FIFO [=y]
```

####  USB gadget描述符配置

hid、serial不支持复合设备，需要手动修改配置。

usb composite和相关功能描述符配置路径如下：

development/usb-gadget/usb-gadget-scripts/

```
 └── usb
    ├── acm
    ├── adb
    ├── keyboard
    ├── mass_storage
    ├── mouse
    ├── mtp
    ├── mtp_daemon
    ├── printer
    ├── rndis
    ├── serial
    ├── uac1
    ├── udc_daemon
    ├── usb_wakeup
    └── uvc
```

## 应用程序使用说明

### USB host

#### USB host Mass Storage

1. 插入Ｕ盘弹出打印信息

```
[ 46.070034] usb 1-1: new high-speed USB device number 2 using dwc2

[ 46.296250] usb 1-1: New USB device found, idVendor=0951, idProduct=1666

[ 46.303202] usb 1-1: New USB device strings: Mfr=1, Product=2, SerialNumber=3

[ 46.310592] usb 1-1: Product: DataTraveler 3.0

[ 46.315185] usb 1-1: Manufacturer: Kingston

[ 46.319508] usb 1-1: SerialNumber: 60A44C413C4EB211A98A00A0

[ 46.325821] usb-storage 1-1:1.0: USB Mass Storage device detected

[ 46.342363] scsi host0: usb-storage 1-1:1.0

[ 47.726378] scsi 0:0:0:0: Direct-Access Kingston DataTraveler 3.0 PMAP PQ: 0 ANSI: 6

[ 47.736215] sd 0:0:0:0: [sda] 30277632 512-byte logical blocks: (15.5 GB/14.4 GiB)

[ 47.752538] sd 0:0:0:0: [sda] Write Protect is off

[ 47.759990] sd 0:0:0:0: [sda] No Caching mode page found

[ 47.765648] sd 0:0:0:0: [sda] Assuming drive cache: write through

[ 47.780215] sda: sda1

[ 47.789136] sd 0:0:0:0: [sda] Attached SCSI removable disk
```

2. 挂载U盘到文件系统

```bash
# mount -t vfat /dev/sda1 mnt/
```

3. 进入/mnt下,进行数据交互

#### USB host camera

1. 插入摄像头识别成功

```
[ 56.560037] usb 1-1: new high-speed USB device number 2 using dwc2

[ 56.910300] usb 1-1: New USB device found, idVendor=058f, idProduct=5608

[ 56.917231] usb 1-1: New USB device strings: Mfr=3, Product=1, SerialNumber=0

[ 56.924635] usb 1-1: Product: USB 2.0 Camera

[ 56.929114] usb 1-1: Manufacturer: Alcor Micro, Corp.

[ 56.939481] uvcvideo: Found UVC 1.00 device USB 2.0 Camera (058f:5608)

[ 56.949251] input: USB 2.0 Camera as /devices/platform/ahb2/13500000.otg/usb1/1-1/1-1:1.0/input/input0
```

2. 拍照测试

```
usage: uvcview [-d <device>] [-c <count>]

--help -H print this message

--print_formats -F print video device info

--device -d , video device, default is /dev/video0

--width -w grab width

--height -h grab height

--count -c set the count to grab

--rate -r frame sample rate(fps)

--yuv -y use yuyv input format

--timeout -t select timeout

--match -m do picture match test

--perf -p do performance test
```

注：*/dev/vidio5是usb camera生成的标准video节点．*

```bash
# ./grab -w 640 -h 480 -d /dev/video5 -y -c 3
```

3. 生成图像

```
p-0.jpg　p-１.jpg　p-２.jpg
```

#### USB host hid mouse

1. 插入鼠标设备到控制器

```
[ 23.240032] usb 1-1: new low-speed USB device number 2 using dwc2

 [ 23.453954] usb 1-1: New USB device found, idVendor=046d, idProduct=c077

 [ 23.460890] usb 1-1: New USB device strings: Mfr=1, Product=2, SerialNumber=0

 [ 23.468252] usb 1-1: Product: USB Optical Mouse

 [ 23.473168] usb 1-1: Manufacturer: Logitech

 [ 23.483619] input: Logitech USB Optical Mouse as /devices/platform/ahb2/13500000.otg/usb1/1-1/1-1:1.0/0003:046D:C077.0001/input/input2

 [ 23.499237] hid-generic 0003:046D:C077.0001: input,hidraw0: USB HID v1.11 Mouse [Logitech USB Optical Mouse] on usb-13500000.otg-1/input0
```

1. 执行捕获事件的程序后移动鼠标触发事件

```bash
# cd /testsuite/usb_test/usb_host/getevent_test/

# ./getevent_test 0

/dev/input/mouse0

evdev version: 0.0.0

name:

features: relative reserved unknown unknown unknown unknown unknown unknown unknown

/dev/input/mouse0: open, fd = 3

Sun Mar 1 16:38:18 2020.000000 type 0x0011; code 0x000e; value 0x00000000; Led

[ 125.626285] random: nonblocking pool is initialized

Wed Dec 25 06:26:16 2019.000000 type 0x0011; code 0x000e; value 0x00000000; Led

Thu Dec 26 00:59:52 2019.000000 type 0x0011; code 0x000e; value 0x00000000; Led

Mon Dec 23 18:27:20 2019.000000 type 0x0011; code 0x000e; value 0x00000000; Led

Thu Dec 26 00:55:36 2019.000000 type 0x0011; code 0x000e; value 0x00000000; Led

Wed Dec 25 06:47:36 2019.000000 type 0x0011; code 0x000e; value 0x00000000; Led

Tue Dec 24 12:31:04 2019.000000 type 0x0011; code 0x000e; value 0x00000000; Led

Wed Dec 25 06:47:36 2019.000000 type 0x0011; code 0x000e; value 0x00000000; Led

Mon Dec 23 18:10:16 2019.000000 type 0x0011; code 0x000e; value 0x00000000; Led

....

....

....
```

#### USB host rndis

1. 插入USB网卡设备到控制器

```
 [ 1843.410034] usb 2-1: new high-speed USB device number 5 using dwc2

 [ 1843.620356] usb 2-1: New USB device found, idVendor=1a40, idProduct=0101

 [ 1843.627288] usb 2-1: New USB device strings: Mfr=0, Product=1, SerialNumber=0

 [ 1843.634689] usb 2-1: Product: USB2.0 HUB

 [ 1843.639454] hub 2-1:1.0: USB hub found

 [ 1843.646003] hub 2-1:1.0: 4 ports detected

 [ 1844.040036] usb 2-1.3: new high-speed USB device number 6 using dwc2

 [ 1844.141426] usb 2-1.3: New USB device found, idVendor=0fe6, idProduct=9900

 [ 1844.148538] usb 2-1.3: New USB device strings: Mfr=1, Product=2, SerialNumber=3

 [ 1844.156122] usb 2-1.3: Product: USB 10/100 LAN

 [ 1844.160740] usb 2-1.3: Manufacturer: CoreChips

 [ 1844.165333] usb 2-1.3: SerialNumber: 00E099059B46

 [ 1844.172548] cdc_ether 2-1.3:2.0 eth0: register 'cdc_ether' at usb-13540000.usb-1.3, CDC Ethernet Device, 00:e0:99:05:9b:46
```

2. 开发板端查看USB网卡信息

```bash
# ifconfig -a

....

....

 eth0 Link encap:Ethernet HWaddr 00:E0:99:05:9B:46

 BROADCAST MULTICAST MTU:1500 Metric:1

 RX packets:0 errors:0 dropped:0 overruns:0 frame:0

 TX packets:0 errors:0 dropped:0 overruns:0 carrier:0

 collisions:0 txqueuelen:1000

 RX bytes:0 (0.0 B) TX bytes:0 (0.0 B)....

....
```

3. 开发板端配置USB网卡ip

```bash
# ifconfig usb0 192.168.4.251 up

 eth0 Link encap:Ethernet HWaddr 00:0E:C6:FA:54:FA

 inet addr:192.168.4.251 Bcast:192.168.4.255 Mask:255.255.255.0

 inet6 addr: fe80::20e:c6ff:fefa:54fa/64 Scope:Link

 UP BROADCAST RUNNING MULTICAST MTU:1500 Metric:1

 RX packets:30 errors:0 dropped:0 overruns:0 frame:0

 TX packets:34 errors:0 dropped:0 overruns:0 carrier:0

 collisions:0 txqueuelen:1000

 RX bytes:2926 (2.8 KiB) TX bytes:3356 (3.2 KiB)
```

4. pc端配置USB网卡ip

```bash
# sudo ifconfig eno1 192.168.4.252 up

 eno1 Link encap:以太网 硬件地址 9c:8e:99:de:df:1e

 inet 地址:192.168.4.252 广播:192.168.4.255 掩码:255.255.255.0

 UP BROADCAST RUNNING MULTICAST MTU:1500 跃点数:1

 接收数据包:1179470 错误:7 丢弃:0 过载:0 帧数:5

 发送数据包:276650 错误:0 丢弃:0 过载:0 载波:0

 碰撞:0 发送队列长度:1000

 接收字节:851127307 (851.1 MB) 发送字节:38706156 (38.7 MB)

 中断:20 Memory:fe500000-fe520000
```

5. 开发板端测试网卡

```bash
# ping 192.168.4.252

PING 192.168.4.252 (192.168.4.252): 56 data bytes

 64 bytes from 192.168.4.252: seq=0 ttl=64 time=0.538 ms

 64 bytes from 192.168.4.252: seq=1 ttl=64 time=0.520 ms

 64 bytes from 192.168.4.252: seq=2 ttl=64 time=0.571 ms

 64 bytes from 192.168.4.252: seq=3 ttl=64 time=0.536 ms

....

....
```

6. pc端测试网卡

```bash
# ping 192.168.4.251

PING 192.168.4.251 (192.168.4.251) 56(84) bytes of data.

 64 bytes from 192.168.4.251: icmp_seq=1 ttl=64 time=0.728 ms

 64 bytes from 192.168.4.251: icmp_seq=2 ttl=64 time=0.419 ms

 64 bytes from 192.168.4.251: icmp_seq=3 ttl=64 time=0.421 ms

 64 bytes from 192.168.4.251: icmp_seq=4 ttl=64 time=0.454 ms

 64 bytes from 192.168.4.251: icmp_seq=5 ttl=64 time=0.426 ms

 64 bytes from 192.168.4.251: icmp_seq=6 ttl=64 time=0.450 ms

 64 bytes from 192.168.4.251: icmp_seq=7 ttl=64 time=0.449 ms

....

....
```

#### USB host cdrom

1. 插入usb光驱设备到控制器

```
# [61192.659785] usb 2-1: new high-speed USB device number 3 using dwc2

[61192.864785] usb 2-1: New USB device found, idVendor=0e8d, idProduct=1887

[61192.871745] usb 2-1: New USB device strings: Mfr=1, Product=2, SerialNumber=3

[61192.879121] usb 2-1: Product: Portable Super Multi Drive

[61192.884866] usb 2-1: Manufacturer: Hitachi-LG Data Storage Inc

[61192.890926] usb 2-1: SerialNumber: K0SMAHC3451

[61192.901854] usb-storage 2-1:1.0: USB Mass Storage device detected

[61192.922011] scsi host1: usb-storage 2-1:1.0

[61193.927502] scsi 1:0:0:0: CD-ROM think plusUltraslimDVD 1.01 PQ: 0 ANSI: 0

[61193.959517] sr 1:0:0:0: [sr0] scsi3-mmc drive: 24x/24x writer dvd-ram cd/rw xa/form2 cdda tray

[61193.970802] sr 1:0:0:0: Attached scsi generic sg0 type 5
```

2. 挂载光驱到文件系统

```bash
# mkdir /mnt/cdrom

# mount -t iso9660 -o ro /dev/sr0 /mnt/cdrom
```

3. 进入/mnt/cdrom下,读取数据。

### USB device

启用USB device功能

打开usb启动脚本

```bash
# vi /etc/init.d/S90usb
```

![](assets/USB_OTG控制器接口1.png)

按需取消功能注释

#### USB device mass storage

1. 制作盘符

```bash
# dd if=/dev/zero of=fat32.img bs=1k count=2048

# mkfs.vfat fat32.img
```

2. 将盘符加入

```bash
# echo fat32.img > /sys/kernel/config/usb_gadget/demo/functions/mass_storage.0/lun.0/file
```

3. 完成后会在PC端弹出盘符并支持热插拔

#### USB device adb

启动后无需任何配置直接开始adb功能

#### USB device uvc

1. pc端识别设备

```bash
$ dmesg

 [2187560.274157] usb 2-1.5: new high-speed USB device number 19 using ehci-pci

 [2187560.371029] usb 2-1.5: New USB device found, idVendor=18d1, idProduct=d002

 [2187560.371033] usb 2-1.5: New USB device strings: Mfr=1, Product=2, SerialNumber=3

 [2187560.371035] usb 2-1.5: Product: composite-demo

 [2187560.371037] usb 2-1.5: Manufacturer: ingenic

 [2187560.371039] usb 2-1.5: SerialNumber: 0123456789ABCDEF

 [2187560.394857] uvcvideo: Found UVC 1.00 device composite-demo (18d1:d002)

 [2187560.403005] input: composite-demo as /devices/pci0000:00/0000:00:1d.0/usb2/2-1/2-1.5/2-1.5:1.0/input/input141
```

2. 开发板执行测试程序

webcam_gadget使用方法：

```
 Usage: webcam_gadget [options]

 Available options are

 -b Use bulk mode

 -d Do not use any real V4L2 capture device

 -h Print this help screen and exit

 -i images dir for [uvc-WxH.jpg uvc-WxH.yuv]

 -m Streaming mult for ISOC (b/w 0 and 2)

 -n Number of Video buffers (b/w 2 and 32)

 -o <IO method> Select UVC IO method:

 0 = MMAP

 1 = USER_PTR

 -s <speed> Select USB bus speed (b/w 0 and 2)

 0 = Full Speed (FS)

 1 = High Speed (HS)

 2 = Super Speed (SS)

 -t Streaming burst (b/w 0 and 15)

 -u device UVC Video Output device

 -v device V4L2 Video Capture device

 -e device HELIX Video Capture device
```

camera动态测试：

example:

```bash
 # ./webcam_gadget -u /dev/video12 -v /dev/video4 -e /dev/video0
```

pc端测试工具：


**linux os:**

 Video tools: xawtv

 Video tools: cheese webcam booth

**windows os:**

 Video tools: amcap

**手机端:**

APP: usb 摄像头

**注**: 华为手机请使用legacy配置

#### USB device hid

1. pc端识别设备

```bash
$ dmesg

[2180880.531376] usb 2-1.5: new high-speed USB device number 115 using ehci-pci

[2180880.628285] usb 2-1.5: New USB device found, idVendor=18d1, idProduct=d002

[2180880.628292] usb 2-1.5: New USB device strings: Mfr=1, Product=2, SerialNumber=3

[2180880.628304] usb 2-1.5: Product: composite-demo

[2180880.628307] usb 2-1.5: Manufacturer: ingenic

[2180880.628309] usb 2-1.5: SerialNumber: 0123456789ABCDEF

[2180880.635882] input: ingenic composite-demo as /devices/pci0000:00/0000:00:1d.0/usb2/2-1/2-1.5/2-1.5:1.0/0003:18D1:D002.006C/input/input126

[2180880.691853] hid-generic 0003:18D1:D002.006C: input,hidraw3: USB HID v1.01 Keyboard [ingenic composite-demo] on usb-0000:00:1d.0-1.5/input0
```

2. 开发板执行测试程序

```bash
# cd /testsuite/usb_test/usb_gadget/hid_gadget_test
```

模拟usb键盘

```bash
# ./hid_gadget_test /dev/hidg0 keyboard
```

模拟usb鼠标

```bash
# ./hid_gadget_test /dev/hidg1 mouse
```

#### USB device ncm

1. pc端识别设备

* pc端识设备信息：

```bash
$ dmesg

 识别信息：

[3441418.912760] usb 1-9: new high-speed USB device number 86 using xhci_hcd

[3441419.069204] usb 1-9: New USB device found, idVendor=18d1, idProduct=d002, bcdDevice= 1.00

[3441419.069209] usb 1-9: New USB device strings: Mfr=1, Product=2, SerialNumber=3

[3441419.069213] usb 1-9: Product: composite-demo

[3441419.069216] usb 1-9: Manufacturer: ingenic

[3441419.069219] usb 1-9: SerialNumber: 0123456789ABCDEF

[3441419.093626] cdc_ncm 1-9:1.0: MAC-Address: 3a:8d:bb:87:e8:f4

[3441419.094246] cdc_ncm 1-9:1.0 usb0: register 'cdc_ncm' at usb-0000:00:14.0-9, CDC NCM, 3a:8d:bb:87:e8:f4

[3441419.164108] cdc_ncm 1-9:1.0 enx3a8dbb87e8f4: renamed from usb0

[3441419.247977] cdc_ncm 1-9:1.0 enx3a8dbb87e8f4: 425 mbit/s downlink 425 mbit/s uplink
```

* pc端识别到usb网卡设备：

```bash
$ ifconfig -a

enx3a8dbb87e8f4: flags=4099<UP,BROADCAST,MULTICAST>  mtu 1500
        ether 3a:8d:bb:87:e8:f4  txqueuelen 1000  (以太网)
        RX packets 0  bytes 0 (0.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 0  bytes 0 (0.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0
```

2. 主机端配置usb网卡设备ip：

```bash
$ sudo ifconfig enx3a8dbb87e8f4 192.168.4.99 up
```

3. 开发板端配置网卡设备ip:

```bash
$ sudo ifconfig usb0 192.168.4.100 up
```

4. 主机端ping开发板

```bash
$ ping 192.168.4.100

 PING 192.168.4.100 (192.168.4.100) 56(84) bytes of data.

 64 bytes from 192.168.4.100: icmp_seq=1 ttl=64 time=0.034 ms

 64 bytes from 192.168.4.100: icmp_seq=2 ttl=64 time=0.034 ms

 64 bytes from 192.168.4.100: icmp_seq=3 ttl=64 time=0.031 ms
```

5. 开发板ping主机

```bash
$ ping 192.168.4.99

 PING 192.168.4.99 (192.168.4.99) 56(84) bytes of data.

 64 bytes from 192.168.4.99: icmp_seq=1 ttl=64 time=0.034 ms

 64 bytes from 192.168.4.99: icmp_seq=2 ttl=64 time=0.034 ms

 64 bytes from 192.168.4.99: icmp_seq=3 ttl=64 time=0.031 ms
```

#### USB device remote NDIS

1. pc端识别设备

* pc端识设备信息：

```bash
$ dmesg

 识别信息：

 [2188325.480982] usb 2-1.5: new high-speed USB device number 21 using ehci-pci

 [2188325.577912] usb 2-1.5: New USB device found, idVendor=18d1, idProduct=d002

 [2188325.577919] usb 2-1.5: New USB device strings: Mfr=1, Product=2, SerialNumber=3

 [2188325.577930] usb 2-1.5: Product: composite-demo

 [2188325.577932] usb 2-1.5: Manufacturer: ingenic

 [2188325.577934] usb 2-1.5: SerialNumber: 0123456789ABCDEF

 [2188325.586180] rndis_host 2-1.5:1.0 usb0: register 'rndis_host' at usb-0000:00:1d.0-1.5, RNDIS device, 52:ea:19:19:5f:69

 [2188325.650205] rndis_host 2-1.5:1.0 enp0s29u1u5: renamed from usb0

 [2188325.670351] IPv6: ADDRCONF(NETDEV_UP): enp0s29u1u5: link is not ready
```

* pc端识别到usb网卡设备：

```bash
$ ifconfig -a

 enp0s29u1u5 Link encap:Ethernet HWaddr 52:ea:19:19:5f:69

 UP BROADCAST RUNNING MULTICAST MTU:1500 Metric:1

 RX packets:0 errors:0 dropped:0 overruns:0 frame:0

 TX packets:0 errors:0 dropped:0 overruns:0 carrier:0

 collisions:0 txqueuelen:1000

 RX bytes:0 (0.0 B) TX bytes:0 (0.0 B)
```

2. 主机配置usb网卡设备ip：

```bash
$ sudo ifconfig enp0s29u1u5 192.168.4.99 up
```

3. 开发板端配置网卡设备ip:

```bash
$ sudo ifconfig usb0 192.168.4.100 up
```

4. 主机端ping开发板

```bash
$ ping 192.168.4.100

 PING 192.168.4.100 (192.168.4.100) 56(84) bytes of data.

 64 bytes from 192.168.4.100: icmp_seq=1 ttl=64 time=0.034 ms

 64 bytes from 192.168.4.100: icmp_seq=2 ttl=64 time=0.034 ms

 64 bytes from 192.168.4.100: icmp_seq=3 ttl=64 time=0.031 ms
```

5. 开发板ping主机

```bash
$ ping 192.168.4.99

 PING 192.168.4.99 (192.168.4.99) 56(84) bytes of data.

 64 bytes from 192.168.4.99: icmp_seq=1 ttl=64 time=0.034 ms

 64 bytes from 192.168.4.99: icmp_seq=2 ttl=64 time=0.034 ms

 64 bytes from 192.168.4.99: icmp_seq=3 ttl=64 time=0.031 ms
```

#### USB device serial

1. pc端识别设备

```bash
$ dmesg

 设备信息：

[263657.347638] usb 2-1.7: new high-speed USB device number 56 using ehci-pci

[263657.460495] usb 2-1.7: New USB device found, idVendor=18d1, idProduct=d002

[263657.460498] usb 2-1.7: New USB device strings: Mfr=1, Product=2, SerialNumber=3

[263657.460499] usb 2-1.7: Product: composite-demo

[263657.460500] usb 2-1.7: Manufacturer: ingenic

[263657.460501] usb 2-1.7: SerialNumber: img,x2660-halley-board

[263898.619118] usbserial_generic 2-1.7:1.0: The "generic" usb-serial driver is only for testing and one-off prototypes.

[263898.619121] usbserial_generic 2-1.7:1.0: Tell linux-usb@vger.kernel.org to add your device to a proper driver.

[263898.619124] usbserial_generic 2-1.7:1.0: generic converter detected

[263898.620255] usb 2-1.7: generic converter now attached to ttyUSB0
```

2. pc端配置usb serial的VID和PID

```bash
$ echo 0x18d1 0xd002 > /sys/bus/usb-serial/module_drivers/drivers/generic/new_id
```

3. 开发板与pc端交互测试

 pc端测试：

 注：`ttyUSB*`对应usb serial设备

```bash
$ echo 1111111111 > /dev/ttyUSB1

$ cat /dev/ttyUSB1
```

 开发板测试：

```bash
# cat /dev/ttyGS0

# echo 222 > /dev/ttyGS0
```

#### USB device printer

1. pc端识别设备

```bash
$ dmesg
```
设备信息：
```
 usblp 2-1.7:1.4: usblp0: USB Bidirectional printer dev 38 if 4 alt 0 proto 2 vid 0x18D1 pid 0xD002
```

2. 开发板与pc端交互测试

 开发板测试：

```bash
# cd /testsuite/usb_test/usb_gadget/prn_example/

# ./prn_example -read_data

# cat data_file | ./prn_example -write_data
```

 pc端测试：

```bash
$ echo 111 > /dev/usb/lp0

$ cat /dev/usb/lp0
```

#### USB device uac1.0

1. pc端识别设备

```bash
$ aplay -l

 声卡信息：

 card 1: compositedemo [composite-demo], device 0: USB Audio [USB Audio]

 Subdevices: 1/1

 Subdevice #0: subdevice #0
```

2. 开发板执行测试程序，pc端选择usb 声卡设备，播放声音

 注：*配置speaker音频通道，其中声卡1为usb声卡设备*

 开发板端执行:

```bash
# arecord -f dat -t wav -D hw:1,0 | aplay -D hw:0,0 &
```

 pc端执行:

```bash
$ aplay -D hw:1,0 test.wav
```

3. pc端选择usb 声卡设备，开发板录音，pc端播放声音

 注：*配置speaker音频通道，其中声卡1为usb声卡设备*

 开发板端执行:

```bash
# arecord -f dat -t wav -D hw:0,0 | aplay -D hw:1,0 &
```

 pc端执行:
```bash
$ arecord -f dat -t wav -D hw:1,0 | aplay -D hw:0,0
```

#### USB device serial

1. pc端识别设备

```bash
$ dmesg
```

 设备信息：

```
 [2189021.818872] usb 2-1.5: new high-speed USB device number 25 using ehci-pci

 [2189021.915645] usb 2-1.5: New USB device found, idVendor=0525, idProduct=a4a7

 [2189021.915651] usb 2-1.5: New USB device strings: Mfr=1, Product=2, SerialNumber=0

 [2189021.915655] usb 2-1.5: Product: Gadget Serial v2.4

 [2189021.915659] usb 2-1.5: Manufacturer: Linux 4.4.94+ with 13500000.otg

 [2189021.922892] cdc_acm 2-1.5:2.0: ttyACM0: USB ACM device
```

2. 互发信息

 pc端：

```bash
$ echo 1111111111 > /dev/ttyACM0

$ cat /dev/ttyACM0
```

 开发板端：

```bash
# cat ttyGS0

# echo 2222 > ttyGS0
```

### USB眼图调整

X26xx USB IP遵循USB2.0规范，支持多种测试模式，包括Test_J模式、Test_K模式、Test_SE0_NAK 模式、Test_Packet模式和Test_Force_Enable模式。

作为device进行眼图测试，需要利用USB官方测试软件工具 HSETT，使其进入test_Packet测试模式。

作为host进行眼图测试，需要配置otg控制器寄存器HPRT[16,13]位，进入test_Packet测试模式。

x26xx usb Phy的可用下表调整眼图以获取最佳信号质量，usb phy port0寄存器映射到总线基地址为0x10078000,usb phy port1寄存器映射到总线基地址为0x10078400。

|  |  |  |  |
| --- | --- | --- | --- |
| Addr | Bit region | Default | Description |
| 0x18 | [7:3] | 5'b10110 | 45ohm HS ODT value tuning & FS/LS driver strength tuning5’b11111: smallest HS ODT value & largest FS/LS drive strength and fastest FS/LS slew rate.5’b10000: biggest HS ODT value & smallest FS/LS drive strength and slowest FS/LS slew rate. |
| 0x30 | [3] | 1'b1 | Tx HS pre_emphasize strength configure,3’b111 represents the strongest3’b000 the weakest |
|  | [6:4] | 5’b10101 | HS eye height tuning3’b000 : 400mv3’b001 : 362mv3’b010 : 350mv3’b011 : 387.5mv3’b100 : 412.5mv3’b101 : 425mv3’b110 : 437.5mv3’b111 : 450mv (default) |

## 注意事项

### 修改usb gadget配置

修改默认支持的gadget设备,配置文件路径如下:

***development/usb-gadget/usb-gadget-scripts/S90usb***

当使用某个function时，需要将其它功能用’#’注释上:

注: hid和serial只能单独使用,不支持复合设备

```
#/etc/init.d/usb/printer $1

#/etc/init.d/usb/uvc    $1

/etc/init.d/usb/adb     $1

#/etc/init.d/usb/acm    $1

#/etc/init.d/usb/mtp    $1

#/etc/init.d/usb/mass_storage $1

#/etc/init.d/usb/keyboard       $1

#/etc/init.d/usb/mouse  $1

#/etc/init.d/usb/rndis  $1

#/etc/init.d/usb/uac1   $1

#/etc/init.d/usb/serial $1
```

