# 合宙Air780E模组linux平台适配


## 参考链接

https://doc.openluat.com/wiki/37?wiki_page_id=4454

## 测试环境

    硬件: Air780E-H模组
         君正x2000_HALLEY5_V3.1开发板
         SIM卡
    软件: Air780E-H模组固件AirM2M_780E_V1149_LTE_LPAT.binpkg
         君正4.4.94/5.10内核


## 内核驱动配置

* usb host RNDIS驱动配置
```
Symbol: USB_USBNET [=y]
Type  : tristate
Defined at drivers/net/usb/Kconfig:133
  Prompt: Multi-purpose USB Networking Framework
  Depends on: NETDEVICES [=y] && USB_NET_DRIVERS [=y]
  Location:
     -> Device Drivers
       -> Network device support (NETDEVICES [=y])
(1)      -> USB Network Adapters (USB_NET_DRIVERS [=y])
Selects: MII [=y]
Selected by [y]:
    - USB_NET_RNDIS_WLAN [=y] && NETDEVICES [=y] && WLAN [=y] && USB [=y] && CFG80211 [=y]
```
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
Selected by [y]:
    - USB_NET_RNDIS_WLAN [=y] && NETDEVICES [=y] && WLAN [=y] && USB [=y] && CFG80211 [=y]
```

```
Symbol: USB_NET_RNDIS_WLAN [=y]
Type  : tristate
Defined at drivers/net/wireless/Kconfig:91
  Prompt: Wireless RNDIS USB support
  Depends on: NETDEVICES [=y] && WLAN [=y] && USB [=y] && CFG80211 [=y]
  Location:
    -> Device Drivers
      -> Network device support (NETDEVICES [=y])
        -> Wireless LAN (WLAN [=y])
Selects: USB_NET_DRIVERS [=y] && USB_USBNET [=y] && USB_NET_CDCETHER [=y] && \  
USB_NET_RNDIS_HOST [=y]
```

* usb host ACM驱动配置

```
Symbol: USB_ACM [=y]
Type  : tristate
Defined at drivers/usb/class/Kconfig:7
  Prompt: USB Modem (CDC ACM) support
  Depends on: USB_SUPPORT [=y] && USB [=y] && TTY [=y]
  Location:
    -> Device Drivers
      -> USB support (USB_SUPPORT [=y])
  Selected by [n]:
    - USB_VL600 [=n] && NETDEVICES [=y] && USB_NET_DRIVERS [=y] && USB_NET_CDCETHER [=y] && TTY [=y]
    - USB_PULSE8_CEC [=n] && MEDIA_CEC_SUPPORT [=n] && USB_SUPPORT [=y] && TTY [=y]
    - USB_RAINSHADOW_CEC [=n] && MEDIA_CEC_SUPPORT [=n] && USB_SUPPORT [=y] && TTY [=y]
```

## 使用方法

1. 将Air780E模块通过usb线连接到开发板

2. 长按Air780E模块POW按键开机,开发板作为usb主机识别到usb网卡设备

```
    # [ 8441.650016] usb 1-1: new high-speed USB device number 7 using dwc2
      [ 8441.901615] usb 1-1: New USB device found, idVendor=19d1, idProduct=0001, bcdDevice= 2.00
      [ 8441.910095] usb 1-1: New USB device strings: Mfr=1, Product=2, SerialNumber=3
      [ 8441.917460] usb 1-1: Product: EigenComm Compo
      [ 8441.921979] usb 1-1: Manufacturer: EigenComm
      [ 8441.926389] usb 1-1: SerialNumber: 000000000001
      [ 8441.935474] rndis_host 1-1:1.0 eth0: register 'rndis_host' at usb-13500000.otg-1, RNDIS device, 20:89:84:6a:96:ab
      [ 8441.963514] cdc_acm 1-1:1.2: ttyACM0: USB ACM device
      [ 8441.986680] cdc_acm 1-1:1.4: ttyACM1: USB ACM device
      [ 8442.011365] cdc_acm 1-1:1.6: ttyACM2: USB ACM device

