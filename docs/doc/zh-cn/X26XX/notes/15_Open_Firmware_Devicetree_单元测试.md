# Open Firmware Devicetree 单元测试

## 内核配置

```
Symbol: OF_UNITTEST [=n]                                             
Type  : bool                                                         
Defined at drivers/of/Kconfig:15                                     
  Prompt: Device Tree runtime unit tests                             
  Depends on: OF [=y] && !SPARC                                      
  Location:                                                          
    -> Device Drivers                                                
      -> Device Tree and Open Firmware support (OF [=y])             
        -> Device Tree runtime unit tests (OF_UNITTEST [=n])                 
        -> Device Tree overlays (OF_OVERLAY [=n])
```

将这两个配置项选上

```
drivers/of/Makefile

-obj-$(CONFIG_OF_UNITTEST) += unittest-data/
+#obj-$(CONFIG_OF_UNITTEST) += unittest-data/


module_drivers/dts/Makefile

 obj-$(CONFIG_BUILTIN_DTB)	+= $(addsuffix .o, $(dtb-y))
+
+obj-$(CONFIG_OF_UNITTEST) += ../../drivers/of/unittest-data/
```

修改、编译后烧录

## 测试

kernel阶段看到如下打印，则表明开始进行设备树单元测试。

```
### dt-test ### start of unittest - you will see error messages
```

当输出如下打印时，说明测试结束。

```
### dt-test ### end of unittest - 311 passed, 0 failed
```





*注：由于原有目录结构改变，所有驱动都单独放在了module_drivers下。这种情况下单元测试的设备树补充文件（testcases.dtbo）的编译要早于设备树，此时kernel通过__dtb_start获取的设备树地址是补充文件的地址，无法用于初始化。如果要进行设备树单元测试要保证设备树补充文件在设备树编译后编译。*

**注意：此设备树单元测试仅为开发内核启用。测试将使用 TAINT_TEST 污染内核。测试将导致在控制台上打印 ERROR 和 WARNING 消息。测试将导致在控制台上打印堆栈跟踪。测试可能会使设备树处于损坏状态。如果不确定，请在此处选择 N。启用此选项不安全。**

