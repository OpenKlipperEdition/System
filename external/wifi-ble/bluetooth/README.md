# 如何添加一个蓝牙固件安装？
1. 首先创建一个自己蓝牙对应的目录，我们以添加BCM4345C5（Broadcom公司生产的无线芯片，用于提供 Wi-Fi 和蓝牙功能）为例，我们将在`bluetooth_bcm`目录下创建了一个`bluetooth_bcm_4345c5`目录，如下所示：
```Shell
xxx@xxx:${PROJECT_TOP_DIR}/external/bluetooth/bluetooth_bcm$ ll
total 28
drwxr-xr-x 5 cwang sw 4096 Oct 26 11:34 ./
drwxr-xr-x 6 cwang sw 4096 Oct 26 11:39 ../
-rw-r--r-- 1 cwang sw 1221 Oct 16 17:32 Build.mk
-rw-r--r-- 1 cwang sw  644 Oct 26 11:33 Makefile
drwxr-xr-x 2 cwang sw 4096 Oct 26 11:34 bluetooth_bcm_43430a1/
drwxr-xr-x 2 cwang sw 4096 Oct 26 11:34 bluetooth_bcm_43430a1_AzureWave/
drwxr-xr-x 2 cwang sw 4096 Oct 26 11:34 bluetooth_bcm_4345c5/
cwang@user:~/work/x2000-submit/external/bluetooth/bluetooth_bcm$ 
```
2. 然后进入到创建的目录下，将需要拷贝安装的固件放到这个目录下，比如`bluetooth_bcm_4345c5`目录下存放了`BCM4345C5_003.006.006.0058.0135.hcd`固件文件，如下所示：
```shell
xxx@xxx:${PROJECT_TOP_DIR}/external/bluetooth/bluetooth_bcm/bluetooth_bcm_4345c5$ ll
total 64
drwxr-xr-x 2 cwang sw  4096 Oct 26 11:34 ./
drwxr-xr-x 5 cwang sw  4096 Oct 26 11:34 ../
-rw-r--r-- 1 cwang sw 50981 Oct 26 11:33 BCM4345C5_003.006.006.0058.0135.hcd
```
3. 创建`Build.mk`文件，参考BCM4345C5的`Build.mk`文件进行修改，如下：
```shell
LOCAL_PATH := $(my-dir)

include $(CLEAR_VARS)
# 编译模块名，根据自己的实际情况进行修改，之后需要将这个加入到自己的${device}.mk中
LOCAL_MODULE := install_bt_bcm4345c5_firmware
# 选择开发模式：userdebug、eng、optional。
LOCAL_MODULE_TAGS := optional
# 目标文件拷贝的路径（根据自己的需求进行修改，$(TARGET_FS_BUILD)定位到了根文件系统的根目录）
LOCAL_MODULE_PATH := $(TARGET_FS_BUILD)/firmware
BCM4345C5_FILES := $(notdir $(wildcard $(LOCAL_PATH)/BCM4345C5*))
# 要拷贝的本地文件
LOCAL_COPY_FILES := $(BCM4345C5_FILES)
include $(BUILD_MULTI_PREBUILT)
```
4. 如果说要拷贝的多个文件在多个目录下并且需要安装到不同的目录下，比如RTL8723DS芯片（Realtek公司生产的无线芯片，用于提供Wi-Fi和蓝牙功能）所需要安装的文件，我们可以使用CMake完成，具体可以参考`bluetooth_rtl8723ds`目录下的`Buil.mk`和`CMakeLists.txt`文件进行修改。
```shell
xxx@xxx:${PROJECT_TOP_DIR}/external/bluetooth/rtl_BlueZ/bluetooth_rtl8723ds$ tree
.
├── Build.mk
├── CMakeLists.txt
├── bin
│   ├── bluetoothctl
│   ├── bluetoothd
│   ├── hciconfig
│   └── rtk_hciattach
├── etc
│   └── dbus-1
│       └── system.d
│           └── bluetooth.conf
└── rtlbt
    ├── rtl8723d_config
    └── rtl8723d_fw

5 directories, 9 files
xxx@xxx:${PROJECT_TOP_DIR}/external/bluetooth/rtl_BlueZ/bluetooth_rtl8723ds$ cat Build.mk 
LOCAL_PATH := $(my-dir)

include $(CLEAR_VARS)
CMAKE_PATH = $(LOCAL_PATH)
# 编译模块名，根据自己的实际情况进行修改，之后需要将这个加入到自己的${device}.mk中
LOCAL_MODULE := install_bt_rtl8723ds_firmware
# 选择开发模式：userdebug、eng、optional。
LOCAL_MODULE_TAGS := optional
include $(BUILD_CMAKE_DEVICE)
xxx@xxx:${PROJECT_TOP_DIR}/external/bluetooth/rtl_BlueZ/bluetooth_rtl8723ds$ cat CMakeLists.txt 
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(install-bt-rtl8723ds-firmware)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/bluetooth_rtl8723ds/bin/ DESTINATION /usr/bin)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/bluetooth_rtl8723ds/rtlbt/ DESTINATION /firmware)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/bluetooth_rtl8723ds/etc/dbus-1/system.d/ DESTINATION /etc/dbus-1/system.d)
```
5. 完成上面的操作后我们就可以在自己的`${device}.mk`文件中添加相关的编译了，需要注意的是我们在安装固件时还需要安装相应的bsa_server，如果使用的是xburst架构则选择`install_bsa_server_fp32`或`install_bsa_server_AzureWave_fp32`编译模块名（的编译模块名），如果使用的是xburst2架构则选择`install_bsa_server_fp64`或`install_bsa_server_AzureWave_fp64`模块名（说明：模块名根据bluetooth bsa_server with AMPAK或bluetooth bsaserver with AzureWave进行相应的选择）。以X2000 Halley5为例（使用的是BCM4345C5芯片），我们可以在`${PROJECT_TOP_DIR}/device/halley5/halley5_base.mk`文件中添加如下内容：
```Shell
BLUETOOTH_FIRMWARE := install_bt_bcm4345c5_firmware
BSA_SERVER := install_bsa_server_fp64
PRODUCT_MODULES += $(BLUETOOTH_FIRMWARE) \
				   $(BSA_SERVER)
```
6. 编译文件系统时就会安装相应的bluetooth固件了。