#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/un.h>
#include <sys/reboot.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <dirent.h>

//#define DEBUG_ENABLE
//#define DEBUG_FRAME_ATTR
//#define DEBUG_RWLOCK
#include "dlog.h"
#include "dpu.h"
#include "convert.h"
#include "dmabuf_allocator.h"
#include "systemserver.h"
#include "binder_common.h"
#include "binder_ipc.h"
#include "binder_io.h"
#include "fifo.h"
#include "list.h"
#include "sys_common.h"

#include "mt_timer.h"

#define NAME_LEN 30

#define SCREEN_WIDTH  720
#define SCREEN_HEIGHT 1280
#define SCREEN_FMT    IMPP_PIX_FMT_BGRA_8888

#define LOG_TAG		OSD_SERVICE_NAME


#ifdef DEBUG_RWLOCK
int32_t debug_rwlock_flags = 1;
#else
int32_t debug_rwlock_flags = 0;
#endif // DEBUG_RWLOCK

#define RWLOCK_WRLOCK(LOCK) \
{ \
    if (debug_rwlock_flags) \
        LOGD("%s-%d: server wrlock\n", __func__, __LINE__); \
    pthread_rwlock_wrlock(LOCK);\
}

#define RWLOCK_RDLOCK(LOCK)\
{ \
    if (debug_rwlock_flags) \
        LOGD("%s-%d: server rdlock\n", __func__, __LINE__); \
    pthread_rwlock_rdlock(LOCK);\
}

#define RWLOCK_UNLOCK(LOCK)\
{ \
    if (debug_rwlock_flags) \
        LOGD("%s-%d: server unlock\n", __func__, __LINE__); \
    pthread_rwlock_unlock(LOCK);\
}

TIMER_CREATE(timer);

struct OsdServer
{
    int32_t clientNum;
    struct list_head clients_list;
    pthread_rwlock_t clients_rwlock;
    struct list_head flush_list;
    pthread_rwlock_t flush_rwlock;


    int32_t layerNums;
    int32_t layerBits;
    int32_t orderBits;
    IHal_Dpu_FrameDesc_t frame;

    IHal_Dpu_Handle_t *dpuHandle;
    struct IHal_MemHandle *memHandle;

};
struct OsdServer *g_osd_server_handler = NULL;
struct list_head g_dead_list;
pthread_rwlock_t g_dead_rwlock;

struct DeadEntry
{
    pid_t ClientPid;
    int32_t clientNum;
    pthread_mutex_t Lock;
    int32_t ClientHandler;
    struct list_head DeadList;
    struct list_head ClientList;
    struct binder_death *DeathInfo;
};

struct MemDesc
{
    int32_t Index;
    int32_t IpcDmaFd;
    int32_t MemSize;
    void *IpcMemPaddr;
    void *IpcMemVaddr;
};

// 客户端类型
enum ClientType
{
    OTHER = 1,
    UI,
    VIDEO,
};

// 源图像信息参数
struct SrcAttr
{
    IMPP_PIX_FMT srcFmt;   // 图像格式 NV12 RGB888....
    int32_t srcWidth;   // 图像宽度
    int32_t srcHeight;  // 图像高度
};

// 要显示上屏的参数
struct DisplayAttr
{
    int32_t scaleWidth;   // 缩放宽度
    int32_t scaleHeight;  // 缩放高度
    int32_t posX;         // 显示到屏上的x轴坐标
    int32_t posY;         // 显示到屏上的y轴坐标
    int32_t alpha;        // 不透明度
};

struct OsdClientDetail
{
    pid_t ClientPid;
    int32_t ClientHandler;
    enum ClientType Type;
    struct list_head Entry;
    struct list_head FlushEntry;
    struct list_head DeadClientEntry;

    struct SrcAttr ImageAttr;
    IHal_DpuOSD_LayerAttr_t Attr;
    int32_t Flags;
    pthread_mutex_t Lock;
    pthread_cond_t Cond;

    int32_t ref_count;
    int32_t need_flush;
    int32_t timer_fd;

    int32_t LayerId;
    int32_t MemSize;
    int32_t MemDescNum;
    struct MemDesc *desc;
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

int32_t layerId[4] = {
    Dpu_Layer0,
    Dpu_Layer1,
    Dpu_Layer2,
    Dpu_Layer3,
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
    NOT_SUPPORT_SRC_FMT,
    NOT_SUPPORT_FRAME_RATE,
};

uint8_t *g_temp_frame = NULL;

void is_remove_from_flush_list(struct OsdClientDetail *clientInfo)
{
    struct OsdClientDetail *clientDetail = NULL;

    RWLOCK_RDLOCK(&g_osd_server_handler->flush_rwlock);
    list_for_each_entry(clientDetail, &g_osd_server_handler->flush_list, FlushEntry)
    {
        if (clientDetail == clientInfo)
        {
            remove_from_flush_list(clientDetail);
            RWLOCK_UNLOCK(&g_osd_server_handler->flush_rwlock);
            return;
        }

    }
    RWLOCK_UNLOCK(&g_osd_server_handler->flush_rwlock);

    return;
}

void client_dead(tIpcThreadInfo *info, void *ptr)
{
    IMPP_BufferInfo_t buf_info;
    int type = 0;
    pid_t clientPid = 0;
    tIpcThreadInfo * ti = binder_get_thread_info();
    struct OsdClientDetail *clientInfo = (struct OsdClientDetail *)ptr;
    struct DeadEntry *entry = (struct DeadEntry *)ptr;

    pthread_mutex_lock(&entry->Lock);
    list_for_each_entry(clientInfo, &entry->ClientList, DeadClientEntry)
    {
        struct OsdClientDetail *prev_clientDetail = container_of(clientInfo->DeadClientEntry.prev, struct OsdClientDetail, DeadClientEntry);
        while (get_clientDetail_refcount(clientInfo) != 1)
        {
            usleep(5000);
        }

        is_remove_from_flush_list(clientInfo);

        release_priority_and_del_clientDetail(clientInfo);

        list_del(&clientInfo->DeadClientEntry);
        entry->clientNum--;

        TIMER_DEL(timer, clientInfo->timer_fd);

        pthread_mutex_lock(&clientInfo->Lock);

        clientPid  = clientInfo->ClientPid;
        type = clientInfo->Type;

        free_layer(clientInfo);

        buf_info.fd = clientInfo->desc[0].IpcDmaFd;
        buf_info.size = clientInfo->MemSize;
        buf_info.paddr = clientInfo->desc[0].IpcMemPaddr;
        buf_info.vaddr = clientInfo->desc[0].IpcMemVaddr;

        free_mem(buf_info);
        clientInfo->MemSize = 0;
        clientInfo->MemDescNum = 0;
        free(clientInfo->desc);
        clientInfo->desc = NULL;

        pthread_mutex_unlock(&clientInfo->Lock);

        pthread_mutex_destroy(&clientInfo->Lock);
        free(clientInfo);

        LOGE("The client(%d) type(%d) has died and all resources have been released.\n", clientPid, type);
        clientInfo = prev_clientDetail;

    }
    pthread_mutex_unlock(&entry->Lock);

    binder_cmd_unlink_to_death(ti, entry->ClientHandler, entry->DeathInfo);
    binder_cmd_release(ti, entry->ClientHandler);

    del_deadEntry_from_dead_list(entry);

	IHal_Dpu_DmmuReleaseAll(g_osd_server_handler->dpuHandle);
    free(entry->DeathInfo);
    free(entry);
}

int get_clientDetail_refcount(struct OsdClientDetail *clientInfo)
{
    int32_t ref_count = 0;

    if (clientInfo == NULL)
    {
        LOGE("paramter clientInfo is NULL\n");
        return -1;
    }

    pthread_mutex_lock(&clientInfo->Lock);

    ref_count = clientInfo->ref_count;

    LOGD("in func %s, client(%d) ref_count %d\n", __func__, clientInfo->ClientPid, ref_count);
    pthread_mutex_unlock(&clientInfo->Lock);

    return ref_count;

}

struct OsdClientDetail *find_and_add_references_clientDetail(pid_t clientPid, uint32_t type)
{
    int32_t ref_count = 0;
    struct OsdClientDetail *temp = NULL;

