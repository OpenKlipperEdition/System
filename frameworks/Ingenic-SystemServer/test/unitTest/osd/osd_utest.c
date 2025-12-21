#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "dmabuf_allocator.h"
#include "systemserver.h"
#include "binder_common.h"
#include "binder_ipc.h"
#include "binder_io.h"

#undef CU_TRUE
#undef CU_FALSE
#define CU_TRUE 0
#define CU_FALSE 1
#include <assert.h>
#include <CUnit/CUnit.h>
#include <CUnit/Automated.h>
#include <CUnit/Basic.h>
#include <CUnit/Console.h>

enum ClientType
{
    OTHER = 1,
    UI,
    VIDEO,
};

enum
{
    REGISTER_CLIENT = 1,
    UNREGISTER_CLIENT,
    ALLOC_DISPMEM,
    FREE_DISPMEM,
    FLUSH_LAYER_DATA,
    PARSE_IMAGEATTR,
    PARSE_DISPLAYATTR,
    SET_FRAME_RATE,
};

enum
{
    SUCCESS = 0,
    NO_HANDLER,
    NO_USABLE_LAYER,
    NO_AVAILABLE_PRIORITY,
    NO_CLIENT,
    NO_IPCMEM,
    NO_HEAPMEM,
    BUF_INFO_ERROR,
    PARAM_ERROR,
    MISSING_NECESSARY_PARAMETERS,
    NOT_SUPPORT_FRAME_RATE,
};

/**
 * @brief 图像格式
 */
typedef enum {
        PIX_FMT_NV12,
        PIX_FMT_NV21,
        PIX_FMT_NV16,
        PIX_FMT_YUV422,
        PIX_FMT_YUV422P,
        PIX_FMT_I420,
        PIX_FMT_I422,
        PIX_FMT_YUV420p,
        PIX_FMT_RAW8,
        PIX_FMT_RAW10,
        PIX_FMT_RGBA_8888,
        PIX_FMT_RGBX_8888,
        PIX_FMT_BGRA_8888,
        PIX_FMT_BGRX_8888,
        PIX_FMT_ABGR_8888,
        PIX_FMT_ARGB_8888,
        PIX_FMT_RGB_888,
        PIX_FMT_BGR_888,
        PIX_FMT_RGB_565,
        PIX_FMT_RGB_555,
        PIX_FMT_RGBA_5551,
        PIX_FMT_BGRA_5551,
        PIX_FMT_ARGB_1555,
        PIX_FMT_HSV,

        /* Camera Raw8 */
        PIX_FMT_SBGGR8,
        PIX_FMT_SGBRG8,
        PIX_FMT_SGRBG8,
        PIX_FMT_SRGGB8,

        /* Camera Raw10 */
        PIX_FMT_SBGGR10,
        PIX_FMT_SGBRG10,
        PIX_FMT_SGRBG10,
        PIX_FMT_SRGGB10,

        /* Camera Raw12 */
        PIX_FMT_SBGGR12,
        PIX_FMT_SGBRG12,
        PIX_FMT_SGRBG12,
        PIX_FMT_SRGGB12,

        /* Y only.*/
        PIX_FMT_GREY,
        PIX_FMT_Y4,
        PIX_FMT_Y6,
        PIX_FMT_Y12,
        PIX_FMT_Y16,

        /* YUV422. */
        PIX_FMT_YUYV,
        PIX_FMT_YYUV,
        PIX_FMT_YVYU,
        PIX_FMT_VYUY,

        /* YUV444. */
        PIX_FMT_YUV444,

		PIX_FMT_MJPEG,

		/* TILE420 */
		PIX_FMT_JZ420B,

}PixFmt_t;

/**
 * @brief 源图像信息参数
 */
struct ImageAttr
{
    PixFmt_t imageFmt;    /*<! 图像格式 NV12 RGB888.... >*/
    int32_t imageWidth;   /*<! 图像宽度 >*/
    int32_t imageHeight;  /*<! 图像高度 >*/
};

/**
 * @brief 要显示上屏的参数
 */
