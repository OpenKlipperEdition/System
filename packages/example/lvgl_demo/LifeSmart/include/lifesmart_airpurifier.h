/*
 *  @file lifesmart_airpurifier.h
 *
 */
#ifndef LIFESMART_AIRPURIFIER_H
#define LIFESMART_AIRPURIFIER_H

/*******************
 *     INCLUDES   *
 *******************/

/*******************
 *     DEFINES    *
 *******************/

/*******************
 *     TYPEDFS
 *******************/
enum AIRPURIFIER_GEAR {
	GEAR_LOW = 0,
	GEAR_MID,
	GEAR_HIGH,
};

/*******************
 * GLOBAL PROTOTYPES
 *******************/


/*******************
 *    MACROS
 *******************/
void create_airpurifier_window(lv_obj_t* screen);
void close_airpurifier_window();

#endif  /* LIFESMART_AIRPURIFIER_H */
