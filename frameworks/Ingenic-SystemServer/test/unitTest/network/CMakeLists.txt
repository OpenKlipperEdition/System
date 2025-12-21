


# SET(SER_SRC_DIR ${PROJECT_SOURCE_DIR}/src/service/camera_service.c)


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


add_executable(network_serUTest ${CMAKE_CURRENT_SOURCE_DIR}/network_ser_unitest.c  ${FIFO_FILES} ${SYSLOG_FILES})
add_executable(networkClitest ${CMAKE_CURRENT_SOURCE_DIR}/network_cli_test.c  ${FIFO_FILES} ${SYSLOG_FILES})
TARGET_LINK_LIBRARIES(network_serUTest cbinder)
TARGET_LINK_LIBRARIES(networkClitest cbinder)
# TARGET_LINK_LIBRARIES(network_serUTest cunit)
