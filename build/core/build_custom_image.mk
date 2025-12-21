LOCAL_MODULE:=$(strip $(LOCAL_MODULE))
LOCAL_MODULE_BUILD:=$(LOCAL_MODULE)
LOCAL_PATH := $(strip $(LOCAL_PATH))
LOCAL_MODULE_PATH := $(strip $(LOCAL_MODULE_PATH))

__pagesize?=2048
__lebsize?=126976	# PHY_BLOCKSIZE-2*PAGESIZE
__lebcount?=1024		# 1024 * PHY_BLOCKSIZE = NAND PARTION SIZE

__mkfs_ubi_cmd?=$(TOP_DIR)/$(OUT_DEVICE_OBJ_DIR)/buildroot-intermediate/host/sbin/mkfs.ubifs

include $(BUILD_SYSTEM)/base_ruler.mk
include $(BUILD_SYSTEM)/module_install.mk
ALL_MODULES_CLEAN:=$(filter-out $(PREFIX)$(LOCAL_MODULE), $(ALL_MODULES_CLEAN))


# LOCAL_MKIMG_SRC_DIR 源打包路径
# LOCAL_MKIMG_CMD    打包命令
# LOCAL_MKIMG_OUT_DIR    输出名称或输出相对路径加名称。
__mkimg_src_file:=$(strip $(LOCAL_MKIMG_SRC_FILE))
__mkimg_cmd:=$(strip $(LOCAL_MKIMG_CMD))
__mkimg_out_file:=$(strip $(LOCAL_MKIMG_OUT_FILE))

ifeq (x$(strip $(__mkimg_out_file)),x)
$(warning "MKIMAGE OUT is not set, default value is $(LOCAL_MODULE).image")
__mkimg_out_file:=$(LOCAL_MODULE).image
endif

__mkimg_obj_path:=$(TOP_DIR)/$(OUT_DEVICE_OBJ_DIR)/$(LOCAL_MODULE)-intermediate
__mkimg_base_dir:=$(TOP_DIR)/$(LOCAL_PATH)

ifeq (x$(strip $(__mkimg_src_file)), x)
$(warning  LOCAL_MKIMG_SRC_FILE is not set or empty.)
endif


# 默认 COMMMAND 为 ubifs
ifeq (x$(strip $(__mkimg_cmd)),x)
__mkimg_cmd:=$(__mkfs_ubi_cmd) -m $(__pagesize) -e $(__lebsize) -c $(__lebcount) -r $(__mkimg_src_file) -o $(__mkimg_out_file)
endif

ifneq ($(patsubst /%,,$(__mkimg_cmd)),)
__mkimg_stamp_img:=$(__mkimg_obj_path)/$(__mkimg_out_file)
else
__mkimg_stamp_img:=$(__mkimg_out_file)
endif


$(__mkimg_stamp_img):__obj_path:=$(__mkimg_obj_path)
$(__mkimg_stamp_img):__build_cmd:=$(__mkimg_cmd)
$(__mkimg_stamp_img):$(__mkimg_base_dir)/Build.mk
$(__mkimg_stamp_img):$(__mkimg_src_file) $(wildcard $(__mkimg_src_file)/*)
	mkdir -p $(dir $@)
	cd $(__obj_path); $(__build_cmd)

__mkimg_out_img:=$(TOP_DIR)/$(OUT_IMAGE_DIR)/$(notdir $(__mkimg_out_file))
$(__mkimg_out_img):$(__mkimg_stamp_img)
	cp $< $@

$(LOCAL_MODULE):$(LOCAL_DEPANNER_MODULES) $(__mkimg_out_img)

$(LOCAL_MODULE)-clean:__stamp_image:=$(__mkimg_stamp_img)
$(LOCAL_MODULE)-clean:__out_image:=$(__mkimg_out_img)
$(LOCAL_MODULE)-clean:__obj_path:=$(__mkimg_obj_path)

$(LOCAL_MODULE)-clean:
	rm -f $(__stamp_image)
	rm -f $(__out_image)
	rm -rf $(__obj_path)

__dep-clean:=$(addsuffix -clean, $(LOCAL_DEPANNER_MODULES))
$(LOCAL_MODULE)-depclean:$(__dep-clean) $(LOCAL_MODULE)-clean


ifneq ($(filter $(LOCAL_MODULE),$(PRODUCT_OTA_MODULES)),)
ALL_CUSTOM_BIN+=$(__mkimg_out_img)
endif


