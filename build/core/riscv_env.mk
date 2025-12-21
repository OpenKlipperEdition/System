$(RISCV_CMAKE_TEMPLATE):
	mkdir -p $(dir $@)
	echo "set(CMAKE_SYSTEM_NAME               Generic)" > $@
	echo "set(CMAKE_SYSTEM_PROCESSOR          riscv32)" >> $@
	echo "set(CMAKE_C_COMPILER                \"$(TOP_DIR)/$(RISCV_SDK_TOOLCHAIN_PATH)/bin/riscv32-ingenicv0-elf-gcc\")" >> $@
	echo "set(CMAKE_ASM_COMPILER               \"$(TOP_DIR)/$(RISCV_SDK_TOOLCHAIN_PATH)/bin/riscv32-ingenicv0-elf-gcc\")" >> $@
	echo "set(CMAKE_CXX_COMPILER               \"$(TOP_DIR)/$(RISCV_SDK_TOOLCHAIN_PATH)/bin/riscv32-ingenicv0-elf-g++\")" >> $@
	echo "set(CMAKE_OBJCOPY                    \"$(TOP_DIR)/$(RISCV_SDK_TOOLCHAIN_PATH)/bin/riscv32-ingenicv0-elf-objcopy\")" >> $@
	echo "set(CMAKE_SIZE                      \"$(TOP_DIR)/$(RISCV_SDK_TOOLCHAIN_PATH)/bin/riscv32-ingenicv0-elf-size\")" >> $@
	echo "set(CMAKE_OBJDUMP                      \"$(TOP_DIR)/$(RISCV_SDK_TOOLCHAIN_PATH)/bin/riscv32-ingenicv0-elf-objdump\")" >> $@
	echo "set(CMAKE_EXECUTABLE_SUFFIX_ASM     \".elf\")" >> $@
	echo "set(CMAKE_EXECUTABLE_SUFFIX_C     \".elf\")" >> $@
	echo "set(CMAKE_EXECUTABLE_SUFFIX_CXX     \".elf\")" >> $@
	echo "set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)" >> $@
