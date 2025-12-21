# 如何添加一个WIFI固件安装？
1. 首先创建一个自己WIFI对应的目录，比如AP6256（一款集成了 Wi-Fi 和蓝牙功能的芯片模组）在`wifi_bcm`目录下创建了一个`wifi_bcm_ap6256`目录，如下所示：
```shell
xxx@xxx:${PROJECT_TOP_DIR}/external/wifi/wifi_bcm$ ll
total 36
drwxr-xr-x 8 cwang sw 4096 Oct 25 17:50 ./
drwxr-xr-x 7 cwang sw 4096 Oct 25 17:50 ../
-rw-r--r-- 1 cwang sw  221 Oct 25 17:50 README.md
drwxr-xr-x 2 cwang sw 4096 Oct 25 16:53 wifi_bcm_43362/
drwxr-xr-x 2 cwang sw 4096 Oct 25 16:53 wifi_bcm_43438/
drwxr-xr-x 2 cwang sw 4096 Oct 25 16:46 wifi_bcm_4345/
drwxr-xr-x 2 cwang sw 4096 Oct 25 16:53 wifi_bcm_ap6203/
drwxr-xr-x 2 cwang sw 4096 Oct 25 16:53 wifi_bcm_ap6256/
drwxr-xr-x 2 cwang sw 4096 Oct 25 16:46 wpa_bin/
```
2. 然后进入到创建的目录下，将需要拷贝的固件放到这个目录下，比如AP6256在这个目录下存放了`fw_bcm43456c5_ag.bin`（Broadcom BCM43456c5 芯片所需的固件文件，用于支持无线网络功能）和`nvram_ap6256.txt`（AP6256 芯片模组中的 NVRAM 文件，其中存储了一些无线网络参数和配置信息），如下：
```shell
xxx@xxx:${PROJECT_TOP_DIR}/external/wifi/wifi_bcm/wifi_bcm_ap6256$ ll
total 588
drwxr-xr-x 2 cwang sw   4096 Oct 25 16:53 ./
drwxr-xr-x 8 cwang sw   4096 Oct 25 17:50 ../
-rw-r--r-- 1 cwang sw 579388 Oct 25 16:46 fw_bcm43456c5_ag.bin
-rw-r--r-- 1 cwang sw   2440 Oct 25 16:46 nvram_ap6256.txt
```
3. 创建`Build.mk`文件，参考AP6256的`Build.mk`文件进行修改，如下：
```shell
LOCAL_PATH := $(my-dir)

include $(CLEAR_VARS)
CMAKE_PATH = $(LOCAL_PATH)
# 编译模块名，根据自己的实际情况进行修改，之后需要将这个加入到自己的${device}.mk中
LOCAL_MODULE := install_wifi_ap6256_firmware
# 选择开发模式：userdebug、eng、optional。
LOCAL_MODULE_TAGS := optional
# 这个DESTDIR变量之后会传递给CMake文件（$(TOP_DIR)/$(TARGET_FS_BUILD)表示的就是根文件系统的根目录）
CMAKE_CONF_OPTS := -DDESTDIR=$(TOP_DIR)/$(TARGET_FS_BUILD)
include $(BUILD_CMAKE_DEVICE)
```
4. 创建`CMakeLists.txt`文件，根据自己的需要进行安装拷贝固件文件，AP6256的如下：
```CMake
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

# 工程名/项目名，根据自己的实际情况进行修改
PROJECT(install-wifi-ap6256-firmware)

# WIFI_EXPORT_ENV这个变量表示用于生成配置WIFI环境变量脚本文件的脚本文件路径及文件名，给export_env.cmake使用
# 在这里我们需要找到export_env.sh脚本文件，这个需要根据实际情况进行修改路径
set(WIFI_EXPORT_ENV ${PROJECT_SOURCE_DIR}/../../export_env.sh)
# WIFI_ENV_FILE这个变量是在根文件系统下生成的配置环境变量的脚本文件的路径及文件名（不需要修改），给export_env.cmake使用
set(WIFI_ENV_FILE ${DESTDIR}/etc/profile.d/env_setup.sh)

# 下面是安装相关脚本文件及固件的命令
install(PROGRAMS ${PROJECT_SOURCE_DIR}/../wpa_bin/wifi_up.sh DESTINATION /bin/)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/../wpa_bin/wifi_down.sh DESTINATION /bin/)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/../wpa_bin/download_wifi_firmware.sh DESTINATION /bin/)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/../wpa_bin/S41network_firmware DESTINATION /etc/init.d/)

install(FILES ${PROJECT_SOURCE_DIR}/fw_bcm43456c5_ag.bin ${PROJECT_SOURCE_DIR}/nvram_ap6256.txt
		DESTINATION /firmware/)

# 包含export_env.cmake文件，路径需要根据实际情况进行修改，如果在CMakeLists.txt文件需要用到里面的变量则需要将其命令放到前面
include(${PROJECT_SOURCE_DIR}/../../export_env.cmake)
```
5. 创建完`Build.mk`和`CMakeLists.txt`文件之后，当前目录下的结构如下（以AP6256为例）：
```shell
xxx@xxx:${PROJECT_TOP_DIR}/external/wifi/wifi_bcm/wifi_bcm_ap6256$ ll
total 588
drwxr-xr-x 2 cwang sw   4096 Oct 25 16:53 ./
drwxr-xr-x 8 cwang sw   4096 Oct 25 17:50 ../
-rw-r--r-- 1 cwang sw    238 Oct 24 15:19 Build.mk
-rw-r--r-- 1 cwang sw    728 Oct 24 16:38 CMakeLists.txt
-rw-r--r-- 1 cwang sw 579388 Oct 25 16:46 fw_bcm43456c5_ag.bin
-rw-r--r-- 1 cwang sw   2440 Oct 25 16:46 nvram_ap6256.txt
```
6. 整个wifi固件添加完成之后，我们就可以在自己的`${device}.mk`文件中添加相关的编译了，以X2000 Halley5为例，我们可以在`${PROJECT_TOP_DIR}/device/halley5/halley5_base.mk`文件中添加如下内容：
```Shell
WIFI_AP_MODE := install_wifi_ap_mode
WIFI_FIRMWARE := install_wifi_ap6256_firmware
PRODUCT_MODULES += $(WIFI_AP_MODE) \
				   $(WIFI_FIRMWARE)
```
7. 编译文件系统时就会安装相应的wifi固件了。