```

3. 查看Air780E网卡信息
```
    > #ifconfig -a
        eth0    Link encap:Ethernet  HWaddr 20:89:84:6A:96:AB  
                BROADCAST MULTICAST  MTU:1500  Metric:1
                RX packets:0 errors:0 dropped:0 overruns:0 frame:0
                TX packets:0 errors:0 dropped:0 overruns:0 carrier:0
                collisions:0 txqueuelen:1000 
                RX bytes:0 (0.0 B)  TX bytes:0 (0.0 B)

        eth1     Link encap:Ethernet  HWaddr 12:5E:71:1E:48:65  
                BROADCAST MULTICAST  MTU:1500  Metric:1
                RX packets:0 errors:0 dropped:0 overruns:0 frame:0
                TX packets:0 errors:0 dropped:0 overruns:0 carrier:0
                collisions:0 txqueuelen:1000 
                RX bytes:0 (0.0 B)  TX bytes:0 (0.0 B)

        lo      Link encap:Local Loopback  
                inet addr:127.0.0.1  Mask:255.0.0.0
                UP LOOPBACK RUNNING  MTU:65536  Metric:1
                RX packets:0 errors:0 dropped:0 overruns:0 frame:0
                TX packets:0 errors:0 dropped:0 overruns:0 carrier:0
                collisions:0 txqueuelen:1000 
                RX bytes:0 (0.0 B)  TX bytes:0 (0.0 B)
```

4. 使用eth0接口获取DHCP地址
```
    > #udhcpc -i eth0
        udhcpc: started, v1.31.1
        udhcpc: sending discover
        udhcpc: sending select for 192.168.10.2
        udhcpc: lease of 192.168.10.2 obtained, lease time 86400
        deleting routers
        adding dns 192.168.10.3
        adding dns 192.168.10.4
```

```
    > #ifconfig -a
        eth0    Link encap:Ethernet  HWaddr 20:89:84:6A:96:AB  
                inet addr:192.168.10.2  Bcast:192.168.10.255  Mask:255.255.255.0
                UP BROADCAST RUNNING MULTICAST  MTU:1500  Metric:1
                RX packets:7 errors:0 dropped:5 overruns:0 frame:0
                TX packets:2 errors:0 dropped:0 overruns:0 carrier:0
                collisions:0 txqueuelen:1000 
                RX bytes:1296 (1.2 KiB)  TX bytes:772 (772.0 B)

        eth1    Link encap:Ethernet  HWaddr 12:5E:71:1E:48:65  
                BROADCAST MULTICAST  MTU:1500  Metric:1
                RX packets:0 errors:0 dropped:0 overruns:0 frame:0
                TX packets:0 errors:0 dropped:0 overruns:0 carrier:0
                collisions:0 txqueuelen:1000 
                RX bytes:0 (0.0 B)  TX bytes:0 (0.0 B)

        lo      Link encap:Local Loopback  
                inet addr:127.0.0.1  Mask:255.0.0.0
                UP LOOPBACK RUNNING  MTU:65536  Metric:1
                RX packets:0 errors:0 dropped:0 overruns:0 frame:0
                TX packets:0 errors:0 dropped:0 overruns:0 carrier:0
                collisions:0 txqueuelen:1000 
                RX bytes:0 (0.0 B)  TX bytes:0 (0.0 B)
```

5. 联网测试
```
    > #ping www.baidu.com
        PING www.baidu.com (220.181.38.149): 56 data bytes
        64 bytes from 220.181.38.149: seq=0 ttl=52 time=19.777 ms
        64 bytes from 220.181.38.149: seq=1 ttl=52 time=35.005 ms
        64 bytes from 220.181.38.149: seq=2 ttl=52 time=42.940 ms
        64 bytes from 220.181.38.149: seq=3 ttl=52 time=33.574 ms
        64 bytes from 220.181.38.149: seq=4 ttl=52 time=42.402 ms
```

6. 使用提供的sendat小工具发送AT指令(ttyACM2为Air780E模块AT指令发送节点)
```
    > #./sendat /dev/ttyACM2
        AT

        OK
        AT+CPIN?

        +CPIN: READY

        OK
        AT+CEREG?

        +CEREG: 0,1

        OK
        AT+CSQ

        +CSQ: 21,0

        OK
        ATI

        AirM2M_780E_V1149_LTE_LPAT

        OK
```
### sendat应用程序

**编译即可使用**
```
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>
#include <sys/types.h>
#include <poll.h>

int ttyfd;
int stdout_changed = 0;
int tty_changed = 0;
struct termios stdout_tio;
struct termios tty_tio;

void restore_tio()
{
        if (stdout_changed)
        {
                stdout_tio.c_lflag |= ECHO;
                tcsetattr(STDIN_FILENO, TCSANOW, &stdout_tio);
        }
        if (tty_changed)
                tcsetattr(ttyfd, TCSANOW, &tty_tio);
}

void int_handler(int signum)
{
        close(ttyfd);
        exit(EXIT_SUCCESS);
}

