#LOCAL_EXT_NAME:=
LOCAL_EXT_NAME:=.elf
HOST:=

ifeq (x$(LOCAL_MODULE_SECTION),x)
$(error "LOCAL_MODULE_SECTION should be set.")
endif

RTOS_TOP := $(FREERTOS_PATH)
PUBLIC_MODULE_SECTION_LDS:=$(BUILD_SYSTEM)/rtos/scripts/section.lds
PUBLIC_MODULE_SECTION_JSON:=$(BUILD_SYSTEM)/rtos/scripts/section.json

RTOS_CFLAGS :=
RTOS_CFLAGS += -isystem $(TARGET_SYSROOT)/usr/include
RTOS_CFLAGS += -isystem $(shell $(TARGET_GCC) -print-file-name=include)
RTOS_CFLAGS += -isystem $(shell $(TARGET_GCC) -print-file-name=include-fixed)
RTOS_CFLAGS += -isystem $(RTOS_TOP)/include
RTOS_CFLAGS += -include $(RTOS_TOP)/include/config.h
RTOS_CFLAGS += -I=/usr/include/easylogger
RTOS_CFLAGS += $(DEVICE_RTOS_CFLAGS)
RTOS_LDFLAGS :=  $(DEVICE_RTOS_LDFLAGS)

ifneq (x$(LOCAL_RTOS_C_INCLUDES),x)
RTOS_CFLAGS += $(addpreix -I$(RTOS_TOP)/,$(LOCAL_RTOS_C_INCLUDES))
endif


LOCAL_DISABLE_GLOBAL_CFLAGS := true
LOCAL_CFLAGS += -std=gnu99 -Wno-enum-compare -fno-strict-aliasing -Wno-format-zero-length -Wall -Wstrict-prototypes -fno-stack-protector -O2 -ffunction-sections -fdata-sections -G 0 -EL -fno-pic -mno-abicalls -static -march=mips32r2 -mabi=32 -fno-builtin -ffreestanding -nostdinc -nostdlib $(RTOS_CFLAGS)

LOCAL_CPPFLAGS += -Wno-enum-compare -Wno-format-zero-length -Wall -fno-stack-protector -ffunction-sections -fdata-sections -O2  -G 0 -EL -fno-pic -mno-abicalls -static -march=mips32r2 -mabi=32 -ffreestanding -nostdinc -nostdlib -fno-rtti -fno-exceptions -isystem $(TARGET_SYSROOT)/usr/include/stdc++ --std=c++11 $(RTOS_CFLAGS) -D_LIBCPP_BUILDING_LIBRARY -D_LIBCPP_NO_EXCEPTIONS -fno-rtti

LOCAL_ASMFLAGS += -D__ASSEMBLY__

#LOCAL_OUT_INSTALL_DIR:=$(OUT_DEVICE_BINRARY_DIR)
LOCAL_OUT_INSTALL_DIR:=$(OUT_DEVICE_STATIC_DIR)

ifeq ($(strip $(LOCAL_TARGET_MODULE_CLASS)),)
LOCAL_TARGET_MODULE_CLASS := DEPANNER
endif

include $(BUILD_SYSTEM)/base_ruler2.mk

LOCAL_MODULE_BUILD:=$(LOCAL_MODULE)
include $(BUILD_SYSTEM)/module_install.mk

ifneq (x$(LOCAL_FILTER_MODULE),x)

RTOS_MODULE_IMPORT_FUNC:=$(LOCAL_MODULE_IMPORT) $(RTOS_MODULE_IMPORT_FUNC)

MODULE_LDS := $(dir $(LOCAL_OUT_MODULE_BUILD))script.lds

$(LOCAL_INSTALL_MODULE):$(LOCAL_OUT_MODULE_BUILD)
	$(call host-mkdir,$(dir $@))
	$(hide) $(call host-cp,$^,$@)


$(MODULE_LDS):PRIVATE_MODULE_SECTION:=$(LOCAL_MODULE_SECTION)
$(MODULE_LDS):$(LOCAL_MODULE_IMPORT) $(PUBLIC_MODULE_SECTION_LDS) $(BUILD_FREERTOS_SYSTEM_MAP) $(PUBLIC_MODULE_SECTION_JSON)
	mkdir -p $(dir $@)
	$(BUILD_SYSTEM)/rtos/mkrtoslds.py $^ $(PRIVATE_MODULE_SECTION) > $@

$(LOCAL_OUT_MODULE_BUILD):PRIVATE_MODULE_OBJS:=$(LOCAL_MODULE_OBJS)
$(LOCAL_OUT_MODULE_BUILD):PRIVATE_MODULE_LDS:=$(MODULE_LDS)
$(LOCAL_OUT_MODULE_BUILD):PRIVATE_LOCAL_LDFLAGS:=$(LOCAL_LDFLAGS)
$(LOCAL_OUT_MODULE_BUILD):$(MODULE_LDS) $(LOCAL_MODULE_OBJS) $(RTOS_MODULES)
	$(hide) $(TARGET_LD) -G 0 -static -n -nostdlib -EL -m elf32ltsmip --gc-sections -Bstatic -T$(PRIVATE_MODULE_LDS) ${RTOS_LDFLAGS} -L${TARGET_SYSROOT}/usr/lib --start-group $(PRIVATE_MODULE_OBJS) ${PRIVATE_STATIC_LIBRARIES} ${PRIVATE_LOCAL_LDFLAGS} -lnosys -lc -lg -lm -lgcc --end-group -o $@


LOCAL_OUT_MODULE_INSTALL:=$(OUT_IMAGE_DIR)/$(LOCAL_MODULE).$(LOCAL_MODULE_SECTION).bin

RTOS_SECTION_IMAGE += $(LOCAL_OUT_MODULE_INSTALL)


$(LOCAL_OUT_MODULE_INSTALL):$(LOCAL_OUT_MODULE_BUILD)
	$(hide) $(TARGET_OBJCOPY) -O binary $< $@

$(LOCAL_MODULE):$(LOCAL_OUT_MODULE_INSTALL)

endif
$(LOCAL_MODULE)-distclean:$(LOCAL_OUT_MODULE_BUILD)
	rm -rf $(dir $(LOCAL_OUT_MODULE_BUILD))
