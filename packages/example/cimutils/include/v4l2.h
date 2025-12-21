#ifndef _V4L2_H_
#define _V4L2_H_

#include <camera.h>
#include <stdbool.h>


struct camera;
struct setformat {
    __u32 width;
    __u32 height;
    __u32 pixfmt;
    __u32 type;
};

int try_format(int fd, struct setformat *setformat);
int helix_set_format(int fd, struct setformat *setformat);
int reqbufs(int fd, __u32 buf_count, __u32 type);
bool type_is_mplane(__u32 type);


#endif
