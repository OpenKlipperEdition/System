ifeq ($(strip $(TARGET_STORAGE_MEDIUM)),nor)
LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)
UBOOT_BUILD_CONFIG := template
UBOOT_TARGET_FILE := u-boot-with-spl.bin
LOCAL_MODULE := uboot
UBOOT_PATH := $(TOP_DIR)/u-boot
UBOOT_BOARD_PATH := $(LOCAL_PATH)/uboot-overlay
LOCAL_MODULE_TAGS := optional

include $(BUILD_UBOOT)

# copy above to add a new uboot config.

endif # end nor

