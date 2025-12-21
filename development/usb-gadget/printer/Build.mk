LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)

USB_TEST_DEVICE := usb_test/usb_gadget
LOCAL_MODULE := prn_example
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/$(USB_TEST_DEVICE)/$(LOCAL_MODULE)
LOCAL_SRC_FILES := prn_example.c
LOCAL_LDLIBS := -lc -lstdc++

include $(BUILD_EXECUTABLE)

