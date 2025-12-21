deps := \
	$(systemimage) \
	$(ALL_MODULES) \
	$(ALL_PREBUILT_MODULES)

sdk_dir := $(OUT_HOST_DIR)/sdk
sdk_name := manhattan-sdk_$(USER)
ifeq ($(HOST_OS),darwin)
	INTERNAL_SDK_HOST_OS_NAME := mac
else
	INTERNAL_SDK_HOST_OS_NAME := $(HOST_OS)
endif
ifneq ($(filter win_sdk,$(MAKECMDGOALS)),)
	INTERNAL_SDK_HOST_OS_NAME := windows
endif
	sdk_name := $(sdk_name)_$(INTERNAL_SDK_HOST_OS_NAME)


INSTALL_SDK_TARGET := $(sdk_dir)/$(sdk_name).zip
$(INSTALL_SDK_TARGET): PRIVATE_NAME := $(sdk_name)
$(INSTALL_SDK_TARGET): PRIVATE_DIR :=$(sdk_dir)/$(sdk_name)

ALL_BUILD_SAMPLE_PATH = $(foreach m,$(ALL_MODULE_PATH), \
	$(if $(findstring packages/example/Sample,$(m)), \
		$(m),) \
)

SOURCES_PATH := $(TOPDIR)/development/source

DOC_FILES := $(TOPDIR)/docs/*

ifneq ($(filter win_sdk,$(MAKECMDGOALS)),)
WIN_CROSS_TOOLCHAINS := $(TOPDIR)/prebuilts/toolchains/mips-gcc-glibc-mingw32
WIN_HOST_TOOLS := $(TOPDIR)/prebuilts/win-tools
WIN_SDK_BUILD := $(TOPDIR)/build/core/tools/sdk-build.cmd

WIN_SDK_BUILD: $(shell $(BUILD_SYSTEM)/tools/shell_to_cmd.sh)

deps += $(WIN_SDK_BUILD)
endif

define mv-sdk-files
@echo $(1) "->" $(2)
mkdir -p $(dir $(2))
$(hide) mv $(1) $(2)
endef

# define copy-sdk-files
# @echo $(1) "->" $(2)
# mkdir -p $(dir $(2))
# $(hide) cp -daf $(1) $(2)
# endef


define copy-sdk-files-nocheck
@echo $(1) "->" $(2)
mkdir -p $(dir $(2)); \
 cp -daf $(1) $(2);
endef

define copy-sdk-files
@echo $(1) "->" $(2)
if [ -d $(1) -o -f $(1) ];then mkdir -p $(dir $(2)); \
 cp -daf $(1) $(2); else echo "ignore $(1)"; fi
endef

$(INSTALL_SDK_TARGET): $(deps) doc
	@echo "\033[32m\033[1mPackage SDK: $@ \033[0m "
	$(hide) rm -rf $(PRIVATE_DIR) $@
	$(call copy-sdk-files,$(OUT_DEVICE_INCLUDE_DIR),$(PRIVATE_DIR)/platform/usr/include)
	$(call copy-sdk-files,$(OUT_DEVICE_ANDROID_INCLUDE_DIR),$(PRIVATE_DIR)/platform/usr/android/include)
ifeq (x$(strip $(OTA)),x)
	$(call copy-sdk-files,$(OUT_DEVICE_SHARED_DIR),$(PRIVATE_DIR)/platform/usr/lib)
	$(call copy-sdk-files,$(TARGET_FS_BUILD)/lib,$(PRIVATE_DIR)/platform/lib)
	$(call copy-sdk-files,$(TARGET_FS_BUILD)/lib,$(PRIVATE_DIR)/platform/lib)
ifeq (x$(strip $(BUILD_TARGET_OS_TYPE)),xfreertos)
	$(call copy-sdk-files,$(OUT_DEVICE_STATIC_DIR),$(PRIVATE_DIR)/platform/usr/lib)
	$(call copy-sdk-files,${SECTIONIMAGE}, $(PRIVATE_DIR)/platform/images/)
	$(call copy-sdk-files,${FREERTOSIMAGE}, $(PRIVATE_DIR)/platform/images/)
	$(call copy-sdk-files,$(BUILD_FREERTOS_INTERMEDIATE)/System.map, $(PRIVATE_DIR)/platform/images/)
endif
else
	$(call copy-sdk-files,$(TARGET_INSTALL_PATH)/update_fs/usr/lib,$(PRIVATE_DIR)/platform/usr/lib)
	$(call copy-sdk-files,$(TARGET_INSTALL_PATH)/update_fs/lib,$(PRIVATE_DIR)/platform/lib)
	$(call copy-sdk-files,$(OUT_DEVICE_SHARED_DIR)/*,$(PRIVATE_DIR)/platform/usr/lib)
	$(call copy-sdk-files,$(TARGET_FS_BUILD)/lib/*,$(PRIVATE_DIR)/platform/lib)
endif
	$(call copy-sdk-files,$(OUT_DEVICE_STATIC_DIR)/*,$(PRIVATE_DIR)/platform/usr/lib)
ifneq (x$(strip $(BUILD_TARGET_OS_TYPE)),xfreertos)
	$(call copy-sdk-files,$(TOPDIR)/build/core,$(PRIVATE_DIR)/build/core)
	$(call copy-sdk-files,$(TOPDIR)/build/Makefile,$(PRIVATE_DIR)/Makefile)
endif
ifneq ($(filter win_sdk,$(MAKECMDGOALS)),)
	$(call copy-sdk-files,$(WIN_CROSS_TOOLCHAINS),$(PRIVATE_DIR)/prebuilts/toolchains/mips-gcc-glibc)
	$(call copy-sdk-files,$(WIN_HOST_TOOLS)/make.exe,$(PRIVATE_DIR)/make.exe)
	$(call copy-sdk-files,$(TOPDIR)/build/core/tools/sdk-build.cmd,$(PRIVATE_DIR)/sdk-build.cmd)
else
ifeq (x$(strip $(BUILD_TARGET_OS_TYPE)),xfreertos)
	$(call copy-sdk-files-nocheck,$(TOPDIR)/build/core/rtos/sdks/*,$(PRIVATE_DIR)/)
else
	$(call copy-sdk-files,$(TOPDIR)/build/core/tools/sdk-build,$(PRIVATE_DIR)/sdk-build)
endif
	$(call copy-sdk-files,$(COMPILER_PATH),$(PRIVATE_DIR)/prebuilts/toolchains/mips-gcc-glibc)
ifdef $(COMPILER_PATH_WIN)
	$(error "[error]:COMPILER_PATH_WIN is empty!")
endif
	$(call copy-sdk-files,$(COMPILER_PATH_WIN),$(PRIVATE_DIR)/prebuilts/toolchains/mips-gcc-win)
endif


ifneq ($(strip x$(ALL_BUILD_SAMPLE_PATH)),x)
	$(call copy-sdk-files,$(ALL_BUILD_SAMPLE_PATH),$(PRIVATE_DIR)/Sample/)
endif
	$(call copy-sdk-files,$(SOURCES_PATH),$(PRIVATE_DIR)/source)
	$(call copy-sdk-files,$(DOC_FILES),$(PRIVATE_DIR)/docs/)
	chmod -R ug+rwX $(PRIVATE_DIR)
	cd $(dir $@) && zip -rq  -y $(notdir $@) $(PRIVATE_NAME)
