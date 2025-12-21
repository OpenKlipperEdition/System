# fb_stage_wip driver usage instructions

## 1. Introduction to module functions

Layer characteristics:
* Supports four-layer display processing units;
* Input format supports RGB88, ARGB8888, RGB565, RGB555, ARGB1555, NV12/NV2, YUV42;
* Supports image cropping;
* 4-layer transparent blending;
* 2-layer scaling support;
* Supports write-back DMA;
* X26XX supports write-back rotation, and supports image rotation at angles of 0°, 90°, 180°, and 270°;

Display characteristics:
Support TFT (MIPI-DPI), SLCD (MIPI-DBI type A, B and C), MIPI-DSI;

## 2. Location of driver source code

Location of driver source code:

***module_drivers/drivers/video/fbdev/ingenic/fb_stage_wip***
To use it, change module_drivers/drivers/video/fbdev/ingenic/Makefile
"-obj-$(CONFIG_FB_INGENIC_STAGE) += fb_stage/" to
"-obj-$(CONFIG_FB_INGENIC_STAGE) += fb_stage_wip/

## 3. Device tree configuration

```
&dpu {
        status = "okay";
        ingenic,disable-rdma-fb = <1>;
        ingenic,rot_angle = <0>;
        /*Defines the init state of composer fb export infomations.*/
        ingenic,layer-exported = <1 1 0 0>;
        ingenic,layer-frames   = <2 2 2 2>;
        ingenic,layer-framesize = <720 1280>, <720 1280>, <320 240>, <320 240>;   /*Max framesize for each layer.*/
        layer,color_mode        = <0 0 0 0>;                                    /*src fmt,*/
        layer,src-size          = <720 1280>, <720 1280>, <320 240>, <240 200>; /*Layer src size should smaller than framesize*/
        layer,target-size       = <720 1280>, <720 640>, <160 240>, <240 200>;  /*Target Size should smaller than src_size.*/
        layer,target-pos        = <0 0>, <0 640>, <340 480>, <100 980>; /*target pos , the start point of the target panel.*/
        layer,enable            = <1 1 1 1>;                                    /*layer enabled or disabled.*/
        ingenic,logo-pan-layer  = <0>;                                          /*on which layer should init logo pan on.*/
        port {
                dpu_out_ep: endpoint {
                        remote-endpoint = <&panel_fw050_ep>;
            };
        };
};
```

## 4. Kernel compilation configuration

### 4.1 Controller Driver Configuration

```
Symbol: FB_INGENIC_STAGE [=y]
Type
: tristate
Prompt: Ingenic Framebuffer Driver for stage
Location:
-> Ingenic device-drivers Configurations
-> [LCD] Panel/Touchscreen Drivers
-> Ingenic Framebuffer Driver (FB_INGENIC [=y])
Defined at module_drivers/drivers/video/fbdev/ingenic/fb_stage/Kconfig
```

### 4.2 Display Configuration (STAGE_FW050)

Different screen configurations can be selected for different screen models

```
CONFIG_STAGE_FW050:
lcd panel FW050, for ingenicfb drivers.
Symbol: STAGE_FW050 [=y]
Type
: tristate
Prompt: lcd panel FW050
Location:
-> Ingenic device-drivers Configurations
-> [LCD] Panel/Touchscreen Drivers
-> Ingenic Framebuffer Driver (FB_INGENIC [=y])
-> Ingenic Framebuffer Driver for stage (FB_INGENIC_STAGE [=y])
-> Supported lcd panels (FB_INGENIC_DISPLAYS_STAGE [=y])
Defined at module_drivers/drivers/video/fbdev/ingenic/fb_stage/displays/Kconfig
```

## 5. Notes

(1). Only x26XX series support rotation

(2). In device tree configuration, layer-framesize needs to be configured as the maximum display area of the screen to ensure enough space can be used. For example, for a vertical 720p screen, it should be configured as <720 1280>.

(3). color_mode needs to be set as 0 or 1, and src_size needs to be set to the same size as layer-framesize.

(4) Only supports two-layer scaling. Layer 2 and layer 3 do not support NV12, NV21 or YUV422 format.

(5). When screen cracking occurs, add the number of buffers in device tree and kernel compilation configuration. If they are configured differently, choose the smaller one to use.

Select for in kernel compilation configuration:

```
Symbol: FB_INGENIC_NR_FRAMES [=3]
Type  : integer
Prompt: how many frames support
  Location:
    -> Device Drivers
      -> Graphics support
        -> Frame buffer Devices
          -> Ingenic Framebuffer Driver (FB_INGENIC [=y])
Prompt: how many frames support
  Location:
    -> Ingenic device-drivers Configurations
      -> [LCD] Panel/Touchscreen Drivers
        -> Ingenic Framebuffer Driver (FB_INGENIC [=y])
  Defined at drivers/video/fbdev/ingenic/Kconfig:30
  Depends on: HAS_IOMEM [=y] && FB_INGENIC [=y]
```

The number of buffers is configured by modifying ingenic and layer-frames in the device tree

(6) X2000 and X2500 use fw050 screen only support 2 lane.
