#ifndef __STEREO_HPP__
#define __STEREO_HPP__
#include <unistd.h>
#include <stdio.h>
#include "../../mjpg_streamer.h"
#include "../../utils.h"
#include "httpd.h"
#include <stdint.h>
#include "cJSON.h"
#define save_image(filename, width, height, size, buf)			\
  do{									\
    FILE *fd = fopen(filename, "w+");					\
    if (fd == 0)							\
      fprintf(stderr, "error open file %s %s\n", filename, strerror(errno)); \
    else {								\
      fwrite(buf, size, 1, fd);						\
      fclose(fd);							\
    }									\
  }while(0)

#define out_put(filename, stride, height, esize, srcp)	\
  do{							\
    FILE *fp = fopen(filename, "wb");			\
    if (fp == NULL)					\
      {							\
	printf("** Error: open %s fail\n", filename);	\
	exit(-1);					\
      }							\
    fwrite(srcp, esize, stride* height, fp);		\
    fclose(fp);						\
  }while(0)

#define INS_JSON "ins.json"
#define EXT_JSON "ext.json"
#define SGBM_JSON "sgbmParam.json"
#define SGBM_CUS_JSON "sgbmParam_custom.json"
#define CALIB_JSON "calibParam.json"
#define CALIB_CUS_JSON "calibParam_custom.json"

struct coordinate_trans {
    double pixel_x;
    double pixel_y;
    double world_x;
    double world_y;
    double world_z;
    int disp;
  };
typedef struct coordinate_trans _coordinate;
struct stereoCamera {
    uint8_t *left_img;
    uint8_t *right_img;
    int width;
    int height;
    int fmt;
    int size;
};

struct calib_param {
    double bs_w;
    double bs_h;
    double is_w;
    double is_h;
    double lrc_x;
    double lrc_y;
    double sm;
};
typedef struct calib_param _calibparam;

#ifdef __cplusplus
extern "C" {
#endif

    int compute_depth(_coordinate *point1, _coordinate *point2, _coordinate *central);
    void init_stereo();
    int init_stereo_calibrate();
    int calibrate_clean();
    int calibrate_apply();
    pthread_t compute_stereo_disparity(globals *pglobal, int input_number);
    pthread_t calibrate_get_corner_point(globals *pglobal, int input_number);
    int split_image_data(globals *, int input_number, struct stereoCamera *);

    int disparity_jpeg_compress(unsigned char** buffer);
    void setup_sgbm( const char* str);
    void setup_calib(const char *str);
    int calibrate();
    void get_calibrate_rms(double *lrms, double *rrms, double *stereorms);
    void get_calibrate_Q(char *p);
    void get_calibrate_T(char *p);
    void get_calibrate_MQ(double **p);
    int get_env_flag();

    int calibrate_check_rectify(globals *pglobal, int input_number,
                                uint8_t **frame, int *frame_size);

    int compress_yuv_to_jpeg(unsigned char **outbuffer, unsigned char *inbuffer,
			     int width, int height, int fmt, int quality);
    int calibrate_isvalid();

#ifdef __cplusplus
}
#endif
#endif
