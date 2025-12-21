#include "binder_common.h"
#include "binder_ipc.h"
#include "binder_io.h"
#include <sys/time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdbool.h>
//#define DEBUG_ENABLE
#include "list.h"
#include "dlog.h"
#include "impp.h"
#include "impp_fifo.h"
#include "dmabuf_allocator.h"
#include "sys_common.h"
#include "systemserver.h"

#define LOG_TAG		"osd_client"

// 客户端类型
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

struct MemDesc
{
    int32_t Index;
    int32_t IpcDmaFd;   // dma-buf fd
    int32_t MemSize;
    void *IpcMemPaddr;   // dma-buf 的物理地址（可能用不到）
    void *IpcMemVaddr;   // dma-buf在本进程空间的虚拟地址
};

// 源图像信息参数
struct ImageAttr
{
    IMPP_PIX_FMT imageFmt;   // 图像格式 NV12 RGB888....
    IHAL_INT32 imageWidth;   // 图像宽度
    IHAL_INT32 imageHeight;  // 图像高度
};

// 要显示上屏的参数
struct DisplayAttr
{
    IHAL_INT32 scaleWidth;   // 缩放宽度
    IHAL_INT32 scaleHeight;  // 缩放高度
    IHAL_INT32 posX;         // 显示到屏上的x轴坐标
    IHAL_INT32 posY;         // 显示到屏上的y轴坐标
    IHAL_INT32 alpha;        // 不透明度
};

struct DeadEntry
{
    pthread_mutex_t Lock;
    int32_t ServerHandler;
    struct list_head ClientList;
    struct binder_death *DeathInfo;
};

struct DeadEntry *g_dead_entry = NULL;

// 客户端句柄
struct OsdClient
{
    struct OsdClient *UserPtr;
    int32_t ServerHandler;  // 服务端handler引用
    enum ClientType Type;

    struct ImageAttr imageAttr;

    struct list_head List;

    impp_fifo_t MemFifo;

    int32_t IpcDmaFd;   // dma-buf fd
    int32_t MemSize;
    int32_t MemDescNum;
    struct MemDesc *desc;
};

static bool g_binder_thread_run = false;

// 创建客户端句柄，并注册到server端
struct OsdClient *ISS_CreateOsdClt(enum ClientType type);

// 销毁客户端句柄，并向客户端发送注销命令
int32_t ISS_DestoryOsdClt(struct OsdClient *clt);

// 申请ipc内存，跨进程传输显示数据的buffer
int32_t ISS_AllocDispMem(struct OsdClient *clt, int32_t size, uint32_t nmemb);

// 释放ipc内存
int32_t ISS_FreeDispMem(struct OsdClient *clt);

// 设置源图像参数
int32_t ISS_SetImageAttr(struct OsdClient *clt, struct ImageAttr attr);

// 设置显示参数
int32_t ISS_SetDisplayAttr(struct OsdClient *clt, struct DisplayAttr attr);

int32_t ISS_SetFrameRate(struct OsdClient *clt, int32_t frame_rate);

// 刷新图像数据并通知server端有新数据
int32_t ISS_Flush(struct OsdClient *clt, void *addr, int32_t size);

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
        LOGE("in %s, client(%d) cmd(%#x) exec failed, failed code is %#x!!!\n", __func__, getpid(), cmd, remote_reply);
        return -2;
    }

    binder_cmd_freebuf(ti, reply->data0);
    LOGD("in %s, client(%d) cmd(%#x) exec success!!!\n", __func__, getpid(), cmd);
    return 0;
}

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

void add_clientinfo_to_dead_entry(struct DeadEntry *entry, struct OsdClient *clientInfo)
{
    pthread_mutex_lock(&entry->Lock);

    list_add_tail(&clientInfo->List, &entry->ClientList);

    pthread_mutex_unlock(&entry->Lock);
}

void del_clientinfo_from_dead_entry(struct OsdClient *clientInfo)
{
    pthread_mutex_lock(&g_dead_entry->Lock);

    list_del(&clientInfo->List);

    pthread_mutex_unlock(&g_dead_entry->Lock);
}

