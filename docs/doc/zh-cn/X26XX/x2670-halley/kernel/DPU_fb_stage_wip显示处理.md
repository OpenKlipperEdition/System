# fb_stage_wip驱动使用说明

## 1.模块功能介绍
图层特性:
显示处理单元支持 4 层显示;
输入格式支持 RGB88, ARGB8888, RGB565, RGB555, ARGB1555, NV12/NV21, YUV422;
支持图像裁剪;
支持 4 层透明混合处理;
支持 2 层缩放;
支持写回DMA;
X26XX支持写回旋转 支持角度为 0°,90°,180°,270°的图像旋转;

显示特性:
支持 TFT(MIPI-DPI)
,SLCD(MIPI-DBI type A,B and C),MIPI-DSI;
## 2.驱动源码位置

驱动源码所在位置：

***module_drivers/drivers/video/fbdev/ingenic/fb_stage_wip***
使用时将module_drivers/drivers/video/fbdev/ingenic/Makefile中
-obj-$(CONFIG_FB_INGENIC_STAGE)  += fb_stage/改为
-obj-$(CONFIG_FB_INGENIC_STAGE)  += fb_stage_wip/

## 3.设备树配置

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
## 4.内核编译配置

### 4.1控制器驱动配置

```
CONFIG_FB_INGENIC_STAGE_WIP:                                                                                        
                                                                                                                    
Framebuffer support for the Version 12 DPU SoC.                                                                     
                                                                                                                    
Symbol: FB_INGENIC_STAGE_WIP [=y]                                                                                   
Type  : tristate                                                                                                    
Defined at module_drivers/drivers/video/fbdev/ingenic/fb_stage_wip/Kconfig:1                                        
  Prompt: Ingenic Framebuffer Driver for stage wip                                                                  
  Depends on: FB_INGENIC [=y]                                                                                       
  Location:                                                                                                         
    -> Ingenic device-drivers Configurations                                                                        
      -> [LCD] Panel/Touchscreen Drivers                                                                            
Selects: FB_INGENIC_DISPLAYS_STAGE_WIP [=y] && FB_CFB_FILLRECT [=y] && FB_CFB_COPYAREA [=y] && FB_CFB_IMAGEBLIT [=y]
```

### 4.2显示屏配置(STAGE_FW050)

可根据不同的屏幕型号选择不同的屏幕配置

```
CONFIG_STAGE_FW050:                                                                             
                                                                                                
lcd panel FW050, for ingenicfb drivers.                                                         
                                                                                                
Symbol: STAGE_FW050 [=y]                                                                        
Type  : tristate                                                                                
Defined at module_drivers/drivers/video/fbdev/ingenic/fb_stage/displays/Kconfig:39              
  Prompt: lcd panel FW050                                                                       
  Depends on: FB_INGENIC [=y] && FB_INGENIC_STAGE [=n] && FB_INGENIC_DISPLAYS_STAGE [=n]        
  Location:                                                                                     
    -> Ingenic device-drivers Configurations                                                    
      -> [LCD] Panel/Touchscreen Drivers                                                        
        -> Ingenic Framebuffer Driver for stage (FB_INGENIC_STAGE [=n])                         
          -> Supported lcd panels (FB_INGENIC_DISPLAYS_STAGE [=n])                              
Defined at module_drivers/drivers/video/fbdev/ingenic/fb_stage_wip/displays/Kconfig:39          
  Prompt: lcd panel FW050                                                                       
  Depends on: FB_INGENIC [=y] && FB_INGENIC_STAGE_WIP [=y] && FB_INGENIC_DISPLAYS_STAGE_WIP [=y]
  Location:                                                                                     
    -> Ingenic device-drivers Configurations                                                    
      -> [LCD] Panel/Touchscreen Drivers                                                        
        -> Ingenic Framebuffer Driver for stage wip (FB_INGENIC_STAGE_WIP [=y])                 
          -> display stage wip Supported lcd panels (FB_INGENIC_DISPLAYS_STAGE_WIP [=y])        
```



## 5.注意事项

(1).只有x26XX系列支持旋转

(2).在设备树配置中layer-framesize需要配置为显示屏最大显示区域确保有足够的空间可以使用。例如720p的竖屏，要配置为<720 1280>

(3).color_mode需要设置为0或1,src_size需要设置为和layer-framesize同等大小

(4).只支持两层缩放,layer2和layer3不支持NV12、NV21、YUV422格式

(5).出现裂屏的现象时，在设备树和内核编译配置中添加 buffer的数量，当二者配置不同时，选小的使用

在内核编译配置中选择为

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

设备树中通过修改ingenic,layer-frames来配置buffer数量

(6).X2000和X2500使用fw050屏只支持2lane

