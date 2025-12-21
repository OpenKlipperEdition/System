LOCAL_PATH := $(my-dir)

include $(CLEAR_VARS)
CMAKE_PATH=$(LOCAL_PATH)
LOCAL_MODULE:= raw_lvgldrv
LOCAL_MODULE_TAGS :=optional
LOCAL_DEPANNER_MODULES := cp_lvdrv_conf
# CMAKE_CONF_OPTS=-DTOPDIR=$(TOP_DIR) -DINSTALL_DIR=$(TARGET_INSTALL_PATH)
include $(BUILD_CMAKE_DEVICE)


include $(CLEAR_VARS)
LOCAL_MODULE =   cp_lvdrv_conf
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_INSTALL_PATH)/sysroot/usr/include/lvgl
LOCAL_COPY_FILES := lv_drv_conf.h:lv_drv_conf.h
include $(BUILD_MULTI_PREBUILT)



