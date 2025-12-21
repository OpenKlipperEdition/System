#!/bin/sh
/usr/data/ota_res/recovery
if [ $? != 0 ]
then
    /sbin/poweroff
else
    /bin/echo a5a5a5a5 > /usr/data/STAGE
    /bin/sync
    /sbin/reboot
fi

