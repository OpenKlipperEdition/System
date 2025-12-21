#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <time.h>

#include <codec.h>
#include <display.h>
#include "bs.h"
#include "convert.h"


#define PACKET_LEN  512 * 1024
char packet[PACKET_LEN];


struct configInfo {
        int width;
        int height;
        char *srcfile;
        char *fb;
        int nbuffers;
};

static struct configInfo input_params;

IHal_CodecHandle_t *codec_handle = NULL;
IHal_SampleFB_Handle_t *fb0_handle = NULL;
pthread_t display_tid;
pthread_t render_tid;
int srcfd = -1;

static int buffer_dqueued[5] = { -1, -1, -1, -1, -1};

static struct option long_options[] = {
        { "pixel_size", required_argument, NULL, 's'},
        { "src_file  ", required_argument, NULL, 'i'},
        { "fb_device", required_argument, NULL, 'd'},
        { "num_buffers", required_argument, NULL, 'n'},
        { "help      ", no_argument,       NULL, 'h'},
        {NULL, 0, NULL, 0},
};

#define print_opt_help(opt_index, help_str)             \
        do {                                \
                        printf("   -%c or --%s          %s", (char)long_options[opt_index].val, long_options[opt_index].name, help_str); \
        } while (0)

static void usage()
{
        printf("\nh264dec-example usage:\n");
        print_opt_help(0, "Resolution size of H264 encoded code stream file. The format is WxH, for example: 1280x720.\n");
        print_opt_help(1, "File name and path of H264 encoded code stream file.\n");
        print_opt_help(2, "fb device name, eg: /dev/fb0.\n");
        print_opt_help(3, "num of buffers to use for display & decode., eg -n 3\n");
        print_opt_help(4, "show help.\n");
}

static int get_paramt(int argc, char *argv[])
{
        int ret = 0;
        memset(&input_params, 0, sizeof(struct configInfo));

        char optstring[] = "s:i:d:n:h";
        while ((ret = getopt_long(argc, argv, optstring, long_options, NULL)) != -1) {
                switch (ret) {
                case 's':
                        if (2 != sscanf(optarg, "%dx%d", &input_params.width, &input_params.height)) {
                                printf("The resolution format of the resolution size is incorrect!\r\n");
                                exit(-1);
                        }
                        break;
                case 'i':
                        input_params.srcfile = optarg;
                        break;
                case 'd':
                        input_params.fb = optarg;
                        break;
                case 'n':
                        input_params.nbuffers = atoi(optarg);
                        break;
                case 'h':
                        usage();
                        exit(0);
                case '?':
                default:
                        usage();
                        exit(-1);
                }
        }

        if (0 == input_params.width || 0 == input_params.height || NULL == input_params.srcfile) {
                printf("Please input paramt!!!\r\n");
                usage();
                exit(-1);
        }

        return 0;
}



typedef struct {
        unsigned char *stream;
        unsigned char type;
        unsigned int size;
} nal_unit_t;

#define MAX_NALS_PER_TIME       9000    //25fps@60min

typedef struct {
        nal_unit_t nal[MAX_NALS_PER_TIME];
        int nal_count;
} nal_units_t;

nal_units_t nal_units;


static int m_find_start_code(unsigned char *buf, unsigned int len, unsigned int *prefix_bytes)
{
        unsigned char *start = buf;
        unsigned char *end = buf + len;
        int index = 0;

        if (start >= end) {
                return -1;
        }

        while (start < end) {
#if 1
                /* next24bits */
                index = start - buf;

                if (start[0] == 0 && start[1] == 0 && start[2] == 1) {
                        *prefix_bytes = 3;
                        return index + 3;
                }

                start++;


                /* end of data buffer, no need to find. */
                if ((start + 3) == end) {
                        return -1;
                }
#endif
        }

        return -1;
}

static unsigned int get_nal_size(char *buf, unsigned int len)
{
        char *start = buf;
        char *end = buf + len;
        unsigned int size = 0;

        if (start >= end) {
                return -1;
        }

        while (start < end) {

                if (start[0] == 0 && start[1] == 0 && start[2] == 1) {
                        break;
                }

                start++;
        }

        size = (unsigned int)(start - buf);

        return size;
}

