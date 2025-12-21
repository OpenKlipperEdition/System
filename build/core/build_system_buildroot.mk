include $(BUILD_SYSTEM)/build_buildroot.mk

FILE_SYSTEM_TYPE:=$(strip $(FILE_SYSTEM_TYPE))

BUILD_SYSTEMIMAGE_TARGET:=$(OUT_IMAGE_DIR)/system.$(FILE_SYSTEM_TYPE)

define MODULES_DEP_ROOTFS
$(1):rootfs
endef
$(foreach m,$(ALL_MODULES), $(eval $(call MODULES_DEP_ROOTFS,$(m))))

systemimage:rootfs $(ALL_MODULES) post-image usrdata_image

rootfs: buildroot

post-image: buildroot-post-image $(BUILD_SYSTEMIMAGE_TARGET)

$(BUILD_SYSTEMIMAGE_TARGET):$(BUILD_BUILDROOT_INTERMEDIATE_OBJ)
	cp -f $^ $@

.PHONY:rootfs systemimage post-image

#################################################################################
OTA_INSTALL_STEMP=$(OUT_DEVICE_OBJ_DIR)/ota/.stamp_install
DIFFOTA_INSTALL_STEMP=$(OUT_DEVICE_OBJ_DIR)/ota-diff/.stamp_install
UBOOT_BIN?=$(OUT_IMAGE_DIR)/uboot
KERNEL_BIN?=$(OUT_IMAGE_DIR)/kernel
KERNEL_RECOVERY_BIN?=$(OUT_IMAGE_DIR)/kernel_recovery
SYSTEM_BIN?=$(OUT_IMAGE_DIR)/system.ubifs

DIFFOTA_PACKGE_TEMP_DIR=$(TOP_DIR)/$(OUT_DEVICE_OBJ_DIR)/ota-diff/allpackage
OTA_PACKGE_TEMP_DIR=$(TOP_DIR)/$(OUT_DEVICE_OBJ_DIR)/ota/allpackage
OTA_PACKAGE_OUTDIR=$(OUT_IMAGE_DIR)/ota
DIFFOTA_PACKAGE_OUTDIR=$(OUT_IMAGE_DIR)/ota-diff

PACKAGE_CONFIG?=tools/updater/ota_package_maker/example/config
DIFF_PACKAGE_CONFIG?=tools/updater/ota_package_maker/example/config
PACKAGE_PYTOOL=otapackage

PRODUCT_DEVICE_PATH:=$(DEVICE_PATH)
KEYNAME?=$(PRODUCT_DEVICE_PATH)/security/releasekey
OTA_PACKAGE_MAKER?=$(TOP_DIR)/tools/updater/ota_package_maker
DEVICE_TYPE?=nand
SINGLE_PKG?=no
# slicesize is B
OTA_SLICESIZE?=1048576
# limit size is kB
OTA_LIMITSIZE?=307200


OTA_FUNC_DIR_STEMP=$(OUT_DEVICE_OBJ_DIR)/ota/ota_package_maker/.stamp_install
$(OTA_FUNC_DIR_STEMP):
	mkdir -p $(dir $@)
	cp -rfT ${OTA_PACKAGE_MAKER} $(dir $@)
	@install -D /dev/null $@

define OTA_COPY_FILES
.PHONY: $(1)
$(1):$(2)
	mkdir -p $$(dir $$@)
	cp $$^ $$@
endef

NV_BIN:=$(OUT_IMAGE_DIR)/zero
$(NV_BIN):
	mkdir -p $(dir $@)
	dd if=/dev/zero of=$@ bs=1k count=512

ALL_CUSTOM_BIN+=$(EXT_CUSTOM_BINS)
OTA_FILES_BINS:=$(KERNEL_BIN) $(KERNEL_RECOVERY_BIN) $(SYSTEM_BIN) $(REPACK_UBOOT_BIN) $(ALL_CUSTOM_BIN)

