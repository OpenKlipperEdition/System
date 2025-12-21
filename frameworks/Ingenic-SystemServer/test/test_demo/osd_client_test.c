#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "iss_osd.h"

int main(int argc, char * argv[])
{
    int ret = 0;
    int fd = 0;
    int read_size = 0;
    char pic_name[20] = {0};
    char frame[720 * 1280 * 4] = { 0 };
    struct ImageAttr image_attr;
    struct OsdClient *clt = NULL;
   
    clt = ISS_CreateOsdClt(OTHER);
    if (clt == NULL)
    {
        printf("ISS_CreateOsdClt failed!!!\n");
        exit(-1);
    }
    printf("ISS_CreateOsdClt sucess!!!\n");

    image_attr.imageFmt = PIX_FMT_BGRA_8888;
    image_attr.imageWidth = 720;
    image_attr.imageHeight = 1280;
    ret = ISS_SetImageAttr(clt, image_attr);
    if (ret != 0)
    {
        printf("ISS_SetImageAttr failed!!!\n");
        exit(-1);
    }
    printf("ISS_SetImageAttr sucess!!!\n");

    ret = ISS_AllocDispMem(clt, 720 * 1280 * 4, 3);
    if (ret != 0)
    {
        printf("ISS_AllocDispMem failed!!!\n");
        exit(-1);
    }
    printf("ISS_AllocDispMem sucess!!!\n");

    int index = 0;
    while (1)
    {
        index %= 7;
        if (index == 0)
            index = 1;
        sprintf(pic_name, "/720x1280_%d.rgb", index);
        fd = open(pic_name, O_RDWR);
        if (fd < 0)
        {
            printf("open %s failed!!!\n", pic_name);
        }

        read_size = read(fd, &frame, 720 * 1280 * 4);

        ISS_Flush(clt, &frame, read_size);

        index++;
        close(fd);
	    sleep(3);
    }

    ret = ISS_FreeDispMem(clt);
    if (ret != 0)
    {
        printf("ISS_FreeDispMem failed!!!\n");
    }
    printf("ISS_FreeDispMem sucess!!!\n");

    ret = ISS_DestoryOsdClt(clt);
    if (ret != 0)
    {
        printf("ISS_DestoryOsdClt failed!!!\n");
    }
    printf("ISS_DestoryOsdClt sucess!!!\n");

    return 0;
}

