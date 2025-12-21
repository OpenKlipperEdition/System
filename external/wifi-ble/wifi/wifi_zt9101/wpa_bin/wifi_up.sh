#!/bin/sh
rfkill unblock wifi
ifconfig wlan0 up
ifconfig lo up

#wpa_supplicant -B -i wlan0 -c ${env_wifi_wpa_supplicant_conf} &
wpa_supplicant -i wlan0 -c ${env_wifi_wpa_supplicant_conf} &

# Networking configuration IP address timeout for 5 seconds
#udhcpc -t 5 -T 1 -q -n -i wlan0
udhcpc -i wlan0
if [ $? -ne 0 ]
then
     killall -9 wpa_supplicant
fi
