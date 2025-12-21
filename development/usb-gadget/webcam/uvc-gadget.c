/*
 * UVC gadget test application
 *
 * Copyright (C) 2010 Ideas on board SPRL <laurent.pinchart@ideasonboard.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 */

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/usb/ch9.h>
#include <linux/usb/video.h>
#include <linux/videodev2.h>

#include <uvc.h>
#include <v4l2.h>
#include <v4l2-jpegenc.h>

/* Enable debug prints. */
#undef ENABLE_BUFFER_DEBUG
#undef ENABLE_USB_REQUEST_DEBUG

#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define max(a, b) (((a) > (b)) ? (a) : (b))

#define clamp(val, min, max)                                                                                           \
	({                                                                                                                 \
	 typeof(val) __val = (val);                                                                                     \
	 typeof(min) __min = (min);                                                                                     \
	 typeof(max) __max = (max);                                                                                     \
	 (void)(&__val == &__min);                                                                                      \
	 (void)(&__val == &__max);                                                                                      \
	 __val = __val < __min ? __min : __val;                                                                         \
	 __val > __max ? __max : __val;                                                                                 \
	 })

#define ARRAY_SIZE(a) ((sizeof(a) / sizeof(a[0])))
#define pixfmtstr(x) (x) & 0xff, ((x) >> 8) & 0xff, ((x) >> 16) & 0xff, ((x) >> 24) & 0xff

/*
 * The UVC webcam gadget kernel driver (g_webcam.ko) supports changing
 * the Brightness attribute of the Processing Unit (PU). by default. If
 * the underlying video capture device supports changing the Brightness
 * attribute of the image being acquired (like the Virtual Video, VIVI
 * driver), then we should route this UVC request to the respective
 * video capture device.
 *
 * Incase, there is no actual video capture device associated with the
 * UVC gadget and we wish to use this application as the final
 * destination of the UVC specific requests then we should return
 * pre-cooked (static) responses to GET_CUR(BRIGHTNESS) and
 * SET_CUR(BRIGHTNESS) commands to keep command verifier test tools like
 * UVC class specific test suite of USBCV, happy.
 *
 * Note that the values taken below are in sync with the VIVI driver and
 * must be changed for your specific video capture device. These values
 * also work well in case there in no actual video capture device.
 */
#define PU_BRIGHTNESS_MIN_VAL 0
#define PU_BRIGHTNESS_MAX_VAL 255
#define PU_BRIGHTNESS_STEP_SIZE 1
#define PU_BRIGHTNESS_DEFAULT_VAL 127

/* ---------------------------------------------------------------------------
 * Generic stuff
 */


/* Buffer representing one video frame */
struct buffer {
	struct v4l2_buffer buf;
	void *start;
	size_t length;
};


/* ---------------------------------------------------------------------------
 * UVC specific stuff
 */

struct uvc_frame_info {
	unsigned int width;
	unsigned int height;
	unsigned int intervals[8];
};

struct uvc_format_info {
	unsigned int fcc;
	const struct uvc_frame_info *frames;
};

static const struct uvc_frame_info uvc_frames_yuyv[] = {
	{
		640,
		360,
		{333333, 666666},
	},
	{
		640,
		480,
		{333333, 666666},
	},
	{
		1280,
		720,
		{333333, 666666},
	},
	{
		1920,
		1080,
		{710000, 2000000},
	},
	{
		0,
		0,
		{0,},
	},
};

static const struct uvc_frame_info uvc_frames_mjpeg[] = {
	{
		640,
		360,
		{333333, 666666},
	},
	{
		640,
		480,
		{333333, 666666},
	},
	{
		1280,
		720,
		{333333, 666666},
	},
	{
		1920,
		1080,
		{710000, 2000000},
	},
	{
		0,
		0,
		{0,},
	},
};

static const struct uvc_format_info uvc_formats[] = {
	{V4L2_PIX_FMT_YUYV, uvc_frames_yuyv},
	{V4L2_PIX_FMT_MJPEG, uvc_frames_mjpeg},
};

/* ---------------------------------------------------------------------------
 * V4L2 and UVC device instances
 */

/* Represents a V4L2 based video capture device */
struct v4l2_device {
	/* v4l2 device specific */
	int v4l2_fd;
	int is_streaming;
	int is_busy;
	char *v4l2_devname;

	/* v4l2 buffer specific */
	enum io_method io;
	struct buffer *mem;
	unsigned int nbufs;
	unsigned int buf_index;

	/* v4l2 buffer queue and dequeue counters */
	unsigned long long int qbuf_count;
	unsigned long long int dqbuf_count;

	/* uvc device hook */
	struct uvc_device *udev;

	/* helix device hook */
	struct helix_device *edev;
};

/* Represents a UVC based video output device */
struct uvc_device {
	/* uvc device specific */
	int uvc_fd;
	int is_streaming;
	int run_standalone;
	char *uvc_devname;

	/* uvc control request specific */

	struct uvc_streaming_control probe;
	struct uvc_streaming_control commit;
	int control;
	struct uvc_request_data request_error_code;
	unsigned int brightness_val;

	/* uvc buffer specific */
	enum io_method io;
	struct buffer *mem;
	struct buffer *dummy_buf;
	unsigned int nbufs;
	unsigned int fcc;
	unsigned int width;
	unsigned int height;
	unsigned int buf_index;

	unsigned int bulk;
	uint8_t color;
	unsigned int imgsize;
	void *imgdata;

	/* USB speed specific */
	int mult;
	int burst;
	int maxpkt;
	enum usb_device_speed speed;

	/* uvc specific flags */
	int first_buffer_queued;
	int uvc_shutdown_requested;
	int uvc_close_requested;

	/* uvc buffer queue and dequeue counters */
	unsigned long long int qbuf_count;
	unsigned long long int dqbuf_count;

	/* v4l2 device hook */
	struct v4l2_device *vdev;

	/* helix device hook */
	struct helix_device *edev;
};

/* Represents a HELIX based video capture and output device */
struct helix_device {
	/* helix device specific */
	int helix_fd;
	char *helix_devname;
	int is_busy;
};


/* forward declarations */
static int uvc_video_stream(struct uvc_device *dev, int enable);
static int image_load(struct uvc_device *dev, const char *img);

/*Global parameters*/
static char *mjpeg_image = NULL;
static int dummy_data_gen_mode = 0;
static struct camera_info camera_inf;

/*convert yuv 420 planar (uv interleaved) (nv12) to yuv 422
 * args:
 *      dst: pointer to frame buffer (yuyv)
 *      dstStride: stride of framebuffer
 *      src: pointer to temp buffer containing yuv420 (nv12) planar data frame
 *      width: picture width
 *      height: picture height
 */
void nv12_to_yuyv (uint8_t *dst,int dstStride, uint8_t *src, int width, int height)
{
	uint8_t *py;
	uint8_t *puv;

	int linesize = width * 2;
	int offsety=0;
	int offsetuv=0;
	int dw = dstStride - (width << 1);

	py=src;
	puv=py+(width*height);

	int h=0;
	int w=0;

	int wy=0;
	int wuv=0;

	for(h=0;h<height;h+=2)
	{
		wy=0;
		wuv=0;

		for(w=0;w<linesize;w+=4)
		{
			/*y00*/
			*dst++ = py[wy + offsety];
			/*y10*/
			dst[dstStride-1] = py[wy + offsety + width];

			/*u0*/
			uint8_t u0 = puv[wuv + offsetuv];
			*dst++ = u0;
			/*u0*/
			dst[dstStride-1] = u0;

			/*y01*/
			*dst++ = py[(wy + 1) + offsety];
			/*y11*/
			dst[dstStride-1] = py[(wy + 1) + offsety + width];

			/*v0*/
			uint8_t v0 = puv[(wuv + 1) + offsetuv];
			*dst++ = v0;
			/*v0*/
			dst[dstStride-1] = v0;

			wuv+=2;
			wy+=2;
		}

		dst += dstStride + dw;
		offsety += width * 2;
		offsetuv +=  width;
	}
}

void grey_to_yuyv (uint8_t *dst,int dstStride, uint8_t *src, int width, int height)
{
	uint8_t *py;
	uint8_t *puv;

	int linesize = width * 2;
	int offsety=0;
	int offsetuv=0;
	int dw = dstStride - (width << 1);

	py=src;
	puv=py+(width*height);

	int h=0;
	int w=0;

	int wy=0;
	int wuv=0;

	for(h=0;h<height;h+=2)
	{
		wy=0;
		wuv=0;

		for(w=0;w<linesize;w+=4)
		{
			/*y00*/
			*dst++ = py[wy + offsety];
			/*y10*/
			dst[dstStride-1] = py[wy + offsety + width];

			/*u0*/
			uint8_t u0 = puv[wuv + offsetuv];
			u0 = 0x80;
			*dst++ = u0;
			/*u0*/
			dst[dstStride-1] = u0;

			/*y01*/
			*dst++ = py[(wy + 1) + offsety];
			/*y11*/
			dst[dstStride-1] = py[(wy + 1) + offsety + width];

			/*v0*/
			uint8_t v0 = puv[(wuv + 1) + offsetuv];
			v0 = 0x80;
			*dst++ = v0;
			/*v0*/
			dst[dstStride-1] = v0;

			wuv+=2;
			wy+=2;
		}

		dst += dstStride + dw;
		offsety += width * 2;
		offsetuv +=  width;
	}
}

/*
 * jpeg encodec init.
 */
