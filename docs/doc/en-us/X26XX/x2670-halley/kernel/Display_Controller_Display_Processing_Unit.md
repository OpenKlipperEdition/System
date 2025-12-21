# Display Controller

## Module Function Introduction

* Layer Properties:

The display processing unit supports 4 layers of DMA channels;

The input format supports RGB88, ARGB8888, RGB565, RGB555, ARGB1555, NV12/NV21, YUV422;

Supports 2-level TLB;

Supports image cropping;

Supports 4-layer transparent mixed processing;

Supports 2-layer scaling;

Support writing back to DMA;

* Display Characteristics:

Support TFT (MIPI-DPI), SLCD (MIPI-DBI type A, B and C), MIPI-DSI;

Support for image rotation at angles of 0°, 90°, 180° and 270°;

## Drive source code location

Location of driver source code:

***module_drivers/drivers/video/fbdev/ingenic/fb_stage***

## Add a screen

1. Create a new screen file under ***module_drivers/drivers/video/fbdev/ingenic/fb_stage/displays/***

Take FW050 for example:

```
touch panel-fw050.c
```

2. Add screen flags and dependencies in Kconfig as follows:

***module_drivers/drivers/video/fbdev/ingenic/fb_stage/displays/Kconfig***

```
config STAGE_FW050

 tristate "lcd panel FW050"

 depends on FB_INGENIC_DISPLAYS_STAGE

 help

 lcd panel FW050, for ingenicfb drivers.
```

3. Add a new compilation file in Makefile. The path of the file is as follows:

***module_drivers/drivers/video/fbdev/ingenic/fb_stage/displays/Makefile***

Take FW050 for example:

```
obj-$(CONFIG_STAGE_FW050) += panel-fw050.o
```

4. After configuring the screen file according to the screen manual, select the newly added screen in the kernel compilation configuration.
5. Precautions:
* Ensure that all voltages of MIPI LCD are normal
* Ensure that the reset pin timing is normal
* Ensure that the clk setting of LCD is normal, and the gap is not too large. Otherwise, the screen may not be bright
* Ensure that all parameters of LCD such as hbp,fbp,vfb,vbp are normal
* If there are initialization parameters
* The MIPI screen needs to ensure that all setting parameters of MIPI screen and MIPI interface parameter setting function are correct

## Device tree configuration

Location of device tree:

***module_driver/dts/x2600.dtsi***

DPU Controller Description:

```c
dpu: dpu@0x13050000 {

    compatible = "ingenic,x2600-dpu";

    reg = <0x13050000 0x10000>;

    interrupt-parent = <&core_intc>;

    interrupts = <IRQ_LCD>, <IRQ_ROTATE>;

    status = "disabled";

};
```

ROTATE controller description:

```c
rotate : rotate@0x13070000 {

    compatible="ingenic,x2600-rotate";

    reg=<0x130700000 x10000>;

    interrupt-parent=<&core_intc>;

    interrupts=<IRQ_ROTATE>;

    status="okay";

};
```

### Default configuration of device tree

The default compilation of device tree will generate DPU controller devices and ROTATE devices.

#### DPU controller configuration

Adjust settings according to screen size and model configuration

|  |  |
| --- | --- |
| ingenic,disable-rdma-fb | Choose whether to close RDMA: 0 do not close; 1 close. |
| ingenic,rot_angle | Default rotation angle |
| ingenic,layer-exported | Layer export; 0 does not export; 1 exports. |
| ingenic,layer-frames | Number of framebuffers supported per layer: maximum 3. |
| ingenic,layer-framesize | the size of the framebuffer |
| layer, color mode | Format of input image: |
| 0: RGB888 | 1: ARGB8888 |
| 2: RGB555 | 3: ARGB1555 |
| 4: RGB565 | 5: YUV422 |
| 6:NV12 | 7:NV21 |
| layer, src-size | Source image size |
| layer, target-size | Target image size |
| layer, target-pos | Image start position |
| layer,enable | Enable Layer: 1: Enabled, 0: Disabled. |
| remote-endpoint | Remote endpoint: display configuration corresponds to controller configuration |

Configure as follows in ***X2660_HALLEY_MIPI_LCD_FW050.dtsi***:

```c
&dpu {

    status = "okay";

    ingenic,disable-rdma-fb = <1>; /*disable rdma mode*/

    ingenic,rot_angle = <0>;

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

