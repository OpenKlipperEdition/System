# GMAC千兆以太网控制器接口

## 模块功能介绍

RGMII（Reduced Gigabit Media Independent Interface）是Reduced GMII（吉比特介质独立接口）。RGMII均采用4位数据接口，工作时钟125MHz，并且在上升沿和下降沿同时传输数据，因此传输速率可达1000Mbps。RGMII数据结构符合IEEE以太网标准，接口定义见IEEE 802.3-2000, RGMII支持10/100/1000兆的总线接口速度。

RMII Reduced Media Independent Interface 简化媒体独立接口,是IEEE 802.3u标准中除MII接口之外的另一种实现。RMII支持10兆和100兆的总线接口速度。

## 驱动源码位置

内核驱动代码位置:

***module_drivers/drivers/net/ethernet/ingenic***
```c
├── ethtool.c
├── ingenic_mac.c
├── ingenic_mac.h
├── Kconfig
├── Makefile
├── readme
├── synopGMAC_Dev.c
├── synopGMAC_Dev.h
├── synopGMAC_plat.c
└── synopGMAC_plat.h
```

## 设备树配置

设备树所在位置：

kernel内核(version <= 5.10)dts文件路径：

***module_driver/dts/x2000.dtsi***

kernel内核(version > 5.10)dts文件路径：

***module_driver/dts/x2000/x2000.dtsi***

网卡接口可以配置为RGMII 和 RMII，在SDK中默认配置为RGMII接口，用户可以根据自己实际的情况配置为RMII接口

双网卡控制器定义：
```c
mac0: mac@0x134b0000 {

 compatible = "ingenic,x2000-mac";

 reg = <0x134b0000 0x2000>;

 interrupt-parent = <&core_intc>;

 interrupts = <IRQ_GMAC0>;

 status = "disabled";

 ingenic,rst-ms = <10>;

};

mac1: mac@0x134a0000 {

 compatible = "ingenic,x2000-mac";

 reg = <0x134a0000 0x2000>;

 interrupt-parent = <&core_intc>;

 interrupts = <IRQ_GMAC1>;

 status = "disabled";

 ingenic,rst-ms = <10>;

};
```

### 设备树默认配置

Halley5_V1.x 系列开发板支持双网卡

Halley5_V2.x 系列开发板支持单网卡 MAC1。

Halley5_V3.x 系列开发板支持单网卡 MAC1。

设备树属性说明如下:



|  |  |
| --- | --- |
| **属性名称** | **说明** |
| **ingenic,rst-gpio** | 定义复位GPIO |
| **ingenic,rst-ms** | 定义复位时间 |
| **ingenic,rst-delay-ms** | 复位后延时时间，延时期间不对phy进行任何操作 |
| **ingenic,mac-mode** | RGMII 或者 RMII |
| **ingenic,mode-reg** |  |
| **ingenic,rx-clk-delay** | 仅RGMII模式根据实际情况调整时钟采样偏移 |
| **ingenic,tx-clk-delay** | 仅RGMII模式 |
| **ingenic,phy-clk-freq** | 如果使用芯片供给phy的工作时钟, 需要配置phy芯片工作时钟频率 |

注意：

1.依据开发板设计更改mac设备对应phy的复位gpio,复位有效电平,复位需要的保持的时间

2.如果工作在RGMII模式下需要配置TXCLK和RXCLK的delay,配置的精度是19.5ps,配置值范围0-128,delay的时间范围0-2.5ns.具体配置值需要由phy来确定,TXCLK和RXCLK和data保证2ns左右的delay,所以如果phy端已经做了delay,mac控制器端就不需要设置或补足差值

