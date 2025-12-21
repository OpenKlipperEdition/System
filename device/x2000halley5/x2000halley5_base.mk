#Tools and libraries necessary by program

#kernel & uboot
#PRODUCT_MODULES := $(KERNEL_TARGET_IMAGE) \
	$(UBOOT_TARGET_FILE)
PRODUCT_MODULES := 	uboot \
			kernel

ifneq ($(strip $(TARGET_EXT2_SUPPORT)),burn)
RUNTESTDEMO_UTILS := 	RuntestScript \
			StorageMediaTestScript \
			AwakeTestScript	\
			OceanWingBtTest

# OTA support
ifeq ($(strip $(TARGET_EXT2_SUPPORT)),ota)
PRODUCT_MODULES += kernel_recovery	\
		   recovery		\
		   libupdater		\
		   libsysutils

ifeq ($(strip $(TARGET_STORAGE_MEDIUM)),msc)
PRODUCT_MODULES += getpackage
endif # end msc
else
PRODUCT_MODULES += install-mount-userdata
endif

PRODUCT_MODULES += webcam_gadget \
		   getevent_test \
		   hid_gadget_test \
		   prn_example \
		   usbhid_device_sample \
		   grab \
		   umtprd \
		   install_usb_composite_srcipts

ifeq (3.10.14,$(findstring $(TARGET_EXT_SUPPORT),3.10.14))
PRODUCT_MODULES += isp
endif

ifneq ($(strip $(TARGET_STORAGE_MEDIUM)),nor)
CAMERA_UTILS := cimutils
ISP_TUNING_UTILS := v4l2-isp-tuning
HASH_UTILS := hash
BLUETOOTH_FIRMWARE := install_bt_bcm4345c5_firmware
BSA_SERVER := install_bsa_server_fp64
WIFI_AP_MODE := install_wifi_ap_mode
WIFI_FIRMWARE := install_wifi_ap6256_firmware
else
BLUETOOTH_FIRMWARE := 
BSA_SERVER := 
WIFI_AP_MODE := install_wifi_ap_mode
WIFI_FIRMWARE := install_wifi_ap6256_firmware
endif

ifeq ($(strip $(TARGET_DEVICE_SUBVERSION)),v30withlvgl)
PRODUCT_MODULES += \
	LVGL_LifeSmart_fs
endif

#the device applications & test demo
ifneq ($(strip $(TARGET_STORAGE_MEDIUM)),nor)
PRODUCT_MODULES += mjpg-streamer \
				   ingenic-mpp

PRODUCT_MODULES += $(RUNTESTDEMO_UTILS) \
		   $(CAMERA_UTILS) \
		   $(ISP_TUNING_UTILS) \
		   $(HASH_UTILS)

else
PRODUCT_MODULES += $(RUNTESTDEMO_UTILS)

endif
PRODUCT_MODULES += $(BLUETOOTH_FIRMWARE) \
				   $(BSA_SERVER) \
				   $(WIFI_AP_MODE) \
				   $(WIFI_FIRMWARE)
endif