struct DisplayAttr
{
    int32_t scaleWidth;   /*<! 缩放宽度 >*/
    int32_t scaleHeight;  /*<! 缩放高度 >*/
    int32_t posX;         /*<! 显示到屏上的x轴坐标 >*/
    int32_t posY;         /*<! 显示到屏上的y轴坐标 >*/
    int32_t alpha;        /*<! 不透明度 >*/
};

struct MemDesc
{
    int32_t IpcDmaFd;   // dma-buf fd
    int32_t MemSize;
    void *IpcMemPaddr;   // dma-buf 的物理地址（可能用不到）
    void *IpcMemVaddr;   // dma-buf在本进程空间的虚拟地址
};

struct OsdClient
{
    int32_t ServerHandler;  // 服务端handler引用
    enum ClientType Type;

    struct ImageAttr imageAttr;

    struct binder_death *DeathInfo;

    int32_t IpcDmaFd;   // dma-buf fd
    int32_t MemSize;
    int32_t MemDescNum;
    struct MemDesc desc[1];
};

static int32_t recevied_msg_cb(uint32_t code, tBinderIo* msg, tBinderIo* reply, uint32_t flag)
{

    return 0;
}

tBinderService osd_client_cb = {
    .transact_cb = recevied_msg_cb,
    .link_to_death_cb = NULL,
    .unlink_to_death_cb = NULL,
    .death_notify_cb = NULL,
};

int32_t msg_proxy(int32_t handler, tBinderIo *bio, tBinderIo *reply, int32_t cmd)
{
    int32_t ret = 0;
    int32_t remote_reply = 0;
    tIpcThreadInfo * ti = binder_get_thread_info();

    ret = binder_cmd_sync_call(ti, bio, reply, handler, 0);
    if (BINDER_STATUS_OK != ret)
    {
        return -1;
    }

    remote_reply = (int32_t)binder_io_get_uint32(reply);
    if (SUCCESS != remote_reply)
    {
        return -2;
    }

    binder_cmd_freebuf(ti, reply->data0);
    return 0;
}

struct OsdClient *cUnit_CreateOsdClt(enum ClientType type, pid_t pid)
{
    int32_t ret = 0;
    int32_t remote_reply = 0;
    int32_t handler = 0;
    tBinderIo bio, reply;
    struct binder_death *dead_cb = NULL;
    struct OsdClient *clientInfo = NULL;
    tIpcThreadInfo * ti = binder_get_thread_info();
    char binder_buf[DEFAULT_BINDER_IOBUF_SIZE] = {0};

    clientInfo = malloc(sizeof(struct OsdClient));
    if (clientInfo == NULL)
    {
        goto no_mem;
    }

    handler = binder_get_service(OSD_SERVICE_NAME);
    if (handler == 0)
    {
        goto no_service;
    }

    memset(&bio, 0, sizeof(tBinderIo));
    memset(&reply, 0, sizeof(tBinderIo));
    binder_io_init(&bio, binder_buf, sizeof(binder_buf), DEFAULT_OFFSET_LIST_SIZE);

    binder_io_append_uint32(&bio, pid);
    binder_io_append_uint32(&bio, (uint32_t)REGISTER_CLIENT);
    binder_io_append_uint32(&bio, (uint32_t)type);
    binder_io_append_obj(&bio, &osd_client_cb);

    ret = msg_proxy(handler, &bio, &reply, REGISTER_CLIENT);
    if (ret != 0)
    {
        goto register_client_failed;
    }

    clientInfo->ServerHandler = handler;
    clientInfo->Type = type;

    return clientInfo;
no_heapmem:
register_client_failed:
no_service:
    free(clientInfo);
no_mem:
    return NULL;

}

int32_t cUnit_DestoryOsdClt(struct OsdClient *clt, pid_t pid)
{
    int32_t ret = 0;
    tBinderIo bio, reply;
    tIpcThreadInfo * ti = binder_get_thread_info();
    char binder_buf[DEFAULT_BINDER_IOBUF_SIZE] = {0};

    memset(&bio, 0, sizeof(tBinderIo));
    memset(&reply, 0, sizeof(tBinderIo));
    binder_io_init(&bio, binder_buf, sizeof(binder_buf), DEFAULT_OFFSET_LIST_SIZE);

    binder_io_append_uint32(&bio, pid);
    binder_io_append_uint32(&bio, (uint32_t)UNREGISTER_CLIENT);
    binder_io_append_uint32(&bio, (uint32_t)clt->Type);

    ret = msg_proxy(clt->ServerHandler, &bio, &reply, UNREGISTER_CLIENT);
    if (ret != 0)
    {
        return -3;
    }

//    free_Mem(clt);

    free(clt);

    return 0;

}

