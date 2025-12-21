#!/bin/sh
killall -9 udhcpc
killall -9 wpa_supplicant
ifconfig mlan0 0.0.0.0
ifconfig mlan0 down
rfkill block wifi
