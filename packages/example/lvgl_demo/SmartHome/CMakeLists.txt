cmake_minimum_required(VERSION 3.8.10)
project(SmartHome VERSION 1.0.0 LANGUAGES C)

set(projname smarthome)

# 设置编译选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -g0")
add_definitions(-DLV_CONF_INCLUDE_SIMPLE -DLV_LVGL_H_INCLUDE_SIMPLE)

option(CMAKE_NV3041_LCD "support NV3041 LCD using spi" OFF)
if(CMAKE_NV3041_LCD)
  message("NV3041 LCD Support")
  add_definitions(-DNV3041_LCD)
endif()

INCLUDE_DIRECTORIES(
	${CMAKE_SOURCE_DIR}/include
	${SYSROOT_DIR}/usr/include/lvgl
	${SYSROOT_DIR}/usr/include/lvgl/lv_drivers
)

# 设置源文件目录
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)

# 收集源文件
file(GLOB ASRCS ${SRC_DIR}/*.S)
file(GLOB CSRCS ${SRC_DIR}/*.c)


# 生成目标可执行文件
add_executable(${projname} ${ASRCS} ${CSRCS})

# 链接 LVGL 和 LVDRV 库
target_link_libraries(${projname} lvgl lv_drivers freetype ts pthread)

# 安装目标可执行文件
INSTALL(TARGETS ${projname} RUNTIME DESTINATION bin)
