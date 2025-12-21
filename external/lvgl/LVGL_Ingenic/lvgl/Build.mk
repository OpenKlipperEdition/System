LOCAL_PATH := $(my-dir)

include $(CLEAR_VARS)
CMAKE_PATH=$(LOCAL_PATH)
LOCAL_MODULE:= ingenic_lvgl
LOCAL_MODULE_TAGS :=optional
LOCAL_DEPANNER_MODULES := cp_lv_conf_ingenic
CMAKE_CONF_OPTS := -DSYSROOT_DIR=$(TOP_DIR)/$(SYSROOT)
include $(BUILD_CMAKE_DEVICE)

include $(CLEAR_VARS)
LOCAL_MODULE =   cp_lv_conf_ingenic
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_INSTALL_PATH)/sysroot/usr/include/lvgl
LOCAL_COPY_FILES := lv_conf.h:lv_conf.h
include $(BUILD_MULTI_PREBUILT)


