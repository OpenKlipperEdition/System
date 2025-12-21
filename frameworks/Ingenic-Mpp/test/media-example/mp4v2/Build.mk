LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := camera-mp4-recoder
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include ./include/mp4v2 ./include
LOCAL_SRC_FILES := mp4Encoder.cpp  camera_mp4.cpp
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread -lmp4v2 -lstdc++
include $(BUILD_EXECUTABLE)