    RWLOCK_RDLOCK(&g_osd_server_handler->clients_rwlock);

    list_for_each_entry(temp, &g_osd_server_handler->clients_list, Entry)
    {
        if (temp->ClientPid == clientPid && temp->Type == type)
        {
            RWLOCK_UNLOCK(&g_osd_server_handler->clients_rwlock);
            pthread_mutex_lock(&temp->Lock);
            ref_count = client_ref_increase(temp);
            pthread_mutex_unlock(&temp->Lock);
            return temp;
        }
    }

    RWLOCK_UNLOCK(&g_osd_server_handler->clients_rwlock);

    return NULL;
}

struct DeadEntry *find_deadEntry_in_dead_list(pid_t clientPid)
{
    int32_t ref_count = 0;
    struct DeadEntry *temp = NULL;

    RWLOCK_RDLOCK(&g_dead_rwlock);

    list_for_each_entry(temp, &g_dead_list, DeadList)
    {
        if (temp->ClientPid == clientPid)
        {
            RWLOCK_UNLOCK(&g_dead_rwlock);
            return temp;
        }
    }

    RWLOCK_UNLOCK(&g_dead_rwlock);

    return NULL;
}

void add_deadEntry_to_dead_list(struct DeadEntry *entry)
{
    int32_t ref_count = 0;
    struct DeadEntry *temp = NULL;

    RWLOCK_WRLOCK(&g_dead_rwlock);

    list_add_tail(&entry->DeadList, &g_dead_list);

    RWLOCK_UNLOCK(&g_dead_rwlock);

    return;
}

void del_deadEntry_from_dead_list(struct DeadEntry *entry)
{
    RWLOCK_WRLOCK(&g_dead_rwlock);

    list_del(&entry->DeadList);

    RWLOCK_UNLOCK(&g_dead_rwlock);

    return;
}

struct OsdClientDetail *check_flush_list_client( pid_t clientPid)
{
    int32_t ref_count = 0;
    struct OsdClientDetail *temp = NULL;

    RWLOCK_RDLOCK(&g_osd_server_handler->flush_rwlock);

    list_for_each_entry(temp, &g_osd_server_handler->flush_list, FlushEntry)
    {
        if (temp->ClientPid == clientPid)
        {
            RWLOCK_UNLOCK(&g_osd_server_handler->flush_rwlock);
            return temp;
        }
    }

    RWLOCK_UNLOCK(&g_osd_server_handler->flush_rwlock);

