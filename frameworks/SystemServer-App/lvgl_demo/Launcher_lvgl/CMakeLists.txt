CMAKE_MINIMUM_REQUIRED(VERSION 3.27)
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_C_COMPILER "mips-linux-gnu-gcc")
SET(CMAKE_CXX_COMPILER "mips-linux-gnu-g++")
SET(CMAKE_C_FLAGS "-pthread -fPIC -march=mips32r2")
SET(CMAKE_CXX_FLAGS "-std=c++11 -pthread -fPIC")
PROJECT(osd_client_launcher_lvgl_widgets)


add_definitions(-DLV_CONF_INCLUDE_SIMPLE)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -g0 -DLV_CONF_INCLUDE_SIMPLE -DLV_LVGL_H_INCLUDE_SIMPLE")


INCLUDE_DIRECTORIES(
	${SYSROOT_DIR}/usr/include/lvgl
	${SYSROOT_DIR}/usr/include/lvgl/lv_drivers
)

ADD_LIBRARY(clockapp SHARED ${PROJECT_SOURCE_DIR}/app_calendar.c ${PROJECT_SOURCE_DIR}/clock_bg.c ${PROJECT_SOURCE_DIR}/clock_hou.c ${PROJECT_SOURCE_DIR}/clock_min.c ${PROJECT_SOURCE_DIR}/clock_sec.c)
TARGET_LINK_LIBRARIES(clockapp lvgl dl)

ADD_LIBRARY(cameraapp SHARED ${PROJECT_SOURCE_DIR}/app_camera.c ${PROJECT_SOURCE_DIR}/take_photos.c ${PROJECT_SOURCE_DIR}/take_videos.c ${PROJECT_SOURCE_DIR}/record_start.c ${PROJECT_SOURCE_DIR}/back_home.c)
TARGET_LINK_LIBRARIES(cameraapp lvgl dl freetype pthread)

ADD_LIBRARY(playerapp SHARED ${PROJECT_SOURCE_DIR}/app_player.c ${PROJECT_SOURCE_DIR}/icon_img.c)
TARGET_LINK_LIBRARIES(playerapp lvgl dl ffplay)

add_executable(osd_client_launcher_lvgl_widgets
            ${PROJECT_SOURCE_DIR}/osd_client_launcher_lvgl_widgets.c
            ${PROJECT_SOURCE_DIR}/launcher.c
            ${PROJECT_SOURCE_DIR}/launcher_background_image.c
            ${PROJECT_SOURCE_DIR}/icon_img.c
            ${PROJECT_SOURCE_DIR}/key_back.c
            ${PROJECT_SOURCE_DIR}/key_home.c
            ${PROJECT_SOURCE_DIR}/key_tool.c
)


SET_TARGET_PROPERTIES("osd_client_launcher_lvgl_widgets" PROPERTIES OUTPUT_NAME "osd_client_launcher_lvgl_widgets")
target_link_libraries(osd_client_launcher_lvgl_widgets lvgl lv_drivers ts cbinder systemServer dl freetype)


install(FILES ${CMAKE_BINARY_DIR}/osd_client_launcher_lvgl_widgets DESTINATION /usr/bin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install(FILES ${CMAKE_BINARY_DIR}/libclockapp.so DESTINATION /usr/lib PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install(FILES ${CMAKE_BINARY_DIR}/libcameraapp.so DESTINATION /usr/lib PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)


install(DIRECTORY ${PROJECT_SOURCE_DIR}/assets/
        DESTINATION /assets
		USE_SOURCE_PERMISSIONS
)
install(FILES ${CMAKE_BINARY_DIR}/libplayerapp.so DESTINATION /usr/lib PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