双网卡板级RGMII配置如下：
```c
&mac0 { 

 pinctrl-names = "default", "reset";

 pinctrl-0 = <&mac0_rgmii_p0_normal>, <&mac0_rgmii_p1_normal>;

 pinctrl-1 = <&mac0_rgmii_p0_rst>, <&mac0_rgmii_p1_normal>;

 status = "okay";

 ingenic,rst-gpio = <&gpb 2 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;

 ingenic,rst-ms = <10>;

 ingenic,rst-delay-ms = <15>;

 ingenic,mac-mode = <RGMII>;

 ingenic,mode-reg = <0xb00000e4>;

 ingenic,rx-clk-delay = <0x2>;

 ingenic,tx-clk-delay = <0x3f>;

 /*force mac mode*/

 ingenic,mac-force = <MAC_OFF>;

 ingenic,mac-autoneg = <MAC_OFF>;

 ingenic,mac-speed = <MAC_SPEED_100M>;

 ingenic,mac-duplex = <MAC_DUPLEX_HALF>;

};

&mac1 {

 pinctrl-names = "default", "reset";

 pinctrl-0 = <&mac1_rgmii_p0_normal>, <&mac1_rgmii_p1_normal>;

 pinctrl-1 = <&mac1_rgmii_p0_rst>, <&mac1_rgmii_p1_normal>;

 status = "okay";

 ingenic,rst-gpio = <&gpb 26 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;

 ingenic,rst-ms = <10>;

 ingenic,rst-delay-ms = <15>;

 ingenic,mac-mode = <RGMII>;

 ingenic,mode-reg = <0xb00000e8>;

 ingenic,rx-clk-delay = <0x0>;

 ingenic,tx-clk-delay = <0x3f>;

 /*force mac mode*/

 ingenic,mac-force = <MAC_OFF>;

 ingenic,mac-autoneg = <MAC_OFF>;

 ingenic,mac-speed = <MAC_SPEED_100M>;

 ingenic,mac-duplex = <MAC_DUPLEX_HALF>;

};
```

### 设备树自定义配置

RMII 配置参考如下:
```c
&mac0 {

        pinctrl-names = "default", "reset";

        pinctrl-0 = <&mac0_rmii_p0_normal>, <&mac0_rmii_p1_normal>，<&mac0_phy_clk>;

        pinctrl-1 = <&mac0_rmii_p0_rst>, <&mac0_rmii_p1_normal>;

        status = "okay";

        ingenic,rst-gpio = <&gpb 0 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;

        ingenic,rst-ms = <10>;

        ingenic,rst-delay-ms = <15>;

        ingenic,mac-mode = <RMII>;

        ingenic,mode-reg = <0xb00000e4>;

        ingenic,phy-clk-freq = <50000000>;

};

&mac1 {

        pinctrl-names = "default", "reset";

        pinctrl-0 = <&mac1_rmii_p0_normal>, <&mac1_rmii_p1_normal>， <&mac1_phy_clk>;

        pinctrl-1 = <&mac1_rmii_p0_rst>, <&mac1_rmii_p1_normal>;

        status = "disable";

        ingenic,rst-gpio = <&gpb 26 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;

        ingenic,rst-ms = <10>;

        ingenic,rst-delay-ms = <15>;

        ingenic,mac-mode = <RMII>;

        ingenic,mode-reg = <0xb00000e8>;

        ingenic,phy-clk-freq = <50000000>;

};
```

## 内核编译配置

内核配置INGENIC_MAC，配置说明如下：
```
 Symbol: INGENIC_MAC [=y] 

 Type : tristate 

 Defined at module_drivers/drivers/net/ethernet/ingenic/Kconfig 

 Prompt: ingenic on-chip MAC support 
```

### 内核默认编译配置

内核默认配置GMAC驱动，配置界面如下：

![](assets/GMAC千兆以太网控制器接口.0.png)

### 内核自定义编译配置



|  |  |
| --- | --- |
| **可配置选项** |  **配置说明** |
| **Ingenic gmac receive descriptor number[80..10240]** | gmac 控制器接收数据包的缓存数量，缓存越大可以减少满载时的丢包率，经过测试设置8192可以避免控制器层出现丢包, 这种情况运行时一个网卡的动态内存会使用16M-32M |
| **Dual core mutex transmission** | 使用双网卡时配置， 配置后会把双网卡处接收数据协议栈的过程互斥处理，可以减少指令cache的miss率 |

## 模块内核差异

无

## 设备节点生成

驱动加载成功后，执行ifconfig -a会出现eth0或者eth1.

