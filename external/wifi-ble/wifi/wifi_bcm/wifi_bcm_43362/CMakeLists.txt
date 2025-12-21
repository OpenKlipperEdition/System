CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(install-wifi-bcm43362-firmware)

set(WIFI_EXPORT_ENV ${PROJECT_SOURCE_DIR}/../../export_env.sh)
set(WIFI_ENV_FILE ${DESTDIR}/etc/profile.d/env_setup.sh)

install(PROGRAMS ${PROJECT_SOURCE_DIR}/../wpa_bin/wifi_up.sh DESTINATION /bin/)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/../wpa_bin/wifi_down.sh DESTINATION /bin/)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/../wpa_bin/wifi_download_fw_and_up.sh DESTINATION /bin/)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/../wpa_bin/wifi_auto_start_up.sh DESTINATION /etc/profile.d/)

install(PROGRAMS ${PROJECT_SOURCE_DIR}/fw_bcmdhd-43362a2-apsta.bin
				 ${PROJECT_SOURCE_DIR}/nvram_ap6181.txt
		DESTINATION /lib/firmware/)

include(${PROJECT_SOURCE_DIR}/../../export_env.cmake)
