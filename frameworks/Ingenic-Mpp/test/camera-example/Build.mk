LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := impp-camera-example
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := camera_example.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)



