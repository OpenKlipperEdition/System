# Makefile define rules which make buildroot.
#
#	Values input when include this Makefile
#	==========================
#	BUILDROOT_CONFIG: # mandatory. buildroot defconfig
#	BUILDROOT_CONFIG_PATH: # optional, define where the defconfig file is.
#				default under 'configs'
#	BUILDROOT_PATH:   # optional.  default $(TOP_DIR)/buildroot
#
#	Values output after included this Makefile:
#	=======================================
#	BUILD_BUILDROOT_INTERMEDIATE_OBJ
#
#	support make:
#	============
#	make buildroot # compile buildroot according to BUILDROOT_XX Vars.
#	make buildroot-post-image #
#	make buildroot-clean #
#	make buildroot-xx # transfer targets to buildroot environment.
#
#	introduction:
#	===========
#	use a provided buildroot config file, which can make
#	target rootfs image type.
#	eg: ext2, jiffs2, ubifs.
#	all params to make an image is configure in the config file
#



FILE_SYSTEM_TYPE:=$(strip $(FILE_SYSTEM_TYPE))
ifeq ($(FILE_SYSTEM_TYPE),)
$(error **** FILE_SYSTEM_TYPE, $(FILE_SYSTEM_TYPE) must be defined)
endif
ifeq ($(BUILDROOT_CONFIG),)
$(error **** BUILDROOT_CONFIG, $(BUILDROOT_CONFIG) must be defined)
endif


BUILDROOT_PATH ?= $(TOP_DIR)/buildroot

# convert both relative & absolute PATH to absolute PATH.
BUILDROOT_PATH := $(addprefix $(TOP_DIR)/,$(subst $(TOP_DIR)/,,$(strip $(BUILDROOT_PATH))))

BUILDROOT_CONFIG_PATH ?= $(BUILDROOT_PATH)/configs


BUILDROOT_DEFCONFIG:=$(BUILDROOT_CONFIG_PATH)/$(BUILDROOT_CONFIG)


BUILD_BUILDROOT_INTERMEDIATE:=$(TOP_DIR)/$(OUT_DEVICE_OBJ_DIR)/buildroot-intermediate
BUILD_BUILDROOT_INTERMEDIATE_OBJ:=$(BUILD_BUILDROOT_INTERMEDIATE)/images/rootfs.$(FILE_SYSTEM_TYPE)


# define target stamps.
BUILDROOT_TARGET_BUILT		= $(BUILD_BUILDROOT_INTERMEDIATE)/.stamp_built
BUILDROOT_TARGET_CONFIGURED	= $(BUILD_BUILDROOT_INTERMEDIATE)/.stamp_configured


ROOTFS_OVERLAY_DIR:=$(strip $(ROOTFS_OVERLAY_DIR))
ROOTFS_BUSYBOX_CONFIG:=$(strip $(ROOTFS_BUSYBOX_CONFIG))

buildroot:$(BUILDROOT_TARGET_BUILT) $(TARGET_FS_BUILD) $(TARGET_SYSROOT) $(OUT_HOST_DIR)

# force buildroot built?
buildroot-rebuild: buildroot-clean $(BUILDROOT_TARGET_BUILT)

$(BUILDROOT_TARGET_BUILT):$(BUILDROOT_TARGET_CONFIGURED)
	make -C $(BUILDROOT_PATH) O=$(BUILD_BUILDROOT_INTERMEDIATE) -j$(MAKE_JLEVEL)
	touch $@

buildroot-configure: buildroot-clean buildroot-clean-config $(BUILDROOT_TARGET_CONFIGURED)

$(BUILDROOT_TARGET_CONFIGURED):$(BUILD_BUILDROOT_INTERMEDIATE)/.config
	touch $@

