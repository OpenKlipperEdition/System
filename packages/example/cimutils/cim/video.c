/*
 * V4L2 Spec Appendix B.
 *
 * Video Capture Example
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>	//getopt_long()
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>	//for videodev2.h
#include <linux/videodev2.h>
#include "headers.h"

#define CLEAR(x)	memset(&(x), 0, sizeof(x))
#define DBG()	do {} while(0)

struct buffer {
	void *	start;
	size_t	length;
	void *	hist_addr;
};

static char *		dev_name	= NULL;
struct buffer *		buffers		= NULL;
static unsigned int	n_buffers	= 0;

static void errno_exit(const char *s)
{
	fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
	exit(EXIT_FAILURE);
}

static int xioctl(int fd, int request, void *arg)
{
	int r;

	do {
		r = ioctl(fd, request, arg);
	} while (-1 == r && EINTR == errno);

	return r;
}

static void cal_hist(struct camera_info * camera_inf, unsigned int index, unsigned int *addr)
{
	unsigned int w,h;
	unsigned int i;
	unsigned int *hist;
	unsigned int *hist_src;
	unsigned char *tmp;

	w = camera_inf->param.width;
	h = camera_inf->param.height;

	hist = (unsigned int *)malloc(256*4);
	if(!hist) {
		errno_exit("NO MEM");
	}

	memset(hist, 0, 256*4);

	hist_src = (unsigned int *)buffers[index].hist_addr;
	switch (camera_inf->fmt.fourcc) {
	case V4L2_PIX_FMT_YUYV:
		for(i = 0; i < (w * h / 2); i++) {
			tmp = (unsigned char *)&addr[i];
			hist[tmp[0]]++;
			hist[tmp[2]]++;
		}
		for(i=0; i < 256; i++) {
			if(hist[i] != hist_src[i])
				printf("hist %d not equal sl:%d  sy:%d !\n",i, hist[i], hist_src[i]);
			else
				;
//				printf("hist %d equal sl:%d  sy:%d !\n",i, hist[i], hist_src[i]);
		}
		break;
	case V4L2_PIX_FMT_GREY:
		tmp = (unsigned char *)addr;
		for(i = 0; i < w * h; i++) {
			hist[tmp[i]]++;
		}
		for(i=0; i < 256; i++) {
			if(hist[i] != hist_src[i])
				printf("hist %d not equal sl:%d  sy:%d !\n",i, hist[i], hist_src[i]);
			else
				;
//				printf("hist %d equal sl:%d  sy:%d !\n",i, hist[i], hist_get.hist_a[i]);
		}
		break;
	default:
		printf("hist not support format!\n");
		break;
	}
	free(hist);
}

static int read_frame(struct camera_info *camera_inf, struct camera_ctl *camera_ctl)
{
	struct v4l2_buffer buf;
	unsigned int i;
	unsigned long long starttime,endtime;
	int fd = camera_inf->fd_cim;
	enum io_method io = camera_ctl->io_method;

	switch (io) {
	case IO_METHOD_READ:
		if (-1 == read(fd, buffers[0].start, buffers[0].length)) {
			switch (errno) {
			case EAGAIN:
				return 0;
			case EIO:
			default:
				errno_exit("read");
			}
		}
		break;

	case IO_METHOD_MMAP:
		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;

		if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				return 0;
			case EIO:
			default:
				errno_exit("VIDIOC_DQBUF");
			}
		}

		assert(buf.index < n_buffers);
		camera_inf->buf_vaddr = (void *)buffers[buf.index].start;
		if (camera_ctl->cam_opt == OPS_PREVIEW)
		{
			if (camera_ctl->ops.priview_picture){
				camera_ctl->ops.priview_picture(camera_inf);
			}
		} else if (camera_ctl->cam_opt == OPS_CAPTURE){
			if (camera_ctl->ops.capture_picture)
				camera_ctl->ops.capture_picture(camera_inf, camera_ctl);
		}

		if (camera_inf->param.debug == 1 && camera_ctl->cam_opt == OPS_PREVIEW)
		{
			if (camera_ctl->ops.camera_fps)
				camera_ctl->ops.camera_fps(camera_inf);
		}

		if(camera_inf->histgram) {
			cal_hist(camera_inf, buf.index, (unsigned int *)buffers[buf.index].start);
		}

		if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
			errno_exit("VIDIOC_QBUF");

		break;

	case IO_METHOD_USERPTR:
		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;

		if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				return 0;
			case EIO:
			default:
				errno_exit("VIDIOC_DQBUF");
			}
		}
	        int page_size = getpagesize();
	        int buf_size = (buf.length  + page_size - 1) & ~(page_size - 1);
		for (i=0; i<n_buffers; ++i) {
			if (buf.m.userptr == (unsigned long)buffers[i].start
			 && buf_size == buffers[i].length){
				break;
			}
		}

		camera_inf->buf_vaddr = (void *)buffers[buf.index].start;
		if (camera_ctl->cam_opt == OPS_PREVIEW)
		{
			if (camera_ctl->ops.priview_picture)
				camera_ctl->ops.priview_picture(camera_inf);
		} else if (camera_ctl->cam_opt == OPS_CAPTURE){
			if (camera_ctl->ops.capture_picture)
				camera_ctl->ops.capture_picture(camera_inf, camera_ctl);
		}

		if (camera_inf->param.debug == 1 && camera_ctl->cam_opt == OPS_PREVIEW)
		{
			if (camera_ctl->ops.camera_fps)
				camera_ctl->ops.camera_fps(camera_inf);
		}

		if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
			errno_exit("VIDIOC_QBUF");

		break;
	}

	camera_inf->squeue++;

	return 1;
}

static void mainloop(struct camera_info *camera_inf, struct camera_ctl *camera_ctl)
{
	unsigned int count;
	int fd = camera_inf->fd_cim;

	count = 1;

	while (count-- > 0) {
		for (;;) {
			fd_set fds;
			struct timeval tv;
			int r;

			FD_ZERO(&fds);
			FD_SET(fd, &fds);

			/* Timeout. */
			tv.tv_sec = 20;
			tv.tv_usec = 0;

			r = select(fd+1, &fds, NULL, NULL, &tv);

			if (-1 == r) {
				if (EINTR == errno)
					continue;

				errno_exit("select");
			}

			if (0 == r) {
				fprintf(stderr, "select timeout\n");
				exit(EXIT_FAILURE);
			}

			if (read_frame(camera_inf, camera_ctl))
				break;
		}
	}
}

