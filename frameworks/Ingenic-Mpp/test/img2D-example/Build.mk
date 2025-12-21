LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)

ifeq ($(strip $(TARGET_BOARD_PLATFORM)), "x2500")

LOCAL_MODULE := csc-example
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:= $(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := csc_test.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

LOCAL_MODULE := cam-csc-example
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:= $(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := camera_csc_test.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

LOCAL_MODULE := cam-rot-example
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := cam_rotate_test.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

LOCAL_MODULE := dual-cam-rot-display-example
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := dual_cam_rot_display_test.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

LOCAL_MODULE := osd-example
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := osd_test.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

LOCAL_MODULE := cam-pic-osd-display-example
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := camera_pic_osd_display_test.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

LOCAL_MODULE := rotator-x2500-example
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := rotate_test_x2500.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

LOCAL_MODULE := camera-osd-enc-display-example
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := camera_osd_enc_display.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

LOCAL_MODULE := cam-pic-rot-csc-dpu-osd-example
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := camera_pic_rotater_csc_dpu_osd_test.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

LOCAL_MODULE := drawbox-example
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := drawbox_test.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)


endif

ifneq ($(findstring $(strip $(TARGET_BOARD_PLATFORM)), "m300" "x2000"),)
LOCAL_MODULE := rotator-x2000-example
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := rotate_test_x2000.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)
endif
