#ifndef UPDATE_FLAG_H
#define UPDATE_FLAG_H

/*
 * NV flags
 */
#define SYSINFO_FLAG_NV_UPDATE_START         0x5A5A5A5A
#define SYSINFO_FLAG_NV_UPDATE_DONE          0xA5A5A5A5

struct sysinfo_flag_nv {
    unsigned int version;
	unsigned int boot;
    unsigned int step;
    unsigned int start;
    unsigned int finish;
    unsigned int needfullpkg;//use for diffpkg failed
    unsigned int rot_angle;
    unsigned int reservedspace[15];
};


enum sysinfo_flag_id {
    SYSINFO_FLAG_NV,
    SYSINFO_FLAG_MAX
};

struct sysinfo_flag_ops {
    int (*read)(int id, void *flag);
    int (*write)(int id, void *flag);
};

extern struct sysinfo_flag_ops sysinfo_flag_ops;
#define GET_SYSINFO_FLAG()  ((struct sysinfo_flag_ops*)&(sysinfo_flag_ops))
#endif
