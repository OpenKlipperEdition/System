# Display Controller 显示处理单元

## 模块功能介绍

* 图层特性：

显示处理单元支持4层DMA通道；

输入格式支持RGB88，ARGB8888，RGB565，RGB555，ARGB1555，NV12/NV21, YUV422；

支持2级TLB；

支持图像裁剪；

支持4层透明混合处理；

支持2层缩放；

支持写回DMA；

* 显示特性：

支持TFT（MIPI-DPI），SLCD（MIPI-DBI type A，B and C），MIPI-DSI；

## 驱动源码位置

驱动源码所在位置：

***module_drivers/drivers/video/fbdev/ingenic/fb_stage***

## 添加屏

1. 在***module_drivers/drivers/video/fbdev/ingenic/fb_stage/displays/***下新建相关屏的文件

以FW050为例
```bash
$ touch panel-fw050.c
```

2. 在Kconfig中添加屏的标志和依赖，路径如下：

***module_drivers/drivers/video/fbdev/ingenic/fb_stage/displays/Kconfig***

以FW050为例
```c
config STAGE_FW050 

 tristate "lcd panel FW050"

 depends on FB_INGENIC_DISPLAYS_STAGE

 help

 lcd panel FW050, for ingenicfb drivers.
```

3. 在Makefile中添加新的编译文件,文件路径如下：

***module_drivers/drivers/video/fbdev/ingenic/fb_stage/displays/Makefile***

以FW050为例
```c
obj-$(CONFIG_STAGE_FW050) += panel-fw050.o
```

4. 根据屏幕手册配置完成屏文件后，在内核编译配置中选择新添加的屏。
5. 注意事项
* 保证MIPI lcd 的各路电压都正常
* 保证复位引脚时序正常
* 保证lcd 的clk设置正常，差距不易过大，否则可能导致屏幕不亮
* 保证lcd 的hbp，fbp，vfb，vbp等各项参数正常
* 如果有初始化参数，需保证初始化参数是对的
* MIPI屏需保证MIPI 屏幕各项设置参数以及MIPI接口参数设置函数是对的

## 设备树配置

设备树所在位置：

kernel内核(version <= 5.10)dts文件路径：

***module_driver/dts/x2000.dtsi***

kernel内核(version > 5.10)dts文件路径：

***module_driver/dts/x2000/x2000.dtsi***

DPU控制器描述：
```c
dpu: dpu@0x13050000 {

 compatible = "ingenic,x2000-dpu";

 reg = <0x13050000 0x10000>;

 interrupt-parent = <&core_intc>;

 interrupts = <IRQ_LCD>;

 status = "disabled";

 };
```

### 设备树默认配置

设备树默认编译会产生DPU控制器设备。

#### DPU控制器配置

根据屏幕大小以及型号配置相应的设置

|  |  |
| --- | --- |
| ingenic,disable-rdma-fb | 选择是否关闭RDMA：0不关闭; 1关闭 |
| ingenic,layer-exported | 图层导出；0不导出; 1导出 |
| ingenic,layer-frames | 每层支持framebuffer的个数：最大为3 |
| ingenic,layer-framesize | framebuffer的大小 |
| layer,color_mode | 输入图像的格式： |
| 0:RGB888 | 1:ARGB8888 |
| 2:RGB555 | 3:ARGB1555 |
| 4:RGB565 | 5:YUV422 |
| 6:NV12 | 7:NV21 |
| layer,src-size | 源图像大小 |
| layer,target-size | 目标图像大小 |
| layer,target-pos | 图像起始位置 |
| layer,enable | 使能层：1：使能 0:未使能 |
| remote-endpoint | 远程端点：显示屏配置与控制器配置对应 |

在halley5_v30.dts中配置如下：
```c
&dpu {

 status = "okay";

 ingenic,disable-rdma-fb = <1>;

 /*Defines the init state of composer fb export infomations.*/

 ingenic,layer-exported = <1 1 0 0>;

 ingenic,layer-frames = <2 2 2 2>;

 ingenic,layer-framesize = <720 1280>, <720 1280>, <320 240>, <320 240>; /*Max framesize for each layer.*/

 layer,color_mode = <0 0 0 0>; /*src fmt,*/

 layer,src-size = <720 1280>, <720 1280>, <320 240>, <240 200>; /*Layer src size should smaller than framesize*/

 layer,target-size = <720 1280>, <720 640>, <160 240>, <240 200>; /*Target Size should smaller than src_size.*/

 layer,target-pos = <0 0>, <0 640>, <340 480>, <100 980>; /*target pos , the start point of the target panel.*/

 layer,enable = <1 1 1 1>; /*layer enabled or disabled.*/

 ingenic,logo-pan-layer = <0>; /*on which layer should init logo pan on.*/

 port {

 dpu_out_ep: endpoint {

 remote-endpoint = <&panel_fw050_ep>;

 };

 };

};
```

#### **显示屏配置**

|  |  |
| --- | --- |
| compatible | 兼容的屏幕，与屏幕文件设置的对应 |
| status | okay：使用设备 disable：不使用设备 |
| pinctrl-0 | 根据具体的屏选择不同的引脚配置 |
| ingenic,vdd-en-gpio | 供电引脚：PC03 |
| ingenic,rst-gpio | 复位引脚：PC04 |
| ingenic,lcd-pwm-gpio | 背光引脚：PC01 |

```c
display-dbi {

 compatible = "simple-bus";

 #interrupt-cells = <1>;

 #address-cells = <1>;

 #size-cells = <1>;

 ranges = <>;
 panel_fw050 {

 compatible = "ingenic,fw050";

 status = "okay";

 pinctrl-names = "default";

 pinctrl-0 = <&smart_lcd_pb_te>;

 ingenic,vdd-en-gpio = <&gpc 3 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;

 ingenic,rst-gpio = <&gpc 4 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;

 /* ingenic,lcd-pwm-gpio = <&gpc 1 GPIO_ACTIVE_HIGH INGENIC_GPIO_NOBIAS>;*/

 port {

 panel_fw050_ep: endpoint {

 remote-endpoint = <&dpu_out_ep>;

 };

 };

 };

};
```

### 设备树自定义配置

用户可根据实际需求关闭dpu设备，将以上节点配置为disabled。

#### x2000 DPU 支持的屏幕种类

* smart lcd: dbi硬件接口，液晶屏有自己的ram，防裂屏需要有te功能
* tft lcd: dpi硬件接口，没有ram，需要按照一定的帧率刷新屏幕
* mipi smart lcd: dsi硬件接口，液晶屏有自己的ram
* mipi tft lcd: dsi硬件接口，没有ram，需要按照一定的帧率刷新屏幕

#### 匹配其他型号显示屏

* 设置是否使用rdma，层数，帧大小，输入图像格式，源图像大小，目标图像大小，图像显示的起始位置等
* DPU控制器端点（port）与显示屏配置端点对应
* 配置兼容的屏（compatible）
* 根据不同型号的屏选择控制引脚(pinctrl-0)，不同的引脚设置在module_drivers/dts/x2000-pinctrl.dtsi或module_drivers/dts/x2000/x2000-pinctrl.dtsi
* 根据不同的屏配置其他与屏相关的引脚，例如供电引脚，复位引脚等

## 内核模块差异

## 屏幕配置

### 屏幕配置
```c
struct lcd_panel {

