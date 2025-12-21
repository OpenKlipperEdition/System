/*
 *  @file lifesmart_scene.h
 *
 */
#ifndef LIFESMART_SCENE_H
#define LIFESMART_SCENE_H

/*******************
 *     INCLUDES   *
 *******************/

/*******************
 *     DEFINES    *
 *******************/
#define MUSIC_PATH "/assets/songs/"
#define MUSIC_SCENE_AT_WORK        MUSIC_PATH"work.wav"
#define MUSIC_SCENE_STUDYING        MUSIC_PATH"study.wav"
#define MUSIC_SCENE_AT_DINNER      MUSIC_PATH"dinner.wav"
#define MUSIC_SCENE_AT_PARTY       MUSIC_PATH"party.wav"
#define MUSIC_SCENE_MORNING       MUSIC_PATH"morning.wav"
#define MUSIC_SCENE_AT_NIGHT      MUSIC_PATH"night.wav"
#define MUSIC_SCENE_DO_SPORTS    MUSIC_PATH"sport.wav"
#define MUSIC_SCENE_HAVE_REST    MUSIC_PATH"rest.wav"
/*******************
 *     TYPEDFS
 *******************/
enum _scene_mode_t {
	SCENE_AT_WORK = 0,
	SCENE_STUDYING,
	SCENE_AT_DINNER,
	SCENE_AT_PARTY,
	SCENE_MORNING,
	SCENE_AT_NIGHT,
	SCENE_DO_SPORTS,
	SCENE_HAVE_REST
} scene_mode_t;
/*******************
 * GLOBAL PROTOTYPES
 *******************/


/*******************
 *    MACROS
 *******************/
void create_scene_window(lv_obj_t* screen);
void close_scene_window();

#endif  /* LIFESMART_SCENE_H */
