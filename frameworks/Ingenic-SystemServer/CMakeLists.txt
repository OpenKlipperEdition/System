CMAKE_MINIMUM_REQUIRED(VERSION 3.27)
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_C_COMPILER "mips-linux-gnu-gcc")
SET(CMAKE_CXX_COMPILER "mips-linux-gnu-g++")
SET(CMAKE_C_FLAGS "-pthread -fPIC -march=mips32r2 -DBINDER_IPC_32BIT=1")
SET(CMAKE_CXX_FLAGS "-std=c++11 -pthread -fPIC")
PROJECT(Ingenic-SystemServer)


SET(BINDER_FILES
	${PROJECT_SOURCE_DIR}/src/common/binder/src/binder_hal.c
	${PROJECT_SOURCE_DIR}/src/common/binder/src/binder_io.c
	${PROJECT_SOURCE_DIR}/src/common/binder/src/binder_ipc.c
)

SET(SYSLOG_FILES
	${PROJECT_SOURCE_DIR}/src/common/syslog/log.c
)

SET(FIFO_FILES
	${PROJECT_SOURCE_DIR}/src/common/fifo/fifo.c
)

SET(TIMER_FILES
	${PROJECT_SOURCE_DIR}/src/common/timer/mt_timer.c
)

SET(CAMERA_CLIENTS
	${PROJECT_SOURCE_DIR}/src/client/camera.c
)

SET(ENCODER_CLIENTS
	${PROJECT_SOURCE_DIR}/src/client/encoder.c
)

SET(WDT_CLIENTS
	${PROJECT_SOURCE_DIR}/src/client/watchdog_client.c
)

SET(POWER_CLIENTS
	${PROJECT_SOURCE_DIR}/src/client/power_client.c
)

SET(BACKLIGHT_CLIENTS
	${PROJECT_SOURCE_DIR}/src/client/backlight_client.c
)

SET(EVENT_CLIENTS
	${PROJECT_SOURCE_DIR}/src/client/event_client.c
)

SET(OSD_CLIENTS
	${PROJECT_SOURCE_DIR}/src/client/osd.c
)

SET(NETWORK_CLIENTS
	${PROJECT_SOURCE_DIR}/src/client/network.c
)

SET(CLIENTS_FILES
	${CAMERA_CLIENTS}
	${ENCODER_CLIENTS}
	${WDT_CLIENTS}
	${POWER_CLIENTS}
	${BACKLIGHT_CLIENTS}
	${EVENT_CLIENTS}
	${OSD_CLIENTS}
	${NETWORK_CLIENTS}
)

SET(WPA_FILES ${PROJECT_SOURCE_DIR}/src/common/libwpa/os_unix.c
		${PROJECT_SOURCE_DIR}/src/common/libwpa/wpa_ctrl.c
)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/include
					${PROJECT_SOURCE_DIR}/include/common
					${PROJECT_SOURCE_DIR}/src/client
					${PROJECT_SOURCE_DIR}/src/common
					${PROJECT_SOURCE_DIR}/src/common/binder/include
					${PROJECT_SOURCE_DIR}/src/common/syslog
					${PROJECT_SOURCE_DIR}/src/common/fifo
					${PROJECT_SOURCE_DIR}/src/common/list
					${PROJECT_SOURCE_DIR}/src/common/timer
					${PROJECT_SOURCE_DIR}/src/common/libwpa
					${PROJECT_SOURCE_DIR}/src/service
)

find_library(IMPPLIB NAMES impp PATHS ${CMAKE_BINARY_DIR}/../../../../)
ADD_LIBRARY(cbinder SHARED ${BINDER_FILES})
ADD_LIBRARY(systemServer SHARED ${SYSLOG_FILES} ${FIFO_FILES}  ${CLIENTS_FILES} ${BINDER_FILES})
ADD_LIBRARY(wpa SHARED ${WPA_FILES})

