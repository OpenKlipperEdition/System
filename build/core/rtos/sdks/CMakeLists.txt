cmake_minimum_required(VERSION 3.8.1)
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)
PROJECT (freertos_app)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(packAll)

option(MERGE_BASE_SECTION "merge platform base section" ON)
set(BASE_SECTION_FILE  "${CMAKE_SOURCE_DIR}/platform/images/section.bin")
if(MERGE_BASE_SECTION)
  if ("${BASE_SECTION_FILE}X" STREQUAL "X")
    message(FATAL_ERROR "MERGE_BASE_SECTION is enable, but BASE_SECTION_FILE is nil, please check your command")
  endif()
endif()
set(ARG_GEN_CUSTOM_SEC_BIN -m ${MERGE_BASE_SECTION} -p ${BASE_SECTION_FILE} -j ${SYS_SECTION_JSON} -o custom.sec.bin -s 3M)
include(custom.cmake)

get_property(SUB_SECTION_BINS GLOBAL PROPERTY GSUB_SECTION_BINS)
get_property(SUB_SECTION_BINS_TARGET GLOBAL PROPERTY GSUB_SECTION_BINS_TARGET)

if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux")
    set(EXEC_EXTENTION_SUFFIX "")
else()
    set(EXEC_EXTENTION_SUFFIX ".exe")
endif()

# error of clean custom.sec.bin
add_custom_target(custom.sec.bin ALL
 # COMMAND ${SYS_SEC2BIN_CMD} ${SYS_SECTION_JSON} custom.sec.bin 3M ${SUB_SECTION_BINS}
  COMMAND ${CMAKE_BINARY_DIR}/host-tools/sec2bin ${ARG_GEN_CUSTOM_SEC_BIN} -i "${SUB_SECTION_BINS}"
  BYPRODUCTS custom.sec.bin
  DEPENDS ${SUB_SECTION_BINS_TARGET} ${CMAKE_BINARY_DIR}/host-tools/sec2bin${EXEC_EXTENTION_SUFFIX}
  COMMENT "custom.sec.bin generate"
  VERBATIM)

# Not work
set_property(GLOBAL PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
  custom.sec.bin)

include(host-tools)
install(FILES ${CMAKE_BINARY_DIR}/custom.sec.bin DESTINATION images)
install(FILES platform/images/rtos-with-spl.bin DESTINATION images)
