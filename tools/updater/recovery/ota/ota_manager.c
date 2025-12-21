/*
 *  Copyright (C) 2016, Zhang YanMing <jamincheung@126.com>
 *
 *  Linux recovery updater
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/reboot.h>
#include <dirent.h>
#include <utils/log.h>
#include <utils/assert.h>
#include <utils/linux.h>
#include <utils/common.h>
#include <utils/compare_string.h>
#include <utils/file_ops.h>
#include <utils/minizip.h>
#include <utils/verifier.h>
#include <utils/signal_handler.h>
#include <ota/ota_manager.h>
#include <block/sysinfo/sysinfo_manager.h>


#define LOG_TAG "ota_manager"

#define ALARM_TIME_OUT  (30 * 60)   //30mins
#define VERSION_NEW_FILE  				"/usr/data/VERSION_NEW"
#define VERSION_OLD_FILE  				"/usr/data/VERSION_OLD"
#define VERSION_OTA_FILE					"/usr/data/VERSION"
#define UPDATE_RET_FILE      				"/usr/data/UPDATE_RET"

static const char* base_version_file = "BASE_VERSION";
static const char* remote_diffpkg_dirname = "difference";
static const char* remote_fullpkg_dirname = "full";
static const char* prefix_sha1_table = "sha1Tab";
static const char* prefix_global_xml = "global.xml";
static const char* prefix_device_xml = "device.xml";
static const char* prefix_update_xml = "update.xml";
static const char* prefix_update_pkg = "update";
#ifdef MMC_DEVICE
static const char* prefix_local_update_path = "/storage/update";
#else
static const char* prefix_local_update_path = "/tmp/update";
#endif

static const int update_wbuffer_method = UPDATE_WBUFFER_ALLOWABLE_MINIMUM_SIZE;
static int64_t next_write_offset;
//static struct gui* gui;
static struct play* play;

static void *main_task(void *param);
static void update_stages(enum update_stage_t stage, struct bm_event *event);

static void play_sound_tips(const char *tips) {
	char cmd[128]={0};
	snprintf(cmd, 128, "/usr/data/ota_res/update_play.sh %s", tips);
	system(cmd);
}

static void bm_event_listener(struct block_manager* bm,
        struct bm_event* event, void* param) {
    struct ota_manager* this = (struct ota_manager *)param;

    bm->dump_event(bm, event);

    /*
     * TODO
     */
    (void)this;
    update_stages(UPDATING, event);
}

static void set_process_info(struct ota_manager *this,
        int type, int64_t off, int64_t cnt) {
    struct block_manager *bm = this->mtd_bm;
    int progress = (int)(off * 100 / cnt);
    struct bm_event info;

    info.operation = type;
    info.progress = progress;

    if (BM_GET_LISTENER(bm))
        BM_GET_LISTENER(bm)(bm, &info, bm->param);
}

static int start(struct ota_manager* this) {
    int error = 0;

    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    error = pthread_create(&tid, &attr, main_task, (void *) this);
    if (error) {
        LOGE("pthread_create failed: %s", strerror(errno));
        pthread_attr_destroy(&attr);
        return -1;
    }

    pthread_attr_destroy(&attr);

    return 0;
}

static int stop(struct ota_manager* this) {
    LOGI("Stop ota manager.\n");

    return 0;
}

static int check_pkg_sha1(struct ota_manager* this, const char* path, int index) {
	FILE *fp=NULL;
	char cmd[64]={0};
	char buff[64]={0};
	int ret = 0;

	sprintf(cmd,"sha1sum %s", path);
	fp=popen(cmd, "r");
    if (fp == NULL) {
        LOGE("popen error,cmd: %s: %s\n", cmd, strerror(errno));
        return -1;
    }
	memset(buff,0,sizeof(buff));
	fread(buff,1,40,fp);
	printf("%s",buff);

	if(strncmp(buff, this->uf->sha1_table[index], 40)) {
		LOGD("check sha1 fail!\n");
		ret = -1;
	}else{
		LOGD("check sha1 pass!\n");
		ret = 0;
	}

	pclose(fp);
	return ret;
}

static int verify_update_pkg(struct ota_manager* this, const char* path) {
    int nkeys = 0;

    RSAPublicKey* keys = load_keys(g_data.public_key_path, &nkeys);
    if (keys == NULL) {
        LOGE("Failed to load public keys from: %s\n", g_data.public_key_path);
        return -1;
    }

    if (verify_file(path, keys, nkeys) == VERIFY_FAILURE) {
        LOGE("Failed to verify file: %s\n", path);
        return -1;
    }

    return 0;
}

static int creat_unzip_dir() {
    dir_delete(prefix_local_update_path);
    if (dir_create(prefix_local_update_path) < 0) {
        LOGE("Failed to create %s\n", prefix_local_update_path);
        return -1;
    }

    return 0;
}

static int check_device_info(struct ota_manager* this,
        struct device_info* device_info) {

    if (!strcmp(device_info->type, "nand")
            || !strcmp(device_info->type, "nor")) {

        if (device_info->part_count !=
                this->mtd_bm->get_partition_count(this->mtd_bm)) {
            LOGE("Partition count error\n");
            return -1;
        }

        struct list_head* pos;
        list_for_each(pos, &device_info->list) {
            struct part_info *info = list_entry(pos, struct part_info, head);

            char blkname[8] = {0};
            int blknum = 0;
            sscanf(info->block_name, "mtdblock%d", &blknum);
            sprintf(blkname, "mtd%d", blknum);

            int64_t offset = this->mtd_bm->get_partition_start_by_name(this->mtd_bm,
                    blkname);
            int64_t size = this->mtd_bm->get_partition_size_by_name(this->mtd_bm,
                    blkname);

            if ((offset != info->offset) || (size != info->size)) {
                LOGE("Failed to check partition: %s\n", info->name);
                return -1;
            }
        }

    } else if (!strcmp(device_info->type, "mmc")) {
	LOGI("-----mmc device-----\n");

    } else
        assert_die_if(1, "Unsupport device type: %s\n", device_info->type);

    return 0;
}