 unsigned int num_modes; /*显示模式支持的数量，固定值1*/ 

 struct fb_videomode *modes; /*显示模式*/

 struct jzdsi_data *dsi_pdata; /*如果屏幕是mipi dsi接口需要实现*/

 enum ingenic_lcd_type lcd_type; /*屏幕种类，如果是mipi tft lcd需要配置LCD_TYPE_TFT，其它的按功能配置*/

 unsigned int bpp; /*不需要填充*/

 unsigned int width; /*屏幕实际物理宽度，单位mm*/

 unsigned int height; /*屏幕实际物理高度，单位mm*/

 struct smart_config *smart_config; /*如果屏幕是smart lcd需要实现*/

 struct tft_config *tft_config; /*如果屏幕是tft lcd需要实现*/

 unsigned dither_enable:1; /*打开dither功能*/

 struct {

 unsigned dither_red;

 unsigned dither_green;

 unsigned dither_blue;

 } dither;

};

struct fb_videomode {

 const char *name; /* optional */

 u32 refresh; /*配置帧率，驱动会根据配置参数和帧率计算pixclock*/

 u32 xres; /*显示有效宽度，按照屏手册填写*/

 u32 yres; /*显示有效高度，按照屏手册填写*/

 u32 left_margin; /*tft屏时按照手册填写，当smart lcd时赋值0*/

