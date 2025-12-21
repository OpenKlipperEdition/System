CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(install-wifi-ap-mode)

set(WIFI_EXPORT_ENV ${PROJECT_SOURCE_DIR}/../export_env.sh)
set(WIFI_ENV_FILE ${DESTDIR}/etc/profile.d/env_setup.sh)

include(${PROJECT_SOURCE_DIR}/../export_env.cmake)

file(COPY ${PROJECT_SOURCE_DIR}/etc/ DESTINATION ${DESTDIR}/etc/)
execute_process(COMMAND sh -c "sed -i 's/your_wifi_interface/${WIFI_HOSTP_INTERFACE}/' ${DESTDIR}/etc/hostapd.conf"
				COMMAND sh -c "sed -i 's/your_wifi_interface/${WIFI_HOSTP_INTERFACE}/' ${DESTDIR}/etc/udhcpd.conf")
install(PROGRAMS ${PROJECT_SOURCE_DIR}/wifi_ap_mode_stop.sh ${PROJECT_SOURCE_DIR}/wifi_ap_mode_start.sh
		DESTINATION /usr/bin/)
