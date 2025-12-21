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

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


#include <utils/log.h>
#include <utils/assert.h>
#include <configure/configure_file.h>
#include <utils/file_ops.h>
#include <lib/config/libconfig.h>
#include <version.h>
#include "cjson_helper/cjson_helper.h"

#define LOG_TAG "configure_file"

static const char* prefix_application = "Application";
static const char* prefix_version = "Version";
static const char* prefix_server_setting = "Server";
static const char* prefix_server_ip = "ip";
static const char* prefix_server_url = "url";

struct ota_configure {
    char ip[100];
    char url[1024];
    char rpt_url[1024];
    int upgrade;
    int force_upgrade;
    int curr_ver;
    int new_ver;
};


static void dump(struct configure_file* this) {
    LOGI("=========================\n");
    LOGI("Dump configure file\n");
    LOGI("Version:    %s\n", this->version);
    LOGI("Server IP:  %s\n", this->server_ip);
    LOGI("Server URL: %s\n", this->server_url);
    LOGI("=========================\n");
}

static char *get_json_data(const char *jsonfile) {
	FILE *f_json = NULL;

	long json_size;

	char *json_data = NULL;
	LOGI("open %s\n", jsonfile);
	f_json = fopen(jsonfile, "r");
	if (f_json == NULL) {
		LOGI("open %s  failed\n", jsonfile);
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

static void dump_ota_configure(struct ota_configure *conf) {
	LOGI("======dump_ota_configure======\n");
	LOGI("ip: %s\n", conf->ip);
	LOGI("url: %s\n", conf->url);
	LOGI("rpt_url: %s\n", conf->rpt_url);
 	LOGI("upgrade: %d\n", conf->upgrade);
	LOGI("force_upgrade: %d\n", conf->force_upgrade);
	LOGI("curr_ver: %d\n", conf->curr_ver);
	LOGI("new_ver: %d\n", conf->new_ver);
	LOGI("======dump_end======\n");
}

static void parse_ota_configure(struct ota_configure *conf, char *jsonfile) {
	LOGI("jsonfile name : %s\n", jsonfile);
	char *p = get_json_data(jsonfile);

	char *ip;
	char *url;
	char *rpt_url;

	if (NULL == p) {
		LOGI("get_json_data failed, return\n");
		return;
	}
	cJSON * pJson = cJSON_Parse(p);
	if (NULL == pJson) {
		LOGI(" failed parse  json file:%s, return\n", jsonfile);
		free(p);
		return;
	} else {
		cJSON_Print(pJson);
	}
	cJSON * root = cJSON_GetObjectItem(pJson, "server");
	if (root) {
		ip = cJSON_GetItemStringValue("null", root, "ip");
		sprintf(conf->ip, "%s", ip);
		url = cJSON_GetItemStringValue("null", root, "url");
		sprintf(conf->url, "%s", url);
		rpt_url = cJSON_GetItemStringValue("null", root, "rpt_url");
		sprintf(conf->rpt_url, "%s", rpt_url);
/*		conf->upgrade = cJSON_GetItemIntValue(0, root, "upgrade");
		conf->force_upgrade = cJSON_GetItemIntValue(0, root, "force_upgrade");
		conf->curr_ver = cJSON_GetItemIntValue(0, root, "curr_ver");
		conf->new_ver = cJSON_GetItemIntValue(0, root, "new_ver");
*/
	}
	cJSON_Delete(pJson);
	free(p);
}

static int parse(struct configure_file* this, const char* path) {

	assert_die_if(path == NULL, "path is NULL");

	struct ota_configure conf;

	memset(&conf, 0, sizeof(conf));
	parse_ota_configure(&conf, path);
	dump_ota_configure(&conf);

	if (strlen(conf.ip) != 0) {
		this->server_ip = strdup(conf.ip);
	}

	if (strlen(conf.url) != 0) {
		this->server_url = strdup(conf.url);
	}
	dump(this);
	return 0;

}

static int parse_current_version(struct configure_file* this) {
//    const char *version_path = "/usr/data";
    const char *version_file = "/usr/data/current_version_full.conf";
//    struct configure_file config;
    assert_die_if(this->server_url == NULL, "server_url is NULL");

    struct ota_configure conf;

	memset(&conf, 0, sizeof(conf));
	parse_ota_configure(&conf, version_file);
	dump_ota_configure(&conf);

	if (strlen(conf.ip) != 0) {
		this->server_ip = strdup(conf.ip);
	}

	if (strlen(conf.url) != 0) {
		this->server_url = strdup(conf.url);
	}

	dump(this);
	return 0;
}

void construct_configure_file(struct configure_file* this) {
    this->parse = parse;
    this->dump = dump;
    this->parse_current_version = parse_current_version;
}

void destruct_configure_file(struct configure_file* this) {
    if (this->version)
        free(this->version);
    if (this->server_ip)
        free(this->server_ip);
    if (this->server_url)
        free(this->server_url);

    this->parse = NULL;
    this->dump = NULL;
    this->parse_current_version = NULL;
}
