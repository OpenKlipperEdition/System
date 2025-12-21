CMAKE_MINIMUM_REQUIRED(VERSION 3.8)

PROJECT(install-mount-userdata)

IF(STORAGE_MEDIUM STREQUAL "nand")
	install(PROGRAMS ${PROJECT_SOURCE_DIR}/S21mount DESTINATION /etc/init.d/)
	install(PROGRAMS ${PROJECT_SOURCE_DIR}/mount_ubifs.sh DESTINATION /usr/bin/)
ELSEIF(STORAGE_MEDIUM STREQUAL "msc")
	install(PROGRAMS ${PROJECT_SOURCE_DIR}/S21data DESTINATION /etc/init.d/)
	install(PROGRAMS ${PROJECT_SOURCE_DIR}/mount_mmc.sh DESTINATION /usr/bin/)
ELSE()
	install(CODE "message(STATUS \"The ${STORAGE_MEDIUM} ${PROJECT_NAME} is not support!!!\")")
ENDIF()
