LOCAL_PATH := $(my-dir)

PWM_TEST_DIR=pwm_smc_example
include $(CLEAR_VARS)
LOCAL_MODULE=pwm_smc_example
LOCAL_MODULE_TAGS:=optional
LOCAL_SRC_FILES:= example.c pwm_smc.c

LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/$(PWM_TEST_DIR)
LOCAL_CFLAGS := -std=gnu99 -Wall -O0
LOCAL_LDLIBS := -lc -lm -lpthread
include $(BUILD_EXECUTABLE)