add_executable(binder_service_manager ${PROJECT_SOURCE_DIR}/src/common/binder/src/binder_srv_manager.c)
add_executable(EventServer ${SYSLOG_FILES} ${PROJECT_SOURCE_DIR}/src/service/eventServer.c ${FIFO_FILES} ${TIMER_FILES})
add_executable(WatchdogServer ${SYSLOG_FILES} ${PROJECT_SOURCE_DIR}/src/service/watchdogServer.c ${FIFO_FILES} ${TIMER_FILES})
add_executable(PowerServer ${SYSLOG_FILES} ${PROJECT_SOURCE_DIR}/src/service/powerManagerServer.c ${FIFO_FILES} ${TIMER_FILES})
add_executable(BacklightServer ${SYSLOG_FILES} ${PROJECT_SOURCE_DIR}/src/service/backlightServer.c ${FIFO_FILES} ${TIMER_FILES})
add_executable(OsdServer ${SYSLOG_FILES} ${PROJECT_SOURCE_DIR}/src/service/osd_service.c ${FIFO_FILES} ${TIMER_FILES})
add_executable(cameraService ${SYSLOG_FILES} ${PROJECT_SOURCE_DIR}/src/service/camera_service.c ${FIFO_FILES})
add_executable(encoderService ${SYSLOG_FILES} ${PROJECT_SOURCE_DIR}/src/service/encoder_service.c ${FIFO_FILES})



add_executable(networkService ${SYSLOG_FILES} ${PROJECT_SOURCE_DIR}/src/service/network_service.c ${FIFO_FILES})
TARGET_LINK_LIBRARIES(systemServer ${IMPPLIB})
TARGET_LINK_LIBRARIES(binder_service_manager cbinder)
TARGET_LINK_LIBRARIES(EventServer cbinder)
TARGET_LINK_LIBRARIES(WatchdogServer cbinder)
TARGET_LINK_LIBRARIES(PowerServer cbinder)
TARGET_LINK_LIBRARIES(BacklightServer cbinder)
TARGET_LINK_LIBRARIES(OsdServer cbinder ${IMPPLIB})
TARGET_LINK_LIBRARIES(cameraService cbinder ${IMPPLIB})
TARGET_LINK_LIBRARIES(encoderService cbinder ${IMPPLIB})
TARGET_LINK_LIBRARIES(networkService cbinder wpa)


install(FILES ${CMAKE_BINARY_DIR}/libcbinder.so DESTINATION /usr/lib PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install(FILES ${CMAKE_BINARY_DIR}/libsystemServer.so DESTINATION /usr/lib PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install(FILES ${CMAKE_BINARY_DIR}/libwpa.so DESTINATION /usr/lib PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

SET(INSTALL_SERVER_EXECUTE
	${CMAKE_BINARY_DIR}/binder_service_manager
	${CMAKE_BINARY_DIR}/EventServer
	${CMAKE_BINARY_DIR}/WatchdogServer
	${CMAKE_BINARY_DIR}/PowerServer
	${CMAKE_BINARY_DIR}/BacklightServer
	${CMAKE_BINARY_DIR}/OsdServer
	${CMAKE_BINARY_DIR}/cameraService
	${CMAKE_BINARY_DIR}/encoderService
	${CMAKE_BINARY_DIR}/networkService
)
install(FILES ${INSTALL_SERVER_EXECUTE} DESTINATION /usr/bin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

SET(INSTALL_INCLUDE_FILE
	${PROJECT_SOURCE_DIR}/include/backlight_client.h
	${PROJECT_SOURCE_DIR}/include/encoder.h
	${PROJECT_SOURCE_DIR}/include/event_client.h
	${PROJECT_SOURCE_DIR}/include/iss_camera.h
	${PROJECT_SOURCE_DIR}/include/iss_common.h
	${PROJECT_SOURCE_DIR}/include/iss_osd.h
	${PROJECT_SOURCE_DIR}/include/power_client.h
	${PROJECT_SOURCE_DIR}/include/systemserver_config.h
	${PROJECT_SOURCE_DIR}/include/watchdog_client.h
)
install(FILES ${INSTALL_INCLUDE_FILE}  DESTINATION /usr/include )


add_subdirectory(test/unitTest/camera)
add_subdirectory(test/unitTest/osd)
add_subdirectory(test/unitTest/encoder)
add_subdirectory(test/unitTest/network)
add_subdirectory(test/test_demo)