int is_dead_entry_empty(struct DeadEntry *entry)
{
    int empty = 0;
    pthread_mutex_lock(&entry->Lock);

    empty = list_empty(&entry->ClientList);

    pthread_mutex_unlock(&entry->Lock);

    return empty;
}

struct OsdClient *convert_to_OsdClient(void *ptr)
{
    struct OsdClient **client = ptr;
    if (*client == NULL)
    {
	    free(ptr);
    }

    return *client;
}

void server_dead(tIpcThreadInfo *info, void *ptr)
{
    tIpcThreadInfo *ti = binder_get_thread_info();
    struct DeadEntry *entry = ptr;
    struct OsdClient *clientInfo = NULL;

    LOGE("client(%d) %s osd server is dead!!!\n", getpid(), __func__);

    pthread_mutex_lock(&entry->Lock);
    list_for_each_entry(clientInfo, &entry->ClientList, List)
    {
        struct OsdClient *prev_client = container_of(clientInfo->List.prev, struct OsdClient, List);
	    clientInfo->UserPtr = NULL;

        list_del(&clientInfo->List);

        free_Mem(clientInfo);
        sleep(2);
        free(clientInfo);
        clientInfo = prev_client;
    }
    pthread_mutex_unlock(&entry->Lock);
    binder_cmd_unlink_to_death(ti, entry->ServerHandler, entry->DeathInfo);
    binder_cmd_release(ti, entry->ServerHandler);
    free(entry->DeathInfo);
    free(entry);
    g_dead_entry = NULL;
}

int32_t free_Mem(struct OsdClient *clt)
{
    if (clt == NULL)
    {
        LOGE("in %s, param clt is NULL!!!\n", __func__);
        return -1;
    }

    if (clt->IpcDmaFd == 0)
    {
        LOGE("in %s, No memory requested, no need to release\n", __func__);
        return 0;
    }

    munmap(clt->desc[0].IpcMemVaddr, clt->MemSize);
    close(clt->IpcDmaFd);

    clt->IpcDmaFd = 0;
    clt->MemSize = 0;
    clt->MemDescNum = 0;

    free(clt->desc);
    clt->desc = NULL;
    impp_fifo_deinit(&clt->MemFifo);

    return 0;
}

int32_t alloc_Mem(struct OsdClient *clientInfo, int32_t fd, int32_t size, uint32_t nmemb)
{
    int32_t i = 0;
    void *ptr = NULL;

    if (clientInfo == NULL)
    {
        LOGE("in %s, param clientInfo is NULL!!!\n", __func__);
        return -1;
    }

    if (fd == 0)
    {
        LOGE("in %s, param fd is 0\n", __func__);
        return -1;
    }

    ptr = mmap(NULL, size * nmemb, PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED)
    {
        LOGE("mmap() failed: %s fd:%d size: %d\n", strerror(errno), fd, size);
        return -2;
    }

    clientInfo->desc = malloc(sizeof(struct MemDesc) * nmemb);

    impp_fifo_init(&clientInfo->MemFifo, nmemb);

    clientInfo->IpcDmaFd = fd;
    clientInfo->MemSize = size * nmemb;
    clientInfo->MemDescNum = nmemb;

    for (i = 0; i < nmemb; i++)
    {
	clientInfo->desc[i].Index = i;
    	clientInfo->desc[i].IpcDmaFd = fd;
    	clientInfo->desc[i].MemSize = size;
    	clientInfo->desc[i].IpcMemVaddr = ptr + i * size;
    	clientInfo->desc[i].IpcMemPaddr = NULL;
	impp_fifo_queue(&clientInfo->MemFifo, &clientInfo->desc[i], IMPP_WAIT_FOREVER);
    }

    return 0;
}

struct OsdClient *ISS_CreateOsdClt(enum ClientType type)
{
    int32_t ret = 0;
    int32_t remote_reply = 0;
    int32_t handler = 0;
    tBinderIo bio, reply;
    struct binder_death *dead_cb = NULL;
    struct OsdClient *clientInfo = NULL;
    struct OsdClient **userPtr = NULL;
    tIpcThreadInfo * ti = binder_get_thread_info();
    char binder_buf[DEFAULT_BINDER_IOBUF_SIZE] = {0};

