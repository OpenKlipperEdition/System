LOCAL_PATH := $(my-dir)

include $(CLEAR_VARS)
CMAKE_PATH=$(LOCAL_PATH)
LOCAL_MODULE:= libsysutils
LOCAL_MODULE_TAGS :=optional
LOCAL_DEPANNER_MODULES += libicutils
include $(BUILD_CMAKE_DEVICE)

