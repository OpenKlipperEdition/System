LOCAL_PATH := $(my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := install_bsa_server_fp64
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_FS_BUILD)/usr/bin
BSA_SERVER_FILES := $(notdir $(wildcard $(LOCAL_PATH)/bsa_server))
LOCAL_COPY_FILES := $(BSA_SERVER_FILES)
include $(BUILD_MULTI_PREBUILT)

