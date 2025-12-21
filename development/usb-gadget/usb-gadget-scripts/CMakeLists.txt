CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(install-usb-scripts)

add_executable(udc_daemon_c udc_daemon_c.c)

install(PROGRAMS ${PROJECT_SOURCE_DIR}/S90usb DESTINATION /etc/init.d/)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/usb/acm              DESTINATION /etc/init.d/usb)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/usb/adb              DESTINATION /etc/init.d/usb)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/usb/keyboard         DESTINATION /etc/init.d/usb)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/usb/mouse            DESTINATION /etc/init.d/usb)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/usb/mass_storage     DESTINATION /etc/init.d/usb)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/usb/mtp              DESTINATION /etc/init.d/usb)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/usb/mtp_daemon       DESTINATION /etc/init.d/usb)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/usb/printer          DESTINATION /etc/init.d/usb)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/usb/ncm              DESTINATION /etc/init.d/usb)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/usb/rndis            DESTINATION /etc/init.d/usb)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/usb/serial           DESTINATION /etc/init.d/usb)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/usb/uac1             DESTINATION /etc/init.d/usb)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/usb/usb_wakeup       DESTINATION /etc/init.d/usb)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/usb/uvc              DESTINATION /etc/init.d/usb)
# Replace script "udc_daemon" with C code implement, which will save 1M of running memory.
# install(PROGRAMS ${PROJECT_SOURCE_DIR}/usb/udc_daemon       DESTINATION /etc/init.d/usb)
install(TARGETS udc_daemon_c              DESTINATION /etc/init.d/usb/)