#### Display Configuration

|  |  |
| --- | --- |
| compatible | Compatible screens, corresponding to screen file settings |
| status | okay：use device disable：do not use device |
| pinctrl-0 | Select different pin configurations according to specific screens |
| ingenic,vdd-en-gpio | Power pin: PC08. |
| ingenic, rst-gpio | Reset pin: PC15 |
| ingenic, lcd-pwm-gpio |Backlight pin: PE02 |


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

        ingenic,vdd-en-gpio = <&gpc 8 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;

        ingenic,rst-gpio = <&gpc 15 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;

        /* ingenic,lcd-pwm-gpio = <&gpe 2 GPIO_ACTIVE_HIGH INGENIC_GPIO_NOBIAS>;*/

        port {

            panel_fw050_ep: endpoint {

                remote-endpoint = <&dpu_out_ep>;

            };

        };

    };

};
```

### Device tree custom configuration

Users can disable dpu devices and rotate devices according to actual needs, and configure these nodes as disabled.

#### Screen types supported by x2600 DPU

* smart lcd: dbi hardware interface, lcd screen has its own ram, anti-cracking screen need to have te function
* tft lcd: dpi hardware interface, no ram, need to refresh the screen according to a certain frame rate
* mipi smart lcd: dsi hardware interface, LCD screen has its own ram
* mipi tft lcd: dsi hardware interface, no ram, need to refresh the screen according to a certain frame rate

#### Compatible with other model displays

* Set whether to use rdma, number of layers, frame size, input image format, source image size, target image size, start position of image display, etc
* The DPU controller endpoint (port) corresponds to the display configuration endpoint
* Configure a compatible screen
* Select control pins (pinctrl-0) according to different screen models, different pin settings in ***module_drivers/dts/x2600-pinctrl.dtsi***
* Configure other pins related to the screen, such as power supply pin, reset pin, etc. according to different screens

## Kernel version differences

## Screen Configuration

### Screen Configuration

```c
struct lcd_panel {

    unsigned int num_modes; /*number of display mode supports, fixed value 1*/

    struct fb_videomode *modes; /*display mode*/

    struct jzdsi_data *dsi_pdata; /*if the screen is a mipi dsi interface you need to use this structure pointer*/

    enum ingenic_lcd_type lcd_type; /*screen type, if it is mipi tft lcd need to configure LCD_TYPE_TFT, other by function configuration*/

    unsigned int bpp; /*no filler required*/

    unsigned int width; /*actual physical width of the screen in mm*/

    unsigned int height; /*actual physical height of the screen in mm*/

    struct smart_config *smart_config; /*if the screen is a smart lcd you need to use this structure pointer*/

    struct tft_config *tft_config; /*if the screen is tft lcd you need to use this struct pointer*/

    unsigned dither_enable:1; /*turn on the dither function*/

    struct {

        unsigned dither_red;

        unsigned dither_green;

        unsigned dither_blue;

    } dither;

};

struct fb_videomode {

    const char *name; /* optional */

    u32 refresh; /*configure the frame rate, the driver will calculate pixclock* according to the configuration parameters and frame ratek*/

    u32 xres; /*display effective width, as per screen manual*/

    u32 yres; /*display effective height, according to the screen manual*/

    u32 left_margin; /*fill in according to the manual when tft screen, and assign 0 when smart lcd*/

    u32 right_margin; /*fill in according to the manual when tft screen, and assign 0 when smart lcd*/

    u32 upper_margin; /*fill in according to the manual when tft screen, and assign 0 when smart lcd*/

    u32 lower_margin; /*fill in according to the manual when tft screen, and assign 0 when smart lcd*/

    u32 hsync_len; /*fill in according to the manual when tft screen, and assign 0 when smart lcd*/

    u32 vsync_len; /*fill in according to the manual when tft screen, and assign 0 when smart lcd*/

