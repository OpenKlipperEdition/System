#!/bin/sh
echo "in update_play:param:$1"
if [ -f "$1" ];then
	/usr/bin/aplay $1
else
	echo "no $1 file found"
fi
