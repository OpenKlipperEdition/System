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
#include "isp-tuning.h"


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

int init_camera_device(int fd)
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
	stream_fmt.fmt.pix.width = 1920;
	stream_fmt.fmt.pix.height = 1080;
	stream_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_SBGGR10;
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

int start_capturing(int fd,int gain,int exposure)

{
	unsigned int i;
	enum v4l2_buf_type type;
	struct v4l2_control ctrl;

	for(i = 0;i < n_buffer;i ++)
	{
		struct v4l2_buffer buf;

		bzero(&buf,sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		if(-1 == ioctl(fd,VIDIOC_QBUF,&buf))
		{
			perror("Fail to ioctl 'VIDIOC_QBUF'");
			exit(EXIT_FAILURE);
		}
		printf("VIDIOC_QBUF DONE\n");
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if(-1 == ioctl(fd,VIDIOC_STREAMON,&type))
	{
		printf("i = %d.\n",i);
		perror("Fail to ioctl 'VIDIOC_STREAMON'");
		exit(EXIT_FAILURE);
	}

	/*gain*/
	ctrl.id = V4L2_CID_ANALOGUE_GAIN;
	ctrl.value = gain;
	if(-1 == ioctl(fd, VIDIOC_S_CTRL, &ctrl)){
		perror("Failed to set gain\n");
	}
	if(-1 == ioctl(fd, VIDIOC_G_CTRL, &ctrl)){
		perror("Failed to get gain\n");
	}

	/*exposure*/
	ctrl.id = V4L2_CID_EXPOSURE;
	ctrl.value = exposure;
	if(-1 == ioctl(fd, VIDIOC_S_CTRL, &ctrl)){
		perror("Failed to set exposure\n");
	}
	if(-1 == ioctl(fd, VIDIOC_G_CTRL, &ctrl)){
		perror("Failed to get exposure\n");
	}

	return 0;
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


int mainloop(int fd)

{
	int count = 3;
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

void usage(void)
{
	printf("\
		-i [video_id]\n \
		-a [again control]\n \
		-e [exposure control]\n");
}

int main(int argc, char *argv[])
{
	int fd = -1;
	int ch = 0;

	if(argc < 2){
		usage();
		return -1;
	}

	int video_id = 3;
	int gain=0;
	int exposure=0;

	while ((ch = getopt(argc, argv, "i:a:e:")) != -1)
	{
		switch (ch) 
		{
			case 'i':
				video_id = atoi(optarg);
				break;
			case 'a':
				gain = atoi(optarg);
				break;
			case 'e':
				exposure = atoi(optarg);
				break;
			case '?':
				printf("Unknown option: %c\n",(char)optopt);
				break;
		}
	}

	fd = open_camera_device(video_id);

	init_camera_device(fd);
	start_capturing(fd,gain, exposure);
	mainloop(fd);
	stop_capturing(fd);
	uninit_camer_device(fd);
	close_camer_device(fd);

	return 0;
}
