#include <inttypes.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <utils/log.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <types.h>
#include <utils/assert.h>
#include <lib/libcommon.h>
#include <lib/mtd/mtd-user.h>
#include <autoconf.h>
#include <lib/crc/libcrc.h>
#include <utils/list.h>
#include <utils/common.h>
#include <block/block_manager.h>
#include <block/fs/fs_manager.h>
#include <block/mtd/mtd.h>
#include <block/sysinfo/sysinfo_manager.h>

#define LOG_TAG "sysinfo_flag"

/*
 * The total size reserved for flag area is 1KB
 */

static void dump_data(int64_t offset, unsigned char *buf, int length) {
	int64_t addr = offset;
	int size = length * 4;
	char data[512];
	char *unit = NULL;
	int i;

	unit = calloc(1, size);
	if (unit == NULL) {
		LOGE("cannot alloc more memory\n");
		goto out;
	}
	for (i = 0; i < length / 2; i += 2) {
		unsigned short s = 0;
		if ((i % 16) == 0)
			sprintf(unit, "%08llx: ", addr + i);

		s = ((buf[i]) & 0xff) + ((buf[i + 1] & 0xff) << 8);

		sprintf(data, "%04x ", s);
		strcat(unit, data);
		if (((i + 2) % 16) == 0) {
			strcat(unit, "\n");
			printf("%s", unit);
		}
	}
	if (unit)
		free(unit);
	return;
out:
	if (unit)
		free(unit);
	assert_die_if(1, "crashed at %s\n", __func__);
	return;
}

static int sysinfo_flag_read(int id, void *flag) {
	struct sysinfo_manager *sys_m = GET_SYSINFO_MANAGER();
	char *sysinfo_buf = (char *)flag;
	char mode;

	if (flag == NULL) {
		LOGE("Parameter flag is null\n");
		return -1;
	}

	mode = SYSINFO_OPERATION_DEV;
	if (sys_m->get_value(sys_m, id, &sysinfo_buf, mode) < 0) {
		LOGE("Cannot get sysinfo by operation mode %d\n", mode);
		return -1;
	}

	dump_data(sys_m->get_offset(sys_m, id),
			(unsigned char*)sysinfo_buf,
			sys_m->get_length(sys_m, id));
	return 0;
}

static int sysinfo_flag_write(int id, void *flag) {
	struct sysinfo_manager *sys_m = GET_SYSINFO_MANAGER();
	char *sysinfo_buf = (char *)flag;
	char mode;

	if (flag == NULL) {
		LOGE("Parameter flag is null\n");
		return -1;
	}

	mode = SYSINFO_OPERATION_DEV;
	if (sys_m->set_value(sys_m, id, sysinfo_buf, mode)) {
		LOGE("Cannot set value by operation mode %d\n", mode);
		return -1;
	}
	return 0;
}

struct sysinfo_flag_ops sysinfo_flag_ops = {
	.read = sysinfo_flag_read,
	.write = sysinfo_flag_write,
};
