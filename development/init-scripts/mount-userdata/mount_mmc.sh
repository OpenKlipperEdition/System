#!/bin/sh

if [ -z "$1" ] || [ -z "$2" ]; then
	echo "Error: Partition number or name and mount point are required. Usage: $0 <partition_number|partition_name> <mount_point>"
	exit 1
fi

identifier=$1
mount_point=$2

if [[ "$identifier" =~ ^[0-9]+$ ]]; then
	DEVICE_PATH=$(grep 'mmcblk' /proc/partitions | awk -v part_num="$identifier" '$4 ~ "p" part_num {print "/dev/" $4}')
else
	part_num=$(fdisk -l 2>/dev/null | grep -E '^[[:space:]]+[0-9]+[[:space:]]' | awk -v name="$identifier" '$NF == name {print $1}')

	if [ -n "$part_num" ]; then
		DEVICE_PATH=$(grep 'mmcblk' /proc/partitions | awk -v part_num="$part_num" '$4 ~ "p" part_num {print "/dev/" $4}')
	else
		echo "Error: No matching partition found for name $identifier."
		exit 1
	fi
fi

if [ -z "$DEVICE_PATH" ] || [ ! -b "$DEVICE_PATH" ]; then
	echo "Error: No matching device found for identifier $identifier."
	exit 1
fi

echo "Found device: $DEVICE_PATH"

mkdir -p "$mount_point"

for attempt in $(seq 1 3); do
	if /bin/mount -t ext4 -o defaults,noatime,nodiratime,nobarrier,discard "$DEVICE_PATH" "$mount_point"; then
		echo "Mounted $DEVICE_PATH to $mount_point successfully."
		exit 0
	else
		echo "Failed to mount $DEVICE_PATH to $mount_point. Retrying... (Attempt $attempt/3)"
	fi
done

echo "Maximum mount attempts reached. Attempting to create a new filesystem and retry mounting..."
if /sbin/mke2fs -t ext4 "$DEVICE_PATH"; then
	echo "New filesystem created successfully."
	if /bin/mount -t ext4 -o defaults,noatime,nodiratime,nobarrier,discard "$DEVICE_PATH" "$mount_point"; then
		echo "Mounted $DEVICE_PATH to $mount_point successfully after creating a new filesystem."
		exit 0
	else
		echo "Failed to mount $DEVICE_PATH to $mount_point even after creating a new filesystem. Exiting."
		exit 1
	fi
else
	echo "Failed to create a new filesystem. Exiting!"
	exit 1
fi
