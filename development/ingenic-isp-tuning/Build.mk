LOCAL_PATH := $(my-dir)
#==================================================
include $(CLEAR_VARS)
LOCAL_MODULE=v4l2-isp-tuning
LOCAL_MODULE_TAGS:= optional

LOCAL_SRC_FILES:= main.c\
		 isp_tuning_api.c

LOCAL_LDLIBS := -lc

LOCAL_C_INCLUDES:= include

include $(BUILD_EXECUTABLE)
#===================================================
include $(CLEAR_VARS)
LOCAL_MODULE=v4l2-raw-tuning
LOCAL_MODULE_TAGS:= optional

LOCAL_SRC_FILES:= raw-tuning.c

LOCAL_LDLIBS := -lc

LOCAL_C_INCLUDES:= include

include $(BUILD_EXECUTABLE)
#===================================================
