#!/bin/sh

# This file is executed by build/envsetup.sh, and can use anything
# defined in envsetup.sh.
#
# In particular, you can add lunch options with the add_lunch_combo
# function: add_lunch_combo generic-nand-eng
#
#   Note:
#   eng         --> compile in engineering mode
#   user        --> compile in user mode
#   userdebug   --> compile in userdebug mode, with all modules tag with
#                   eng,user,userdebug. gdb can be used to debug.
#
#  each word split by '_',
#  TARGET_DEVICE.TARGET_DEVICE_SUBVERSION split by '.',
#	1. char '.' is not allowed in TARGET_DEVICE_SUBVERSION
#	2. TARGET_DEVICE_SUBVERSION can be empty
#
# [TARGET_DEVICE.TARGET_DEVICE_SUBVERSION]_[TARGET_STORAGE_MEDIUM]_[TARGET_EXT_SUPPORT]_[TARGET_EXT2_SUPPORT]-eng
# [TARGET_DEVICE.TARGET_DEVICE_SUBVERSION]_[TARGET_STORAGE_MEDIUM]_[TARGET_EXT_SUPPORT]_[TARGET_EXT2_SUPPORT]-user
# [TARGET_DEVICE.TARGET_DEVICE_SUBVERSION]_[TARGET_STORAGE_MEDIUM]_[TARGET_EXT_SUPPORT]_[TARGET_EXT2_SUPPORT]-userdebug
#

KERNEL_DIRECTORY=$(find "$(pwd)/kernel" -maxdepth 1 -type d -name "kernel-4.4.94")
if [ -n "${KERNEL_DIRECTORY}" ]
then
	# use nand as main fs storage. replace 'halley5.v12' to the boardname.
	#add_lunch_combo x2000halley5.v10_nand_4.4.94-eng
	#add_lunch_combo x2000halley5.v10_nand_4.4.94-user
	#add_lunch_combo x2000halley5.v10_nand_4.4.94-userdebug

	## use nor as main fs storage
	#add_lunch_combo x2000halley5.v10_nor_4.4.94-eng
	#add_lunch_combo x2000halley5.v10_nor_4.4.94-user
	#add_lunch_combo x2000halley5.v10_nor_4.4.94-userdebug

	## use msc as main fs storage
	#add_lunch_combo x2000halley5.v10_msc_4.4.94-eng
	#add_lunch_combo x2000halley5.v10_msc_4.4.94-user
	#add_lunch_combo x2000halley5.v10_msc_4.4.94-userdebug

	## use nand as main fs storage for OTA
	#add_lunch_combo x2000halley5.v10_nand_4.4.94_ota-eng
	#add_lunch_combo x2000halley5.v10_nand_4.4.94_ota-user
	#add_lunch_combo x2000halley5.v10_nand_4.4.94_ota-userdebug

	# use nand as main fs storage.
	add_lunch_combo x2000halley5.v20_nand_4.4.94-eng
	#add_lunch_combo x2000halley5.v20_nand_4.4.94-user
	#add_lunch_combo x2000halley5.v20_nand_4.4.94-userdebug

	# use nor as main fs storage
	add_lunch_combo x2000halley5.v20_nor_4.4.94-eng
	#add_lunch_combo x2000halley5.v20_nor_4.4.94-user
	#add_lunch_combo x2000halley5.v20_nor_4.4.94-userdebug

	# use msc as main fs storage
	add_lunch_combo x2000halley5.v20_msc_4.4.94-eng
	#add_lunch_combo x2000halley5.v20_msc_4.4.94-user
	#add_lunch_combo x2000halley5.v20_msc_4.4.94-userdebug

	# use nand as main fs storage for OTA
	add_lunch_combo x2000halley5.v20_nand_4.4.94_ota-eng
	#add_lunch_combo x2000halley5.v20_nand_4.4.94_ota-user
	#add_lunch_combo x2000halley5.v20_nand_4.4.94_ota-userdebug

	# use msc as main fs storage for burn nandflash
	add_lunch_combo x2000halley5.v20_msc_4.4.94_burn-eng
	#add_lunch_combo x2000halley5.v20_msc_4.4.94_burn-user
	#add_lunch_combo x2000halley5.v20_msc_4.4.94_burn-userdebug

	# use nand as main fs storage.
	add_lunch_combo x2000halley5.v30_nand_4.4.94-eng
	#add_lunch_combo x2000halley5.v30_nand_4.4.94-user
	#add_lunch_combo x2000halley5.v30_nand_4.4.94-userdebug

	# use nand as main fs storage for OTA
	add_lunch_combo x2000halley5.v30_nand_4.4.94_ota-eng

	# use nor as main fs storage
	add_lunch_combo x2000halley5.v30_nor_4.4.94-eng
	#add_lunch_combo x2000halley5.v30_nor_4.4.94-user
	#add_lunch_combo x2000halley5.v30_nor_4.4.94-userdebug

	# use msc as main fs storage
	add_lunch_combo x2000halley5.v30_msc_4.4.94-eng
	#add_lunch_combo x2000halley5.v30_msc_4.4.94-user
	#add_lunch_combo x2000halley5.v30_msc_4.4.94-userdebug

	# use mmc as main fs storage for OTA
	add_lunch_combo x2000halley5.v30_msc_4.4.94_ota-eng
