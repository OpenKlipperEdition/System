/*
 * @file smarthome_scene.h - is provided for use with Ingenic products.
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
#ifndef SMARTHOME_SCENE_H
#define SMARTHOME_SCENE_H

/*******************
 *     INCLUDES   *
 *******************/

/*******************
 *     DEFINES    *
 *******************/
#define MUSIC_PATH "/assets/songs/"
#define MUSIC_SCENE_AT_WORK           MUSIC_PATH"work.wav"
#define MUSIC_SCENE_AT_HOME           MUSIC_PATH"rest.wav"
#define MUSIC_SCENE_SLEEP             MUSIC_PATH"night.wav"
#define MUSIC_SCENE_AT_DINNER         MUSIC_PATH"dinner.wav"
#define MUSIC_SCENE_AT_PARTY          MUSIC_PATH"party.wav"
#define MUSIC_SCENE_ON_VACATION       MUSIC_PATH"sport.wav"

/*******************
 *     TYPEDFS
 *******************/
enum _scene_mode_t {
	SCENE_AT_WORK = 0,
	SCENE_AT_HOME,
	SCENE_SLEEP,
	SCENE_AT_DINNER,
	SCENE_AT_PARTY,
	SCENE_ON_VACATION
} scene_mode_t;
/*******************
 * GLOBAL PROTOTYPES
 *******************/


/*******************
 *    MACROS
 *******************/
void create_scene_window(lv_obj_t* screen);
void destroy_scene_window(); //销毁创建的场景窗口

#endif  /* SMARTHOME_SCENE_H */
