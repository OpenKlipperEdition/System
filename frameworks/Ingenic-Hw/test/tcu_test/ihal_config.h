#ifndef __IHAL_CONFIG_H__
#define __IHAL_CONFIG_H__

#define DEVNAME  "sys/devices/platform/apb/10002000.tcu/enable"
#define CONTROLLER_ADDR   0x10002000

#if defined  X2600E_HALLEY || X2600_HALLEY7
#define TCU0_DEVNAME   "/sys/devices/platform/ahb_mcu/13630000.tcu0/enable"
#define TCU1_DEVNAME   "/sys/devices/platform/ahb_mcu/13640000.tcu1/enable"
#define TCU0_CONTROLLER_ADDR  0x13630000
#define TCU1_CONTROLLER_ADDR  0x13640000

#endif

#endif