int encodec_init(struct uvc_device *dev)
{
	struct helix_device *edev;

	edev= dev->edev;
	camera_inf.param.width = dev->width;
	camera_inf.param.height = dev->height;
	camera_inf.fmt.fourcc = V4L2_PIX_FMT_NV12;
	camera_inf.fd_helix = edev->helix_fd;
	camera_inf.encodec_mode = 1;

	if (jpeg_enc_init(&camera_inf) < 0) {
		jpeg_enc_stop(&camera_inf);
		fprintf(stderr, "jpeg_enc_init failed\n");
		return -1;
	}

	return 0;
}

/*
 * Converting nv12 to JPEG format using hardware.
 */
int nv12_to_jpeg(uint8_t *p_dst, uint8_t *src)
{
	int frame_size;

	camera_inf.buf_vaddr = src;

	if (camera_inf.encodec_mode == 1) {
		frame_size = jpeg_enc_start(&camera_inf, p_dst);
	}

	return frame_size;
}

/* ---------------------------------------------------------------------------
 * V4L2 streaming related
 */

static int v4l2_uninit_device(struct v4l2_device *dev)
{
	unsigned int i;
	int ret;

	switch (dev->io) {
		case IO_METHOD_MMAP:
			for (i = 0; i < dev->nbufs; ++i) {
				ret = munmap(dev->mem[i].start, dev->mem[i].length);
				if (ret < 0) {
					printf("V4L2: munmap failed\n");
					return ret;
				}
			}

			free(dev->mem);
			break;

		case IO_METHOD_USERPTR:
		default:
			break;
	}

	return 0;
}

static int v4l2_reqbufs_mmap(struct v4l2_device *dev, int nbufs)
{
	struct v4l2_requestbuffers req;
	unsigned int i = 0;
	int ret;

	CLEAR(req);

	req.count = nbufs;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	ret = ioctl(dev->v4l2_fd, VIDIOC_REQBUFS, &req);
	if (ret < 0) {
		if (ret == -EINVAL)
			printf("V4L2: does not support memory mapping\n");
		else
			printf("V4L2: VIDIOC_REQBUFS error %s (%d).\n", strerror(errno), errno);
		goto err;
	}

	if (!req.count)
		return 0;

	if (req.count < 2) {
		printf("V4L2: Insufficient buffer memory.\n");
		ret = -EINVAL;
		goto err;
	}

	/* Map the buffers. */
	dev->mem = calloc(req.count, sizeof dev->mem[0]);
	if (!dev->mem) {
		printf("V4L2: Out of memory\n");
		ret = -ENOMEM;
		goto err;
	}

	for (i = 0; i < req.count; ++i) {
		memset(&dev->mem[i].buf, 0, sizeof(dev->mem[i].buf));

		dev->mem[i].buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		dev->mem[i].buf.memory = V4L2_MEMORY_MMAP;
		dev->mem[i].buf.index = i;

		ret = ioctl(dev->v4l2_fd, VIDIOC_QUERYBUF, &(dev->mem[i].buf));
		if (ret < 0) {
			printf(
					"V4L2: VIDIOC_QUERYBUF failed for buf %d: "
					"%s (%d).\n",
					i, strerror(errno), errno);
			ret = -EINVAL;
			goto err_free;
		}

		dev->mem[i].start =
			mmap(NULL /* start anywhere */, dev->mem[i].buf.length, PROT_READ | PROT_WRITE /* required */,
					MAP_SHARED /* recommended */, dev->v4l2_fd, dev->mem[i].buf.m.offset);

		if (MAP_FAILED == dev->mem[i].start) {
			printf("V4L2: Unable to map buffer %u: %s (%d).\n", i, strerror(errno), errno);
			dev->mem[i].length = 0;
			ret = -EINVAL;
			goto err_free;
		}

		dev->mem[i].length = dev->mem[i].buf.length;
		printf("V4L2: Buffer %u mapped at address %p.\n", i, dev->mem[i].start);
	}

	dev->nbufs = req.count;
	printf("V4L2: %u buffers allocated.\n", req.count);

	return 0;

err_free:
	free(dev->mem);
err:
	return ret;
}

static int v4l2_reqbufs_userptr(struct v4l2_device *dev, int nbufs)
{
	struct v4l2_requestbuffers req;
	int ret;

	CLEAR(req);

	req.count = nbufs;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;

	ret = ioctl(dev->v4l2_fd, VIDIOC_REQBUFS, &req);
	if (ret < 0) {
		if (ret == -EINVAL)
			printf("V4L2: does not support user pointer i/o\n");
		else
			printf("V4L2: VIDIOC_REQBUFS error %s (%d).\n", strerror(errno), errno);
		return ret;
	}

	dev->nbufs = req.count;
	printf("V4L2: %u buffers allocated.\n", req.count);

	return 0;
}

static int v4l2_reqbufs(struct v4l2_device *dev, int nbufs)
{
	int ret = 0;

	switch (dev->io) {
		case IO_METHOD_MMAP:
			ret = v4l2_reqbufs_mmap(dev, nbufs);
			break;

		case IO_METHOD_USERPTR:
			ret = v4l2_reqbufs_userptr(dev, nbufs);
			break;

		default:
			ret = -EINVAL;
			break;
	}

	return ret;
}

static int v4l2_qbuf_mmap(struct v4l2_device *dev)
{
	unsigned int i;
	int ret;

	for (i = 0; i < dev->nbufs; ++i) {
		memset(&dev->mem[i].buf, 0, sizeof(dev->mem[i].buf));

		dev->mem[i].buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		dev->mem[i].buf.memory = V4L2_MEMORY_MMAP;
		dev->mem[i].buf.index = i;

		ret = ioctl(dev->v4l2_fd, VIDIOC_QBUF, &(dev->mem[i].buf));
		if (ret < 0) {
			printf("V4L2: VIDIOC_QBUF failed : %s (%d).\n", strerror(errno), errno);
			return ret;
		}

		dev->qbuf_count++;
	}

	return 0;
}

static int v4l2_qbuf(struct v4l2_device *dev)
{
	int ret = 0;

	switch (dev->io) {
		case IO_METHOD_MMAP:
			ret = v4l2_qbuf_mmap(dev);
			break;

		case IO_METHOD_USERPTR:
			/* Empty. */
			ret = 0;
			break;

		default:
			ret = -EINVAL;
			break;
	}

	return ret;
}