int32_t cUnit_AllocIpcMem(struct OsdClient *clt, int32_t size, uint32_t nmemb, pid_t pid)
{
    int32_t ret = 0;
    int32_t dmabuf_Fd = 0;
    void *ptr = NULL;
    tBinderIo bio, reply;
    char binder_buf[DEFAULT_BINDER_IOBUF_SIZE] = {0};

    if (clt == NULL)
    {
        return -1;
    }

    memset(&bio, 0, sizeof(tBinderIo));
    memset(&reply, 0, sizeof(tBinderIo));
    binder_io_init(&bio, binder_buf, sizeof(binder_buf), DEFAULT_OFFSET_LIST_SIZE);

    binder_io_append_uint32(&bio, pid);
    binder_io_append_uint32(&bio, (uint32_t)ALLOC_DISPMEM);
    binder_io_append_uint32(&bio, (uint32_t)clt->Type);
    binder_io_append_uint32(&bio, (uint32_t)size);
    binder_io_append_uint32(&bio, (uint32_t)nmemb);

    ret = msg_proxy(clt->ServerHandler, &bio, &reply, ALLOC_DISPMEM);
    if (ret != 0)
    {
        return -3;
    }

    dmabuf_Fd = binder_io_get_fd(&reply, 0);
    if (dmabuf_Fd == 0)
    {
        return -3;
    }

    ptr = mmap(NULL, size * nmemb, PROT_WRITE, MAP_SHARED, dmabuf_Fd, 0);
    if (ptr == MAP_FAILED)
    {
        return -2;
    }

    clt->desc[0].IpcDmaFd = dmabuf_Fd;
    clt->desc[0].MemSize = size * nmemb;
    clt->desc[0].IpcMemVaddr = ptr;
    clt->desc[0].IpcMemPaddr = NULL;

    return 0;
}

int32_t cUnit_FreeIpcMem(struct OsdClient *clt, pid_t pid)
{
    int32_t ret = 0;
    tBinderIo bio, reply;
    tIpcThreadInfo * ti = binder_get_thread_info();
    char binder_buf[DEFAULT_BINDER_IOBUF_SIZE] = {0};

    memset(&bio, 0, sizeof(tBinderIo));
    memset(&reply, 0, sizeof(tBinderIo));
    binder_io_init(&bio, binder_buf, sizeof(binder_buf), DEFAULT_OFFSET_LIST_SIZE);

    binder_io_append_uint32(&bio, pid);
    binder_io_append_uint32(&bio, (uint32_t)FREE_DISPMEM);
    binder_io_append_uint32(&bio, (uint32_t)clt->Type);

    ret = msg_proxy(clt->ServerHandler, &bio, &reply, FREE_DISPMEM);
    if (ret != 0)
    {
        return -3;
    }

    munmap(clt->desc[0].IpcMemVaddr, clt->desc[0].MemSize);
    close(clt->IpcDmaFd);

    clt->desc[0].IpcDmaFd = 0;
    clt->desc[0].MemSize = 0;
    clt->desc[0].IpcMemPaddr = 0;
    clt->desc[0].IpcMemVaddr = 0;

    return 0;
}

int32_t cUnit_SetImageAttr(struct OsdClient *clt, struct ImageAttr attr, pid_t pid)
{
    int32_t ret = 0;
    tBinderIo bio, reply;
    tIpcThreadInfo * ti = binder_get_thread_info();
    char binder_buf[DEFAULT_BINDER_IOBUF_SIZE] = {0};

    memset(&bio, 0, sizeof(tBinderIo));
    memset(&reply, 0, sizeof(tBinderIo));
    binder_io_init(&bio, binder_buf, sizeof(binder_buf), DEFAULT_OFFSET_LIST_SIZE);

    binder_io_append_uint32(&bio, pid);
    binder_io_append_uint32(&bio, (uint32_t)PARSE_IMAGEATTR);
    binder_io_append_uint32(&bio, (uint32_t)clt->Type);
    binder_io_append_data(&bio, &attr, sizeof(struct ImageAttr));

    ret = msg_proxy(clt->ServerHandler, &bio, &reply, PARSE_IMAGEATTR);
    if (ret != 0)
    {
        return -3;
    }

    return 0;
}

