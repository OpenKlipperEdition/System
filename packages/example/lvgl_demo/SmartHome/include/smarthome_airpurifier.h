/*
 * @file smarthome_airpurifier.h - is provided for use with Ingenic products.
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
#ifndef SMARTHOME_AIRPURIFIER_H
#define SMARTHOME_AIRPURIFIER_H

/*******************
 *     INCLUDES   *
 *******************/

/*******************
 *     DEFINES    *
 *******************/

/*******************
 *     TYPEDFS
 *******************/ 

/*******************
 * GLOBAL PROTOTYPES
 *******************/


/*******************
 *    MACROS
 *******************/
void create_airpurifier_window(lv_obj_t* screen);
void destroy_airpurifier_window();

#endif  /* SMARTHOME_AIRPURIFIER_H */
