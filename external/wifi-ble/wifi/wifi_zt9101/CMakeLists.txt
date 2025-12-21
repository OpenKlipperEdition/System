CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(install-wifi-zt9101-firmware)

set(WIFI_EXPORT_ENV ${PROJECT_SOURCE_DIR}/../export_env.sh)
set(WIFI_ENV_FILE ${DESTDIR}/etc/profile.d/env_setup.sh)

install(PROGRAMS ${PROJECT_SOURCE_DIR}/wpa_bin/wifi_up.sh DESTINATION /bin/)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/wpa_bin/wifi_down.sh DESTINATION /bin/)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/wpa_bin/wifi_download_fw_and_up.sh DESTINATION /bin/)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/wpa_bin/wifi_auto_start_up.sh DESTINATION /etc/profile.d/)

install(PROGRAMS ${PROJECT_SOURCE_DIR}/ZT9101_fw_7128e8e.bin
		 ${PROJECT_SOURCE_DIR}/ZT9101V30_fw_8546b3a.bin
		 ${PROJECT_SOURCE_DIR}/wifi.cfg
		DESTINATION /lib/firmware/)

include(${PROJECT_SOURCE_DIR}/../export_env.cmake)
