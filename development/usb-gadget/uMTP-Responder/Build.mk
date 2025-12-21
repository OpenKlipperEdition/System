LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)



LOCAL_MODULE := umtprd
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/usr/sbin
LOCAL_SRC_FILES := $(subst $(LOCAL_PATH), , $(wildcard $(LOCAL_PATH)/src/*.c))	\
		$(subst $(LOCAL_PATH), , $(wildcard $(LOCAL_PATH)/src/mtp_operations/*.c))

LOCAL_C_INCLUDES:= inc
LOCAL_LDLIBS := -lc -lpthread
LOCAL_DEPANNER_MODULES := umtprd_conf

include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)
LOCAL_MODULE := umtprd_conf
LOCAL_MODULE_PATH := $(TARGET_FS_BUILD)
LOCAL_MODULE_TAGS := optional
LOCAL_COPY_FILES := /etc/umtprd/umtprd.conf:conf/umtprd.conf.ingenic

include $(BUILD_PREBUILT)
