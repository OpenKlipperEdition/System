#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>
#include <assert.h>
#include <rotater.h>


#define SRC_FMT_FLAG 1
#define DST_FMT_FLAG 2



struct picture_info {
        char *srcfile;
        unsigned int sWidth;
        unsigned int sHeight;
        IMPP_PIX_FMT srcfmt;
        IMPP_PIX_FMT dstfmt;
        char *process;
        char *dstfile;
};

static int flag = 0;

static struct option long_options[] = {
        { "src_file", required_argument, NULL, 'i'},
        { "pic_size", required_argument, NULL, 's'},
        { "src_fmt ", required_argument, &flag, SRC_FMT_FLAG},
        { "dst_fmt ", required_argument, &flag, DST_FMT_FLAG},
        { "process ", required_argument, NULL, 'p'},
        { "dst_file", required_argument, NULL, 'o'},
        { "help    ", no_argument,       NULL, 'h'},
        {NULL, 0, NULL, 0},
};

#define print_opt_help(opt_index, help_str)                             \
        do {                                                            \
                        printf("   -%c or --%s          %s", (char)long_options[opt_index].val, long_options[opt_index].name, help_str); \
        } while (0)

static void usage()
{
        printf("\nrotator-x2000-example usage:\n");
        print_opt_help(0, "Source file.\n");
        print_opt_help(1, "The resolution of the source file. The format is WxH, for example: 720x1280.\n");
        printf("   --%s                Source file pixel format.\n", long_options[2].name);
        printf("   --%s                Destination file pixel format.\n", long_options[3].name);
        print_opt_help(4, "Rotator processing method. The default rotation is angle90.\n");
        print_opt_help(5, "The output file.\n");
        print_opt_help(6, "Help.\n");
}

static struct picture_info input_param;

static IMPP_PIX_FMT rot_srcfmtstr_to_imppfmt(const char *strfmt)
{
        assert(strfmt);

        if (0 == strcasecmp("BGRA8888", strfmt)) {
                return IMPP_PIX_FMT_BGRA_8888;
        } else if (0 == strcasecmp("RGB565", strfmt)) {
                return IMPP_PIX_FMT_RGB_565;
        } else if (0 == strcasecmp("RGB555", strfmt)) {
                return IMPP_PIX_FMT_RGB_555;
        } else if (0 == strcasecmp("ARGB1555", strfmt)) {
                return IMPP_PIX_FMT_ARGB_1555;
        } else if (0 == strcasecmp("BGR888", strfmt)) {
                return IMPP_PIX_FMT_BGR_888;
		} else if (0 == strcasecmp("YUYV", strfmt)) {
                return IMPP_PIX_FMT_YUYV;
        } else {
                printf("Src format not supported!\r\n");
                return -1;
        }
}

static IMPP_PIX_FMT rot_dstfmtstr_to_imppfmt(const char *strfmt)
{
        assert(strfmt);

        if (0 == strcasecmp("BGRA8888", strfmt)) {
                return IMPP_PIX_FMT_BGRA_8888;
        } else if (0 == strcasecmp("RGB565", strfmt)) {
                return IMPP_PIX_FMT_RGB_565;
        } else if (0 == strcasecmp("RGB555", strfmt)) {
                return IMPP_PIX_FMT_RGB_555;
        } else if (0 == strcasecmp("YUYV", strfmt)) {
                return IMPP_PIX_FMT_YUYV;
        } else {
                printf("Dst fotmat not supported!\r\n");
                return -1;
        }
}

static int get_paramt(int argc, char *argv[])
{
        int ret = 0;
        if (argc < 2) {
                printf("please input paramt!!!\r\n");
                usage();
                exit(-1);
        }

        memset(&input_param, 0, sizeof(struct picture_info));
        input_param.process = "angle90";
        input_param.srcfmt = -1;
        input_param.dstfmt = -1;

        char optstring[] = "i:s:p:o:h";
        while ((ret = getopt_long(argc, argv, optstring, long_options, NULL)) != -1) {
                switch (ret) {
                case 'i':
                        input_param.srcfile = optarg;
                        break;
                case 's':
                        if (2 != sscanf(optarg, "%dx%d", &input_param.sWidth, &input_param.sHeight)) {
                                printf("The resolution format of the source image is incorrect!\r\n");
                                exit(-1);
                        }
                        break;
                case 0:
                        switch (flag) {
                        case SRC_FMT_FLAG:
                                input_param.srcfmt = rot_srcfmtstr_to_imppfmt(optarg);
                                if (-1 == input_param.srcfmt) {
                                        exit(-1);
                                }
                                break;
                        case DST_FMT_FLAG:
                                input_param.dstfmt = rot_dstfmtstr_to_imppfmt(optarg);
                                if (-1 == input_param.dstfmt) {
                                        exit(-1);
                                }
                                break;
                        }
                        break;
                case 'p':
                        input_param.process = optarg;
                        break;
                case 'o':
                        input_param.dstfile = optarg;
                        break;
                case 'h':
                        usage();
                        exit(EXIT_SUCCESS);
                case '?':
                        usage();
                        exit(-1);
                }
        }

        if (NULL == input_param.srcfile || 0 == input_param.sWidth || 0 == input_param.sHeight || \
            -1 == input_param.srcfmt || -1 == input_param.dstfmt || NULL == input_param.dstfile) {
                printf("Parameter input error!!!!\r\n");
                usage();
                exit(-1);
        }

        return 0;
}