int main(int args, const char *argv[])
{
        if (args != 2)
        {
                fprintf(stderr, "Usage: ./sendat ttyACM\n");
                exit(EXIT_FAILURE);
        }

        atexit(restore_tio);

        char readbuf[256];
        const char *ttyPath = argv[1];
        const char *atCommand = argv[2];

        ttyfd = open(ttyPath, O_RDWR | O_NOCTTY | O_NDELAY);
        if (ttyfd < 0)
        {
                perror("open");
                exit(EXIT_FAILURE);
        }

        signal(SIGINT, int_handler);

        struct termios tio;
        int ret = tcgetattr(ttyfd, &tio);
        if (ret == -1)
        {
                perror("tcgetattr");
                exit(EXIT_FAILURE);
        }
        memcpy(&tty_tio, &tio, sizeof(struct termios));
        tio.c_iflag = 0;
        tio.c_oflag = 0;
        tio.c_cflag = CS8 | CREAD | CLOCAL;
        tio.c_cflag &= (~CRTSCTS);
        tio.c_lflag = 0;
        tio.c_cc[VMIN] = 1;
        tio.c_cc[VTIME] = 0;
        if (cfsetospeed(&tio, B115200) < 0 || cfsetispeed(&tio, B115200) < 0)
        {
                perror("cfseti/ospeed");
                exit(EXIT_FAILURE);
        }
        ret = tcsetattr(ttyfd, TCSANOW, &tio);
        if (ret == -1)
        {
                perror("tcsetattr");
                exit(EXIT_FAILURE);
        }
        tty_changed = 1;

        struct termios outio;
        ret = tcgetattr(STDIN_FILENO, &outio);
        if (ret == -1)
        {
                perror("tcgetattr");
                exit(EXIT_FAILURE);
        }
        memcpy(&stdout_tio, &outio, sizeof(struct termios));
        stdout_changed = 1;
        outio.c_lflag &= ~ECHO;
        ret = tcsetattr(STDIN_FILENO, TCSANOW, &outio);

        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

        struct pollfd fds[] = {
                {STDIN_FILENO, POLLIN, 0},
                {ttyfd, POLLIN, 0}
        };

        while (1)
        {
                ret = poll(fds, sizeof(fds)/sizeof(struct pollfd), 0);
                if (ret == -1)
                {
                        perror("poll");
                        exit(EXIT_FAILURE);
                }
                if (fds[0].revents & POLLIN)
                {
                        memset(readbuf, 0, sizeof(readbuf));
                        ret = read(fds[0].fd, readbuf, sizeof(readbuf));
                        if (ret < 0 && errno != EAGAIN)
                        {
                                perror("read");
                                exit(EXIT_FAILURE);
                        }
                        if (ret > 0)
                        {
                                strcpy(readbuf+ret-1, "\r\n");
                                // printf("%s", readbuf);
                                ret = write(ttyfd, readbuf, strlen(readbuf));
                                if (ret < 0)
                                {
                                        perror("write");
                                        exit(EXIT_FAILURE);
                                }
                        }
                }
                if (fds[1].revents & POLLIN)
                {
                        memset(readbuf, 0, sizeof(readbuf));
                        ret = read(fds[1].fd, readbuf, sizeof(readbuf));
                        if (ret == -1 && errno != EAGAIN)
                        {
                                perror("read");
                                exit(EXIT_FAILURE);
                        }
                        printf("%s", readbuf);
                }
        }

        int commandLen = strlen(atCommand);
        char *buf = (char *)malloc(commandLen + 3);
        sprintf(buf, "%s\r\n", atCommand);
        ret = write(ttyfd, buf, strlen(buf));
        if (ret < 0)
        {
                perror("write");
                exit(EXIT_FAILURE);
        }
        free(buf);

        while (1)
        {
                ret = read(ttyfd, readbuf, sizeof(readbuf));
                if (ret < 0 && errno != EAGAIN)
                {
                        perror("read");
                        exit(EXIT_FAILURE);
                }
                if (ret > 0)
                        write(1, readbuf, ret);
        }

        return 0;
}
```

## ppp拨号上网配置

ppp内核驱动配置

```
Symbol: PPP [=y]
Type  : tristate
Defined at drivers/net/ppp/Kconfig:6
  Prompt: PPP (point-to-point protocol) support
  Depends on: NETDEVICES [=y]
  Location:
    -> Device Drivers
      -> Network device support (NETDEVICES [=y])
   Selects: SLHC [=y]   
   Selected by [n]:
     - IPWIRELESS [=n] && PCMCIA [=n] && NETDEVICES [=y] && TTY [=y]