## 应用程序使用说明

执行ifconfig -a 命令查看支持的网络设备，如果出现eth0 或者eth1则说明网卡加载成功
```bash
# ifconfig -a

eth0      Link encap:Ethernet  HWaddr 1E:01:7F:E6:D3:26 

          BROADCAST MULTICAST  MTU:1500  Metric:1

          RX packets:0 errors:0 dropped:0 overruns:0 frame:0

          TX packets:0 errors:0 dropped:0 overruns:0 carrier:0

          collisions:0 txqueuelen:1000

          RX bytes:0 (0.0 B)  TX bytes:0 (0.0 B)

eth1      Link encap:Ethernet  HWaddr 3E:4C:9D:F4:74:4B

          BROADCAST MULTICAST  MTU:1500  Metric:1

          RX packets:0 errors:0 dropped:0 overruns:0 frame:0

          TX packets:0 errors:0 dropped:0 overruns:0 carrier:0

          collisions:0 txqueuelen:1000

          RX bytes:0 (0.0 B)  TX bytes:0 (0.0 B)

lo        Link encap:Local Loopback

          LOOPBACK  MTU:65536  Metric:1

          RX packets:0 errors:0 dropped:0 overruns:0 frame:0

          TX packets:0 errors:0 dropped:0 overruns:0 carrier:0

          collisions:0 txqueuelen:1

          RX bytes:0 (0.0 B)  TX bytes:0 (0.0 B)
```

配置网络IP
```bash
# ifconfig eth0 IP                /*根据实际选择的网口配置eth0或eth1*/
```

使用ping命令查看网络连接情况
```bash
# ping IP -I eth0                /*通过-I 选项可以指定ping 命令使用的网口*/
```

### 网络性能测试

找一台有千兆以太网功能的电脑，使用网线直连，使用iperf3命令测试，电脑当做服务端，开发板做客户端

* 电脑端执行命令
```bash
# iperf3  -s
```

* 测试单向发送性能
```bash
# iperf3  -c 192.168.4.105 -u -b 1000M -l 65507 -t 10
```

* 测试单向接收性能
```bash
# iperf3  -c 192.168.4.105 -u -b 1000M -l 65507 -t 10 -R
```

* 应用层提高网络性能的方法

 以下方法根据具体应用场景使用，设置不合理反而会影响测试结果
```bash
# sysctl -w net.core.rmem_default=10485760  
```

* 增加网络核心层接收的缓存, 此设置会使用10M内存， 双网口桥接不使用
```bash
# sysctl -w net.core.rmem_max=10485760 
```

* 增加网络核心层接收的缓存

指定内核处理接收网络协议栈使用的cpu核，指定后可以避免内核动态切换过程造成双核使用不均衡造成的性能不稳定：
```bash
# echo 0 > /proc/irq/61/smp_affinity_list   
```

* 指定cpu0处理gmac1接收的数据
```bash
# echo 1 > /proc/irq/63/smp_affinity_list   
```

* 指定cpu1处理gmac0接收的数据， gmac0中断号63  gmac1中断号61
```bash
# sysctl -w net.core.netdev_max_backlog=4000 
```

* 增加转接过程中数据包缓存长度，可以减少数据抖动但消耗内存越大
```bash
# echo 2 > /sys/class/net/eth0/queues/rx-0/rps_cpus 
```

* 指定cpu1处理eth0接收到数据包，输入1指定cpu0，2指定cpu1, 3指定cpu0-1

### 1588硬件时间戳测试

使用2块带有以太网接口的开发板，使用网线直连。

使用CONFIG_INGENIC_GMAC_USE_HWSTAMP宏配置kernel的1588功能。

文件系统中需要有测试1588功能的测试程序，默认使用linuxptp测试，linuxptp测试工程在manhatton工程的buildroot中，关于linuxptp使用方法可以参照网络资料。

网络正常工作后：

一个开发板做slave执行命令：
```bash
# ptp4l -E -4 -H -i eth0 -s -m
```

另一个开发板做master执行命令：
```bash
# ptp4l -E -4 -H -i eth0 -m
```
