#ifndef SYSINFO_MANAGER_H
#define SYSINFO_MANAGER_H

#include <block/sysinfo/flag.h>

enum sysinfo_operation_mode {
    SYSINFO_OPERATION_RAM,
    SYSINFO_OPERATION_DEV,
};

struct sysinfo_manager {
    int64_t (*get_offset)(struct sysinfo_manager *this, int id);
    int64_t (*get_length)(struct sysinfo_manager *this, int id);
    int (*get_value)(struct sysinfo_manager *this, int id, char **buf, char flag);
    int (*set_value)(struct sysinfo_manager *this, int id, char *buf, char flag);
    int (*init)(struct sysinfo_manager *this);
    int (*exit)(struct sysinfo_manager *this);
    void *binder;
};

void sysinfo_manager_bind(struct sysinfo_manager *this, void *target);

extern struct sysinfo_manager sysinfo;

#define GET_SYSINFO_BINDER(t)   (t->binder)
#define GET_SYSINFO_ID(t)  (SYSINFO_##t)
#define GET_SYSINFO_MANAGER()  ((struct sysinfo_manager*)&sysinfo)

#endif
