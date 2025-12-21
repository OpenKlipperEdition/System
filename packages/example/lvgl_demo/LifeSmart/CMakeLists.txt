cmake_minimum_required(VERSION 3.8.10)
project(smarthome C)

# 设置编译选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -g0 -DLV_CONF_INCLUDE_SIMPLE -DLV_LVGL_H_INCLUDE_SIMPLE")

# 设置源文件目录
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)

INCLUDE_DIRECTORIES(
	${CMAKE_SOURCE_DIR}/include
	${SYSROOT_DIR}/usr/include/lvgl
	${SYSROOT_DIR}/usr/include/lvgl/lv_drivers
)


# 收集源文件
file(GLOB ASRCS ${SRC_DIR}/*.S)
file(GLOB CSRCS ${SRC_DIR}/*.c)

# 生成目标可执行文件
add_executable(smarthome ${ASRCS} ${CSRCS})

# 链接 LVGL 和 LVDRV 库
target_link_libraries(smarthome lvgl lv_drivers freetype ts pthread)

# 安装目标可执行文件
INSTALL(TARGETS smarthome RUNTIME DESTINATION bin)