static int merge_imginfo_into_partinfo(struct ota_manager* this,
        struct device_info* device_info,  struct update_info* update_info) {

    struct list_head* pos_devinfo;

    list_for_each(pos_devinfo, &device_info->list){
        struct part_info *part_info = list_entry(pos_devinfo, struct part_info,
                head);

        INIT_LIST_HEAD(&part_info->list);

        uint64_t part_info_left_boundary = part_info->offset;
        uint64_t part_info_right_boundary = part_info_left_boundary +
            part_info->size;

        struct list_head* pos_update = NULL;
		list_for_each(pos_update, &update_info->list) {
            struct image_info* image_info = list_entry(pos_update,
                    struct image_info, head);

            if (image_info->offset < part_info_left_boundary)
                continue;
            if (image_info->offset >= part_info_right_boundary)
				continue;

            if ((image_info->offset + image_info->size) > part_info_right_boundary) {
                LOGE("Image offset 0x%llx, length %lld is overlap with current part\n",
                        image_info->offset,  image_info->size);
                goto out;
            }

            list_add_tail(&image_info->head_part, &part_info->list);
            part_info->image_count++;
            part_info->total_chunks += image_info->chunkcount;
        }
    }

    return 0;

out:
    return -1;
}

static int check_device_update_info(struct ota_manager* this,
        const char* path, struct device_info* device_info,
        struct update_info* update_info) {

    char local_path[1024] = {0};

#ifndef LOCAL_PACKAGE
    /*
     * Verifier update000.zip
     */
    LOGI("Verifying %s\n", path);

    if (file_exist(path) < 0 || check_pkg_sha1(this, path, 0) < 0 || verify_update_pkg(this, path) < 0)
        return -1;

    /*
     * Un-zip update pkg
     */
    LOGI("Unziping %s\n", path);
    if (unzip(path, prefix_local_update_path, NULL, 1) < 0) {
        LOGE("Failed to unzip %s to %s\n", path, prefix_local_update_path);
        return -1;
    }
#endif
    /*
     * Parse & check device info
     */
    memset(local_path, 0, sizeof(local_path));
    sprintf(local_path, "%s/%s", prefix_local_update_path, prefix_device_xml);

    LOGI("Parsing %s\n", local_path);

    if (this->uf->parse_device_xml(this->uf, local_path, device_info) < 0) {
        LOGE("Failed to parse %s\n", local_path);
        return -1;
    }

    /*
     * Check device info
     */
    LOGI("Checking device info: %s\n", device_info->type);
    if (check_device_info(this, device_info) < 0) {
        LOGE("Failed to check device info for %s\n", device_info->type);
        return -1;
    }

    /*
     * Parse update info
     */
    memset(local_path, 0, sizeof(local_path));
    sprintf(local_path, "%s/%s", prefix_local_update_path, prefix_update_xml);

    LOGI("Parsing %s\n", local_path);

    if (this->uf->parse_update_xml(this->uf, local_path, update_info) < 0) {
        LOGE("Failed to parse %s\n", local_path);
        return -1;
    }
    this->uf->dump_update_info(this->uf, update_info);

    /*
     * Check relation between device info and image info
     */
    LOGI("Merge imageinfo into partinfo: %s\n", device_info->type);
    if (merge_imginfo_into_partinfo(this, device_info, update_info) < 0) {
        LOGE("Failed to merge imageinfo into partinfo %s\n", device_info->type);
        return -1;
    }
    this->uf->dump_device_info(this->uf, device_info);
    return 0;
}

static int download_and_create_sha1_table(
		struct ota_manager* this) {

	char path[PATH_MAX] = {0};

	/*
	 * Download sha1Tab
	 */
	memset(path, 0, sizeof(path));
	sprintf(path, "%s/%s", this->cf->server_url, prefix_sha1_table);
	LOGI("Downloading %s\n", path);
	if (download_file(path, prefix_local_update_path) < 0) {
		LOGE("Failed to download %s\n", path);
		return -2;
	}

	/*
	 * Create sha1 table
	 */
	memset(path, 0, sizeof(path));
	sprintf(path, "%s/%s", prefix_local_update_path, prefix_sha1_table);
	LOGI("Parsing %s\n", path);
	if (this->uf->create_sha1_table(this->uf, path) < 0) {
		LOGE("Failed to parse %s\n", path);
		return -1;
	}

	return 0;
}

static int download_and_parse_global_conf(
        struct ota_manager* this) {

    char path[PATH_MAX] = {0};

#ifndef LOCAL_PACKAGE
    /*
     * Download global.xml
     */
    memset(path, 0, sizeof(path));
    sprintf(path, "%s/%s", this->cf->server_url, prefix_global_xml);
    LOGI("Downloading %s\n", path);
    if (download_file(path, prefix_local_update_path) < 0) {
        LOGE("Failed to download %s\n", path);
        return -2;
    }
#endif

    /*
     * Parse global.xml
     */
    memset(path, 0, sizeof(path));
    sprintf(path, "%s/%s", prefix_local_update_path, prefix_global_xml);
    LOGI("Parsing %s\n", path);
    if (this->uf->parse_global_xml(this->uf, path) < 0) {
        LOGE("Failed to parse %s\n", path);
        return -1;
    }
    this->uf->dump_device_type_list(this->uf);

    return 0;
}

static int download_and_parse_update_conf(
        struct ota_manager* this,
        struct device_info* device_info,
        struct update_info* update_info,
        char* devtype) {

    char path[PATH_MAX] = {0};
#ifndef LOCAL_PACKAGE
    /*
     * Download update000.zip
     */
    memset(path, 0, sizeof(path));
    sprintf(path, "%s/%s/%s%s", this->cf->server_url, devtype,
            prefix_update_pkg, "000.zip");
    LOGI("Downloading %s\n", path);

    if (download_file(path, prefix_local_update_path) < 0) {
        LOGE("Failed to download %s\n", path);
        return -2;
    }

    /*
     * Verify update000.zip
     */
    memset(path, 0, sizeof(path));
    sprintf(path, "%s/%s%s", prefix_local_update_path, prefix_update_pkg, "000.zip");
#endif

    if (check_device_update_info(this, path, device_info,
                update_info) < 0)
        return -1;

    return 0;
}
int get_data(const char *path) {
	char data[50] = { 0 };
	int ret = -1;
	int fd;
	fd = open(path, O_RDWR);
	if (fd < 0) {
		printf("--->failed to open %s!\n", path);
	} else {
		read(fd, data, sizeof(data));
		close(fd);
		ret = atoi(data);
	}
	printf("--->%s = %d\n", path, ret);
	return ret;
}

