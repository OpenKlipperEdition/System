# X2600-SDK-Kernel-5.10\_v3.0-ReleaseNote

## SDK软件更新

1.编译工具链升级
    

      gcc version 7.2.0 (Ingenic MIPS LINUX Tools R5.2.1.sr03 Default\_xburst2\_glibc2.38 multilib 2023.11-30 12:45:27)

　　修复Y2038

2.Buildroot升级
    

      由2020.02升级为2023.08版本，部分软件包升级；修复Y2038

3.Kernel更新
    

　 1. 由kernel-5.10.21升级至kernel-5.10.186 ,修复部分cve漏洞

    2.修复系统休眠唤醒问题

    3.修复TCU模式切换异常问题

    4.修复已知的ISP处理异常的问题

4.Uboot更新
    

　 1.更新修复USB-Gadget设备驱动

     2.支持X2600的屏幕显示功能

     3.新增nandfash支持(GD5F4GM8UE/KANY1D4S2WD/F35SQB004G)

     4.修复部分nand设备的ECC错误

     5.修复一些已知的问题(USB/MSC/GPIO/LCD等)

5.软件库更新:

     Ingenic-MPP:

     1.优化display模块的显示效果

     2.完善camera模块的采集格式

     3.修复已知的图像编解码问题

     4.修正了图像格式的描述，以ffmpeg转换生成的为准

     Ingenic-Hw:

      此版本新增的应用软件库，便于用户空间对pwm/i2c/spi/gpio/wdt/tcu等外设进行访问

     LVGL:

      1.external/lvgl/LVGL\_RAW:LVGL原始代码未做优化的版本 

      2.external/lvgl/LVGL\_INGENIC:针对显示优化适配的版本，对显示要求高的推荐使用该版本

## SDK目录结构调整

|  目录  |   X2600 kernel-4.4.94 & kernel-5.10 Linux V2.0  |  X2600 kernel-5.10 Linux V3.0  |
| --- | --- | --- |
|  buildroot  |  buildroot版本为2020.02.x  |  1、buildroot升级为2023.08.3 2、wifi/ble固件移动到external目录下  |
|  development  |  Ingenic-Mpp/  |  usb-gadget/ usb-host/  |
|  device  |  板级编译配置文件  |  板级编译配置文件  |
|  docs  |  开发文档  |  该目录下文档不再更新，最新文档gitee： https://gitee.com/ingenic-dev/ingenic-linux-docs  |
|  external  |  android/ debugger/  |  android/ bluetooth\_demo/ debugger/ lvgl/ wifi-ble/  |
|  frameworks  |  ai/ reserveheap/  |  Ingenic-Hw/ Ingenic-Mpp/ Ingenic-SystemServer/ ai/ reserveheap/  |
|  Kernel  |  Kernel4.4.94&kernel5.10  |  仅包含kernel4.4.94或者kernel5.10  |
|  packages  |  example sample  |  example/cimutils/ example/grab/ example/lvgl\_demo/ example/sadc/ example/security\_utils/  |
|  prebuilts  |  sd-burntools/ toolchains/  |  toolchains/mips-gcc720-glibc238/  |
|  tools/  |  无  |  updater/  |