```

```
Symbol: PPP_DEFLATE [=y]  
Type  : tristate
Defined at drivers/net/ppp/Kconfig:57
  Prompt: PPP Deflate compression
  Depends on: NETDEVICES [=y] && PPP [=y]
  Location:
    -> Device Drivers
      -> Network device support (NETDEVICES [=y])
        -> PPP (point-to-point protocol) support (PPP [=y])
Selects: ZLIB_INFLATE [=y] && ZLIB_DEFLATE [=y]
```
```    
Symbol: PPP_FILTER [=y]
Type  : bool
Defined at drivers/net/ppp/Kconfig:72
  Prompt: PPP filtering
  Depends on: NETDEVICES [=y] && PPP [=y]
  Location:
    -> Device Drivers
      -> Network device support (NETDEVICES [=y])
        -> PPP (point-to-point protocol) support (PPP [=y])
```

```
Symbol: PPP_MPPE [=y]  
Type  : tristate 
Defined at drivers/net/ppp/Kconfig:85
  Prompt: PPP MPPE compression (encryption)
  Depends on: NETDEVICES [=y] && PPP [=y]
  Location:
    -> Device Drivers
       -> Network device support (NETDEVICES [=y])     
          -> PPP (point-to-point protocol) support (PPP [=y])
Selects: CRYPTO [=y] && CRYPTO_SHA1 [=y] && CRYPTO_LIB_ARC4 [=y]
```

```
Symbol: PPP_MULTILINK [=y]
Type  : bool
Defined at drivers/net/ppp/Kconfig:98
  Prompt: PPP multilink support
  Depends on: NETDEVICES [=y] && PPP [=y]
  Location:
    -> Device Drivers
      -> Network device support (NETDEVICES [=y])
         -> PPP (point-to-point protocol) support (PPP [=y])
```

```
Symbol: PPPOE [=y]                                                                           
Type  : tristate
Defined at drivers/net/ppp/Kconfig:120
  Prompt: PPP over Ethernet
  Depends on: NETDEVICES [=y] && PPP [=y]
  Location:
   -> Device Drivers
      -> Network device support (NETDEVICES [=y])
         -> PPP (point-to-point protocol) support (PPP [=y])         
```

```
Symbol: PPP_ASYNC [=y]
Type  : tristate
Defined at drivers/net/ppp/Kconfig:152
  Prompt: PPP support for async serial ports 
  Depends on: NETDEVICES [=y] && TTY [=y] && PPP [=y]
  Location:
   -> Device Drivers
      -> Network device support (NETDEVICES [=y])
         -> PPP (point-to-point protocol) support (PPP [=y]) 
Selects: CRC_CCITT [=y]
```

```
Symbol: PPP_SYNC_TTY [=y]
Type  : tristate
Defined at drivers/net/ppp/Kconfig:166
  Prompt: PPP support for sync tty ports
  Depends on: NETDEVICES [=y] && TTY [=y] && PPP [=y]
  Location:
   -> Device Drivers
      -> Network device support (NETDEVICES [=y]) 
         -> PPP (point-to-point protocol) support (PPP [=y])
```

文件系统pppd配置

```
Symbol: BR2_PACKAGE_PPPD [=y]
Type  : bool
Prompt: pppd
  Location:
    -> Target packages
      -> Networking applications
  Defined at package/pppd/Config.in:1  
  Depends on: !BR2_STATIC_LIBS [=n] && !BR2_TOOLCHAIN_USES_MUSL [=n] && BR2_USE_MMU [=y]
  Selects: BR2_PACKAGE_OPENSSL [=y]
  Selected by [n]:
  - BR2_PACKAGE_RP_PPPOE [=n] && !BR2_STATIC_LIBS [=n] && !BR2_TOOLCHAIN_USES_MUSL [=n]\
  && BR2_USE_MMU [=y]
  - BR2_PACKAGE_NETWORK_MANAGER_PPPD [=n] && BR2_PACKAGE_NETWORK_MANAGER [=n] && !\
  BR2_TOOLCHAIN_USES_MUSL [=n]
```

```
Symbol: BR2_PACKAGE_PPPD_FILTER [=y]
Type  : bool
Prompt: filtering
  Location:
    -> Target packages
       -> Networking applications
         -> pppd (BR2_PACKAGE_PPPD [=y]) 
  Defined at package/pppd/Config.in:14
  Depends on: BR2_PACKAGE_PPPD [=y]
  Selects: BR2_PACKAGE_LIBPCAP [=n]
```
