


INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/include
                    ${PROJECT_SOURCE_DIR}/include/common
                    ${PROJECT_SOURCE_DIR}/include/mpp
                    ${PROJECT_SOURCE_DIR}/src/client
                    ${PROJECT_SOURCE_DIR}/src/service
                    ${PROJECT_SOURCE_DIR}/src/common
                    ${PROJECT_SOURCE_DIR}/src/common/binder/include
                    ${PROJECT_SOURCE_DIR}/src/common/syslog
                    ${PROJECT_SOURCE_DIR}/src/common/fifo
                    ${PROJECT_SOURCE_DIR}/src/common/list
                    ${PROJECT_SOURCE_DIR}/src/common/timer
                    ${PROJECT_SOURCE_DIR}/src/service
)


add_executable(camera_demo camera_test.c)
add_executable(encoder_demo encoder_test.c)

add_executable(test_client test_client.c)
add_executable(wdt_client wdt_client.c)
add_executable(power_client power_client.c)
add_executable(backlight_client backlight_client_test.c)
add_executable(evt_client event_client_test.c)
add_executable(network_example network_example.c)
#add_executable(osd_client osd_client_test.c)


TARGET_LINK_LIBRARIES(camera_demo cbinder systemServer)
TARGET_LINK_LIBRARIES(encoder_demo cbinder systemServer)
TARGET_LINK_LIBRARIES(test_client cbinder systemServer)
TARGET_LINK_LIBRARIES(wdt_client cbinder systemServer)
TARGET_LINK_LIBRARIES(power_client cbinder systemServer)
TARGET_LINK_LIBRARIES(backlight_client cbinder systemServer)
TARGET_LINK_LIBRARIES(evt_client cbinder systemServer)
TARGET_LINK_LIBRARIES(network_example  systemServer)
#TARGET_LINK_LIBRARIES(osd_client cbinder systemServer)

install(FILES ${CMAKE_BINARY_DIR}/test/test_demo/network_example DESTINATION /testsuite/systemServerDemo PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install(FILES ${CMAKE_BINARY_DIR}/test/test_demo/test_client DESTINATION /testsuite/systemServerDemo PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install(FILES ${CMAKE_BINARY_DIR}/test/test_demo/wdt_client DESTINATION /testsuite/systemServerDemo PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install(FILES ${CMAKE_BINARY_DIR}/test/test_demo/power_client DESTINATION /testsuite/systemServerDemo  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install(FILES ${CMAKE_BINARY_DIR}/test/test_demo/backlight_client DESTINATION /testsuite/systemServerDemo PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install(FILES ${CMAKE_BINARY_DIR}/test/test_demo/evt_client DESTINATION /testsuite/systemServerDemo PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
#install(FILES ${CMAKE_BINARY_DIR}/test/test_demo/osd_client DESTINATION /testsuite/systemServerDemo PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install(FILES  ${CMAKE_BINARY_DIR}/test/test_demo/camera_demo DESTINATION /testsuite/systemServerDemo PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install(FILES  ${CMAKE_BINARY_DIR}/test/test_demo/encoder_demo DESTINATION /testsuite/systemServerDemo PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

#install(DIRECTORY ./osd_test_src/ DESTINATION /)
