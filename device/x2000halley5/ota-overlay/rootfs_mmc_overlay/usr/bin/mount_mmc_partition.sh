#!/bin/sh
PRG_NAME=$0
MOUNT_PARTITION=$1
MOUNT_PATH=$2

# 显示错误和使用方法并退出
usage_exit()
{
    echo $1
    echo "usage: ${PRG_NAME} mount_partition mount_path"
    exit 1
}

# 显示错误并退出
error_exit()
{
    echo $1
    exit 1
}

if [ "${MOUNT_PATH}" = "" ]
then
	usage_exit "error: too few args"
fi

if [ "$3" != "" ]
then
	usage_exit "error: too many args"
fi


if [ ! -d ${MOUNT_PATH} ]
then
	mkdir -p ${MOUNT_PATH}
fi

mountpoint -q ${MOUNT_PATH}
if [ "$?" -eq 0 ]
then
	echo "waring: ${MOUNT_PATH} is a mountpoint that will be uninstalled and remounted"
	umount ${MOUNT_PATH}
fi

mount | grep -q ${MOUNT_PARTITION}
if [ "$?" -eq 0 ]
then
	mount | grep ${MOUNT_PARTITION} | grep ${MOUNT_PATH}
	if [ "$?" -ne 0 ]
	then
		MOUNT_RESULT=`mount | grep ${MOUNT_PARTITION}`
		echo "${MOUNT_PARTITION} is mounted : ${MOUNT_RESULT}"
	fi
	/bin/umount ${MOUNT_PARTITION}
fi

mount -t ext4 -o defaults,noatime,nodiratime,nobarrier,discard ${MOUNT_PARTITION} ${MOUNT_PATH}
if [ $? -ne 0 ]
then
	echo "mke2fs -t ext4 ${MOUNT_PARTITION}"
	mke2fs ${MOUNT_PARTITION}
	mount -t ext4 -o defaults,noatime,nodiratime,nobarrier,discard ${MOUNT_PARTITION} ${MOUNT_PATH}
	if [ $? -eq 0 ]
	then
		echo "success: Successfully mounted ${MOUNT_PARTITION} to ${MOUNT_PATH}"
	fi
else
	echo "success: Successfully mounted ${MOUNT_PARTITION} to ${MOUNT_PATH}"
fi
