CMAKE_MINIMUM_REQUIRED(VERSION 3.27)
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_C_COMPILER "mips-linux-gnu-gcc")
SET(CMAKE_CXX_COMPILER "mips-linux-gnu-g++")
SET(CMAKE_C_FLAGS "-pthread -fPIC -march=mips32r2")
SET(CMAKE_CXX_FLAGS "-std=c++11 -pthread -fPIC")
PROJECT(osd_client_lvgl_demo_widget)


add_definitions(-DLV_CONF_INCLUDE_SIMPLE)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -g0 ")


add_executable(osd_client_lvgl_demo_widget
    ${PROJECT_SOURCE_DIR}/osd_client_lvgl_demo_widgets.c
    ${PROJECT_SOURCE_DIR}/lv_demo_widgets.c
    ${PROJECT_SOURCE_DIR}/mouse_cursor_icon.c
    ${PROJECT_SOURCE_DIR}/assets/img_clothes.c
    ${PROJECT_SOURCE_DIR}/assets/img_demo_widgets_avatar.c
    ${PROJECT_SOURCE_DIR}/assets/img_lvgl_logo.c
)

SET_TARGET_PROPERTIES("osd_client_lvgl_demo_widget" PROPERTIES OUTPUT_NAME "osd_client_lvgl_demo_widget")

target_link_libraries(osd_client_lvgl_demo_widget lvgl lv_drivers ts cbinder systemServer)

install(FILES ${CMAKE_BINARY_DIR}/osd_client_lvgl_demo_widget DESTINATION /usr/bin/ PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