void stop_capturing(struct camera_ctl *camera_ctl)
{
	enum v4l2_buf_type type;
	enum io_method io = camera_ctl->io_method;
	int fd = camera_ctl->camera_inf->fd_cim;

	switch (io) {
	case IO_METHOD_READ:
		/* nothing to do */
		break;
	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
			errno_exit("VIDIOC_STREAMOFF");

		break;
	}
}

void start_capturing(struct camera_ctl *camera_ctl)
{
	unsigned int i;
	enum v4l2_buf_type type;
	enum io_method io = camera_ctl->io_method;
	int fd = camera_ctl->camera_inf->fd_cim;

	switch(io) {
	case IO_METHOD_READ:
		/* nothing to do */
		break;
	case IO_METHOD_MMAP:
		for (i=0; i<n_buffers; ++i) {
			struct v4l2_buffer buf;

			CLEAR(buf);

			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = i;

			if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
				errno_exit("VIDIOC_QBUF");
		}

		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
			errno_exit("VIDIOC_STREAMON");

		break;

	case IO_METHOD_USERPTR:
		for (i=0; i<n_buffers; ++i) {
			struct v4l2_buffer buf;

			CLEAR(buf);

			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_USERPTR;
			buf.index = i;
			buf.m.userptr = (unsigned long)buffers[i].start;
			buf.length = buffers[i].length;

			if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
				errno_exit("VIDIOC_QBUF");
		}

		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
			errno_exit("VIDIOC_STREAMON");

		break;
	}
}


static void init_read(unsigned int buffer_size)
{
	buffers = calloc(1, sizeof(*buffers));

	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	buffers[0].length = buffer_size;
	buffers[0].start = malloc(buffer_size);

	if (!buffers[0].start) {
		fprintf(stderr, "Out of memory2\n");
		exit(EXIT_FAILURE);
	}
}