    clientInfo = malloc(sizeof(struct OsdClient));
    if (clientInfo == NULL)
    {
        LOGE("can't malloc heap mem\n");
        goto no_mem;
    }
    memset(clientInfo, 0, sizeof(struct OsdClient));
    INIT_LIST_HEAD(&clientInfo->List);

    userPtr = malloc(sizeof(struct OsdClient *));
    if (userPtr == NULL)
    {
        LOGE("can't malloc heap mem\n");
        goto no_mem;
    }

    handler = binder_get_service(OSD_SERVICE_NAME);
    if (handler == 0)
    {
        LOGE("server %s is not alive\n", OSD_SERVICE_NAME);
        goto no_service;
    }

    memset(&bio, 0, sizeof(tBinderIo));
    memset(&reply, 0, sizeof(tBinderIo));
    binder_io_init(&bio, binder_buf, sizeof(binder_buf), DEFAULT_OFFSET_LIST_SIZE);

    binder_io_append_uint32(&bio, (uint32_t)getpid());
    binder_io_append_uint32(&bio, (uint32_t)REGISTER_CLIENT);
    binder_io_append_uint32(&bio, (uint32_t)type);
    binder_io_append_obj(&bio, &osd_client_cb);

    ret = msg_proxy(handler, &bio, &reply, REGISTER_CLIENT);
    if (ret != 0)
    {
        LOGE("in %s, register client failed\n", __func__);
        goto register_client_failed;
    }

    if (g_dead_entry == NULL)
    {
        g_dead_entry = malloc(sizeof(struct DeadEntry));
        if (g_dead_entry == NULL)
        {
            LOGE("in func %s, alloc g_dead_entry mem failed!!!\n", __func__);
            goto no_heapmem;
        }
        memset(g_dead_entry, 0, sizeof(struct DeadEntry));

        dead_cb = malloc(sizeof(struct binder_death));
        if (dead_cb == NULL)
        {
            LOGE("in func %s, alloc dead_cb mem failed!!!\n", __func__);
            goto no_heapmem;
        }
        memset(dead_cb, 0, sizeof(struct binder_death));
        dead_cb->death_cb = server_dead;
	    dead_cb->ptr = g_dead_entry;

        binder_cmd_link_to_death(ti, handler, (void *)(dead_cb));
        flush_commands(ti);

        pthread_mutex_init(&g_dead_entry->Lock, NULL);
        INIT_LIST_HEAD(&g_dead_entry->ClientList);
        g_dead_entry->ServerHandler = handler;
        g_dead_entry->DeathInfo = dead_cb;
    }

    add_clientinfo_to_dead_entry(g_dead_entry, clientInfo);

	if(g_binder_thread_run == false){
		binder_thread_enter_loop(1, 0);
		g_binder_thread_run = true;
	}

    clientInfo->ServerHandler = handler;
    clientInfo->Type = type;
    clientInfo->UserPtr = userPtr;

    *userPtr = clientInfo;

    return userPtr;
no_heapmem:
register_client_failed:
no_service:
    free(clientInfo);
no_mem:
    return NULL;

}