    u32 sync; /*when the screen is mipi dsi or tft lcd need to be assigned (FB_SYNC_HOR_HIGH_ACT & FB_SYNC_VERT_HIGH_ACT), the other screen is not concerned about the*/

    u32 vmode; /*default value：FB_VMODE_NONINTERLACED*/

    u32 flag;

};
```

### smart LCD configuration

```c
struct smart_config {

    unsigned int te_switch; /* smart LCD te function control */

    unsigned int te_mipi_switch; /* set mipi dsi smart LCD te function control */

    unsigned int te_md; /* 0:te leading edge valid, 1: trailing edge valid */

    unsigned int te_dp; /* 0:te active low, 1: active high */

    unsigned int te_anti_jit; /* 0:te signal keeps 1 pixclk valid, 1:te signal keeps 3 pixclk valid */

    unsigned int dc_md; /* 0:DC high level data, low level command, 1:DC high level command, low level data */

    unsigned int wr_md; /* 0: falling edge sampling, 1: rising edge sampling */

    enum smart_ LCD _type smart_type; /* smart slcd type, supporting 6800/8080/spi-3/spi-4 */

    enum smart_ LCD _format pix_fmt; /* bus data format, support 565,666, etc */

    enum smart_ LCD _dwidth dwidth; /* Data bus width */

    enum smart_ LCD _cwidth cwidth; /* Command bus width */

    unsigned int bus_width;

    unsigned long write_gram_cmd; /* The command to be sent before sending data. Default value: 0x2c */

    unsigned int length_cmd; /* no implementation required */

    struct smart_ LCD _data_table * data_table; /* Configure Screen Command Table */

    unsigned int length_data_table; /* number of configuration screen commands */

    int (* init) (void); /* no implementation required */

    int (* gpio_for_slcd) (void); /* no implementation required */

};
```

### tft lcd configuration

```c
struct tft_config {

    unsigned int pix_clk_inv; /* 0:pixclk default output, 1: invert pixclk */

    unsigned int de_dl; /* 0:DE pin high output valid data, 1: low output valid data */

    unsigned int sync_dl; /* 0:vsync and hsync pins output valid data at high level, 1: output valid data at low level */

    enum tft_ LCD _color_even color_even; /* bus RGB order in even rows */

    enum tft_ LCD _color_odd color_odd; /* bus RGB order in odd row */

    enum tft_ LCD _mode mode; /* bus data format, support 888/666/565 */

};
```

### mipi dsi lcd configuration

1. mipi dsi lcd configuration

```c
struct jzdsi_data jzdsi_pdata = {

    .modes = & panel_modes, /* display mode */

    .video_config.no_of_lanes = 2, /* according to the hardware connection, support 1, 2lane */

    .video_config.virtual_channel = 0, /* Default: 0 */

    .video_config.color_coding = COLOR_CODE_24BIT, /* RGB888:COLOR_CODE_24BIT, RGB565:COLOR_CODE_16BIT_CONFIG1, note: COLOR_CODE_24BIT need to be configured when smart LCD */

    .video_config.video_mode = VIDEO_BURST_WITH_SYNC_PULSES, /* Default: VIDEO_BURST_WITH_SYNC_PULSES */

    .video_config.receive_ ack_packets = 0, /* Default: 0 */

    .video_config.is_18_loosely = 0, /* Default: 0 */

    .video_config.data_en_polarity = 1, /* Default: 1 */

    .video_config.byte_clock = 0, /* Default value: 0, the driver is automatically calculated according to the configuration parameters */

    .video_config.byte_clock_coef = MIPI_PHY_BYTE_CLK_COEF_MUL6_DIV5, /* byte_clock coefficient, the coefficient needs to be changed according to the actual situation to ensure that the smaller the coefficient, the better under normal display conditions, for example: MUL6_DIV5 = 1.2 (multiplied by 6 divided by 5)*/

    .dsi_config.max_lanes = 2, /* fixed value: 2 */

    .dsi_config.max_ hs_to_lp_cycles = 100, /* Default value: 100 */

