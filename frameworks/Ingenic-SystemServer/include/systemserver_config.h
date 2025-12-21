#ifndef __SYSTEMSERVER_CONFIG_H__
#define __SYSTEMSERVER_CONFIG_H__


#define SYSTEM_SERVICE_NAME "ingenic.systemServer.service"


#define ENABLE_WATCHDOG_SERVER                  1
#define WDT_SERVICE_NAME                        "WatchdogManagerService"
#define WDT_SERVICE_EXECUTE_FILE                "WatchdogServer"


#define ENABLE_BACKLIGHT_SERVER                 1
#define BACKLIGHT_MANAGER_SERVICE_NAME          "BacklightManagerService"
#define BACKLIGHT_MANAGER_SERVICE_EXECUTE_FILE  "BacklightServer"


#define ENABLE_POWERMANAGER_SERVER              1
#define POWER_MANAGER_SERVICE_NAME              "PowerManagerService"
#define POWER_MANAGER_SERVICE_EXECUTE_FILE      "PowerServer"


#define ENABLE_EVENTMANAGER_SERVER              1
#define EVENT_MANAGER_SERVICE_NAME              "EventManagerService"
#define EVENT_MANAGER_SERVICE_EXECUTE_FILE      "EventServer"

#define ENABLE_OSD_SERVER                  1
#define OSD_SERVICE_NAME                        "OsdManagerService"
#define OSD_SERVICE_EXECUTE_FILE                "OsdServer"

#define ENABLE_CAMERA_SERVICE	1
#define CAMERA_SERVICE_NAME						"iss.camera_service"
#define CAMERA_SERVICE_EXECUTE_FILE             "cameraService"


#define ENABLE_ENCODER_SERVICE	1
#define ENCODER_SERVICE_NAME					 "iss.encoder_service"
#define ENCODER_SERVICE_EXECUTE_FILE             "encoderService"


#define NETWORK_SERVICE_NAME					 "iss.network.service"

#endif	//__SYSTEMSERVER_CONFIG_H__



