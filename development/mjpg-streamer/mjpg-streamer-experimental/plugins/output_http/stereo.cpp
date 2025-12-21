#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <limits.h>

#include <linux/version.h>
#include <linux/types.h>          /* for videodev2.h */
#include <linux/videodev2.h>

#include <iostream>
#include <stdint.h>
#include "../../mjpg_streamer.h"
#include "../../utils.h"

#include "httpd.h"
#include "stereo.hpp"
#include <linux/videodev2.h>
#include <iostream>

#include <stereo/IMat.hpp>
#include <stereo/UtilImageIO.hpp>
#include <stereo/CalibrateIO.hpp>
#include <stereo/StereoDisparity.hpp>
#include <stereo/ImgWarp.hpp>
#include <stereo/test.hpp>
#include "stereo/undistort.hpp"

using namespace std;
using namespace JzStereo;
extern  pthread_mutex_t disparity_mutex;
extern  pthread_cond_t disparity_update;
extern char *www_folder;
IMat *disparity = NULL;
int min_disparity = 0;
int range_disparity = 0;
SgbmDisparityParam sgbmParam;
IMat ImapL0, ImapL1;
IMat ImapR0, ImapR1;
long long tmp_map, tmp_sgbm, tmp_remap;
long long time_map, time_sgbm, time_remap;
int env_isvalid = 0;

JzStereo::Size image_size;
struct camera_param {
    double cx;
    double cy;
    double f;
    double Tx;
    double sw;
};

void *disparity_thread(void* cameras)
{
    struct stereoCamera *input = (struct stereoCamera *)cameras;

    // add thread lock ..... disp8
    pthread_mutex_lock(&disparity_mutex);

    IMat right(input->right_img, input->width, input->height, IMat_8U);
    IMat left(input->left_img, input->width, input->height, IMat_8U);
    IMat L_rect, R_rect;
    tmp_remap = GetMicrosecondCount();
    JzStereo_internal::remap(left, L_rect, ImapL0, ImapL1, JzStereo_internal::INTER_LINEAR);
    JzStereo_internal::remap(right, R_rect, ImapR0, ImapR1, JzStereo_internal::INTER_LINEAR);
    time_remap = GetMicrosecondCount() - tmp_remap;

    if (disparity != NULL)
        disparity->release();

    tmp_sgbm = GetMicrosecondCount();
    disparity = StereoDisparity::densDisparity(L_rect, R_rect, sgbmParam, STEREO_SGBM);
    time_sgbm = GetMicrosecondCount() - tmp_sgbm;
    OPRINT("DISPARITY THREAD get disparity data( %p) disparity (%p)\n",
            disparity->ptr<short>(), disparity);

    right.release();
    left.release();
    L_rect.release();
    R_rect.release();

    OPRINT("remap time %lld  sgbm time %lld \n", time_remap, time_sgbm);
    // do compute, set sgbm
    // unlock ..... disp8
    pthread_cond_broadcast(&disparity_update);
    pthread_mutex_unlock(&disparity_mutex);
    return NULL;
}

