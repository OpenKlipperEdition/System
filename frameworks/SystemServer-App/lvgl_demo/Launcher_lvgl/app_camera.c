/*********************
 *       INCLUDES
 *********************/
#include "lvgl/lvgl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dlfcn.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include "systemserver_config.h"
#include "iss_camera.h"
#include <unistd.h>
#include "iss_osd.h"
#include "encoder.h"


#define FONT_PATH						"/assets/fonts/sourcehan.ttf"
#define FUNC_TABVIEW_W					280
#define FUNC_TABVIEW_H					220
#define TABVIEW_TAB_H					80
#define TAB_BTN_RADIUS					50
#define CAMERA_PREVIEW_DEV_NODE			"/dev/video4"
#define CAMERA_PREVIEW_WIDTH			640
#define CAMERA_PREVIEW_HEIGHT			480
#define OSD_CAMERA_PREVIEW_WIDTH		720
#define OSD_CAMERA_PREVIEW_HEIGHT		928
#define OSD_CAMERA_PREVIEW_Y_OFFSET		88
#define CAMERA_SAVE_DEV_NODE			"/dev/video5"
#define CAMERA_SAVE_WIDTH				1280
#define CAMERA_SAVE_HEIGHT				720


static lv_obj_t * main_page;
static lv_obj_t * close_btn;
static lv_ft_info_t font_info;
static lv_style_t style_container_default;
static lv_obj_t * tab_container;
static lv_obj_t * tabview;
static lv_obj_t * tab_picture;
static lv_obj_t * tab_video;
static lv_obj_t * tab_btns;
static lv_obj_t * btn_picturing;
static lv_obj_t * btn_video;
static lv_obj_t * label_timer;
static lv_timer_t* record_timer;
static lv_timer_t* toast_timer;
static int record_time = 0;

static ISS_CamHandle_t preview_cam_handle;
static struct OsdClient *preview_osd_clt = NULL;
static pthread_t preview_thread_tid;
static EncChnHandle_t jpeg_enc_handle;
static pthread_cond_t btn_picturing_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t btn_picturing_mutex = PTHREAD_MUTEX_INITIALIZER;
static EncChnHandle_t h264_enc_handle;
static bool h264enc_start_flag = false;
static pthread_mutex_t h264enc_start_mutex = PTHREAD_MUTEX_INITIALIZER;


LV_IMG_DECLARE(take_photos)
LV_IMG_DECLARE(take_videos)
LV_IMG_DECLARE(record_start)
LV_IMG_DECLARE(back_home)


static void destroy_camera_ui(void);
static void deinitialize_camera_preview(void);


struct test_priv {
    int fd;
}finish_cb_priv;