int main(int argc, char **argv)
{
        int ret = 0;

        get_paramt(argc, argv);

        IHal_Rot_Handle_t *rot_handle;
        IHal_Rot_ChanAttr_t rot_chan_attr;
        memset(&rot_chan_attr, 0, sizeof(IHal_Rot_ChanAttr_t));
        rot_chan_attr.sWidth = input_param.sWidth;
        rot_chan_attr.sHeight = input_param.sHeight;
        rot_chan_attr.srcBufType = IMPP_INTERNAL_BUFFER;
        rot_chan_attr.dstBufType = IMPP_INTERNAL_BUFFER;
        rot_chan_attr.numSrcBuf = 1;
        rot_chan_attr.numDstBuf = 1;
        rot_chan_attr.srcFmt = input_param.srcfmt;
        rot_chan_attr.dstFmt = input_param.dstfmt;
        if ((IMPP_PIX_FMT_YUYV == rot_chan_attr.srcFmt && IMPP_PIX_FMT_YUYV != rot_chan_attr.dstFmt) || \
            (IMPP_PIX_FMT_YUYV != rot_chan_attr.srcFmt && IMPP_PIX_FMT_YUYV == rot_chan_attr.dstFmt)) {
                printf("The X2000 Rotator does not support conversion between YUV and RGB formats.\r\n");
                return -1;
        }
        rot_handle = IHal_Rot_CreateChan(&rot_chan_attr);
        if (!rot_handle) {
                printf("rot handle create error\r\n");
                return -1;
        }

        IMPP_BufferInfo_t rot_srcbuf;
        memset(&rot_srcbuf, 0, sizeof(rot_srcbuf));
        IHal_Rot_SrcDmaBufGet(rot_handle, &rot_srcbuf, 0);

        int srcfd = open(input_param.srcfile, O_RDWR);
        if (-1 == srcfd) {
                printf("Open srcfile failed!\r\n");
                goto destroy_rotchan;
        }

        int dstfd = open(input_param.dstfile, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (-1 == dstfd) {
                printf("Open dstfile failed!\r\n");
                goto close_srcfile;
        }

        if (IMPP_PIX_FMT_BGRA_8888 == rot_chan_attr.srcFmt || IMPP_PIX_FMT_BGR_888 == rot_chan_attr.srcFmt) {
                read(srcfd, (void *)rot_srcbuf.vaddr, input_param.sWidth * input_param.sHeight * 4);
        } else {
                read(srcfd, (void *)rot_srcbuf.vaddr, input_param.sWidth * input_param.sHeight * 2);
        }

        IMPP_FrameInfo_t rot_sframe;
        memset(&rot_sframe, 0, sizeof(rot_sframe));
        rot_sframe.fd = rot_srcbuf.fd;
        rot_sframe.index = rot_srcbuf.index;
        rot_sframe.size = rot_srcbuf.size;
        rot_sframe.vaddr = rot_srcbuf.vaddr;
        if (0 == strcasecmp("angle0", input_param.process)) {
                ret = IHal_Rot_ProcessFrame(rot_handle, &rot_sframe, ROT_PROCESS_ROTATE, ROT_ANGLE_0);
        } else if (0 == strcasecmp("angle90", input_param.process)) {
                ret = IHal_Rot_ProcessFrame(rot_handle, &rot_sframe, ROT_PROCESS_ROTATE, ROT_ANGLE_90);
        } else if (0 == strcasecmp("angle180", input_param.process)) {
                ret = IHal_Rot_ProcessFrame(rot_handle, &rot_sframe, ROT_PROCESS_ROTATE, ROT_ANGLE_180);
        } else if (0 == strcasecmp("angle270", input_param.process)) {
                ret = IHal_Rot_ProcessFrame(rot_handle, &rot_sframe, ROT_PROCESS_ROTATE, ROT_ANGLE_270);
        } else if (0 == strcasecmp("vflip", input_param.process)) {
                ret = IHal_Rot_ProcessFrame(rot_handle, &rot_sframe, ROT_PROCESS_FLIP, ROT_ANGLE_0);
        } else if (0 == strcasecmp("hflip", input_param.process)) {
                ret = IHal_Rot_ProcessFrame(rot_handle, &rot_sframe, ROT_PROCESS_MIRROR, ROT_ANGLE_0);
        } else {
                printf("The rotator processing method is incorrect!!!\r\n");
                goto close_dstfile;
        }
        if (ret) {
                printf("Process failed!!!!!!!!\r\n");
                goto close_dstfile;
        }

        IMPP_FrameInfo_t rot_dframe;
        memset(&rot_dframe, 0, sizeof(rot_dframe));
        IHal_Rot_GetFrame(rot_handle, &rot_dframe);
        lseek(dstfd, 0, SEEK_SET);
        write(dstfd, (void *)rot_dframe.vaddr, rot_dframe.size);
        IHal_Rot_ReleaseFrame(rot_handle, &rot_dframe);

        close(dstfd);
        close(srcfd);
        IHal_Rot_DestroyChan(rot_handle);

        return 0;

close_dstfile:
        close(dstfd);
close_srcfile:
        close(srcfd);
destroy_rotchan:
        IHal_Rot_DestroyChan(rot_handle);

        return -1;
}
