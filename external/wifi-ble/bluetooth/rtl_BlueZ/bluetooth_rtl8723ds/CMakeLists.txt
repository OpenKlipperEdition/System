CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(install-bt-rtl8723ds-firmware)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/bluetooth_rtl8723ds/bin/ DESTINATION /usr/bin)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/bluetooth_rtl8723ds/rtlbt/ DESTINATION /firmware)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/bluetooth_rtl8723ds/etc/dbus-1/system.d/ DESTINATION /etc/dbus-1/system.d)