int32_t ISS_DestoryOsdClt(struct OsdClient *clt)
{
    int32_t ret = 0;
    tBinderIo bio, reply;
    struct OsdClient *clientInfo = NULL;
    tIpcThreadInfo * ti = binder_get_thread_info();
    char binder_buf[DEFAULT_BINDER_IOBUF_SIZE] = {0};

    if (clt == NULL)
    {
        LOGE("in %s, param clt is NULL\n", __func__);
        return -1;
    }

    clientInfo = convert_to_OsdClient((void *)clt);
    if (clientInfo == NULL)
    {
        LOGE("in %s, Unregistered client or client destroyed!!!\n", __func__);
        return -2;
    }

    memset(&bio, 0, sizeof(tBinderIo));
    memset(&reply, 0, sizeof(tBinderIo));
    binder_io_init(&bio, binder_buf, sizeof(binder_buf), DEFAULT_OFFSET_LIST_SIZE);

    binder_io_append_uint32(&bio, (uint32_t)getpid());
    binder_io_append_uint32(&bio, (uint32_t)UNREGISTER_CLIENT);
    binder_io_append_uint32(&bio, (uint32_t)clientInfo->Type);

    ret = msg_proxy(clientInfo->ServerHandler, &bio, &reply, UNREGISTER_CLIENT);
    if (ret != 0)
    {
        LOGE("in %s, unregister client failed\n", __func__);
        return -3;
    }

    if (clientInfo->IpcDmaFd != 0)
    {
        free_Mem(clientInfo);
    }

    del_clientinfo_from_dead_entry(clientInfo);

    if (is_dead_entry_empty(g_dead_entry))
    {
        binder_cmd_unlink_to_death(ti, g_dead_entry->ServerHandler, g_dead_entry->DeathInfo);
        binder_cmd_release(ti, g_dead_entry->ServerHandler);
        free(g_dead_entry->DeathInfo);
        free(g_dead_entry);
        g_dead_entry = NULL;
    }
    
    free(clientInfo->UserPtr);
    free(clientInfo);
    return 0;
}

int32_t ISS_AllocDispMem(struct OsdClient *clt, int32_t size, uint32_t nmemb)
{
    int32_t ret = 0;
    int32_t dmabuf_Fd = 0;
    tBinderIo bio, reply;
    struct OsdClient *clientInfo = NULL;
    tIpcThreadInfo * ti = binder_get_thread_info();
    char binder_buf[DEFAULT_BINDER_IOBUF_SIZE] = {0};

    if (clt == NULL)
    {
        LOGE("in %s, param clt is NULL\n", __func__);
        return -1;
    }

    clientInfo = convert_to_OsdClient((void *)clt);
    if (clientInfo == NULL)
    {
        LOGE("in %s, Unregistered client or client destroyed!!!\n", __func__);
        return -2;
    }

    if (clientInfo->desc != NULL)
    {
	LOGE("The disp mem already exists.\n");
	return -2;
    }

    memset(&bio, 0, sizeof(tBinderIo));
    memset(&reply, 0, sizeof(tBinderIo));
    binder_io_init(&bio, binder_buf, sizeof(binder_buf), DEFAULT_OFFSET_LIST_SIZE);

    binder_io_append_uint32(&bio, (uint32_t)getpid());
    binder_io_append_uint32(&bio, (uint32_t)ALLOC_DISPMEM);
    binder_io_append_uint32(&bio, (uint32_t)clientInfo->Type);
    binder_io_append_uint32(&bio, (uint32_t)size);
    binder_io_append_uint32(&bio, (uint32_t)nmemb);

    ret = msg_proxy(clientInfo->ServerHandler, &bio, &reply, ALLOC_DISPMEM);
    if (ret != 0)
    {
        LOGE("in %s, Remote call failed\n", __func__);
        return -3;
    }

    dmabuf_Fd = binder_io_get_fd(&reply, 0);
    LOGD("in %s, client(%d) ISS_AllocDispMem success fd is %d!!!\n", __func__, getpid(), dmabuf_Fd);

    if (dmabuf_Fd == 0)
    {
        LOGE("in %s, Failed to obtain dmabuf fd\n", __func__);
        return -3;
    }

    ret = alloc_Mem(clientInfo, dmabuf_Fd, size, nmemb);
    if (ret != 0)
    {
        LOGE("in %s, alloc ipc mem error\n", __func__);
        return -4;
    }

    return 0;
}

