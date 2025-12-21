CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(install-wifi-ap6203-firmware)

set(WIFI_EXPORT_ENV ${PROJECT_SOURCE_DIR}/../../export_env.sh)
set(WIFI_ENV_FILE ${DESTDIR}/etc/profile.d/env_setup.sh)

install(PROGRAMS ${PROJECT_SOURCE_DIR}/../wpa_bin/wifi_up.sh DESTINATION /bin/)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/../wpa_bin/wifi_down.sh DESTINATION /bin/)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/../wpa_bin/wifi_download_fw_and_up.sh DESTINATION /bin/)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/../wpa_bin/wifi_auto_start_up.sh DESTINATION /etc/profile.d/)

install(FILES ${PROJECT_SOURCE_DIR}/fw_bcm43013c1_ag.bin
			  ${PROJECT_SOURCE_DIR}/nvram_ap6203bm.txt
		DESTINATION /lib/firmware/)

include(${PROJECT_SOURCE_DIR}/../../export_env.cmake)