#ifdef __cplusplus
extern "C"
{
#endif
    static inline void
    get_world_coordinate(struct coordinate_trans *point, struct camera_param *param)
    {
        double px = point->pixel_x;
        double py = point->pixel_y;

        point->disp = disparity->at<short>(py, px)/16 + min_disparity;
        // point->disp = disparity->at<short>(py, px)/16;
        double w = point->disp * param->Tx + param->sw;
        point->world_z = param->f / w;
        point->world_y = (py + param->cy) / w;
        point->world_x = (px + param->cx) / w;
    }

    static inline int
    region_avg_coordinate(_coordinate *corner1, _coordinate *corner2,
			  _coordinate *central, struct camera_param *param)
    {
        const int DISP_SHIFT = StereoSgbm::DISP_SHIFT;
        const int DISP_SCALE = (1 << DISP_SHIFT);
        int INVALID_DISP = min_disparity - 1, INVALID_DISP_SCALED = INVALID_DISP * DISP_SCALE;

        if (abs(corner1->pixel_y - corner2->pixel_y) > 32
                || abs(corner1->pixel_x - corner2->pixel_x) > 32) {
            OPRINT("Too large region for getting valid depth\n");

            central->world_z = -1;
            central->world_y = -1;
            central->world_x = -1;
            central->disp = INVALID_DISP;
            return -1;
        }

        int valid_count = 0;
        double w;
        double Tx = param->Tx;
        double sw = param->sw;
        double cy = param->cy;
        double cx = param->cx;
        double f  = param->f;
        double x = 0, y = 0, z = 0;
        short disp = 0;
        for (int j = corner1->pixel_y; j < corner2->pixel_y + 1; j++) {
            for (int i = corner1->pixel_x; i < corner2->pixel_x + 1; i++) {
                short  val = disparity->at<short>(j, i);
                if (val == INVALID_DISP_SCALED)
                    continue;

                valid_count++;
                val = val / 16 + min_disparity;
                // 12 is a template threshold.
                if (val < min_disparity + 12)
                    continue;

                if (valid_count > 1) {
                    double w = val * Tx + sw;
                    if (w == 0)
                        continue;

                    disp = (disp + val)/2;
                    z = (z + f / w) / 2;
                    y = (y + (j + cy) / w) / 2;
                    x = (x + (i + cx) / w) / 2;
                } else {
                    double w = val * Tx + sw;
                    if (w == 0)
                        continue;
                    disp = val;
                    z = f / w;
                    y = (j + cy) / w;
                    x = (i + cx) / w;
                }
                if (!(isnormal(z) && isnormal(y) && isnormal(z)))
                    break;
            }
	}
        DBG("%f %f %f %d %d\n", z, y, x, disp, valid_count);
        if (valid_count > 0 && isnormal(z) && isnormal(y) && isnormal(z)) {
            central->world_z = z;
            central->world_y = y;
            central->world_x = x;
            central->disp = disp;
        } else {
            central->world_z = 0;
            central->world_y = 0;
            central->world_x = 0;
            central->disp = INVALID_DISP;
        }
        return 0;
    }

    int compute_depth(_coordinate *point1, _coordinate *point2,  _coordinate *central)
    {
        DBG("DEPTH get depth (%f, %f) (%f, %f)\n",
                point1->pixel_x, point1->pixel_y,
                point2->pixel_x, point2->pixel_y);
        struct camera_param param;

        double *Q;
        get_calibrate_MQ(&Q);

        param.cx = *(Q + 1*3);
        param.cy = *(Q + 1*4 + 3);
        param.f  = *(Q + 2*4 + 3);
        param.Tx = *(Q + 3*4 + 2);
        param.sw = *(Q + 3*4 + 3);
        free(Q);
        region_avg_coordinate(point1, point2, central, &param);
        return 0;
    }

    pthread_t compute_stereo_disparity(globals *pglobal, int input_number)
    {
        int ret;
        pthread_t stereo_client;
        struct stereoCamera *cameras = (struct stereoCamera*)malloc(sizeof(struct stereoCamera));
        split_image_data(pglobal, input_number, cameras);

        if (image_size.width != cameras->width || image_size.height != cameras->height)
            OPRINT("calibrated image size (%d, %d), camera image (%d, %d), Invalid!!\n",
                    image_size.width, image_size.height, cameras->width, cameras->height);

        DBG("width is %d height %d size %d \n",
                pglobal->in[input_number]._videow,
                pglobal->in[input_number]._videoh,
                pglobal->in[input_number]._videosize);
        ret = pthread_create(&stereo_client, NULL, disparity_thread, cameras);
        return stereo_client;
override:
        free(cameras);
        return -1;
    }

    int disparity_jpeg_compress(unsigned char **outbuffer)
    {
	IMat disp8;
	int quality = 90;
        pthread_mutex_lock(&disparity_mutex);
        disparity->convertTo(disp8, IMat_8U, 255/((range_disparity + min_disparity) *16.));
        int out_size = compress_yuv_to_jpeg(outbuffer, (unsigned char*)disp8.data,
					    image_size.width, image_size.height,
					    V4L2_PIX_FMT_Y4, quality);
        pthread_mutex_unlock(&disparity_mutex);
        return out_size;
    }

    void setup_sgbm( const char* str)
    {
        cJSON *root;
        root = cJSON_Parse(str);

        sgbmParam.minDisparity = atoi(cJSON_GetObjectItem(root, "minDisparity")->valuestring);
        sgbmParam.numDisparities = atoi(cJSON_GetObjectItem(root, "numDisparities")->valuestring);
        sgbmParam.SADWindowSize = atoi(cJSON_GetObjectItem(root, "SADWindowSize")->valuestring);
        sgbmParam.preFilterCap = atoi(cJSON_GetObjectItem(root, "preFilterCap")->valuestring);
        sgbmParam.uniquenessRatio = atoi(cJSON_GetObjectItem(root, "uniquenessRatio")->valuestring);
        sgbmParam.P1 = atoi(cJSON_GetObjectItem(root, "P1")->valuestring);
        sgbmParam.P2 = atoi(cJSON_GetObjectItem(root, "P2")->valuestring);
        sgbmParam.speckleWindowSize = atoi(cJSON_GetObjectItem(root, "speckleWindowSize")->valuestring);
        sgbmParam.speckleRange = atoi(cJSON_GetObjectItem(root, "speckleRange")->valuestring);
        sgbmParam.disp12MaxDiff = atoi(cJSON_GetObjectItem(root, "disp12MaxDiff")->valuestring);
        sgbmParam.mode = atoi(cJSON_GetObjectItem(root, "mode")->valuestring);
        min_disparity = sgbmParam.minDisparity;
        range_disparity = sgbmParam.numDisparities;

        OPRINT("----------------------------------------\n")
        OPRINT("sgbmParam.minDisparity = %d \n", sgbmParam.minDisparity);
        OPRINT("sgbmParam.numDisparities = %d \n", sgbmParam.numDisparities);
        OPRINT("sgbmParam.SADWindowSize = %d \n", sgbmParam.SADWindowSize);
        OPRINT("sgbmParam.preFilterCap = %d \n", sgbmParam.preFilterCap);
        OPRINT("sgbmParam.uniquenessRatio = %d \n", sgbmParam.uniquenessRatio);
        OPRINT("sgbmParam.P1 = %d \n", sgbmParam.P1);
        OPRINT("sgbmParam.P2 = %d \n", sgbmParam.P2);
        OPRINT("sgbmParam.speckleWindowSize = %d \n", sgbmParam.speckleWindowSize);
        OPRINT("sgbmParam.speckleRange = %d \n", sgbmParam.speckleRange);
        OPRINT("sgbmParam.disp12MaxDiff = %d \n", sgbmParam.disp12MaxDiff);
        OPRINT("sgbmParam.mode = %d \n", sgbmParam.mode);
        OPRINT("----------------------------------------\n")
    }

    void init_stereo()
    {
        //TODO:
        image_size.width = 640;
        image_size.height = 480;

        // init calibrate param.
        init_stereo_calibrate();

        /* init mutex: in server thread init */
        /* init sgbm arguments  */
        char *buffer;
        string sourceparam = string(www_folder) + SGBM_JSON;
        string cusparam = string(www_folder) + SGBM_CUS_JSON;

        FILE *fp ;
        fp = fopen(cusparam.c_str(), "r");
        if(fp != NULL ) {
            fseek(fp, 0, SEEK_END);
            int len = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            buffer = (char*)malloc(len + 1);
            memset(buffer, 0, len + 1);
            size_t result =	fread(buffer, 1, len, fp);
            if (result > 0) {
                setup_sgbm(buffer);
            }
            fclose(fp);
        }else {
            sgbmParam.minDisparity = 0;
            sgbmParam.numDisparities = 128;
            min_disparity = sgbmParam.minDisparity;
            range_disparity = sgbmParam.numDisparities;
            sgbmParam.SADWindowSize = 5;
            sgbmParam.preFilterCap = 63;
            sgbmParam.uniquenessRatio = 10;
            sgbmParam.P1 = 600;
            sgbmParam.P2 = 2400;
            sgbmParam.speckleWindowSize = 100;
            sgbmParam.speckleRange = 32;
            sgbmParam.disp12MaxDiff = 1;
            sgbmParam.mode = 0;

            const char *str ="{\"filename\":\"sgbmParam.json\", \
                              \"minDisparity\":\"0\", \
                              \"numDisparities\":\"128\", \
                              \"SADWindowSize\":\"5\", \
                              \"preFilterCap\":\"63\", \
                              \"uniquenessRatio\":\"10\", \
                              \"P1\":\"600\", \
                              \"P2\":\"2400\", \
                              \"speckleWindowSize\":\"100\", \
                              \"speckleRange\":\"32\", \
                              \"disp12MaxDiff\":\"1\", \
                              \"mode\":\"0\" }";
            FILE *f = fopen(sourceparam.c_str(), "r");
            if(f == NULL) {
                f = fopen(sourceparam.c_str(), "w");
                if(f != NULL) {
                    fwrite(str, sizeof(char), strlen(str), f);
                    fclose(f);
                }
            }
        }
    }

    int get_env_flag()
    {
        return env_isvalid;
    }
#ifdef __cplusplus
}
#endif