static bool remote_has_diffpkg(char* server_url) {
	bool has_diffpkg;
	char path[PATH_MAX] = {0};

	if (creat_unzip_dir() < 0){
		LOGE("creat_unzip_dir error\n");
		return -1;
	}
	memset(path, 0, sizeof(path));
	sprintf(path, "%s/%s/%s", server_url, remote_diffpkg_dirname, prefix_global_xml);
	if (download_file(path, prefix_local_update_path) < 0) {
		LOGE("Failed to download %s, no diff package\n", path);
		has_diffpkg = false;
	}else{
		LOGE("diff package detected\n");
		has_diffpkg = true;
	}
	dir_delete(prefix_local_update_path);

	return has_diffpkg;
}

static bool is_base_version(char* server_url) {
	bool is_base_version;
	char path[PATH_MAX] = {0};
	if (creat_unzip_dir() < 0){
		LOGE("creat_unzip_dir error\n");
		return -1;
	}
	memset(path, 0, sizeof(path));
	sprintf(path, "%s/%s/%s", server_url, remote_diffpkg_dirname, base_version_file);
	if (download_file(path, prefix_local_update_path) < 0) {
		LOGE("Failed to download %s\n", path);
		is_base_version = false;
	}else{
		memset(path, 0, sizeof(path));
		sprintf(path, "%s/%s", prefix_local_update_path, base_version_file);
		int bv = get_data(path);
		int cv = get_data("/usr/data/VERSION_OLD");
		if(bv == cv) {
			is_base_version = true;
			LOGI("is base version,base version=%d,current version=%d\n", bv, cv);
		} else {
			is_base_version = false;
			LOGI("is not base version,base version=%d,current version=%d\n", bv, cv);
		}
	}
	dir_delete(prefix_local_update_path);

	return is_base_version;
}
#ifdef MMC_DEVICE
static int nv_read(struct sysinfo_flag_nv* nv) {
	FILE *fp = NULL;
	char file_path[32] = {0};

	sprintf(file_path, "/dev/mmcblk%dp3", MMC_INDEX);

	fp = fopen(file_path, "rb");
	if(NULL==fp)
	{
		perror("fopen");
	}
	fread(nv,1,sizeof(struct sysinfo_flag_nv),fp);
	fclose(fp);
	return 0;
}
static int nv_write(struct sysinfo_flag_nv* nv) {
	FILE *fp = NULL;
	char file_path[32] = {0};

	sprintf(file_path, "/dev/mmcblk%dp3", MMC_INDEX);
	fp=fopen(file_path, "w");
	if(NULL==fp)
	{
		perror("fopen");
	}
	fwrite(nv,1,sizeof(struct sysinfo_flag_nv),fp);
	fclose(fp);
	return 0;
}
#else
static int nv_read(struct sysinfo_flag_nv* nv) {
    if (GET_SYSINFO_FLAG()->read(SYSINFO_FLAG_NV,
                (void *)nv) < 0) {
        LOGE("Cannot read nv flag%d\n", SYSINFO_FLAG_NV);
        return -1;
    }
    return 0;
}

static int nv_write(struct sysinfo_flag_nv* nv) {
    if (GET_SYSINFO_FLAG()->write(SYSINFO_FLAG_NV,
                (void *)nv) < 0) {
        LOGE("Cannot write nv flag%d\n", SYSINFO_FLAG_NV);
        return -1;
    }
    return 0;
}
#endif

static int get_ddr_type(char *mediumtype, char* ddr_type) {
	FILE *fp = NULL;
	int i = 0;

	if (!strcmp(mediumtype, "mmc")) {
#ifdef MMC_DEVICE
		char file_path[32] = {0};

		sprintf(file_path, "/dev/mmcblk%d", MMC_INDEX);
		fp = fopen(file_path, "rb");
		if (!fp) {
			perror("fopen");
			return -1;
		}
		fseek(fp, 0x4480, SEEK_SET);
#endif
	} else if (!strcmp(mediumtype, "nand")) {
		fp = fopen("/dev/mtd0", "rb");
		if (NULL == fp) {
			perror("fopen");
			return -1;
		}

		fseek(fp, 0x80, SEEK_SET);
	} else if (!strcmp(mediumtype, "nor")) {

	} else {
		LOGE("mediumtype not supported.\n");
		return -1;
	}

	fread(ddr_type, 4, sizeof(char), fp);
	for (i=0; i<4; i++)
		 printf("read ddr_type[%d]=0x%x\n", i, ddr_type[i]);
	fclose(fp);

	return 0;
}
static int set_ddr_type(char *mediumtype, char* ddr_type) {
	FILE *fp = NULL;
	int i = 0;

	if (!strcmp(mediumtype, "mmc")) {
#ifdef MMC_DEVICE
		char file_path[32] = {0};

		sprintf(file_path, "/dev/mmcblk%d", MMC_INDEX);
		fp = fopen(file_path, "rb+");
		if (!fp) {
			perror("fopen");
			return -1;
		}
		fseek(fp, 0x4480, SEEK_SET);
#endif
	} else if (!strcmp(mediumtype, "nand")) {
		fp = fopen("/dev/mtd0", "rb+");
		if (NULL == fp) {
			perror("fopen");
			return -1;
		}

		fseek(fp, 0x80, SEEK_SET);
	} else if (!strcmp(mediumtype, "nor")) {

	} else {
		LOGE("mediumtype not supported.\n");
		return -1;
	}

	fwrite(ddr_type, 4, sizeof(char), fp);

	fclose(fp);

	return 0;
}
static int clear_ddr_type(char* ubootpath) {
	char ddr_type[4] = {0};
	FILE*fp=fopen(ubootpath,"rb+");
	if(NULL==fp)
	{
		perror("fopen");
	}
	fseek(fp, 0x4480, SEEK_SET);
	memset(&ddr_type, 0, sizeof(ddr_type));
	fwrite(ddr_type,4,sizeof(char),fp);
	fclose(fp);
	return 0;
}