static unsigned long long get_time_ms(void)
{
        struct timeval tv;
        gettimeofday(&tv, NULL);

        return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static int calc_fps(void)
{

        static unsigned long long last_time = 0;
        static int framecount = 0;
        static int fps = 0;
        unsigned long long cur_time = get_time_ms();

        if (last_time == 0) {
                last_time = get_time_ms();
        }

        framecount ++;
        if (cur_time - last_time >= 1000) {
                fps = framecount;
                framecount = 0;
                last_time = cur_time;
                printf("fps:%d\n", fps);
        }

        return fps;
}


int extract_nal_units(nal_units_t *nals, unsigned char *buf, unsigned int len)
{
        int index = 0;
        int next_nal = 0;
        unsigned char *start = buf;
        unsigned char *end = buf + len;
        int size = 0;
        unsigned int prefix_bytes; /*000001/00000001*/
        nal_unit_t *nal;

        unsigned int left = len;
        int i = 0;

        int eos = 0;
        nals->nal_count = 0;

        for (i = 0; i < MAX_NALS_PER_TIME; i++) {

                nal = &nals->nal[i];

                index = m_find_start_code(start, left, &prefix_bytes);
                if (index < 0) {
                        printf("no start code found!\n");
                        return -1;
                }

                nal->stream = start;
                nal->type = nal->stream[index];

                nal->size = get_nal_size(start + index, left - index) + index;
                nals->nal_count++;

                start += nal->size;
                left -= nal->size;

                if (left <= 0) {
                        /*EOS*/
                        break;
                }

        }

        if (left) {
                printf("too many nals extraced!, %d bytes left\n", left);
        }

        return len - left;
}

/* 从NALS 中合并一个完整的Frame，将Frame中所有的slice 拼接到一起。*/
int  get_slice_frame(nal_unit_t *nals, int start, int nal_counts, unsigned char **slice_frame_buffer, unsigned int *slice_frame_size, int *got_frame)
{
        nal_unit_t *nal = NULL;
        int i = 0;

        int first_mb_in_slice = 0;
        int slice_type = 0;
        int new_frame = 0;
        bs_t current_nal_bs;

        *got_frame = 0;
        *slice_frame_size = 0;

        for (i = start; i < nal_counts; i++) {

                nal = &nal_units.nal[i];

                bs_init(&current_nal_bs, nal->stream + 4, nal->size - 4);


                if ((nal->type & 0x1f) == 0x5 || (nal->type & 0x1f) == 0x1) {
                        first_mb_in_slice = bs_read_ue(&current_nal_bs);
                        slice_type = bs_read_ue(&current_nal_bs);


                        if (first_mb_in_slice == 0) {

                                if (new_frame) {
                                        *got_frame = 1;
                                        break;
                                }
                                new_frame = 1;
                        }

                }

                if (*slice_frame_buffer == NULL) {
                        *slice_frame_buffer = nal->stream;
                }

                *slice_frame_size += nal->size;
        }

        return i;
}

int stopped = 0;

static int signal_handle(int sig)
{

        int ret = 0;

        stopped = 1;

        pthread_cancel(render_tid);
        pthread_join(render_tid, NULL);

        ret = IHal_Codec_Stop(codec_handle);
        assert(0 == ret);

        ret = IHal_CodecDestroy(codec_handle);
        assert(0 == ret);


        IHal_CodecDeInit();

        ret = close(srcfd);
        assert(0 == ret);


        exit(0);

        return 0;
}

void *render_thread(void *arg)
{

        /*
        	两种场景:
        	场景1： <适合软件绘图流程,后有数据>
        		1. WaitForVsync
        		2. GetNextBufferToRender
        		3. do software Render on NextBuffer
        		4. hw Render NextBuffer.

        	场景2：<适合Buffer共享流程，其他硬件先产生数据>

        		1. GetNextBufferToDisplay
        		2. hw Render NextBuffer <DPU>
        		3. WaitForVsync
        		4. GetNextBufferToRender
        		5. do some special render.
        		6. other hardware render.

        */

        int ret = 0;
        IHAL_CodecStreamInfo_t codec_dststream;
        IMPP_BufferInfo_t fb0_buf;
		int vsync = 0;
		struct tile420_fmt tile420_buf;
		struct nv12_fmt nv12_buf;

        while (!stopped) {
                ret = IHal_Codec_WaitDstAvailable(codec_handle, IMPP_WAIT_FOREVER);
                // ret = IHal_Codec_WaitDstAvailable(codec_handle, IMPP_NO_WAIT);
                assert(0 == ret);

                ret = IHal_Codec_DequeueDstBuffer(codec_handle, &codec_dststream);
                assert(0 == ret);

				clock_t start = clock();
				ret = IHal_SampleFB_WaitForVsync(fb0_handle, &vsync);
				assert(0 == ret);
				clock_t end = clock();
				double elapse = (double)(end - start) / CLOCKS_PER_SEC * 1000;

				// 填充新数据.
				IHal_SampleFB_GetMem(fb0_handle, &fb0_buf);
				long s = random();

				tile420_buf.width = input_params.width;
				tile420_buf.stride = input_params.width;
				tile420_buf.height = input_params.height;
				tile420_buf.y = codec_dststream.mp.vaddr[0];
				tile420_buf.uv = codec_dststream.mp.vaddr[1];
				nv12_buf.width = input_params.width;
				nv12_buf.stride = input_params.width;
				nv12_buf.height = input_params.height;
				nv12_buf.d = fb0_buf.vaddr;
				IHal_Convert_Tile420ToNV12(tile420_buf, &nv12_buf);

                IHal_SampleFB_Update(fb0_handle, &fb0_buf);

				calc_fps();

				ret = IHal_Codec_QueueDstBuffer(codec_handle, &codec_dststream);
				assert(0 == ret);
        }

        pthread_exit(NULL);
}

int main(int argc, char **argv)
{
        int ret = 0;
        IHal_CodecParam h264dec_param;
        int srcbuf_nums = 0;
        int dstbuf_nums = 0;
        IHal_SampleFB_Attr fb_attr;
        IMPP_BufferInfo_t fb0_buf;
        unsigned long filesize = 0;

        get_paramt(argc, argv);
        
	memset(&fb_attr, 0, sizeof(IHal_SampleFB_Attr));
        //sprintf(&fb_attr.node[0], "%s", "/dev/fb0");
        sprintf(&fb_attr.node[0], "%s", input_params.fb);
        fb_attr.mode = Composer_Mode;
        fb_attr.frame_width = input_params.width;
        fb_attr.frame_height = input_params.height;
        fb_attr.crop_x = 0;
        fb_attr.crop_y = 0;
        fb_attr.crop_w = 0;
        fb_attr.crop_h = 0;
        fb_attr.alpha = 255;
        fb_attr.frame_fmt    = IMPP_PIX_FMT_NV12;
        fb0_handle = IHal_SampleFB_Init(&fb_attr);
        assert(NULL != fb0_handle);

	input_params.nbuffers = IHal_SampleFB_GetBufferNumbers(fb0_handle);

        //IHal_SampleFB_SetSrcFrameSize(fb0_handle, input_params.width, input_params.height);
        //IHal_SampleFB_SetTargetFrameSize(fb0_handle, 480, 640);
        //IHal_SampleFB_SetTargetPos(fb0_handle, 0, 0);
        //IHal_SampleFB_SetZorder(fb0_handle, Order_3);

        // 动态调整的参数需要配合IHal_SampleFB_CompRestart()更新配置.
        //IHal_SampleFB_CompRestart(fb0_handle);

        ret = IHal_CodecInit();
        assert(0 == ret);

        codec_handle = IHal_CodecCreate(H264_DEC);
        assert(0 != codec_handle);

        memset(&h264dec_param, 0, sizeof(IHal_CodecParam));
        h264dec_param.codec_type = H264_DEC;
        // h264dec_param.codecparam.h264d_param.need_split = ;
        h264dec_param.codecparam.h264d_param.src_width = input_params.width;          /* 源图像宽度 */
        h264dec_param.codecparam.h264d_param.src_height = input_params.height;        /* 源图像高度 */
		h264dec_param.codecparam.h264d_param.dst_fmt = IMPP_PIX_FMT_JZ420B;             /* 目标像素格式 */
        ret = IHal_Codec_SetParams(codec_handle, &h264dec_param);
        assert(0 == ret);

        srcbuf_nums = IHal_Codec_CreateSrcBuffers(codec_handle, IMPP_INTERNAL_BUFFER, input_params.nbuffers);
        assert(1 <= srcbuf_nums);

        dstbuf_nums = IHal_Codec_CreateDstBuffer(codec_handle, IMPP_INTERNAL_BUFFER, input_params.nbuffers);
        assert(1 <= dstbuf_nums);


        ret = IHal_Codec_Start(codec_handle);
        assert(0 == ret);

        srcfd = open(input_params.srcfile, O_RDONLY);
        assert(-1 != srcfd);

        /* int dstfd = -1; */
        /* dstfd = open("out_test.nv12", O_RDWR | O_CREAT | O_TRUNC, 0666); */
        /* assert(-1 != dstfd); */

        filesize = lseek(srcfd, 0, SEEK_END);
        lseek(srcfd, 0, SEEK_SET);

        int index = 0;
        int packet_len = 0;
        nal_unit_t *nal = NULL;
        nal_unit_t *next_nal = NULL;
        nal_unit_t *last_nal = NULL;
        bs_t current_nal_bs;
        bs_t next_nal_bs;
        unsigned long current_offset = 0;
        int merge_sps_pps = 0;
        char *slice_frame_buffer = NULL;// malloc(1024 * 1024 *2);
        unsigned long slice_frame_size = 0;
        char *p_slice_frame_buffer = slice_frame_buffer;
        int eof = 0;
        IMPP_BufferInfo_t codec_srcbuf;
        int nal_count = 0;
        int got_frame = 0;
        int start = 0;


        signal(SIGINT, signal_handle);

//	pthread_create(&render_tid, NULL, render_thread, (void *)codec_handle);
        pthread_create(&render_tid, NULL, render_thread, (void *)fb0_handle);

        while (1) {
                memset(&codec_srcbuf, 0, sizeof(IMPP_BufferInfo_t));

                packet_len = read(srcfd, packet, PACKET_LEN);
                if (packet_len <= 0) {
                        // lseek(srcfd,0,SEEK_SET);
                        break;
                }

                nal_units.nal_count = 0;
                ret = extract_nal_units(&nal_units, packet, packet_len);

                if (ret < packet_len) {
                        printf("packet not extracted completely!\n");
                }

                nal_count = 0;

                last_nal = &nal_units.nal[nal_units.nal_count - 1];
                current_offset = lseek(srcfd, 0, SEEK_CUR);

                if (current_offset < filesize) {
                        nal_count = nal_units.nal_count - 1;
                        lseek(srcfd, -last_nal->size, SEEK_CUR);
                } else {
                        /*EOF.*/
                        nal_count = nal_units.nal_count;
                        eof = 1;
                        printf("----EOF----!\n");
                }

                got_frame = 0;
                start = 0;

                do {
                        slice_frame_buffer = NULL;
                        ret = get_slice_frame(&nal_units, start, nal_count, &slice_frame_buffer, &slice_frame_size, &got_frame);
                        start = ret;

                        if (ret == nal_count) {
                                if (!got_frame && eof == 0) {
                                        /* reseek to last nal decoded.*/
                                        lseek(srcfd, -slice_frame_size, SEEK_CUR);
                                }

                                break;
                        }


                        /* decode nal units */
                        ret = IHal_Codec_GetSrcBuffer(codec_handle, index, &codec_srcbuf);
                        assert(0 == ret);

                        for (int i = 0; i < codec_srcbuf.mplane.numPlanes; i++) {
                                memcpy(codec_srcbuf.mplane.vaddr[i], slice_frame_buffer, slice_frame_size);
                        }
                        codec_srcbuf.byteused = slice_frame_size;

                        ret = IHal_Codec_QueueSrcBuffer(codec_handle, &codec_srcbuf);
                        assert(0 == ret);


                        ret = IHal_Codec_WaitSrcAvailable(codec_handle, IMPP_WAIT_FOREVER);
                        assert(0 == ret);

                        ret = IHal_Codec_DequeueSrcBuffer(codec_handle, &codec_srcbuf);
                        assert(0 == ret);
                } while (1);
        }

exit_loop:

        /* close(dstfd); */

        ret = close(srcfd);
        assert(0 == ret);

        stopped = 1;
        pthread_cancel(render_tid);
        pthread_join(render_tid, NULL);


        ret = IHal_Codec_Stop(codec_handle);
        assert(0 == ret);

        ret = IHal_CodecDestroy(codec_handle);
        assert(0 == ret);


        IHal_CodecDeInit();

        return 0;
}
