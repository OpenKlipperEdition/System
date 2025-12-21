# Makefile define rules which make uboot.
#	Vars used by this Makefile,
#
#	UBOOT_BUILD_CONFIG: # define uboot config files
#	UBOOT_TARGET_FILE:  # eg:u-boot-with-spl.bin ?
#	UBOOT_PATH:   # e:$(TOPDIR)/uboot ??
#
#	support make:
#	make $(LOCALMODULE) # eg: make uboot
#	make $(LOCALMODULE)-clean # eg: make uboot-clean
#
#


# TODO: add ENV check and warning

ifeq ($(strip $(UBOOT_BUILD_CONFIG)),)
$(error *** unspecified UBOOT_BUILD_CONFIG: $(UBOOT_BUILD_CONFIG). ***)
endif

ifeq ($(strip $(UBOOT_TARGET_FILE)),)
$(error **** unspecified UBOOT_TARGET_FILE: $(UBOOT_TARGET_FILE). ***)
endif


ifneq ($(UBOOT_TARGET_FILE),spl/u-boot-spl.bin)
BUILD_UBOOT_TARGET_$(LOCAL_MODULE):=$(OUT_IMAGE_DIR)/$(LOCAL_MODULE)
else
BUILD_UBOOT_TARGET_$(LOCAL_MODULE):=$(TOP_DIR)/$(TARGET_FS_BUILD)/firmware/uboot-spl-noheader.bin
endif

BUILD_UBOOT_INTERMEDIATE:=$(TOP_DIR)/$(OUT_DEVICE_OBJ_DIR)/$(LOCAL_MODULE)-intermediate
BUILD_UBOOT_INTERMEDIATE_OBJ:=$(BUILD_UBOOT_INTERMEDIATE)/$(UBOOT_TARGET_FILE)

MAKE_JLEVEL ?= 1
UBOOT_PATH  ?= $(TOP_DIR)/u-boot

ifneq ($(UBOOT_TARGET_FILE),spl/u-boot-spl.bin)
$(BUILD_UBOOT_TARGET_$(LOCAL_MODULE)):$(BUILD_UBOOT_INTERMEDIATE_OBJ)
	cp -f $^ $@
else
$(BUILD_UBOOT_TARGET_$(LOCAL_MODULE)):$(BUILD_UBOOT_INTERMEDIATE_OBJ)
	mkdir -p $(TOP_DIR)/$(TARGET_FS_BUILD)/firmware
	dd if=$^ of=$@ skip=2048 bs=1
endif


ifeq ($(UBOOT_PATH)/Build.mk, $(wildcard $(UBOOT_PATH)/Build.mk))
$(error ======== u-boot/Build.mk found, remove it before going ========)
endif

define RELATIVE
$(subst $1/,,$2)
endef

define RELATIVE_TOP
$(eval pc=$(filter-out /, ,$(subst $(1)/,,$(2))))
$(eval pp:=)
$(foreach m,$(pc),$(eval pp=$(pp)../))
$(pp)
endef

UBOOT_BOARD_REL_PATH:=$(strip $(call RELATIVE,$(TOP_DIR),$(UBOOT_BOARD_PATH)))
UBOOT_REL_TOP_PATH:=$(strip $(call RELATIVE_TOP,$(TOP_DIR),$(UBOOT_PATH)))
UBOOT_BOARD_UBOOT_PATH:=$(UBOOT_REL_TOP_PATH)$(UBOOT_BOARD_REL_PATH)
CP_UBOOT_DIR :=$(subst ../, ,$(UBOOT_BOARD_UBOOT_PATH))
$(info $(CP_UBOOT_DIR),$(notdir $(UBOOT_BOARD_PATH)))

# localize VARS.
$(BUILD_UBOOT_INTERMEDIATE_OBJ):UBOOT_PATH:=$(UBOOT_PATH)
$(BUILD_UBOOT_INTERMEDIATE_OBJ):BUILD_UBOOT_INTERMEDIATE:=$(BUILD_UBOOT_INTERMEDIATE)
$(BUILD_UBOOT_INTERMEDIATE_OBJ):UBOOT_BUILD_CONFIG:=$(UBOOT_BUILD_CONFIG)
$(BUILD_UBOOT_INTERMEDIATE_OBJ):UBOOT_CROSS_COMPILE="CROSS_COMPILE=$(DEVICE_COMPILER_PREFIX)-"
ifneq ($(notdir $(UBOOT_BOARD_PATH)), )
UBOOT_COPY_DEPENDDR :=$(shell  cp -rf $(CP_UBOOT_DIR)/* $(UBOOT_PATH))
else
UBOOT_COPY_DEPENDDR :=
endif

$(BUILD_UBOOT_INTERMEDIATE_OBJ):$(UBOOT_COPY_DEPENDDR)
	make $(UBOOT_CROSS_COMPILE) -C $(UBOOT_PATH) distclean
	make $(UBOOT_CROSS_COMPILE) -C $(UBOOT_PATH) O=$(BUILD_UBOOT_INTERMEDIATE) $(UBOOT_BUILD_CONFIG) -j$(MAKE_JLEVEL)

LOCAL_MODULE_BUILD=$(LOCAL_MODULE)
include $(BUILD_SYSTEM)/module_install.mk
ifneq ($(LOCAL_FILTER_MODULE),)
ALL_MODULES += $(LOCAL_FILTER_MODULE)
ALL_BUILD_MODULES += $(BUILD_UBOOT_TARGET_$(LOCAL_MODULE))
endif

$(LOCAL_MODULE):$(BUILD_UBOOT_TARGET_$(LOCAL_MODULE))


$(LOCAL_MODULE)-clean:BBB0:=$(BUILD_UBOOT_TARGET_$(LOCAL_MODULE))
$(LOCAL_MODULE)-clean:BBB1:=$(BUILD_UBOOT_INTERMEDIATE)
$(LOCAL_MODULE)-clean:UBOOT_PATH:=$(UBOOT_PATH)
$(LOCAL_MODULE)-clean:
	make $(UBOOT_CROSS_COMPILE) -C $(UBOOT_PATH) distclean
	rm -rf $(BBB0)
	rm -rf $(BBB1)

.PHONY: $(BUILD_UBOOT_INTERMEDIATE_OBJ)

UBOOT_PATH:=
BUILD_UBOOT_INTERMEDIATE:=
UBOOT_BUILD_CONFIG:=
