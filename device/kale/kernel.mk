KERNEL_VER=$(strip $(TARGET_EXT_SUPPORT))
include $(CLEAR_VARS)
LOCAL_MODULE=kernel
KERNEL_PATH=$(TOP_DIR)/kernel/kernel-$(KERNEL_VER)
#KERNEL_CONFIG_PATH:=$(DEVICE_PATH)/kernel_cfg/$(KERNEL_VER)
KERNEL_CONFIG_PATH:=$(KERNEL_PATH)/arch/mips/configs
LOCAL_MODULE_TAGS :=optional

KERNEL_TARGET_IMAGE:=uImage
KERNEL_IMAGE_PATH:=arch/mips/boot/


ifeq ($(strip $(TARGET_DEVICE_SUBVERSION)), v10)

ifeq ($(strip $(TARGET_EXT_SUPPORT)),4.4.94)
ifeq ($(strip $(TARGET_STORAGE_MEDIUM)),msc)
KERNEL_BUILD_CONFIG:=kale_v10_linux_msc_defconfig
endif # end msc

ifeq ($(strip $(TARGET_STORAGE_MEDIUM)),nor)
KERNEL_BUILD_CONFIG:=kale_v10_linux_sfc_nor_defconfig
endif # end nor

ifeq ($(strip $(TARGET_STORAGE_MEDIUM)),nand)
KERNEL_BUILD_CONFIG:=kale_v10_linux_sfc_nand_defconfig
endif # end nand
endif # end 4.4.94

ifeq ($(strip $(TARGET_EXT_SUPPORT)),5.10)
KERNEL_BUILD_CONFIG:=kale_v10_linux_defconfig
endif # end 5.10

endif # end subversion v10

ifeq ($(strip $(TARGET_DEVICE_SUBVERSION)), v20)

ifeq ($(strip $(TARGET_EXT_SUPPORT)),4.4.94)
ifeq ($(strip $(TARGET_STORAGE_MEDIUM)),msc)

ifeq ($(strip $(TARGET_EXT2_SUPPORT)),burn)
KERNEL_BUILD_CONFIG:=kale_v20_linux_msc_burn_defconfig
else
KERNEL_BUILD_CONFIG:=kale_v20_linux_msc_defconfig
endif

endif # end msc

ifeq ($(strip $(TARGET_STORAGE_MEDIUM)),nor)
KERNEL_BUILD_CONFIG:=kale_v20_linux_sfc_nor_defconfig
endif # end nor

ifeq ($(strip $(TARGET_STORAGE_MEDIUM)),nand)
KERNEL_BUILD_CONFIG:=kale_v20_linux_sfc_nand_defconfig
endif # end nand
endif # end 4.4.94

endif # end subversion v20

include $(BUILD_KERNEL)