    return NULL;
}

int release_clientDetail(struct OsdClientDetail *clientInfo)
{
    int32_t ref_count = 0;
    if (clientInfo == NULL)
    {
        LOGE("paramter clientInfo is NULL\n");
        return -1;
    }

    pthread_mutex_lock(&clientInfo->Lock);

    ref_count = client_ref_decrease(clientInfo);

    LOGD("in func %s, client(%d) ref_count(-) %d\n", __func__, clientInfo->ClientPid, ref_count);

    pthread_mutex_unlock(&clientInfo->Lock);

    return ref_count;
}

int add_clientDetail(struct OsdClientDetail *clientInfo)
{
    int32_t add_flag = 0;
    struct OsdClientDetail *temp = NULL;

    if (clientInfo == NULL)
    {
        LOGE("paramter clientInfo is NULL\n");
        return -1;
    }

    RWLOCK_WRLOCK(&g_osd_server_handler->clients_rwlock);

    if (list_empty(&g_osd_server_handler->clients_list))
    {
        list_add_tail(&clientInfo->Entry, &g_osd_server_handler->clients_list);
    }
    else
    {
        list_for_each_entry(temp, &g_osd_server_handler->clients_list, Entry)
        {
            if (temp->Type < clientInfo->Type)
            {
                list_add_head(&clientInfo->Entry, &temp->Entry);
                add_flag = 1;
                break;
            }
        }

        if (!add_flag)
        {
            struct OsdClientDetail *tail_info = container_of(g_osd_server_handler->clients_list.prev, struct OsdClientDetail, Entry);
            list_add_tail(&clientInfo->Entry, &tail_info->Entry);
            add_flag = 1;
        }
    }

    client_ref_increase(clientInfo);
    osd_clientNum_increase();

    RWLOCK_UNLOCK(&g_osd_server_handler->clients_rwlock);

    return 0;
}

int del_clientDetail(struct OsdClientDetail *clientInfo)
{
    if (clientInfo == NULL)
    {
        LOGE("paramter clientInfo is NULL\n");
        return -1;
    }

    RWLOCK_WRLOCK(&g_osd_server_handler->clients_rwlock);

    list_del(&clientInfo->Entry);
    osd_clientNum_decrease();

    RWLOCK_UNLOCK(&g_osd_server_handler->clients_rwlock);

    return 0;
}

int find_client_in_dead_entry(struct DeadEntry *entry, pid_t clientPid, struct OsdClientDetail *clientInfo)
{
    pthread_mutex_lock(&entry->Lock);

    if (entry->ClientPid == 0)
    {
        entry->ClientPid = clientPid;
    }

    list_add_tail(&clientInfo->DeadClientEntry, &entry->ClientList);
    entry->clientNum++;

    pthread_mutex_unlock(&entry->Lock);
}

void add_client_to_dead_entry(struct DeadEntry *entry, pid_t clientPid, struct OsdClientDetail *clientInfo)
{
    pthread_mutex_lock(&entry->Lock);

    if (entry->ClientPid == 0)
    {
        entry->ClientPid = clientPid;
    }

    list_add_tail(&clientInfo->DeadClientEntry, &entry->ClientList);
    entry->clientNum++;

    pthread_mutex_unlock(&entry->Lock);
}

void del_client_from_dead_entry(struct DeadEntry *entry, struct OsdClientDetail *clientInfo)
{
    pthread_mutex_lock(&entry->Lock);

    list_del(&clientInfo->DeadClientEntry);
    entry->clientNum--;

    pthread_mutex_unlock(&entry->Lock);
}

int is_dead_entry_empty(struct DeadEntry *entry)
{
    int count = 0;
    pthread_mutex_lock(&entry->Lock);

    count = entry->clientNum;

    pthread_mutex_unlock(&entry->Lock);

    return ((count == 0) ? 1 : 0);
}

int client_ref_increase(struct OsdClientDetail *clientInfo)
{
    if (clientInfo == NULL)
    {
        LOGE("paramter clientInfo is NULL\n");
        return -1;
    }

    clientInfo->ref_count++;

    return clientInfo->ref_count;
}

int client_ref_decrease(struct OsdClientDetail *clientInfo)
{
    if (clientInfo == NULL)
    {
        LOGE("paramter clientInfo is NULL\n");
        return -1;
    }

    clientInfo->ref_count--;

    return clientInfo->ref_count;
}

int add_to_flush_list(struct OsdClientDetail *clientInfo)
{
    if (clientInfo == NULL)
    {
        LOGE("paramter clientInfo is NULL\n");
        return -1;
    }

    RWLOCK_WRLOCK(&g_osd_server_handler->flush_rwlock);

    list_add_tail(&clientInfo->FlushEntry, &g_osd_server_handler->flush_list);
    client_ref_increase(clientInfo);

    RWLOCK_UNLOCK(&g_osd_server_handler->flush_rwlock);

    return 0;
}

int remove_from_flush_list(struct OsdClientDetail *clientInfo)
{
    if (clientInfo == NULL)
    {
        LOGE("paramter clientInfo is NULL\n");
        return -1;
    }

    list_del(&clientInfo->FlushEntry);
    client_ref_decrease(clientInfo);

    return 0;
}

int get_layerId_index(int32_t id)
{
    int32_t i = 0;

    for (i = 0; i < (sizeof(layerId) / sizeof(layerId[0])); i++)
    {
        if (layerId[i] == id)
        {
            return i;
        }
    }
}

void print_frame_attr(IHal_Dpu_FrameDesc_t frame)
{
    int32_t i = 0;

    LOGD("frame.dpu_func = %#x\n", frame.dpu_func);
    LOGD("frame.func_desc.osd_frame.osd_flags = %#x\n", frame.func_desc.osd_frame.osd_flags);
    LOGD("frame.func_desc.osd_frame.wback_enable = %d\n", frame.func_desc.osd_frame.wback_enable);
    LOGD("frame.func_desc.osd_frame.wback_fmt = %#x\n", frame.func_desc.osd_frame.wback_fmt);
    LOGD("frame.func_desc.osd_frame.outWidth = %d\n", frame.func_desc.osd_frame.outWidth);
    LOGD("frame.func_desc.osd_frame.outHeight = %d\n", frame.func_desc.osd_frame.outHeight);

    for (i = 0; i < 4; i++)
    {
        LOGD("frame.func_desc.osd_frame.layer[%d].srcFmt = %#x\n", i, frame.func_desc.osd_frame.layer[i].srcFmt);
        LOGD("frame.func_desc.osd_frame.layer[%d].srcWidth = %d\n", i, frame.func_desc.osd_frame.layer[i].srcWidth);
        LOGD("frame.func_desc.osd_frame.layer[%d].srcHeight = %d\n", i, frame.func_desc.osd_frame.layer[i].srcHeight);
        LOGD("frame.func_desc.osd_frame.layer[%d].srcCropx = %#x\n", i, frame.func_desc.osd_frame.layer[i].srcCropx);
        LOGD("frame.func_desc.osd_frame.layer[%d].srcCropy = %#x\n", i, frame.func_desc.osd_frame.layer[i].srcCropy);
        LOGD("frame.func_desc.osd_frame.layer[%d].srcCropw = %#x\n", i, frame.func_desc.osd_frame.layer[i].srcCropw);
        LOGD("frame.func_desc.osd_frame.layer[%d].srcCroph = %#x\n", i, frame.func_desc.osd_frame.layer[i].srcCroph);
        LOGD("frame.func_desc.osd_frame.layer[%d].scale_enable = %d\n", i, frame.func_desc.osd_frame.layer[i].scale_enable);
        LOGD("frame.func_desc.osd_frame.layer[%d].scaleHeight = %#x\n", i, frame.func_desc.osd_frame.layer[i].scaleHeight);
        LOGD("frame.func_desc.osd_frame.layer[%d].scaleWidth = %#x\n", i, frame.func_desc.osd_frame.layer[i].scaleWidth);
        LOGD("frame.func_desc.osd_frame.layer[%d].osd_posX = %#x\n", i, frame.func_desc.osd_frame.layer[i].osd_posX);
        LOGD("frame.func_desc.osd_frame.layer[%d].osd_posY = %#x\n", i, frame.func_desc.osd_frame.layer[i].osd_posY);
        LOGD("frame.func_desc.osd_frame.layer[%d].osd_order = %#x\n", i, frame.func_desc.osd_frame.layer[i].osd_order);
        LOGD("frame.func_desc.osd_frame.layer[%d].alpha = %#x\n", i, frame.func_desc.osd_frame.layer[i].alpha);
        LOGD("frame.func_desc.osd_frame.layer[%d].paddr = %#x\n", i, frame.func_desc.osd_frame.layer[i].paddr);
        LOGD("frame.func_desc.osd_frame.layer[%d].vaddr = %#x\n", i, frame.func_desc.osd_frame.layer[i].vaddr);
        LOGD("frame.func_desc.osd_frame.layer[%d].uv_vaddr = %#x\n", i, frame.func_desc.osd_frame.layer[i].uv_vaddr);
        LOGD("frame.func_desc.osd_frame.layer[%d].color_order = %#x\n", i, frame.func_desc.osd_frame.layer[i].color_order);
    }

}

void *osd_main_thread(void *arg)
{
    int32_t i = 0;
    int32_t ret = 0;
    IMPP_FrameInfo_t outframe;
    IMPP_BufferInfo_t displaybuf;

    pthread_setname_np(pthread_self(), "osd_main_thread");

    while (1)
    {
        RWLOCK_WRLOCK(&g_osd_server_handler->flush_rwlock);
        if (g_osd_server_handler->clientNum == 0 || list_empty(&g_osd_server_handler->flush_list))
        {
            RWLOCK_UNLOCK(&g_osd_server_handler->flush_rwlock);
            usleep(400);
            continue;
        }

        if (!list_empty(&g_osd_server_handler->flush_list))
        {
            struct OsdClientDetail *clientDetail = NULL;
            list_for_each_entry(clientDetail, &g_osd_server_handler->flush_list, FlushEntry)
            {
                    pthread_mutex_lock(&clientDetail->Lock);
                    int32_t index = 0;
                    struct OsdClientDetail *prev_clientDetail = container_of(clientDetail->FlushEntry.prev, struct OsdClientDetail, FlushEntry);
                    IHal_DpuOSD_LayerAttr_t *attr = NULL;

                    index = get_layerId_index(clientDetail->LayerId);
                    attr = memcpy(&g_osd_server_handler->frame.func_desc.osd_frame.layer[index],
                            &clientDetail->Attr,
                            sizeof(IHal_DpuOSD_LayerAttr_t));

                    g_osd_server_handler->frame.func_desc.osd_frame.osd_flags |= clientDetail->LayerId;

                    pthread_mutex_unlock(&clientDetail->Lock);
            }

            for (i = 0; i < 4; i++)
            {
                if (!(g_osd_server_handler->layerBits & (1 << i)))
                {
                    g_osd_server_handler->frame.func_desc.osd_frame.layer[i].osd_order = 0;
                }
            }
        }

#ifdef DEBUG_FRAME_ATTR
        print_frame_attr(g_osd_server_handler->frame);
#endif // DEBUG_FRAME_ATTR

	IHal_Dpu_RDMA_GetFrame(g_osd_server_handler->dpuHandle, &displaybuf);
    	displaybuf.fd = 0;
    	IHal_Dpu_Composer_Set_WbackBuffers(g_osd_server_handler->dpuHandle, &displaybuf, 0);

        ret = IHal_Dpu_Composer_Process(g_osd_server_handler->dpuHandle, &g_osd_server_handler->frame);

        IHal_Dpu_Composer_GetFrame(g_osd_server_handler->dpuHandle, &outframe);

        IHal_Dpu_Composer_ReleaseFrame(g_osd_server_handler->dpuHandle, &outframe);

	IHal_Dpu_RDMA_PutFrame(g_osd_server_handler->dpuHandle, &displaybuf);

        int32_t vsync = 0;
        int32_t ret = 0;

        IHal_Dpu_WaitForVsync(g_osd_server_handler->dpuHandle, &vsync);

        if (!list_empty(&g_osd_server_handler->flush_list))
        {
            struct OsdClientDetail *clientDetail = NULL;
            list_for_each_entry(clientDetail, &g_osd_server_handler->flush_list, FlushEntry)
            {
                    pthread_mutex_lock(&clientDetail->Lock);
                    int32_t index = 0;
                    struct OsdClientDetail *prev_clientDetail = container_of(clientDetail->FlushEntry.prev, struct OsdClientDetail, FlushEntry);

                    remove_from_flush_list(clientDetail);
                    clientDetail->need_flush = 0;
                    pthread_mutex_unlock(&clientDetail->Lock);
                    pthread_cond_signal(&clientDetail->Cond);
                    clientDetail = prev_clientDetail;
            }
        }

        RWLOCK_UNLOCK(&g_osd_server_handler->flush_rwlock);
    }

}

int check_dev_fb_num(struct OsdServer *serverInfo)
{
    DIR *dir = opendir("/dev");
    if (dir == NULL)
    {
        perror("Failed to open directory");
        return -1;
    }

    struct dirent *entry;
    char prefix[] = "fb";
    int32_t count = 0;
    size_t prefix_len = 2;

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_name[0] == '.')
        {
            continue;
        }

