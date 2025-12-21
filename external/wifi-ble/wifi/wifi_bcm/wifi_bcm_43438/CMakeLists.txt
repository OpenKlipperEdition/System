CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(install-wifi-bcm43438-firmware)

set(WIFI_EXPORT_ENV ${PROJECT_SOURCE_DIR}/../../export_env.sh)
set(WIFI_ENV_FILE ${DESTDIR}/etc/profile.d/env_setup.sh)

install(PROGRAMS ${PROJECT_SOURCE_DIR}/../wpa_bin/wifi_up.sh DESTINATION /bin/)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/../wpa_bin/wifi_down.sh DESTINATION /bin/)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/../wpa_bin/wifi_download_fw_and_up.sh DESTINATION /bin/)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/../wpa_bin/wifi_auto_start_up.sh DESTINATION /etc/profile.d/)

IF(KERNEL_VERSION STREQUAL "6.6")
install(FILES ${PROJECT_SOURCE_DIR}/new_fw/fw_bcm43438a1.bin ${PROJECT_SOURCE_DIR}/new_fw/nvram_ap6212a.txt
		DESTINATION /lib/firmware/)
ELSE()
file(REMOVE "${DESTDIR}/lib/firmware/fw_bcm43438a1.bin")
file(REMOVE "${DESTDIR}/lib/firmware/BCM43438_A1.cal")
file(GLOB BCM43438_FIRMWARES ${PROJECT_SOURCE_DIR}/fw_43438_a[01] ${PROJECT_SOURCE_DIR}/nv_43438_a[01])
file(COPY ${BCM43438_FIRMWARES} DESTINATION ${DESTDIR}/lib/firmware/wifi_bcm_43438)
execute_process(COMMAND sh -c "ln -s wifi_bcm_43438/fw_43438_a1 ${DESTDIR}/lib/firmware/fw_bcm43438a1.bin"
				COMMAND sh -c "ln -s wifi_bcm_43438/nv_43438_a1 ${DESTDIR}/lib/firmware/BCM43438_A1.cal")
install(FILES ${BCM43438_FIRMWARES} DESTINATION /lib/firmware/wifi_bcm_43438)
ENDIF()

include(${PROJECT_SOURCE_DIR}/../../export_env.cmake)
