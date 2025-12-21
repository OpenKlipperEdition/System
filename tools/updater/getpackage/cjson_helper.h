#ifndef _CJSON_HELPER_H_
#define _CJSON_HELPER_H_

#include "little_things.h"
#include <cjson/cJSON.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 本文件提供对cJSON的快速调用,提供如下的宏作为调用
 *
 * cJSON_GetItem : 快速获得子json对象
 * cJSON_GetItemStringValue : 快速获得子json对象的valuestring,必须是cJSON_String类型
 * cJSON_GetItemIntValue    : 快速获得子json对象的valueint,必须是cJSON_Number类型
 * cJSON_GetItemDoubleValue : 快速获得子json对象的valuedouble,必须是cJSON_Number类型
 *
 */

/**
 * 快速获得子json对象
 * @param json: 父json, 指向 cJSON 结构体的指针
 * @param x:    可变参数,用于描述子json在父json中的位置
 *              x: const char *: 表示一般的json对象name
 *              x: int: 表示json数组中的index
 * @return_val: 子json对象,如果不成功,那么返回NULL
 *
 * 例子:
 *   // json_str 的内容如下
 *   {
 *      "root" : {
 *          "child0" : {
 *              "child1" : {
 *                  "item0" : "hello obj",
 *                  "item1" : "hello obj again"
 *              }
 *          },
 *          "array" : [
 *              {"item0" : "hello array"},
 *              {"item1" : "hello array again"}
 *          ]
 *      }
 *   }
 *
 *   cJSON *root = cJSON_Parse(json_str);
 *
 *   cJSON *item_obj = cJSON_GetItem(cjson, "root", "child0", "child1", "item0");
 *   if (item_obj)
 *      printf("root.child0.child1.item0: %s\n", item_obj->valuestring);
 *
 *   cJSON *item_arr = cJSON_GetItem(cjson, "root", "array", 0, "item0");
 *   if (item_arr)
 *      printf("root.array[0].item0: %s\n", item_arr->valuestring);
 *
 */
#define cJSON_GetItem(json, x...) cJSON_GetItem_va(json, comac_argc(x), x)

/**
 * 详情请看 cJSON_GetItem 的说明
 * cJSON_GetItemStringValue : 快速获得子json对象的valuestring,必须是cJSON_String类型
 * cJSON_GetItemIntValue    : 快速获得子json对象的valueint,必须是cJSON_Number类型
 * cJSON_GetItemDoubleValue : 快速获得子json对象的valuedouble,必须是cJSON_Number类型
 * @param error_val : 解析错误时的返回值
 */
#define cJSON_GetItemStringValue(error_val, json, x...) cJSON_GetItemStringValue_va(json, error_val, comac_argc(x), x)
#define cJSON_GetItemIntValue(error_val, json, x...) cJSON_GetItemIntValue_va(json, error_val, comac_argc(x), x)
#define cJSON_GetItemDoubleValue(error_val, json, x...) cJSON_GetItemDoubleValue_va(json, error_val, comac_argc(x), x)

/*
 * functions called by helper macros, don't care those functions
 */
cJSON *cJSON_GetItem_va(cJSON *json, int n, ...);
const char *cJSON_GetItemStringValue_va(cJSON *json, const char *error_val, int n, ...);
int cJSON_GetItemIntValue_va(cJSON *json, int error_val, int n, ...);
double cJSON_GetItemDoubleValue_va(cJSON *json, double error_val, int n, ...);

#ifdef __cplusplus
}
#endif

#endif /* _CJSON_HELPER_H_ */
