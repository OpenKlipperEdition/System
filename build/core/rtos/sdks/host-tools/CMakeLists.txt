cmake_minimum_required(VERSION 3.8.1)
project(host-tools)
set(CMAKE_C_FLAGS "-O2")
include_directories(cJSON)
add_executable(mkrtoslds mkrtoslds.c)
add_executable(sec2bin sec2bin.c cJSON/cJSON.c)

install(TARGETS mkrtoslds DESTINATION ${CMAKE_BINARY_DIR})
install(TARGETS sec2bin DESTINATION ${CMAKE_BINARY_DIR})
