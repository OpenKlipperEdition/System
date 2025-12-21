project(LVGL_DEMO_WIDGETS)
cmake_minimum_required(VERSION 3.8.10)
add_definitions(-DLV_CONF_INCLUDE_SIMPLE)
set (CMAKE_C_FLAGS "-O2 -g0 ")
add_executable(LVGL_DEMO_WIDGET
	assets/img_clothes.c
	assets/img_demo_widgets_avatar.c
	assets/img_lvgl_logo.c
	lv_demo_widgets.c
	mouse_cursor_icon.c
	main.c
)

SET_TARGET_PROPERTIES("LVGL_DEMO_WIDGET" PROPERTIES OUTPUT_NAME "LVGL_DEMO_WIDGET")
target_link_libraries(LVGL_DEMO_WIDGET lvgl lv_drivers)
INSTALL(TARGETS LVGL_DEMO_WIDGET RUNTIME DESTINATION bin)

