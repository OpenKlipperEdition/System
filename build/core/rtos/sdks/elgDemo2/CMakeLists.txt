cmake_minimum_required(VERSION 3.8.1)
PROJECT (elgDemo)

# LOCAL_XX is must set for each section
set(LOCAL_SECTION_TYPE    "section1")
set(LOCAL_SECTION_TARGET elgDemo2.elf)
set(LOCAL_IMPORT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/import/import.txt)
#
set(SRC_FILE main.c)
set(CMAKE_C_FLAGS "-I=/usr/include/easylogger")
add_executable(${LOCAL_SECTION_TARGET} ${SRC_FILE})

target_compile_options(${LOCAL_SECTION_TARGET} PRIVATE -O0 -g0 )

# include section binary generate cmake
include(secgen)
