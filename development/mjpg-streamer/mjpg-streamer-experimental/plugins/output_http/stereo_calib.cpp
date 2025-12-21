#include <unistd.h>

#include <fstream>
#include <stdio.h>
#include <iostream>
#include <stereo/test.hpp>
#include "../../mjpg_streamer.h"
#include "../../utils.h"
#include "httpd.h"
#include <stereo/calibrate.hpp>
#include "stereo.hpp"
#include <opencv4/opencv2/core.hpp>
#include <stereo/IMat.hpp>
#include <stereo/undistort.hpp>
using namespace cv;
using namespace std;

stereo_calibrate *stereo_calib;
pthread_mutex_t calibrate_mutex;
pthread_cond_t calibrate_update;
extern JzStereo::IMat ImapL0, ImapL1;
extern JzStereo::IMat ImapR0, ImapR1;
extern JzStereo::Size image_size;
extern  pthread_mutex_t disparity_mutex;
extern char* www_folder;
extern int env_isvalid;

void *corner_point_thread(void* cameras)
{
    struct stereoCamera *input = (struct stereoCamera *)cameras;
    pthread_mutex_lock(&calibrate_mutex);
    Mat right(Size(input->width, input->height), CV_8UC1, input->right_img);
    Mat left(Size(input->width, input->height), CV_8UC1, input->left_img);
    long long tmp_time = GetMicrosecondCount();

    bool ret  = stereo_calib->get_corners_point(left, right, 0);
    tmp_time = GetMicrosecondCount() - tmp_time;

    right.release();
    left.release();

    long error;
    if(ret == true) {
        error = 0;
    } else {
        error = -1;
    }

    DBG("get corner point time %lld \n", tmp_time);
    pthread_cond_broadcast(&calibrate_update);
    pthread_mutex_unlock(&calibrate_mutex);
    return ((void*) error);
}


