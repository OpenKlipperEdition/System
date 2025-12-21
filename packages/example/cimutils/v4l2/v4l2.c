#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <inttypes.h>
#include <getopt.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>

#include <v4l2.h>
#include "headers.h"

bool type_is_mplane(__u32 type)
{
    switch (type) {
        case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
        case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
            return true;

        default:
            return false;
    }
}

void setup_format(struct v4l2_format *format, struct setformat *setformat)
{
    __u32 sizeimage;

    memset(format, 0, sizeof(*format));
    format->type = setformat->type;

    if(setformat->pixfmt == V4L2_PIX_FMT_H264 || setformat->pixfmt == V4L2_PIX_FMT_JPEG) {
        sizeimage = setformat->width * setformat->height * 2;	/*压缩后的数据不可能超过原始帧大小.*/
    } else {
        sizeimage = 0;
    }

    if (type_is_mplane(setformat->type)) {
        format->fmt.pix_mp.width = setformat->width;
        format->fmt.pix_mp.height = setformat->height;
        format->fmt.pix_mp.plane_fmt[0].sizeimage = sizeimage;
        format->fmt.pix_mp.pixelformat = setformat->pixfmt;
    } else {
        format->fmt.pix.width = setformat->width;
        format->fmt.pix.height = setformat->height;
        format->fmt.pix.pixelformat = setformat->pixfmt;
    }
}

int try_format(int fd, struct setformat *setformat)
{
    struct v4l2_format format;
    int rc;

    setup_format(&format, setformat);

    rc = ioctl(fd, VIDIOC_TRY_FMT, &format);
    if (rc < 0) {
        fprintf(stderr, "Unable to try format for type %d: %s\n", setformat->type,
                strerror(errno));
        return -1;
    }
    return 0;
}

int helix_set_format(int fd, struct setformat *setformat)
{
    int rc;
    struct v4l2_format vfmt;

    if (try_format(fd, setformat))
        return -1;

    setup_format(&vfmt, setformat);

    rc = ioctl(fd, VIDIOC_S_FMT, &vfmt);
    if (rc < 0) {
        fprintf(stderr, "Unable to set format for type %d: %s\n", setformat->type,
                strerror(errno));
        return -1;
    }
    return 0;
}

int reqbufs(int fd, __u32 buf_count, __u32 type)
{
    struct v4l2_requestbuffers reqbufs;
    int err;

    memset(&reqbufs, 0, sizeof(reqbufs));
    reqbufs.count = buf_count;
    reqbufs.type = type;
    reqbufs.memory = V4L2_MEMORY_MMAP;
    err = ioctl(fd, VIDIOC_REQBUFS, &reqbufs);
    if (err >= 0) {
        return reqbufs.count;
    }

    return -1;
}
