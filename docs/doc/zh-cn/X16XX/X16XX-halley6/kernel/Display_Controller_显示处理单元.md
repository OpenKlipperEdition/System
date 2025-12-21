# Display Controller 显示处理单元

## 模块功能介绍

* 图层特性：

输入格式支持RGB888，RGB565，RGB555；

* 显示特性：

支持TFT，SLCD；

## 驱动源码位置

驱动源码所在位置：

***module_drivers/drivers/video/fbdev/ingenic/fb_stage***

## 添加屏

1. 在**module_drivers/drivers/video/fbdev/ingenic/fb_stage/displays/**下新建相关屏的文件

以FW050为例
```bash
touch panel-fw050.c
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

***module_driver/dts/halley6_v10.dts***

需在设备树中包含对应的屏

***module_drivers/dts/halley6_lcd/RD_X1600_HALLEY6_RGB_SLCD_1V0.dtsi***

DPU控制器描述：
```c
dpu: dpu@0x13050000 {

 compatible = "ingenic,x1600-dpu";

 reg = <0x13050000 0x10000>;

 interrupt-parent = <&core_intc>;

 interrupts = <IRQ_LCD>;

 status = "disabled";

 };
```

### 设备树默认配置

设备树默认编译会产生DPU控制器设备。

#### DPU控制器配置

在RD_X1600_HALLEY6_RGB_SLCD_1V0.dtsi中配置如下： 

```c
&dpu {

 status = "okay";

 port {

 dpu_out_ep: endpoint {

 remote-endpoint = <&panel_fw035_ep>;

 };

 };

};
```

#### **显示屏配置**
```c

display-dbi {

 compatible = "simple-bus";

 #interrupt-cells = <1>;

 #address-cells = <1>;

 #size-cells = <1>;

 ranges = <>;

 panel_fw035 {

 compatible = "ingenic,fw035";

 status = "okay";

 pinctrl-names = "default";

 pinctrl-0 = <&smart_lcd_pa_8bit>;

 ingenic,pwm-gpio = <&gpc 0 GPIO_ACTIVE_HIGH INGENIC_GPIO_NOBIAS>;*/

 ingenic,cs-gpio = <&gpa 23 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;

 ingenic,rd-gpio = <&gpa 16 GPIO_ACTIVE_HIGH INGENIC_GPIO_NOBIAS>;

 ingenic,vdd-en-gpio = <&gpa 31 GPIO_ACTIVE_HIGH INGENIC_GPIO_NOBIAS>;

 ingenic,rst-gpio = <&gpb 13 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;

 port {

 panel_fw035_ep: endpoint {

 remote-endpoint = <&dpu_out_ep>;

 };

 }; 

 }; 

};
``` 

### 设备树自定义配置

用户可根据实际需求关闭dpu设备，将以上节点配置为disabled，或匹配其他型号显示屏。

#### **X1600 DPU 支持的屏幕种类**

* smart lcd: dbi硬件接口，液晶屏有自己的ram，防裂屏需要有te功能
* tft lcd: dpi硬件接口，没有ram，需要按照一定的帧率刷新屏幕

#### **匹配其他型号显示屏**

* DPU控制器端点（port）与显示屏配置端点对应
* 配置兼容的屏（compatible）
* 根据不同型号的屏选择控制引脚(pinctrl-0)，不同的引脚设置在module_drivers/dts/ x1600-pinctrl.dtsi
* 根据不同的屏配置其他与屏相关的引脚，例如供电引脚，复位引脚等

## 内核模块差异

## 屏幕配置

### 屏幕配置
```c

struct lcd_panel {

 const char *name;

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

 struct lcd_panel_ops *ops; /*不需要实现*/

};

struct fb_videomode {

 const char *name; /* optional */

 u32 refresh; /*配置帧率，驱动会根据配置参数和帧率计算pixclock*/

 u32 xres; /*显示有效宽度，按照屏手册填写*/

 u32 yres; /*显示有效高度，按照屏手册填写*/

 u32 pixclock;

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

### pwm背光配置

需要使用pwm背光时可在板级.dts配置pwm：
```c

&pwm {

 pinctrl-names = "default";

 pinctrl-0 = <&pwm0_pc>;

 status = "okay";

}; 
```

目前支持的屏幕pwm背光设备树位置：

module_drivers/dts/halley6_lcd

以fw035型号屏幕为例：
```c

backlight {

 compatible = "pwm-backlight";

 pwms = <&pwm 0 1000000>;

 brightness-levels = <0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15>;

 default-brightness-level = <4>;

}; 
```

## 内核编译配置

内核进行控制器和显示屏及背光配置，配置说明如下：

控制器驱动配置：
```
Symbol: FB_INGENIC_STAGE [=y] 

Type : tristate 

Prompt: Ingenic Framebuffer Driver for stage 

 Location: 

 -> Ingenic device-drivers Configurations 

 -> [LCD] Panel/Touchscreen Drivers 

 -> Ingenic Framebuffer Driver (FB_INGENIC [=y]) 

 Defined at module_drivers/drivers/video/fbdev/ingenic/fb_stage/Kconfig
```

显示屏配置
```
Symbol: STAGE_FW035 [=y] 

 

Type : tristate 

Prompt: lcd panel FW035 

 Location: 

 -> Ingenic device-drivers Configurations 

 -> [LCD] Panel/Touchscreen Drivers 

 -> Ingenic Framebuffer Driver (FB_INGENIC [=y]) 

 -> Ingenic Framebuffer Driver for stage (FB_INGENIC_STAGE [=y]) 

 -> Supported lcd panels (FB_INGENIC_DISPLAYS_STAGE [=y]) 

 Defined at module_drivers/drivers/video/fbdev/ingenic/fb_stage/displays/Kconfig 
```

pwm背光驱动配置： 
```
Symbol: PWM_INGENIC_V3 [=y] 

Type : boolean 

Prompt: Ingenic V3 PWM support 

 Location: 

 -> Ingenic device-drivers Configurations 

 -> [PWM] drivers 

 Defined at module_drivers/drivers/pwm/Kconfig
```

## 设备节点生成

驱动加载成功后生成以下节点：

***/dev/fb0***