 u32 right_margin; /*tft屏时按照手册填写，当smart lcd时赋值0*/

 u32 upper_margin; /*tft屏时按照手册填写，当smart lcd时赋值0*/

 u32 lower_margin; /*tft屏时按照手册填写，当smart lcd时赋值0*/

 u32 hsync_len; /*tft屏时按照手册填写，当smart lcd时赋值0*/

 u32 vsync_len; /*tft屏时按照手册填写，当smart lcd时赋值0*/

 u32 sync; /*当mipi dsi tft lcd时需要赋值（FB_SYNC_HOR_HIGH_ACT & FB_SYNC_VERT_HIGH_ACT)，其他的屏幕不关注*/

 u32 vmode; /*默认值：FB_VMODE_NONINTERLACED*/

 u32 flag;

};
```

### smart lcd 配置
```c
struct smart_config {

 unsigned int te_switch; /*smart lcd te功能控制*/

 unsigned int te_mipi_switch; /*设置 mipi dsi smart lcd te功能控制*/

 unsigned int te_md; /*0:te前沿有效，1:后沿有效*/

 unsigned int te_dp; /*0:te低电平有效，1:高电平有效*/

 unsigned int te_anti_jit; /*0:te信号保持1个pixclk有效，1:te信号保持3个pixclk有效*/

 unsigned int dc_md; /*0:DC高电平数据，低电平命令, 1:DC高电平命令，低电平数据*/

 unsigned int wr_md; /*0:下降沿采样，1:上升沿采样 */

 enum smart_lcd_type smart_type; /*smart slcd种类，支持6800/8080/spi-3/spi-4*/

 enum smart_lcd_format pix_fmt; /*总线数据格式，支持565,666等*/

 enum smart_lcd_dwidth dwidth; /*数据总线宽度*/

 enum smart_lcd_cwidth cwidth; /*命令总线宽度*/

 unsigned int bus_width;

 unsigned long write_gram_cmd; /*发送数据前需要发送的命令，默认0x2c*/

 unsigned int length_cmd; /*不需要实现*/

 struct smart_lcd_data_table *data_table; /*配置屏幕命令表*/

 unsigned int length_data_table; /*配置屏幕命令数量*/

 int (*init) (void); /*不需要实现*/

 int (*gpio_for_slcd) (void); /*不需要实现*/

};
```
### tft lcd配置
```c
struct tft_config {

 unsigned int pix_clk_inv; /*0:pixclk默认输出， 1:反转pixclk*/

 unsigned int de_dl; /*0:DE引脚高电平输出有效数据， 1:低电平输出有效数据*/

 unsigned int sync_dl; /*0:vsync和hsync引脚高电平输出有效数据， 1:低电平输出有效数据*/

 enum tft_lcd_color_even color_even; /*偶数行时总线RGB顺序*/

 enum tft_lcd_color_odd color_odd; /*奇数行时总线RGB顺序*/