static void init_mmap(struct camera_info *camera_inf)
{
	struct v4l2_requestbuffers req;
	int fd = camera_inf->fd_cim;

	CLEAR(req);

	req.count	= 3;
	req.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory	= V4L2_MEMORY_MMAP;

	if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support memory mapping\n", dev_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}

	if (req.count < 1) {
		fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
		exit(EXIT_FAILURE);
	}

	buffers = malloc(req.count * sizeof(*buffers));

	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (n_buffers=0; n_buffers<req.count; ++n_buffers) {
		struct v4l2_buffer buf;

		CLEAR(buf);

		buf.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory	= V4L2_MEMORY_MMAP;
		buf.index	= n_buffers;

		if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
			errno_exit("VIDIOC_QUERYBUF");

		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start =
			mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

		if (MAP_FAILED == buffers[n_buffers].start)
			errno_exit("mmap");

		if(camera_inf->histgram) {
			buffers[n_buffers].hist_addr =
				buffers[n_buffers].start + camera_inf->param.frmsize;
		}
	}
}

static void init_userp(struct camera_info *camera_inf, unsigned int buffer_size)
{
	struct v4l2_requestbuffers req;
	unsigned int page_size = 0;
	int fd = camera_inf->fd_cim;

	page_size = getpagesize();
	buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);

	CLEAR(req);

	req.count	= 2;
	req.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory	= V4L2_MEMORY_USERPTR;

	if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support user pointer i/o\n", dev_name);
			exit(EXIT_FAILURE);
		} else
			errno_exit("VIDIOC_REQBUFS");
	}

	buffers = calloc(req.count, sizeof(*buffers));
	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		buffers[n_buffers].length = buffer_size;

		buffers[n_buffers].start = (unsigned char *)valloc(buffer_size);
		debug("==>%s L%d: the buffers[%d].start = %x\n", __func__, __LINE__ , n_buffers,buffers[n_buffers].start);
		if (!buffers[n_buffers].start) {
			fprintf(stderr, "Out of memory\n");
			exit(EXIT_FAILURE);
		}
	}
}

static void query_cap(struct v4l2_capability *cap)
{
	if (cap->capabilities & V4L2_CAP_VIDEO_CAPTURE)
		fprintf(stdout, "Video capture device\n");
	if (cap->capabilities & V4L2_CAP_READWRITE)
		fprintf(stdout, "Read/Write systemcalls\n");
	if (cap->capabilities & V4L2_CAP_STREAMING)
		fprintf(stdout, "Streaming I/O ioctls\n");
}

static void dump_fmt(struct v4l2_format *fmt)
{
	if (fmt->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		fprintf(stdout, "width=%d, height=%d\n", fmt->fmt.pix.width, fmt->fmt.pix.height);
		fprintf(stdout, "pixelformat=%s, field=%d\n", (char *)&fmt->fmt.pix.pixelformat, fmt->fmt.pix.field);
		fprintf(stdout, "bytesperline=%d, sizeimage=%d\n", fmt->fmt.pix.bytesperline, fmt->fmt.pix.sizeimage);
		fprintf(stdout, "colorspace=%d\n", fmt->fmt.pix.colorspace);
	}
}

int open_device_video_id(struct camera_info *camera_inf)
{
        struct v4l2_capability cap;
	char dev_name[20] = {'0'};
	int ret, i;
	int fd;
	int fd_helix;

	camera_inf->fd_cim = -1;

	strcpy(dev_name,camera_inf->video);
	fd = open(dev_name, O_RDWR);
	if (fd > 0) {
		ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
		if(ret < 0) {
			printf("Query capability failed!\n");
		}
		printf("%s QUERYCAP: %s\n", dev_name, (void *)cap.driver);

		if(camera_inf->encodec_mode==1){
			fd_helix = open("/dev/video1", O_RDWR);
			camera_inf->fd_helix = fd_helix;
		}

		camera_inf->fd_cim = fd;


	} else {
		printf("open video device failed: %s\n", dev_name);
		return -1;
	}
	return 0;
}


