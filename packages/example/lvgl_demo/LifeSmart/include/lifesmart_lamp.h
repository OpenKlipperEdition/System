/*
 *  @file lifesmart_lamp.h
 *
 */
#ifndef LIFESMART_LAMP_H
#define LIFESMART_LAMP_H

/*******************
 *     INCLUDES   *
 *******************/

/*******************
 *     DEFINES    *
 *******************/

/*******************
 *     TYPEDFS
 *******************/
enum LAMP_LIGHT_COLOR {
	LIGHT_COLOR_WHITE = 0,
	LIGHT_COLOR_RED,
	LIGHT_COLOR_ORANGE,
	LIGHT_COLOR_YELLOW,
	LIGHT_COLOR_GREEN,
	LIGHT_COLOR_BLUE,
	LIGHT_COLOR_PINK,
	LIGHT_COLOR_PURPLE
};

/*******************
 * GLOBAL PROTOTYPES
 *******************/


/*******************
 *    MACROS
 *******************/
void create_lamp_window(lv_obj_t* screen);
void close_lamp_window();

#endif  /* LIFESMART_LAMP_H */
