#!/bin/sh

count=0

while [ ! -e "/sys/class/net/wlan0/address" ]; do
    if [ "$count" != "10" ]; then
        let count=count+1;
        echo "waiting for /sys/class/net/wlan0/address ....."
        sleep 1;
    else
        echo "failed to wait /sys/class/net/wlan0/address ....."
        echo "do not set mac address, do not wifi_up"
		exit 1
    fi
done

echo "env_wifi_wpa_supplicant_conf ${env_wifi_wpa_supplicant_conf}"
echo "env_wifi_mac_addr_path ${env_wifi_mac_addr_path}"
echo "env_wifi_enable_when_system_up ${env_wifi_enable_when_system_up}"

if [ ! -e ${env_wifi_wpa_supplicant_conf} ]; then
    mkdir -p ${env_wifi_wpa_supplicant_conf%/*}
    echo "ctrl_interface=/var/run/wpa_supplicant" >  ${env_wifi_wpa_supplicant_conf}
    echo "update_config=1"                        >> ${env_wifi_wpa_supplicant_conf}
    echo "country=GB"                             >> ${env_wifi_wpa_supplicant_conf}
	echo "The ${env_wifi_wpa_supplicant_conf} file is in an unconfigured state, lacking the SSID and PSK for the target wireless network."
	exit 1
fi

echo "env_wifi_hostap_conf ${env_wifi_hostap_conf}"

if [ ! -e ${env_wifi_hostap_conf} ]; then
    mkdir -p ${env_wifi_hostap_conf%/*}
    cp /etc/hostapd.conf ${env_wifi_hostap_conf} -v
fi

WIFI_MAC_ADDR=$(macgen)
echo -n "$WIFI_MAC_ADDR">/tmp/wifimac.txt


if [ "${env_wifi_enable_when_system_up}" == "y" ];then
	grep_result=$(grep -qE "^(ssid=|\s+ssid=)" "${env_wifi_wpa_supplicant_conf}")
	if [ "$?" -eq 0 ]
	then
		wifi_up.sh
	else
		echo "The ${env_wifi_wpa_supplicant_conf} file is in an unconfigured state, lacking the SSID for the target wireless network."
		exit 1
	fi
fi
