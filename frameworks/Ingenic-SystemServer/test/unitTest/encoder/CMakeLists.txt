


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


add_executable(encoder_serUTest ${CMAKE_CURRENT_SOURCE_DIR}/encoder_ser_utest.c  ${FIFO_FILES} ${SYSLOG_FILES})
add_executable(encoder_UTest ${CMAKE_CURRENT_SOURCE_DIR}/encoder_utest.c ${PROJECT_SOURCE_DIR}/src/client/encoder.c  ${FIFO_FILES} ${SYSLOG_FILES})

TARGET_LINK_LIBRARIES(encoder_UTest cbinder cunit)

TARGET_LINK_LIBRARIES(encoder_serUTest cbinder)
TARGET_LINK_LIBRARIES(encoder_serUTest cunit)
