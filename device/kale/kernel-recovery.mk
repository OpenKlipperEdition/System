
KERNEL_VER=$(strip $(TARGET_EXT_SUPPORT))
include $(CLEAR_VARS)

ifeq ($(strip $(TARGET_DEVICE_SUBVERSION)), v10)
ifeq ($(strip $(TARGET_STORAGE_MEDIUM)),nand)
KERNEL_RECOVERY_BUILD_CONFIG:=kale_v10_linux_sfc_nand_recovery_defconfig
endif # end nand
endif # end v10

ifeq ($(strip $(TARGET_DEVICE_SUBVERSION)), v20)
ifeq ($(strip $(TARGET_STORAGE_MEDIUM)),nand)
KERNEL_RECOVERY_BUILD_CONFIG:=kale_v20_linux_sfc_nand_recovery_defconfig
endif # end nand
endif # end v20

#KERNEL_RECOVERY_CONFIG_PATH:=$(DEVICE_PATH)/kernel_cfg/$(KERNEL_VER)

LOCAL_MODULE=kernel_recovery
KERNEL_PATH=$(TOP_DIR)/kernel/kernel-$(KERNEL_VER)
KERNEL_IMAGE_PATH:=arch/mips/boot/compressed/
KERNEL_RECOVERY_CONFIG_PATH:=$(KERNEL_PATH)/arch/mips/configs
KERNEL_CONFIG_PATH:=$(KERNEL_RECOVERY_CONFIG_PATH)
KERNEL_BUILD_CONFIG:=$(KERNEL_RECOVERY_BUILD_CONFIG)
KERNEL_TARGET_IMAGE:=xImage
LOCAL_MODULE_TAGS :=optional

include $(BUILD_KERNEL)


#################################################################################
SOURCE_CONFIG:=$(KERNEL_RECOVERY_CONFIG_PATH)/$(KERNEL_RECOVERY_BUILD_CONFIG)

#RECOVERY_RAMDISK:=$(BUILD_KERNEL_INTERMEDIATE)/ramdisk.cpio.gz
RECOVERY_RAMDISK:=$(TOP_DIR)/$(OUT_DEVICE_OBJ_DIR)/$(LOCAL_MODULE)-intermediate/ramdisk.cpio.gz

$(BUILD_KERNEL_INTERMEDIATE_DOT_CONFIG):LOCAL_RAMDISK:=$(RECOVERY_RAMDISK)
$(BUILD_KERNEL_INTERMEDIATE_DOT_CONFIG):$(RECOVERY_RAMDISK)
$(BUILD_KERNEL_INTERMEDIATE_DOT_CONFIG):$(SOURCE_CONFIG)
	mkdir -p $(dir $@)
	cat $< | sed 's#ramdisk.cpio.gz#$(LOCAL_RAMDISK)#' > $@

$(RECOVERY_RAMDISK):KERNEL_RECOVERY_ROOTFS:=$(TOP_DIR)/packages/updater
$(RECOVERY_RAMDISK):
	cd $(KERNEL_RECOVERY_ROOTFS);./mkramdisk.sh $@
