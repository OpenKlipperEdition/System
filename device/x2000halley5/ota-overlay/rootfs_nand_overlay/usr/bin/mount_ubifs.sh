#!/bin/sh

if [ -z "$1" ] || [ -z "$2" ]; then
    echo "Usage: $0 <MTD_partition_name_or_dev> <mount_path>"
    exit 1
fi

mtd_partition_name_or_dev=$1
mount_path=$2

mtd_partition_num=-1

case $mtd_partition_name_or_dev in
    /dev/mtd[0-9]*)
        mtd_partition_num=${mtd_partition_name_or_dev#/dev/mtd}
        ;;
    *)
        while IFS= read -r line; do
            if echo "$line" | grep -q "$mtd_partition_name_or_dev"; then
                mtd_partition_num=`echo "$line" | cut -d: -f1 | sed 's/^mtd//'`
                break
            fi
        done < /proc/mtd
        ;;
esac

if [ "$mtd_partition_num" = "-1" ]; then
    echo "Error: MTD partition with name or device '$mtd_partition_name_or_dev' not found"
    exit 1
fi

ubi_already_attached=false
ubi_device=""
for ubi_mtd_num_file in /sys/class/ubi/*/mtd_num; do
    if [ -f "$ubi_mtd_num_file" ]; then
        mtd_num=`cat $ubi_mtd_num_file`
        if [ "$mtd_num" -eq "$mtd_partition_num" ]; then
            ubi_already_attached=true
            ubi_device=`dirname $ubi_mtd_num_file | xargs basename`
            break
        fi
    fi
done

if [ "$ubi_already_attached" = "false" ]; then
    ubi=0
    for existing_ubi in `ls /sys/class/ubi/ 2>/dev/null`; do
        existing_ubi_num=`echo $existing_ubi | sed 's/^ubi//'`
        if [ "$existing_ubi_num" -ge "$ubi" ]; then
            ubi=$((existing_ubi_num + 1))
        fi
    done

    ubi_device="ubi$ubi"

    if ! ubiattach -m $mtd_partition_num -d $ubi; then
        if ! flash_erase /dev/mtd$mtd_partition_num 0 0; then
            echo "Error: Failed to erase MTD partition /dev/mtd$mtd_partition_num"
            exit 1
        fi

        if ! ubiattach -m $mtd_partition_num -d $ubi; then
            echo "Error: Failed to attach UBI device $ubi_device to MTD partition /dev/mtd$mtd_partition_num"
            exit 1
        fi
    fi
fi

if [ ! -e "/dev/${ubi_device}_0" ]; then
    if ! ubimkvol /dev/$ubi_device -N rootfs -m; then
        echo "Error: Failed to create UBI volume on $ubi_device"
        exit 1
    fi
fi

if [ ! -d "$mount_path" ]; then
    if ! mkdir -p "$mount_path"; then
        echo "Error: Failed to create mount path $mount_path"
        exit 1
    fi
fi

if ! mount -t ubifs /dev/${ubi_device}_0 $mount_path; then
    echo "Error: Failed to mount UBI volume /dev/${ubi_device}_0 to $mount_path"
    exit 1
fi

echo "Successfully attached and mounted UBI device $ubi_device to $mount_path"
