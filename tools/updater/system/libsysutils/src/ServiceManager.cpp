#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sysutils/ServiceManager.h>

#include <icutils/properties.h>

ServiceManager::ServiceManager() {
}

/* The service name should not exceed SERVICE_NAME_MAX to avoid
 * some weird things. This is due to the fact that:
 *
 * - Starting a service is done by writing its name to the "ctl.start"
 *   system property. This triggers the init daemon to actually start
 *   the service for us.
 *
 * - Stopping the service is done by writing its name to "ctl.stop"
 *   in a similar way.
 *
 * - Reading the status of a service is done by reading the property
 *   named "init.svc.<name>"
 *
 * If strlen(<name>) > (PROPERTY_KEY_MAX-1)-9, then you can start/stop
 * the service by writing to ctl.start/stop, but you won't be able to
 * read its state due to the truncation of "init.svc.<name>" into a
 * zero-terminated buffer of PROPERTY_KEY_MAX characters.
 */
#define SERVICE_NAME_MAX  (PROPERTY_KEY_MAX-10)

/* The maximum amount of time to wait for a service to start or stop,
 * in micro-seconds (really an approximation) */
#define  SLEEP_MAX_USEC     2000000  /* 2 seconds */

/* The minimal sleeping interval between checking for the service's state
 * when looping for SLEEP_MAX_USEC */
#define  SLEEP_MIN_USEC      200000  /* 200 msec */

int ServiceManager::start(const char *name) {
    if (strlen(name) > SERVICE_NAME_MAX) {
        printf("Service name '%s' is too long\n", name);
        return 0;
    }
    if (isRunning(name)) {
        printf("Service '%s' is already running\n", name);
        return 0;
    }

    printf("Starting service '%s'\n", name);
    property_set("ctl.start", name);

    int count = SLEEP_MAX_USEC;
    while(count > 0) {
        usleep(SLEEP_MIN_USEC);
        count -= SLEEP_MIN_USEC;
        if (isRunning(name))
            break;
    }
    if (count <= 0) {
        printf("Timed out waiting for service '%s' to start\n", name);
        errno = ETIMEDOUT;
        return -1;
    }
    printf("Sucessfully started '%s'\n", name);
    return 0;
}

int ServiceManager::stop(const char *name) {
    if (strlen(name) > SERVICE_NAME_MAX) {
        printf("Service name '%s' is too long\n", name);
        return 0;
    }
    if (!isRunning(name)) {
        printf("Service '%s' is already stopped\n", name);
        return 0;
    }

    printf("Stopping service '%s'\n", name);
    property_set("ctl.stop", name);

    int count = SLEEP_MAX_USEC;
    while(count > 0) {
        usleep(SLEEP_MIN_USEC);
        count -= SLEEP_MIN_USEC;
        if (!isRunning(name))
            break;
    }

    if (count <= 0) {
        printf("Timed out waiting for service '%s' to stop\n", name);
        errno = ETIMEDOUT;
        return -1;
    }
    printf("Successfully stopped '%s'\n", name);
    return 0;
}

bool ServiceManager::isRunning(const char *name) {
    char propVal[PROPERTY_VALUE_MAX];
    char propName[PROPERTY_KEY_MAX];
    int  ret;

    ret = snprintf(propName, sizeof(propName), "init.svc.%s", name);
    if (ret > (int)sizeof(propName)-1) {
        printf("Service name '%s' is too long\n", name);
        return false;
    }

    if (property_get(propName, propVal, NULL)) {
        if (!strcmp(propVal, "running"))
            return true;
    }
    return false;
}