static int v4l2_process_data(struct v4l2_device *dev)
{
	int ret;
	struct v4l2_buffer vbuf;
	struct v4l2_buffer ubuf;
	uint32_t bytesused;
	uint8_t *p_buf;
	unsigned int ubuf_index = 0;
	struct v4l2_format fmt;

	/* Return immediately if V4l2 streaming has not yet started. */
	if (!dev->is_streaming)
		return 0;

	/* 1. Dequeue spent buffer rom V4L2 domain. */
	CLEAR(vbuf);

	vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	switch (dev->io) {
		case IO_METHOD_USERPTR:
			vbuf.memory = V4L2_MEMORY_USERPTR;
			break;

		case IO_METHOD_MMAP:
		default:
			vbuf.memory = V4L2_MEMORY_MMAP;
			break;
	}

	ret = ioctl(dev->v4l2_fd, VIDIOC_DQBUF, &vbuf);
	if (ret < 0) {
		return ret;
	}

	/* 2. Queue video buffer to UVC domain. */
	CLEAR(ubuf);
	ubuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	switch (dev->udev->io) {
		case IO_METHOD_MMAP:
			ubuf.memory = V4L2_MEMORY_MMAP;
			break;

		case IO_METHOD_USERPTR:
		default:
			ubuf.memory = V4L2_MEMORY_USERPTR;
			break;
	}

	if(dev->udev->is_streaming) {
		/* Dequeue the spent buffer from UVC domain */
		unsigned int try_count = 1000;
		do {
			ret = ioctl(dev->udev->uvc_fd, VIDIOC_DQBUF, &ubuf);
			if(ret < 0) {
				//perror("Falied to dqbuf from uvc");
			}

		} while(ret < 0 && --try_count);

		if(!try_count) {
			//perror("Failed to dqbuf from uvc!");
			goto err_dqubuf;
		}

		ubuf_index = ubuf.index;

	} else {
		ubuf_index = dev->udev->qbuf_count;
	}

#if 0
	printf("----------ubuf.index: %d, dev->udev->mem[ubuf.index].start: %x, ubuf.m.userptr: %x\n", ubuf_index, dev->udev->mem[ubuf_index].start, ubuf.m.userptr);
#endif

	CLEAR(fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(dev->v4l2_fd, VIDIOC_G_FMT, &fmt);
	if (ret < 0) {
		return ret;
	}

	/* 3. Process v4l2 buffer to uvc buffer. */
	p_buf = dev->udev->mem[ubuf_index].start;
	switch (dev->udev->fcc) {
		case V4L2_PIX_FMT_YUYV:
			/* buffer switch format grey_to_yuyv */
			if(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_GREY)
				grey_to_yuyv(p_buf, dev->udev->width * 2,
						dev->mem[vbuf.index].start, dev->udev->width, dev->udev->height);
			else
			/* buffer switch format nv12_to_yuyv */
				nv12_to_yuyv(p_buf, dev->udev->width * 2,
						dev->mem[vbuf.index].start, dev->udev->width, dev->udev->height);
			bytesused = dev->udev->width * dev->udev->height *2;

			break;

		case V4L2_PIX_FMT_MJPEG:
			/* buffer switch format nv12_to_jpeg */
#if 0
			{
				int fd = open("/mnt/camera_source.yuv", O_RDWR | O_CREAT | O_APPEND, 0775);
				write(fd, dev->mem[vbuf.index].start, vbuf.bytesused);
				close(fd);
			}

#endif
			bytesused = nv12_to_jpeg(p_buf, dev->mem[vbuf.index].start);

#if 0
			{
				char filename[32];

				sprintf(filename, "/mnt/u-jpg-%d.jpg", dev->udev->qbuf_count);

				int fd = open(filename, O_RDWR | O_CREAT, 0775);

				write(fd, p_buf, bytesused);

				close(fd);
			}
#endif
			break;

		default:
			printf("UVC: format is no support.\n");
	}

	/* 4. Qbuf to UVC.*/
	CLEAR(ubuf);
	ubuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	switch (dev->udev->io) {
		case IO_METHOD_MMAP:
			ubuf.memory = V4L2_MEMORY_MMAP;
			break;

		case IO_METHOD_USERPTR:
		default:
			ubuf.memory = V4L2_MEMORY_USERPTR;
			ubuf.m.userptr = p_buf;
			break;
	}

	ubuf.index = ubuf_index;
	ubuf.length = dev->udev->mem[ubuf_index].length;
	ubuf.bytesused = bytesused;

#if 1
	ret = ioctl(dev->udev->uvc_fd, VIDIOC_QBUF, &ubuf);
	if (ret < 0) {
		/* Check for a USB disconnect/shutdown event. */
		if (errno == ENODEV) {
			dev->udev->uvc_shutdown_requested = 1;
			printf(
					"UVC: Possible USB shutdown requested from "
					"Host, seen during VIDIOC_QBUF\n");
			return 0;
		} else {
			printf("Failed to qbuf to uvc index: %d, continue\n", ubuf.index);
			perror("Failed to qbuf to uvc");
		}
	}
#endif

	dev->udev->qbuf_count++;

	/* 5. Qbuf to v4l2.*/
	ret = ioctl(dev->v4l2_fd, VIDIOC_QBUF, &vbuf);
	if (ret < 0) {
		perror("Failed to qbuf to camera!");
		return ret;
	}

	if (!dev->udev->first_buffer_queued && !dev->udev->run_standalone && dev->udev->qbuf_count == dev->udev->nbufs) {
	//if (!dev->udev->first_buffer_queued && !dev->udev->run_standalone) {
		uvc_video_stream(dev->udev, 1);
		dev->udev->first_buffer_queued = 1;
		dev->udev->is_streaming = 1;
	}

	//dev->udev->qbuf_count %= dev->udev->nbufs;

	return 0;

err_dqubuf:
	ret = ioctl(dev->v4l2_fd, VIDIOC_QBUF, &vbuf);
	if (ret < 0) {
		perror("Failed to qbuf to camera!");
		return ret;
	}

	return ret;
}

/* ---------------------------------------------------------------------------
 * V4L2 generic stuff
 */

static int v4l2_get_format(struct v4l2_device *dev)
{
	struct v4l2_format fmt;
	int ret;

	CLEAR(fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	ret = ioctl(dev->v4l2_fd, VIDIOC_G_FMT, &fmt);
	if (ret < 0) {
		return ret;
	}

	printf("V4L2: Getting current format: %c%c%c%c %ux%u\n", pixfmtstr(fmt.fmt.pix.pixelformat), fmt.fmt.pix.width,
			fmt.fmt.pix.height);

	return 0;
}

static int v4l2_set_format(struct v4l2_device *dev, struct v4l2_format *fmt)
{
	int ret;

	ret = ioctl(dev->v4l2_fd, VIDIOC_S_FMT, fmt);
	if (ret < 0) {
		printf("V4L2: Unable to set format %s (%d).\n", strerror(errno), errno);
		return ret;
	}

	printf("V4L2: Setting format to: %c%c%c%c %ux%u\n", pixfmtstr(fmt->fmt.pix.pixelformat), fmt->fmt.pix.width,
			fmt->fmt.pix.height);


	return 0;
}

static int v4l2_set_ctrl(struct v4l2_device *dev, int new_val, int ctrl)
{
	struct v4l2_queryctrl queryctrl;
	struct v4l2_control control;
	int ret;

	CLEAR(queryctrl);

	switch (ctrl) {
		case V4L2_CID_BRIGHTNESS:
			queryctrl.id = V4L2_CID_BRIGHTNESS;
			ret = ioctl(dev->v4l2_fd, VIDIOC_QUERYCTRL, &queryctrl);
			if (-1 == ret) {
				if (errno != EINVAL)
					printf(
							"V4L2: VIDIOC_QUERYCTRL"
							" failed: %s (%d).\n",
							strerror(errno), errno);
				else
					printf(
							"V4L2_CID_BRIGHTNESS is not"
							" supported: %s (%d).\n",
							strerror(errno), errno);

				return ret;
			} else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
				printf("V4L2_CID_BRIGHTNESS is not supported.\n");
				ret = -EINVAL;
				return ret;
			} else {
				CLEAR(control);
				control.id = V4L2_CID_BRIGHTNESS;
				control.value = new_val;

				ret = ioctl(dev->v4l2_fd, VIDIOC_S_CTRL, &control);
				if (-1 == ret) {
					printf("V4L2: VIDIOC_S_CTRL failed: %s (%d).\n", strerror(errno), errno);
					return ret;
				}
			}
			printf("V4L2: Brightness control changed to value = 0x%x\n", new_val);
			break;

		default:
			/* TODO: We don't support any other controls. */
			return -EINVAL;
	}

	return 0;
}

static int v4l2_open(struct v4l2_device *v4l2, char *devname);
static int v4l2_close(struct v4l2_device *dev);
static int v4l2_set_fmt(struct v4l2_device *vdev, struct v4l2_format *s_fmt);

static int v4l2_start_capturing(struct v4l2_device *dev)
{
	int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	int ret;

	ret = ioctl(dev->v4l2_fd, VIDIOC_STREAMON, &type);
	if (ret < 0) {
		printf("V4L2: Unable to start streaming: %s (%d).\n", strerror(errno), errno);
		return ret;
	}

	printf("V4L2: Starting video stream.\n");

	return 0;
}

static int v4l2_stop_capturing(struct v4l2_device *dev)
{
	enum v4l2_buf_type type;
	int ret;

	switch (dev->io) {
		case IO_METHOD_MMAP:
			type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

			ret = ioctl(dev->v4l2_fd, VIDIOC_STREAMOFF, &type);
			if (ret < 0) {
				printf("V4L2: VIDIOC_STREAMOFF failed: %s (%d).\n", strerror(errno), errno);
				return ret;
			}

			break;
		default:
			/* Nothing to do. */
			break;
	}

	return 0;
}
static int v4l2_set_fmt(struct v4l2_device *vdev, struct v4l2_format *s_fmt)
{
	int ret = -EINVAL;

	/* Get the default image format supported. */
	ret = v4l2_get_format(vdev);
	if (ret < 0)
		goto err;

	/*
	 * Set the desired image format.
	 * Note: VIDIOC_S_FMT may change width and height.
	 */
	ret = v4l2_set_format(vdev, s_fmt);
	if (ret < 0)
		goto err;

	/* Get the changed image format. */
	ret = v4l2_get_format(vdev);
	if (ret < 0)
		goto err;

	return 0;
err:
	return ret;

}

static int v4l2_open(struct v4l2_device *vdev, char *devname)
{
	struct v4l2_capability cap;
	int fd;
	int ret = -EINVAL;

	fd = open(devname, O_RDWR | O_NONBLOCK, 0);
	if (fd == -1) {
		printf("V4L2: device open failed: %s (%d).\n", strerror(errno), errno);
		return ret;
	}

	ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
	if (ret < 0) {
		printf("V4L2: VIDIOC_QUERYCAP failed: %s (%d).\n", strerror(errno), errno);
		goto err;
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		printf("V4L2: %s is no video capture device\n", devname);
		goto err;
	}

	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		printf("V4L2: %s does not support streaming i/o\n", devname);
		goto err;
	}


	printf("V4L2 device is %s on bus %s\n", cap.card, cap.bus_info);

	vdev->v4l2_fd = fd;

	printf("v4l2 open succeeded, file descriptor = %d\n", fd);

	return 0;

err:
	close(fd);

	return ret;
}

static int v4l2_close(struct v4l2_device *dev)
{
	int ret;

	ret = close(dev->v4l2_fd);
	if (ret) {
		printf("V4L2: device close failed: %s (%d).\n", strerror(errno), errno);
	}

	return ret;
}

static int helix_open(struct helix_device *edev, char *devname)
{
	int fd;

	fd = open(devname, O_RDWR | O_NONBLOCK, 0);
	if (fd == -1) {
		printf("helix: device open failed: %s (%d).\n", strerror(errno), errno);
		return -EINVAL;
	}

	edev->helix_fd = fd;

	return 0;
}

static int helix_close(struct helix_device *edev)
{
	int ret;

	ret = close(edev->helix_fd);
	if (ret) {
		printf("HELIX: device close failed: %s (%d).\n", strerror(errno), errno);
	}

	return ret;
}

/* ---------------------------------------------------------------------------
 * UVC generic stuff
 */