        if (strncmp(entry->d_name, prefix, prefix_len) == 0)
        {
            int32_t index = entry->d_name[prefix_len] - '0';
            if (index == 0)
            {
                continue;
            }
            count++;
        }
    }

    serverInfo->layerNums = count;
    closedir(dir);

    return 0;
}

int iss_osd_init(void)
{
    void *ptr = NULL;
    IHal_Dpu_InitStruct_t initStruct;
    IHal_Dpu_Handle_t *dpuHandle = NULL;
    struct IHal_MemHandle *memHandle = NULL;

    ptr = malloc(sizeof(struct OsdServer));
    if (ptr == NULL)
    {
        LOGE("error malloc mem to server handler\n");
        exit(-1);
    }

    memset(ptr, 0, sizeof(struct OsdServer));

    initStruct.num_outmem = 4;
    dpuHandle = IHal_Dpu_Init(&initStruct);
    if (dpuHandle == NULL)
    {
        LOGE("error init dpu handler\n");
        exit(-1);
    }


    g_osd_server_handler = (struct OsdServer *)ptr;
    g_osd_server_handler->dpuHandle = dpuHandle;

    g_osd_server_handler->frame.dpu_func = DPU_OSD_Func;
    g_osd_server_handler->frame.func_desc.osd_frame.osd_flags = 0;
    g_osd_server_handler->frame.func_desc.osd_frame.outWidth = SCREEN_WIDTH;
    g_osd_server_handler->frame.func_desc.osd_frame.outHeight = SCREEN_HEIGHT;
    g_osd_server_handler->frame.func_desc.osd_frame.wback_enable = 1;
    g_osd_server_handler->frame.func_desc.osd_frame.wback_fmt = SCREEN_FMT;

    g_osd_server_handler->layerBits = 0x0;
    g_osd_server_handler->orderBits = 0x0;
    g_osd_server_handler->clientNum = 0;
    INIT_LIST_HEAD(&g_osd_server_handler->clients_list);
    INIT_LIST_HEAD(&g_osd_server_handler->flush_list);
    pthread_rwlock_init(&g_osd_server_handler->clients_rwlock, NULL);
    pthread_rwlock_init(&g_osd_server_handler->flush_rwlock, NULL);

    pthread_rwlock_init(&g_dead_rwlock, NULL);
    INIT_LIST_HEAD(&g_dead_list);
    check_dev_fb_num(g_osd_server_handler);

    memHandle = IHal_MemInit(OSD_SERVICE_NAME);
    if (memHandle == NULL)
    {
        LOGE("error init mem handler\n");
        exit(-1);
    }
    g_osd_server_handler->memHandle = memHandle;

    g_temp_frame = malloc(SCREEN_WIDTH * SCREEN_HEIGHT * 4);
    LOGD("%s init ok!!!\n", OSD_SERVICE_NAME);
    return 0;
}

int iss_osd_start(void)
{
    int32_t ret = 0;
    pthread_t tid;

    TIMER_INIT(timer, 10);

    ret = pthread_create(&tid, NULL, osd_main_thread, NULL);
    if (ret != 0)
    {
        LOGE("create osd_main_thread error!!!\n");
        exit(-1);
    }

    return ret;
}

int osd_manager_stop(void)
{

    free(g_temp_frame);

    return 0;
}

void osd_clientNum_increase()
{
    g_osd_server_handler->clientNum++;
}

void osd_clientNum_decrease()
{
    g_osd_server_handler->clientNum--;
}

int assign_priority(enum ClientType type)
{
    int32_t i = 0;
    int32_t index = 0;
    int32_t orderBits = 0;
    int32_t used_bits = -1;
    int32_t get_order_flag = 0;
    struct OsdClientDetail *temp = NULL;

    orderBits = g_osd_server_handler->orderBits;
    LOGD("in func %s, manager orderBits %#x\n", __func__, orderBits);

    if (orderBits == 0)
    {
        used_bits = DPU_OSD_Order3;
        g_osd_server_handler->orderBits |= (1 << used_bits);

        return used_bits;
    }

    orderBits = 0;
    list_for_each_entry(temp, &g_osd_server_handler->clients_list, Entry)
    {
        pthread_mutex_lock(&temp->Lock);
        if (temp->Type >= type)
        {
            orderBits |= (1 << temp->Attr.osd_order);
            pthread_mutex_unlock(&temp->Lock);
            continue;
        }
        if (!get_order_flag)
        {
            get_order_flag = 1;
            used_bits = temp->Attr.osd_order;
        }
        temp->Attr.osd_order -= 1;
        orderBits |= (1 << temp->Attr.osd_order);
        pthread_mutex_unlock(&temp->Lock);
    }

    if (!get_order_flag)
    {
        index = __builtin_ffs(orderBits);
        used_bits = index - 2;
        orderBits |= (1 << used_bits);
    }

    if (used_bits == -1)
    {
        return -1;
    }
    orderBits |= (1 << used_bits);
    g_osd_server_handler->orderBits = orderBits;
    LOGD("in func %s, manager orderBits will set to %#x\n", __func__, orderBits);

    return used_bits;
}

