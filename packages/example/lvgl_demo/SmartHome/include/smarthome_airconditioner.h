/*
 * @file smarthome_airconditioner.h - is provided for use with Ingenic products.
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
#ifndef SMARTHOME_AIRCONDITIONER_H
#define SMARTHOME_AIRCONDITIONER_H

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
void create_airconditioner_window(lv_obj_t* screen);
void destroy_airconditioner_window();

#endif  /* SMARTHOME_AIRCONDITIONER_H */
