LOCAL_PATH := $(my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := ai_test
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := ai_test.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := ao_test
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := ao_test.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := ai_aenc_test
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := ai_aenc_test.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := ao_adec_test
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := ao_adec_test.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp
include $(BUILD_EXECUTABLE)

# ifeq ($(findstring $(strip $(TARGET_BOARD_PLATFORM)), "m300" "x2000"),)
# include $(CLEAR_VARS)
# LOCAL_MODULE := aec_test
# LOCAL_MODULE_TAGS := optional
# LOCAL_DEPANNER_MODULES := ingenic-mpp
# LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
# LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
# LOCAL_SRC_FILES := aec_test.c
# LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
# include $(BUILD_EXECUTABLE)
# else
# include $(CLEAR_VARS)
# LOCAL_MODULE := aec_test_x2000
# LOCAL_MODULE_TAGS := optional
# LOCAL_DEPANNER_MODULES := ingenic-mpp
# LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
# LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
# LOCAL_SRC_FILES := aec_test_x2000.c
# LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
# include $(BUILD_EXECUTABLE)
# endif

include $(CLEAR_VARS)
LOCAL_MODULE := resampler_test
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := resampler_test.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp
include $(BUILD_EXECUTABLE)

