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
#include <block/sysinfo/sysinfo_manager.h>
#include <block/block_manager.h>
#include <block/fs/fs_manager.h>
#include <block/mtd/mtd.h>

#define LOG_TAG "sysinfo_manager"

const char *sysinfo_flag_name[] = {
    "mtd1",
    NULL
};

static int64_t sysinfo_get_offset(struct sysinfo_manager *this, int id) {

	struct block_manager *bm = (struct block_manager *)GET_SYSINFO_BINDER(this);
    if (bm == NULL) {
        LOGE("Cannot get binder bm\n");
        goto  out;
    }
	int64_t start = bm->get_partition_start_by_name(bm, sysinfo_flag_name[id]);
	if (start < 0) {
		LOGE("Cannot get mtd partiton start by name\n");
		goto out;
	}

	return start;
out:
	return -1;
}

static int64_t sysinfo_get_length(struct sysinfo_manager *this, int id) {
	if (id == SYSINFO_FLAG_NV)
		return sizeof(struct sysinfo_flag_nv);

	return 0;
}

static int sysinfo_get_value(struct sysinfo_manager *this,
		int id, char **buf, char flag)
{
	int64_t offset, len;
	offset = sysinfo_get_offset(this, id);
	len = sysinfo_get_length(this, id);
	if ((offset < 0) || (len < 0))
		goto out;

	if (flag == SYSINFO_OPERATION_DEV) {
		struct filesystem* fs = fs_new(BM_FILE_TYPE_NORMAL);
		struct block_manager *bm = (struct block_manager *)GET_SYSINFO_BINDER(this);
		struct mtd_dev_info *mtd = NULL;
		if (fs == NULL) {
			LOGE("Cannot instance filesystem %s\n", BM_FILE_TYPE_NORMAL);
			goto out;
		}
		if (bm == NULL) {
			LOGE("Cannot get binder bm\n");
			goto  out;
		}
		mtd = mtd_get_dev_info_by_offset(bm, offset);
		if (mtd == NULL) {
			LOGE("offset 0x%llx cannot be recognised by mtd\n", offset);
			goto out;
		}

		fs->set_params(fs, *buf, offset,
				len, BM_OPERATION_METHOD_RANDOM, mtd, bm);
		if (!strcmp(bm->name, BM_BLOCK_TYPE_MTD)) {

			if (fs->get_max_mapped_size_in_partition(fs) <= 0) {
				LOGE("Cannot get max mapped size by fs \'%s\'\n", fs->name);
				goto out;
			}
			if (fs->read(fs) < 0) {
				LOGE("Cannot read at offset 0x%llx by length %lld\n", offset, len);
				goto out;
			}
		} else if (!strcmp(bm->name, BM_BLOCK_TYPE_MMC)) {

		}
		fs_destroy(&fs);
	} else if (flag == SYSINFO_OPERATION_RAM) {
	}

	return 0;
out:
	return -1;
}


/*
 * Write schedual is isssued as read one block data, update some of the data, rewrite to storage device
 * Note: Max write size if less than block size
 */
static int sysinfo_set_value(struct sysinfo_manager *this,
		int id, char *buf, char flag)
{
	struct filesystem* fs = NULL;
	char *tmpbuf = NULL;
	int64_t offset, len;

	offset = sysinfo_get_offset(this, id);
	len = sysinfo_get_length(this, id);
	if ((offset < 0) || (len < 0))
		goto out;

	if (flag == SYSINFO_OPERATION_DEV) {
		struct block_manager *bm = (struct block_manager *)GET_SYSINFO_BINDER(this);
		struct mtd_dev_info *mtd = NULL;
		int64_t blkaligned_addr = 0;
		fs = fs_new(BM_FILE_TYPE_NORMAL);
		if (fs == NULL) {
			LOGE("Cannot instance filesystem %s\n", BM_FILE_TYPE_NORMAL);
			goto out;
		}
		if (bm == NULL) {
			LOGE("Cannot get binder bm\n");
			goto  out;
		}

		if (!strcmp(bm->name, BM_BLOCK_TYPE_MTD)) {
			mtd = mtd_get_dev_info_by_offset(bm, offset);
			if (mtd == NULL) {
				LOGE("offset 0x%llx cannot be recognised by mtd\n", offset);
				goto out;
			}
			tmpbuf = calloc(1, mtd->eb_size);
			if (tmpbuf == NULL) {
				LOGE("Cannot alloc any memory space, requested length %d\n", mtd->eb_size);
				goto out;
			}

			if (len > mtd->eb_size) {
				LOGE("Max write size cannot be more than %lld bytes\n", len);
				goto out;
			}
			blkaligned_addr = offset & (~(mtd->eb_size - 1));
			fs->set_params(fs, tmpbuf, blkaligned_addr,
					mtd->eb_size, BM_OPERATION_METHOD_RANDOM, mtd, bm);
			if (fs->get_max_mapped_size_in_partition(fs) <= 0) {
				LOGE("Cannot get max mapped size by fs \'%s\'\n", fs->name);
				goto out;
			}
			if (fs->read(fs) < 0) {
				LOGE("Cannot read at offset 0x%llx by length %d\n",
						offset, mtd->eb_size);
				goto out;
			}
			memcpy(tmpbuf + offset - blkaligned_addr, buf, len);
			if (fs->erase(fs) < 0) {
				LOGE("Cannot erase at offset 0x%llx by length %d\n",
						offset, mtd->eb_size);
				goto out;
			}
			fs_set_params_process(fs, mtd->eb_size);
			if (fs->write(fs) < 0) {
				LOGE("Cannot write at offset 0x%llx by length %d\n",
						offset, mtd->eb_size);
				goto out;
			}
			free(tmpbuf);
		} else if (!strcmp(bm->name, BM_BLOCK_TYPE_MMC)) {

		}
		fs_destroy(&fs);
	}

	return 0;
out:
	if (tmpbuf) {
		free(tmpbuf);
	}
	if (fs) {
		fs_destroy(&fs);
	}
	return -1;
}

static int sysinfo_init(struct sysinfo_manager *this) {
	return 0;
}

static int sysinfo_exit(struct sysinfo_manager *this) {
	return 0;
}

void sysinfo_manager_bind(struct sysinfo_manager *this, void *target) {
	struct block_manager *bm = (struct block_manager *)target;
	this->binder = bm;
	bm->sysinfo = this;
}

struct sysinfo_manager sysinfo = {
	.get_offset = sysinfo_get_offset,
	.get_length = sysinfo_get_length,
	.get_value = sysinfo_get_value,
	.set_value  = sysinfo_set_value,
	.init = sysinfo_init,
	.exit = sysinfo_exit,
};
