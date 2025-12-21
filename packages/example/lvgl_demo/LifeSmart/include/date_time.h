/*
 * @file date_time.h
 *
 */

#ifndef DATE_TIME_H
#define DATE_TIME_H


/*******************
 *     INCLUDES   *
 *******************/

/*******************
 *     DEFINES    *
 *******************/

/*******************
 *     TYPEDFS
 *******************/ 
typedef struct _date_time_t {
    int32_t second;
    int32_t minute;
    int32_t hour;
    int32_t day;
    int32_t wday;
    int32_t month;
    int32_t year;
} date_time_t;

/*******************
 * GLOBAL PROTOTYPES
 *******************/

/*******************
 *    MACROS
 *******************/
/**
 * @method date_time_create
 * 创建date_time对象，初始值为当前日期和时间
 * 
 * @return {date_time_t*} 返回date_time对象。
 */
date_time_t* date_time_create(void);

/**
 * @method date_time_ini
 * 初始为当前日期和时间。
 * @param {date_time_t*} dt date_time对象
 * 
 * @return {date_time_t*} 返回date_time对象。
 */
date_time_t* date_time_init(date_time_t* dt);

/**
 * @method date_time_set
 * 设置当前时间。
 *
 * @param {date_time_t*} dt date_time对象。
 *
 */
void date_time_set(date_time_t* dt);

/**
 * @method date_time_from_time
 * 从time转换而来。
 *
 * @param {date_time_t*} dt date_time对象。
 * @param {uint64_t} time 时间。
 *
 */
void date_time_from_time(date_time_t* dt, uint64_t time);

/**
 * @method date_time_get_days
 * 获取指定年份月份的天数。
 * 
 * @param {uint32_t} year 年份
 * @param {uint32_t} month 月份(1-12)
 * 
 * @return {int32_t} 返回大于0表示天数，否则表示失败。
 */
int32_t date_time_get_days(uint32_t year, uint32_t month);

/**
 * @method date_time_get_wday
 * 获取指定日期是周几(0-6, 0->Sunday)。
 * 
 * @param {uint32_t} year 年份
 * @param {uint32_t} month 月份(1-12)
 * @param {uint32_t} day 日(1-31)
 * 
 * @return {int32_t} 返回大于等于0表示周几(0-6)，否则表示失败。
 */
int32_t date_time_get_wday(uint32_t year, uint32_t month, uint32_t day);


#endif /* DATE_TIME_H */