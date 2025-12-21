set(CMAKE_SYSROOT ${CMAKE_SOURCE_DIR}/platform/)
add_compile_options(-Wno-enum-compare -fno-strict-aliasing
  -isystem ${CMAKE_SOURCE_DIR}/platform/usr/include -Wno-format-zero-length -Wall -fno-stack-protector -O2
  -ffunction-sections -fdata-sections -G 0 -EL
  -fno-pic -mno-abicalls -static -march=mips32r2 -fno-builtin
  -mabi=32)

include_directories(${CMAKE_SOURCE_DIR}/platform/usr/include)
set_property(GLOBAL PROPERTY LINK_FLAGS
  "-G 0 -static -n -nostdlib -EL -m elf32ltsmip  \
--gc-sections -Bstatic  \
-L${CMAKE_SOURCE_DIR}/platform/usr/lib \
--start-group -lstart  -lnosys -lc -lg -lm -lgcc --end-group")

set(SYS_SECTION_JSON "${CMAKE_MODULE_PATH}/tools/scripts/section.json")
set(SYS_SECTION_LDS "${CMAKE_MODULE_PATH}/tools/scripts/section.lds")
set(SYS_SYSTEM_MAP "${CMAKE_SOURCE_DIR}/platform/images/System.map")
#set(SYS_SEC2BIN_CMD "${CMAKE_MODULE_PATH}/tools/sec2bin.py")
set(SYS_SEC2BIN_CMD "${CMAKE_BINARY_DIR}/host-tools/sec2bin")
set(SYS_MKRTOSLDS_CMD "${CMAKE_MODULE_PATH}/tools/mkrtoslds.py")

set(LOCAL_SETCTION_TYPE  "section1")
set_property(GLOBAL APPEND PROPERTY GSUB_SECTION_BINS)
set_property(GLOBAL APPEND PROPERTY GSUB_SECTION_BINS_TARGET)