 enum tft_lcd_mode mode; /*总线数据格式，支持888/666/565*/

};
```
### mipi dsi lcd配置

1. mipi dsi lcd 配置
```c
struct jzdsi_data jzdsi_pdata = {

 .modes = &panel_modes, /*显示模式*/

 .video_config.no_of_lanes = 2, /*按照硬件连接填写，支持1、2lane*/

 .video_config.virtual_channel = 0, /*默认值:0*/

 .video_config.color_coding = COLOR_CODE_24BIT, /*RGB888:COLOR_CODE_24BIT, RGB565:COLOR_CODE_16BIT_CONFIG1，注：当smart lcd时需要配置COLOR_CODE_24BIT*/

 .video_config.video_mode = VIDEO_BURST_WITH_SYNC_PULSES, /*默认值:VIDEO_BURST_WITH_SYNC_PULSES*/

 .video_config.receive_ack_packets = 0, /*默认值:0*/

 .video_config.is_18_loosely = 0, /*默认值:0*/

 .video_config.data_en_polarity = 1, /*默认值:1*/

 .video_config.byte_clock = 0, /*默认值:0,驱动根据配置参数自动计算*/

 .video_config.byte_clock_coef = MIPI_PHY_BYTE_CLK_COEF_MUL6_DIV5, /*byte_clock系数，需要根据实际情况变动系数，保证正常显示情况下系数越小越好，例：MUL6_DIV5=1.2(乘6除5)*/

 .dsi_config.max_lanes = 2, /*固定值:2*/

 .dsi_config.max_hs_to_lp_cycles = 100, /*默认值:100*/ 

 .dsi_config.max_lp_to_hs_cycles = 40, /*默认值:40*/

 .dsi_config.max_bta_cycles = 4095, /*默认值:4095*/

 .dsi_config.color_mode_polarity = 1, /*默认值：1*/

 .dsi_config.shut_down_polarity = 1, /*默认值：1*/

 .dsi_config.max_bps = 2750, /*默认值:2.75Gbps*/

 .bpp_info = 24, /*RGB888:24 RGB565:16，注：当smart lcd时需要配置24*/

};
```

1. mipi dsi屏幕寄存器通用配置方式

发送大于等于2个参数:

 struct dsi_cmd_packet cmd = {0x39, 0x05, 0x00, {0x2A, 0x00, 0x00, 0x02, 0xCF}}

 0x39: 命令种类，发送大于等于2个参数

 0x05: 参数个数，5个参数

 0x00: 无意义，默认填充0x00

 {0x2A, 0x00, 0x00, 0x02, 0xCF}: 5个参数值

发送2个参数:

 struct dsi_cmd_packet cmd = {0x15, 0xC2, 0x08}

 0x15: 命令种类，发送2个参数

 0xC2: 第一个参数

 0x08: 第二个参数

发送1个参数

 struct dsi_cmd_packet cmd = {0x05, 0x10, 0x00}

 0x05: 命令种类，发送1个参数

 0x10: 第一个参数

 0x00: 无意义，默认填充0x00

### pwm背光配置

需要使用pwm背光时可进行如下配置：
```c
&pwm {

pinctrl-names="default";

pinctrl-0=<*** ***&pwm1_pc>; /*按照实际情况配置pwm的gpio*/

status="okay";

};

backlight {

compatible="pwm-backlight";

pwms=<&pwm 1 1000000>; /*选择pwm1控制背光，设置period1000000ns*/

brightness-levels=<0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15>;/*背光等级，可根据需求调整*/

default-brightness-level=<4>;

}
```

## 内核编译配置

### 控制器驱动配置（FB_INGENIC_STATE）：
```
Symbol: FB_INGENIC_STAGE [=y] 

Type : tristate 

Prompt: Ingenic Framebuffer Driver for stage 

 Location: 

 -> Ingenic device-drivers Configurations 

 -> [LCD] Panel/Touchscreen Drivers 

 -> Ingenic Framebuffer Driver (FB_INGENIC [=y]) 

 Defined at module_drivers/drivers/video/fbdev/ingenic/fb_stage/Kconfig 

