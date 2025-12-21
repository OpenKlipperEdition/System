/**
 * @file date_time.c
 *
 * No license to Ingenic property rights is granted, Ingenic assumes no
 * liability, provides no warranty either expressed or implied relating
 * to the usage, or intellectual property right infringement except
 * as provided for by Ingenic Terms and Conditions of Sale.
 *
 * All rights reserved by Ingenic Semiconductor CO., LTD.
 *
 * Creator: yflu <yafei.lu@ingenic.com>
 * Maintainer: yflu <yafei.lu@ingenic.com>
 * Created: 2022/04/28
 * Updated:
 */


/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"
#include "sys/time.h"
#include "time.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "date_time.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static void get_current_system_time(date_time_t* dt)
{
    time_t now = time(0);
    struct tm* t = localtime(&now);

    dt->second = t->tm_sec;
    dt->minute = t->tm_min;
    dt->hour = t->tm_hour;
    dt->day = t->tm_mday;
    dt->month = t->tm_mon + 1;
    dt->year = t->tm_year + 1900;
    dt->wday = t->tm_wday;
}

static void set_current_system_time(date_time_t* dt)
{
    struct tm tms;
    time_t t = 0;
    memset(&tms, 0x00, sizeof(tms));

    tms.tm_year = dt->year - 1900;
    tms.tm_mon = dt->month - 1;
    tms.tm_mday = dt->day;
    tms.tm_hour = dt->hour;
    tms.tm_min = dt->minute;
    tms.tm_sec = dt->second;

    t = mktime(&tms);

    struct timeval tv;
    tv.tv_sec = t;
    tv.tv_usec = 0;
    if (settimeofday(&tv, (struct timezone*)0) < 0) {
       printf("stime failed\n");
    }
}

void date_time_from_time(date_time_t* dt, uint64_t time)
{
    time_t tm = time;
    struct tm* t = localtime(&tm);
    if (dt == NULL) return;

    memset(dt, 0x00, sizeof(date_time_t));

    dt->second = t->tm_sec;
    dt->minute = t->tm_min;
    dt->hour = t->tm_hour;
    dt->day = t->tm_mday;
    dt->day = t->tm_mon + 1;
    dt->year = t->tm_year + 1900;
    dt->wday = t->tm_wday;
}

date_time_t* date_time_create(void)
{
    date_time_t* dt = (date_time_t*)malloc(sizeof(date_time_t));

    return date_time_init(dt);
}

date_time_t* date_time_init(date_time_t* dt)
{
    if (dt == NULL) return NULL;
    memset(dt, 0x00, sizeof(date_time_t));

    get_current_system_time(dt);

    return dt;
}

void date_time_set(date_time_t* dt)
{
    if (dt == NULL) return;

    set_current_system_time(dt);
}

bool date_time_is_leap(uint32_t year)
{
    if ((year % 100) == 0) {
        return (year % 400) == 0;
    } else {
        return (year % 4) == 0;
    }
}

int32_t date_time_get_days(uint32_t year, uint32_t month)
{
    int days = 0;
    int days_of_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12) return -1;

    days = days_of_month[month - 1];
    if (days == 28) {
        if (date_time_is_leap(year)) {
            days = 29;
        }
    }

    return days;
}

int32_t date_time_get_wday(uint32_t year, uint32_t month, uint32_t day)
{
    int32_t w = 0;
    uint32_t y = year;
    uint32_t m = month;
    uint32_t d = day;

    if (month < 1 || month > 12) return -1;
    if (date_time_get_days(year, month) < day || day <= 0) return -1;

    if (y >= 1582 && m >= 10 && d > 4) {
        /*1582年10月4日后：w = (d + 1+ 2*m+3*(m+1)/5+y+y/4-y/100+y/400)%7;*/
        w = (d + 1 + 2 * m + 3 * (m + 1) / 5 + y + y / 4 - y / 100 + y / 400) % 7;
    } else {
        /*1582年10月4日前：w = (d+1+2*m+3*(m+1)/5+y+y/4+5) % 7;*/
        w = (d + 1 + 2 * m + 3 * (m + 1) / 5 + y + y / 4 + 5) % 7;
    }

    /*Sunday = 0*/
    w = (w + 1) % 7;

    return w;
}





