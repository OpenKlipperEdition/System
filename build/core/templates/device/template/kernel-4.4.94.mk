ifeq ($(strip $(TARGET_STORAGE_MEDIUM)),nor)
KERNEL_VER = $(strip $(TARGET_EXT_SUPPORT))
LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE = kernel
KERNEL_PATH = $(TOP_DIR)/kernel/kernel-$(KERNEL_VER)
KERNEL_CONFIG_PATH := $(KERNEL_PATH)/arch/mips/configs
LOCAL_MODULE_TAGS := optional
KERNEL_TARGET_IMAGE := uImage
KERNEL_IMAGE_PATH := arch/mips/boot/
KERNEL_BUILD_CONFIG := template_linux_defconfig
KERNEL_BOARD_OVERLAY_PATH := $(LOCAL_PATH)/kernel-overlay
KERNEL_BOARD_PATH:=$(LOCAL_PATH)/kernel-extern-kconfig

include $(BUILD_KERNEL)

# copy above to add a new kernel config.

endif # end nor
