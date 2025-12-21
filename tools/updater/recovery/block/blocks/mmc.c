#include <inttypes.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <ctype.h>
#include <types.h>
#include <utils/assert.h>
#include <utils/log.h>
#include <lib/mtd/jffs2-user.h>
#include <lib/libcommon.h>
#include <lib/mtd/mtd-user.h>
#include <autoconf.h>
#include <lib/crc/libcrc.h>
#include <utils/list.h>
#include <block/sysinfo/sysinfo_manager.h>
#include <block/fs/fs_manager.h>
#include <block/block_manager.h>
#define LOG_TAG BM_BLOCK_TYPE_MMC
static int mmc_block_init(struct block_manager* this) {
	return 0;
}
static int mmc_block_exit(struct block_manager* this) {
	return 0;
}
static int mmc_chip_erase(struct block_manager *this) {
	return 0;
}

static int64_t mmc_block_erase(struct block_manager* this, int64_t offset,
                               int64_t length) {
	return 0;
}
static int64_t mmc_block_read(struct block_manager* this, char* partnode, int64_t offset,
							  char* buf, int64_t length) {
	return 0;
}

#define BLOCK_TO_WRITE  (1024 *1024) //4096
#define BLOCK2_TO_WRITE  4096
#define BLOCK3_TO_WRITE  64
#define BLOCK4_TO_WRITE  4
#define BLOCK5_TO_WRITE  1
static int64_t mmc_block_write(struct block_manager* this, char* infile, char* outfile,
					int64_t offset, int64_t length){
	LOGE("mmc write: infile=%s,outfile=%s,offset=%lld,length=%lld\n",infile,outfile,offset,length);
	char cmd[256] ={0};
	int idx = 0;
	int blk_size[]={BLOCK_TO_WRITE,BLOCK2_TO_WRITE,BLOCK3_TO_WRITE,BLOCK4_TO_WRITE,BLOCK5_TO_WRITE};
	int64_t skippos = 0;
	int64_t seekpos = 0;
	int64_t bs = 0;
	int64_t count = 0;
	int64_t wlen = length;
	while(wlen){
		for(idx=0; idx<sizeof(blk_size)/sizeof(int); idx++) {
			if(wlen >= blk_size[idx]){
				break;
			}
		}
		bs =  (wlen >= blk_size[idx]) ? blk_size[idx] : 1;
		count = (wlen >= blk_size[idx]) ? (wlen / blk_size[idx]) : wlen;
		seekpos = (offset + length - wlen) / bs;
		skippos = (length - wlen) / bs;
		sprintf(cmd, "dd if=%s of=%s bs=%lld skip=%lld seek=%lld count=%lld conv=fsync",infile, outfile, bs, skippos, seekpos, count);
		LOGE("mmc write cmd: %s\n",cmd);
		system(cmd);
		wlen -= bs * count;
	}
	return length;
}

int mmc_block_format(struct block_manager* this) {
	return 0;
}
static struct bm_operate_prepare_info* mmc_block_prepare(
    struct block_manager* this, int64_t offset, int64_t length,
    struct bm_operation_option *option) {
	return NULL;
}
static uint32_t mmc_get_prepare_leb_size(struct block_manager* this) {
	return 0;
}
static int64_t mmc_get_prepare_write_start(struct block_manager* this) {
	return 0;
}
static int64_t mmc_get_max_size_mapped_in(struct block_manager* this) {
	return 0;
}
static int64_t mmc_block_finish(struct block_manager* this) {
	return 0;
}
static int mmc_get_partition_count(struct block_manager* this) {
	return 0;
}
static int64_t mmc_get_partition_size_by_name(struct block_manager* this,
        char *name) {
	return 0;
}
static int64_t mmc_get_partition_size_by_offset(struct block_manager* this,
        int64_t offset) {
	return 0;
}
static int64_t mmc_get_partition_start_by_name(struct block_manager* this,
        char *name) {
	return 0;
}
static int64_t mmc_get_partition_start_by_offset(struct block_manager* this,
        int64_t offset) {
	return 0;
}
static int64_t mmc_get_capacity(struct block_manager* this) {
	return 0;
}
static int mmc_get_blocksize_by_offset(struct block_manager* this, int64_t offset) {
	return 0;
}
static int mmc_get_pagesize_by_offset(struct block_manager* this, int64_t offset) {
	return 0;
}
static char* mmc_get_block_type_by_offset(struct block_manager* this, int64_t offset) {
    return BM_BLOCK_TYPE_MMC;
}

static int mmc_install_filesystem(struct block_manager* this) {
    BM_MTD_FILE_TYPE_INIT(user_list);
    struct list_head *head = &this->list_fs_head;

    if (head->next == NULL || head->prev == NULL)
        INIT_LIST_HEAD(head);

    for (int i = 0; i < ARRAY_SIZE(user_list); i++) {
        struct filesystem* fs = fs_get_suppoted_by_name(user_list[i]);
        if (fs == NULL) {
            LOGE("Cannot get filesystem \"%s\"\n", user_list[i]);
            return -1;
        }

        if (fs_register(head, fs) < 0) {
            LOGE("Failed in register filesystem \"%s\"\n", user_list[i]);
            return -1;
        }

        LOGD("filesystem \"%s\" is installed\n", user_list[i]);
    }

    return 0;
}
static int mmc_uninstall_filesystem(struct block_manager* this) {
    struct filesystem* fs;

    BM_MTD_FILE_TYPE_INIT(user_list);

    for (int i = ARRAY_SIZE(user_list) - 1; i >= 0; i--) {

        fs = fs_get_registered_by_name(&this->list_fs_head, user_list[i]);
        if (fs == NULL)
            continue;

        if (fs_unregister(&this->list_fs_head, fs) < 0) {
            LOGE("Failed in unregister filesystem \"%s\"\n", user_list[i]);
            return -1;
        }

    }

    return 0;
}
static struct block_manager mmc_manager =  {
    .name = BM_BLOCK_TYPE_MTD,
    .chip_erase = mmc_chip_erase,
    .erase = mmc_block_erase,
    .write_m = mmc_block_write,
    .format = mmc_block_format,
    .prepare = mmc_block_prepare,
    .get_prepare_leb_size = mmc_get_prepare_leb_size,
    .get_prepare_write_start = mmc_get_prepare_write_start,
    .get_prepare_max_mapped_size = mmc_get_max_size_mapped_in,
    .finish = mmc_block_finish,
    .get_partition_count = mmc_get_partition_count,
    .get_partition_size_by_name = mmc_get_partition_size_by_name,
    .get_partition_size_by_offset = mmc_get_partition_size_by_offset,
    .get_partition_start_by_name =  mmc_get_partition_start_by_name,
    .get_partition_start_by_offset =  mmc_get_partition_start_by_offset,
    .get_capacity = mmc_get_capacity,
    .get_blocksize = mmc_get_blocksize_by_offset,
    .get_iosize = mmc_get_pagesize_by_offset,
    .get_block_type = mmc_get_block_type_by_offset,
};

int mmc_manager_init(void) {
	if (!mmc_block_init(&mmc_manager)
            && !register_block_manager(&mmc_manager)
            && !mmc_install_filesystem(&mmc_manager))
	{
        return 0;
	}

    return -1;
}

int mmc_manager_destroy(void) {
    if (!mmc_uninstall_filesystem(&mmc_manager)
            && !unregister_block_manager(&mmc_manager)
            && !mmc_block_exit(&mmc_manager))
        return 0;

    return -1;
}
