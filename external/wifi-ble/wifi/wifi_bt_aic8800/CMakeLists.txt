CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(install-wifi-bt-aic8800)


install(PROGRAMS ${PROJECT_SOURCE_DIR}/wpa_bin/wifi_download_fw_and_up.sh DESTINATION /bin/)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/wpa_bin/wifi_up.sh DESTINATION /bin/)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/wpa_bin/wifi_down.sh DESTINATION /bin/)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/wpa_bin/wifi_auto_start_up.sh DESTINATION /etc/profile.d/)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/wifi_bt_firmware/ DESTINATION /vendor/etc/firmware/)

include(${PROJECT_SOURCE_DIR}/../export_env.cmake)