    .dsi_config.max_ lp_to_hs_cycles = 40, /* Default: 40 */

    .dsi_config.max_ bta_cycles = 4095, /* Default: 4095 */

    .dsi_config.color_mode_polarity = 1, /* Default: 1 */

    .dsi_config.shut_down_polarity = 1, /* Default: 1 */

    .dsi_config.max_bps = 2750, /* Default: 2.75Gbps */

    .bpp_info = 24, /* RGB888:24 RGB565:16, note: when smart LCD needs to be configured 24 */

};
```

2. mipi dsi screen register common configuration method

Send at least 2 parameters:

```c
struct dsi_cmd_packet cmd = {0x39, 0x05, 0x00, {0x2A, 0x00, 0x00, 0x02, 0xCF}}
```

0x39: Command type, sending more than or equal to two parameters.

0x05: Number of parameters, 5 parameters.

0x00: No meaning, default fill with 0x00.

{0x2A, 0x00, 0x00, 0x02, 0xCF}: five parameter values.

Send 2 parameters:

```c
struct dsi_cmd_packet cmd = {0x15, 0xC2, 0x08}
```

0x15: Command type, send two parameters.

0xC2: The first parameter.

0x08: The second parameter.

Send 1 parameter:

```c
struct dsi_cmd_packet cmd = {0x05, 0x10, 0x00}
```

0x05: Command type, send one parameter.

0x10: The first parameter.

0x00: No meaning, default fill with 0x00.

### pwm backlight configuration

When using pwm backlight, you can configure as follows:

```c
&pwm {

    pinctrl-names="default ";

    pinctrl-0 =<& pwm10_pc>; /* configure pwm gpio according to the actual situation */

    status="okay ";

};

backlight {

    compatible="pwm-backlight ";

    pwms =<& pwm 10 1000000>; /* select pwm1 to control the backlight and set the period1000000ns */

    brightness-levels =<0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15>;/* Backlight level, adjustable as required */

    default-brightness-level=<4>;

}
```

## Kernel compilation configuration

### Controller-driven configuration (FB_INGENIC_STATE):

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

### ROTATE drive configuration (ROT_INGENIC):

```
Symbol: ROT_INGENIC [=y]

Type  : tristate

Prompt: X2600 Rotator
 Location:

 -> Ingenic device-drivers Configurations

 -> [LCD] Panel/Touchscreen Drivers

 -> Ingenic Framebuffer Driver (FB_INGENIC [=y])

 Defined at module_drivers/drivers/video/fbdev/ingenic/Kconfig:47

 Depends on: SOC_X2600 [=y] && FB_INGENIC [=y]


Symbol: ROT_INGENIC_BUFS [=2]

Type  : integer

Prompt: how many rot buf support

 Location:

 -> Ingenic device-drivers Configurations

 -> [LCD] Panel/Touchscreen Drivers

 -> Ingenic Framebuffer Driver (FB_INGENIC [=y])

 -> X2600 Rotator (ROT_INGENIC [=y])

 Defined at module_drivers/drivers/video/fbdev/ingenic/Kconfig:55

 Depends on: ROT_INGENIC [=y]
```

### pwm backlight drive configuration (PWM_INGENIC_V3):

```
Symbol: PWM_INGENIC_V3 [=y]

Type : tristate

Prompt: Ingenic PWM V3 support

 Location:

 -> Ingenic device-drivers Configurations

 -> [PWM] drivers

 Defined at module_drivers/drivers/pwm/Kconfig
```

### Display Configuration (STAGE_FW050)

The screen with Mipi DSI interface supports automatic detection. The automatic detection will judge one by one whether the selected screen is available, and stop detecting when a screen is available or all screens are unavailable. Here's the FW050 screen selection.

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

Defined at module_drivers/drivers/video/fbdev/ingenic/fb_stage/displays/Kconfig:33

 Depends on: FB_INGENIC [=y] && FB_INGENIC_DISPLAYS_STAGE [=y]
```

## How to use the tlb function

1. The address of the requested data buffer needs to be aligned to 4096.
2. Data buffer virtual memory to physical memory mapping and tlb table creation function.