static void update_stages(enum update_stage_t stage,
        struct bm_event *event) {
	char cmd[256] ={0};
	int param_sec = 0;
	char sstage[][32] ={"UPDATING","UPDATE_START",
		"UPDATE_TIMEOUT","UPDATE_FAILURE",
		"UPDATE_SUCCESS","NETWORK_CONNECT_FAILURE",
		"NETWORK_CONNECT_SUCCESS","UPDATE_NET_CONFIG"};
    sync();

    /*
     * TODO
     */
	if(stage < 8) {
		if(event != NULL)
			param_sec = event->progress;
		sprintf(cmd, "/usr/data/ota_res/update_stage.sh %s %d", sstage[stage], param_sec);
		LOGD("*********cmd: %s\n",cmd);
		system(cmd);
	}
    switch (stage) {
    case UPDATE_SUCCESS:
		sprintf(cmd, "cp -f %s %s", VERSION_NEW_FILE, VERSION_OTA_FILE);
		system(cmd);
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "echo 0 > %s", UPDATE_RET_FILE);
		system(cmd);

        sleep(8);
        exit(0);
        break;
    case UPDATE_TIMEOUT:
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "echo 103 > %s", UPDATE_RET_FILE);
		system(cmd);

        sleep(8);
		system("reboot");
        break;
    case UPDATE_FAILURE:
		sprintf(cmd, "cp -f %s %s", VERSION_OLD_FILE, VERSION_OTA_FILE);
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "echo 101 > %s", UPDATE_RET_FILE);
		system(cmd);

        sleep(8);
		system("reboot");
        break;
    case NETWORK_CONNECT_FAILURE:
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "echo 102 > %s", UPDATE_RET_FILE);
		system(cmd);

        sleep(8);
        exit(-NETWORK_CONNECT_FAILURE);
        break;
    default:
        break;
    }
}

static int update_package(struct ota_manager* this,
        struct update_info* update_info, struct part_info* part_info,
        struct image_info* image_info, const char* path,
        uint32_t chunk_index) {
    int error = 0;
    int fd = 0;
    static uint32_t write_buffer_size, write_media_leap;
    static char *write_buffer = NULL;
    struct image_info* first_image, *last_image;
    uint32_t readsize, filesize;

    fd = open(path, O_RDWR);
    if (fd < 0) {
        LOGE("Cannot open file at %s\n", path);
        goto out;
    }

    filesize = get_file_size(path);
    first_image = list_entry(part_info->list.next, struct image_info, head_part);
    last_image = list_entry(part_info->list.prev, struct image_info, head_part);
    if ((first_image == NULL) || (last_image == NULL)) {
        LOGE("Cannot get first or last image from partition\n");
        goto out;
    }

    if (!strcmp(update_info->devtype, "nand")
            || !strcmp(update_info->devtype, "nor")) {

        struct block_manager* bm = this->mtd_bm;
        int64_t cur_write_offset = 0;

        if (!strcmp(first_image->name, image_info->name)
                && (chunk_index == 1)) {
            struct bm_operation_option option;
            error = bm->set_operation_option(bm, &option,
                    BM_OPERATION_METHOD_PARTITION, image_info->fs_type);
            if (error < 0) {
                LOGE("Failed to get operation option\n");
                goto out;
            }

            struct bm_operate_prepare_info* prepare_info =
                bm->prepare(bm, image_info->offset, image_info->size, &option);
            if (prepare_info == NULL) {
                LOGE("Failed to perpare, offset=0x%llx\n",
                        image_info->offset);
                goto out;
            }

            if (bm->get_prepare_leb_size(bm) < 0) {
                LOGE("Failed to get leb size, image write offset at %lld\n",
                        image_info->offset);
                goto out;
            }

            if (write_buffer == NULL) {
                if (update_wbuffer_method ==
                        UPDATE_WBUFFER_ALLOWABLE_MINIMUM_SIZE) {
                    write_buffer_size = bm->get_prepare_leb_size(bm);
                    write_media_leap = bm->get_blocksize(bm, image_info->offset);

                } else if (update_wbuffer_method ==
                        UPDATE_WBUFFER_FIXED_WITH_CHUCK_SIZE) {
                    write_buffer_size = image_info->chunksize;
                    write_media_leap =
                        (image_info->chunksize / bm->get_prepare_leb_size(bm))
                        * bm->get_blocksize(bm, image_info->offset);
                }

                write_buffer = malloc(write_buffer_size);
                if (write_buffer == NULL) {
                    LOGE("Failed to alloc any more memory, requested size %d",
                            write_buffer_size);
                    goto out;
                }
            }

            if ((option.method != BM_OPERATION_METHOD_PARTITION)
                    && ((bm->get_prepare_max_mapped_size(bm) + image_info->offset)
                        > (part_info->offset + part_info->size))) {
                LOGE("Overstep the boundary at 0x%llx, image write offset 0x%llx, size %lld\n",
                        part_info->offset + part_info->size, image_info->offset,
                        bm->get_prepare_max_mapped_size(bm));
                goto out;
            }

            error = bm->erase(bm, image_info->offset,
                    bm->get_partition_size_by_offset(bm, image_info->offset));
            if (error < 0) {
                LOGE("Failed to erase, offset=0x%llx, length=0x%llx\n",
                        image_info->offset,
                        bm->get_partition_size_by_offset(bm, image_info->offset));
                goto out;
            }

            cur_write_offset = bm->get_prepare_write_start(bm);
            if (cur_write_offset < 0) {
                LOGE("Failed to get write offset, gotten 0x%llx\n", cur_write_offset);
                goto out;
            }
        }

        if (next_write_offset > (part_info->offset + part_info->size)) {
            LOGE("Bad write offset at %lld\n",  next_write_offset);
            goto out;
        }

        if (next_write_offset && !cur_write_offset) {
            cur_write_offset = MAX(next_write_offset, image_info->offset);
        }

        char *buffer = write_buffer;
        while(filesize) {
            readsize = MIN(filesize, write_buffer_size);
            uint32_t already_read = 0;
            while(already_read != readsize) {
                error = read(fd, buffer + already_read, readsize - already_read);
                if (error < 0) {
                    LOGE("Failed to read %d size\n", readsize - already_read);
                    goto out;
                }

                already_read += error;
            }

            next_write_offset = bm->write(bm, cur_write_offset, write_buffer, readsize);
            if (next_write_offset < 0) {
                LOGE("Failed to write, offset=0x%llx, lenght=0x%llx\n",
                        cur_write_offset, image_info->size);
                goto out;
            }

            filesize -= readsize;
            cur_write_offset = next_write_offset;
        }

        if (!strcmp(last_image->name, image_info->name)
                && (chunk_index == image_info->chunkcount)) {
            error = bm->finish(bm);
            if (error < 0) {
                LOGE("Failed to issue bm finish, chunk index is %d\n", chunk_index);
                goto out;
            }

            if (write_buffer) {
                free(write_buffer);
                write_buffer = NULL;
            }
        }

	} else if (!strcmp(update_info->devtype, "mmc")) {
        struct block_manager* bm = this->mtd_bm;
		int seekpos = 0;
		if (image_info->update_mode == UPDATE_MODE_CHUNK) {
			seekpos = (chunk_index-1) * image_info->chunksize;
		}
		char* outfile[256] = {0};
		sprintf(outfile,"/dev/%s",part_info->block_name);
		bm->write_m(bm, path, outfile, seekpos, filesize);
	} else
        assert_die_if(1, "Unsupport device type: %s\n", update_info->devtype);

    if (fd > 0) {
        close(fd);
        fd = 0;
    }
    return 0;
out:
    if (write_buffer) {
        free(write_buffer);
        write_buffer = NULL;
    }
    if (fd > 0) {
        close(fd);
        fd = 0;
    }
    return -1;
}

