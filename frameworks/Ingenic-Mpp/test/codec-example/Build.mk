LOCAL_PATH := $(my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := camera-h264enc
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := camera_h264enc_test_v1.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := dual-camera-h264enc
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := dual_camera_h264enc_test_v1.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := camera-jpegenc
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := camera_jpegenc_test.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)


ifeq ($(strip $(TARGET_BOARD_PLATFORM)), "x2500")
include $(CLEAR_VARS)
LOCAL_MODULE := camera-h265enc
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := camera_h265enc_test_v1.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := dual-camera-h264enc-h265enc
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := dual_camera_h264enc_h265enc.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := file-jpegenc-x2500-example
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := file_jpegenc_x2500_test.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)
endif

ifneq ($(findstring $(strip $(TARGET_BOARD_PLATFORM)), "m300" "x2000"),)
include $(CLEAR_VARS)
LOCAL_MODULE := file-jpegenc-x2000-example
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := file_jpegenc_x2000_test.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := h264dec-example
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := h264dec_test.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := h264dec-example-thread
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := h264dec_test_thread.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := h264dec-convert-tile420-to-nv12-x2000-example
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := h264dec_convert_tile420_to_nv12_test.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := h264dec-example-fb-memcpy
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := h264dec_test_fb_memcpy.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := jpegdec-example
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := jpegdec_test.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

endif

ifeq ($(findstring x26,$(TARGET_BOARD_PLATFORM)),x26)
include $(CLEAR_VARS)
LOCAL_MODULE := h264dec-example
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := h264dec_test.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := h264dec-example-thread
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := h264dec_test_thread.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)
LOCAL_MODULE := h264dec-example-fb-memcpy
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := h264dec_test_fb_memcpy.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := jpegdec-x2600-example
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := jpegdec_x2600_test.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := file-jpegenc-x2600-example
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := file_jpegenc_x2600_test.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

endif
