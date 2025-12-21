/**
 * @file fbdev.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "fbdev.h"
#include "order_link.h"
#if USE_FBDEV || USE_BSD_FBDEV

#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <assert.h>

#if USE_BSD_FBDEV
#include <sys/fcntl.h>
#include <sys/time.h>
#include <sys/consio.h>
#include <sys/fbio.h>
#else  /* USE_BSD_FBDEV */
#include <linux/fb.h>
#endif /* USE_BSD_FBDEV */

/*********************
 *      DEFINES
 *********************/
#define FB_DEVICE_FILENAME getenv("FB_DEVICE")

#ifndef FBDEV_PATH
#define FBDEV_PATH  "/dev/fb0"
#endif

#define DIRTY_LIST_NUM 2
#define MATH_MAX(a, b) ((a) > (b) ? (a) : (b))
#define MATH_MIN(a, b) ((a) < (b) ? (a) : (b))

/**********************
 *      TYPEDEFS
 **********************/
enum {
  FB_TAG_SPARE = 0,
  FB_TAG_READY,
  FB_TAG_BUSY
};


/**********************
 *      STRUCTURES
 **********************/

struct bsd_fb_var_info{
    uint32_t xoffset;
    uint32_t yoffset;
    uint32_t xres;
    uint32_t yres;
    int bits_per_pixel;
 };

struct bsd_fb_fix_info{
    long int line_length;
    long int smem_len;
};


/**********************
 *  STATIC PROTOTYPES
 **********************/
static void area_join(lv_area_t* rect1, lv_area_t* rect2);
static void my_sem_wait(sem_t *sem);
static void* fbswap_thread(void* ctx);
static void init_fblist(int num);
static int get_spare_fb();
static int get_busy_fb();
static int get_ready_fb();

/**********************
 *  STATIC VARIABLES
 **********************/
#if USE_BSD_FBDEV
static struct bsd_fb_var_info vinfo;
static struct bsd_fb_fix_info finfo;
#else
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;
#endif /* USE_BSD_FBDEV */
static char *fbp = 0;
static long int screensize = 0;
static int fbfd = 0;
static int fb_nr;
static Link* s_fblist[3];
static lv_area_t s_rect_list[DIRTY_LIST_NUM]={0};
static uint8_t *s_buf;

