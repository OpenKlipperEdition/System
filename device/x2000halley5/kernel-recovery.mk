
KERNEL_VER=$(strip $(TARGET_EXT_SUPPORT))
include $(CLEAR_VARS)

ifeq ($(strip $(TARGET_DEVICE_SUBVERSION)), v10)
ifeq ($(strip $(TARGET_STORAGE_MEDIUM)),nand)
KERNEL_RECOVERY_BUILD_CONFIG:=x2000_halley5_v10_linux_sfc_nand_recovery_gen_defconfig
endif # end nand
endif # end v10

ifeq ($(strip $(TARGET_DEVICE_SUBVERSION)), v20)
ifeq ($(strip $(TARGET_STORAGE_MEDIUM)),nand)
KERNEL_RECOVERY_BUILD_CONFIG:=x2000_halley5_v20_linux_sfc_nand_recovery_gen_defconfig
endif # end nand
endif # end v20

ifeq ($(strip $(TARGET_DEVICE_SUBVERSION)), v30)
ifeq ($(strip $(TARGET_STORAGE_MEDIUM)),nand)
KERNEL_RECOVERY_BUILD_CONFIG:=x2000_halley5_v30_linux_sfc_nand_recovery_gen_defconfig
endif # end nand
endif # end v20


ifeq ($(strip $(TARGET_DEVICE_SUBVERSION)), v30)
ifeq ($(strip $(TARGET_STORAGE_MEDIUM)),msc)
KERNEL_RECOVERY_BUILD_CONFIG:=x2000_halley5_v30_linux_mmc_recovery_gen_defconfig
endif # end mmc
endif # end v30

#KERNEL_RECOVERY_CONFIG_PATH:=$(DEVICE_PATH)/kernel_cfg/$(KERNEL_VER)

LOCAL_MODULE=kernel_recovery
RECOVERY_KERNEL_PATH := $(TOP_DIR)/kernel/kernel-$(KERNEL_VER)
KERNEL_PATH:=$(RECOVERY_KERNEL_PATH)
KERNEL_IMAGE_PATH:=arch/mips/boot/compressed/
KERNEL_RECOVERY_CONFIG_PATH:=$(KERNEL_PATH)/arch/mips/configs
KERNEL_CONFIG_PATH:=$(KERNEL_RECOVERY_CONFIG_PATH)
KERNEL_BUILD_CONFIG:=$(KERNEL_RECOVERY_BUILD_CONFIG)
KERNEL_TARGET_IMAGE:=xImage
LOCAL_MODULE_TAGS :=optional
NEED_INSTALL_MODULES = no

include $(BUILD_KERNEL)


#################################################################################
SOURCE_CONFIG:=$(KERNEL_RECOVERY_CONFIG_PATH)/$(KERNEL_RECOVERY_BUILD_CONFIG)

#RECOVERY_RAMDISK:=$(BUILD_KERNEL_INTERMEDIATE)/ramdisk.cpio.gz
RECOVERY_RAMDISK:=$(TOP_DIR)/$(OUT_DEVICE_OBJ_DIR)/$(LOCAL_MODULE)-intermediate/ramdisk.cpio.gz
ifneq ($(strip $(TARGET_STORAGE_MEDIUM)),msc)
RAMDISK_FILE_LIST_CONF:=$(DEVICE_PATH)/ota-overlay/ramdisk_config/ramdisk_$(TARGET_STORAGE_MEDIUM).conf
else
RAMDISK_FILE_LIST_CONF:=$(DEVICE_PATH)/ota-overlay/ramdisk_config/ramdisk_mmc.conf
endif

$(RECOVERY_KERNEL_PATH)/arch/mips/configs/$(KERNEL_DEFCONFIG):$(KERNEL_RECOVERY_CONFIG_PATH)/$(KERNEL_BUILD_CONFIG)
	cp $< $@
