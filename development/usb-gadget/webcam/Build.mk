LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := webcam_gadget
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/usr/sbin
LOCAL_SRC_FILES := uvc-gadget.c	\
		v4l2/v4l2.c	\
		v4l2enc/v4l2-jpegenc.c

LOCAL_C_INCLUDES:= include
LOCAL_LDLIBS := -lc -lstdc++

include $(BUILD_EXECUTABLE)