unsigned int set_format(struct camera_info *camera_inf)
{
	struct v4l2_format fmt;
	unsigned int min;
	int idx = 0;
	int fd = camera_inf->fd_cim;

	CLEAR(fmt);

	fmt.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(camera_inf->do_crop) {
		fmt.fmt.pix.width	= camera_inf->width;
		fmt.fmt.pix.height	= camera_inf->height;
	} else {
		fmt.fmt.pix.width	= camera_inf->param.width;
		fmt.fmt.pix.height	= camera_inf->param.height;
	}
	fmt.fmt.pix.pixelformat	= camera_inf->fmt.fourcc;
	fmt.fmt.pix.field	= V4L2_FIELD_ANY;/*V4L2_FIELD_NONE;*/
//	fmt.fmt.pix.priv	= camera_inf->fmt.fmt_priv;

	if (-1 == ioctl(fd, VIDIOC_S_INPUT, &idx)) {
		fprintf(stderr, " VIDIOC_S_INPUT error\n");
		return -1;
	}
	if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt)){
		printf("==>%s L%d: set error  !!!!!!!!!!!!! \n",__func__, __LINE__);
		errno_exit("VIDIOC_S_FMT");
	}
	if (-1 ==  xioctl(fd, VIDIOC_G_FMT, &fmt)){
		fprintf(stderr, " VIDIOC_G_FMT error\n");
		return -1;
	}
//	if((camera_inf->param.width != fmt.fmt.pix.width) || (camera_inf->param.height != fmt.fmt.pix.height)){
//		camera_inf->param.width = fmt.fmt.pix.width;
//		camera_inf->param.height = fmt.fmt.pix.height;
//	}

	min = fmt.fmt.pix.width;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;

	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

	debug("==>%s L%d: sizeimage=0x%x fmt.fmt.pix.width = 0x%x fmt.fmt.pix.height = 0x%x\n", __func__, __LINE__, fmt.fmt.pix.sizeimage,fmt.fmt.pix.width, fmt.fmt.pix.height);

	return fmt.fmt.pix.sizeimage;
}

static int set_histgram_pram(struct camera_info *camera_inf)
{
	struct v4l2_control v4l2_ctl;
	int ret;
	int fd = camera_inf->fd_cim;

	/* format check */
	if((camera_inf->fmt.fourcc != V4L2_PIX_FMT_YUYV) &&
			(camera_inf->fmt.fourcc != V4L2_PIX_FMT_GREY)) {
		printf("histgram not support format!\n");
		return -EINVAL;
	}

	v4l2_ctl.id = JZ_CID_HIST_EN;
	ret = ioctl(fd, VIDIOC_S_CTRL, &v4l2_ctl);
	if(ret < 0) {
		printf("hist en failed!\n");
		return ret;
	}

	if(camera_inf->hist_mul != -1) {
		v4l2_ctl.value = camera_inf->hist_mul;
	} else {
		v4l2_ctl.value = 0x20;
	}
	v4l2_ctl.id = JZ_CID_HIST_GAIN_MUL;
	ret = ioctl(fd, VIDIOC_S_CTRL, &v4l2_ctl);
	if(ret < 0) {
		printf("set hist mul failed!\n");
		return ret;
	}

	if(camera_inf->hist_add != -1) {
		v4l2_ctl.value = camera_inf->hist_add;
	} else {
		v4l2_ctl.value = 0;
	}
	v4l2_ctl.id = JZ_CID_HIST_GAIN_ADD;
	ret = ioctl(fd, VIDIOC_S_CTRL, &v4l2_ctl);
	if(ret < 0) {
		printf("set hist add failed!\n");
		return ret;
	}

	return 0;
}