int32_t cUnit_SetDisplayAttr(struct OsdClient *clt, struct DisplayAttr attr, pid_t pid)
{
    int32_t ret = 0;
    tBinderIo bio, reply;
    tIpcThreadInfo * ti = binder_get_thread_info();
    char binder_buf[DEFAULT_BINDER_IOBUF_SIZE] = {0};

    memset(&bio, 0, sizeof(tBinderIo));
    memset(&reply, 0, sizeof(tBinderIo));
    binder_io_init(&bio, binder_buf, sizeof(binder_buf), DEFAULT_OFFSET_LIST_SIZE);

    binder_io_append_uint32(&bio, pid);
    binder_io_append_uint32(&bio, (uint32_t)PARSE_DISPLAYATTR);
    binder_io_append_uint32(&bio, (uint32_t)clt->Type);
    binder_io_append_data(&bio, &attr, sizeof(struct DisplayAttr));

    ret = msg_proxy(clt->ServerHandler, &bio, &reply, PARSE_DISPLAYATTR);
    if (ret != 0)
    {
        return -3;
    }

    return 0;
}

int32_t cUnit_SetFrameRate(struct OsdClient *clt, int32_t frame_rate, pid_t pid)
{
    int32_t ret = 0;
    tBinderIo bio, reply;
    tIpcThreadInfo * ti = binder_get_thread_info();
    char binder_buf[DEFAULT_BINDER_IOBUF_SIZE] = {0};

    memset(&bio, 0, sizeof(tBinderIo));
    memset(&reply, 0, sizeof(tBinderIo));
    binder_io_init(&bio, binder_buf, sizeof(binder_buf), DEFAULT_OFFSET_LIST_SIZE);

    binder_io_append_uint32(&bio, pid);
    binder_io_append_uint32(&bio, (uint32_t)SET_FRAME_RATE);
    binder_io_append_uint32(&bio, (uint32_t)clt->Type);
    binder_io_append_uint32(&bio, (uint32_t)frame_rate);

    ret = msg_proxy(clt->ServerHandler, &bio, &reply, SET_FRAME_RATE);
    if (ret != 0)
    {
        return -3;
    }

    return 0;
}

int32_t cUnit_Flush(struct OsdClient *clt, void *addr, int32_t size, pid_t pid)
{
    int32_t ret = 0;
    tBinderIo bio, reply;
    tIpcThreadInfo * ti = binder_get_thread_info();
    char binder_buf[DEFAULT_BINDER_IOBUF_SIZE] = {0};
    struct dma_buf_sync sync = {
        .flags = DMA_BUF_SYNC_RW,
    };

    memset(&bio, 0, sizeof(tBinderIo));
    memset(&reply, 0, sizeof(tBinderIo));
    binder_io_init(&bio, binder_buf, sizeof(binder_buf), DEFAULT_OFFSET_LIST_SIZE);

    binder_io_append_uint32(&bio, pid);
    binder_io_append_uint32(&bio, (uint32_t)FLUSH_LAYER_DATA);
    binder_io_append_uint32(&bio, (uint32_t)clt->Type);
    binder_io_append_uint32(&bio, (uint32_t)size);

    sync.flags |= MEM_FLUSH_START;
    ioctl(clt->desc[0].IpcDmaFd, DMA_BUF_IOCTL_SYNC, &sync);
    memcpy(clt->desc[0].IpcMemVaddr, addr, size);
    sync.flags = DMA_BUF_SYNC_RW;
    sync.flags |= MEM_FLUSH_END;
    ioctl(clt->desc[0].IpcDmaFd, DMA_BUF_IOCTL_SYNC, &sync);

    binder_io_append_uint32(&bio, (uint32_t)0);
    ret = msg_proxy(clt->ServerHandler, &bio, &reply, FLUSH_LAYER_DATA);
    if (ret != 0)
    {
        return -3;
    }

    return 0;
}

