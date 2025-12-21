# ingenic-docs

该文档仓库主要维护君正各芯片平台开发文档和应用笔记等。
- 基于Linux开发平台各类常见文档，包含快速上手指南、uboot、kernel、rootfs、应用程序编写等常见指导。
- 部分芯片平台提供的基于 libbare 裸机操作系统的开发文档。


## Linux平台开发文档

- [X16XX](zh-cn/X16XX/README.md)
- [X2000](zh-cn/X2000/README.md)
- [X2500](zh-cn/X2500/README.md)
- [X26XX](zh-cn/X26XX/README.md)
  
## Linux 通用应用指南

理论适应于所有的Linux平台，与具体芯片平台关联程度不大，可用于通用配置参考。
- [基于RNDIS的DHCP服务器搭建方法](zh-cn/linux-app-notes/基于RNDIS的DHCP服务器搭建方法.md)
- [USB信号质量测试进入测试模式方法](zh-cn/linux-app-notes/USB信号质量测试.md)
- [CAT1 4G模块Linux适配说明](zh-cn/linux-app-notes/合宙Air780E_linux平台适配.md)
- [Linux应用内存泄露调试方法](zh-cn/Debug-tools/内存检测工具.md)
- [OTA 在线升级方案](zh-cn/linux-app-notes/OTA升级方案/README.md)

**与芯片平台相关的应用笔记请参考各芯片平台子目录文档**


### IMPP-Linux媒体库
最新源码仓库链接: 
- [ingenic-impp 源码](https://gitee.com/ingenic-dev/ingenic-impp)
- [IMPP-Linux 开发文档](zh-cn/IMPP-Linux/README.md)

## uboot 通用应用指南
- [uboot cdc acm 通信示例](zh-cn/uboot-app-notes/USB_CDC通讯类实现.md)

## 裸系统平台 Libbare Hal Drivers
类CMSIS Hal Drivers，主要面向无操作系统环境，适合控制类应用开发。

- [libbare-cpu](https://gitee.com/ingenic-dev/libbare-cpu)


## 文档编写规范
- [文档编写规范](文档编写规范.md)
