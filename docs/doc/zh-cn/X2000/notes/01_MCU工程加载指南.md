# 内核配置
## 驱动位置

module_drivers/drivers/remoteproc/mcu_remoteproc

## 设备树配置

### 备树所在位置：
```c
module_drivers/dts/x2000.dtsi
```
### mcu描述
```c
 mcu: mcu@0x13420000 {
     compatible = "ingenic,x2000-mcu";
     reg = <0x13420000 0x10000>;
     interrupt-parent = <&core_intc>;
     interrupt-names = "pdmam";
     interrupts = <IRQ_PDMAM>;
     ingenic,tcsm_size = <16384>;
 };

```
### 设备树默认配置
```c
在板级设备树中添加
&mcu {
    status = "okay";
};
```

## 内核编译配置
```c
Symbol: INGENIC_RPROC [=y]
Type  : tristate
Defined at module_drivers/drivers/remoteproc/Kconfig:1
  Prompt: ingenic remoteproc support
  Depends on: HAS_DMA [=y]
  Location:
    -> Ingenic device-drivers Configurations
(1)   -> [Remoteproc] drivers
Selects: REMOTEPROC [=y]
```
# 小核配置
## 小核SDK
- 小核SDK名称: libbare-cpu

- 小核SDK下载地址：
https://gitee.com/ingenic-dev/libbare-cpu

## 小核工程编译

以template-mcu为例，提供以下三种编译方式，首先配置交叉编译工具链到环境变量，然后进行编译．

### 基于Makefile编译 
```c
$ make
```
会在build目录生成template.elf, template.bin文件.

### 基于cmake构建 - （推荐）
```c
$ mkdir build
$ cd build
$ cmake -DCMAKE_TOOLCHAIN_FILE=../mips-gcc-sde-elf.cmake ..
$ make
```
会在build目录下生成template.elf，template.bin文件.

### 基于vscode集成开发环境
```c
前提：确认cmake 版本 cmake-v3.23.1 ，可以使用最新的cmake版本。 注意：不要使用工具链的cmake。否则vscode 会报错。

流程如下：

1. vscode 打开工程
2. 选择cmake kits：cmake-kits "GCC for ingenic cross compile on Linux"
3. lunch(F5) 运行编译、调试
4. 选择状态栏,build， 进行代码编译.

```
## 加载和运行程序
```
1. 将bin文件放置在开发板/lib/firmware目录下(该目录需要手动创建);
2. 将bin文件更名为libmcu-bare.bin;
3. 执行以下命令进行加载
    echo 1 > /sys/devices/platform/ahb2/13420000.mcu/load_fw
```
