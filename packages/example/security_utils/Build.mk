LOCAL_PATH := $(my-dir)

include $(CLEAR_VARS)
CMAKE_PATH=$(LOCAL_PATH)
LOCAL_MODULE:= security_test
LOCAL_MODULE_TAGS :=optional
include $(BUILD_CMAKE_DEVICE)
