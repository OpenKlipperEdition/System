### build freertos
FREERTOS_PATH ?= $(TOP_DIR)/freertos
FREERTOS_PATH := $(addprefix $(TOP_DIR)/,$(subst $(TOP_DIR)/,,$(strip $(FREERTOS_PATH))))

FILE_SYSTEM_TYPE:=$(strip $(FILE_SYSTEM_TYPE))
BUILD_SYSTEMIMAGE_TARGET:=$(OUT_IMAGE_DIR)/system.$(FILE_SYSTEM_TYPE)

PUBLIC_MODULE_SECTION_JSON:=$(BUILD_SYSTEM)/rtos/scripts/section.json

$(info KERNEL_BOARD_OVERLAY_PATH: $(KERNEL_BOARD_OVERLAY_PATH))

BUILD_FREERTOS_INTERMEDIATE_SYNC:=$(BUILD_FREERTOS_INTERMEDIATE)/.stamp_install
BUILD_FREERTOS_INTERMEDIATE_OVERRIDE:=$(BUILD_FREERTOS_INTERMEDIATE)/.stamp_override
RTOS_TARGET_CONFIGURED:=$(FREERTOS_PATH)/$(basename $(FREERTOS_CONFIG))

# define target stamps.
RTOS_TARGET_CONFIGURED	= $(FREERTOS_PATH)/configs/$(notdir $(FREERTOS_CONFIG))

$(RTOS_TARGET_CONFIGURED):$(BUILD_FREERTOS_INTERMEDIATE_SYNC)
$(RTOS_TARGET_CONFIGURED):$(BUILD_FREERTOS_INTERMEDIATE_OVERRIDE)
$(RTOS_TARGET_CONFIGURED):$(FREERTOS_CONFIG)
	cp $< $@

$(BUILD_FREERTOS_INTERMEDIATE_SYNC):$(FREERTOS_PATH)/Makefile
	mkdir -p $(dir $@)
#	rsync -zvra $(dir $^) $(dir $@)
	install -D /dev/null $@