static pthread_t s_t_fbswap;
static sem_t s_sem_spare;
static sem_t s_sem_ready;
static pthread_mutex_t s_lck_fblist;
static bool s_app_quited = false;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void fbdev_init(void)
{
    // Open the file for reading and writing
    char* fb_device = FB_DEVICE_FILENAME;
    if(fb_device == NULL){
        fb_device = FBDEV_PATH;
        printf("{%s}[%s](%d) : use default '%s'\n", __FILE__, __func__, __LINE__, fb_device);
    }
    fbfd = open(fb_device, O_RDWR);
    if(fbfd == -1) {
        perror("Error: cannot open framebuffer device");
        return;
    }
    printf("The framebuffer device was opened successfully.\n");


    // Get fixed screen information
    if(ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
        perror("Error reading fixed information");
        return;
    }

    // Get variable screen information
    if(ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
        perror("Error reading variable information");
        return;
    }

    LV_LOG_INFO("%dx%d, %dbpp", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

    // Figure out the size of the screen in bytes
    screensize =  finfo.smem_len; //finfo.line_length * vinfo.yres;
    fb_nr = vinfo.yres_virtual / vinfo.yres ;
    printf("fb numbur : %d\n",fb_nr);
    init_fblist(fb_nr);

    // Map the device to memory
    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if((intptr_t)fbp == -1) {
        perror("Error: failed to map framebuffer device to memory");
        return;
    }
    vinfo.yoffset = 0;
    ioctl(fbfd, FBIOPAN_DISPLAY, &vinfo);

    // Don't initialise the memory to retain what's currently displayed / avoid clearing the screen.
    // This is important for applications that only draw to a subsection of the full framebuffer.
#ifndef NOUSING_SWAP
    //s_buf = (uint8_t*)malloc(vinfo.yres * finfo.line_length);
    pthread_mutex_init (&s_lck_fblist, NULL);
    sem_init(&s_sem_spare, 0, fb_nr);
    sem_init(&s_sem_ready, 0, 0);
    pthread_create(&s_t_fbswap,NULL,(void *)fbswap_thread,NULL);
#endif
    LV_LOG_INFO("The framebuffer device was mapped to memory successfully");

}

void fbdev_exit(void)
{
    s_app_quited = true;
#ifndef NOUSING_SWAP
    sem_post(&s_sem_spare);
    sem_post(&s_sem_ready);
    usleep(200*1000);

    void* ret = NULL;
    pthread_join(s_t_fbswap,&ret);

    sem_destroy(&s_sem_spare);

    sem_destroy(&s_sem_ready);
    Link_free(s_fblist[0]);
    Link_free(s_fblist[1]);
    Link_free(s_fblist[2]);
    //free(s_buf);
    pthread_mutex_destroy(&s_lck_fblist);
#endif
    close(fbfd);
}

lv_color_t* fbdev_get_buff(int i){
    uint8_t* buff = NULL;
    if(i < fb_nr ){
        uint32_t size = vinfo.yres * finfo.line_length;
        buff = fbp + size * i;
    }
    return (lv_color_t*)buff;
}

void fbdev_copy_buff(void* buff, const lv_area_t * area, lv_color_t * color_p, uint8_t mode){
    /*Truncate the area to the screen*/
    int32_t act_x1 = area->x1 < 0 ? 0 : area->x1;
    int32_t act_y1 = area->y1 < 0 ? 0 : area->y1;
    int32_t act_x2 = area->x2 > (int32_t)vinfo.xres - 1 ? (int32_t)vinfo.xres - 1 : area->x2;
    int32_t act_y2 = area->y2 > (int32_t)vinfo.yres - 1 ? (int32_t)vinfo.yres - 1 : area->y2;

    lv_coord_t w = (act_x2 - act_x1 + 1);
    lv_coord_t h = (act_y2 - act_y1 + 1);
    long int location = 0;
    long int byte_location = 0;
    unsigned char bit_location = 0;

    /*32 or 24 bit per pixel*/
    if(vinfo.bits_per_pixel == 32 || vinfo.bits_per_pixel == 24) {
        uint32_t * fbp32 = (uint32_t *)buff;
        int32_t y;
        if(act_x1 == 0 && act_y1 == 0 && w == vinfo.xres && h == vinfo.yres){
            uint32_t size = vinfo.yres * finfo.line_length;
            memcpy(fbp32, color_p, size);
        } else {
            if(mode == 0){
                for(y = act_y1; y <= act_y2; y++) {
                    location = (act_x1) + (y) * finfo.line_length / 4;
                    memcpy(&fbp32[location], (uint32_t *)color_p, (act_x2 - act_x1 + 1) * 4);
                    color_p += w;
                }
            } else {
                uint32_t *src_p = (uint32_t*)(color_p) + (act_x1 + act_y1 * finfo.line_length / 4);
                for(y = act_y1; y <= act_y2; y++) {
                    location = (act_x1) + (y) * finfo.line_length / 4;
                    memcpy(&fbp32[location], src_p, (act_x2 - act_x1 + 1) * 4);
                    src_p += finfo.line_length / 4;
                }
            }
        }
    }
}

/**
 * Flush a buffer to the marked area
 * @param drv pointer to driver where this function belongs
 * @param area an area where to copy `color_p`
 * @param color_p an array of pixels to copy to the `area` part of the screen
 */
void new_fbdev_flush(lv_disp_drv_t * drv, const lv_area_t * get_area, lv_color_t * color_p)
{
    if(fbp == NULL ||
            get_area->x2 < 0 ||
            get_area->y2 < 0 ||
            get_area->x1 > (int32_t)vinfo.xres - 1 ||
            get_area->y1 > (int32_t)vinfo.yres - 1) {
        lv_disp_flush_ready(drv);
        return;
    }

    uint8_t* buff = NULL;
    lv_color_t *buf0 = fbdev_get_buff(0);
    lv_color_t *buf1 = fbdev_get_buff(1);
    uint8_t id = 0;
    if(color_p == buf0){
        id = 0;
    } else {
        id = 1;
    }
    vinfo.yoffset = id * vinfo.yres;
    ioctl(fbfd, FBIOPAN_DISPLAY, &vinfo);

    //int dummy = 0;
    //ioctl(fbfd, FBIO_WAITFORVSYNC, &dummy);
    lv_disp_t* disp = _lv_refr_get_disp_refreshing();
    lv_area_t* area = NULL;
    for(int i = 0; i < disp->inv_p;i++){
        if(disp->inv_area_joined[i] != 1){
            area = &disp->inv_areas[i];
            if(color_p == buf0){
                fbdev_copy_buff(buf1,area,buf0,1);
            } else {
                fbdev_copy_buff(buf0,area,buf1,1);
            }
        }
    }
    lv_disp_flush_ready(drv);
}
void fbdev_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_p)
{
    if (s_app_quited) {
        return;
    }
    uint8_t* buff = NULL;
/*
    lv_disp_t* disp = _lv_refr_get_disp_refreshing();
    disp->inv_areas[LV_INV_BUF_SIZE];
    disp->inv_area_joined[LV_INV_BUF_SIZE];
    disp->inv_p;
    lv_area_t area0 = disp->inv_areas[0];
    lv_area_t t_area = {area0.x1,area0.y1,area0.x2,area0.y2};
    lv_area_t* area = &t_area;
    lv_area_t* a_area;
    for(int i = 0; i < disp->inv_p;i++){
        if(disp->inv_area_joined[i] != 1){
            area_join(area,&disp->inv_areas[i]);
 //           a_area=&disp->inv_areas[i];
//            printf("\033[31m[%s]%s:%d %d %d %d %d\033[0m\n",__FILE__,__func__,__LINE__,a_area->x1,a_area->y1,a_area->x2,a_area->y2);
        }
    }
    */
    if(fbp == NULL ||
            area->x2 < 0 ||
            area->y2 < 0 ||
            area->x1 > (int32_t)vinfo.xres - 1 ||
            area->y1 > (int32_t)vinfo.yres - 1) {
        lv_disp_flush_ready(drv);
        return;
    }
//printf("\033[34m[%s]%s:%d %d %d %d %d\033[0m\n",__FILE__,__func__,__LINE__,area->x1,area->y1,area->x2,area->y2);
#ifndef NOUSING_SWAP
    my_sem_wait(&s_sem_spare);
    if (s_app_quited) {
        lv_disp_flush_ready(drv);
      return;
    }

    pthread_mutex_lock(&s_lck_fblist);
    int spare_fb = get_spare_fb();
    //assert(spare_fb);
    pthread_mutex_unlock(&s_lck_fblist);
    if(spare_fb == -1){
        lv_disp_flush_ready(drv);
        return;
    }
    uint32_t size = vinfo.yres * finfo.line_length;
    buff = fbp + size * spare_fb;
    /*
    //draw
    fbdev_copy_buff(s_buf,area,color_p,0);
    */
    //merge
    lv_area_t merge_area={0,0,0,0};
    area_join(&merge_area, area);
    for(int i = 0; i < DIRTY_LIST_NUM;i++){
        area_join(&merge_area,&s_rect_list[i]);
    }
    for(int i = DIRTY_LIST_NUM-1; i > 0; i--) {
        s_rect_list[i].x1 = s_rect_list[i-1].x1;
        s_rect_list[i].y1 = s_rect_list[i-1].y1;
        s_rect_list[i].x2 = s_rect_list[i-1].x2;
        s_rect_list[i].y2 = s_rect_list[i-1].y2;
    }
    s_rect_list[0].x1 = area->x1;
    s_rect_list[0].y1 = area->y1;
    s_rect_list[0].x2 = area->x2;
    s_rect_list[0].y2 = area->y2;

    //fbdev_copy_buff(buff, &merge_area, (lv_color_t *)s_buf,1);
    fbdev_copy_buff(buff, &merge_area, color_p,1);
    //fbdev_copy_buff(buff,area,color_p,0);

    pthread_mutex_lock(&s_lck_fblist);
    pushElem(s_fblist[FB_TAG_READY],spare_fb);
    sem_post(&s_sem_ready);
    pthread_mutex_unlock(&s_lck_fblist);
#else
    static int i = 0;
    uint32_t size = vinfo.yres * finfo.line_length;
    buff = fbp + size * i;
    /*
    //merge
    lv_area_t merge_area;
    area_join(&merge_area, area);
    for(int i = 0; i < DIRTY_LIST_NUM;i++){
        area_join(&merge_area,&s_rect_list[i]);
    }
    for(int i = DIRTY_LIST_NUM-1; i > 0; i--) {
        s_rect_list[i].x1 = s_rect_list[i-1].x1;
        s_rect_list[i].y1 = s_rect_list[i-1].y1;
        s_rect_list[i].x2 = s_rect_list[i-1].x2;
        s_rect_list[i].y2 = s_rect_list[i-1].y2;
    }
    s_rect_list[0].x1 = area->x1;
    s_rect_list[0].y1 = area->y1;
    s_rect_list[0].x2 = area->x2;
    s_rect_list[0].y2 = area->y2;
    */
    //fbdev_copy_buff(buff,area,color_p,0);
    //fbdev_copy_buff(buff, &merge_area, color_p,1);
    fbdev_copy_buff(buff, area, color_p,1);
    vinfo.yoffset = i * vinfo.yres;
    ioctl(fbfd, FBIOPAN_DISPLAY, &vinfo);
    i = (i+1)%fb_nr;
    int dummy = 0;
    #ifndef DISABLE_VSYNC
    ioctl(fbfd, FBIO_WAITFORVSYNC, &dummy);
    #endif
#endif

    //May be some direct update command is required
    //ret = ioctl(state->fd, FBIO_UPDATE, (unsigned long)((uintptr_t)rect));

    lv_disp_flush_ready(drv);
}

