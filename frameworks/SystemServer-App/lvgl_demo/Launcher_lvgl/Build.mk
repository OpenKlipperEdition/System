LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)
CMAKE_PATH=$(LOCAL_PATH)
LOCAL_MODULE:= osd_client_launcher_lvgl_widgets
LOCAL_MODULE_TAGS :=optional
LOCAL_DEPANNER_MODULES:=ingenic-systemServer ingenic_lvgl ingenic_lvgldrv ffplay-lib
CMAKE_CONF_OPTS := -DSYSROOT_DIR=$(TOP_DIR)/$(SYSROOT)
include $(BUILD_CMAKE_DEVICE)


