#!/usr/bin/env bash

base_path=$(cd `dirname $0`; pwd)/rootfs-recovery
out_path=$(dirname $1)
if [[ $2 = "mmc" ]];
then
	if [[ $3 = "x1600" ]];then
                if [[ $4 = "5.10" ]];then
					ramdisk_name=rootfs-recovery-mmc-x1600-glibc238
                else
					ramdisk_name=rootfs-recovery-mmc-x1600
                fi
	elif [[ $3 = "x2000" ]];then
		ramdisk_name=rootfs-recovery-mmc-x2000
	fi
elif [[ $2 = "nand" ]];
then
	if [[ $3 = "x1600" ]];then
                if [[ $4 = "5.10" ]];then
					ramdisk_name=rootfs-recovery-nand-x1600-glibc238
                else
					ramdisk_name=rootfs-recovery-nand-x1600
                fi
	elif [[ $3 = "x2500" ]];then
		if [[ $4 = "5.10" ]];then
			ramdisk_name=rootfs-recovery-nand-x2500-glibc238
		else
			ramdisk_name=rootfs-recovery-nand-x2500
		fi
	elif [[ $3 = "x2000" ]];then
                if [[ $4 = "5.10" ]];then
					ramdisk_name=rootfs-recovery-nand-x2000-glibc238
				else
					ramdisk_name=rootfs-recovery-nand-x2000
				fi
	elif [[ $3 = "m300" ]];then
		ramdisk_name=rootfs-recovery-nand-m300
	elif [[ $3 = "x2660" ]];then
		ramdisk_name=rootfs-recovery-nand-x2600
	elif [[ $3 = "x2670" ]];then
		ramdisk_name=rootfs-recovery-nand-x2600
    elif [[ $3 = "x2600e" ]];then
        if [[ $4 = "5.10" ]];then
            ramdisk_name=rootfs-recovery-nand-x2600e-glibc238
        else
            ramdisk_name=rootfs-recovery-nand-x2600e
        fi
    elif [[ $3 = "x2600" ]];then
        if [[ $4 = "5.10" ]];then
            ramdisk_name=rootfs-recovery-nand-x2600-glibc238
        else
            ramdisk_name=rootfs-recovery-nand-x2600
        fi
    elif [[ $3 = "x2670m" ]];then
        ramdisk_name=rootfs-recovery-nand-x2600

    fi
fi

echo $out_path
echo $base_path
echo $ramdisk_name
mkdir -p $out_path
mkdir -p $base_path/$ramdisk_name/dev
mkdir -p $base_path/$ramdisk_name/home
mkdir -p $base_path/$ramdisk_name/mnt
mkdir -p $base_path/$ramdisk_name/root
mkdir -p $base_path/$ramdisk_name/proc
mkdir -p $base_path/$ramdisk_name/sys
mkdir -p $base_path/$ramdisk_name/tmp
mkdir -p $base_path/$ramdisk_name/opt
mkdir -p $base_path/$ramdisk_name/run
mkdir -p $base_path/$ramdisk_name/var
mkdir -p $base_path/$ramdisk_name/var/run
mkdir -p $base_path/$ramdisk_name/var/lib/misc
mkdir -p $base_path/$ramdisk_name/usr/data
mkdir -p $base_path/$ramdisk_name/etc/boa
mkdir -p $base_path/$ramdisk_name/usr/lib
mkdir -p $base_path/$ramdisk_name/usr/lib


cd $base_path/$ramdisk_name
echo "mknod dev/console c 5 1;mknod dev/null c 1 3;find . | cpio -ov -H newc > $out_path/ramdisk.cpio" | fakeroot
cd -
gzip -f $out_path/ramdisk.cpio