```c
 unsigned int gtlb_base = ioctl(int fd, JZFB_DMMU_MAP, struct dpu_dmmu_map_info *di)
```

If the data buffer is obtained multiple times, you need to call the function multiple times to return the Tlb table address. You can also complete the buffer application and call the function once at a time.

```c
int fd /*fb device file descriptor*/

unsigned int gtlb_base /*tlb level 1 page table address, the address value is unique, so the return value will be the same after multiple calls*/

struct dpu_dmmu_map_info {

    unsigned int addr; /*buffer address*/

    unsigned int len; /*length of buffer address*/

};
```

3. Configure tlb first-level page table address function

```c
ioctl(int fd, JZFB_USE_TLB, unsigned int gtlb_base)；

int fd /*fb device file descriptor */

unsigned int gtlb_base
```

4. Pass buffer address of each layer to driver

```c
 struct jzfb_lay_cfg {

    .addr[0] = 0x7xxxxxxx; /*0 frame address*/

    .addr[1] = 0x7xxxxxxx; /*1 frame address*/

    .addr[2] = 0x7xxxxxxx; /*2 frame address*/

}
```

5. Refresh cache function after updating image data

```c
ioctl(int fd, JZFB_DMMU_FLUSH_CACHE, struct dpu_dmmu_map_info *di);
```

## Device Node Generation

After successful loading of the driver, the following nodes are generated:

```
/dev/fb0    /dev/fb1
```

## Functional test instructions

### Test methods

#### dpu function test

The test involves rdma write-back functionality, which requires enabling rdma mode in the LCD device tree:

```c
ingenic,disable-rdma-fb = <0>;
```

Taking x2660 halley v1.0 as an example, modify the DPU configuration in ***X2660_HALLEY_MIPI_LCD_FW050.dtsi*** as follows:

```c
&dpu {

    status = "okay";

    ingenic,disable-rdma-fb = <0>;

    ingenic,rot_angle = <0>;

    ....

    ....

}
```

DPU function test using IMPP test demo: dpu-osd-example

Required image files for testing: 720x1280.argb, 640x480.nv12. The test image file needs to be in the same directory as the test demo.

Execute test command

```
./dpu-osd-example
```

Observe whether the LCD display image conforms to the expected test settings

#### composer mode switch display format

```
echo 6 > /sys/devices/platform/ahb0/13050000.dpu/layer0/src_fmt /*specify LCD display format*/

echo 1 > /sys/devices/platform/ahb0/13050000.dpu/comp_update
```

#### rdma mode enabled

Modify screen device tree dpu part

```c
ingenic,disable-rdma-fb = <0>; /*disable rdma mode*/
```

#### rotate image rotation function test

The test command is as follows, which makes the Junzheng logo rotate 180 degrees.

```
cat /dev/fb0 > logo.rgb

echo 720x1280 > /sys/devices/platform/ahb0/13050000.dpu/layer0/src_size

echo 720x1280 > /sys/devices/platform/ahb0/13050000.dpu/layer0/target_size

echo 720 > /sys/devices/platform/ahb0/13050000.dpu/layer0/stride

echo 180 > /sys/devices/platform/ahb0/13050000.dpu/rot_angle

cat logo.rgb > /dev/fb0

echo 1 > /sys/devices/platform/ahb0/13050000.dpu/comp_update
```

## Caution

1. It supports up to 2 layers of scaling, and the scaling can be any two layers.

2. Only layers 1 and 2 support yuv422, nv12, nv21.

3. When a TFT screen cracks, you can modify the number of framebuffers (the default is 1). You can change it to 2 or 3. After modifying the number of framebuffers in the kernel compilation configuration, modify it again in the device tree, taking the smaller value between them.

The kernel compilation configuration is as follows:

```
Symbol: FB_INGENIC_NR_FRAMES [=1]

Type : integer

Prompt: how many frames support

Location:

 -> Ingenic device-drivers Configurations

 -> [LCD] Panel/Touchscreen Drivers

 -> Ingenic Framebuffer Driver (FB_INGENIC [=y])

Defined at drivers/video/fbdev/ingenic/Kconfig
```