$(OTA_INSTALL_STEMP): $(NV_BIN) $(OTA_FUNC_DIR_STEMP) $(OTA_FILES_BINS)
	@mkdir -p $(OTA_PACKAGE_OUTDIR)
	@mkdir -p $(OTA_PACKGE_TEMP_DIR)/${DEVICE_TYPE}
	cp $(UBOOT_BIN) $(OTA_PACKGE_TEMP_DIR)/${DEVICE_TYPE} && \
	cp $(OTA_FILES_BINS) $(OTA_PACKGE_TEMP_DIR)/${DEVICE_TYPE} && \
	cd $(dir $(OTA_FUNC_DIR_STEMP)); $(PYTHON_TOOL) -m $(PACKAGE_PYTOOL) --otamode="fullpkg" --output=$(TOP_DIR)/$(OTA_PACKAGE_OUTDIR) --imgpath=$(OTA_PACKGE_TEMP_DIR) --configpath=$(TOP_DIR)/$(PACKAGE_CONFIG) --publickey=$(KEYNAME).x509.pem --privatekey=$(KEYNAME).pk8  --slicesize=$(OTA_SLICESIZE) --limitsize=$(OTA_LIMITSIZE) --singlepkg=$(SINGLE_PKG);cd - && \
	cd $(OTA_PACKAGE_OUTDIR);7zr a ota-package -r global.xml  ${DEVICE_TYPE} sha1Tab VERSION;cd -
	@install -D /dev/null $(OTA_INSTALL_STEMP)
	@echo "ota package create to " $(OUT_IMAGE_DIR)/ota

DEST_SYSTEM:=$(TOP_DIR)/$(TARGET_FS_BUILD)
SRC_SYSTEM_PATH?=$(DIFFOTA_PACKGE_TEMP_DIR)
DEST_SYSTEM_PATH?=$(DIFFOTA_PACKGE_TEMP_DIR)/${DEVICE_TYPE}

DEST_SYSTEM_INFO=$(shell du -s $(DEST_SYSTEM)/)
LIMIT_SIZE:=$(word 1, $(DEST_SYSTEM_INFO))

$(DIFFOTA_INSTALL_STEMP):$(KERNEL_BIN) $(KERNEL_RECOVERY_BIN) $(SYSTEM_BIN) $(OTA_FUNC_DIR_STEMP)
	@mkdir -p $(DIFFOTA_PACKGE_TEMP_DIR)/${DEVICE_TYPE}
	@mkdir -p $(DIFFOTA_PACKGE_TEMP_DIR)/${DEVICE_TYPE}/system
	@mkdir -p $(DIFFOTA_PACKAGE_OUTDIR)
	cp $(UBOOT_BIN) $(DIFFOTA_PACKGE_TEMP_DIR)/${DEVICE_TYPE} && \
	cp $(KERNEL_BIN) $(DIFFOTA_PACKGE_TEMP_DIR)/${DEVICE_TYPE} && \
	cp $(KERNEL_RECOVERY_BIN) $(DIFFOTA_PACKGE_TEMP_DIR)/${DEVICE_TYPE} && \
	cp -dpa $(DEST_SYSTEM)/* $(DIFFOTA_PACKGE_TEMP_DIR)/${DEVICE_TYPE}/system && \
	cp $(OUT_DEVICE_OBJ_DIR)/buildroot-intermediate/host/sbin/bsdiff $(dir $(OTA_FUNC_DIR_STEMP))
	cd $(dir $(OTA_FUNC_DIR_STEMP)); $(PYTHON_TOOL) -m $(PACKAGE_PYTOOL) --otamode="diffpkg" --srcpath=$(SRC_SYSTEM_PATH) --dstpath=$(DEST_SYSTEM_PATH)  --output=$(TOP_DIR)/$(DIFFOTA_PACKAGE_OUTDIR) --limitsize=$(LIMIT_SIZE) --configpath=$(TOP_DIR)/$(DIFF_PACKAGE_CONFIG) --publickey=$(KEYNAME).x509.pem --privatekey=$(KEYNAME).pk8 --singlepkg=$(SINGLE_PKG) --limitsize=$(OTA_LIMITSIZE);cd - && \
	cd $(DIFFOTA_PACKAGE_OUTDIR);7zr a ota-package -r global.xml  ${DEVICE_TYPE} sha1Tab VERSION;cd -
	@install -D /dev/null $(DIFFOTA_INSTALL_STEMP)
	@echo "ota package create to " $(OUT_IMAGE_DIR)/ota-diff

.PHONY:ota_mkpackage
.PHONY:ota_mkdiffpackage
.PHONY:usrdata_image

usrdata_image:buildroot $(DEV_USRDATA_MODE)
usrdata_image-clean:$(DEV_USRDATA_MODE_CLEAN)

ota_mkpackage:$(OTA_INSTALL_STEMP)
ota_mkdiffpackage:$(DIFFOTA_INSTALL_STEMP)

ota_mkpackage-clean:
	rm -rf $(OUT_IMAGE_DIR)/ota
