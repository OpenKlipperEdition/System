#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <assert.h>
#include <linux/videodev2.h>
#include <linux/v4l2-controls.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <getopt.h>
#include <ctype.h>

#include "isp-tuning.h"

char options[opt_last];

typedef struct
{
	void *start;
	int length;
}BUFTYPE;
BUFTYPE *user_buf = NULL;
int n_buffer = 0;

int open_camera_device(int video_id)
{
	int fd;
	char device_name[64] = {0};
	snprintf(device_name, sizeof(device_name), "/dev/video%d", video_id);
	fd = open(device_name, O_RDWR);
	if(fd < 0)
		perror("open");
	return fd;
}


int init_mmap(int fd)
{
	int i = 0;
	int ret = 0;
	struct v4l2_requestbuffers reqbuf;

	bzero(&reqbuf,sizeof(reqbuf));
	reqbuf.count = 3;
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;

	ret = ioctl(fd, VIDIOC_REQBUFS, &reqbuf);
	if(ret < 0)
		perror("ioctl2");
	n_buffer = reqbuf.count;
	printf("n_buffer = %d\n",n_buffer);

	user_buf = calloc(reqbuf.count,sizeof(*user_buf));
	if(user_buf == NULL){
		fprintf(stderr,"Out of memory\n");
		return -1;
	}
	printf("user_buf = %x\n", user_buf);

	for(i = 0; i < reqbuf.count; i ++)
	{
		struct v4l2_buffer buf;

		bzero(&buf,sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		if(-1 == ioctl(fd,VIDIOC_QUERYBUF,&buf))
		{
			perror("Fail to ioctl : VIDIOC_QUERYBUF");
			exit(EXIT_FAILURE);
		}
		user_buf[i].length = buf.length;
		user_buf[i].start =
			mmap(NULL,
			 buf.length,
			 PROT_READ | PROT_WRITE,
			 MAP_SHARED,
			 fd,buf.m.offset
			);
		if(MAP_FAILED == user_buf[i].start)
		{
			perror("Fail to mmap");
			exit(EXIT_FAILURE);
		}
	}

	return 0;
}

int init_camera_device(int fd, struct isp_tuning_para_t *isp_tuning_para)
{
	struct v4l2_fmtdesc fmt;
	struct v4l2_capability cap;
	struct v4l2_format stream_fmt;
	int ret;

	/*video fromat suppoted by device*/
	memset(&fmt,0,sizeof(fmt));
	fmt.index = 0;
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	while((ret = ioctl(fd,VIDIOC_ENUM_FMT,&fmt)) == 0)
	{
		fmt.index ++ ;
		printf("{pixelformat = %c%c%c%c},description = '%s'\n",
				fmt.pixelformat & 0xff,(fmt.pixelformat >> 8)&0xff,
				(fmt.pixelformat >> 16) & 0xff,(fmt.pixelformat >> 24)&0xff,
				fmt.description);
	}
	ret = ioctl(fd,VIDIOC_QUERYCAP,&cap);
	if(ret < 0){
		perror("FAIL to ioctl VIDIOC_QUERYCAP");
		exit(EXIT_FAILURE);
	}
	if(!(cap.capabilities & V4L2_BUF_TYPE_VIDEO_CAPTURE))
	{
		printf("The Current device is not a video capture device\n");
		exit(EXIT_FAILURE);
	}
	if(!(cap.capabilities & V4L2_CAP_STREAMING))
	{
		printf("The Current device does not support streaming i/o\n");
		exit(EXIT_FAILURE);
	}



	stream_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	stream_fmt.fmt.pix.width = isp_tuning_para->width;
	stream_fmt.fmt.pix.height = isp_tuning_para->height;
	stream_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
	stream_fmt.fmt.pix.field = V4L2_FIELD_ANY;

	printf("VIDIOC_S_FMT = %x\n", VIDIOC_S_FMT);
	if(-1 == ioctl(fd,VIDIOC_S_FMT,&stream_fmt))
	{
		perror("VIDIOC_S_FMT Fail to ioctl");
		exit(EXIT_FAILURE);
	}

	init_mmap(fd);

	return 0;
}


int isp_tuning_hflip(int fd, struct isp_tuning_para_t *isp_tuning_para)
{
	int ret;

	ret =isp_tuning_set_hflip(fd,isp_tuning_para->hflip);
	if(ret ==-1){
		perror("Failed to set hflip\n");
		return -1;
	}

	printf("%s %d\n",__func__,__LINE__);
	ret = isp_tuning_get_hflip(fd);
	if(ret ==-1){
		perror("Failed to get hflip\n");
		return -2;
	}

	printf("hflip: %d\n", ret);

	return 0;
}

int isp_tuning_vflip(int fd, struct isp_tuning_para_t *isp_tuning_para)
{
	int ret;

	ret =isp_tuning_set_vflip(fd,isp_tuning_para->vflip);
	if(ret ==-1){
		perror("Failed to set vflip\n");
		return -1;
	}

	ret =isp_tuning_get_vflip(fd);
	if(ret == -1){
		perror("Failed to get vflip\n");
		return -2;
	}

	printf("vflip: %d\n",ret);

	return 0;
}

int isp_tuning_sharpness(int fd, struct isp_tuning_para_t *isp_tuning_para)
{
	int ret;

	ret= isp_tuning_set_sharpness(fd,isp_tuning_para->sharpness);
	if(ret ==-1){
		perror("Failed to set sharpness\n");
		return -1;
	}

	ret = isp_tuning_get_sharpness(fd);
	if(ret ==-1){
		perror("Failed to get sharpness\n");
		return -2;
	}

	printf("sharpness: %d\n", ret);

	return 0;
}

int isp_tuning_contrast(int fd, struct isp_tuning_para_t *isp_tuning_para)
{
	int ret ;
	ret =isp_tuning_set_contrast(fd,isp_tuning_para->contrast);
	if(ret ==-1){
		perror("Failed to set contrast\n");
		return -1;
	}

	ret =isp_tuning_get_contrast(fd);
	if(ret==-1){
		perror("Failed to get contrast\n");
		return -2;
	}

	printf("contrast: %d\n",ret);

	return 0;
}

int isp_tuning_saturation(int fd, struct isp_tuning_para_t *isp_tuning_para)
{
	int ret;

	ret =isp_tuning_set_saturation(fd,isp_tuning_para->saturation);
	if(ret ==-1){
		perror("Failed to set saturation\n");
		return -1;
	}

	ret =isp_tuning_get_saturation(fd);
	if(ret ==-1){
		perror("Failed to get saturation\n");
		return -2;
	}

	printf("saturation: %d\n", ret);

	return 0;
}

int isp_tuning_brightness(int fd, struct isp_tuning_para_t *isp_tuning_para)
{
	int ret;
	ret =isp_tuning_set_brightness(fd,isp_tuning_para->brightness);
	if(ret ==-1){
		perror("Failed to set brightness\n");
		return -1;
	}

	ret =isp_tuning_get_brightness(fd);
	if(ret ==-1){
		perror("Failed to get brightness\n");
		return -2;
	}

	printf("brightness: %d\n", ret);

	return 0;
}

int isp_tuning_frequency(int fd, struct isp_tuning_para_t *isp_tuning_para)
{
	int ret;

	ret =isp_tuning_set_frequency(fd,isp_tuning_para->frequency);
	if(ret ==-1){
		perror("Failed to set frequency\n");
		return -1;
	}

	ret =isp_tuning_get_frequency(fd);
	if(ret ==-1){
		perror("Failed to get frequency\n");
		return -2;
	}

	if(ret == 1)
		printf("power line frequency= 50HZ\n");
	else if(ret == 2)
		printf("power line frequency= 60HZ\n");
	else
		printf("power line frequency disabled\n");

	return 0;
}

int isp_tuning_modules(int fd, struct isp_tuning_para_t *isp_tuning_para)
{
	int ret;
	ret =isp_tuning_set_modules(fd,isp_tuning_para->modules);
	if(ret	==-1){
		perror("Failed to set modules\n");
		return -1;
	}

	usleep(50000);

	ret = isp_tuning_get_modules(fd);
	if(ret ==-1){
		perror("Failed to get modules\n");
		return -2;
	}

	printf("modules control: 0x%08x\n", ret);

	return 0;
}

int isp_tuning_dn(int fd, struct isp_tuning_para_t *isp_tuning_para)
{
	int ret;

	ret =isp_tuning_set_dn(fd, isp_tuning_para->dn);
	if(ret ==-1){
		perror("Failed to set day or night\n");
		return -1;
	}

	ret = isp_tuning_get_dn(fd);
	if (ret ==-1){
		perror("Failed to get day or night\n");
		return -2;
	}

	printf("day or night: %d\n", ret);

	return 0;
}

int isp_tuning_luma(int fd, struct isp_tuning_para_t *isp_tuning_para)
{
	int ret ;

	ret =isp_tuning_get_luma(fd);
	if(ret ==-1){
		perror("Failed to get luma\n");
		return ret;
	}

	printf("luma: %d\n",ret);

	return 0;
}


int isp_tuning_wb_red(int fd, struct isp_tuning_para_t *isp_tuning_para)
{
	int ret;

	ret =isp_tuning_set_wb_red(fd,isp_tuning_para->red_value);
	if(ret ==-1){
		perror("Fail to set wb red");
		return ret;
	}

	return ret;
}


int isp_tuning_wb_blue(int fd, struct isp_tuning_para_t *isp_tuning_para)
{

	int ret;

	ret =isp_tuning_set_wb_blue(fd,isp_tuning_para->blue_value);
	if(ret ==-1){
		perror("Fail to set wb blue");
		return ret;
	}

	return 0;
}


int isp_tuning_preset_wb(int fd, struct isp_tuning_para_t *isp_tuning_para)
{
	int ret ;

	ret =isp_tuning_set_preset_wb(fd,isp_tuning_para->preset_wb, isp_tuning_para->red_value,isp_tuning_para->blue_value);
	if(ret ==-1)
		return ret;

	usleep(5000);

	ret =isp_tuning_get_preset_wb(fd);
	if(ret ==-1){
		perror("Failed to get wb\n");
		return -2;
	}
	isp_tuning_para->preset_wb =ret;

	ret =isp_tuning_get_wb_red(fd);
	if(ret ==-1){
		perror("Failed to get wb red_value\n");
		return -3;
	}
	isp_tuning_para->red_value =ret;

	ret =isp_tuning_get_wb_blue(fd);
	if(ret ==-1){
		perror("Failed to get wb blue_value\n");
		return -4;
	}
	isp_tuning_para->blue_value=ret;

	printf("N_PRESET_WHITE_BALANCE: %d mode, red_value=%d, blue_value=%d \n", isp_tuning_para->preset_wb, isp_tuning_para->red_value, isp_tuning_para->blue_value);

	return 0;
}

int isp_tuning_auto_wb(int fd, struct isp_tuning_para_t *isp_tuning_para)
{
	int ret;

	ret =isp_tuning_set_auto_wb(fd,isp_tuning_para->auto_wb,isp_tuning_para->red_value,isp_tuning_para->blue_value);
	if(ret ==-1)
		return ret ;

	usleep(5000);

	ret =isp_tuning_get_auto_wb(fd);
	if(ret ==-1){
		perror("Failed to get wb\n");
		return -2;
	}
	isp_tuning_para->auto_wb =ret;

	ret =isp_tuning_get_wb_red(fd);
	if(ret ==-1){
		perror("Failed to get wb red_value\n");
		return -3;
	}
	isp_tuning_para->red_value =ret;

	ret =isp_tuning_get_wb_blue(fd);
	if(ret ==-1){
		perror("Failed to get wb blue_value\n");
		return -4;
	}
	isp_tuning_para->blue_value=ret;

	printf("WHITE_BALANCE: %d mode, red_value=%d, blue_value=%d \n", isp_tuning_para->auto_wb, isp_tuning_para->red_value, isp_tuning_para->blue_value);

	return 0;
}

int isp_tuning_exp(int fd, struct isp_tuning_para_t *isp_tuning_para)
{
	int ret;

	ret =isp_tuning_set_exp(fd,isp_tuning_para->exp);
	if(ret ==-1){
		perror("Failed to set exposure auto.\n");
		return -1;
	}
	ret = isp_tuning_get_exp(fd);
	if(ret ==-1){
		perror("Failed to get exposure auto.\n");
		return -1;
	}

	printf("now exposure: %d\n", ret);

	return 0;
}

int isp_tuning_gain(int fd, struct isp_tuning_para_t *isp_tuning_para)
{
	int ret;

	ret =isp_tuning_set_gain(fd,isp_tuning_para->gain);
	if(ret ==-1){
		perror("Failed to set gain.\n");
		return -1;
	}

	usleep(50000);

	ret =isp_tuning_get_gain(fd);
	if(ret==-1){
		perror("Failed to get gain.\n");
		return -2;
	}

	printf("again value (1024=1x,2048=2x,4096=4x,8192=8x): %d\n", ret);

	return 0;
}


int isp_tuning_hilightdepress(int fd, struct isp_tuning_para_t *isp_tuning_para)
{
	int ret ;

	ret= isp_tuning_set_hilightdepress(fd,isp_tuning_para->hilightdepress);
	if(ret ==-1){
		perror("Failed to set hilightdepress ");
		return -1;
	}

	ret =isp_tuning_get_hilightdepress(fd);
	if(ret ==-1){
		perror("Failed to get hilightdepress ");
		return -2;
	}

	printf("hilightdepress strength: %d\n", ret);

	return 0;
}

extern void usage(void);
static int roi_ae_get_param(char *optarg,tisp_area_t *attr)
{
	char *value;
	char *subs = optarg;
	int opt = 0;

	while (*subs != '\0') {
		static char *const subopts[] = {
			"enable",
			"top",
			"bottom",
			"left",
			"right",
			NULL
		};

		opt = getsubopt(&subs, subopts, &value);
		if (value == NULL) {
			fprintf(stderr, "No value given to suboption <%s>\n",
					subopts[opt]);
			return -1;
		}
		switch (opt) {
			case 0:
				attr->enable = atoi(value);
				break;
			case 1:
				attr->top = atoi(value);
				break;
			case 2:
				attr->bottom = atoi(value);
				break;
			case 3:
				attr->left = atoi(value);
				break;
			case 4:
				attr->right = atoi(value);
				break;
			default:
				usage();
		}
	}
	return 0;
}


int isp_tuning_roi_ae(int fd, struct isp_tuning_para_t *isp_tuning_para)
{
	int ret;
	isp_tuning_set_roi_ae(fd,isp_tuning_para->roi_ae_attr.enable,isp_tuning_para->roi_ae_attr.top,isp_tuning_para->roi_ae_attr.bottom,         \
								isp_tuning_para->roi_ae_attr.left,isp_tuning_para->roi_ae_attr.right);
	if(ret  ==-1){
		perror("Failed to set roi ae\n");
		return ret;
	}

	printf("roi_ae enable=%d, top=%d, bottom=%d, left=%d, right=%d\n", isp_tuning_para->roi_ae_attr.enable, isp_tuning_para->roi_ae_attr.top, isp_tuning_para->roi_ae_attr.bottom, \
										isp_tuning_para->roi_ae_attr.left, isp_tuning_para->roi_ae_attr.right);
	return 0;
}

int start_capturing(int fd, struct isp_tuning_para_t *isp_tuning_para)
{
	unsigned int i;
	unsigned int ret = 0;
	enum v4l2_buf_type type;
	struct v4l2_control ctrl;

	for(i = 0;i < n_buffer;i ++)
	{
		struct v4l2_buffer buf;

		bzero(&buf,sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		if(-1 == ioctl(fd, VIDIOC_QBUF, &buf))
		{
			perror("Fail to ioctl 'VIDIOC_QBUF'");
			exit(EXIT_FAILURE);
		}
		printf("VIDIOC_QBUF DONE\n");
	}


	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(-1 == ioctl(fd, VIDIOC_STREAMON, &type))
	{
		printf("i = %d.\n",i);
		perror("Fail to ioctl 'VIDIOC_STREAMON'");
		exit(EXIT_FAILURE);
	}
	printf("VIDIOC_STREAMON DONE\n");


	/*hflip*/
	if(options[opt_hflip])
		ret = isp_tuning_hflip(fd, isp_tuning_para);

	/*vflip*/
	if(options[opt_vflip])
		ret = isp_tuning_vflip(fd, isp_tuning_para);

	/*sharpness*/
	if(options[opt_sharpness])
		ret = isp_tuning_sharpness(fd, isp_tuning_para);

	/*contraste*/
	if(options[opt_contrast])
		ret = isp_tuning_contrast(fd, isp_tuning_para);

	/*saturation*/
	if(options[opt_saturation])
		ret = isp_tuning_saturation(fd, isp_tuning_para);

	/*brightness*/
	if(options[opt_brightness])
		ret = isp_tuning_brightness(fd, isp_tuning_para);

	/*power line frequency*/
	if(options[opt_antiflicker])
		ret = isp_tuning_frequency(fd, isp_tuning_para);

	/*modules control*/
	if(options[opt_module])
		ret = isp_tuning_modules(fd, isp_tuning_para);

	/*day night mode*/
	if(options[opt_day_or_night])
		ret = isp_tuning_dn(fd, isp_tuning_para);

	/*luma*/
	if(options[opt_luma])
		ret = isp_tuning_luma(fd, isp_tuning_para);

	/*exposure, 0 auto other value is set value*/
	if(options[opt_exposure])
		ret = isp_tuning_exp(fd, isp_tuning_para);

	/*gain, 0 auto other value is set value*/
	if(options[opt_gain])
		ret = isp_tuning_gain(fd, isp_tuning_para);
	/* white blance  red*/
	if(options[opt_red])
		        ret =isp_tuning_wb_red(fd,isp_tuning_para);

	/* white blance  blue*/
	if(options[opt_blue])
		        ret =isp_tuning_wb_blue(fd,isp_tuning_para);

	/* preset white blance, 1 auto other value is set value*/
	if(options[opt_preset_wb])
		ret = isp_tuning_preset_wb(fd, isp_tuning_para);

	/*auto/manual white blance, 1 auto other value is set value*/
	if(options[opt_auto_wb])
		ret = isp_tuning_auto_wb(fd, isp_tuning_para);

	/*hilightdepress,  0 auto other value is set value*/
	if(options[opt_hilightdepress])
		ret = isp_tuning_hilightdepress(fd, isp_tuning_para);

	/*roi_ae */
	if(options[opt_roi_ae])
		ret = isp_tuning_roi_ae(fd, isp_tuning_para);

	return ret;
}

int process_image(void *addr,int length)
{
	FILE *fp;
	static int num = 0;
	char picture_name[20];

	sprintf(picture_name,"/tmp/picture%d.yuv",num++);

	if((fp = fopen(picture_name,"w")) == NULL)
	{
		perror("Fail to fopen");
		exit(EXIT_FAILURE);
	}

	fwrite(addr,length,1,fp);
	usleep(500);

	fclose(fp);

	return 0;
}

int read_frame(int fd)
{
	struct v4l2_buffer buf;
	unsigned int i;

	bzero(&buf,sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	if(-1 == ioctl(fd,VIDIOC_DQBUF,&buf))
	{
		perror("Fail to ioctl 'VIDIOC_DQBUF'");
		exit(EXIT_FAILURE);
	}
	assert(buf.index < n_buffer);
	process_image(user_buf[buf.index].start,user_buf[buf.index].length);
	if(-1 == ioctl(fd,VIDIOC_QBUF,&buf))
	{
		perror("Fail to ioctl 'VIDIOC_QBUF'");
		exit(EXIT_FAILURE);
	}
	return 1;
}

void stop_capturing(int fd)
{
	enum v4l2_buf_type type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(-1 == ioctl(fd,VIDIOC_STREAMOFF,&type))
	{
		perror("Fail to ioctl 'VIDIOC_STREAMOFF'");
		exit(EXIT_FAILURE);
	}
	return;
}


int mainloop(int fd, int count)
{
	struct v4l2_control ctrl;
	int i = 0;

	while(count--)
	{
		for(;;)
		{
			fd_set fds;
			struct timeval tv;
			int r;

			FD_ZERO(&fds);
			FD_SET(fd,&fds);

			/*Timeout*/
			tv.tv_sec = 3;
			tv.tv_usec = 0;

			r = select(fd + 1,&fds,NULL,NULL,&tv);

			if(-1 == r)
			{
				if(EINTR == errno)
					continue;

				perror("Fail to select");
				exit(EXIT_FAILURE);
			}

			if(0 == r)
			{
				fprintf(stderr,"select Timeout\n");
				stop_capturing(fd);
				exit(EXIT_FAILURE);
			}

			if(read_frame(fd))
				break;
		}
	}
	return 0;
}


void uninit_camer_device()
{
	unsigned int i;

	for(i = 0;i < n_buffer;i ++)
	{
		if(-1 == munmap(user_buf[i].start,user_buf[i].length))
		{
			exit(EXIT_FAILURE);
		}
	}

	free(user_buf);

	return;
}

void close_camer_device(int fd)
{
	if(-1 == close(fd))
	{
		perror("Fail to close fd");
		exit(EXIT_FAILURE);
	}

	return;
}

static struct option long_options[] = {
         {"width", required_argument, 0, opt_width},
         {"height", required_argument, 0, opt_height},
         {"video-id", required_argument, 0, opt_video_id},
         {"count", required_argument, 0, opt_count},
         {"hflip", required_argument, 0, opt_hflip},
         {"vflip", required_argument, 0, opt_vflip},
         {"sharpness", required_argument, 0, opt_sharpness},
         {"contrast", required_argument, 0, opt_contrast},
         {"saturation", required_argument, 0, opt_saturation},
         {"brightness", required_argument, 0, opt_brightness},
         {"exposure", required_argument, 0, opt_exposure},
         {"gain", required_argument, 0, opt_gain},
         {"antiflicker", required_argument, 0, opt_antiflicker},
         {"module", required_argument, 0, opt_module},
         {"day-or-night", required_argument, 0, opt_day_or_night},
         {"luma", no_argument, 0, opt_luma},
	 {"roi_ae", required_argument, 0, opt_roi_ae},
         {"preset-wb", required_argument, 0, opt_preset_wb},
         {"auto-wb", required_argument, 0, opt_auto_wb},
         {"red", required_argument, 0, opt_red},
         {"blue", required_argument, 0, opt_blue},
         {"hilightdepress", required_argument, 0, opt_hilightdepress},
         {"help", no_argument, 0, opt_help},
         {0,0,0,0},
};


void usage(void)
{
	printf("-%c --width [image width]\n", opt_width);
	printf("-%c --height [image height]\n", opt_height);
	printf("-%c --video-id [/dev/videox]\n", opt_video_id);
	printf("-%c --count [out put frame_count]\n", opt_count);
	printf("-%c --hflip [0:disable 1:enable]\n", opt_hflip);
	printf("-%c --vflip [0:disable 1:enable]\n", opt_vflip);
	printf("-%c --sharpness range[0-255]\n", opt_sharpness);
	printf("-%c --contrast range[0-255]\n", opt_contrast);
	printf("-%c --saturation range[0-255]\n", opt_saturation);
	printf("-%c --brightness range[0-255]\n", opt_brightness);
	printf("-%c --exposure [0:auto, others manual exposure]\n", opt_exposure);
	printf("-%c --gain [0:auto, others manual gain]\n", opt_gain);
	printf("-%c --antiflicker [0:disable 1:50Hz 2:60Hz]\n", opt_antiflicker);
	printf("-%c --module isp module bypass control\n", opt_module);
	printf("-%c --day-or-night [0:day_mode 1:night_mode]\n", opt_day_or_night);
	printf("-%c --luma [get image luma]\n", opt_luma);
	printf("--roi_ae enable=<enable>,top=<top>,bottom=<bottom>,left=<left>,right=<right>\n");
	printf("--preset-wb ten preset wb mode\n\t[0:manual 1:auto 2:incandescent 3:fluorescent 4:fluorescent_h 5:horizon 6:daylight 8:cloudy 9:shade 10:custom]\n\twhen preset_wb = 0 or 10, red and blue chroma balance should be set\n");
	printf("--auto-wb wb mode\n\t[0:manual 1:auto]\n\twhen auto_wb = 0, red and blue chroma balance should be set\n");
	printf("-%c --red red chroma balance\n", opt_red);
	printf("-%c --blue blue chroma balance\n", opt_blue);
	printf("-%c --hilightdepress\n", opt_hilightdepress);
}

int main(int argc, char *argv[])
{
	int fd = -1;
	int ch = 0;
	int frame_count = 3;
	char short_options[26 * 2 * 3 + 1];
	int idx = 0;
	int option_index = 0;
	int i = 0;

	if(argc < 2){
		usage();
		return -1;
	}

	for (i = 0; long_options[i].name; i++) {
		if (!isalpha(long_options[i].val))
			continue;
		short_options[idx++] = long_options[i].val;
		if (long_options[i].has_arg == required_argument) {
			short_options[idx++] = ':';
		} else if (long_options[i].has_arg == optional_argument) {
			short_options[idx++] = ':';
			short_options[idx++] = ':';
		}
	}


	struct isp_tuning_para_t isp_tuning_para = { //default param
		.width = 1920,
		.height = 1080,
		.video_id = 5,
	};

	while((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1)
	{
		options[(int)ch] = 1;
		switch (ch)
		{
			case opt_width:
				isp_tuning_para.width = atoi(optarg);
				break;
			case opt_height:
				isp_tuning_para.height = atoi(optarg);
				break;
			case opt_video_id:
				isp_tuning_para.video_id = atoi(optarg);
				break;
			case opt_count:
				frame_count = atoi(optarg);
				break;
			case opt_hflip:
				isp_tuning_para.hflip = atoi(optarg);
				break;
			case opt_vflip:
				isp_tuning_para.vflip = atoi(optarg);
				break;
			case opt_sharpness:
				isp_tuning_para.sharpness = atoi(optarg);
				break;
			case opt_contrast:
				isp_tuning_para.contrast = atoi(optarg);
				break;
			case opt_saturation:
				isp_tuning_para.saturation = atoi(optarg);
				break;
			case opt_brightness:
				isp_tuning_para.brightness = atoi(optarg);
				break;
			case opt_exposure:
				isp_tuning_para.exp = atoi(optarg);
				break;
			case opt_gain:
				isp_tuning_para.gain = atoi(optarg);
				break;
			case opt_antiflicker:
				isp_tuning_para.frequency = atoi(optarg);
				break;
			case opt_module:
				isp_tuning_para.modules = strtol(optarg, NULL, 16);
				break;
			case opt_day_or_night:
				isp_tuning_para.dn = atoi(optarg);
				break;
			case opt_auto_wb:
				isp_tuning_para.auto_wb = atoi(optarg);
				break;
			case opt_preset_wb:
				isp_tuning_para.preset_wb = atoi(optarg);
				break;
			case opt_red:
				isp_tuning_para.red_value = atoi(optarg);
				break;
			case opt_blue:
				isp_tuning_para.blue_value = atoi(optarg);
				break;
			case opt_hilightdepress:
				isp_tuning_para.hilightdepress = atoi(optarg);
				break;
			case opt_roi_ae:
				roi_ae_get_param(optarg, &isp_tuning_para.roi_ae_attr);
				break;
			case opt_help:
			default:
				usage();
		}
	}

	fd = open_camera_device(isp_tuning_para.video_id);

	init_camera_device(fd, &isp_tuning_para);
	start_capturing(fd, &isp_tuning_para);
	mainloop(fd, frame_count);
	stop_capturing(fd);
	uninit_camer_device(fd);
	close_camer_device(fd);

	return 0;
}