static int update_images(struct ota_manager* this,
        struct device_info* device_info,
        struct update_info* update_info) {

    uint32_t index = 1;
    uint32_t chunk_count = 0;
    uint32_t need_umount = 0;
    int ret = 0;
	char ddr_type[4] = {0};
    char path[PATH_MAX] = {0};
    struct sysinfo_flag_nv* sysinfo_flag_nv;
    sysinfo_flag_nv = malloc(sizeof(struct sysinfo_flag_nv));
    memset((void *)sysinfo_flag_nv, 0, sizeof(struct sysinfo_flag_nv));

    struct list_head* sub_pos;
    list_for_each(sub_pos, &update_info->list) {
        struct image_info* info = list_entry(sub_pos, struct image_info,
                head);
        chunk_count += info->chunkcount;
    }

	struct list_head* pos_devinfo;
	do {
		list_for_each(pos_devinfo, &device_info->list) {
			struct part_info* part_info = list_entry(pos_devinfo,
					struct part_info, head);

			if (part_info->image_count > 0) {
				LOGI("Updating partition: \"%s: %s\"\n", part_info->name,
						part_info->block_name);

				struct list_head* pos_imageinfo;
				list_for_each(pos_imageinfo, &part_info->list) {
					struct image_info* image_info = list_entry(pos_imageinfo,
							struct image_info, head_part);
					next_write_offset = 0;

					if(nv_read(sysinfo_flag_nv) < 0) {
						LOGE("Failed to get sysinfo flag nv\n");
						goto error;
					}

					index = sysinfo_flag_nv->step + 1;
					if(index < image_info->pkg_index ||
					   index > image_info->pkg_index + image_info->chunkcount - 1) {
						if(index > chunk_count)
							return 0;
						else
							continue;
					} else if(!strcmp(image_info->pkg_type, "fullpkg")) {
						index = image_info->pkg_index;
					}

					if(sysinfo_flag_nv->boot != image_info->boot_mode) {
						sysinfo_flag_nv->boot = image_info->boot_mode;
						if(nv_write(sysinfo_flag_nv) < 0) {
							LOGE("Failed to set sysinfo flag nv\n");
							goto error;
						}
						LOGE("recovery upgrade finished,reboot\n");
						play_sound_tips("/usr/data/ota_res/tips/recovery_reboot.wav");
						sync();
						sleep(10);
/*						system("reboot -f");*/
						system("reboot");
						sleep(5);
						goto error; //exit
					}

					if(!strcmp(image_info->name, "uboot")) {
						get_ddr_type(device_info->type, &ddr_type);
					}
					LOGI("Updating image: \"%s\"\n", image_info->name);
					if(!strcmp(image_info->pkg_type, "diffpkg")) {
						char cmd[256]={0};
						if(!strcmp(image_info->fs_type, "directory")) {
							sprintf(cmd,"mkdir %s/ota;mount /dev/%s %s/ota",prefix_local_update_path,part_info->block_name,prefix_local_update_path);
							ret=system(cmd);
							LOGI("\n ret = %d  ,\n WIFEXITED(ret) = %d,\n  WEXITSTATUS(ret)= %d \n",ret,WIFEXITED(ret),WEXITSTATUS(ret));
							if((ret==-1) || !WIFEXITED(ret) ||  WEXITSTATUS(ret)) {
								LOGE("Failed to mount %s\n", part_info->name);
								memset(cmd, 0, sizeof(cmd));
								sprintf(cmd,"umount %s/ota",prefix_local_update_path);
								system(cmd);
								goto error;
							}
							need_umount=1;
						}else{
							sprintf(cmd, "mkdir %s/ota",prefix_local_update_path);
							LOGI("mmc write cmd: %s\n",cmd);
							system(cmd);
							char* infile[256] = {0};
							char* outfile[256] = {0};
							struct block_manager* bm = this->mtd_bm;

							sprintf(infile,"/dev/%s",part_info->block_name);
							sprintf(outfile,"%s/ota/%s",prefix_local_update_path, image_info->name);
							bm->write_m(bm, infile, outfile, 0, image_info->size);
							if(!strcmp(image_info->name, "uboot"))
								clear_ddr_type(outfile);
						}
					}
					for (int j = 1; j <= image_info->chunkcount; j++) {

#ifndef LOCAL_PACKAGE
						int retry_count = 0;

						while (1) {
							if (creat_unzip_dir() < 0)
								goto error;

							memset(path, 0, sizeof(path));
							sprintf(path, "%s/%s/%s%03d.zip", this->cf->server_url,
									device_info->type, prefix_update_pkg, index);

							LOGI("Downloading %s\n", path);
							if (download_file(path, prefix_local_update_path) < 0) {
								LOGE("Failed to download %s to %s\n", path,  prefix_local_update_path);
								play_sound_tips("/usr/data/ota_res/tips/download_updatefile_fail.wav");
								goto error_retry;
							}

							memset(path, 0, sizeof(path));
							sprintf(path, "%s/%s%03d.zip", prefix_local_update_path,
									prefix_update_pkg, index);
							LOGI("Verifying %s\n", path);
							if (file_exist(path) < 0 || check_pkg_sha1(this, path, index) < 0 || verify_update_pkg(this, path) < 0){
								play_sound_tips("/usr/data/ota_res/tips/verify_updatefile_error.wav");
								goto error_retry;
							}

							LOGI("Unziping %s\n", path);
							if(!strcmp(image_info->pkg_type, "fullpkg")) {
								if (unzip(path, prefix_local_update_path, NULL, 1) < 0) {
									LOGE("Failed to unzip %s to %s\n", path,  prefix_local_update_path);
									play_sound_tips("/usr/data/ota_res/tips/unzip_updatefile_fail.wav");
									goto error_retry;
								}

								memset(path, 0, sizeof(path));
								if (image_info->update_mode == UPDATE_MODE_FULL)
									sprintf(path, "%s/%s", prefix_local_update_path,
											image_info->name);
								else
									sprintf(path, "%s/%s_%03d", prefix_local_update_path,
											image_info->name, j);

								if (file_exist(path) < 0) {
									LOGE("Failed to access %s: %s\n", path, strerror(errno));
									play_sound_tips("/usr/data/ota_res/tips/access_updatefile_fail.wav");
									goto error_retry;
								}

								if ((j != image_info->chunkcount)
									&& (get_file_size(path) != image_info->chunksize)) {
									LOGE("Image %s size error\n", image_info->name);
									play_sound_tips("/usr/data/ota_res/tips/updatefile_size_error.wav");
									goto error_retry;
								}
							} else {
								char cmd[256]={0};
								sprintf(cmd, "unzip %s -d %s",path,prefix_local_update_path);
								ret=system(cmd);
								if((ret==-1) || !WIFEXITED(ret) ||  WEXITSTATUS(ret)) {
									LOGE("Failed to unzip %s\n", path);
									goto error_retry;
								}
							}

							break;
error_retry:
							if (retry_count++ < 10) {
								usleep(2 * 1000 * 1000);
								LOGE("retrying downlod cnt: %d\n", retry_count);
							} else {
								play_sound_tips("/usr/data/ota_res/tips/reconnect_net_fail.wav");
								LOGE("failed after retrying cnt: %d\n", retry_count);
								goto error;
							}
						}
#else
						memset(path, 0, sizeof(path));
						if (image_info->update_mode == UPDATE_MODE_FULL)
							sprintf(path, "%s/update/%s%03d/%s", prefix_local_update_path,
									prefix_update_pkg, index, image_info->name);
						else
							sprintf(path, "%s/update/%s%03d/%s_%03d", prefix_local_update_path,
									prefix_update_pkg, index, image_info->name, j);
#endif

						if(!strcmp(image_info->pkg_type, "fullpkg")) {
							LOGI("Updating \"%s\"\n", path);
							if (update_package(this, update_info, part_info,
											   image_info, path, j) < 0) {
								LOGE("Failed to write %s\n", path);
								goto error;
							}
						} else {
							char cmd[256]={0};
							memset(path, 0, sizeof(path));
#ifndef LOCAL_PACKAGE
							sprintf(path, "%s/%s%03d", prefix_local_update_path,
									prefix_update_pkg, index);
#else
							sprintf(path, "%s/update/%s%03d", prefix_local_update_path,
									prefix_update_pkg, index);
#endif
							sprintf(cmd, "%s/otadiff.sh %s/ota %s",path,prefix_local_update_path,path);
							LOGI("cmd:%s\n",cmd);
							ret=system(cmd);
							LOGI("\n ret = %d  ,\n WIFEXITED(ret) = %d,\n  WEXITSTATUS(ret)= %d \n",ret,WIFEXITED(ret),WEXITSTATUS(ret));
							if((ret ==-1) || !WIFEXITED(ret)) {
								LOGE("Failed to execute cmd: %s\n",cmd);
								goto error;
							} else if (WEXITSTATUS(ret)){
								LOGE("diffpkg fail! back to fullpkg version!\n");
								sysinfo_flag_nv->needfullpkg = 1;
								if(nv_write(sysinfo_flag_nv) < 0) {
									LOGE("Failed to set sysinfo flag nv\n");
									goto error;
								}
								sync();
								system("reboot");
								sleep(5);
								goto error;
							}
						}

						sysinfo_flag_nv->step = index;
						if(nv_write(sysinfo_flag_nv) < 0) {
							LOGE("Failed to set sysinfo flag nv\n");
							goto error;
						}
						set_process_info(this, BM_OPERATION_WRITE, index, chunk_count);
						if(index == chunk_count) {
							if(!strcmp(image_info->pkg_type, "diffpkg")) {
								char cmd[256] ={0};
								if(need_umount) {
									sprintf(cmd,"umount %s/ota",prefix_local_update_path);
									ret=system(cmd);
									LOGI("\n ret = %d  ,\n WIFEXITED(ret) = %d,\n  WEXITSTATUS(ret)= %d \n",ret,WIFEXITED(ret),WEXITSTATUS(ret));
									need_umount=0;
								}else{
									struct block_manager* bm = this->mtd_bm;
									char* infile[256] = {0};
									char* outfile[256] = {0};
									sprintf(infile,"%s/ota/%s",prefix_local_update_path, image_info->name);
									sprintf(outfile,"/dev/%s",part_info->block_name);

									int filesize = get_file_size(infile);
									bm->write_m(bm, infile, outfile, 0, filesize);
								}
								memset(cmd, 0, sizeof(cmd));
								sprintf(cmd,"rm -r %s/ota",prefix_local_update_path);
								LOGI("cmd:%s\n",cmd);
								system(cmd);
							}
							if(!strcmp(image_info->name, "uboot")) {
								LOGE("set_ddr_type\n");
								set_ddr_type(device_info->type, ddr_type);
							}
							return 0;
						} else
							index++;
					}
					if(!strcmp(image_info->pkg_type, "diffpkg")) {
						char cmd[256] ={0};
						if(need_umount) {
							sprintf(cmd,"umount %s/ota",prefix_local_update_path);
							ret=system(cmd);
							LOGI("\n ret = %d  ,\n WIFEXITED(ret) = %d,\n  WEXITSTATUS(ret)= %d \n",ret,WIFEXITED(ret),WEXITSTATUS(ret));
							need_umount=0;
						}else{
							struct block_manager* bm = this->mtd_bm;
							char* infile[256] = {0};
							char* outfile[256] = {0};
							sprintf(infile,"%s/ota/%s",prefix_local_update_path, image_info->name);
							sprintf(outfile,"/dev/%s",part_info->block_name);

							int filesize = get_file_size(infile);
							bm->write_m(bm, infile, outfile, 0, filesize);
						}
						memset(cmd, 0, sizeof(cmd));
						sprintf(cmd,"rm -r %s/ota",prefix_local_update_path);
						LOGI("cmd:%s\n",cmd);
						system(cmd);
					}
					if(!strcmp(image_info->name, "uboot")) {
						LOGI("set_ddr_type\n");
						set_ddr_type(device_info->type, ddr_type);
					}
				}
			}
		}
	} while (1);

error:
    return -1;
}