void release_priority_and_del_clientDetail(struct OsdClientDetail *clientInfo)
{
    int32_t i = 0;
    int32_t find_flag = 0;
    int32_t orderBits = 0;
    int32_t used_bits = 0;
    struct OsdClientDetail *temp = NULL;

    RWLOCK_WRLOCK(&g_osd_server_handler->clients_rwlock);
    list_for_each_entry(temp, &g_osd_server_handler->clients_list, Entry)
    {
        pthread_mutex_lock(&clientInfo->Lock);
        if (temp == clientInfo)
        {
            find_flag = 1;
            LOGD("find_flag = %d\n", find_flag);
            pthread_mutex_unlock(&clientInfo->Lock);
            continue;
        }
        LOGD("find_flag = %d\n", find_flag);
        if (find_flag)
        {
            temp->Attr.osd_order += 1;
        }
        orderBits |= (1 << temp->Attr.osd_order);
        pthread_mutex_unlock(&clientInfo->Lock);
    }

    list_del(&clientInfo->Entry);
    osd_clientNum_decrease();

    LOGD("orderBits %#x\n", g_osd_server_handler->orderBits);
    LOGD("will change to %#x\n", orderBits);
    g_osd_server_handler->orderBits = orderBits;

    RWLOCK_UNLOCK(&g_osd_server_handler->clients_rwlock);

    return;
}

int allocation_layer(struct OsdClientDetail *clientInfo)
{
    int32_t i = 0;
    int32_t index = 0;
    int32_t layerNums = 0;
    int32_t layerBits =  0;
    int32_t layerOrder = 0;

    RWLOCK_WRLOCK(&g_osd_server_handler->clients_rwlock);
    layerNums = g_osd_server_handler->layerNums;
    layerBits = g_osd_server_handler->layerBits;
    LOGD("in func %s, layerNums = %d layerBits = %#x\n", __func__, layerNums, layerBits);

    index = __builtin_ffs(~layerBits);

    if (index > layerNums)
    {
        RWLOCK_UNLOCK(&g_osd_server_handler->clients_rwlock);
        return -1;
    }

    layerOrder = assign_priority(clientInfo->Type);
    if (layerOrder == -1)
    {
        RWLOCK_UNLOCK(&g_osd_server_handler->clients_rwlock);
        return -2;
    }

    layerBits |= (1 << (index - 1));
    g_osd_server_handler->layerBits = layerBits;

    LOGD("layerBits = %#x\n", layerBits);

    RWLOCK_UNLOCK(&g_osd_server_handler->clients_rwlock);

    pthread_mutex_lock(&clientInfo->Lock);

    clientInfo->LayerId = layerId[index - 1];

    clientInfo->Attr.osd_order = layerOrder;

    pthread_mutex_unlock(&clientInfo->Lock);

    LOGD("in func %s, client(%d) alloc %d layer layerOrder %d\n", __func__, clientInfo->ClientPid, layerId[index - 1], layerOrder);

    return i;
}

void free_layer(struct OsdClientDetail *clientInfo)
{
    int32_t i = 0;
    int32_t index = 0;
    int32_t layerNums = 0;
    int32_t layerBits =  0;
    int32_t layerid = 0;
    int32_t layerorder = 0;

    RWLOCK_WRLOCK(&g_osd_server_handler->clients_rwlock);
    layerNums = g_osd_server_handler->layerNums;
    layerBits = g_osd_server_handler->layerBits;
    layerid = clientInfo->LayerId;
    layerorder = clientInfo->Attr.osd_order;

    layerBits &= (~(clientInfo->LayerId));

    g_osd_server_handler->layerBits = layerBits;

    g_osd_server_handler->frame.func_desc.osd_frame.osd_flags &= (~(clientInfo->LayerId));

    index = get_layerId_index(clientInfo->LayerId);

    RWLOCK_UNLOCK(&g_osd_server_handler->clients_rwlock);

    LOGD("in func %s, client(%d) free Layer %d \n", __func__, clientInfo->ClientPid, layerid);
    return;
}

void *alloc_mem(int32_t size, uint32_t nmemb, IMPP_BufferInfo_t *out_info)
{
    void *dataBuffer = NULL;

    if (out_info == NULL)
    {
        LOGE("in %s, paramter out_info is NULL!!!\n", __func__);
        return NULL;
    }

    RWLOCK_RDLOCK(&g_osd_server_handler->clients_rwlock);

    dataBuffer = IHal_MemAlloc(g_osd_server_handler->memHandle, (int32_t)size * nmemb, out_info);

    RWLOCK_UNLOCK(&g_osd_server_handler->clients_rwlock);

    return dataBuffer;
}

int free_mem(IMPP_BufferInfo_t buffer_info)
{
    int32_t retval = 0;

    RWLOCK_RDLOCK(&g_osd_server_handler->clients_rwlock);

    retval = IHal_MemFree(g_osd_server_handler->memHandle, buffer_info);

    RWLOCK_UNLOCK(&g_osd_server_handler->clients_rwlock);

    return retval;
}

int trigger_frame(void *arg)
{
    int32_t need_flush = 0;
    pid_t client_pid = 0;
    struct OsdClientDetail *clientInfo = arg;

    pthread_mutex_lock(&clientInfo->Lock);
    need_flush = clientInfo->need_flush;
    client_pid = clientInfo->ClientPid;
    pthread_mutex_unlock(&clientInfo->Lock);

    if (need_flush)
    {
        if (check_flush_list_client(client_pid))
        {
            return 0;
        }
        else
        {
            add_to_flush_list(clientInfo);
        }
    }

    return 0;
}