#define TEST_FPS			0
#if TEST_FPS
static unsigned long long get_time_ms(void)
{
        struct timeval tv;
        gettimeofday(&tv, NULL);

        return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static int calc_fps(void)
{

        static unsigned long long last_time = 0;
        static int framecount = 0;
        static int fps = 0;
        unsigned long long cur_time = get_time_ms();

        if (last_time == 0) {
                last_time = get_time_ms();
        }

        framecount ++;
        if (cur_time - last_time >= 1000) {
                fps = framecount;
                framecount = 0;
                last_time = cur_time;
                printf("fps:%d\n", fps);
        }

        return fps;
}
#endif

static void* preview_thread(void* arg)
{
	DataBuffer_t buf;
	int ret = 0;

	while (1) {
		ret = ISS_GetCameraData(preview_cam_handle, &buf);
		if (!ret) {
			ISS_Flush(preview_osd_clt, buf.vaddr, buf.size);
			ISS_ReleaseCameraData(preview_cam_handle, &buf);
		} else {
			usleep(5*1000);
		}
	}
}


void jpeg_stream_callback(DataBuffer_t* buffer, void* priv)
{
    struct test_priv* cb_priv = (struct test_priv*)priv;

	pthread_mutex_lock(&btn_picturing_mutex);

	if (cb_priv->fd != -1) {
		write(cb_priv->fd, buffer->vaddr, buffer->size);
		close(cb_priv->fd);
		cb_priv->fd = -1;
	}

	pthread_cond_signal(&btn_picturing_cond);

	pthread_mutex_unlock(&btn_picturing_mutex);
}

static void btn_picturing_clicked(lv_event_t* e)
{
	EncoderChnParam_t chn_param;
	char cap_filename[128];
	struct timeval tv;
    struct tm *lt;

	memset(&chn_param, 0, sizeof(EncoderChnParam_t));
	chn_param.srcType = SRC_CAMERA;
	sprintf(chn_param.videoNode, CAMERA_SAVE_DEV_NODE);
	chn_param.srcBufferNum = 2;
	chn_param.dstBufferNum = 2;
	chn_param.param.type = ENC_JPEG;
	ENC_JPEG_Param_t *enc_param = &chn_param.param.EncParam.jpeg_param;
	enc_param->qp = 35;
	enc_param->quality = 50;
	enc_param->enc_width = CAMERA_SAVE_WIDTH;
	enc_param->enc_height = CAMERA_SAVE_HEIGHT;
	enc_param->srcfmt = PIX_FMT_NV12;

	jpeg_enc_handle = ISS_CreateEncodeChn(&chn_param);
	if (!jpeg_enc_handle) {
	    printf("%s create encode chn failed ##\r\n",__func__);
	    exit(-1);
	}

	memset(cap_filename, 0, sizeof(cap_filename));
    gettimeofday(&tv, NULL);
    lt = localtime(&tv.tv_sec);
	sprintf(cap_filename, "/tmp/IMG_%d%02d%02d_%02d%02d%02d%06ld.jpg",
			lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
			lt->tm_hour, lt->tm_min, lt->tm_sec, tv.tv_usec);

	finish_cb_priv.fd = open(cap_filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
	EncoderFinish_Cb_t finish_cb;
	finish_cb.enc_finish_cb = jpeg_stream_callback;
	finish_cb.priv = &finish_cb_priv;
	ISS_EncodeChn_SetFinishCallBack(jpeg_enc_handle, &finish_cb);

	ISS_EncodeChn_Start(jpeg_enc_handle);

	pthread_mutex_lock(&btn_picturing_mutex);
	pthread_cond_wait(&btn_picturing_cond, &btn_picturing_mutex);
	pthread_mutex_unlock(&btn_picturing_mutex);

	ISS_EncodeChn_Stop(jpeg_enc_handle);
	ISS_DestroyEncodeChn(jpeg_enc_handle);
}


static void record_timer_countdown(lv_timer_t *t)
{
    char result[1024];
    record_time++;
    int second = (record_time % 3600) % 60;
    int minute = (record_time % 3600) / 60;
    int hour = record_time / 3600;

    sprintf(result, "%02d:%02d:%02d", hour, minute, second);
    lv_label_set_text(label_timer, result);
}


void h264enc_stream_callback(DataBuffer_t* buffer,void* priv)
{
    struct test_priv* cb_priv = (struct test_priv*)priv;

	if (-1 != cb_priv->fd)
		write(cb_priv->fd, buffer->vaddr, buffer->size);
}


static void btn_video_triggered(lv_event_t* e)
{
	lv_obj_t * button = lv_event_get_target(e);
	if(lv_obj_has_state(button, LV_STATE_CHECKED)) {  //开始拍摄
		lv_obj_clear_flag(lv_tabview_get_content(tabview), LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_add_state(tab_btns, LV_STATE_DISABLED);

		EncoderChnParam_t  chn_param;
		memset(&chn_param, 0, sizeof(EncoderChnParam_t));
		chn_param.srcType = SRC_CAMERA;
		sprintf(chn_param.videoNode, CAMERA_SAVE_DEV_NODE);
		chn_param.srcBufferNum = 2;
		chn_param.dstBufferNum = 2;
		chn_param.param.type = ENC_H264;

		ENC_H26x_Param_t *enc_param = &chn_param.param.EncParam.h26x_param;
		enc_param->rc_mode = RC_MODE_VBR;
		enc_param->bitrate = 1500;
		enc_param->frameRate = 30;
		enc_param->gop = 90;
		enc_param->min_qp = 30;
		enc_param->max_qp = 45;
		enc_param->idrFreq = 30;
		enc_param->enc_width = CAMERA_SAVE_WIDTH;
		enc_param->enc_height = CAMERA_SAVE_HEIGHT;
		enc_param->srcfmt = PIX_FMT_NV12;

		h264_enc_handle = ISS_CreateEncodeChn(&chn_param);
		if(!h264_enc_handle){
		    printf("%s create encode chn failed ##\r\n",__func__);
		    exit(-1);
		}

		char cap_filename[128];
		struct timeval tv;
    	struct tm *lt;
		memset(cap_filename, 0, sizeof(cap_filename));
    	gettimeofday(&tv, NULL);
    	lt = localtime(&tv.tv_sec);
		sprintf(cap_filename, "/tmp/VID_%d%02d%02d_%02d%02d%02d%06ld.h264",
				lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
				lt->tm_hour, lt->tm_min, lt->tm_sec, tv.tv_usec);

		finish_cb_priv.fd = open(cap_filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
		EncoderFinish_Cb_t finish_cb;
		finish_cb.enc_finish_cb = h264enc_stream_callback;
		finish_cb.priv = &finish_cb_priv;
		ISS_EncodeChn_SetFinishCallBack(h264_enc_handle, &finish_cb);

		ISS_EncodeChn_Start(h264_enc_handle);

		pthread_mutex_lock(&h264enc_start_mutex);
		h264enc_start_flag = true;
		pthread_mutex_unlock(&h264enc_start_mutex);

		lv_obj_clear_flag(label_timer, LV_OBJ_FLAG_HIDDEN);
		record_timer = lv_timer_create(record_timer_countdown, 1000, NULL);
	} else {	// 拍摄结束
		lv_timer_del(record_timer);
		record_timer = NULL;
		record_time = 0;
		lv_label_set_text(label_timer, "00:00:00");
		lv_obj_add_flag(label_timer, LV_OBJ_FLAG_HIDDEN);

		ISS_EncodeChn_Stop(h264_enc_handle);
		pthread_mutex_lock(&h264enc_start_mutex);
		h264enc_start_flag = false;
		pthread_mutex_unlock(&h264enc_start_mutex);
		close(finish_cb_priv.fd);
		finish_cb_priv.fd = -1;
		ISS_DestroyEncodeChn(h264_enc_handle);

		lv_obj_clear_state(tab_btns, LV_STATE_DISABLED);
		lv_obj_add_flag(lv_tabview_get_content(tabview), LV_OBJ_FLAG_SCROLLABLE);
	}

}

static void scroll_begin_event(lv_event_t* e)
{
    /*Disable the scroll animations. Triggered when a tab button is clicked */
    if(lv_event_get_code(e) == LV_EVENT_SCROLL_BEGIN) {
        lv_anim_t * a = lv_event_get_param(e);
        if(a)  a->time = 0;
    }
}

static void camera_close_event_cb1(lv_event_t * e)
{
	void * handle = lv_event_get_user_data(e);
	lv_event_code_t event = lv_event_get_code(e);

	if (event == LV_EVENT_SHORT_CLICKED) {
		pthread_mutex_lock(&h264enc_start_mutex);
		if (h264enc_start_flag) {
			lv_timer_del(record_timer);
			record_timer = NULL;
			record_time = 0;
			lv_label_set_text(label_timer, "00:00:00");
			lv_obj_add_flag(label_timer, LV_OBJ_FLAG_HIDDEN);

			ISS_EncodeChn_Stop(h264_enc_handle);
			h264enc_start_flag = false;
			close(finish_cb_priv.fd);
			finish_cb_priv.fd = -1;
			ISS_DestroyEncodeChn(h264_enc_handle);

			lv_obj_clear_state(tab_btns, LV_STATE_DISABLED);
			lv_obj_add_flag(lv_tabview_get_content(tabview), LV_OBJ_FLAG_SCROLLABLE);
		}
		pthread_mutex_unlock(&h264enc_start_mutex);
		deinitialize_camera_preview();
		destroy_camera_ui();
		dlclose(handle);
	}
}

static void create_camera_ui(lv_obj_t * parent, void *handle)
{
	main_page = lv_obj_create(parent);
	lv_obj_set_size(main_page, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(main_page, lv_color_hex(0x000000), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(main_page, LV_OPA_100, LV_STATE_DEFAULT);
    lv_obj_set_style_radius(main_page, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(main_page, 0, LV_STATE_DEFAULT);
    lv_obj_align_to(main_page, parent, LV_ALIGN_CENTER, 0, 0);

	close_btn = lv_imgbtn_create(main_page);
	lv_obj_set_size(close_btn, 120, 100);
	lv_obj_align_to(close_btn, main_page, LV_ALIGN_TOP_LEFT, 0, -24);
    lv_obj_add_event_cb(close_btn, camera_close_event_cb1, LV_EVENT_ALL, handle);
	lv_imgbtn_set_src(close_btn, LV_IMGBTN_STATE_RELEASED, NULL, &back_home, NULL);

	lv_style_init(&style_container_default);
	lv_style_set_radius(&style_container_default, 0);
	lv_style_set_border_width(&style_container_default, 0);
	lv_style_set_pad_all(&style_container_default, 0);
	lv_style_set_bg_opa(&style_container_default, LV_OPA_0);

	tab_container = lv_obj_create(main_page);
	lv_obj_set_size(tab_container, FUNC_TABVIEW_W, FUNC_TABVIEW_H);
    lv_obj_align(tab_container, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_style(tab_container, &style_container_default, LV_PART_MAIN);

	tabview = lv_tabview_create(tab_container, LV_DIR_TOP, TABVIEW_TAB_H);
	lv_obj_set_size(tabview, FUNC_TABVIEW_W, FUNC_TABVIEW_H);
    lv_obj_add_event_cb(lv_tabview_get_content(tabview), scroll_begin_event, LV_EVENT_SCROLL_BEGIN, NULL);
    lv_obj_set_style_bg_opa(tabview, LV_OPA_0, LV_PART_MAIN);

	tab_picture = lv_tabview_add_tab(tabview, "照相");
	tab_video = lv_tabview_add_tab(tabview, "录像");

	tab_btns = lv_tabview_get_tab_btns(tabview);
    lv_obj_set_style_text_color(tab_btns, lv_color_hex(0x404040), LV_PART_MAIN);
    lv_obj_set_style_text_color(tab_btns, lv_color_black(), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_font(tab_btns, font_info.font, LV_PART_MAIN);
    lv_obj_set_style_radius(tab_btns, TAB_BTN_RADIUS, LV_PART_MAIN);

    int state = lv_obj_get_state(tab_btns);
    lv_obj_set_style_border_width(tab_btns, 0, state);
    lv_obj_set_style_bg_opa(tab_btns, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_style_border_width(tab_btns, 0, LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(tab_btns, lv_color_hex(0xffffff), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(tab_btns, LV_OPA_30, LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_radius(tab_btns, TAB_BTN_RADIUS, LV_PART_ITEMS | LV_STATE_CHECKED);

    btn_picturing = lv_imgbtn_create(tab_picture);
    lv_obj_set_size(btn_picturing, 90, 90);
    lv_obj_align(btn_picturing, LV_ALIGN_CENTER, 0, 0);
    lv_imgbtn_set_src(btn_picturing, LV_IMGBTN_STATE_RELEASED, &take_photos, NULL, NULL);
    lv_obj_add_event_cb(btn_picturing, btn_picturing_clicked, LV_EVENT_CLICKED, NULL);

	btn_video = lv_imgbtn_create(tab_video);
    lv_obj_set_size(btn_video, 90, 90);
    lv_obj_align(btn_video, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(btn_video, LV_OBJ_FLAG_CHECKABLE);
    lv_imgbtn_set_src(btn_video, LV_IMGBTN_STATE_RELEASED, NULL, &take_videos, NULL);
    lv_imgbtn_set_src(btn_video, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &record_start, NULL);
    lv_obj_add_event_cb(btn_video, btn_video_triggered, LV_EVENT_VALUE_CHANGED , NULL);

	label_timer = lv_label_create(main_page);
	lv_obj_align(label_timer, LV_ALIGN_TOP_MID, 0, 5);
	lv_obj_set_style_text_font(label_timer, font_info.font, LV_PART_MAIN);
	lv_obj_set_style_text_color(label_timer, lv_color_hex(0xffffff), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(label_timer, LV_OPA_100, LV_PART_MAIN);
	lv_obj_set_style_bg_color(label_timer, lv_color_hex(0xff0000), LV_PART_MAIN);
	lv_obj_set_style_radius(label_timer, 5, LV_PART_MAIN);
	lv_label_set_text(label_timer, "00:00:00");

	lv_obj_add_flag(label_timer, LV_OBJ_FLAG_HIDDEN);
}

static void destroy_camera_ui(void)
{
	if (label_timer) {
        lv_obj_del(label_timer);
        label_timer = NULL;
	}

	if (btn_video) {
        lv_obj_del(btn_video);
        btn_video = NULL;
	}

	if (btn_picturing) {
        lv_obj_del(btn_picturing);
        btn_picturing = NULL;
	}

	if (tab_video) {
        lv_obj_del(tab_video);
        tab_video = NULL;
	}

	if (tab_picture) {
        lv_obj_del(tab_picture);
        tab_picture = NULL;
	}

	if (tabview) {
        lv_obj_del(tabview);
        tabview = NULL;
	}

	if (tab_container) {
        lv_obj_del(tab_container);
        tab_container = NULL;
	}

	lv_style_reset(&style_container_default);

	if (close_btn) {
        lv_obj_del(close_btn);
        close_btn = NULL;
	}

	if (main_page) {
        lv_obj_del(main_page);
        main_page = NULL;
	}
}

static void initialize_camera_preview(void)
{
	int ret = 0;
	CameraInitParam_t cam_param;
	struct ImageAttr image_attr;
	struct DisplayAttr display_attr;

	memset(&cam_param, 0, sizeof(cam_param));
	cam_param.fmt = PIX_FMT_NV12;
	cam_param.width = CAMERA_PREVIEW_WIDTH;
	cam_param.height = CAMERA_PREVIEW_HEIGHT;
	cam_param.buffer_num = 3;
	sprintf(cam_param.node, CAMERA_PREVIEW_DEV_NODE);
	preview_cam_handle = ISS_CameraInit(&cam_param);
	if(!preview_cam_handle)
		return;

	preview_osd_clt = ISS_CreateOsdClt(VIDEO);
	if (preview_osd_clt == NULL) {
		printf("[Error : %s : %d] ISS_CreateOsdClt failed!!!\n", __func__, __LINE__);
		exit(-1);
	}

	image_attr.imageFmt = PIX_FMT_NV12;
	image_attr.imageWidth = CAMERA_PREVIEW_WIDTH;
	image_attr.imageHeight = CAMERA_PREVIEW_HEIGHT;
	ret = ISS_SetImageAttr(preview_osd_clt, image_attr);
	if (ret != 0) {
		printf("[Error : %s : %d] ISS_SetImageAttr failed!!!\n", __func__, __LINE__);
		exit(-1);
	}

	display_attr.scaleWidth = OSD_CAMERA_PREVIEW_WIDTH;
	display_attr.scaleHeight = OSD_CAMERA_PREVIEW_HEIGHT;
	display_attr.posX = 0;
	display_attr.posY = OSD_CAMERA_PREVIEW_Y_OFFSET;
	display_attr.alpha = 255;
	ret = ISS_SetDisplayAttr(preview_osd_clt, display_attr);
	if (ret != 0) {
		printf("[Error : %s : %d] ISS_SetDisplayAttr failed!!!\n", __func__, __LINE__);
		exit(-1);
	}

	ret = ISS_AllocDispMem(preview_osd_clt,  CAMERA_PREVIEW_WIDTH * CAMERA_PREVIEW_HEIGHT * 3 / 2, 3);
	if (ret != 0) {
		printf("[Error : %s : %d] ISS_AllocIpcMem failed!!!\n", __func__, __LINE__);
		exit(-1);
	}

	ISS_SetFrameRate(preview_osd_clt, 60);

	pthread_create(&preview_thread_tid, NULL, preview_thread, NULL);

	return;
}

static void deinitialize_camera_preview(void)
{
	pthread_cancel(preview_thread_tid);
	pthread_join(preview_thread_tid, NULL);

	ISS_CameraDeInit(preview_cam_handle);

	ISS_FreeDispMem(preview_osd_clt);

	ISS_DestoryOsdClt(preview_osd_clt);

	return;
}




void app_camera_create(lv_obj_t * parent, void *handle)
{
    font_info.name = FONT_PATH;
    font_info.weight = 26;
    font_info.style = FT_FONT_STYLE_NORMAL;
    lv_ft_font_init(&font_info);

	create_camera_ui(parent, handle);

	initialize_camera_preview();
}
