# Hash module driver interface

## Module Function Introduction

* Support hash algorithm acceleration, support SHA1 RFC-3174 standard and MD5 RFC-1321 standard
* Support SHA 160/224/256/384/512
* Support MD5 128bit

## Drive source code location

Location of driver source code:

***module_drivers/drivers/crypto/ingenic-hash.c***

## Device tree configuration

Location of device tree:

***module_driver/dts/x2600.dtsi***

Hash controller definition:

```c
hash: hash@0x13470000 {

    compatible = "ingenic,hash";

    reg = <0x13470000 0x10000>;

    interrupt-parent = <&core_intc>;

    interrupts = <IRQ_HASH>;

    status = "ok";

};
```

### Default configuration of device tree

The default compilation of device tree will produce hash devices.

### Device tree custom configuration

Users can close hash devices according to actual needs and configure this node as disabled.

## Kernel compilation configuration

The kernel driver compilation option CRYPTO_DEV_INGENIC_SHA

```
Symbol: CRYPTO_USER_API_HASH [=y]

Type : tristate

Prompt: User-space interface for hash algorithms

 Location:

 -> Cryptographic API (CRYPTO [=y])

 Defined at crypto/Kconfig:1607

 Depends on: CRYPTO [=y] && NET [=y]

 Selects: CRYPTO_HASH [=y] && CRYPTO_USER_API [=n]


Symbol: CRYPTO_DEV_INGENIC_SHA [=y]

Type : tristate

Prompt: Support for Ingenic SHA hw accelerator

 Location:

 -> Cryptographic API (CRYPTO [=y])

 -> Hardware crypto devices (CRYPTO_HW [=n])

Prompt: [SHA] Support for Ingenic SHA hw accelerator

 Location:

 -> Ingenic device-drivers Configurations

Defined at drivers/crypto/Kconfig:493

Depends on: CRYPTO [=y] && CRYPTO_HW [=n] && (MACH_XBURST [=y] || MACH_XBURST2 [=n])

 Selects: CRYPTO_ALGAPI [=y] && CRYPTO_ALGAPI [=y]
```

### Kernel custom compile configuration

Users can turn off this device according to their actual needs.

## Device Node Generation

```
/sys/bus/platform/drivers/hash
```

## Application Instructions

Reference application test program path:

***packages/example/security_utils/hash/***

```
├── CMakeLists.txt
├── hash.c
├── hash_openssl.c
├── include
│   ├── bignum.h
│   ├── hash.h
│   └── hash_openssl.h
└── main.c
```

Test method:

```
hash_test in_file out_file
```

in_file is the name of the data file to be encrypted, and the encrypted data will be stored in out_file.

Related test data:

***packages/example/security_utils/test_data/hash_test/***

```
├── hash_result.txt
└── hash_test_data.txt
```

* hash_result.txt: This data is result data
* hash_test_data.txt: This data is test data

```
# hash_test hash_test_data.txt hash_result.txt
```

The log after successful encryption:

```
hash crypt success!!
```