static int update_devices(struct ota_manager* this) {
    int error = 0;
    int ret = 0;
    struct sysinfo_flag_nv* sysinfo_flag_nv;
    sysinfo_flag_nv = malloc(sizeof(struct sysinfo_flag_nv));
    memset((void *)sysinfo_flag_nv, 0, sizeof(struct sysinfo_flag_nv));

#ifndef LOCAL_PACKAGE
    if (creat_unzip_dir() < 0)
        goto error;
	ret = download_and_create_sha1_table(this);
	if (ret < 0) {
		goto error;
	}
	ret = download_and_parse_global_conf(this);
	if (ret < 0) {
		goto error;
	}
#else
	this->uf->init_global_conf(this->uf);
#endif

    const char** device_type_list = this->uf->get_device_type_list(this->uf);
    for (int i = 0; device_type_list[i]; i++) {
        const char* devtype = device_type_list[i];

        LOGI("Updating device: \"%s\"\n", devtype);
        struct device_info* device_info =
            this->uf->get_device_info_by_devtype(this->uf, devtype);
        struct update_info* update_info =
            this->uf->get_update_info_by_devtype(this->uf, devtype);
        if (device_info == NULL || update_info == NULL) {
            LOGE("Failed to find device info or update_info for %s\n", devtype);
            goto error;
        }

        if(download_and_parse_update_conf(this, device_info,
                    update_info, devtype) < 0) {
            LOGE("Failed to download and parse update conf for %s\n", devtype);
            goto error;
        }

        if(nv_read(sysinfo_flag_nv) < 0) {
            LOGE("Failed to get sysinfo flag nv\n");
            goto error;
        }

        sysinfo_flag_nv->start = SYSINFO_FLAG_NV_UPDATE_START;
        if(nv_write(sysinfo_flag_nv) < 0) {
            LOGE("Failed to set sysinfo flag nv\n");
            goto error;
        }

        if(update_images(this, device_info, update_info) < 0) {
            LOGE("Updating pkg error\n");
            goto error;
        }

        LOGI("Updating Finish\n");
        memset((void *)sysinfo_flag_nv, 0, sizeof(struct sysinfo_flag_nv));
        sysinfo_flag_nv->finish = SYSINFO_FLAG_NV_UPDATE_DONE;
        if(nv_write(sysinfo_flag_nv) < 0) {
            LOGE("Failed to set sysinfo flag nv\n");
            goto error;
        }
    }

    dir_delete(prefix_local_update_path);
    return 0;
error:
#ifndef LOCAL_PACKAGE
    dir_delete(prefix_local_update_path);
#endif
    return -1;
}

