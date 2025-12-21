# DTRNG模块驱动接口

## 模块功能介绍

真随机数发生器，用于在加解密场景提供真随机数的功能。

## 驱动源码位置

驱动源码所在位置：

***module_drivers/drivers/char/hw_random/ingenic-rng.c***

## 设备树配置

kernel内核dts文件路径：

kernel内核(version <= 5.10)dts文件路径：

***module_driver/dts/x2600.dtsi***

kernel内核(version > 5.10)dts文件路径：

***module_driver/dts/x2600/x2600.dtsi***

设备树描述：

```c
dtrng: dtrng@0x10072000 {

    compatible = "ingenic,dtrng";

    reg = <0x10072000 0x100>;

    interrupt-parent = <&core_intc>;

    interrupts = <IRQ_DTRNG>;

    status = "disabled";

};
```

### 设备树默认配置

默认编译会产生dtrng设备。

```c
&dtrng {

 status = "okay";

};
```

### 设备树自定义配置

如果客户需要关闭dtrng设备，可以将以上节点配置为disabled。

```c
&dtrng {

 status = "disabled";

};
```

## 内核编译配置

内核配置HW_RANDOM_INGENIC，配置说明如下：

```
Symbol: HW_RANDOM_INGENIC [=y] 

Type : tristate 

Prompt: Ingenic HW Random Number Generator support 

 Location: 

 -> Device Drivers 

 -> Character devices 

 -> Hardware Random Number Generator Core support (HW_RANDOM [=y])

Prompt: [RANDOM] Ingenic HW Random Number Generator support 

 Location: 

 -> Ingenic device-drivers Configurations 

 -> [RANDOM] drivers 

 Defined at drivers/char/hw_random/Kconfig:384 

 Depends on: HW_RANDOM [=y] && MIPS [=y]
```

### 内核默认编译配置

内核默认配置dtrng驱动，配置界面如下：

![](assets/DTRNG模块驱动接口.0.png)

### 内核自定义编译配置

客户可以根据实际情况，去掉该驱动的配置。

## 设备节点生成

当驱动加载成功后，会生成/dev/hwrng, 用户可以根据此节点判断驱动是否成功加载。

## 应用程序使用说明

测试应用程序路径：

**packages/example/security_utils/dtrng/**

```
├── app_dtrng.c
└── CMakeLists.txt
```

测试方法：

```
dtrng_test out_file
```

out_file为指定生成的随机数文件，输出的随机数数据将存于out_file文件。

测试结果：

```
# ./dtrng_test out_file

random_num register value 0x49f794bc

# ./dtrng_test out_file

random_num register value 0xcaa60aaa

# ./dtrng_test out_file

random_num register value 0x0c6a7eba

# ./dtrng_test out_file

random_num register value 0xd208541c

# ./dtrng_test out_file

random_num register value 0x63f8e65b

# ./dtrng_test out_file

random_num register value 0xaf77fdbe

# cat out_file 

��w�^x_x0019_��t��r��������@�I3�6��"�D# 
```

