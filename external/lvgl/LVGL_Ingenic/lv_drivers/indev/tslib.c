/**
 * @file tslib.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "tslib_drv.h"
#if USE_TSLIB

#include "tslib.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <linux/limits.h>
#include <dirent.h>


/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static char *find_ts_dev(void);
static void *ts_event_handler(void *arg);

/**********************
 *  STATIC VARIABLES
 **********************/
static pthread_mutex_t read_mutex;
static struct tsdev* ts;
static char* filename;
typedef struct _event_t {
    int x;
    int y;
    int button;
} event_t;
#define EL_MAX 8
static event_t event_list[EL_MAX]={0};
static int el_w = 0, el_r = 0;
static bool tslib_pressed;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Initialize the tslib interface
 */
void tslib_init(void)
{
    if (!tslib_set_file(LIBINPUT_NAME)) {
        return;
    }

}
/**
 * reconfigure the device file for tslib
 * @param dev_name set the tslib device filename
 * @return true: the device file set complete
 *         false: the device file doesn't exist current system
 */
bool tslib_set_file(char* dev_name)
{
    filename = find_ts_dev();
    if(filename == NULL){
        filename = dev_name;
    }
    ts = ts_open(filename, 0);
    if(ts != NULL){
        ts_config(ts);
    } else {
        return false;
    }
    memset(event_list,0,sizeof(event_list));
    tslib_pressed = false;
    //ts_close(info.ts);
	pthread_mutex_init(&read_mutex,NULL);
    pthread_t tid;
    pthread_create(&tid,NULL,ts_event_handler, NULL);

    return true;
}
/**
 * Get the current position and state of the tslib
 * @param data store the tslib data here
 */
void tslib_read(lv_indev_drv_t * drv, lv_indev_data_t * data)
{
    static event_t save = {0,0,0};
    if(drv->type != LV_INDEV_TYPE_POINTER)
        return ;
    if(el_w > 0){
        pthread_mutex_lock(&read_mutex);
        data->point.x = event_list[el_r].x;
        data->point.y = event_list[el_r].y;
        data->state = event_list[el_r].button;
        el_r ++;
        if(el_r < el_w) {
            data->continue_reading = true;
        } else {
            data->continue_reading = false;
            //如果el_r 有值, 判断此为最后一个事件,保存到save;
            if(event_list[el_r].x || event_list[el_r].y ||  event_list[el_r].button){
                el_r++;
            }
            memcpy(&save,&event_list[el_r-1],sizeof(event_t));
            memset(event_list,0,sizeof(event_list));
            el_w = el_r = 0;
        }
        pthread_mutex_unlock(&read_mutex);
    }else{
        data->point.x = save.x;
        data->point.y = save.y;
        data->state = save.button;
    }

    if(data->point.x < 0)
        data->point.x = 0;
    if(data->point.y < 0)
        data->point.y = 0;
    if(data->point.x >= drv->disp->driver->hor_res)
        data->point.x = drv->disp->driver->hor_res - 1;
    if(data->point.y >= drv->disp->driver->ver_res)
        data->point.y = drv->disp->driver->ver_res - 1;
    return ;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static char* find_ts_dev(void){
    char* status_path = "/sys/class/input/%s/device/name";
    static char* path = NULL;
    DIR *dir;
    struct dirent *ent;
    if(path != NULL){
        free(path);
    }
    if (!(dir = opendir("/dev/input"))) {
        perror("unable to open directory /dev/input");
        return NULL;
    }

    while ((ent = readdir(dir))) {
        if (strncmp(ent->d_name, "event", 5) != 0) {
            continue;
        }

        path = malloc((11 + strlen(ent->d_name)) * sizeof(char));
        if (!path) {
            perror("could not allocate memory for device node path");
            return NULL;
        }
        strcpy(path, "/dev/input/");
        strcat(path, ent->d_name);
        char * device_path = malloc((strlen(status_path) + strlen(ent->d_name)) * sizeof(char));
        sprintf(device_path,status_path,ent->d_name);
        printf("try read %s \n",device_path);
        FILE* file = fopen(device_path, "r");
        if (file == NULL) {
            printf("无法打开设备节点\n");
            free(device_path);
            free(path);
            path = NULL;
            continue;
        }

        char content[20];  // 假设设备节点内容不超过100个字符
        fgets(content, sizeof(content), file);
        printf("%s", content);  // 输出设备节点的内容
        free(device_path);
        fclose(file);
        if (strncmp(content, "goodix-ts", 9) != 0) {
            free(path);
            path = NULL;
            continue;
        }else{
            return path;
        }
    }
    return path;
}
static void *ts_event_handler(void *arg)
{
    pthread_detach(pthread_self());
    struct ts_sample in = {0};
    int ret = -1;
    bool button_down = false;

    while(1){
    if (ts != NULL) {
        ret = ts_read(ts, &in, 1);
    } else {
        break;
    }

    if (ret == 0) {
        continue;
    } else if (ret < 0) {
        usleep(5000);

        filename = find_ts_dev();
        if (access(filename, R_OK) == 0) {
            if (ts != NULL) {
                ts_close(ts);
            }
            ts = ts_open(filename, 1);

            if (ts == NULL) {
                perror("print tslib: ");
            } else {
                ts_config(ts);
            }
        }
        continue;
    }

    if (in.pressure > 0) {
        if (tslib_pressed) {
            button_down = LV_INDEV_STATE_PR;//MOVE
        } else {
            button_down = LV_INDEV_STATE_PR;//DOWN
            tslib_pressed = true;
        }
    } else {
        if (tslib_pressed) {
            button_down = LV_INDEV_STATE_REL;
        }
        tslib_pressed = false;
        button_down = LV_INDEV_STATE_REL;
    }
    pthread_mutex_lock(&read_mutex);
    if(el_w == 0 || (el_w > 0 &&(event_list[el_w-1].x != in.x
                || event_list[el_w-1].y != in.y || event_list[el_w-1].button != button_down))){
        event_list[el_w].x = in.x;
        event_list[el_w].y = in.y;
        event_list[el_w].button = button_down;
        if(++el_w > EL_MAX-1 ){
            el_w = EL_MAX-1;
        }
    }
    pthread_mutex_unlock(&read_mutex);
    }
}

#endif
