#!/bin/sh
rfkill unblock wifi
ifconfig mlan0 up
ifconfig lo up

#wpa_supplicant -B -i mlan0 -c ${env_wifi_wpa_supplicant_conf} &
wpa_supplicant -i mlan0 -c ${env_wifi_wpa_supplicant_conf} &

# Networking configuration IP address timeout for 5 seconds
#udhcpc -t 5 -T 1 -q -n -i mlan0
udhcpc -i mlan0
if [ $? -ne 0 ]
then
     killall -9 wpa_supplicant
fi
