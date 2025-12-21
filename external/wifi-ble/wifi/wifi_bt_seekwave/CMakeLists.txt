CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(install-wifi-bt-seekwave)

set(WIFI_EXPORT_ENV ${PROJECT_SOURCE_DIR}/../export_env.sh)
set(WIFI_ENV_FILE ${DESTDIR}/etc/profile.d/env_setup.sh)

install(PROGRAMS ${PROJECT_SOURCE_DIR}/wifi_bt_bin/wifi_auto_start_up.sh DESTINATION /etc/profile.d/)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/wifi_bt_bin/wifi_download_fw_and_up.sh DESTINATION /bin/)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/wifi_bt_bin/wifi_up.sh DESTINATION /bin/)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/wifi_bt_bin/wifi_down.sh DESTINATION /bin/)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/wifi_bt_bin/seekwave_ea6xxxx/ DESTINATION /lib/firmware/)

include(${PROJECT_SOURCE_DIR}/../export_env.cmake)
