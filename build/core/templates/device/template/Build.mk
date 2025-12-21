include $(DEVICE_PATH)/uboot.mk

ifeq ($(strip $(TARGET_EXT_SUPPORT)),4.4.94)
        include $(DEVICE_PATH)/kernel-4.4.94.mk
endif

ifeq ($(strip $(TARGET_EXT_SUPPORT)),5.10)
        include $(DEVICE_PATH)/kernel-5.10.mk
endif