struct OsdClient *g_clt = NULL;
void TEST_SERVER_REGISTER_API_000(void)
{
	struct OsdClient *clt = cUnit_CreateOsdClt(OTHER, 1);
	CU_ASSERT_PTR_NOT_NULL_FATAL(clt);

	g_clt = clt;
}


void TEST_SERVER_ALLOC_DISPMEM_API_001(void)
{
	int ret = 0;
	ret = cUnit_AllocIpcMem(g_clt, 720 * 1280 * 4, 1, 1);
	CU_ASSERT_EQUAL_FATAL(ret, 0);
}

void TEST_SERVER_SET_IMAGEATTR_API_002(void)
{
	int ret = 0;
	struct ImageAttr attr;

	attr.imageWidth = 720;
	attr.imageHeight = 1280;
	attr.imageFmt = PIX_FMT_RGB_565;

	ret = cUnit_SetImageAttr(g_clt, attr, 1);
	CU_ASSERT_EQUAL_FATAL(ret, 0);
}

void TEST_SERVER_SET_DISPATTR_API_003(void)
{
	int ret = 0;
	struct DisplayAttr attr;

	attr.alpha = 255;
	attr.scaleWidth = 320;
	attr.scaleHeight = 640;
	attr.posX = 50;
	attr.posY = 50;

	ret = cUnit_SetDisplayAttr(g_clt, attr, 1);
	CU_ASSERT_EQUAL_FATAL(ret, 0);
}

void TEST_SERVER_SET_FRAMERATE_API_004(void)
{
	int ret = 0;

	ret = cUnit_SetFrameRate(g_clt, 60, 1);
	CU_ASSERT_EQUAL_FATAL(ret, 0);
}

void TEST_SERVER_FLUSH_API_005(void)
{
	int ret = 0;
	char frame[720 * 1280 * 4] = { 0 };

	ret = cUnit_Flush(g_clt, frame, 720 * 1280 * 4, 1);
	CU_ASSERT_EQUAL_FATAL(ret, 0);
}

void TEST_SERVER_FREE_DISPMEM_API_006(void)
{
	int ret = 0;

	ret = cUnit_FreeIpcMem(g_clt, 1);
	CU_ASSERT_EQUAL_FATAL(ret, 0);
}

void TEST_SERVER_UNREGISTER_API_007(void)
{
	int ret = 0;

	ret = cUnit_DestoryOsdClt(g_clt, 1);
	CU_ASSERT_EQUAL_FATAL(ret, 0);
}

CU_TestInfo osd_api_test[] = {
	{"osd service basic api register clt test", TEST_SERVER_REGISTER_API_000},
	{"osd service basic api alloc ipcmem test", TEST_SERVER_ALLOC_DISPMEM_API_001},
	{"osd service basic api set imageAttr test", TEST_SERVER_SET_IMAGEATTR_API_002},
	{"osd service basic api set dispAttr test", TEST_SERVER_SET_DISPATTR_API_003},
	{"osd service basic api set frameRate test", TEST_SERVER_SET_FRAMERATE_API_004},
	{"osd service basic api flush test", TEST_SERVER_FLUSH_API_005},
	{"osd service basic api free ipcmem test", TEST_SERVER_FREE_DISPMEM_API_006},
	{"osd service basic api unregister clt test", TEST_SERVER_UNREGISTER_API_007},
	CU_TEST_INFO_NULL,
};

CU_SuiteInfo osd_test_suite[] = {
	{"Osd Service Basic API Function Testing", NULL, NULL, NULL, NULL, osd_api_test},
	CU_TEST_INFO_NULL,
};

static void add_test(void)
{
    assert(NULL != CU_get_registry());
    assert(!CU_is_test_running());
    if(CUE_SUCCESS != CU_register_suites(osd_test_suite))
    {
        exit(-1);
    }
}

static int run_unit_test(void)
{
    if(CU_initialize_registry())
    {
        printf("initialize CU failed\r\n");
        exit(-1);
    }
    else
    {
        add_test();

        // console mode
        CU_console_run_tests();

        CU_cleanup_registry();
        return CU_get_error();
    }
}

int main(int argc,char** argv)
{
    run_unit_test();
    return 0;
}

