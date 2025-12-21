/*
 *  @file lifesmart_airconditioner.h
 *
 */
#ifndef LIFESMART_AIRCONDITIONER_H
#define LIFESMART_AIRCONDITIONER_H

/*******************
 *     INCLUDES   *
 *******************/

/*******************
 *     DEFINES    *
 *******************/

/*******************
 *     TYPEDFS
 *******************/
enum AIRCONDITIONER_MODE {
	MODE_COOLING = 0,
	MODE_HEATING,
	MODE_DEHUMIFY,
	MODE_WIND,
};

/*******************
 * GLOBAL PROTOTYPES
 *******************/


/*******************
 *    MACROS
 *******************/
void create_airconditioner_window(lv_obj_t* screen);
void close_airconditioner_window();

#endif  /* LIFESMART_AIRCONDITIONER_H */