$(BUILD_KERNEL_INTERMEDIATE_DOT_CONFIG):$(RECOVERY_KERNEL_PATH)/arch/mips/configs/$(KERNEL_DEFCONFIG)
#$(BUILD_KERNEL_INTERMEDIATE_DOT_CONFIG):LOCAL_RAMDISK:=$(RECOVERY_RAMDISK)
#$(BUILD_KERNEL_INTERMEDIATE_DOT_CONFIG):$(RECOVERY_RAMDISK)
#$(BUILD_KERNEL_INTERMEDIATE_DOT_CONFIG):$(SOURCE_CONFIG)
#	mkdir -p $(dir $@)
#	cat $< | sed 's#ramdisk.cpio.gz#$(LOCAL_RAMDISK)#' > $@
#	cp $< $(KERNEL_RECOVERY_CONFIG_PATH)

$(RECOVERY_RAMDISK):KERNEL_RECOVERY_ROOTFS:=$(TOP_DIR)/$(OUT_DEVICE_OBJ_DIR)/$(LOCAL_MODULE)-intermediate/ramdisk
$(RECOVERY_RAMDISK):
	mkdir -p $(dir $@)
	mkdir -p $(KERNEL_RECOVERY_ROOTFS)
	if [ "$(TARGET_STORAGE_MEDIUM)" == "msc" ] ; then mkdir -p $(KERNEL_RECOVERY_ROOTFS)/etc/ota_res/ ; cp $(DEVICE_PATH)/ota-overlay/package_config/partition_mmc.conf $(KERNEL_RECOVERY_ROOTFS)/etc/ota_res/ ;fi
	cd -P $(TOP_DIR)/$(TARGET_FS_BUILD) ; tar -cpf - --exclude="*.git" --exclude="*.o" --exclude="*.o.cmd" --exclude="*.a" $(shell cat $(RAMDISK_FILE_LIST_CONF) | grep -v '#') | tar -xpf - -C $(KERNEL_RECOVERY_ROOTFS)
	cd -P $(KERNEL_RECOVERY_ROOTFS)/etc/profile.d/ ; echo "echo start update..." > update_startup.sh ; echo "cp /usr/data/wpa_supplicant.conf /etc/" >> update_startup.sh; echo "/usr/data/ota_res/update.sh &" >> update_startup.sh ; chmod 777 update_startup.sh
	cd $(KERNEL_RECOVERY_ROOTFS) ; echo "mknod dev/console c 5 1 ; mknod dev/null c 1 3 ; find . | cpio -H newc -o | gzip -n > $@" | fakeroot
ifeq ($(strip $(TARGET_STORAGE_MEDIUM)),nand)
ifeq ($(strip $(TARGET_DEVICE_SUBVERSION)), v10)
KERNEL_RECOVERY_BUILD_SRC_CONFIG:=x2000_halley5_v10_linux_sfc_nand_recovery_defconfig
endif
ifeq ($(strip $(TARGET_DEVICE_SUBVERSION)), v20)
KERNEL_RECOVERY_BUILD_SRC_CONFIG:=x2000_halley5_v20_linux_sfc_nand_recovery_defconfig
endif
ifeq ($(strip $(TARGET_DEVICE_SUBVERSION)), v30)
KERNEL_RECOVERY_BUILD_SRC_CONFIG:=x2000_halley5_v30_linux_sfc_nand_recovery_defconfig
endif
endif # end nand

ifeq ($(strip $(TARGET_STORAGE_MEDIUM)),msc)
ifeq ($(strip $(TARGET_DEVICE_SUBVERSION)), v30)
KERNEL_RECOVERY_BUILD_SRC_CONFIG:=x2000_halley5_v30_linux_mmc_recovery_defconfig
endif
endif	# end msc


$(KERNEL_RECOVERY_CONFIG_PATH)/$(KERNEL_RECOVERY_BUILD_CONFIG):$(RECOVERY_RAMDISK)
$(KERNEL_RECOVERY_CONFIG_PATH)/$(KERNEL_RECOVERY_BUILD_CONFIG):LOCAL_RAMDISK:=$(RECOVERY_RAMDISK)
$(KERNEL_RECOVERY_CONFIG_PATH)/$(KERNEL_RECOVERY_BUILD_CONFIG):$(KERNEL_RECOVERY_CONFIG_PATH)/$(KERNEL_RECOVERY_BUILD_SRC_CONFIG)
	cat $< | sed 's#ramdisk.cpio.gz#$(LOCAL_RAMDISK)#' > $@

