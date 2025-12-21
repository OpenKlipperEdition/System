# CDBUS控制器接口

## 模块功能介绍

CDBUS控制器支持异步串行通信协议，支持的功能如下：

•引入仲裁机制，自动避免CAN总线等冲突。

•支持双波特率，提供高速通信，最大速率≥ 10 Mbps。

•支持单播、多播和广播。

•最大有效负载数据大小为253字节。

•硬件打包、解包、验证和过滤。

•与传统RS485硬件向后兼容（仍保留仲裁职能）

## 驱动源码位置

驱动源码所在位置：

***module_drivers/drivers/char/ingenic_cdbus.c***

## 设备树配置

设备树所在位置

kernel内核(version <= 5.10)dts文件路径：

***module_drivers/dts/x1600.dtsi***

kernel内核(version > 5.10)dts文件路径：

***module_drivers/dts/x1600/x1600.dtsi***

CDBUS控制器描述
```c
cdbus: cdbus@0x13430000 {

compatible = "ingenic,x1600-cdbus";

 reg = <0x13550000 0x10000>;

interrupt-parent = <&core_intc>;

interrupts = <IRQ_CDBUS>; 

 status = "ok";

};
```

### 设备树默认配置

设备树默认未编译CD_BUS控制器设备。

### 设备树自定义配置

用户可根据实际需求打开CD_BUS控制器设备，例如：
```c
&cdbus {

 status = "okay";

 pinctrl-names = "default";

 pinctrl-0 = <&cdbus_pd>, <&cdbus_tx_en>;

};
```

## 内核编译配置

内核配置INGENIC_CDBUS，配置说明如下： 
```
Symbol: INGENIC_CDBUS [=y] 

Type : boolean 

Prompt: Ingenic Cdbus Driver 

 Location: 

 -> Device Drivers 

(1) -> Character devices 

 Defined at drivers/char/Kconfig
```

### 内核默认编译配置

内核默认编译cdbus驱动, 配置界面如下：

![](assets/CDBUS控制器接口.0.png)

### 内核自定义编译配置

用户可根据实际需求，去掉该驱动的配置。

## 模块内核差异

无

## 设备节点生成

驱动加载成功后生成相应的设备节点
```bash
\# ls /dev/cdbus 
```

## 应用程序使用说明

1. can连接方式

芯片内集成CAN控制器，但还不能进行通讯。CAN控制器需要连接转发器，然后发送给另一个CAN设备的转发器；

转发器连接时。CAN_H连接CAN_H,CAN_L连接CAN_L。当连线出现问题时，会报can bus error错误。

1. cdbus测试需要2块开发板进行收发测试。
2. 接收端开发板执行命如下:
	1. 加载mcu固件：
```bash
# echo 1 > /sys/devices/platform/ahb2/13420000.mcu/load_fw

[ 22.179418] fw: c01a7000: 10000007 00000000 00000000 f4000000 ................

[ 22.186999] fw: c01a7010: f4002f50 f4002f28 f4002f10 f4002f70 P/..(/.../..p/..

[ 22.195087] fw: c01a7020: 40806000 3c1af400 275a2f50 3c1bf400 .`.@...<P/Z'...<

