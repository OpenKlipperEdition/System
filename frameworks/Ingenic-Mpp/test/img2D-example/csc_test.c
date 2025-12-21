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
#include <ipu.h>

struct csc_configInfo {
        char *srcfile;
        unsigned int sWidth;
        unsigned int sHeight;
        IMPP_PIX_FMT dstfmt;
        char *dstfile;
};

static struct option long_options[] = {
        { "src_file", required_argument, NULL, 'i'},
        { "pic_size", required_argument, NULL, 's'},
        { "dst_fmt ", required_argument, NULL, 'f'},
        { "dst_file", required_argument, NULL, 'o'},
        { "help    ", no_argument,       NULL, 'h'},
};

#define print_opt_help(opt_index, help_str)                             \
        do {                                                            \
                        printf("   -%c or --%s          %s", (char)long_options[opt_index].val, long_options[opt_index].name, help_str); \
        } while (0)

static void usage()
{
        printf("\ncsc-example usage:\n");
        print_opt_help(0, "NV12 format source file for color space conversion.\n");
        print_opt_help(1, "The resolution of the source file. The format is WxH, for example: 720x1280.\n");
        print_opt_help(2, "Target Format. The default value is RGBA8888.\n");
        print_opt_help(3, "The output file. The default is csc_out.rgba.\n");
        print_opt_help(4, "Help.\n");
}

static IHal_CSC_Handle_t *handle;
static struct csc_configInfo input_param;

static IMPP_PIX_FMT csc_str_to_imppfmt(const char *strfmt)
{
        assert(strfmt);

        if (0 == strcasecmp("ABGR8888", optarg)) {
                return IMPP_PIX_FMT_ABGR_8888;
        } else if (0 == strcasecmp("ARGB8888", optarg)) {
                return IMPP_PIX_FMT_ARGB_8888;
        } else if (0 == strcasecmp("BGRA8888", optarg)) {
                return IMPP_PIX_FMT_BGRA_8888;
        } else if (0 == strcasecmp("BGRX8888", optarg)) {
                return IMPP_PIX_FMT_BGRX_8888;
        } else if (0 == strcasecmp("HSV", optarg)) {
                return IMPP_PIX_FMT_HSV;
        } else if (0 == strcasecmp("NV12", optarg)) {
                return IMPP_PIX_FMT_NV12;
        } else if (0 == strcasecmp("NV21", optarg)) {
                return IMPP_PIX_FMT_NV21;
        } else if (0 == strcasecmp("RGBA8888", optarg)) {
                return IMPP_PIX_FMT_RGBA_8888;
        } else if (0 == strcasecmp("RGBX8888", optarg)) {
                return IMPP_PIX_FMT_RGBX_8888;
        } else {
                printf("Format not supported!\r\n");
                return -1;
        }
}

static int get_paramt(int argc, char *argv[])
{
        int ret = 0;
        if (argc < 2) {
                printf("please input paramt!!!\n");
                usage();
                exit(-1);
        }

        memset(&input_param, 0, sizeof(struct csc_configInfo));
        input_param.dstfmt = IMPP_PIX_FMT_RGBA_8888;
        input_param.dstfile = "csc_out.rgba";

        char optstring[] = "i:s:f:o:h";
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
                case 'f':
                        input_param.dstfmt = csc_str_to_imppfmt(optarg);
                        if (-1 == input_param.dstfmt) {
                                exit(-1);
                        }
                        break;
                case 'o':
                        input_param.dstfile = optarg;
                        break;
                case 'h':
                        usage();
                        exit(0);
                case '?':
                        usage();
                        exit(-1);
                }
        }

        if (NULL == input_param.srcfile || 0 == input_param.sWidth || 0 == input_param.sHeight) {
                usage();
                exit(-1);
        }

        return 0;
}

int main(int argc, char **argv)
{
        int ret = 0;
        get_paramt(argc, argv);

        IHal_CSC_ChanAttr_t attr;
        memset(&attr, 0, sizeof(IHal_CSC_ChanAttr_t));
        attr.sWidth = input_param.sWidth;
        attr.sHeight = input_param.sHeight;
        attr.isSrcUseExtDmaBuf = 0;
        attr.isDstUseExtDmaBuf = 0;
        attr.numSrcBuf = 1;
        attr.numDstBuf = 1;
        attr.srcFmt = IMPP_PIX_FMT_NV12;
        attr.dstFmt = input_param.dstfmt;

        handle =  IHal_CSC_ChanCreate(&attr);
        if (!handle) {
                printf("csc channel create failed\r\n");
                return -1;
        }

        int srcfd = open(input_param.srcfile, O_RDWR);
        int dstfd = open(input_param.dstfile, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (-1 == srcfd || -1 == dstfd) {
                printf("open file faield \r\n");
                goto free_ch;
        }

        IMPP_BufferInfo_t srcbuf;
        memset(&srcbuf, 0, sizeof(IMPP_BufferInfo_t));
        IHal_CSC_GetSrcBuf(handle, &srcbuf, 0);
        read(srcfd, (void *)srcbuf.vaddr, input_param.sWidth * input_param.sHeight * 3 / 2);

        IPU_DmaBuf_SyncInfo_t syncinfo;
        memset(&syncinfo, 0, sizeof(syncinfo));
        syncinfo.fd = srcbuf.fd;
        syncinfo.sync_type = DMA_SYNC_FOR_WRITE;
        IHal_CSC_FlushCache(handle, &syncinfo);

        IMPP_FrameInfo_t sframe;
        memset(&sframe, 0, sizeof(IMPP_FrameInfo_t));
        sframe.fd = srcbuf.fd;
        sframe.vaddr = srcbuf.vaddr;
        sframe.paddr = srcbuf.paddr;
        sframe.size = srcbuf.size;
        sframe.fmt = IMPP_PIX_FMT_NV12;

        IMPP_FrameInfo_t dframe;
        memset(&dframe, 0, sizeof(IMPP_FrameInfo_t));
        ret = IHal_CSC_FrameProcess(handle, &sframe);
        if (ret) {
                printf("csc process failed\r\n");
                goto close_file;
        }
        printf("process ok !!!!!\r\n");
        ret = IHal_CSC_GetDstFrame(handle, &dframe, IMPP_WAIT_FOREVER);
        if (ret) {
                printf("get csc dst failed\r\n");
                goto close_file;
        }
        printf("csc dst frame size = %d \r\n", dframe.size);
        write(dstfd, (void *)dframe.vaddr, dframe.size);
        IHal_CSC_ReleaseDstFrame(handle, &dframe);

        close(srcfd);
        close(dstfd);
        IHal_CSC_DestroyChan(handle);

        return 0;

close_file:
        close(srcfd);
        close(dstfd);
free_ch:
        IHal_CSC_DestroyChan(handle);

        return -1;
}