int iss_osd_register_client(pid_t clientPid, tBinderIo* recived_msg, tBinderIo* reply)
{
    uint32_t ret = 0;
    int32_t timer_fd = 0;
    int32_t clientHandler = 0;
    enum ClientType type = 0;
    struct itimerspec itimespec;
    struct DeadEntry *deadEntry = NULL;
    struct OsdClientDetail *clientInfo = NULL;
    tIpcThreadInfo * ti = binder_get_thread_info();

    type = (enum ClientType)binder_io_get_uint32(recived_msg);
    clientHandler = binder_io_get_ref(recived_msg, 0);
    clientInfo = find_and_add_references_clientDetail(clientPid, type);
    if (clientInfo != NULL)
    {
        ret = PARAM_ERROR;
        binder_io_append_uint32(reply, ret);
        LOGE("in func %s, this pid(%d) type(%d) is registered!!!\n", __func__, clientPid, type);
        return -1;
    }

    clientInfo = malloc(sizeof(struct OsdClientDetail));
    if (clientInfo == NULL)
    {
        ret = NO_HEAPMEM;
        binder_io_append_uint32(reply, ret);
        LOGE("in func %s, alloc clientInfo mem failed!!!\n", __func__);
        return -1;
    }

    memset(clientInfo, 0, sizeof(struct OsdClientDetail));

    clientInfo->Attr.alpha = 255;
    clientInfo->ref_count = 0;
    clientInfo->ClientPid = clientPid;
    clientInfo->ClientHandler = clientHandler;
    clientInfo->Type = type;

    pthread_mutex_init(&clientInfo->Lock, NULL);
    pthread_cond_init(&clientInfo->Cond, NULL);

    INIT_LIST_HEAD(&clientInfo->Entry);
    INIT_LIST_HEAD(&clientInfo->FlushEntry);
    INIT_LIST_HEAD(&clientInfo->DeadClientEntry);

    deadEntry = find_deadEntry_in_dead_list(clientPid);
    if (deadEntry != NULL)
    {
        add_client_to_dead_entry(deadEntry, clientPid, clientInfo);
    }
    else
    {
        if (clientHandler == 0)
        {
            ret = NO_HANDLER;
            binder_io_append_uint32(reply, ret);
            LOGE("in func %s, get client handler failed!!!\n", __func__);
            goto no_handler;
        }
        else
        {
            binder_cmd_acquire(ti, clientHandler);
	        flush_commands(ti);

            struct binder_death *dead_cb = malloc(sizeof(struct binder_death));
            if (dead_cb == NULL)
            {
                ret = NO_HEAPMEM;
                binder_io_append_uint32(reply, ret);
                LOGE("in func %s, alloc dead_cb mem failed!!!\n", __func__);
                goto no_heapmem;
            }

            deadEntry = malloc(sizeof(struct DeadEntry));
            if (deadEntry == NULL)
            {
                ret = NO_HEAPMEM;
                free(dead_cb);
                binder_io_append_uint32(reply, ret);
                LOGE("in func %s, alloc DeadEntry mem failed!!!\n", __func__);
                goto no_heapmem;
            }

            memset(deadEntry, 0, sizeof(struct DeadEntry));
            pthread_mutex_init(&deadEntry->Lock, NULL);
            INIT_LIST_HEAD(&deadEntry->DeadList);
            INIT_LIST_HEAD(&deadEntry->ClientList);

            add_client_to_dead_entry(deadEntry, clientPid, clientInfo);
            add_deadEntry_to_dead_list(deadEntry);

            memset(dead_cb, 0, sizeof(struct binder_death));
	        dead_cb->death_cb = client_dead;
	        dead_cb->ptr = deadEntry;

            LOGD("in func %s, register dead_cb!!!\n", __func__);
            binder_cmd_link_to_death(ti, clientHandler, (void *)(dead_cb));
	        flush_commands(ti);

            pthread_mutex_lock(&deadEntry->Lock);
            deadEntry->DeathInfo = dead_cb;
            deadEntry->ClientHandler = clientHandler;
            pthread_mutex_unlock(&deadEntry->Lock);
        }
    }

    ret = allocation_layer(clientInfo);
    if (ret == -1 || ret == -2)
    {
        ret = ((ret == -1) ? NO_USABLE_LAYER : NO_AVAILABLE_PRIORITY);
        binder_io_append_uint32(reply, ret);
        LOGE("in func %s, %s layer!!!\n", __func__, ((ret == -1) ? "no usable" : "no available priority"));
        goto no_usable_layer;
    }
    add_clientDetail(clientInfo);

    itimespec.it_value.tv_sec = 0;
    itimespec.it_value.tv_nsec = 5;
    itimespec.it_interval.tv_sec = 0;
    itimespec.it_interval.tv_nsec = 33333333;
    timer_fd = TIMER_ADD(timer, &itimespec, -1, (timer_callback_t)trigger_frame, clientInfo);
    clientInfo->timer_fd = timer_fd;

    binder_io_append_uint32(reply, SUCCESS);
    LOGD("in func %s, iss_osd_register_client client(%d) type(%d) handler(%d) SUCCESS!!!\n", __func__, clientPid, type, clientHandler);
    return 0;

no_usable_layer:
    free(deadEntry->DeathInfo);
    free(deadEntry);
no_heapmem:
no_handler:
    free(clientInfo);
    return -1;
}

int iss_osd_unregister_client(pid_t clientPid, tBinderIo* recived_msg, tBinderIo* reply)
{
    uint32_t ret = 0;
    uint32_t type = 0;
    struct DeadEntry *deadEntry = NULL;
    struct OsdClientDetail *clientInfo = NULL;
    tIpcThreadInfo * ti = binder_get_thread_info();

    type = binder_io_get_uint32(recived_msg);

    clientInfo = find_and_add_references_clientDetail(clientPid, type);
    if (clientInfo == NULL)
    {
        ret = NO_CLIENT;
        binder_io_append_uint32(reply, ret);
        LOGE("in func %s, this pid(%d) is not registered!!!\n", __func__, clientPid);
        goto no_client;
    }

    release_priority_and_del_clientDetail(clientInfo);

    TIMER_DEL(timer, clientInfo->timer_fd);

    free_layer(clientInfo);

    deadEntry = find_deadEntry_in_dead_list(clientPid);
    del_client_from_dead_entry(deadEntry, clientInfo);

    if (is_dead_entry_empty(deadEntry))
    {
        
        binder_cmd_unlink_to_death(ti, deadEntry->ClientHandler, deadEntry->DeathInfo);
        binder_cmd_release(ti, deadEntry->ClientHandler);
        del_deadEntry_from_dead_list(deadEntry);
        free(deadEntry->DeathInfo);
        free(deadEntry);
    }

    release_clientDetail(clientInfo);

	IHal_Dpu_DmmuReleaseAll(g_osd_server_handler->dpuHandle);
    pthread_mutex_destroy(&clientInfo->Lock);
    free(clientInfo);
    binder_io_append_uint32(reply, SUCCESS);
    LOGD("in func %s, iss_osd_unregister_client client(%d) type(%d) SUCCESS!!!\n", __func__, clientPid, type);
    return 0;
no_client:
    return -1;
}

int iss_osd_alloc_dispmem(pid_t clientPid, tBinderIo* recived_msg, tBinderIo* reply)
{
    int32_t i = 0;
    uint32_t ret = 0;
    uint32_t size = 0;
    uint32_t type = 0;
    uint32_t nmemb = 0;
    uint32_t max_nmemb = 0;
    void *ptr = NULL;
    char *dataBuffer = NULL;
    IMPP_BufferInfo_t buffer_info;
    struct OsdClientDetail *clientInfo = NULL;

    type = binder_io_get_uint32(recived_msg);

    clientInfo = find_and_add_references_clientDetail(clientPid, type);
    if (clientInfo == NULL)
    {
        ret = NO_CLIENT;
        binder_io_append_uint32(reply, ret);
        LOGE("in func %s, this pid(%d) is not registered!!!\n", __func__, clientPid);
        goto no_client;
    }

    size = binder_io_get_uint32(recived_msg);
    nmemb = binder_io_get_uint32(recived_msg);

    memset(&buffer_info, 0, sizeof(IMPP_BufferInfo_t));
    dataBuffer = alloc_mem((int32_t)size, nmemb, &buffer_info);
    if (dataBuffer == NULL)
    {
        ret = NO_IPCMEM;
        binder_io_append_uint32(reply, ret);
        LOGE("in func %s, not have enough ipc mem!!!\n", __func__, clientPid);
        goto no_ipcmem;
    }
	IHal_Dpu_DmmuOps(g_osd_server_handler->dpuHandle,&buffer_info,true);

    pthread_mutex_lock(&clientInfo->Lock);

    clientInfo->desc = malloc(sizeof(struct MemDesc) * nmemb);
    clientInfo->MemSize = size * nmemb;
    clientInfo->MemDescNum = nmemb;

    for (i = 0; i < nmemb; i++)
    {
	clientInfo->desc[i].Index = i;
    	clientInfo->desc[i].IpcDmaFd = buffer_info.fd;
    	clientInfo->desc[i].MemSize = size;
    	clientInfo->desc[i].IpcMemPaddr = buffer_info.paddr + i * size;
    	clientInfo->desc[i].IpcMemVaddr = buffer_info.vaddr + i * size;
    }
    clientInfo->Attr.vaddr = clientInfo->desc[0].IpcMemVaddr;

    binder_io_append_uint32(reply, SUCCESS);
    binder_io_append_fd(reply, clientInfo->desc[0].IpcDmaFd);

    client_ref_decrease(clientInfo);
    pthread_mutex_unlock(&clientInfo->Lock);

    LOGD("in func %s, client(%d) alloc ipcmem size(%d) success!!!\n", __func__, clientInfo->ClientPid, size);
    return 0;
no_ipcmem:
no_client:
    return -1;
}

