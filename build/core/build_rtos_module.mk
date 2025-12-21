#LOCAL_EXT_NAME:=
LOCAL_EXT_NAME:=.a
HOST:=

#ROOT_PATH:=$(TOP_DIR)/$(TARGET_INSTALL_PATH)/freertos
RTOS_TOP := $(FREERTOS_PATH)

RTOS_CFLAGS :=
RTOS_CFLAGS += -isystem $(TARGET_SYSROOT)/usr/include
RTOS_CFLAGS += -isystem $(shell $(TARGET_GCC) -print-file-name=include)
RTOS_CFLAGS += -isystem $(shell $(TARGET_GCC) -print-file-name=include-fixed)
RTOS_CFLAGS += -isystem $(RTOS_TOP)/include
RTOS_CFLAGS += -include $(RTOS_TOP)/include/config.h
RTOS_CFLAGS += -I=/usr/include/easylogger
RTOS_CFLAGS += $(DEVICE_RTOS_CFLAGS)
RTOS_LDFLAGS := $(DEVICE_RTOS_LDFLAGS)

RTOS_CFLAGS += -Wno-enum-compare -fno-strict-aliasing -Wno-format-zero-length -Wall -fno-stack-protector -O2 -ffunction-sections -fdata-sections -G 0 -EL -fno-pic -mno-abicalls -static -march=mips32r2 -mabi=32
RTOS_CFLAGS += -fno-builtin -ffreestanding -nostdinc -nostdlib

ifneq (x$(LOCAL_RTOS_C_INCLUDES),x)
RTOS_CFLAGS += $(addprefix -I$(RTOS_TOP)/,$(LOCAL_RTOS_C_INCLUDES))
endif

LOCAL_DISABLE_GLOBAL_CFLAGS := true
LOCAL_CFLAGS += -std=gnu99  -Wstrict-prototypes $(RTOS_CFLAGS)
LOCAL_CPPFLAGS += -isystem $(TARGET_SYSROOT)/usr/include/stdc++ --std=c++11 $(RTOS_CFLAGS) -D_LIBCPP_BUILDING_LIBRARY -D_LIBCPP_NO_EXCEPTIONS -fno-rtti

#LOCAL_OUT_INSTALL_DIR:=$(OUT_DEVICE_BINRARY_DIR)
LOCAL_OUT_INSTALL_DIR:=$(OUT_DEVICE_STATIC_DIR)

ifeq ($(strip $(LOCAL_TARGET_MODULE_CLASS)),)
#LOCAL_TARGET_MODULE_CLASS := DEPANNER
LOCAL_TARGET_MODULE_CLASS := STATIC_LIBRARIES
endif

include $(BUILD_SYSTEM)/base_ruler2.mk

LOCAL_MODULE_BUILD:=$(LOCAL_MODULE)
ifeq ($(strip x$(SDK_BUILD)), x)
include $(BUILD_SYSTEM)/module_install.mk
endif

$(LOCAL_INSTALL_MODULE):$(LOCAL_OUT_MODULE_BUILD)
	$(call host-mkdir,$(dir $@))
	$(hide) $(call host-cp,$^,$@)

$(LOCAL_OUT_MODULE_BUILD):$(LOCAL_MODULE_OBJS)
	$(hide) $(TARGET_AR) -rsv $@ $^

ifeq ($(strip x$(IS_RTOS_LIBRARY)), x)
ifneq (x$(LOCAL_FILTER_MODULE),x)
LOCAL_OUT_MODULE_INSTALL:=$(BUILD_FREERTOS_INTERMEDIATE)/vendor/$(notdir $(LOCAL_OUT_MODULE_BUILD))
RTOS_MODULES += $(LOCAL_OUT_MODULE_INSTALL)

$(LOCAL_OUT_MODULE_INSTALL):$(LOCAL_OUT_MODULE_BUILD)
	$(eval __dest := $(dir $@))
	$(hide) mkdir -p $(__dest)
	$(hide) cp $< $@

endif
endif

$(LOCAL_MODULE):$(RTOS_TOP)/include/config.h
$(LOCAL_MODULE):$(LOCAL_OUT_MODULE_INSTALL)

$(LOCAL_MODULE)-distclean:$(LOCAL_OUT_MODULE_BUILD)
	rm -rf $(dir $(LOCAL_OUT_MODULE_BUILD))