int32_t ISS_FreeDispMem(struct OsdClient *clt)
{
    int32_t ret = 0;
    tBinderIo bio, reply;
    struct OsdClient *clientInfo = NULL;
    tIpcThreadInfo * ti = binder_get_thread_info();
    char binder_buf[DEFAULT_BINDER_IOBUF_SIZE] = {0};

    if (clt == NULL)
    {
        LOGE("in %s, param clt is NULL\n", __func__);
        return -1;
    }

    clientInfo = convert_to_OsdClient((void *)clt);
    if (clientInfo == NULL)
    {
        LOGE("in %s, Unregistered client or client destroyed!!!\n", __func__);
        return -2;
    }

    memset(&bio, 0, sizeof(tBinderIo));
    memset(&reply, 0, sizeof(tBinderIo));
    binder_io_init(&bio, binder_buf, sizeof(binder_buf), DEFAULT_OFFSET_LIST_SIZE);

    binder_io_append_uint32(&bio, (uint32_t)getpid());
    binder_io_append_uint32(&bio, (uint32_t)FREE_DISPMEM);
    binder_io_append_uint32(&bio, (uint32_t)clientInfo->Type);

    ret = msg_proxy(clientInfo->ServerHandler, &bio, &reply, FREE_DISPMEM);
    if (ret != 0)
    {
        LOGE("in %s, Remote call failed\n", __func__);
        return -3;
    }

    ret = free_Mem(clientInfo);
    if (ret != 0)
    {
        LOGE("in %s, free mem error\n", __func__);
        return -3;
    }

    return 0;
}

int32_t ISS_SetImageAttr(struct OsdClient *clt, struct ImageAttr attr)
{
    int32_t ret = 0;
    tBinderIo bio, reply;
    struct OsdClient *clientInfo = NULL;
    tIpcThreadInfo * ti = binder_get_thread_info();
    char binder_buf[DEFAULT_BINDER_IOBUF_SIZE] = {0};

    if (clt == NULL)
    {
        LOGE("in %s, param clt is NULL\n", __func__);
        return -1;
    }

    clientInfo = convert_to_OsdClient((void *)clt);
    if (clientInfo == NULL)
    {
        LOGE("in %s, Unregistered client or client destroyed!!!\n", __func__);
        return -2;
    }

    memset(&bio, 0, sizeof(tBinderIo));
    memset(&reply, 0, sizeof(tBinderIo));
    binder_io_init(&bio, binder_buf, sizeof(binder_buf), DEFAULT_OFFSET_LIST_SIZE);

    binder_io_append_uint32(&bio, (uint32_t)getpid());
    binder_io_append_uint32(&bio, (uint32_t)PARSE_IMAGEATTR);
    binder_io_append_uint32(&bio, (uint32_t)clientInfo->Type);
    binder_io_append_data(&bio, &attr, sizeof(struct ImageAttr));

    ret = msg_proxy(clientInfo->ServerHandler, &bio, &reply, PARSE_IMAGEATTR);
    if (ret != 0)
    {
        LOGE("in %s, Remote call failed\n", __func__);
        return -3;
    }

    memcpy(&clientInfo->imageAttr, &attr, sizeof(struct ImageAttr));

    return 0;
}

int32_t ISS_SetDisplayAttr(struct OsdClient *clt, struct DisplayAttr attr)
{
    int32_t ret = 0;
    tBinderIo bio, reply;
    struct OsdClient *clientInfo = NULL;
    tIpcThreadInfo * ti = binder_get_thread_info();
    char binder_buf[DEFAULT_BINDER_IOBUF_SIZE] = {0};

    if (clt == NULL)
    {
        LOGE("in %s, param clt is NULL\n", __func__);
        return -1;
    }

    clientInfo = convert_to_OsdClient((void *)clt);
    if (clientInfo == NULL)
    {
        LOGE("in %s, Unregistered client or client destroyed!!!\n", __func__);
        return -2;
    }

    memset(&bio, 0, sizeof(tBinderIo));
    memset(&reply, 0, sizeof(tBinderIo));
    binder_io_init(&bio, binder_buf, sizeof(binder_buf), DEFAULT_OFFSET_LIST_SIZE);

    binder_io_append_uint32(&bio, (uint32_t)getpid());
    binder_io_append_uint32(&bio, (uint32_t)PARSE_DISPLAYATTR);
    binder_io_append_uint32(&bio, (uint32_t)clientInfo->Type);
    binder_io_append_data(&bio, &attr, sizeof(struct DisplayAttr));

    ret = msg_proxy(clientInfo->ServerHandler, &bio, &reply, PARSE_DISPLAYATTR);
    if (ret != 0)
    {
        LOGE("in %s, Remote call failed\n", __func__);
        return -3;
    }

    return 0;
}

