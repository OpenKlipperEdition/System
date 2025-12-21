LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := display-comp-example
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := sample_lcd_comp_test.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

LOCAL_MODULE := display-pic-example
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := sample_lcd_pic_test.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

LOCAL_MODULE := display-usrptr-example
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := sample_lcd_comp_usrptr_test.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

LOCAL_MODULE := display-comp-extend-example
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := simple_lcd_comp_test.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

LOCAL_MODULE := display-pic-extend-example
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := simple_lcd_pic_test.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

LOCAL_MODULE := display-comp-local-alpha-example
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := simple_lcd_comp_local_alpha_test.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

LOCAL_MODULE := dpu-osd-example
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := dpu_osd_test.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

LOCAL_MODULE := dpu-camera-example
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := dpu_camera_test.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)

LOCAL_MODULE := camera-pic-dpu-osd-switch-order-example
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-mpp
LOCAL_MODULE_PATH:=$(TARGET_FS_BUILD)/$(TARGET_TESTSUIT_DIR)/impp-example
LOCAL_C_INCLUDES :=$(TARGET_INSTALL_PATH)/sysroot/usr/include
LOCAL_SRC_FILES := camera_pic_dpu_osd_switch_order.c
LOCAL_LDLIBS := -lc -lm -ldl  -limpp -lpthread
include $(BUILD_EXECUTABLE)
