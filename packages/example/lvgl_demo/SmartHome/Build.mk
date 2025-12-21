LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)
CMAKE_PATH=$(LOCAL_PATH)
LOCAL_MODULE=LVGL_SmartHome
LOCAL_MODULE_TAGS :=optional
CMAKE_CONF_OPTS := -DSYSROOT_DIR=$(TOP_DIR)/$(SYSROOT)
include $(BUILD_CMAKE_DEVICE)
#####################################################
include $(CLEAR_VARS)
LOCAL_MODULE := LVGL_SmartHome_Release
LOCAL_MODULE_TAGS :=optional
LOCAL_MODULE_PATH := $(TARGET_FS_BUILD)/assets
LOCAL_MODULE_CLASS := DIR
LOCAL_MODULE_DIR := assets
LOCAL_DEPANNER_MODULES := ingenic_lvgl ingenic_lvgldrv LVGL_SmartHome
include $(BUILD_MULTI_COPY)
############################################
include $(CLEAR_VARS)

LOCAL_MODULE := LVGL_SmartHome_fs
LOCAL_MODULE_TAGS :=optional
LOCAL_MODULE_PATH :=$(TARGET_FS_BUILD)/etc
LOCAL_MODULE_CLASS := DIR
LOCAL_MODULE_DIR := ./runtime/etc
LOCAL_MODULE_OUT_DIR:= etc
LOCAL_DEPANNER_MODULES := LVGL_SmartHome_Release
include $(BUILD_MULTI_COPY)
