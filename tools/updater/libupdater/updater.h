#ifndef _UPDATER_H_
#define _UPDATER_H_

#include <stdint.h>

#ifdef  __cplusplus
extern "C" {
#endif


struct ota_configure {
    char ip[100];
    char url[1024];
    char rpt_url[1024];
    int upgrade;
    int force_upgrade;
    int curr_ver;
    int new_ver;
};

/**
 * @brief 检查ota 服务器的更新版本
 * @param url_config_file ota 服务器的url配置文件，用于指定服务器的地址
 *                            如果为NULL，则使用默认的文件 "/usr/data/ota_res/recovery.conf"
 * @return < 0 出错
 *         >= 0 ota版本
 */
int64_t updater_get_ota_version(const char *url_config_file);

/**
 * @brief 检查是否有更新可用
 * @param url_config_file ota 服务器的url配置文件，用于指定服务器的地址
 *                            如果为NULL，则使用默认的文件 "/usr/data/ota_res/recovery.conf"
 * @param version_file 记录当前版本的文件，用于同ota 服务器的最新版本做比较
 *                     如果为NULL，则使用默认的文件 "/usr/data/VERSION"
 * @return < 0 出错
 *         = 0 不需要更新
 *         > 0 需要更新,返回值正是更新的版本号
 */
int64_t updater_check_update(const char *url_config_file, const char *version_file);

/**
 * @brief 检查是否有更新可用,通过给定的ota版本
 * @param url_config_file ota 服务器的url配置文件，用于指定服务器的地址
 *                            如果为NULL，则使用默认的文件 "/usr/data/ota_res/recovery.conf"
 * @param version 当前的版本，用于同ota 服务器的最新版本做比较
 * @return < 0 出错
 *         = 0 不需要更新
 *         > 0 需要更新,返回值正是更新的版本号
 */
int64_t updater_check_update_by_versin(const char *url_config_file, int64_t version);

/**
 * @brief 将新的版本号写入文件
 * @param version_file 记录当前版本的文件
 *                     如果为NULL，则使用默认的文件 "/usr/data/VERSION"
 * @return < 0 出错
 *         = 0 成功
 */
int updater_save_version(const char *version_file, int64_t new_vesion);

void parse_ota_configure(struct ota_configure *conf, char *jsonfile);
void dump_ota_configure(struct ota_configure *conf);

#ifdef  __cplusplus
}
#endif

#endif /* _UPDATER_H_ */

