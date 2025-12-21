LOCAL_PATH := $(my-dir)

include $(CLEAR_VARS)
CMAKE_PATH=$(LOCAL_PATH)
LOCAL_MODULE:= ingenic_lvgldrv
LOCAL_MODULE_TAGS :=optional
LOCAL_DEPANNER_MODULES := cp_lvdrv_conf_ingenic
CMAKE_CONF_OPTS := -DSYSROOT_DIR=$(TOP_DIR)/$(SYSROOT)
CMAKE_CONF_OPTS := -DSYSROOT_DIR=$(TOP_DIR)/$(SYSROOT) -DCHIP_PLATFORM=$(TARGET_DEVICE)
include $(BUILD_CMAKE_DEVICE)


include $(CLEAR_VARS)
LOCAL_MODULE =   cp_lvdrv_conf_ingenic
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_INSTALL_PATH)/sysroot/usr/include/lvgl
LOCAL_COPY_FILES := lv_drv_conf.h:lv_drv_conf.h
include $(BUILD_MULTI_PREBUILT)



