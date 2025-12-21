#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/input.h>
#include <unistd.h>
#include <signal.h>
#include <key.h>
#include <cjson/cJSON.h>
#include "cjson_helper.h"
#include "updater.h"

#define URL_CONFIG_FILE "/usr/data/ota_res/recovery.conf"

#define VERSION_FILE "/usr/data/VERSION"
#define VERSION_FILE_NAME "VERSION"
#define TMP_VERSION_FILE "/tmp/VERSION"

#define FORCE_UPDATE_FLAG_FILE_NAME "FORCE_UPDATE_FLAG"
#define TMP_FORCE_UPDATE_FLAG_FILE "/tmp/FORCE_UPDATE_FLAG"

#define VERSION_CONF_FILE_NAME "current_version_full.conf"
#define TMP_VERSION_CONF_FILE "/usr/data/current_version_full.conf"
#define USE_DATA_DIR "/usr/data/"
#define UPDATEVERSIONFILE "/tmp/VERSION"
#define TMPDIR "/tmp/"


#define FMW_DEVICE   "/dev/jz-spinand-fmw"
#define SN_READ _IOR('S', 0x1, unsigned int)
#define SN_GET_CONFIG   _IOR('S', 0x0, unsigned int)
#define SN_NUM_SIZE 16
#define CTEI_NUM_SIZE 15



struct sn_config {
    unsigned int sn_len;
    unsigned int crc_val;
};


static void delchar(char *s,char c){
    int i = 0, j = 0;
   char tmp[80];
      while(s[i]!='\0'){
        if(s[i++] != c)
             tmp[j++]= s[i-1];
   }
    tmp[j] = 0;
    strcpy(s, tmp);
}

static long long get_version(const char *path)
{
    FILE *fp;
    char line[512];
    unsigned long long version;

    fp = fopen(path, "r");
    if(fp == NULL) {
        printf("open %s fail\n", path);
        return -1;
    }

    while (!feof(fp)) {
        memset(line, 0, sizeof(line));
        if(fgets(line, sizeof(line), fp) == NULL)
            continue;
        version = strtoull(line, NULL, 10);
        fclose(fp);
        return version;
    }

    fclose(fp);
    return 0;
}

static char *updater_get_url(const char *path)
{
	struct ota_configure recovery_conf;
	memset(&recovery_conf, 0, sizeof(struct ota_configure));
	parse_ota_configure(&recovery_conf, path);
    if (recovery_conf.url == NULL)
        return NULL;

    char command[strlen(recovery_conf.url) + 128];
    delchar(recovery_conf.url, '"'); //remove " char
    remove(TMP_VERSION_CONF_FILE);
    printf("recovery_conf.url:%s VERSION_CONF_FILE_NAME %s USE_DATA_DIR %s\n", recovery_conf.url, VERSION_CONF_FILE_NAME, USE_DATA_DIR);
    sprintf(command, "wget -c %s/%s -P %s --no-check-certificate", recovery_conf.url, VERSION_CONF_FILE_NAME, USE_DATA_DIR);
    printf("command:%s\n", command);
    if(system(command) < 0)
        return recovery_conf.url;

	struct ota_configure conf;
	memset(&conf, 0, sizeof(struct ota_configure));
	parse_ota_configure(&conf, TMP_VERSION_CONF_FILE);
    return strdup(conf.url);
}

int64_t updater_get_ota_version(const char *url_config_file)
{
    char *url;
    char command[1024];

    if (!url_config_file)
        url_config_file = URL_CONFIG_FILE;

    url = updater_get_url(url_config_file);
    if (!url)
        return -1;
    delchar(url, '"'); //remove " char
    remove(TMP_VERSION_FILE);

    sprintf(command, "wget -c %s/%s -P %s --no-check-certificate", url, VERSION_FILE_NAME, TMPDIR);
    printf("command:%s\n", command);
    free(url);
    if(system(command) < 0)
        return -1;

    return get_version(TMP_VERSION_FILE);
}

int64_t updater_check_update(const char *url_config_file, const char *version_file)
{
    if (!version_file)
        version_file = VERSION_FILE;

    int64_t new_version = updater_get_ota_version(url_config_file);
    int64_t old_version = get_version(version_file);

    if (new_version == -1 || old_version == -1)
        return -1;

    return new_version > old_version ? new_version : 0;
}