int iss_osd_free_dispmem(pid_t clientPid, tBinderIo* recived_msg, tBinderIo* reply)
{
    int32_t retval = 0;
    uint32_t ret = 0;
    uint32_t type = 0;
    uint32_t size = 0;
    char *dataBuffer = NULL;
    IMPP_BufferInfo_t buffer_info;
    struct OsdClientDetail *clientInfo = NULL;

    memset(&buffer_info, 0, sizeof(IMPP_BufferInfo_t));

    type = binder_io_get_uint32(recived_msg);

    clientInfo = find_and_add_references_clientDetail(clientPid, type);
    if (clientInfo == NULL)
    {
        ret = NO_CLIENT;
        binder_io_append_uint32(reply, ret);
        LOGE("in func %s, this pid(%d) is not registered!!!\n", __func__, clientPid);
        goto no_client;
    }

    pthread_mutex_lock(&clientInfo->Lock);

    buffer_info.fd = clientInfo->desc[0].IpcDmaFd;
    buffer_info.size = clientInfo->MemSize;
    buffer_info.paddr = clientInfo->desc[0].IpcMemPaddr;
    buffer_info.vaddr = clientInfo->desc[0].IpcMemVaddr;
    retval = free_mem(buffer_info);
    if (retval != 0)
    {
        ret = BUF_INFO_ERROR;
        binder_io_append_uint32(reply, ret);
        LOGE("in func %s, IHal_MemFree paramter IMPP_BufferInfo_t error\n", __func__, clientPid);
    	client_ref_decrease(clientInfo);
        pthread_mutex_unlock(&clientInfo->Lock);
        goto buf_info_error;
    }

    clientInfo->MemSize = 0;
    clientInfo->MemDescNum = 0;
    free(clientInfo->desc);
    clientInfo->desc = NULL;

    client_ref_decrease(clientInfo);
    pthread_mutex_unlock(&clientInfo->Lock);

    binder_io_append_uint32(reply, SUCCESS);
    return 0;

buf_info_error:
no_client:
    return -1;

}

int check_necessaryParameters(struct OsdClientDetail *clientInfo, uint8_t frame[])
{
    void *ptr = NULL;
    void *uv = NULL;
    int32_t fmt = 0;
    int32_t width = 0;
    int32_t height = 0;
    void *IpcMemVaddr = NULL;
    struct tile420_fmt tile420_buf;
    struct nv12_fmt nv12_buf;

    ptr = clientInfo->Attr.vaddr;
    fmt = clientInfo->ImageAttr.srcFmt;
    width = clientInfo->ImageAttr.srcWidth;
    height = clientInfo->ImageAttr.srcHeight;

    if (ptr == NULL)
    {
        LOGE("in func %s, clientInfo->Attr.vaddr or clientInfo->desc[0].IpcMemVaddr is NULL, is must be used\n", __func__);
        return -1;
    }

    if (fmt == 0 && (width == 0 || height == 0))
    {
        LOGE("fmt = %#x width = %d height = %d\n", fmt, width, height);
        LOGE("in func %s, The srcWidth, srcHeight, and srcFmt of image parameters must not be zero\n", __func__);
        return -1;
    }

    if (fmt == IMPP_PIX_FMT_NV12)
    {
        uv = ptr + (width * height);

        clientInfo->Attr.uv_vaddr = uv;
    }
    else if (fmt == IMPP_PIX_FMT_JZ420B)
    {
	clientInfo->Attr.srcFmt = IMPP_PIX_FMT_NV12;
	tile420_buf.width =  clientInfo->ImageAttr.srcWidth;
	tile420_buf.stride =(clientInfo->ImageAttr.srcWidth + 15) / 16 *  16;
	tile420_buf.height = clientInfo->ImageAttr.srcHeight;
	tile420_buf.y = frame;
	tile420_buf.uv = &frame[640 * 480];

	nv12_buf.width = clientInfo->ImageAttr.srcWidth;
	nv12_buf.stride = (nv12_buf.width + 15) / 16 * 16;
	nv12_buf.height = clientInfo->ImageAttr.srcHeight;
	nv12_buf.d = (void*)g_temp_frame;

	IHal_Convert_Tile420ToNV12(tile420_buf, &nv12_buf);

	clientInfo->Attr.vaddr = g_temp_frame;
        uv = g_temp_frame + (width * height);
        clientInfo->Attr.uv_vaddr = uv;
    }

    return 0;
}

int iss_osd_flush_layer_data(pid_t clientPid, tBinderIo* recived_msg, tBinderIo* reply)
{
    int32_t retval = 0;
    int32_t size = 0;
    uint32_t type = 0;
    int32_t index = 0;
    uint32_t ret = 0;
    IMPP_BufferInfo_t info;
    struct MemDesc *buffer = NULL;
    struct OsdClientDetail *clientInfo = NULL;

    type = binder_io_get_uint32(recived_msg);

    size = binder_io_get_uint32(recived_msg);
    index = binder_io_get_uint32(recived_msg);	

    clientInfo = find_and_add_references_clientDetail(clientPid, type);
    if (clientInfo == NULL)
    {
        ret = NO_CLIENT;
        binder_io_append_uint32(reply, ret);
        LOGE("in func %s, this pid(%d) is not registered!!!\n", __func__, clientPid);
        goto no_client;
    }


    pthread_mutex_lock(&clientInfo->Lock);

    buffer = &clientInfo->desc[index];
    info.fd = buffer->IpcDmaFd;
    IHal_MemFlush(g_osd_server_handler->memHandle, info, MEM_FLUSH_START);
    clientInfo->Attr.vaddr = buffer->IpcMemVaddr;
    retval = check_necessaryParameters(clientInfo, clientInfo->Attr.vaddr);
    if (retval != 0 )
    {
        ret = MISSING_NECESSARY_PARAMETERS;
        binder_io_append_uint32(reply, ret);
        LOGE("in func %s, missing necessary paramters\n", __func__);
        goto missing_necessary_parameters;
    }
    IHal_MemFlush(g_osd_server_handler->memHandle, info, MEM_FLUSH_END);
    clientInfo->need_flush = 1;
    pthread_cond_wait(&clientInfo->Cond, &clientInfo->Lock);

    client_ref_decrease(clientInfo);
    pthread_mutex_unlock(&clientInfo->Lock);

    binder_io_append_uint32(reply, SUCCESS);
    return 0;
missing_necessary_parameters:
    client_ref_decrease(clientInfo);
no_client:
    return -1;
}

