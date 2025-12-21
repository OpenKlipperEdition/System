arch_variant_cflags := -mips32r2 -mfp64

arch_variant_gcc_dir := $(shell ls $(COMPILER_PATH)/lib/gcc/$(DEVICE_COMPILER_PREFIX)/$(COMPILER_LIB_VERSION)/ | grep fp64)
arch_variant_libc_dir := $(shell ls $(COMPILER_PATH)/$(DEVICE_COMPILER_PREFIX)/libc/ | grep mfp64)

ifeq ($(TARGET_DEVICE_CROSS_LIB_TYPE),glibc)
arch_variant_ld_file := /lib/ld-linux-mipsn8.so.1
else ifeq ($(TARGET_DEVICE_CROSS_LIB_TYPE),uclibc)
arch_variant_ld_file := /lib/ld-uClibc-mipsn8.so.0
endif
