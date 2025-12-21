ifeq ($(strip $(TARGET_STORAGE_MEDIUM)),msc)
# **** uboot ****
LOCAL_PATH :=$(my-dir)
include $(CLEAR_VARS)

ifneq ($(strip $(TARGET_EXT2_SUPPORT)),ota)
UBOOT_BUILD_CONFIG:=x2000_halley5_uImage_msc$(TARGET_BOARD_MSC_INDEX)
else
UBOOT_BUILD_CONFIG:=x2000_halley5_xImage_msc$(TARGET_BOARD_MSC_INDEX)_ota
endif # end ota

UBOOT_TARGET_FILE:=u-boot-with-spl-mbr-gpt.bin
LOCAL_MODULE := uboot
UBOOT_PATH := $(TOP_DIR)/u-boot
LOCAL_MODULE_TAGS := optional
include $(BUILD_UBOOT)

# copy above to add a new uboot config.

# **** end uboot ****
endif # end msc


ifeq ($(strip $(TARGET_STORAGE_MEDIUM)),nor)
# **** uboot ****
LOCAL_PATH :=$(my-dir)
include $(CLEAR_VARS)
UBOOT_BUILD_CONFIG:=x2000_halley5_uImage_sfc_nor
UBOOT_TARGET_FILE:=u-boot-with-spl.bin
LOCAL_MODULE := uboot
UBOOT_PATH := $(TOP_DIR)/u-boot
LOCAL_MODULE_TAGS := optional
include $(BUILD_UBOOT)

# copy above to add a new uboot config.

# **** end uboot ****
endif # end nor


ifeq ($(strip $(TARGET_STORAGE_MEDIUM)),nand)
# **** uboot ****
LOCAL_PATH :=$(my-dir)
include $(CLEAR_VARS)

ifneq ($(strip $(TARGET_EXT2_SUPPORT)),ota)
UBOOT_BUILD_CONFIG:=x2000_halley5_uImage_sfc_nand
else
UBOOT_BUILD_CONFIG:=x2000_halley5_xImage_sfc_nand_ota
endif # end ota

UBOOT_TARGET_FILE:=u-boot-with-spl.bin
LOCAL_MODULE := uboot
UBOOT_PATH := $(TOP_DIR)/u-boot
LOCAL_MODULE_TAGS := optional
$(info $(BUILD_UBOOT) 1234 ---- )
include $(BUILD_UBOOT)

# copy above to add a new uboot config.
#
# **** end uboot ****
endif # end nand