static void signal_handler(int signal) {
    LOGE("Update time out.\n");
    update_stages(UPDATE_TIMEOUT, NULL);
}

static int start_netconfig() {
#if 0
    wifi_ctl_msg_t mode = {0};

    mode.cmd = STOP_ALL_WIFI;
    request_wifi_mode(mode);

	system("killall -9 network_apconfig > /dev/null 2>&1");
	system("network_apconfig -t 600 -s 120 &");
#endif
}

static bool try_ping_server(struct ota_manager* ota_mg, const char *server){
	int PING_MAX_RETRY = 100;
	int try_count = 0;
	bool ret = false;
	while (try_count < PING_MAX_RETRY) {
		try_count++;
		if (ota_mg->ni->ping(server) < 0) {
			LOGE("Server %s is unreachable\n", server);
			sleep(1);
		} else {
			LOGD("ping %s success! \n", server);
			return true;
		}
	}
	return false;
}

//static int PING_MAX_RETRY = 10;
static void *main_task(void* param) {
	int try_count = 0;
    struct ota_manager* this = (struct ota_manager*) param;
    enum update_stage_t stage = 0;
    LOGD("============================= start Update main task=============================\n");
    this->sh->set_signal_handler(this->sh, SIGALRM, signal_handler);
    alarm(ALARM_TIME_OUT);

    update_stages(UPDATE_START, NULL);

	struct sysinfo_flag_nv* sysinfo_flag_nv;
	sysinfo_flag_nv = malloc(sizeof(struct sysinfo_flag_nv));
	memset((void *)sysinfo_flag_nv, 0, sizeof(struct sysinfo_flag_nv));

	if(nv_read(sysinfo_flag_nv) < 0)
		LOGE("Failed to get sysinfo flag nv\n");

    this->uf = _new(struct update_file, update_file);
#ifndef LOCAL_PACKAGE
	bool ping_ota_server_success = false;
	/*ping 升级服务，假如正常，说明升级服务器正常*/
    ping_ota_server_success = try_ping_server(this, this->cf->server_ip);
    /*升级服务器连接失败，exit*/
    if (ping_ota_server_success == false)
    	goto exit;


	if (this->ni->icmp_echo(this->ni, this->cf->server_ip, 30000) < 0) {
		LOGE("Server \"%s\" is unreachable\n", this->cf->server_ip);
		stage = NETWORK_CONNECT_FAILURE;
		goto exit;
	}

	this->cf->parse_current_version(this->cf);

	char *pkg_dirname=NULL;
	if(sysinfo_flag_nv->needfullpkg || !remote_has_diffpkg(this->cf->server_url) || !is_base_version(this->cf->server_url)) {
		pkg_dirname = remote_fullpkg_dirname;
	} else {
		pkg_dirname = remote_diffpkg_dirname;
	}
	char *pkg_url = (char *)calloc(1,(strlen(this->cf->server_url) + strlen(pkg_dirname) + 2));
	snprintf(pkg_url, strlen(this->cf->server_url) + strlen(pkg_dirname) + 2, "%s/%s", this->cf->server_url, pkg_dirname);
	if(this->cf->server_url)
		free(this->cf->server_url);
	this->cf->server_url = pkg_url;
	LOGI("this->cf->server_url:%s\n",this->cf->server_url);
#else
	if(sysinfo_flag_nv->start != SYSINFO_FLAG_NV_UPDATE_START){
		char path[PATH_MAX] = {0};
		/*
		 * Verify update.zip
		 */
		memset(path, 0, sizeof(path));
		sprintf(path, "%s/%s.zip", prefix_local_update_path, prefix_update_pkg);
		LOGI("Verifying %s\n", path);
		if (file_exist(path) < 0 || verify_update_pkg(this, path) < 0) {
			LOGE("Failed to verify %s\n", path);
			goto exit;
		}
		/*
		 * Un-zip update pkg
		 */
		LOGI("Unziping %s\n", path);
		char cmd[256]={0};
		sprintf(cmd, "unzip %s -d %s",path,prefix_local_update_path);
		int ret=system(cmd);
		if((ret==-1) || !WIFEXITED(ret) ||  WEXITSTATUS(ret)) {
			LOGE("Failed to unzip %s\n", path);
			goto exit;
		}
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "cp %s/update/update000/* %s", prefix_local_update_path, prefix_local_update_path);
		printf("cmd:%s\n",cmd);
		system(cmd);
	}