fi

KERNEL_DIRECTORY=$(find "$(pwd)/kernel" -maxdepth 1 -type d -name "kernel-5.10")
if [ -n "${KERNEL_DIRECTORY}" ]
then
	# use nand as main fs storage.
	add_lunch_combo x2000halley5.v20_nand_5.10-eng
	#add_lunch_combo x2000halley5.v20_nand_5.10-user
	#add_lunch_combo x2000halley5.v20_nand_5.10-userdebug

	# use nor as main fs storage
	add_lunch_combo x2000halley5.v20_nor_5.10-eng
	#add_lunch_combo x2000halley5.v20_nor_5.10-user
	#add_lunch_combo x2000halley5.v20_nor_5.10-userdebug

	# use msc as main fs storage
	add_lunch_combo x2000halley5.v20_msc_5.10-eng
	#add_lunch_combo x2000halley5.v20_msc_5.10-user
	#add_lunch_combo x2000halley5.v20_msc_5.10-userdebug

	# use nand as main fs storage.
	add_lunch_combo x2000halley5.v30_nand_5.10-eng
	add_lunch_combo x2000halley5.v30_nand_5.10_ota-eng
	#add_lunch_combo x2000halley5.v30_nand_5.10-user
	#add_lunch_combo x2000halley5.v30_nand_5.10-userdebug

	# use nor as main fs storage
	add_lunch_combo x2000halley5.v30_nor_5.10-eng
	#add_lunch_combo x2000halley5.v30_nor_5.10-user
	#add_lunch_combo x2000halley5.v30_nor_5.10-userdebug

	# use msc as main fs storage
	add_lunch_combo x2000halley5.v30_msc_5.10-eng
	add_lunch_combo x2000halley5.v30_msc_5.10_ota-eng
	#add_lunch_combo x2000halley5.v30_msc_5.10-user
	#add_lunch_combo x2000halley5.v30_msc_5.10-userdebug

	# use nand as main fs storage.
	#add_lunch_combo x2000halley5.v30withlvgl_nand_5.10-eng
fi

KERNEL_DIRECTORY=$(find "$(pwd)/kernel" -maxdepth 1 -type d -name "kernel-6.6")
if [ -n "${KERNEL_DIRECTORY}" ]
then
	# use nand as main fs storage.
	add_lunch_combo x2000halley5.v30_nand_6.6-eng
	add_lunch_combo x2000halley5.v30_nand_6.6_ota-eng

	# use nor as main fs storage
	add_lunch_combo x2000halley5.v30_nor_6.6-eng

	# use msc as main fs storage
	add_lunch_combo x2000halley5.v30_msc_6.6-eng
	add_lunch_combo x2000halley5.v30_msc_6.6_ota-eng
fi
# add board with subversion v30
