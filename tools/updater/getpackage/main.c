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
#include <cjson/cJSON.h>
#include "cjson_helper.h"

#define VERSION_FILE "/usr/data/VERSION"
#define TMP_VERSION_CONF_FILE "/usr/data/current_version_full.conf"

static const char* prefix_local_update_path = "/storage/update";
static const char* base_version_file = "BASE_VERSION";
static const char* remote_diffpkg_dirname = "difference";
static const char* remote_fullpkg_dirname = "full";
static const char* prefix_sha1_table = "sha1Tab";
static const char* prefix_global_xml = "global.xml";

struct ota_configure {
    char ip[100];
    char url[1024];
    char rpt_url[1024];
    int upgrade;
    int force_upgrade;
    int curr_ver;
    int new_ver;
};

struct sysinfo_flag_nv {
	unsigned int version;
	unsigned int boot;
	unsigned int step;
	unsigned int start;
	unsigned int finish;
	unsigned int needfullpkg;//use for diffpkg failed
};

static int nv_read(struct sysinfo_flag_nv* nv) {
	FILE *fp = NULL;
	char file_path[32] = {0};

	sprintf(file_path, "/dev/mmcblk%dp3", MMC_INDEX);

	fp=fopen(file_path, "rb");
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
static int check_pkg_sha1(const char* pkgpath, const char* sha1path) {
	FILE *fp=NULL;
	char cmd[64]={0};
	char buff[64]={0};
	int ret = 0;

	sprintf(cmd,"sha1sum %s", pkgpath);
	fp=popen(cmd, "r");
    if (fp == NULL) {
        printf("popen error,cmd: %s: %s\n", cmd, strerror(errno));
        return -1;
    }
	memset(buff,0,sizeof(buff));
	fread(buff,1,40,fp);
	printf("%s",buff);
	pclose(fp);

	fp = fopen(sha1path, "r");
	if (fp == NULL) {
		printf("Failed to open %s: %s\n", sha1path, strerror(errno));
		return -1;
	}
	char sha1[64];
	memset(&sha1, 0, 64);
	fgets(sha1, 64, fp);
	printf("%s",sha1);
	fclose(fp);

	if(strncmp(buff, sha1, 40)) {
		printf("check sha1 fail!\n");
		ret = -1;
	}else{
		printf("check sha1 pass!\n");
		ret = 0;
	}

	return ret;
}

int download_file(const char* file, const char* path) {
    int error = 0;
    int status = 0;
    pid_t pid;

	if (access("/usr/bin/wget", F_OK | R_OK | X_OK)){
        printf("/usr/bin/wget not execuable\n");
        return -1;
    }

    pid = fork();
    if (pid < 0) {
        printf("fork() fail: %s\n", strerror(errno));
        return -1;
    }

    if (!pid) {
        error = execl("/usr/bin/wget", "wget", "-c", "-T", "20", "-q", file,
                "-P", path, "--no-check-certificate", (char*) 0);
        if (error < 0) {
            printf("execl() fail: %s\n", strerror(errno));
            return -1;
        }

    } else {
        while(waitpid(pid, &status, 0) < 0) {
            if(errno != EINTR){
                status = -1;
                break;
            }
        }
    }

    if (status)
        return -1;

    return 0;
}
int download_update_package(){
	char path[1024] = {0};

	char cmd[512] = {0};
	sprintf(cmd,"rm -rf %s",prefix_local_update_path);
	system(cmd);

	struct ota_configure conf;
	memset(&conf, 0, sizeof(struct ota_configure));
	parse_ota_configure(&conf, TMP_VERSION_CONF_FILE);

	/*
	 * Download BASE_VERSION
	 */
	memset(path, 0, sizeof(path));
	snprintf(path,strlen(conf.url)+strlen(base_version_file)+2, "%s/%s", conf.url, base_version_file);
	printf("===>>%s:%d:download %s\n",__func__,__LINE__,path);
	if (download_file(path, prefix_local_update_path) == 0) {
		memset(path, 0, sizeof(path));
		snprintf(path, strlen(prefix_local_update_path)+strlen(base_version_file)+2, "%s/%s", prefix_local_update_path,base_version_file);
		int64_t bv=get_version(path);
		int64_t cv=get_version(VERSION_FILE);
		if(bv != cv) {
			printf("current version is not base version,current_version=%lld,base_version=%lld\n",cv,bv);
			goto error;
		}
	}else
		printf("not found %s, maybe full package ...\n",base_version_file);

	int retry_count = 0;
	while(1) {
		memset(path, 0, sizeof(path));
		sprintf(path, "%s/mmc/update.zip", conf.url);
		printf("===>>%s:%d:download %s\n",__func__,__LINE__,path);
		if (download_file(path, prefix_local_update_path) < 0) {
			printf("Failed to download %s to %s\n", path,  prefix_local_update_path);
			goto error_retry;
		}
		break;

error_retry:
		if (retry_count++ < 10) {
			usleep(2 * 1000 * 1000);
			printf("retrying downlod cnt: %d\n", retry_count);
		} else {
			printf("failed after retrying cnt: %d\n", retry_count);
			goto error;
		}
	}
	return 0;

error:
	return -2;
}

int download_update_package1(){
	int download_fullpkg = 0;
	char path[1024] = {0};
	char pkg_url[1024] = {0};

	char cmd[512] = {0};
	sprintf(cmd,"rm -rf %s",prefix_local_update_path);
	system(cmd);

	struct ota_configure conf;
	memset(&conf, 0, sizeof(struct ota_configure));
	parse_ota_configure(&conf, TMP_VERSION_CONF_FILE);

	struct sysinfo_flag_nv sysinfo_flag_nv;
	memset(&sysinfo_flag_nv, 0, sizeof(struct sysinfo_flag_nv));
	nv_read(&sysinfo_flag_nv);

	if(sysinfo_flag_nv.needfullpkg)
		download_fullpkg = 1;
	else {
		memset(path, 0, sizeof(path));
		snprintf(path,strlen(conf.url)+strlen(remote_diffpkg_dirname)+strlen(prefix_global_xml)+3, "%s/%s/%s", conf.url, remote_diffpkg_dirname, prefix_global_xml);
		printf("===>>%s:%d:download %s\n",__func__,__LINE__,path);
		if (download_file(path, prefix_local_update_path) < 0) {
			printf("no difference package found\n");
			download_fullpkg = 1;
			sysinfo_flag_nv.needfullpkg = 1;
			nv_write(&sysinfo_flag_nv);
		}
	}
	if(download_fullpkg){
		memset(pkg_url, 0, sizeof(path));
		snprintf(pkg_url, strlen(conf.url)+strlen(remote_fullpkg_dirname)+2, "%s/%s", conf.url, remote_fullpkg_dirname);
		/*
		 * Download global.xml
		 */
		memset(path, 0, sizeof(path));
		snprintf(path,strlen(pkg_url)+strlen(prefix_global_xml)+2, "%s/%s", pkg_url, prefix_global_xml);
		printf("===>>%s:%d:download %s\n",__func__,__LINE__,path);
		if (download_file(path, prefix_local_update_path) < 0) {
			printf("Failed to download %s\n", path);
			goto error;
		}
	}else{
		memset(pkg_url, 0, sizeof(path));
		snprintf(pkg_url, strlen(conf.url)+strlen(remote_diffpkg_dirname)+2, "%s/%s", conf.url, remote_diffpkg_dirname);
		/*
		 * Download BASE_VERSION
		 */
		memset(path, 0, sizeof(path));
		snprintf(path,strlen(pkg_url)+strlen(base_version_file)+2, "%s/%s", pkg_url, base_version_file);
		printf("===>>%s:%d:download %s\n",__func__,__LINE__,path);
		if (download_file(path, prefix_local_update_path) < 0) {
			printf("Failed to download %s\n", path);
			goto error;
		}
		memset(path, 0, sizeof(path));
		snprintf(path, strlen(prefix_local_update_path)+strlen(base_version_file)+2, "%s/%s", prefix_local_update_path,base_version_file);
		int64_t bv=get_version(path);
		int64_t cv=get_version(VERSION_FILE);
		if(bv != cv) {
			printf("current version is not base version,current_version=%lld,base_version=%lld\n",cv,bv);
			sysinfo_flag_nv.needfullpkg = 1;
			nv_write(&sysinfo_flag_nv);
			goto error;
		}
	}
	/*
	 * Download sha1Tab
	 */
	memset(path, 0, sizeof(path));
	snprintf(path, strlen(pkg_url)+strlen(prefix_sha1_table)+2, "%s/%s", pkg_url,prefix_sha1_table);
	printf("===>>%s:%d:download %s\n",__func__,__LINE__,path);
	if (download_file(path, prefix_local_update_path) < 0) {
		printf("Failed to download %s to %s\n", path,  prefix_local_update_path);
		goto error;
	}
	int retry_count = 0;
	while(1) {
		memset(path, 0, sizeof(path));
		sprintf(path, "%s/mmc/update.zip", pkg_url);
		printf("===>>%s:%d:download %s\n",__func__,__LINE__,path);
		if (download_file(path, prefix_local_update_path) < 0) {
			printf("Failed to download %s to %s\n", path,  prefix_local_update_path);
			goto error_retry;
		}

		memset(path, 0, sizeof(path));
		sprintf(path, "%s/update.zip", prefix_local_update_path);
		char sha1path[512];
		memset(sha1path, 0, sizeof(sha1path));
		sprintf(sha1path,"%s/%s", prefix_local_update_path, prefix_sha1_table);
		if(check_pkg_sha1(path, sha1path) < 0)
			goto error_retry;
		break;

error_retry:
		if (retry_count++ < 10) {
			usleep(2 * 1000 * 1000);
			printf("retrying downlod cnt: %d\n", retry_count);
		} else {
			printf("failed after retrying cnt: %d\n", retry_count);
			goto error;
		}
	}
	return 0;

error:
	return -2;
}

int main (int argc, char *argv[])
{
	int ret = -1;
	printf("start download package ...\n");
	if(!download_update_package()){
		ret = 0;
	}
	return ret;
}