#ifdef __cplusplus
extern "C"
{
#endif
    static inline void copy_yuv2gray(uint8_t *dst, uint8_t *src,
            int width, int height, int stride, int step)
    {
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++)
                dst[j] = src[j * step];
            dst = dst + width;
            src = src + stride;
        }
    }

    int split_image_data(globals *pglobal, int input_number, struct stereoCamera *cameras)
    {
        uint8_t *source = (uint8_t *)pglobal->in[input_number]._userdata;

        cameras->size = pglobal->in[input_number]._videosize;
        cameras->width = pglobal->in[input_number]._videow / 2;
        cameras->height = pglobal->in[input_number]._videoh;
        cameras->fmt = pglobal->in[input_number]._videofmt;

        int width = cameras->width;
        int height = cameras->height;
        int stride, step;
        if((cameras->left_img = (uint8_t*)malloc(width * height)) == NULL)
            goto override;
        if((cameras->right_img = (uint8_t*)malloc(width * height)) == NULL) {
            free(cameras->left_img);
            goto override;
        }

        if(cameras->fmt == V4L2_PIX_FMT_YUYV)
            step = 2;
        else if(cameras->fmt == V4L2_PIX_FMT_NV12)
            step = 1;

        stride = width * 2 * step;
        copy_yuv2gray(cameras->left_img, source, width, height, stride, step);
        copy_yuv2gray(cameras->right_img, source + width * step, width, height, stride, step);
        // save_image("/tmp/test/left.gray", width, height, width * height, cameras->left_img);
        // save_image("/tmp/test/right.gray", width, height, width * height, cameras->right_img);
        return 0;

override:
        return -1;

    }

    int calibrate_check_rectify(globals *pglobal, int input_number,
            uint8_t **frame, int *frame_size)
    {
        struct stereoCamera *cameras = (struct stereoCamera*)malloc(sizeof(struct stereoCamera));
        split_image_data(pglobal, input_number, cameras);

        pthread_mutex_lock(&calibrate_mutex);

        Mat right(Size(cameras->width, cameras->height), CV_8UC1, cameras->right_img);
        Mat left(Size(cameras->width, cameras->height), CV_8UC1, cameras->left_img);
        Mat canvas = stereo_calib->check_rectify(left, right, 0);

        pthread_mutex_unlock(&calibrate_mutex);

        if (!canvas.empty()) {
            int quality = 90;
            *frame_size = compress_yuv_to_jpeg(frame, (unsigned char*)canvas.ptr(),
                    image_size.width * 2, image_size.height,
                    V4L2_PIX_FMT_Y4, quality);
            return 0;
        } else {
            return -1;
        }
    }

    int calibrate()
    {
        int ret = 0;
        pthread_mutex_lock(&calibrate_mutex);
        // calibrate reture is true/false, trans to 0/!0
        if (stereo_calib->calibrate(1))
            ret = 0;
        else
            ret = -1;
        pthread_mutex_unlock(&calibrate_mutex);
        return ret;
    }

    int calibrate_clean()
    {
        pthread_mutex_lock(&calibrate_mutex);
        stereo_calib->clear();
        pthread_mutex_unlock(&calibrate_mutex);
        return 0;
    }

    pthread_t calibrate_get_corner_point(globals *pglobal, int input_number)
    {
        struct stereoCamera *cameras = (struct stereoCamera*)malloc(sizeof(struct stereoCamera));
        split_image_data(pglobal, input_number, cameras);

        pthread_t stereo_client;
        pthread_create(&stereo_client, NULL, corner_point_thread, cameras);

        return stereo_client;
override:
        free(cameras);
        return -1;
    }

    void init_Imap(){
        Mat cmat_left = stereo_calib->camera_left.camera_matrixl;
        Mat coef_left = stereo_calib->camera_left.dist_coeffl;
        Mat cmat_right = stereo_calib->camera_right.camera_matrixl;
        Mat coef_right = stereo_calib->camera_right.dist_coeffl;
        Mat rotate_left = stereo_calib->Rl;
        Mat n_cmat_left = stereo_calib->Pl;
        Mat rotate_right = stereo_calib->Rr;
        Mat n_cmat_right = stereo_calib->Pr;

        JzStereo::IMat ml(cmat_left.ptr(), cmat_left.cols, cmat_left.rows, IMat_64F);
        JzStereo::IMat dl(coef_left.ptr(), coef_left.cols, coef_left.rows, IMat_64F);
        JzStereo::IMat mr(cmat_right.ptr(), cmat_right.cols, cmat_right.rows, IMat_64F);
        JzStereo::IMat dr(coef_right.ptr(), coef_right.cols, coef_right.rows, IMat_64F);
        JzStereo::IMat rl(rotate_left.ptr(), rotate_left.cols, rotate_left.rows, IMat_64F);
        JzStereo::IMat pl(n_cmat_left.ptr(), n_cmat_left.cols, n_cmat_left.rows, IMat_64F);
        JzStereo::IMat rr(rotate_right.ptr(), rotate_right.cols, rotate_right.rows, IMat_64F);
        JzStereo::IMat pr(n_cmat_right.ptr(), n_cmat_right.cols, n_cmat_right.rows, IMat_64F);

        JzStereo::initUndistortRectifyMap(ml, dl, rl, pl, image_size, IMat_16SC2, ImapL0, ImapL1);
        JzStereo::initUndistortRectifyMap(mr, dr, rr, pr, image_size, IMat_16SC2, ImapR0, ImapR1);
    }

    int calibrate_apply()
    {
        pthread_mutex_lock(&calibrate_mutex);
        pthread_mutex_lock(&disparity_mutex);
        int ret;

        if (stereo_calib->valid) {
            string file_ins = string(www_folder) + INS_JSON;
            string file_ext = string(www_folder) + EXT_JSON;
            ret = stereo_calib->write_parameters(file_ins,file_ext, 1);
        }else
            ret = false;


        if (ret == false) {
            pthread_mutex_unlock(&disparity_mutex);
            pthread_mutex_unlock(&calibrate_mutex);
            return -1;
        }
        init_Imap();
        env_isvalid = 1;
        pthread_mutex_unlock(&disparity_mutex);
        pthread_mutex_unlock(&calibrate_mutex);
        return 0;
    }

    void Mat2char(char *outbuffer , Mat mat)
    {
        std::stringstream ss;
        ss << "\"[ ";
        for(size_t i = 0; i < mat.rows * mat.cols; ++i) {
            if(i != 0)
                ss << ", ";
            ss  << *((double*)mat.data + i);
        }
        ss << " ]\" ";
        std::string s = ss.str();
        memcpy(outbuffer, s.c_str(), s.length());
    }

    void get_calibrate_rms(double *lrms, double *rrms, double *stereo_rms)
    {
        pthread_mutex_lock(&calibrate_mutex);
        *lrms = stereo_calib->camera_left.get_rms();
        *rrms = stereo_calib->camera_right.get_rms();
        *stereo_rms = stereo_calib->get_rms();
        pthread_mutex_unlock(&calibrate_mutex);
    }


    void get_calibrate_MQ(double **buffer)
    {
        pthread_mutex_lock(&calibrate_mutex);
        Mat mat = stereo_calib->Q;
        *buffer = (double*)malloc(sizeof(double) * (mat.rows * mat.cols));
        memcpy(*buffer, (double*)mat.data, sizeof(double) * mat.rows * mat.cols);
        pthread_mutex_unlock(&calibrate_mutex);
    }

    void get_calibrate_Q(char *outbuffer)
    {
        pthread_mutex_lock(&calibrate_mutex);
        Mat matQ = stereo_calib->Q;
        Mat2char(outbuffer, matQ);
        pthread_mutex_unlock(&calibrate_mutex);
    }

    void get_calibrate_T(char *outbuffer)
    {
        pthread_mutex_lock(&calibrate_mutex);
        Mat matT = stereo_calib->T;
        Mat2char(outbuffer, matT);
        pthread_mutex_unlock(&calibrate_mutex);
    }

    void parse_calib_patram(const char *str, _calibparam *calibparam)
    {
        cJSON *root;
        root = cJSON_Parse(str);
        calibparam->bs_w = atof(cJSON_GetObjectItem(root, "bs_w")->valuestring);
        calibparam->bs_h = atof(cJSON_GetObjectItem(root, "bs_h")->valuestring);
        calibparam->is_w = atof(cJSON_GetObjectItem(root, "is_w")->valuestring);
        calibparam->is_h = atof(cJSON_GetObjectItem(root, "is_h")->valuestring);
        calibparam->lrc_x = atof(cJSON_GetObjectItem(root, "lrc_x")->valuestring);
        calibparam->lrc_y = atof(cJSON_GetObjectItem(root, "lrc_y")->valuestring);
        calibparam->sm = atof(cJSON_GetObjectItem(root, "sm")->valuestring);
    }


    /*****************************************************/
    /*   update camera and chessboard param for calibrate
     *   set from calib_setting.html
     *****************************************************/
    void setup_calib(const char *str)
    {
        _calibparam calibparam;
        parse_calib_patram(str,&calibparam);
        Size board_shape(calibparam.bs_w, calibparam.bs_h);
        Size image_size(calibparam.is_w, calibparam.is_h);
        Size square_measure(calibparam.sm, calibparam.sm);
        Point2f low_right_coord (calibparam.lrc_x, calibparam.lrc_y);

        pthread_mutex_lock(&calibrate_mutex);
        stereo_calib->setup(board_shape, image_size, square_measure, low_right_coord);
        pthread_mutex_unlock(&calibrate_mutex);
    }


    /*****************************************************/
    /*   ready camera and chessboard param for calibrate */
    /*****************************************************/
    int init_stereo_calibrate(){
        char *buffer;
        string sourceparam = string(www_folder) + CALIB_JSON;
        string cusparam = string(www_folder) + CALIB_CUS_JSON;
        string file_ins = string(www_folder) + INS_JSON;
        string file_ext = string(www_folder) + EXT_JSON;
        FILE *fp ;

        fp = fopen(cusparam.c_str(), "r");
        if(fp != NULL ) {
            fseek(fp, 0, SEEK_END);
            int len = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            buffer = (char*)malloc(len + 1);
            memset(buffer, 0, len + 1);
            size_t result = fread(buffer, 1, len, fp);
            if (result > 0) {
                _calibparam calibparam;
                parse_calib_patram(buffer,&calibparam);
                Size board_shape(calibparam.bs_w, calibparam.bs_h);
                Size image_size(calibparam.is_w, calibparam.is_h);
                Size square_measure(calibparam.sm, calibparam.sm);
                Point2f low_right_coord (calibparam.lrc_x, calibparam.lrc_y);
                stereo_calib = new stereo_calibrate(board_shape, image_size, square_measure, low_right_coord);
                stereo_calib->load_parameters(file_ins,file_ext,1);
            }
            fclose(fp);
        }else {
            const char *str = "{\"filename\":\"calibParam.json\", \
                               \"bs_w\":\"6\", \
                               \"bs_h\":\"4\", \
                               \"is_w\":\"640\", \
                               \"is_h\":\"480\", \
                               \"lrc_x\":\"137.5\", \
                               \"lrc_y\":\"83.2\", \
                               \"sm\":\"27.5\" }";
            FILE *f = fopen(sourceparam.c_str(), "r");
            if(f == NULL) {
                f = fopen(sourceparam.c_str(), "w");
                if(f != NULL) {
                    fwrite(str, sizeof(char), strlen(str), f);
                    fclose(f);
                }
            }
            Size board_shape(6, 4);
            Size image_size(640, 480);
            Size square_measure(27.5, 27.5);
            Point2f low_right_coord (137.5, 83.2);
            stereo_calib = new stereo_calibrate(board_shape, image_size, square_measure, low_right_coord);
        }
        if(stereo_calib->valid){
            init_Imap();
            env_isvalid = 1;
        }
    }

#ifdef __cplusplus
}
#endif
