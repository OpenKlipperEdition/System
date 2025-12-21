LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)
CMAKE_PATH=$(LOCAL_PATH)
LOCAL_MODULE:= osd_client_lvgl_demo_widget
LOCAL_MODULE_TAGS :=optional
LOCAL_DEPANNER_MODULES:=ingenic-systemServer raw_lvgl raw_lvgldrv
include $(BUILD_CMAKE_DEVICE)