#endif

    if(update_devices(this) < 0)
        stage = UPDATE_FAILURE;
    else
        stage = UPDATE_SUCCESS;

exit:
    _delete(this->uf);
#ifndef LOCAL_PACKAGE
    if (ping_ota_server_success == false) {
		/*进入配网模式*/
    	stage= UPDATE_NET_CONFIG;
		update_stages(stage, NULL);
		start_netconfig();
		while(1){
			if (try_ping_server(this, "www.baidu.com")){
				sleep(15);
				play_sound_tips("/usr/data/ota_res/tips/connect_net_success.wav");
				sleep(5);
				system("sync");
				system("reboot -f");
			}
			sleep(5);
		}
	}
#endif
	update_stages(stage, NULL);
    return NULL;
}

static void load_configure(struct ota_manager* this,
        struct configure_file* cf) {
    assert_die_if(cf == NULL, "cf is NULL\n");

    this->cf = cf;
}

static void load_signal_handler(struct ota_manager* this,
        struct signal_handler* sh) {
    assert_die_if(sh == NULL, "sh is NULL\n");

    this->sh = sh;
}

void construct_ota_manager(struct ota_manager* this) {
    this->start = start;
    this->stop = stop;
    this->load_configure = load_configure;
    this->load_signal_handler = load_signal_handler;

#if 0
    /*
     * Instance gui
     */
    gui = _new(struct gui, gui);
    gui->init(gui);

    /*
     * Instance sound player
     */
    play = _new(struct play, play);
    play->init(play);
#endif

    /*
     * Instance net interface
     */
    this->ni = (struct net_interface*) calloc(1, sizeof(struct net_interface));
    this->ni->construct = construct_net_interface;
    this->ni->destruct = destruct_net_interface;
    this->ni->construct(this->ni, NULL);
    this->ni->init_socket(this->ni);

    /*
     * Instance block manager
     */
    this->mtd_bm = (struct block_manager*) calloc(1, sizeof(struct block_manager));
    this->mtd_bm->construct = construct_block_manager;
    this->mtd_bm->destruct = destruct_block_manager;
    this->mtd_bm->construct(this->mtd_bm, BM_BLOCK_TYPE_MTD, bm_event_listener,
            (void *)this);
}

void destruct_ota_manager(struct ota_manager* this) {
    struct list_head* pos;
    struct list_head* next_pos;

    _delete(this->cf);

    this->cf = NULL;
    this->start = NULL;
    this->stop = NULL;
    this->load_configure = NULL;
    this->load_signal_handler = NULL;

    /*
     * Destruct net_interface
     */
    this->ni->destruct(this->ni);
    free(this->ni);
    this->ni = NULL;

#if 0
    /*
     * Destruct sound player
     */
    play->deinit(play);
    _delete(play);
    play = NULL;

    /*
     * Destruct graphics drawer
     */
    gui->deinit(gui);
    _delete(gui);
    gui = NULL;
#endif

    /*
     * Destruct block manager
     */
    this->mtd_bm->destruct(this->mtd_bm);
    free(this->mtd_bm);
    this->mtd_bm = NULL;
}
