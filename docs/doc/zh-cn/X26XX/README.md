# X26XX 文档仓库介绍

    本仓库文档可用于x2670系列，x2600系列芯片，可以在芯片启动、驱动开发、板级配置，应用开发等方面提供指导。


## X26XX 系列开发板介绍
    X26XX 系列芯片的标准开发板命名为halley7，开发板型号分别为:
    与不同的芯片组合成不同的开发板，如下：
- x2670M/N-halley7
- x2600M/N/E/H-halley7
  
  <span style="color:red;">注意：SDK中可能存在以 x2670-halley 的板级，实际对应开发板 x2670-halley7。</span>


## <span style="color:red;">X26XX 使用必读说明</span>
- [X26XX 使用特别注意](x2670-halley/00_X2600使用特别注意.md)

## X26XX-halley7 开发文档
- [快速开发指南](x2670-halley/01_快速开发指南.md)
- [uboot开发手册](x2670-halley/02_uboot开发手册.md)
- [内核开发手册](x2670-halley/03_内核开发手册.md)
- [应用开发手册](x2670-halley/04_应用开发手册.md)

## X26XX-halley7 应用笔记

<!--
- [快速启动方案指导](notes/01_快速启动方案指导.md)
-->

- [休眠唤醒低功耗调试指导](notes/02_休眠唤醒低功耗指导.md)
- [OTA 在线升级方案](../linux-app-notes/OTA升级方案/README.md)
<!-- - [OTA-Nand 在线升级方案](notes/03_OTA_NAND_在线升级方案.md)
- [OTA-eMMC 在线升级方案](notes/04_OTA_eMMC_在线升级方案.md)
- [OTA-Nor 在线升级方案](notes/05_OTA_Nor_在线升级方案.md) -->
- [模块化驱动指导](notes/06_模块化驱动指导.md)
- [预留内存配置说明文档](notes/07_预留内存配置说明文档.md)
- [PWM_SMC步进电机开发文档](notes/08_PWM_SMC步进电机开发手册.md)
- [LCD Logo 替换解决方案](notes/10_LCD_Logo替换解决方案.md)
- [如何适配一款新的LCD屏幕驱动](notes/11_如何配适一款新的LCD屏幕驱动.md)
- [如何连接WPA3加密网络](notes/13_如何连接WPA3加密网络.md)
<!--
- [如何适配一款新的Camera驱动]()
- [如何使用vscode调试应用程序]()
-->

- [如何使用Lvgl说明文档](notes/09_Lvgl_使用方法说明文档.md)

## X26XX-RISCV 开发手册
- [RISCV快速开发指南](01RISCV快速开发指南.md)
<!-- - [RISCV开发环境搭建](02RISCV开发环境搭建.md) -->
<!-- 
- [RISCV-Freertos应用程序开发](03RISCV_Freertos应用程序开发.md)
- [RISCV-Bare应用程序开发](04RISCV_Bare应用程序开发.md) -->
- [RISCV-RPMSG核间通信API的介绍和使用](05RISCV_RPMSG_核间通信API的介绍和使用.md)
- [X26XX-libbare Hal-Driver API 参考手册](http://ingenic-dev.gitee.io/libbare-doc-x26xx/)

## X26XX-PDMA-XBRUST0 MCU开发手册
- [PDMA XBURST0 MCU工程加载指南](01MCU工程加载指南.md)

## X26XX-halley7 FAQ
- [FAQ]()

## X26XX-halley SDK Release Notes
- [SDK-Kernel-5.10-V3.0-ReleaseNote](releasenotes/X2600-SDK-Kernel-5.10_v3.0-ReleaseNote.md)
