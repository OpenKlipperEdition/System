LOCAL_PATH := $(my-dir)

include $(CLEAR_VARS)
CMAKE_PATH = $(LOCAL_PATH)
LOCAL_MODULE := install_bt_rtl8723ds_firmware
LOCAL_MODULE_TAGS := optional
include $(BUILD_CMAKE_DEVICE)