void fbdev_get_sizes(uint32_t *width, uint32_t *height) {
    if (width)
        *width = vinfo.xres;

    if (height)
        *height = vinfo.yres;
}

void fbdev_set_offset(uint32_t xoffset, uint32_t yoffset) {
    vinfo.xoffset = xoffset;
    vinfo.yoffset = yoffset;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void* fbswap_thread(void* ctx) {

  while (!s_app_quited) {
    my_sem_wait(&s_sem_ready);
    if (s_app_quited) {
      break;
    }

    pthread_mutex_lock(&s_lck_fblist);
    int ready_fbid = get_ready_fb();
    assert(ready_fbid != -1);
    pthread_mutex_unlock(&s_lck_fblist);

    vinfo.yoffset = ready_fbid * vinfo.yres;
    ioctl(fbfd, FBIOPAN_DISPLAY, &vinfo);

    int dummy = 0;
    ioctl(fbfd, FBIO_WAITFORVSYNC, &dummy);

    pthread_mutex_lock(&s_lck_fblist);
    int busy_size = getElemSize(s_fblist[FB_TAG_BUSY]);
    if (busy_size > 1) {
        int last_busy_fb = get_busy_fb();
        if(last_busy_fb != -1){
            pushElem(s_fblist[FB_TAG_SPARE],last_busy_fb);
            sem_post(&s_sem_spare);
        }
    }
    pushElem(s_fblist[FB_TAG_BUSY],ready_fbid);
    pthread_mutex_unlock(&s_lck_fblist);
  }

  return NULL;
}
static void init_fblist(int num) {
  memset(s_fblist, 0, sizeof(s_fblist));
  for (int i = 0; i < 3; i++) {
    s_fblist[i] = initLink();
  }
  for (int i = 1; i < num; i++) {
    pushElem(s_fblist[FB_TAG_SPARE],i);
  }
  pushElem(s_fblist[FB_TAG_SPARE],0);
}
static int get_spare_fb() {
  return popElem(s_fblist[FB_TAG_SPARE]);
}
static int get_busy_fb() {
  return popElem(s_fblist[FB_TAG_BUSY]);
}
static int get_ready_fb() {
  return popElem(s_fblist[FB_TAG_READY]);
}
static void my_sem_wait(sem_t *sem){
    int retval;
    do {
        retval = sem_wait(sem);
    } while (retval < 0 && errno == EINTR);
}
static void area_join(lv_area_t* rect1, lv_area_t* rect2)
{
    rect1->x1 = MATH_MIN(rect1->x1, rect2->x1);
    rect1->y1 = MATH_MIN(rect1->y1, rect2->y1);
    rect1->x2 = MATH_MAX(rect1->x2, rect2->x2);
    rect1->y2 = MATH_MAX(rect1->y2, rect2->y2);
}

#endif
