#!/bin/sh
ps -ef|grep -v grep|grep smarthome > /dev/null
if [ $? -ne 0 ]; then
	date -s "2022-04-28 12:01:00"
    smarthome &
fi
