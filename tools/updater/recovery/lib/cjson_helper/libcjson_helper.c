#include <stdio.h>
#include <stdarg.h>
#include "cjson_helper/cjson_helper.h"

static cJSON *cJSON_GetItem_valist(cJSON *json, int n, va_list list)
{
    int i;
    cJSON *js = json;

    for (i = 0; i < n; i++) {
        int index;
        const char *str;

        if (js->type == cJSON_Array) {
            index = va_arg(list, int);
            js = cJSON_GetArrayItem(js, index);
        } else {
            str = va_arg(list, const char *);
            js = cJSON_GetObjectItem(js, str);
        }

        if (js == NULL)
            break;
    }

    return js;
}

cJSON *cJSON_GetItem_va(cJSON *json, int n, ...)
{
    cJSON *js;
    va_list list;

    va_start(list, n);
    js = cJSON_GetItem_valist(json, n, list);
    va_end(list);

    return js;
}

const char *cJSON_GetItemStringValue_va(cJSON *json, const char *error_val, int n, ...)
{
    cJSON *js;
    va_list list;

    va_start(list, n);
    js = cJSON_GetItem_valist(json, n, list);
    va_end(list);

    if (!js || js->type != cJSON_String)
        return error_val;
    else
        return js->valuestring;
}

int cJSON_GetItemIntValue_va(cJSON *json, int error_val, int n, ...)
{
    cJSON *js;
    va_list list;

    va_start(list, n);
    js = cJSON_GetItem_valist(json, n, list);
    va_end(list);

    if (!js || js->type != cJSON_Number)
        return error_val;
    else
        return js->valueint;
}

double cJSON_GetItemDoubleValue_va(cJSON *json, double error_val, int n, ...)
{
    cJSON *js;
    va_list list;

    va_start(list, n);
    js = cJSON_GetItem_valist(json, n, list);
    va_end(list);

    if (!js || js->type != cJSON_Number)
        return error_val;
    else
        return js->valuedouble;
}