$(BUILD_BUILDROOT_INTERMEDIATE)/.config:
	@# 生成默认配置
	make -C $(BUILDROOT_PATH) O=$(BUILD_BUILDROOT_INTERMEDIATE) defconfig BR2_DEFCONFIG=$(BUILDROOT_DEFCONFIG)

	@# 修改工具链路径
	sed -i \
		-e 's#BR2_TOOLCHAIN_EXTERNAL_PATH.*#BR2_TOOLCHAIN_EXTERNAL_PATH="$(COMPILER_PATH)"#' \
		-e 's#BR2_TOOLCHAIN_EXTERNAL_PREFIX.*#BR2_TOOLCHAIN_EXTERNAL_PREFIX="$(DEVICE_COMPILER_PREFIX)"#' \
		-e 's#BR2_TOOLCHAIN_EXTERNAL_CUSTOM_PREFIX.*#BR2_TOOLCHAIN_EXTERNAL_CUSTOM_PREFIX="$(DEVICE_COMPILER_PREFIX)"#' \
		$(BUILD_BUILDROOT_INTERMEDIATE)/.config

	@# 如果 ROOTFS_OVERLAY_DIR 不为空，则修改 .config 文件中的 BR2_ROOTFS_OVERLAY
	if [ "x$(ROOTFS_OVERLAY_DIR)" != "x" ]; then \
		sed -i -e 's#BR2_ROOTFS_OVERLAY.*#BR2_ROOTFS_OVERLAY="$(ROOTFS_OVERLAY_DIR)"#' $(BUILD_BUILDROOT_INTERMEDIATE)/.config; \
	fi

	@# 如果 ROOTFS_BUSYBOX_CONFIG 不为空，则修改 .config 文件中的 BR2_PACKAGE_BUSYBOX_CONFIG
	if [ "x$(ROOTFS_BUSYBOX_CONFIG)" != "x" ]; then \
		sed -i -e 's#BR2_PACKAGE_BUSYBOX_CONFIG=.*#BR2_PACKAGE_BUSYBOX_CONFIG="$(ROOTFS_BUSYBOX_CONFIG)"#' $(BUILD_BUILDROOT_INTERMEDIATE)/.config; \
	fi

$(TARGET_SYSROOT):$(BUILDROOT_TARGET_BUILT)
	ln -snf $(BUILD_BUILDROOT_INTERMEDIATE)/staging $@

$(TARGET_FS_BUILD):$(BUILDROOT_TARGET_BUILT)
	ln -snf $(BUILD_BUILDROOT_INTERMEDIATE)/target $@

$(OUT_HOST_DIR):$(BUILDROOT_TARGET_BUILT)
	ln -snf $(BUILD_BUILDROOT_INTERMEDIATE)/host $@

#overwrite buildroot origin targets.
buildroot-clean-config:
	rm $(BUILD_BUILDROOT_INTERMEDIATE)/.config -f

buildroot-clean:
	rm -f $(BUILDROOT_TARGET_CONFIGURED)
	rm -f $(BUILDROOT_TARGET_BUILT)

buildroot-distclean:
	rm -rf $(BUILD_BUILDROOT_INTERMEDIATE)

buildroot-post-image:
	make -C $(BUILDROOT_PATH) O=$(BUILD_BUILDROOT_INTERMEDIATE) target-post-image -j$(MAKE_JLEVEL)

#transfer build targets to buildroot origin.
buildroot-%:
	make -C $(BUILDROOT_PATH) O=$(BUILD_BUILDROOT_INTERMEDIATE) $(subst buildroot-,,$@) -j$(MAKE_JLEVEL)
	rm -f $(BUILDROOT_TARGET_CONFIGURED)
	rm -f $(BUILDROOT_TARGET_BUILT)

# Configuration vscode
PRO =
SCRIPTDIR := $(CURDIR)/build
OUTDIR := output/build/$(PRO)
COMPILEDIR = $(CURDIR)/buildroot
WORKDIR := $(COMPILEDIR)/ingenic/$(PRO)
OSDIR := $(COMPILEDIR)/output/host/usr/mipsel-buildroot-linux-gnu/sysroot/usr/lib

$(PRO)-vscode:
	sh $(SCRIPTDIR)/createvscode.sh $(PRO) $(OUTDIR) $(WORKDIR) $(COMPILEDIR) $(OSDIR)

$(PRO)-clean:
	rm $(WORKDIR)/$(PRO).code-workspace $(WORKDIR)/.vscode/ -rf

.PHONY:buildroot buildroot-configure buildroot-rebuild buildroot-post-image $(PRO)-vscode $(PRO)-clean