[ 22.202752] fw: c01a7030: 277b68e0 135b0005 00000000 af400000 .h{'..[.......@.

[ 22.210627] fw: c01a7040: 275a0004 175bfffd 00000000 3c1df400 ..Z'..[........<

[ 22.218203] fw: c01a7050: 27bd78e0 3c1af400 275a1f10 0340f809 .x.'...<..Z'..@.

[ 22.226065] fw: c01a7060: 00000000 00000000 00000000 00000000 ................

[ 22.233675] fw: c01a7070: 00000000 00000000 00000000 00000000 ................
```

* 1. 执行接收程序
```bash
\# ./cdbus_test_recv

[ 32.319573] jz_cdbus_hw_init:373 >>> mode CDBUS-A mode

[ 32.324898] jz_cdbus_hw_init:384 >>> clock CDBUS-A mode

[ 32.330836] jz_cdbus_hw_init:399 >>> interrupt CDBUS-A mode

[ 32.336668] cdbus debug

=================recv===================

main():152 >>> recv header = 0x1fd420c

main():153 >>> recv src_addr = 0xc

main():154 >>> recv dst_addr = 0x42

main():155 >>> recv data_len = 253

main():156 >>> recv data:

1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 6

0 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 91 92 93 94 95 96 97 98 99 100 101 102 103 104 105 106 107 108 109 110 111 11

2 113 114 115 116 117 118 119 120 121 122 123 124 125 126 127 128 129 130 131 132 133 134 135 136 137 138 139 140 141 142 143 144 145 146 147 148 149 150 151 152 153 154

 155 156 157 158 159 160 161 162 163 164 165 166 167 168 169 170 171 172 173 174 175 176 177 178 179 180 181 182 183 184 185 186 187 188 189 190 191 192 193 194 195 196 

197 198 199 200 201 202 203 204 205 206 207 208 209 210 211 212 213 214 215 216 217 218 219 220 221 222 223 224 225 226 227 228 229 230 231 232 233 234 235 236 237 238 2

39 240 241 242 243 244 245 246 247 248 249 250 251 252 253 

====================================
```

1. 发送端开发板执行命令如下：
	1. 加载mcu固件：
```bash
# echo 1 > /sys/devices/platform/ahb2/13420000.mcu/load_fw

[ 22.179418] fw: c01a7000: 10000007 00000000 00000000 f4000000 ................

[ 22.186999] fw: c01a7010: f4002f50 f4002f28 f4002f10 f4002f70 P/..(/.../..p/..

[ 22.195087] fw: c01a7020: 40806000 3c1af400 275a2f50 3c1bf400 .`.@...<P/Z'...<

[ 22.202752] fw: c01a7030: 277b68e0 135b0005 00000000 af400000 .h{'..[.......@.

[ 22.210627] fw: c01a7040: 275a0004 175bfffd 00000000 3c1df400 ..Z'..[........<

[ 22.218203] fw: c01a7050: 27bd78e0 3c1af400 275a1f10 0340f809 .x.'...<..Z'..@.

[ 22.226065] fw: c01a7060: 00000000 00000000 00000000 00000000 ................

[ 22.233675] fw: c01a7070: 00000000 00000000 00000000 00000000 ................
```

* 1. 执行发送程序
```bash
\# ./cdbus_test_send

[ 42.600848] jz_cdbus_hw_init:373 >>> mode CDBUS-A mode

[ 42.606155] jz_cdbus_hw_init:384 >>> clock CDBUS-A mode

[ 42.612076] jz_cdbus_hw_init:399 >>> interrupt CDBUS-A mode

=================send===================

send header = 0x1fd420c

send src_addr = 12

send dst_addr = 66

send data_len = 253

send data:

1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 6

0 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 91 92 93 94 95 96 97 98 99 100 101 102 103 104 105 106 107 108 109 110 111 11

2 113 114 115 116 117 118 119 120 121 122 123 124 125 126 127 128 129 130 131 132 133 134 135 136 137 138 139 140 141 142 143 144 145 146 147 148 149 150 151 152 153 154

 155 156 157 158 159 160 161 162 163 164 165 166 167 168 169 170 171 172 173 174 175 176 177 178 179 180 181 182 183 184 185 186 187 188 189 190 191 192 193 194 195 196 

197 198 199 200 201 202 203 204 205 206 207 208 209 210 211 212 213 214 215 216 217 218 219 220 221 222 223 224 225 226 227 228 229 230 231 232 233 234 235 236 237 238 2

39 240 241 242 243 244 245 246 247 248 249 250 251 252 253 

====================================
```

1. 接收端测试程序代码：
```c

#include <unistd.h>

#include <sys/types.h>

#include <sys/stat.h>

#include <fcntl.h>

#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <sys/ioctl.h>

#include <getopt.h>

#include <stdint.h>

struct cdbus_header {

 unsigned char src_addr;

 unsigned char dst_addr;

 unsigned char data_len;

};

struct cdbus_frame {

 struct cdbus_header header;

 unsigned char data[253];

};

enum cdbus_ioctl_cmd {

 CDBUS_A_MODE,

 CDBUS_BS_MODE,

 CDBUS_HAIF_FULL_DUPLEX_MODE,

 CDBUS_FULL_DUPLEX_MODE,

 CDBUS_SET_LOW_RATE,

 CDBUS_SET_HIGH_RATE,

 CDBUS_WRITE_FILTER,

 CDBUS_WRITE_FILTERM,

 CDBUS_ENABLE_LOOPBACK,

};

static const char *device = "/dev/cdbus";

static uint32_t self_addr = 66;

static int verbose = 1;

int main(int argc, char *argv[])

{

 int i;

 int fd, size;

 struct cdbus_frame recv_frame;

 fd = open(device, O_RDWR);

 if (fd <= 0) {

 printf("%s():%d open failed!\n", __func__, __LINE__);

 return -1;

 }

 ioctl(fd, CDBUS_WRITE_FILTER, self_addr);

 ioctl(fd, CDBUS_WRITE_FILTERM, 0x0);

 do {

 size = read(fd, &recv_frame, sizeof(struct cdbus_frame));

 } while (size == -1);

 if (verbose == 1) {

 printf("=================recv===================\n");

 printf("%s():%d >>> recv header = 0x%x\n", __func__, __LINE__, recv_frame.header);

 printf("%s():%d >>> recv src_addr = 0x%x\n", __func__, __LINE__, recv_frame.header.src_addr);

 printf("%s():%d >>> recv dst_addr = 0x%x\n", __func__, __LINE__, recv_frame.header.dst_addr);

 printf("%s():%d >>> recv data_len = %d\n", __func__, __LINE__, recv_frame.header.data_len);

 printf("%s():%d >>> recv data:\n", __func__, __LINE__);

 for (i = 0; i < recv_frame.header.data_len; i++) {

 printf("%d ", recv_frame.data[i]);

 }

 printf("\n");

 printf("====================================\n\n");

 }

 close(fd);

 return 0;

}

```
1. 发送端测试程序代码：
```c

#include <unistd.h>

#include <sys/types.h>

#include <sys/stat.h>

#include <fcntl.h>

#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <sys/ioctl.h>

#include <getopt.h>

#include <stdint.h>

struct cdbus_header {

 unsigned char src_addr;

 unsigned char dst_addr;

 unsigned char data_len;

};

struct cdbus_frame {

 struct cdbus_header header;

 unsigned char data[253];

};

enum cdbus_ioctl_cmd {

 CDBUS_A_MODE,

 CDBUS_BS_MODE,

 CDBUS_HAIF_FULL_DUPLEX_MODE,

 CDBUS_FULL_DUPLEX_MODE,

 CDBUS_SET_LOW_RATE,

 CDBUS_SET_HIGH_RATE,

 CDBUS_WRITE_FILTER,

 CDBUS_WRITE_FILTERM,

 CDBUS_ENABLE_LOOPBACK,

};

static const char *device = "/dev/cdbus";

static uint32_t src_addr = 1;

static uint32_t dst_addr = 66;

static uint32_t self_addr = 88;

static int verbose = 1;

int main(int argc, char *argv[])

{

 int i;

 unsigned int j = 0;

 int fd, size;

 int send_data_len;

 struct cdbus_frame send_frame;

 send_data_len = 253;

 fd = open(device, O_RDWR);

 if (fd <= 0) {

 printf("%s():%d open failed!\n", __func__, __LINE__);

 return -1;

 }

 ioctl(fd, CDBUS_WRITE_FILTER, self_addr);

 ioctl(fd, CDBUS_WRITE_FILTERM, 0x0);

 // src addr and dst addr

 send_frame.header.src_addr = src_addr;

 send_frame.header.dst_addr = dst_addr;

 // data_len

 send_frame.header.data_len = send_data_len;

 // data

 for (i = 0; i < send_frame.header.data_len; i++)

 send_frame.data[i] = i + 1;

 if (verbose == 1) {

 printf("=================send===================\n");

 printf("send header = 0x%x\n", send_frame.header);

 printf("send src_addr = %d\n", send_frame.header.src_addr);

 printf("send dst_addr = %d\n", send_frame.header.dst_addr);

 printf("send data_len = %d\n", send_frame.header.data_len);

 printf("send data:\n");

 for (i = 0; i < send_frame.header.data_len; i++) {

 printf("%d ", send_frame.data[i]);

 }

 printf("\n");

 printf("====================================\n\n");

 }

 write(fd, &send_frame, send_data_len + sizeof(struct cdbus_header));

 close(fd);

 return 0;

}
```
