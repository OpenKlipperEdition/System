CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(install-wifi-88w8987-firmware)

set(WIFI_EXPORT_ENV ${PROJECT_SOURCE_DIR}/../export_env.sh)
set(WIFI_ENV_FILE ${DESTDIR}/etc/profile.d/env_setup.sh)

install(PROGRAMS ${PROJECT_SOURCE_DIR}/wpa_bin/wifi_up.sh DESTINATION /bin/)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/wpa_bin/wifi_down.sh DESTINATION /bin/)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/wpa_bin/wifi_download_fw_and_up.sh DESTINATION /bin/)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/wpa_bin/wifi_auto_start_up.sh DESTINATION /etc/profile.d/)

install(PROGRAMS ${PROJECT_SOURCE_DIR}/nxp/ed_mac_ctrl_V3_8987.conf
		 ${PROJECT_SOURCE_DIR}/nxp/sd8987_wlan.bin
		 ${PROJECT_SOURCE_DIR}/nxp/sdiouart8987_combo_v0.bin
		 ${PROJECT_SOURCE_DIR}/nxp/txpwrlimit_cfg_8987.conf
		 ${PROJECT_SOURCE_DIR}/nxp/uart8987_bt_v0.bin
		 ${PROJECT_SOURCE_DIR}/wifi_mod_para.conf
		 DESTINATION /lib/firmware/nxp)

include(${PROJECT_SOURCE_DIR}/../export_env.cmake)