int32_t ISS_SetFrameRate(struct OsdClient *clt, int32_t frame_rate)
{
    int32_t ret = 0;
    tBinderIo bio, reply;
    struct OsdClient *clientInfo = NULL;
    tIpcThreadInfo * ti = binder_get_thread_info();
    char binder_buf[DEFAULT_BINDER_IOBUF_SIZE] = {0};

    if (clt == NULL)
    {
        LOGE("in %s, param clt is NULL\n", __func__);
        return -1;
    }

    clientInfo = convert_to_OsdClient((void *)clt);
    if (clientInfo == NULL)
    {
        LOGE("in %s, Unregistered client or client destroyed!!!\n", __func__);
        return -2;
    }

    memset(&bio, 0, sizeof(tBinderIo));
    memset(&reply, 0, sizeof(tBinderIo));
    binder_io_init(&bio, binder_buf, sizeof(binder_buf), DEFAULT_OFFSET_LIST_SIZE);

    binder_io_append_uint32(&bio, (uint32_t)getpid());
    binder_io_append_uint32(&bio, (uint32_t)SET_FRAME_RATE);
    binder_io_append_uint32(&bio, (uint32_t)clientInfo->Type);
    binder_io_append_uint32(&bio, (uint32_t)frame_rate);

    ret = msg_proxy(clientInfo->ServerHandler, &bio, &reply, SET_FRAME_RATE);
    if (ret != 0)
    {
        LOGE("in %s, Remote call failed\n", __func__);
        return -3;
    }

    return 0;
}


int32_t ISS_Flush(struct OsdClient *clt, void *addr, int32_t size)
{
    int32_t ret = 0;
    tBinderIo bio, reply;
    struct MemDesc *buffer = NULL;
    struct OsdClient *clientInfo = NULL;
    tIpcThreadInfo * ti = binder_get_thread_info();
    char binder_buf[DEFAULT_BINDER_IOBUF_SIZE] = {0};
    struct dma_buf_sync sync = {
        .flags = DMA_BUF_SYNC_RW,
    };

    if (clt == NULL || addr == NULL)
    {
        LOGE("in %s, param clt is NULL\n", __func__);
        return -1;
    }

    clientInfo = convert_to_OsdClient((void *)clt);
    if (clientInfo == NULL)
    {
        LOGE("in %s, Unregistered client or client destroyed!!!\n", __func__);
        return -2;
    }

    if (size > clientInfo->desc[0].MemSize)
    {
        LOGE("in %s, param size > ipcmem size\n", __func__);
        return -1;
    }

    memset(&bio, 0, sizeof(tBinderIo));
    memset(&reply, 0, sizeof(tBinderIo));
    binder_io_init(&bio, binder_buf, sizeof(binder_buf), DEFAULT_OFFSET_LIST_SIZE);

    binder_io_append_uint32(&bio, (uint32_t)getpid());
    binder_io_append_uint32(&bio, (uint32_t)FLUSH_LAYER_DATA);
    binder_io_append_uint32(&bio, (uint32_t)clientInfo->Type);
    binder_io_append_uint32(&bio, (uint32_t)size);

    sync.flags |= MEM_FLUSH_START;
    buffer = impp_fifo_dequeue(&clientInfo->MemFifo, IMPP_WAIT_FOREVER);

    ioctl(buffer->IpcDmaFd, DMA_BUF_IOCTL_SYNC, &sync);
    memcpy(buffer->IpcMemVaddr, addr, size);
    sync.flags = DMA_BUF_SYNC_RW;
    sync.flags |= MEM_FLUSH_END;
    ioctl(buffer->IpcDmaFd, DMA_BUF_IOCTL_SYNC, &sync);

    binder_io_append_uint32(&bio, (uint32_t)buffer->Index);
    ret = msg_proxy(clientInfo->ServerHandler, &bio, &reply, FLUSH_LAYER_DATA);
    if (ret != 0)
    {
        LOGE("in %s, Remote call failed\n", __func__);
        return -3;
    }

    impp_fifo_queue(&clientInfo->MemFifo, buffer, IMPP_WAIT_FOREVER);

    return 0;
}
