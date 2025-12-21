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
		   libupdater
endif

PRODUCT_MODULES += webcam_gadget \
		   getevent_test \
		   prn_example \
		   hid_gadget_test \
		   usbhid_device_sample \
		   grab \
		   umtprd

ifeq (3.10.14,$(findstring $(TARGET_EXT_SUPPORT),3.10.14))
PRODUCT_MODULES += isp
endif

ifneq ($(strip $(TARGET_STORAGE_MEDIUM)),nor)
CAMERA_UTILS := cimutils
ISP_TUNING_UTILS := v4l2-isp-tuning
DPU_UTILS := dpu
ROT_UTILS := rotate
HASH_UTILS := hash
VPU_UTILS := VpuTestScript
VPU_DIR_UTILS := VpuTestData
BLUETOOTH_FIRMWARE := install_bt_bcm43430a1_firmware
BSA_SERVER := install_bsa_server_fp64
WIFI_AP_MODE := install_wifi_ap_mode
WIFI_FIRMWARE := install_wifi_bcm43438_firmware
else
BLUETOOTH_FIRMWARE := 
BSA_SERVER := 
WIFI_AP_MODE := install_wifi_ap_mode
WIFI_FIRMWARE := install_wifi_ap6256_firmware
endif

#the device applications & test demo
ifneq ($(strip $(TARGET_STORAGE_MEDIUM)),nor)
PRODUCT_MODULES += v4l2-h264dec	\
		   v4l2-h264enc \
		   v4l2-jpegenc	\
		   v4l2-jpegdec	\
		   DualCameraPreview \
		   mjpg-streamer

PRODUCT_MODULES += $(RUNTESTDEMO_UTILS) \
		   $(CAMERA_UTILS) \
		   $(ISP_TUNING_UTILS) \
		   $(DPU_UTILS) \
		   $(ROT_UTILS) \
		   $(HASH_UTILS)\
		   $(VPU_UTILS) \
		   $(VPU_DIR_UTILS)

else
PRODUCT_MODULES += $(RUNTESTDEMO_UTILS)

endif
PRODUCT_MODULES += $(BLUETOOTH_FIRMWARE) \
				   $(BSA_SERVER) \
				   $(WIFI_AP_MODE) \
				   $(WIFI_FIRMWARE)
endif