int64_t updater_check_update_by_versin(const char *url_config_file, int64_t old_version)
{
    int64_t new_version = updater_get_ota_version(url_config_file);

    if (new_version == -1 || old_version == -1)
        return -1;

    return new_version > old_version ? new_version : 0;
}

int updater_save_version(const char *version_file, int64_t new_vesion)
{
    FILE *file;

    if (!version_file)
        version_file = VERSION_FILE;

    if (new_vesion < 0) {
        printf("new version: %lld < 0, not suppport\n", new_vesion);
        return -1;
    }

    file = fopen(version_file, "w+");
    if (!file) {
        printf("failed to open: %s ", version_file);
        perror("");
        return -1;
    }

    if (fprintf(file, "%lld", new_vesion) < 0) {
        printf("failed to write: %s ", version_file);
        perror("");
        return -1;
    }

    fclose(file);
    return 0;
}

char *get_json_data(const char *jsonfile) {
	FILE *f_json = NULL;

	long json_size;

	char *json_data = NULL;
	printf("open %s\n", jsonfile);
	f_json = fopen(jsonfile, "r");
	if (f_json == NULL) {
		printf("open %s  failed\n", jsonfile);
		return NULL;
	}
	fseek(f_json, 0, SEEK_END); //将指针移动到文件尾部
	json_size = ftell(f_json); //当前指针位置相对于文件首部偏移的字节数
	fseek(f_json, 0, SEEK_SET); //将指针移动到文件首部
	json_data = (char *) malloc(json_size + 1); //向系统申请分配指定size个字节的内存空间
	memset(json_data, 0, json_size + 1);
	fread((void *) json_data, json_size, 1, f_json); //将f_json中的数据读入中json_data中
	fclose(f_json);
	f_json = NULL;
	return (json_data);
}

/*
 * jsonfile 为空，默认打开/usr/data/ota_res/recovery.conf配置文件
 */


void dump_ota_configure(struct ota_configure *conf){
	printf("======dump_ota_configure======\n");
	printf("ip: %s\n", conf->ip);
	printf("url: %s\n", conf->url);
	printf("rpt_url: %s\n", conf->rpt_url);
	printf("upgrade: %d\n", conf->upgrade);
	printf("force_upgrade: %d\n", conf->force_upgrade);
	printf("curr_ver: %d\n", conf->curr_ver);
	printf("new_ver: %d\n", conf->new_ver);
	printf("======dump_end======\n");
}


void parse_ota_configure(struct ota_configure *conf, char *jsonfile) {
	char *p, *ip, *url, *rpt_url, *upgrade, *force_upgrade, *curr_ver, *new_ver;

	if (jsonfile == NULL)
		jsonfile = URL_CONFIG_FILE;
	printf("jsonfile name : %s\n", jsonfile);
	p = get_json_data(jsonfile);

	if (NULL == p) {
		printf("get_json_data failed, return\n");
		return;
	}

	cJSON * pJson = cJSON_Parse(p);
	if (NULL == pJson) {
		printf(" failed parse  json file:%s, return\n", jsonfile);
		free(p);
		return;
	} else {
		cJSON_Print(pJson);
	}

	cJSON * root = cJSON_GetObjectItem(pJson, "server");
	if (root) {
		ip =   cJSON_GetItemStringValue("null", root, "ip");
		url = cJSON_GetItemStringValue("null", root, "url");
		rpt_url = cJSON_GetItemStringValue("null", root, "rpt_url");

		upgrade = cJSON_GetItemStringValue("0", root, "upgrade");
		force_upgrade = cJSON_GetItemStringValue("0", root, "force_upgrade");
		curr_ver = cJSON_GetItemStringValue("0", root, "curr_ver");
		new_ver = cJSON_GetItemStringValue("0", root, "new_ver");

		sprintf(conf->ip, "%s", ip);
		sprintf(conf->url, "%s", url);
		sprintf(conf->rpt_url, "%s", rpt_url);

		conf->upgrade = atoi(upgrade);
		conf->force_upgrade = atoi(force_upgrade);
		conf->curr_ver = atoi(curr_ver);
		conf->new_ver = atoi(new_ver);
	}
	dump_ota_configure(conf);
	cJSON_Delete(pJson);
	free(p);
}
