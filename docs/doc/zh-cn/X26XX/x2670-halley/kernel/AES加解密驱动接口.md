# AES 加解密驱动接口

## 模块功能介绍

* aes对称加解密硬件模块
* Rijdael_fips_197 标准
* 支持128/192/256 CBC/ECB 加解密

## 驱动源码位置

内核驱动所在路径：

```c
 module_drivers/drivers/crypto/ingenic-aes.c
```

## 设备树配置

设备树所在位置：

kernel内核(version <= 5.10)dts文件路径：

***module_driver/dts/x2600.dtsi***

kernel内核(version > 5.10)dts文件路径：

***module_driver/dts/x2600/x2600.dtsi***

AES控制器定义：

```c
aes: aes@0x13430000 {
    compatible = "ingenic,aes";
    reg = <0x13430000 0x10000>;
    interrupt-parent = <&core_intc>;
    interrupts = <IRQ\_AES>;
    status = "ok";

};
```

### 设备树默认配置

设备树默认编译会产生AES设备。

### 设备树自定义配置

用户可根据实际需求关闭AES设备，在板级.dts中将该节点配置为disabled。

```c
&aes {
    status = "disabled";
};  
```

## 内核编译配置

编译选项: CRYPTO_DEV_INGENIC_AES，配置说明如下：

```c
Symbol: CRYPTO_USER_API_SKCIPHER [=n]
Type  : tristate
Prompt: User-space interface for symmetric key cipher algorithms
  Location:
  -> Cryptographic API (CRYPTO [=y])
   Defined at crypto/Kconfig:1616
   Depends on: CRYPTO [=y] && NET [=y] 
   Selects: CRYPTO_BLKCIPHER [=y] && CRYPTO_USER_API [=y]
Symbol: CRYPTO_DEV_INGENIC_AES [=n] 
Type : tristate 
Prompt: Support for INGENIC AES hw engine 
 Location: 
 -> Cryptographic API (CRYPTO [=y]) 
 -> Hardware crypto devices (CRYPTO_HW [=n]) 
Prompt: [AES] Support for INGENIC AES hw engine 
 Location: 
 -> Ingenic device-drivers Configurations 
 Defined at drivers/crypto/Kconfig:311 
 Depends on: CRYPTO [=y] && CRYPTO_HW [=n] && (MACH_XBURST [=y] || MACH_XBURST2 [=n]) 
 Selects: CRYPTO_AES [=y] && CRYPTO_BLKCIPHER2 [=y] && CRYPTO_AES [=y] && CRYPTO_BLKCIPHER2 [=y]
```

### 内核默认编译配置

内核默认未配置AES驱动。

### 内核自定义编译配置

用户可根据实际需求配置AES驱动，配置界面如下：

![](assets/AES加解密驱动接口.0.png)

## 设备节点生成

驱动加载成功后生成以下节点：

/sys/bus/platform/drivers/aes

## 应用程序使用说明

测试应用程序路径：

packages/example/security_utils/aes/

```
├── aes.c
├── CMakeLists.txt
├── gen_key_base.c
├── include
│   ├── aes.h
│   ├── bignum.h
│   ├── gen_key_base.h
│   └── keys.h
└── main.c
```

测试方法:

aes_test in_file out_file key_file op(en:1 de:0)

* int_file：加/解密源数据
* out_file：结果数据
* key_file：密钥
* op 1加密、0解密

相关测试数据：

packages/example/security_utils/test_data/aes_test/

```
├── aes_test_data.txt
├── aes_test_en.txt
└── key.txt
```

* key.txt：key数据
* aes_test_en.txt：该数据为结果数据
* aes_test_data.txt：该数据为待测试数据

使用示例：
```
# aes_test aes_test_data.txt encrypt.txt key.txt 1 
# aes_test encrypt.txt decrypt.txt key.txt 0
# cat aes_text_data.txt
123456789011111
# cat decrypt.txt
123456789011111
```