$(BUILD_FREERTOS_INTERMEDIATE_OVERRIDE):
ifneq ($(notdir $(KERNEL_BOARD_OVERLAY_PATH)), )
	cp -drf $(KERNEL_BOARD_OVERLAY_PATH)/* $(FREERTOS_PATH)
endif
	install -D /dev/null $@

FREERTOSELF:=$(BUILD_FREERTOS_INTERMEDIATE)/zero.elf

FREERTOSIMAGE:=$(BUILD_FREERTOS_INTERMEDIATE)/rtos-with-spl.bin
SYSTEMIMAGE:=$(OUT_IMAGE_DIR)/rtos-with-spl.bin


USERDATA_NAME ?= userdata

ifneq (x$(USERDATA_MODULE),x)
$(TARGET_INSTALL_PATH)/$(USERDATA_NAME)/.stamp:$(USERDATA_MODULE)
	mkdir -p $(dir $@)
	install -D /dev/null $@

# default config
DEVICE_RTOS_FS_TYPE ?= EXFAT
DEVICE_RTOS_FS_SIZE ?= 100M
$(OUT_IMAGE_DIR)/$(USERDATA_NAME).img:$(TARGET_INSTALL_PATH)/$(USERDATA_NAME)/.stamp exfatopt
	$(OUT_HOST_DIR)/tools/exfatopt -o $@ -r $(dir $<) -s ${DEVICE_RTOS_FS_SIZE} -f ${DEVICE_RTOS_FS_TYPE} /

$(USERDATA_NAME): $(OUT_IMAGE_DIR)/$(USERDATA_NAME).img HOST-exfatopt

freertos:$(FREERTOSIMAGE)

systemimage:$(SYSTEMIMAGE) $(ALL_MODULES) $(USERDATA_NAME)
else
systemimage:$(SYSTEMIMAGE) $(ALL_MODULES)
endif

ifeq (x$(DEVICE_TYPE),xmmc)
SECTIONIMAGE := $(OUT_IMAGE_DIR)/section.bin

$(SECTIONIMAGE):$(RTOS_SECTION_IMAGE)
	$(hide) mkdir -p $(dir $@)
	$(hide) $(BUILD_SYSTEM)/rtos/sec2bin.py $(PUBLIC_MODULE_SECTION_JSON) $@ 3M $^


$(SYSTEMIMAGE):$(FREERTOSIMAGE) $(SECTIONIMAGE)
	cp $< $@
else
$(SYSTEMIMAGE):$(FREERTOSIMAGE)
	cp $< $@
endif

RTOS_TARGET_CONFIGURED_FILE:=$(FREERTOS_PATH)/include/config.h

$(RTOS_TARGET_CONFIGURED_FILE):$(RTOS_TARGET_CONFIGURED)
	make -C $(FREERTOS_PATH) O=$(BUILD_FREERTOS_INTERMEDIATE) $(notdir $<) -j$(MAKE_JLEVEL);


EXPORT_FUNC:=$(BUILD_FREERTOS_INTERMEDIATE)/export.sym

$(EXPORT_FUNC): $(RTOS_MODULE_IMPORT_FUNC)
	$(hide) $(BUILD_SYSTEM)/rtos/importmerge.py $(RTOS_MODULE_IMPORT_FUNC) > $@



$(FREERTOSIMAGE):$(RTOS_TARGET_CONFIGURED_FILE) $(ALL_PREBUILT_MODULES) $(RTOS_MODULES) $(EXPORT_FUNC)
	make  -C $(FREERTOS_PATH) O=$(dir $@) -j$(MAKE_JLEVEL)

$(FREERTOSELF):$(FREERTOSIMAGE)
$(BUILD_FREERTOS_SYSTEM_MAP):$(FREERTOSELF)
	$(TARGET_NM) -n $< | grep -v "( [aNUw] ) |( $[adt]) | ( .L)" > $@

freertos-clean:
	make -C $(FREERTOS_PATH) O=$(dir $(FREERTOSIMAGE)) clean
freertos-distclean:
	rm -rf $(BUILD_FREERTOS_INTERMEDIATE)

.PHONY:rootfs freertos freertos-clean systemimage

#################################################################################
OTA_INSTALL_STEMP=$(OUT_DEVICE_OBJ_DIR)/ota/.stamp_install

OTA_PACKGE_TEMP_DIR=$(TOP_DIR)/$(OUT_DEVICE_OBJ_DIR)/ota/allpackage
OTA_PACKAGE_OUTDIR=$(OUT_IMAGE_DIR)/ota

PACKAGE_CONFIG?=packages/updater/ota_package_maker/example/config
PACKAGE_PYTOOL=otapackage

PRODUCT_DEVICE_PATH:=$(DEVICE_PATH)
KEYNAME?=$(PRODUCT_DEVICE_PATH)/security/releasekey
OTA_PACKAGE_MAKER?=$(TOP_DIR)/packages/updater/ota_package_maker
PYTHON_TOOL=$(DEVICE_PATH)/ota/python
DEVICE_TYPE?=nand
SINGLE_PKG?=no
DEVICE_RTOS_OTA_IMAGES?=

OTA_FUNC_DIR_STEMP=$(OUT_DEVICE_OBJ_DIR)/ota/ota_package_maker/.stamp_install
$(OTA_FUNC_DIR_STEMP):
	mkdir -p $(dir $@)
	cp -rfT ${OTA_PACKAGE_MAKER} $(dir $@)
	@install -D /dev/null $@

$(OTA_INSTALL_STEMP):$(FREERTOSIMAGE) $(OTA_FUNC_DIR_STEMP) $(DEVICE_RTOS_OTA_IMAGES)
	mkdir -p $(OUT_IMAGE_DIR)
	dd if=/dev/zero of=$(OUT_IMAGE_DIR)/zero bs=1k count=512
	@mkdir -p $(OTA_PACKGE_TEMP_DIR)/${DEVICE_TYPE}
	@mkdir -p $(OTA_PACKAGE_OUTDIR)
	cp $(FREERTOSIMAGE) $(OTA_PACKGE_TEMP_DIR)/${DEVICE_TYPE}
	@for image in ${DEVICE_RTOS_OTA_IMAGES}; \
		do cp $$image $(OTA_PACKGE_TEMP_DIR)/${DEVICE_TYPE}; done
	cd $(dir $(OTA_FUNC_DIR_STEMP)); $(PYTHON_TOOL) -m $(PACKAGE_PYTOOL) --otamode="fullpkg" --output=$(TOP_DIR)/$(OTA_PACKAGE_OUTDIR) --imgpath=$(OTA_PACKGE_TEMP_DIR) --configpath=$(TOP_DIR)/$(PACKAGE_CONFIG) --publickey=$(KEYNAME).x509.pem --privatekey=$(KEYNAME).pk8 --singlepkg=$(SINGLE_PKG);cd - && \
	cd $(OTA_PACKAGE_OUTDIR);7zr a ota-package -r global.xml  ${DEVICE_TYPE} sha1Tab VERSION;cd -
	@install -D /dev/null $(OTA_INSTALL_STEMP)
	@echo "ota package create to " $(OUT_IMAGE_DIR)/ota

.PHONY:rtos_ota_mkpackage

rtos_ota_mkpackage:$(OTA_INSTALL_STEMP)