static int set_crop(struct camera_info *camera_inf)
{
	struct v4l2_cropcap cropcap;
	struct v4l2_control v4l2_ctl;
	struct v4l2_frmsizeenum fsize;
	struct v4l2_crop crop;
	struct v4l2_rect *rect;
	int fd = camera_inf->fd_cim;

	CLEAR(cropcap);
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
		if(((camera_inf->crop_x + camera_inf->crop_w) > cropcap.bounds.width) ||
			((camera_inf->crop_y + camera_inf->crop_h) > cropcap.bounds.height)) {
			fprintf(stderr, "check crop bounds failed\n");
			exit(EXIT_FAILURE);
		}

		printf("Support the following crop sources:\n");
		fsize.index = 0;
		while(!xioctl(fd, VIDIOC_ENUM_FRAMESIZES, &fsize)) {
			printf("  %d: width = %d height = %d\n",
				fsize.index, fsize.discrete.width, fsize.discrete.height);
			fsize.index++;
		}

		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		rect = &crop.c;
		rect->left = camera_inf->crop_x;
		rect->top = camera_inf->crop_y;
		rect->width = camera_inf->crop_w;
		rect->height = camera_inf->crop_h;

		v4l2_ctl.id = JZ_CID_CROP_WAY;
		v4l2_ctl.value = camera_inf->crop_way;
		if(-1 == xioctl(fd, VIDIOC_S_CTRL, &v4l2_ctl)) {
			fprintf(stderr, "set crop way failed\n");
			exit(EXIT_FAILURE);
		}

		if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
			fprintf(stderr, "set crop failed\n");
			exit(EXIT_FAILURE);
		}
	} else {
		fprintf(stderr, "get cropcap failed\n");
		exit(EXIT_FAILURE);
	}
}

void init_device(struct camera_info *camera_inf, struct camera_ctl *camera_ctl)
{
	struct v4l2_capability cap;
	enum io_method io = camera_ctl->io_method;
	unsigned int frmsize;
	int use_tlb = camera_inf->param.tlb;
	int fd = camera_inf->fd_cim;
	char *io_method_str[3] = {"IO_METHOD_READ", "IO_METHOD_MMAP", "IO_METHOD_USERPTR"};

	debug("==>%s L%d: io method: %s\n", __func__, __LINE__, io_method_str[io]);

	if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s is not V4L2 device\n", dev_name);
			exit(EXIT_FAILURE);
		} else
			errno_exit("VIDIOC_QUERYCAP");
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf(stderr, "%s is not capture device\n", dev_name);
		exit(EXIT_FAILURE);
	}

	switch (io) {
	case IO_METHOD_READ:
		if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
			fprintf(stderr, "%s doesn't support read i/o\n", dev_name);
			exit(EXIT_FAILURE);
		}
		break;
	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
			fprintf(stderr, "%s doesn't support streaming i/o\n", dev_name);
			exit(EXIT_FAILURE);
		}
		break;
	}

	/* select video input, video standard and tune here. */

	frmsize = set_format(camera_inf);
	camera_inf->param.frmsize = frmsize;

	if(camera_inf->do_crop && set_crop(camera_inf))
		exit(EXIT_FAILURE);

	/*set histgram after request buffer*/
	if(camera_inf->histgram) {
		if(set_histgram_pram(camera_inf)) {
			exit(EXIT_FAILURE);
		}
	}

	switch (io) {
	case IO_METHOD_READ:
		init_read(frmsize);
		break;
	case IO_METHOD_MMAP:
		init_mmap(camera_inf);
		break;
	case IO_METHOD_USERPTR:
		init_userp(camera_inf, frmsize);
		break;
	}
}

void uninit_device(struct camera_ctl *camera_ctl, struct camera_info *camera_inf)
{
	unsigned int i;
	int use_tlb = camera_inf->param.tlb;
	enum io_method io = camera_ctl->io_method;

	switch(io) {
	case IO_METHOD_READ:
		free(buffers[0].start);
		break;
	case IO_METHOD_MMAP:
		for (i=0; i<n_buffers; ++i)
			if (-1 == munmap(buffers[i].start, buffers[i].length))
				errno_exit("munmap");
		break;
	case IO_METHOD_USERPTR:
		if (use_tlb)
			for (i=0; i<n_buffers; ++i)
				free(buffers[i].start);
		break;
	}

	if(camera_inf->histgram) {
		free(camera_inf->hist_addr);
	}
	free(buffers);
}

int process_framebuf(struct camera_info *camera_inf, struct camera_ctl *camera_ctl)
{

	mainloop(camera_inf, camera_ctl);

	return 0;
}