int iss_osd_parase_attr(int32_t typeParase, pid_t clientPid, tBinderIo* recived_msg, tBinderIo* reply)
{
    int32_t ret = 0;
    uint32_t type = 0;
    int32_t msgSize = 0;
    char *data = NULL;
    struct SrcAttr imageAttr;
    struct DisplayAttr dispAttr;
    struct OsdClientDetail *clientInfo = NULL;

    LOGD("in func %s, client(%d) iss_osd_parase_attr\n", __func__, clientPid);

    memset(&imageAttr, 0, sizeof(struct SrcAttr));
    memset(&dispAttr, 0, sizeof(struct DisplayAttr));

    type = binder_io_get_uint32(recived_msg);

    clientInfo = find_and_add_references_clientDetail(clientPid, type);
    if (clientInfo == NULL)
    {
        ret = NO_CLIENT;
        binder_io_append_uint32(reply, ret);
        LOGE("in func %s, this pid(%d) is not registered!!!\n", __func__, clientPid);
        goto no_client;
    }

    ret = binder_io_get_data(recived_msg, &data, &msgSize);
    if(ret || msgSize <= 0)
    {
        ret = PARAM_ERROR;
        client_ref_decrease(clientInfo);
        binder_io_append_uint32(reply, ret);
        LOGE("in func %s, can't get image attr\n", __func__);
        goto param_error;
    }

    pthread_mutex_lock(&clientInfo->Lock);

    switch(typeParase)
    {
        case PARSE_IMAGEATTR:
            memcpy(&imageAttr, data, msgSize);
            LOGD("imageAttr.srcFmt = %#x imageAttr.srcWidth = %d\
            imageAttr.srcHeight = %d\n", imageAttr.srcFmt,\
            imageAttr.srcWidth, imageAttr.srcHeight);

            if (clientInfo->LayerId == Dpu_Layer2 || clientInfo->LayerId == Dpu_Layer3)
            {
                if (imageAttr.srcFmt == IMPP_PIX_FMT_NV12)
                {
                    ret = NOT_SUPPORT_SRC_FMT;
                    client_ref_decrease(clientInfo);
                    binder_io_append_uint32(reply, ret);
                    LOGE("in func %s, layer2 and layer3 not support nv12\n", __func__);
                    goto not_support_src_fmt;
                }
            }

            clientInfo->Attr.srcFmt = imageAttr.srcFmt;
            clientInfo->Attr.srcWidth = imageAttr.srcWidth;
            clientInfo->Attr.srcHeight = imageAttr.srcHeight;

	    clientInfo->ImageAttr.srcFmt = imageAttr.srcFmt;
            clientInfo->ImageAttr.srcWidth = imageAttr.srcWidth;
            clientInfo->ImageAttr.srcHeight = imageAttr.srcHeight;

            break;
        case PARSE_DISPLAYATTR:
            memcpy(&dispAttr, data, msgSize);
            LOGD("dispAttr.scaleWidth = %d dispAttr.scaleHeight = %d\
            dispAttr.posX = %d    dispAttr.posY = %d\
            dispAttr.alpha = %d\n", dispAttr.scaleWidth, dispAttr.scaleHeight,\
            dispAttr.posX, dispAttr.posY, dispAttr.alpha);

            clientInfo->Attr.scaleWidth = dispAttr.scaleWidth;
            clientInfo->Attr.scaleHeight = dispAttr.scaleHeight;
            if (dispAttr.scaleWidth != 0 || dispAttr.scaleHeight != 0)
            {
                clientInfo->Attr.scale_enable = true;
            }
            clientInfo->Attr.osd_posX = dispAttr.posX;
            clientInfo->Attr.osd_posY = dispAttr.posY;
            clientInfo->Attr.alpha = dispAttr.alpha;

            break;
    }

    client_ref_decrease(clientInfo);
    pthread_mutex_unlock(&clientInfo->Lock);

    LOGD("in func %s, client(%d) iss_osd_parase_attr success!!!\n", __func__, clientPid);
    binder_io_append_uint32(reply, SUCCESS);
    return 0;
not_support_src_fmt:
    pthread_mutex_unlock(&clientInfo->Lock);
param_error:
no_client:
    return -1;

}

int iss_osd_set_frame_rate(pid_t clientPid, tBinderIo* recived_msg, tBinderIo* reply)
{
    int32_t ret = 0;
    uint32_t type = 0;
    int32_t msgSize = 0;
    char *data = NULL;
    int32_t frame_rate = 0;
    struct itimerspec itimespec;
    struct OsdClientDetail *clientInfo = NULL;

    LOGD("in func %s, client(%d) iss_osd_set_frame_rate\n", __func__, clientPid);

    type = binder_io_get_uint32(recived_msg);

    frame_rate = binder_io_get_uint32(recived_msg);
    if (frame_rate > 60)
    {
        ret = NOT_SUPPORT_FRAME_RATE;
        binder_io_append_uint32(reply, ret);
        LOGE("in func %s, server not support frame rate > 60!!!\n", __func__);
        goto not_support_frame_rate;
    }

    clientInfo = find_and_add_references_clientDetail(clientPid, type);
    LOGD("in func %s, client(%d) find_and_add_references_clientDetail success\n", __func__, clientPid);
    if (clientInfo == NULL)
    {
        ret = NO_CLIENT;
        binder_io_append_uint32(reply, ret);
        LOGE("in func %s, this pid(%d) is not registered!!!\n", __func__, clientPid);
        goto no_client;
    }

    pthread_mutex_lock(&clientInfo->Lock);

    TIMER_DEL(timer, clientInfo->timer_fd);

    itimespec.it_value.tv_sec = 0;
    itimespec.it_value.tv_nsec = 5;
    itimespec.it_interval.tv_sec = 0;
    itimespec.it_interval.tv_nsec = (( 1000000000 / frame_rate));

    clientInfo->timer_fd = TIMER_ADD(timer, &itimespec, -1, (timer_callback_t)trigger_frame, clientInfo);

    client_ref_decrease(clientInfo);
    pthread_mutex_unlock(&clientInfo->Lock);

    LOGD("in func %s, client(%d) frame_rate(%d) iss_osd_set_frame_rate success!!!\n", __func__, clientPid, frame_rate);
    binder_io_append_uint32(reply, SUCCESS);
    return 0;
no_client:
not_support_frame_rate:
    return -1;

}

static int iss_osd_recevied_msg_cb(uint32_t code, tBinderIo* msg, tBinderIo* reply, uint32_t flag)
{
    tIpcThreadInfo * ti = binder_get_thread_info();
    pid_t clientPid = 0;
    uint32_t cmdCode = 0;

    clientPid = binder_io_get_uint32(msg);
    cmdCode = binder_io_get_uint32(msg);

    //LOGD("in func %s, received msg form pid(%d) cmd(%#x)!!!\n", __func__, clientPid, cmdCode);
    switch (cmdCode)
    {
        case REGISTER_CLIENT:
            iss_osd_register_client(clientPid, msg, reply);
            break;
        case UNREGISTER_CLIENT:
            iss_osd_unregister_client(clientPid, msg, reply);
            break;
        case ALLOC_DISPMEM:
            iss_osd_alloc_dispmem(clientPid, msg, reply);
            break;
        case FREE_DISPMEM:
            iss_osd_free_dispmem(clientPid, msg, reply);
            break;
        case FLUSH_LAYER_DATA:
            iss_osd_flush_layer_data(clientPid, msg, reply);
            break;
        case PARSE_IMAGEATTR:
            iss_osd_parase_attr(PARSE_IMAGEATTR, clientPid, msg, reply);
            break;
        case PARSE_DISPLAYATTR:
            iss_osd_parase_attr(PARSE_DISPLAYATTR, clientPid, msg, reply);
            break;
        case SET_FRAME_RATE:
            iss_osd_set_frame_rate(clientPid, msg, reply);
            break;
        default:
            LOGE("the cmdcode %#x is not exist!!!\n", cmdCode);
            break;
    }

    return 0;
}

static tBinderService osdserver = {
    .transact_cb = iss_osd_recevied_msg_cb,
    .link_to_death_cb = NULL,
    .unlink_to_death_cb = NULL,
    .death_notify_cb = NULL,
};

int main()
{
    iss_osd_init();
    iss_osd_start();

	binder_add_service(OSD_SERVICE_NAME, &osdserver);
	binder_thread_enter_loop(1, 0);

	while (1) {
		sleep(2);
	}
}