static int uvc_video_get_format(struct uvc_device *dev)
{
	struct v4l2_format fmt;
	int ret;

	CLEAR(fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

	ret = ioctl(dev->uvc_fd, VIDIOC_G_FMT, &fmt);
	if (ret < 0) {
		perror("UVC: unable to get fmt from uvc!");
		return ret;
	}

	printf("UVC: Getting format  %c%c%c%c %ux%u, sizeimage: %d\n", pixfmtstr(fmt.fmt.pix.pixelformat), fmt.fmt.pix.width, fmt.fmt.pix.height, fmt.fmt.pix.sizeimage);
}
static int uvc_video_set_format(struct uvc_device *dev)
{
	struct v4l2_format fmt;
	int ret;

	CLEAR(fmt);

	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	fmt.fmt.pix.width = dev->width;
	fmt.fmt.pix.height = dev->height;
	fmt.fmt.pix.pixelformat = dev->fcc;
	fmt.fmt.pix.field = V4L2_FIELD_NONE;

	if (dev->fcc == V4L2_PIX_FMT_MJPEG)
		fmt.fmt.pix.sizeimage = dev->width * dev->height * 3 / 2;	// 按原始帧大小分配内存.

	ret = ioctl(dev->uvc_fd, VIDIOC_S_FMT, &fmt);
	if (ret < 0) {
		printf("UVC: Unable to set format %s (%d).\n", strerror(errno), errno);
		return ret;
	}

	printf("UVC: Setting format to: %c%c%c%c %ux%u\n", pixfmtstr(dev->fcc), dev->width, dev->height);

	return 0;
}

static int uvc_video_stream(struct uvc_device *dev, int enable)
{
	int type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	int ret;

	if (!enable) {
		ret = ioctl(dev->uvc_fd, VIDIOC_STREAMOFF, &type);
		if (ret < 0) {
			printf("UVC: VIDIOC_STREAMOFF failed: %s (%d).\n", strerror(errno), errno);
			return ret;
		}

		printf("UVC: Stopping video stream.\n");

		return 0;
	}

	ret = ioctl(dev->uvc_fd, VIDIOC_STREAMON, &type);
	if (ret < 0) {
		printf("UVC: Unable to start streaming %s (%d).\n", strerror(errno), errno);
		return ret;
	}

	printf("UVC: Starting video stream.\n");

	dev->uvc_shutdown_requested = 0;

	return 0;
}

static int uvc_uninit_device(struct uvc_device *dev)
{
	unsigned int i;
	int ret;

	switch (dev->io) {
		case IO_METHOD_MMAP:
			for (i = 0; i < dev->nbufs; ++i) {
				ret = munmap(dev->mem[i].start, dev->mem[i].length);
				if (ret < 0) {
					printf("UVC: munmap failed\n");
					return ret;
				}
			}

			free(dev->mem);
			break;

		case IO_METHOD_USERPTR:
		default:
			for (i = 0; i < dev->nbufs; ++i)
				free(dev->dummy_buf[i].start);

			free(dev->dummy_buf);
			break;
	}

	return 0;
}
static int uvc_open(struct uvc_device **uvc, char *devname)
{
	struct uvc_device *dev;
	struct v4l2_capability cap;
	int fd;
	int ret = -EINVAL;

	fd = open(devname, O_RDWR | O_NONBLOCK);
	if (fd == -1) {
		printf("UVC: device open failed: %s (%d).\n", strerror(errno), errno);
		return ret;
	}

	ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
	if (ret < 0) {
		printf("UVC: unable to query uvc device: %s (%d)\n", strerror(errno), errno);
		goto err;
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)) {
		printf("UVC: %s is no video output device\n", devname);
		goto err;
	}

	dev = calloc(1, sizeof *dev);
	if (dev == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	printf("uvc device is %s on bus %s\n", cap.card, cap.bus_info);
	printf("uvc open succeeded, file descriptor = %d\n", fd);

	dev->uvc_fd = fd;
	*uvc = dev;

	return 0;

err:
	close(fd);
	return ret;
}

static void uvc_close(struct uvc_device *dev)
{
	close(dev->uvc_fd);
	free(dev);
}

/* ---------------------------------------------------------------------------
 * UVC streaming related
 */

static void uvc_video_fill_buffer(struct uvc_device *dev, struct v4l2_buffer *buf)
{
	unsigned int bpl;
	unsigned int i;

	memcpy(dev->mem[buf->index].start, dev->imgdata, dev->imgsize);
	buf->bytesused = dev->imgsize;
}

static int uvc_video_process(struct uvc_device *dev)
{
	struct v4l2_buffer ubuf;
	struct v4l2_buffer vbuf;
	unsigned int i;
	int ret;
	/*
	 * Return immediately if UVC video output device has not started
	 * streaming yet.
	 */
	if (!dev->is_streaming)
		return 0;
	/* Prepare a v4l2 buffer to be dequeued from UVC domain. */
	CLEAR(ubuf);

	ubuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	switch (dev->io) {
		case IO_METHOD_MMAP:
			ubuf.memory = V4L2_MEMORY_MMAP;
			break;

		case IO_METHOD_USERPTR:
		default:
			ubuf.memory = V4L2_MEMORY_USERPTR;
			break;
	}
	if (dev->run_standalone) {
		/* UVC stanalone setup. */
		ret = ioctl(dev->uvc_fd, VIDIOC_DQBUF, &ubuf);
		if (ret < 0)
			return ret;

#ifdef ENABLE_BUFFER_DEBUG
		printf("DeQueued buffer at UVC side = %d\n", ubuf.index);
#endif
		uvc_video_fill_buffer(dev, &ubuf);

		ret = ioctl(dev->uvc_fd, VIDIOC_QBUF, &ubuf);
		if (ret < 0)
			return ret;

#ifdef ENABLE_BUFFER_DEBUG
		printf("ReQueueing buffer at UVC side = %d\n", ubuf.index);
#endif
	} else {

	}

	return 0;
}

static int uvc_video_qbuf_mmap(struct uvc_device *dev)
{
	unsigned int i;
	int ret;

	for (i = 0; i < dev->nbufs; ++i) {
		memset(&dev->mem[i].buf, 0, sizeof(dev->mem[i].buf));

		dev->mem[i].buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		dev->mem[i].buf.memory = V4L2_MEMORY_MMAP;
		dev->mem[i].buf.index = i;

		/* UVC standalone setup. */
		if (dev->run_standalone)
			uvc_video_fill_buffer(dev, &(dev->mem[i].buf));

		ret = ioctl(dev->uvc_fd, VIDIOC_QBUF, &(dev->mem[i].buf));
		if (ret < 0) {
			printf("UVC: VIDIOC_QBUF failed : %s (%d).\n", strerror(errno), errno);
			return ret;
		}

		dev->qbuf_count++;
	}

	return 0;
}

static int uvc_video_qbuf_userptr(struct uvc_device *dev)
{
	unsigned int i;
	int ret;

	/* UVC standalone setup. */
	for (i = 0; i < dev->nbufs; ++i) {
		struct v4l2_buffer buf;

		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		buf.memory = V4L2_MEMORY_USERPTR;
		buf.m.userptr = (unsigned long)dev->dummy_buf[i].start;
		buf.length = dev->dummy_buf[i].length;
		buf.bytesused = dev->dummy_buf[i].length;
		buf.index = i;

		/* UVC standalone setup. */
		if (dev->run_standalone)
			uvc_video_fill_buffer(dev, &buf);

		ret = ioctl(dev->uvc_fd, VIDIOC_QBUF, &buf);
		if (ret < 0) {
			printf("UVC: VIDIOC_QBUF failed : %s (%d).\n", strerror(errno), errno);
			return ret;
		}

		dev->qbuf_count++;
	}

	return 0;
}

static int uvc_video_qbuf(struct uvc_device *dev)
{
	int ret = 0;

	switch (dev->io) {
		case IO_METHOD_MMAP:
			ret = uvc_video_qbuf_mmap(dev);
			break;

		case IO_METHOD_USERPTR:
			ret = uvc_video_qbuf_userptr(dev);
			break;

		default:
			ret = -EINVAL;
			break;
	}

	return ret;
}

static int uvc_video_reqbufs_mmap(struct uvc_device *dev, int nbufs)
{
	struct v4l2_requestbuffers rb;
	unsigned int i;
	int ret;

	CLEAR(rb);

	rb.count = nbufs;
	rb.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	rb.memory = V4L2_MEMORY_MMAP;

	ret = ioctl(dev->uvc_fd, VIDIOC_REQBUFS, &rb);
	if (ret < 0) {
		if (ret == -EINVAL)
			printf("UVC: does not support memory mapping\n");
		else
			printf("UVC: Unable to allocate buffers: %s (%d).\n", strerror(errno), errno);
		goto err;
	}

	if (!rb.count)
		return 0;

	if (rb.count < 2) {
		printf("UVC: Insufficient buffer memory.\n");
		ret = -EINVAL;
		goto err;
	}

	/* Map the buffers. */
	dev->mem = calloc(rb.count, sizeof dev->mem[0]);
	if (!dev->mem) {
		printf("UVC: Out of memory\n");
		ret = -ENOMEM;
		goto err;
	}

	for (i = 0; i < rb.count; ++i) {
		memset(&dev->mem[i].buf, 0, sizeof(dev->mem[i].buf));

		dev->mem[i].buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		dev->mem[i].buf.memory = V4L2_MEMORY_MMAP;
		dev->mem[i].buf.index = i;

		ret = ioctl(dev->uvc_fd, VIDIOC_QUERYBUF, &(dev->mem[i].buf));
		if (ret < 0) {
			printf(
					"UVC: VIDIOC_QUERYBUF failed for buf %d: "
					"%s (%d).\n",
					i, strerror(errno), errno);
			ret = -EINVAL;
			goto err_free;
		}
		dev->mem[i].start =
			mmap(NULL /* start anywhere */, dev->mem[i].buf.length, PROT_READ | PROT_WRITE /* required */,
					MAP_SHARED /* recommended */, dev->uvc_fd, dev->mem[i].buf.m.offset);

		if (MAP_FAILED == dev->mem[i].start) {
			printf("UVC: Unable to map buffer %u: %s (%d).\n", i, strerror(errno), errno);
			dev->mem[i].length = 0;
			ret = -EINVAL;
			goto err_free;
		}

		dev->mem[i].length = dev->mem[i].buf.length;
		printf("UVC: Buffer %u mapped at address %p.\n", i, dev->mem[i].start);
	}

	dev->nbufs = rb.count;
	printf("UVC: %u buffers allocated.\n", rb.count);

	return 0;

err_free:
	free(dev->mem);
err:
	return ret;
}

static int uvc_video_reqbufs_userptr(struct uvc_device *dev, int nbufs)
{
	struct v4l2_requestbuffers rb;
	unsigned int i, j, bpl, payload_size;
	int ret;

	CLEAR(rb);

	rb.count = nbufs;
	rb.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	rb.memory = V4L2_MEMORY_USERPTR;

	ret = ioctl(dev->uvc_fd, VIDIOC_REQBUFS, &rb);
	if (ret < 0) {
		if (ret == -EINVAL)
			printf("UVC: does not support user pointer i/o\n");
		else
			printf("UVC: VIDIOC_REQBUFS error %s (%d).\n", strerror(errno), errno);
		goto err;
	}

	if (!rb.count)
		return 0;

	dev->nbufs = rb.count;
	printf("UVC: %u buffers allocated.\n", rb.count);

	/* Allocate buffers to hold dummy data pattern. */
	dev->dummy_buf = calloc(rb.count, sizeof dev->dummy_buf[0]);
	if (!dev->dummy_buf) {
		printf("UVC: Out of memory\n");
		ret = -ENOMEM;
		goto err;
	}

	switch (dev->fcc) {
		case V4L2_PIX_FMT_YUYV:
			bpl = dev->width * 2;
			payload_size = dev->width * dev->height * 2;
			break;
		case V4L2_PIX_FMT_MJPEG:
			payload_size = dev->width * dev->height * 3 / 2;	//假设图片大小小于width * height * 3 / 2.
			break;
	}

	for (i = 0; i < rb.count; ++i) {
		dev->dummy_buf[i].length = payload_size;
		dev->dummy_buf[i].start = valloc(payload_size);

		if (!dev->dummy_buf[i].start) {
			printf("UVC: Out of memory\n");
			ret = -ENOMEM;
			goto err;
		}
		printf("UVC Creating buffer @ %p, size: %d\n", dev->dummy_buf[i].start, dev->dummy_buf[i].length);

		memset(dev->dummy_buf[i].start, 0, dev->dummy_buf[i].length);
	}

	dev->mem = dev->dummy_buf;

	return 0;

err:
	return ret;
}

static int uvc_video_reqbufs(struct uvc_device *dev, int nbufs)
{
	int ret = 0;

	switch (dev->io) {
		case IO_METHOD_MMAP:
			ret = uvc_video_reqbufs_mmap(dev, nbufs);
			break;

		case IO_METHOD_USERPTR:
			ret = uvc_video_reqbufs_userptr(dev, nbufs);
			break;

		default:
			ret = -EINVAL;
			break;
	}

	return ret;
}

/*
 * This function is called in response to either:
 * 	- A SET_ALT(interface 1, alt setting 1) command from USB host,
 * 	  if the UVC gadget supports an ISOCHRONOUS video streaming endpoint
 * 	  or,
 *
 *	- A UVC_VS_COMMIT_CONTROL command from USB host, if the UVC gadget
 *	  supports a BULK type video streaming endpoint.
 */
static int uvc_handle_streamon_event(struct uvc_device *dev)
{
	int ret;
	struct v4l2_format fmt;
	struct v4l2_fmtdesc fmt_enum;
	struct v4l2_device *vdev;
	struct helix_device *edev;

	vdev = dev->vdev;
	edev = dev->edev;

	/*run_standalone 代表从文件读取stream而不是从camera设备.*/
	if (!dev->run_standalone) {

		if (!vdev->is_busy) {
			ret = v4l2_open(vdev, vdev->v4l2_devname);
			if (ret < 0)
				goto err_v4l2_open;

			vdev->is_busy = 1;
		}

		if (!edev->is_busy && dev->fcc == V4L2_PIX_FMT_MJPEG) {
			ret = helix_open(edev, edev->helix_devname);
			if (ret < 0)
				goto err_helix_open;

			edev->is_busy = 1;
		}

		CLEAR(fmt_enum);
		fmt_enum.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		fmt_enum.index = 0;
		ret = ioctl(vdev->v4l2_fd, VIDIOC_ENUM_FMT, &fmt_enum);
		if (ret < 0) {
			printf("UVC: VIDIOC_ENUM_FMT failed: %s (%d).\n", strerror(errno), errno);
			return ret;
		}

		/*
		 * Try to set the default format at the V4L2 video capture
		 * device as requested by the user.
		 */
		CLEAR(fmt);
		fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		fmt.fmt.pix.width = dev->width;
		fmt.fmt.pix.height = dev->height;
		fmt.fmt.pix.sizeimage = (fmt.fmt.pix.width * fmt.fmt.pix.height * 3 / 2);
//		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
		fmt.fmt.pix.pixelformat = fmt_enum.pixelformat;
		fmt.fmt.pix.field = V4L2_FIELD_ANY;

		ret = v4l2_set_fmt(vdev, &fmt);
		if(ret < 0) {
			printf("[%s:%d] failed to set fmt\n", __func__, __LINE__);
			goto err;
		}

		/*
		 * Ensure that the V4L2 video capture device has already some
		 * buffers queued.
		 */
		ret = v4l2_reqbufs(vdev, vdev->nbufs);
		if(ret < 0) {
			printf("[%s:%d] failed to reqbufs for v4l2 dev\n", __func__, __LINE__);
			goto err;
		}

		ret = v4l2_qbuf(dev->vdev);
		if (ret < 0)
			goto err;

		/* Start V4L2 capturing now. */
		ret = v4l2_start_capturing(dev->vdev);
		if (ret < 0)
			goto err;

		/* encodec init */
		if (dev->fcc == V4L2_PIX_FMT_MJPEG) {
			ret = encodec_init(dev);
			if (ret < 0) {
				printf("[%s:%d] failed to encodec init.\n", __func__, __LINE__);
			}
		}

		dev->vdev->is_streaming = 1;
	} else {

		char filename[32];

		if(dev->fcc == V4L2_PIX_FMT_MJPEG) {
			sprintf(filename, "%s/uvc-%dx%d.jpg", mjpeg_image ? mjpeg_image : "/mnt", dev->width, dev->height);

		} else if(dev->fcc == V4L2_PIX_FMT_YUYV) {
			sprintf(filename, "%s/uvc-%dx%d.yuv", mjpeg_image ? mjpeg_image : "/mnt", dev->width, dev->height);
		}

		ret = image_load(dev, filename);

		if(ret < 0) {
			return -EINVAL;
		}

	}

	/* set format.*/
	ret = uvc_video_set_format(dev);
	if(ret < 0) {
		printf("Failed to set uvc video format.!\n");

		goto err;
	}

	ret = uvc_video_get_format(dev);

	/* Common setup. */
	ret = uvc_video_reqbufs(dev, dev->nbufs);
	if (ret < 0)
		goto err;


	if (dev->run_standalone) {
		/* Queue buffers to UVC domain and start streaming. */
		ret = uvc_video_qbuf(dev);
		if (ret < 0)
			goto err;

		uvc_video_stream(dev, 1);
		dev->first_buffer_queued = 1;
		dev->is_streaming = 1;
	} else {
		dev->qbuf_count = 0;
	}

	return 0;

err:
if (dev->fcc == V4L2_PIX_FMT_MJPEG) {
	helix_close(edev);
}
err_helix_open:
	v4l2_close(vdev);
err_v4l2_open:
	return ret;
}

/* ---------------------------------------------------------------------------
 * UVC Request processing
 */

	static void
uvc_fill_streaming_control(struct uvc_device *dev, struct uvc_streaming_control *ctrl, int iframe, int iformat)
{
	const struct uvc_format_info *format;
	const struct uvc_frame_info *frame;
	unsigned int nframes;

	if (iformat < 0)
		iformat = ARRAY_SIZE(uvc_formats) + iformat;
	if (iformat < 0 || iformat >= (int)ARRAY_SIZE(uvc_formats))
		return;
	format = &uvc_formats[iformat];

	nframes = 0;
	while (format->frames[nframes].width != 0)
		++nframes;

	if (iframe < 0)
		iframe = nframes + iframe;
	if (iframe < 0 || iframe >= (int)nframes)
		return;
	frame = &format->frames[iframe];

	memset(ctrl, 0, sizeof *ctrl);

	ctrl->bmHint = 1;
	ctrl->bFormatIndex = iformat + 1;
	ctrl->bFrameIndex = iframe + 1;
	ctrl->dwFrameInterval = frame->intervals[0];
	switch (format->fcc) {
		case V4L2_PIX_FMT_YUYV:
			ctrl->dwMaxVideoFrameSize = frame->width * frame->height * 2;
			break;
		case V4L2_PIX_FMT_MJPEG:
			ctrl->dwMaxVideoFrameSize = frame->width * frame->height * 3 / 2;
			break;
	}

	/* TODO: the UVC maxpayload transfer size should be filled
	 * by the driver.
	 */
	if (!dev->bulk)
		ctrl->dwMaxPayloadTransferSize = (dev->maxpkt) * (dev->mult + 1) * (dev->burst + 1);
	else
		ctrl->dwMaxPayloadTransferSize = ctrl->dwMaxVideoFrameSize;

	ctrl->bmFramingInfo = 3;
	ctrl->bPreferedVersion = 1;
	ctrl->bMaxVersion = 1;
}

	static void
uvc_events_process_standard(struct uvc_device *dev, struct usb_ctrlrequest *ctrl, struct uvc_request_data *resp)
{
	printf("standard request\n");
	(void)dev;
	(void)ctrl;
	(void)resp;
}

static void uvc_events_process_control(
		struct uvc_device *dev, uint8_t req, uint8_t cs, uint8_t entity_id, uint8_t len, struct uvc_request_data *resp)
{
	switch (entity_id) {
		case 0:
			switch (cs) {
				case UVC_VC_REQUEST_ERROR_CODE_CONTROL:
					/* Send the request error code last prepared. */
					resp->data[0] = dev->request_error_code.data[0];
					resp->length = dev->request_error_code.length;
					break;

				default:
					/*
					 * If we were not supposed to handle this
					 * 'cs', prepare an error code response.
					 */
					dev->request_error_code.data[0] = 0x06;
					dev->request_error_code.length = 1;
					break;
			}
			break;

			/* Camera terminal unit 'UVC_VC_INPUT_TERMINAL'. */
		case 1:
			switch (cs) {
				/*
				 * We support only 'UVC_CT_AE_MODE_CONTROL' for CAMERA
				 * terminal, as our bmControls[0] = 2 for CT. Also we
				 * support only auto exposure.
				 */
				case UVC_CT_AE_MODE_CONTROL:
					switch (req) {
						case UVC_SET_CUR:
							/* Incase of auto exposure, attempts to
							 * programmatically set the auto-adjusted
							 * controls are ignored.
							 */
							resp->data[0] = 0x01;
							resp->length = 1;
							/*
							 * For every successfully handled control
							 * request set the request error code to no
							 * error.
							 */
							dev->request_error_code.data[0] = 0x00;
							dev->request_error_code.length = 1;
							break;

						case UVC_GET_INFO:
							/*
							 * TODO: We support Set and Get requests, but
							 * don't support async updates on an video
							 * status (interrupt) endpoint as of
							 * now.
							 */
							resp->data[0] = 0x03;
							resp->length = 1;
							/*
							 * For every successfully handled control
							 * request set the request error code to no
							 * error.
							 */
							dev->request_error_code.data[0] = 0x00;
							dev->request_error_code.length = 1;
							break;

						case UVC_GET_CUR:
						case UVC_GET_DEF:
						case UVC_GET_RES:
							/* Auto Mode â€“ auto Exposure Time, auto Iris. */
							resp->data[0] = 0x02;
							resp->length = 1;
							/*
							 * For every successfully handled control
							 * request set the request error code to no
							 * error.
							 */
							dev->request_error_code.data[0] = 0x00;
							dev->request_error_code.length = 1;
							break;
						default:
							/*
							 * We don't support this control, so STALL the
							 * control ep.
							 */
							resp->length = -EL2HLT;
							/*
							 * For every unsupported control request
							 * set the request error code to appropriate
							 * value.
							 */
							dev->request_error_code.data[0] = 0x07;
							dev->request_error_code.length = 1;
							break;
					}
					break;

				default:
					/*
					 * We don't support this control, so STALL the control
					 * ep.
					 */
					resp->length = -EL2HLT;
					/*
					 * If we were not supposed to handle this
					 * 'cs', prepare a Request Error Code response.
					 */
					dev->request_error_code.data[0] = 0x06;
					dev->request_error_code.length = 1;
					break;
			}
			break;

			/* processing unit 'UVC_VC_PROCESSING_UNIT' */
		case 2:
			switch (cs) {
				/*
				 * We support only 'UVC_PU_BRIGHTNESS_CONTROL' for Processing
				 * Unit, as our bmControls[0] = 1 for PU.
				 */
				case UVC_PU_BRIGHTNESS_CONTROL:
					switch (req) {
						case UVC_SET_CUR:
							resp->data[0] = 0x0;
							resp->length = len;
							/*
							 * For every successfully handled control
							 * request set the request error code to no
							 * error
							 */
							dev->request_error_code.data[0] = 0x00;
							dev->request_error_code.length = 1;
							break;
						case UVC_GET_MIN:
							resp->data[0] = PU_BRIGHTNESS_MIN_VAL;
							resp->length = 2;
							/*
							 * For every successfully handled control
							 * request set the request error code to no
							 * error
							 */
							dev->request_error_code.data[0] = 0x00;
							dev->request_error_code.length = 1;
							break;
						case UVC_GET_MAX:
							resp->data[0] = PU_BRIGHTNESS_MAX_VAL;
							resp->length = 2;
							/*
							 * For every successfully handled control
							 * request set the request error code to no
							 * error
							 */
							dev->request_error_code.data[0] = 0x00;
							dev->request_error_code.length = 1;
							break;
						case UVC_GET_CUR:
							resp->length = 2;
							memcpy(&resp->data[0], &dev->brightness_val, resp->length);
							/*
							 * For every successfully handled control
							 * request set the request error code to no
							 * error
							 */
							dev->request_error_code.data[0] = 0x00;
							dev->request_error_code.length = 1;
							break;
						case UVC_GET_INFO:
							/*
							 * We support Set and Get requests and don't
							 * support async updates on an interrupt endpt
							 */
							resp->data[0] = 0x03;
							resp->length = 1;
							/*
							 * For every successfully handled control
							 * request, set the request error code to no
							 * error.
							 */
							dev->request_error_code.data[0] = 0x00;
							dev->request_error_code.length = 1;
							break;
						case UVC_GET_DEF:
							resp->data[0] = PU_BRIGHTNESS_DEFAULT_VAL;
							resp->length = 2;
							/*
							 * For every successfully handled control
							 * request, set the request error code to no
							 * error.
							 */
							dev->request_error_code.data[0] = 0x00;
							dev->request_error_code.length = 1;
							break;
						case UVC_GET_RES:
							resp->data[0] = PU_BRIGHTNESS_STEP_SIZE;
							resp->length = 2;
							/*
							 * For every successfully handled control
							 * request, set the request error code to no
							 * error.
							 */
							dev->request_error_code.data[0] = 0x00;
							dev->request_error_code.length = 1;
							break;
						default:
							/*
							 * We don't support this control, so STALL the
							 * default control ep.
							 */
							resp->length = -EL2HLT;
							/*
							 * For every unsupported control request
							 * set the request error code to appropriate
							 * code.
							 */
							dev->request_error_code.data[0] = 0x07;
							dev->request_error_code.length = 1;
							break;
					}
					break;

				default:
					/*
					 * We don't support this control, so STALL the control
					 * ep.
					 */
					resp->length = -EL2HLT;
					/*
					 * If we were not supposed to handle this
					 * 'cs', prepare a Request Error Code response.
					 */
					dev->request_error_code.data[0] = 0x06;
					dev->request_error_code.length = 1;
					break;
			}

			break;

		default:
			/*
			 * If we were not supposed to handle this
			 * 'cs', prepare a Request Error Code response.
			 */
			dev->request_error_code.data[0] = 0x06;
			dev->request_error_code.length = 1;
			break;
	}

	printf("control request (req %02x cs %02x)\n", req, cs);
}

static void uvc_events_process_streaming(struct uvc_device *dev, uint8_t req, uint8_t cs, struct uvc_request_data *resp)
{
	struct uvc_streaming_control *ctrl;

	printf("streaming request (req %02x cs %02x)\n", req, cs);

	if (cs != UVC_VS_PROBE_CONTROL && cs != UVC_VS_COMMIT_CONTROL)
		return;

	ctrl = (struct uvc_streaming_control *)&resp->data;
	resp->length = sizeof *ctrl;

	switch (req) {
		case UVC_SET_CUR:
			dev->control = cs;
			resp->length = 34;
			break;

		case UVC_GET_CUR:
			if (cs == UVC_VS_PROBE_CONTROL)
				memcpy(ctrl, &dev->probe, sizeof *ctrl);
			else
				memcpy(ctrl, &dev->commit, sizeof *ctrl);
			break;

		case UVC_GET_MIN:
		case UVC_GET_MAX:
		case UVC_GET_DEF:
			uvc_fill_streaming_control(dev, ctrl, req == UVC_GET_MAX ? -1 : 0, req == UVC_GET_MAX ? -1 : 0);
			break;

		case UVC_GET_RES:
			CLEAR(ctrl);
			break;

		case UVC_GET_LEN:
			resp->data[0] = 0x00;
			resp->data[1] = 0x22;
			resp->length = 2;
			break;

		case UVC_GET_INFO:
			resp->data[0] = 0x03;
			resp->length = 1;
			break;
	}
}

	static void
uvc_events_process_class(struct uvc_device *dev, struct usb_ctrlrequest *ctrl, struct uvc_request_data *resp)
{
	if ((ctrl->bRequestType & USB_RECIP_MASK) != USB_RECIP_INTERFACE)
		return;

	switch (ctrl->wIndex & 0xff) {
		case UVC_INTF_CONTROL:
			uvc_events_process_control(dev, ctrl->bRequest, ctrl->wValue >> 8, ctrl->wIndex >> 8, ctrl->wLength, resp);
			break;

		case UVC_INTF_STREAMING:
			uvc_events_process_streaming(dev, ctrl->bRequest, ctrl->wValue >> 8, resp);
			break;

		default:
			break;
	}
}
	static void
uvc_events_process_setup(struct uvc_device *dev, struct usb_ctrlrequest *ctrl, struct uvc_request_data *resp)
{
	dev->control = 0;

#ifdef ENABLE_USB_REQUEST_DEBUG
	printf(
			"\nbRequestType %02x bRequest %02x wValue %04x wIndex %04x "
			"wLength %04x\n",
			ctrl->bRequestType, ctrl->bRequest, ctrl->wValue, ctrl->wIndex, ctrl->wLength);
#endif
	switch (ctrl->bRequestType & USB_TYPE_MASK) {
		case USB_TYPE_STANDARD:
			uvc_events_process_standard(dev, ctrl, resp);
			break;

		case USB_TYPE_CLASS:
			uvc_events_process_class(dev, ctrl, resp);
			break;

		default:
			break;
	}
}

	static int
uvc_events_process_control_data(struct uvc_device *dev, uint8_t cs, uint8_t entity_id, struct uvc_request_data *data)
{
	switch (entity_id) {
		/* Processing unit 'UVC_VC_PROCESSING_UNIT'. */
		case 2:
			switch (cs) {
				/*
				 * We support only 'UVC_PU_BRIGHTNESS_CONTROL' for Processing
				 * Unit, as our bmControls[0] = 1 for PU.
				 */
				case UVC_PU_BRIGHTNESS_CONTROL:
					memcpy(&dev->brightness_val, data->data, data->length);
					/* UVC - V4L2 integrated path. */
					if (!dev->run_standalone)
						/*
						 * Try to change the Brightness attribute on
						 * Video capture device. Note that this try may
						 * succeed or end up with some error on the
						 * video capture side. By default to keep tools
						 * like USBCV's UVC test suite happy, we are
						 * maintaining a local copy of the current
						 * brightness value in 'dev->brightness_val'
						 * variable and we return the same value to the
						 * Host on receiving a GET_CUR(BRIGHTNESS)
						 * control request.
						 *
						 * FIXME: Keeping in view the point discussed
						 * above, notice that we ignore the return value
						 * from the function call below. To be strictly
						 * compliant, we should return the same value
						 * accordingly.
						 */
						v4l2_set_ctrl(dev->vdev, dev->brightness_val, V4L2_CID_BRIGHTNESS);
					break;

				default:
					break;
			}

			break;

		default:
			break;
	}

	printf("Control Request data phase (cs %02x entity %02x)\n", cs, entity_id);

	return 0;
}

static int uvc_events_process_data(struct uvc_device *dev, struct uvc_request_data *data)
{
	struct uvc_streaming_control *target;
	struct uvc_streaming_control *ctrl;
	struct v4l2_format fmt;
	const struct uvc_format_info *format;
	const struct uvc_frame_info *frame;
	const unsigned int *interval;
	unsigned int iformat, iframe;
	unsigned int nframes;
	unsigned int *val = (unsigned int *)data->data;
	int ret;

	switch (dev->control) {
		case UVC_VS_PROBE_CONTROL:
			printf("setting probe control, length = %d\n", data->length);
			target = &dev->probe;
			break;

		case UVC_VS_COMMIT_CONTROL:
			printf("setting commit control, length = %d\n", data->length);
			target = &dev->commit;
			break;

		default:
			printf("setting unknown control, length = %d\n", data->length);

			/*
			 * As we support only BRIGHTNESS control, this request is
			 * for setting BRIGHTNESS control.
			 * Check for any invalid SET_CUR(BRIGHTNESS) requests
			 * from Host. Note that we support Brightness levels
			 * from 0x0 to 0x10 in a step of 0x1. So, any request
			 * with value greater than 0x10 is invalid.
			 */
			if (*val > PU_BRIGHTNESS_MAX_VAL) {
				return -EINVAL;
			} else {
				ret = uvc_events_process_control_data(dev, UVC_PU_BRIGHTNESS_CONTROL, 2, data);
				if (ret < 0)
					goto err;

				return 0;
			}
	}

	ctrl = (struct uvc_streaming_control *)&data->data;
	iformat = clamp((unsigned int)ctrl->bFormatIndex, 1U, (unsigned int)ARRAY_SIZE(uvc_formats));
	format = &uvc_formats[iformat - 1];

	nframes = 0;
	while (format->frames[nframes].width != 0)
		++nframes;

	iframe = clamp((unsigned int)ctrl->bFrameIndex, 1U, nframes);
	frame = &format->frames[iframe - 1];
	interval = frame->intervals;

	while (interval[0] < ctrl->dwFrameInterval && interval[1])
		++interval;

	target->bFormatIndex = iformat;
	target->bFrameIndex = iframe;
	switch (format->fcc) {
		case V4L2_PIX_FMT_YUYV:
			target->dwMaxVideoFrameSize = frame->width * frame->height * 2;
			break;
		case V4L2_PIX_FMT_MJPEG:
			target->dwMaxVideoFrameSize = frame->width * frame->height * 3 / 2;
			break;
	}
	target->dwFrameInterval = *interval;

	if (dev->control == UVC_VS_COMMIT_CONTROL) {
		dev->fcc = format->fcc;
		dev->width = frame->width;
		dev->height = frame->height;
	}

	return 0;

err:
	return ret;
}

static void uvc_events_process(struct uvc_device *dev)
{
	struct v4l2_event v4l2_event;
	struct uvc_event *uvc_event = (void *)&v4l2_event.u.data;
	struct uvc_request_data resp;
	int ret;

	ret = ioctl(dev->uvc_fd, VIDIOC_DQEVENT, &v4l2_event);
	if (ret < 0) {
		printf("VIDIOC_DQEVENT failed: %s (%d)\n", strerror(errno), errno);
		return;
	}

	memset(&resp, 0, sizeof resp);
	resp.length = -EL2HLT;

	switch (v4l2_event.type) {
		case UVC_EVENT_CONNECT:
			return;

		case UVC_EVENT_DISCONNECT:
			dev->uvc_shutdown_requested = 1;
			printf(
					"UVC: Possible USB shutdown requested from "
					"Host, seen via UVC_EVENT_DISCONNECT\n");
			return;

		case UVC_EVENT_SHUTDOWN:
			dev->uvc_close_requested = 1;
			printf(
					"UVC: Possible USB shutdown requested from "
					"Host, seen via UVC_EVENT_SHUTDOWN\n");
			return;

		case UVC_EVENT_SETUP:
			uvc_events_process_setup(dev, &uvc_event->req, &resp);
			break;

		case UVC_EVENT_DATA:
			ret = uvc_events_process_data(dev, &uvc_event->data);
			if (ret < 0)
				break;
			return;

		case UVC_EVENT_STREAMON:
			if (!dev->bulk) {
				uvc_handle_streamon_event(dev);
			}
			return;

		case UVC_EVENT_STREAMOFF:
			/* Stop V4L2 streaming... */
			if (!dev->run_standalone && dev->vdev->is_streaming) {
				/* UVC - V4L2 integrated path. */
				v4l2_stop_capturing(dev->vdev);
				v4l2_uninit_device(dev->vdev);

				dev->vdev->is_streaming = 0;

				if (dev->fcc == V4L2_PIX_FMT_MJPEG) {
					jpeg_enc_stop(&camera_inf);

					/* HELIX device close */
					if (dev->edev->is_busy) {
						ret = helix_close(dev->edev);
						if (!ret) {
							/* close success */
							dev->edev->is_busy = 0;
						}
					}

				}


				/* V4L2 device close */
				if (dev->vdev->is_busy) {
					ret = v4l2_close(dev->vdev);
					if (!ret) {
						/* close success */
						dev->vdev->is_busy = 0;
					}
				}
			} else if(dev->run_standalone) {
				free(dev->imgdata);
			}

			/* ... and now UVC streaming.. */
			if (dev->is_streaming) {
				uvc_video_stream(dev, 0);
				uvc_uninit_device(dev);
				dev->is_streaming = 0;
				dev->first_buffer_queued = 0;
			}

			return;
	}

	ret = ioctl(dev->uvc_fd, UVCIOC_SEND_RESPONSE, &resp);
	if (ret < 0) {
		printf("UVCIOC_S_EVENT failed: %s (%d)\n", strerror(errno), errno);
		return;
	}
}

static void uvc_events_init(struct uvc_device *dev)
{
	struct v4l2_event_subscription sub;
	unsigned int payload_size;

	switch (dev->fcc) {
		case V4L2_PIX_FMT_YUYV:
			payload_size = dev->width * dev->height * 2;
			break;
		case V4L2_PIX_FMT_MJPEG:
			payload_size = dev->width * dev->height * 3 / 2;
			break;
	}

	uvc_fill_streaming_control(dev, &dev->probe, 0, 0);
	uvc_fill_streaming_control(dev, &dev->commit, 0, 0);

	if (dev->bulk) {
		/* FIXME Crude hack, must be negotiated with the driver. */
		dev->probe.dwMaxPayloadTransferSize = dev->commit.dwMaxPayloadTransferSize = payload_size;
	}

	memset(&sub, 0, sizeof sub);
	sub.type = UVC_EVENT_SETUP;
	ioctl(dev->uvc_fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
	sub.type = UVC_EVENT_DATA;
	ioctl(dev->uvc_fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
	sub.type = UVC_EVENT_STREAMON;
	ioctl(dev->uvc_fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
	sub.type = UVC_EVENT_STREAMOFF;
	ioctl(dev->uvc_fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
	sub.type = UVC_EVENT_CONNECT;
	ioctl(dev->uvc_fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
	sub.type = UVC_EVENT_DISCONNECT;
	ioctl(dev->uvc_fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
	sub.type = UVC_EVENT_SHUTDOWN;
	ioctl(dev->uvc_fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
}

/* ---------------------------------------------------------------------------
 * main
 */

static int image_load(struct uvc_device *dev, const char *img)
{
	int fd = -1;

	if (img == NULL)
		return;

	fd = open(img, O_RDONLY);
	if (fd == -1) {
		printf("Unable to open MJPEG image '%s'\n", img);
		return -1;
	}

	dev->imgsize = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	dev->imgdata = malloc(dev->imgsize);
	if (dev->imgdata == NULL) {
		printf("Unable to allocate memory for MJPEG image\n");
		dev->imgsize = 0;
		return -ENOMEM;
	}

	read(fd, dev->imgdata, dev->imgsize);
	close(fd);

	return 0;
}

static void usage(const char *argv0)
{
	fprintf(stderr, "Usage: %s [options]\n", argv0);
	fprintf(stderr, "Available options are\n");
	fprintf(stderr, " -b		Use bulk mode\n");
	fprintf(stderr, " -d		Do not use any real V4L2 capture device\n");
	fprintf(stderr, " -h		Print this help screen and exit\n");
	fprintf(stderr, " -i 		images dir for [uvc-WxH.jpg uvc-WxH.yuv]\n");
	fprintf(stderr, " -m		Streaming mult for ISOC (b/w 0 and 2)\n");
	fprintf(stderr, " -n		Number of Video buffers (b/w 2 and 32)\n");
	fprintf(stderr,
			" -o <IO method> Select UVC IO method:\n\t"
			"0 = MMAP\n\t"
			"1 = USER_PTR\n");
	fprintf(stderr,
			" -s <speed>	Select USB bus speed (b/w 0 and 2)\n\t"
			"0 = Full Speed (FS)\n\t"
			"1 = High Speed (HS)\n\t"
			"2 = Super Speed (SS)\n");
	fprintf(stderr, " -t		Streaming burst (b/w 0 and 15)\n");
	fprintf(stderr, " -u device	UVC Video Output device\n");
	fprintf(stderr, " -v device	V4L2 Video Capture device\n");
	fprintf(stderr, " -e device	HELIX Video Capture device\n");
}

int main(int argc, char *argv[])
{
	struct uvc_device *udev;
	struct v4l2_device *vdev;
	struct helix_device *edev;
	struct timeval tv;
	char *uvc_devname = "/dev/video7";
	char *v4l2_devname = "/dev/video6";
	char *helix_devname = "/dev/video1";

	fd_set fdsv, fdsu;
	int ret, opt, nfds;
	int bulk_mode = 0;
	int nbufs = 3;              /* Ping-Pong buffers */
	/* USB speed related params */
	int mult = 0;
	int burst = 0;
	enum usb_device_speed speed = USB_SPEED_SUPER; /* High-Speed */
	enum io_method uvc_io_method = IO_METHOD_USERPTR;
	enum io_method vdev_io_method = IO_METHOD_MMAP;

	while ((opt = getopt(argc, argv, "b:d:f:h:i:m:n:o:s:t:u:v:e:")) != -1) {
		switch (opt) {
			case 'b':
				bulk_mode = 1;
				break;

			case 'd':
				dummy_data_gen_mode = 1;
				break;

			case 'h':
				usage(argv[0]);
				return 1;

			case 'i':
				mjpeg_image = optarg;
				break;

			case 'm':
				if (atoi(optarg) < 0 || atoi(optarg) > 2) {
					usage(argv[0]);
					return 1;
				}

				mult = atoi(optarg);
				printf("Requested Mult value = %d\n", mult);
				break;

			case 'n':
				if (atoi(optarg) < 2 || atoi(optarg) > 32) {
					usage(argv[0]);
					return 1;
				}

				nbufs = atoi(optarg);
				printf("Number of buffers requested = %d\n", nbufs);
				break;

			case 'o':
				if (atoi(optarg) < 0 || atoi(optarg) > 1) {
					usage(argv[0]);
					return 1;
				}

				uvc_io_method = atoi(optarg);
				printf("UVC: IO method requested is %s\n", (uvc_io_method == IO_METHOD_MMAP) ? "MMAP" : "USER_PTR");
				break;

			case 's':
				if (atoi(optarg) < 0 || atoi(optarg) > 2) {
					usage(argv[0]);
					return 1;
				}

				speed = atoi(optarg);
				break;

			case 't':
				if (atoi(optarg) < 0 || atoi(optarg) > 15) {
					usage(argv[0]);
					return 1;
				}

				burst = atoi(optarg);
				printf("Requested Burst value = %d\n", burst);
				break;

			case 'u':
				uvc_devname = optarg;
				break;

			case 'v':
				v4l2_devname = optarg;
				break;

			case 'e':
				helix_devname = optarg;
				break;

			default:
				printf("Invalid option '-%c'\n", opt);
				usage(argv[0]);
				return 1;
		}
	}

	/* Instantiate a V4L2 device */
	vdev = calloc(1, sizeof *vdev);
	if (vdev == NULL) {
		return -ENOMEM;
	}

	/* Instantiate a HELIX device */
	edev = calloc(1, sizeof *edev);
	if (edev == NULL) {
		return -ENOMEM;
	}

	/* Open the UVC device. */
	ret = uvc_open(&udev, uvc_devname);
	if (udev == NULL || ret < 0)
		return 1;

	udev->uvc_devname = uvc_devname;

	if (!dummy_data_gen_mode && !mjpeg_image) {
		vdev->v4l2_devname = v4l2_devname;
		edev->helix_devname = helix_devname;

		/* Bind UVC and V4L2 devices. */
		udev->vdev = vdev;
		vdev->udev = udev;
		udev->edev = edev;
		vdev->edev = edev;

	}

	udev->io = uvc_io_method;
	udev->bulk = bulk_mode;
	udev->nbufs = nbufs;
	udev->mult = mult;
	udev->burst = burst;
	udev->speed = speed;

	if (dummy_data_gen_mode || mjpeg_image)
		/* UVC standalone setup. */
		udev->run_standalone = 1;

	if (!dummy_data_gen_mode && !mjpeg_image) {
		/* UVC - V4L2 integrated path */
		vdev->nbufs = nbufs;

		/* V4L2 IO method */
		vdev->io = vdev_io_method;

		/* V4l2 fd is not open */
		vdev->is_busy = 0;

		/* HELIX fd is not open */
		edev->is_busy = 0;

		/* UVC/V4L2 buf_index init */
		udev->buf_index = 0;
		vdev->buf_index = 0;
	}

	switch (speed) {
		case USB_SPEED_FULL:
			/* Full Speed. */
			if (bulk_mode)
				udev->maxpkt = 64;
			else
				udev->maxpkt = 1023;
			break;

		case USB_SPEED_HIGH:
			/* High Speed. */
			if (bulk_mode)
				udev->maxpkt = 512;
			else
				udev->maxpkt = 1024;
			break;

		case USB_SPEED_SUPER:
		default:
			/* Super Speed. */
			if (bulk_mode)
				udev->maxpkt = 1024;
			else
				udev->maxpkt = 1024;
			break;
	}

#if 0
	if (mjpeg_image)
		image_load(udev, mjpeg_image);
#endif
	/* Init UVC events. */
	uvc_events_init(udev);

	while (1) {
		if (!dummy_data_gen_mode && !mjpeg_image && vdev->is_streaming)
			FD_ZERO(&fdsv);

		FD_ZERO(&fdsu);

		/* We want both setup and data events on UVC interface.. */
		FD_SET(udev->uvc_fd, &fdsu);

		fd_set efds = fdsu;
		fd_set dfds = fdsu;

		/* ..but only data events on V4L2 interface */
		if (!dummy_data_gen_mode && !mjpeg_image && vdev->is_streaming)
			FD_SET(vdev->v4l2_fd, &fdsv);

		/* Timeout. */
		tv.tv_sec = 2;
		tv.tv_usec = 0;

		if (!dummy_data_gen_mode && !mjpeg_image && vdev->is_streaming) {
			nfds = max(vdev->v4l2_fd, udev->uvc_fd);
			ret = select(nfds + 1, &fdsv, &dfds, &efds, &tv);
		} else {
			ret = select(udev->uvc_fd + 1, NULL, &dfds, &efds, NULL);
		}

		if (-1 == ret) {
			printf("select error %d, %s\n", errno, strerror(errno));
			if (EINTR == errno)
				continue;

			break;
		}

		if (0 == ret) {
			printf("select timeout\n");
			break;
		}

		if (FD_ISSET(udev->uvc_fd, &efds))
			uvc_events_process(udev);
		if (FD_ISSET(udev->uvc_fd, &dfds))
			uvc_video_process(udev);
		if (!dummy_data_gen_mode && !mjpeg_image && vdev->is_streaming)
			if (FD_ISSET(vdev->v4l2_fd, &fdsv))
				v4l2_process_data(vdev);


		/* close app.*/
		if(udev->uvc_close_requested) {
			break;
		}

	}

	if (!dummy_data_gen_mode && !mjpeg_image && vdev->is_streaming) {
		/* Stop V4L2 streaming... */
		v4l2_stop_capturing(vdev);
		v4l2_uninit_device(vdev);
		vdev->is_streaming = 0;

		if (udev->fcc == V4L2_PIX_FMT_MJPEG) {
			jpeg_enc_stop(&camera_inf);

			/* HELIX device close */
			if (edev->is_busy) {
				ret = helix_close(edev);
				if (!ret) {
					/* close success */
					edev->is_busy = 0;
				}
			}
		}

		/* V4L2 device close */
		if (vdev->is_busy) {
			ret = v4l2_close(vdev);
			if (!ret) {
				/* close success */
				vdev->is_busy = 0;
			}
		}


		free(vdev);
		free(edev);

	}

	if (udev->is_streaming) {
		/* ... and now UVC streaming.. */
		uvc_video_stream(udev, 0);
		uvc_uninit_device(udev);
		udev->is_streaming = 0;
	}

	uvc_close(udev);
	return 0;
}