### pwm背光驱动配置（PWM_INGENIC_V2）：
```

```
Symbol: PWM_INGENIC_V2 [=y] 

Type : tristate 

Prompt: Ingenic PWM V2 support 

 Location: 

 -> Ingenic device-drivers Configurations 

 -> [PWM] drivers 

 Defined at module_drivers/drivers/pwm/Kconfig
```

### 显示屏配置（STAGE_FW050）

mipi dsi接口的屏幕支持自动探测，自动探测会挨个判断选中的屏幕是否可用，当有一款屏幕可用或者所有的屏幕都不可用时停止探测。如下为选择屏FW050。
```
CONFIG_STAGE_FW050: 

lcd panel FW050, for ingenicfb drivers. 

Symbol: STAGE_FW050 [=y] 

Type : tristate 

Prompt: lcd panel FW050 

 Location: 

 -> Ingenic device-drivers Configurations 

 -> [LCD] Panel/Touchscreen Drivers 

 -> Ingenic Framebuffer Driver (FB_INGENIC [=y]) 

 -> Ingenic Framebuffer Driver for stage (FB_INGENIC_STAGE [=y]) 

 -> Supported lcd panels (FB_INGENIC_DISPLAYS_STAGE [=y]) 

 Defined at module_drivers/drivers/video/fbdev/ingenic/fb_stage/displays/Kconfig 
```

##  tlb功能使用方法

1. 申请的数据buffer地址需要按4096对齐.
2. 数据buffer虚拟内存到物理内存的映射和tlb表的制作函数.
```c
 unsigned int gtlb_base = ioctl(int fd, JZFB_DMMU_MAP, struct dpu_dmmu_map_info *di)
```

 如果使用的数据buffer是多次申请获得的，需要多次调用函返回tlb表地址，也可以一次完成buffer的申请并调用一次函数。
```c
int fd /*fb设备文件描述符*/

 unsigned int gtlb_base /*tlb一级页表地址，地址值唯一，所以多次调用后返回值相同*/

 struct dpu_dmmu_map_info {

 unsigned int addr; /*buffer 地址*/

 unsigned int len; /*buffer地址的长度*/

 };
```

1. 配置tlb一级页表地址函数
```c
 ioctl(int fd, JZFB_USE_TLB, unsigned int gtlb_base)；

 int fd /*fb设备文件描述符 */

 unsigned int gtlb_base
```

2. 给驱动传递每层的buffer地址
```c
struct jzfb_lay_cfg {

 .addr[0] = 0x7xxxxxxx; /*0帧的地址*/

 .addr[1] = 0x7xxxxxxx; /*1帧的地址*/

 .addr[2] = 0x7xxxxxxx; /*2帧的地址*/

}
```

3. 更新图像数据后刷新cache函数
```c
ioctl(int fd, JZFB_DMMU_FLUSH_CACHE, struct dpu_dmmu_map_info *di)
```

## 设备节点生成

驱动加载成功后生成以下节点：

***/dev/fb0***

***/dev/fb1***

## 注意事项

(1)最多支持2层缩放，缩放可以是任意2层

(2)只有1,2层支持yuv422、nv12、nv21

(3)TFT屏出现裂屏时，可以修改framebuffer的数量（默认为1），可以将其修改为2或3，在内核编译配置中修改完framebuffer数量后在设备树中修改，两者取小值。

内核编译配置如下：
```
Symbol: FB_INGENIC_NR_FRAMES [=1]

Type : integer 

Prompt: how many frames support 

Location: 

 -> Ingenic device-drivers Configurations 

 -> [LCD] Panel/Touchscreen Drivers 

 -> Ingenic Framebuffer Driver (FB_INGENIC [=y])

Defined at drivers/video/fbdev/ingenic/Kconfig 

(4)slcd屏出现裂屏时，可以通过te引脚改变
```
