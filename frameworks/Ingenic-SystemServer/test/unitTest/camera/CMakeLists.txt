


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


add_executable(cam_serUTest ${CMAKE_CURRENT_SOURCE_DIR}/camera_ser_utest.c  ${FIFO_FILES} ${SYSLOG_FILES})
TARGET_LINK_LIBRARIES(cam_serUTest cbinder)
TARGET_LINK_LIBRARIES(cam_serUTest cunit)
