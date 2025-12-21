# Must be equal to the value of macro "WIFIMAC_ADDR_PATH" in the firmware
SET(WIFIMAC_ADDR_PATH "/data/misc/wifi/")
# "wpa_supplicant.conf file path" and depends on WPA_SUPPLICANT(default)
SET(WIFI_WPA_SUPPLICANT_CONF "/etc/wpa_supplicant.conf")
# "wifi auto startup" and depends on WPA_SUPPLICANT(default)
SET(WIFI_AUTO_STARTUP y)
# "hostap interface" and depends on WIFI_AP_MODE
set(WIFI_HOSTP_INTERFACE "wlan0")
# "hostapd.conf file path" and depends on WIFI_AP_MODE
set(WIFI_HOSTAP_CONF "/etc/hostapd.conf")

execute_process(COMMAND sh -c "${WIFI_EXPORT_ENV} env_wifi_mac_addr_path \"${WIFIMAC_ADDR_PATH}\" ${WIFI_ENV_FILE}"
		COMMAND sh -c "${WIFI_EXPORT_ENV} env_wifi_enable_when_system_up \"${WIFI_AUTO_STARTUP}\" ${WIFI_ENV_FILE}"
		COMMAND sh -c "${WIFI_EXPORT_ENV} env_wifi_wpa_supplicant_conf \"${WIFI_WPA_SUPPLICANT_CONF}\" ${WIFI_ENV_FILE}"
		COMMAND sh -c "${WIFI_EXPORT_ENV} env_wifi_hostap_conf \"${WIFI_HOSTAP_CONF}\" ${WIFI_ENV_FILE}"
		COMMAND sh -c "${WIFI_EXPORT_ENV} env_wifi_hostap_interface \"${WIFI_HOSTP_INTERFACE}\" ${WIFI_ENV_FILE}"
	       )
