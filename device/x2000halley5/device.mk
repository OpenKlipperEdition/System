PRODUCT:=product
TARGET_PRODUCT_BOARD:=$(TARGET_DEVICE)
TARGET_BOARD_PLATFORM:="x2000"
TARGET_BOARD_ARCH:="mips"
TARGET_BOARD_MSC_INDEX:=2

# For xburst2
TARGET_ARCH_VARIANT:=mips32r2_fp64
# For xburst
#TARGET_ARCH_VARIANT:=mips32r2

LOCAL_PATH := $(shell dirname $(lastword $(MAKEFILE_LIST)))
DEVICE_PATH := $(TOP_DIR)/$(LOCAL_PATH)

MAKE_JLEVEL := 4


#rules to make uboot&kernel&rootfs according to storage medium
#
# Supported FILE_SYSTEM_TYPE:
# ['ubifs', 'jiffs2', 'ext2', "yaffs2", "cramfs"]
#

ifeq ($(strip $(TARGET_STORAGE_MEDIUM)),nor)

ifeq ($(strip $(COMPILER_LIB_VERSION)),12.1.0)
BUILDROOT_CONFIG:=x2000_halley5_nor_gcc1210_defconfig
else
BUILDROOT_CONFIG:=x2000_halley5_nor_defconfig
endif #end 12.1.0
#BUILDROOT_CONFIG_PATH:=$(DEVICE_PATH)
BUILDROOT_PATH:=buildroot
ROOTFS_OVERLAY_DIR:=$(DEVICE_PATH)/rootfs-overlay
FILE_SYSTEM_TYPE:=jffs2

endif #end nor


ifeq ($(strip $(TARGET_STORAGE_MEDIUM)),nand)

ROOTFS_OVERLAY_DIR:=$(DEVICE_PATH)/rootfs-overlay

ifeq ($(strip $(COMPILER_LIB_VERSION)),12.1.0)
BUILDROOT_CONFIG:=x2000_halley5_linux_gcc1210_defconfig
else
BUILDROOT_CONFIG:=x2000_halley5_linux_defconfig
endif #end 12.1.0

ifeq ($(strip $(TARGET_EXT2_SUPPORT)),ota)
ROOTFS_OVERLAY_DIR+=$(DEVICE_PATH)/ota-overlay/rootfs_nand_overlay
PACKAGE_CONFIG:=$(LOCAL_PATH)/ota-overlay/package_config
endif #end ota

BUILDROOT_PATH:=buildroot
FILE_SYSTEM_TYPE:=ubifs

endif #end nand

ifeq ($(strip $(TARGET_STORAGE_MEDIUM)),msc)

ROOTFS_OVERLAY_DIR:=$(DEVICE_PATH)/rootfs-overlay

ifeq ($(strip $(TARGET_EXT2_SUPPORT)),burn)
BUILDROOT_CONFIG:=x2000_halley5_msc_burn_defconfig
else
ifeq ($(strip $(COMPILER_LIB_VERSION)),12.1.0)
BUILDROOT_CONFIG:=x2000_halley5_linux_gcc1210_defconfig
else
BUILDROOT_CONFIG:=x2000_halley5_linux_defconfig
endif #end 12.1.0
endif #end burn

ifeq ($(strip $(TARGET_EXT2_SUPPORT)),ota)
ROOTFS_OVERLAY_DIR+=$(DEVICE_PATH)/ota-overlay/rootfs_mmc_overlay
PACKAGE_CONFIG:=$(LOCAL_PATH)/ota-overlay/package_config
SYSTEM_BIN?=$(OUT_IMAGE_DIR)/system.ext2
DEVICE_TYPE:=mmc
SINGLE_PKG=yes
endif #end ota

BUILDROOT_PATH:=buildroot
FILE_SYSTEM_TYPE:=ext2

endif #end msc

