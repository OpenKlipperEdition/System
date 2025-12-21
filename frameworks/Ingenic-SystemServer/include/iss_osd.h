#ifndef __OSD_CLIENT_H__
#define __OSD_CLIENT_H__
#include "iss_common.h"
/**
 * @brief 客户端类型
 */
typedef enum
{
    OTHER = 1,
    UI,
    VIDEO,
}ClientType_t;

/**
 * @brief 远程返回的调用结果码
 */
enum
{
    SUCCESS = 0,
    NO_HANDLER,
    NO_USABLE_LAYER,
    NO_CLIENT,
    NO_IPCMEM,
    NO_HEAPMEM,
    BUF_INFO_ERROR,
    PARAM_ERROR,
    MISSING_NECESSARY_PARAMETERS,
};



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

/**
 * @brief   创建客户端句柄，并注册到server端
 * @param   需要创建的客户端类型
 * @retval  创建成功返回客户端句柄地址，失败返回NULL，并打印失败原因。
 */
struct OsdClient *ISS_CreateOsdClt(ClientType_t type);

/**
 * @brief   销毁客户端句柄，并向服务端发送销毁命令
 * @param   客户端句柄
 * @retval  成功返回0，失败返回负值，并打印失败原因。
 */
int32_t ISS_DestoryOsdClt(struct OsdClient *clt);

/**
 * @brief   申请ipc内存，跨进程传输显示数据的buffer
 * @param   客户端句柄
 * @param   申请的内存大小，一般为图像数据大小，如果宽为720 高为1280 格式为rgb565，大小应为 720 * 1280 * 4 = 3686400
 * @param   申请的内存块数量
 * @retval  成功返回0，失败返回负值，并打印失败原因。
 */
int32_t ISS_AllocDispMem(struct OsdClient *clt, int32_t size, int32_t nmemb);

/**
 * @brief   释放ipc内存
 * @param   客户端句柄
 * @retval  成功返回0，失败返回负值，并打印失败原因。
 */
int32_t ISS_FreeDispMem(struct OsdClient *clt);

/**
 * @brief   设置源图像参数
 * @param   客户端句柄
 * @param   图像参数
 * @retval  成功返回0，失败返回负值，并打印失败原因。
 */
int32_t ISS_SetImageAttr(struct OsdClient *clt, struct ImageAttr attr);

/**
 * @brief   设置显示参数
 * @param   客户端句柄
 * @param   显示参数
 * @retval  成功返回0，失败返回负值，并打印失败原因。
 */
int32_t ISS_SetDisplayAttr(struct OsdClient *clt, struct DisplayAttr attr);

/**
 * @brief   设置帧率，最大不超过60帧
 * @param   客户端句柄
 * @param   帧率
 * @retval  成功返回0，失败返回负值，并打印失败原因。
 */
int32_t ISS_SetFrameRate(struct OsdClient *clt, int32_t frame_rate);

/**
 * @brief   重新刷新图像
 * @param   客户端句柄
 * @param   源图像数据地址
 * @param   图像大小
 * @retval  成功返回0，失败返回负值，并打印失败原因。
 */
int32_t ISS_Flush(struct OsdClient *clt, void *addr, int32_t size);

#endif // !__OSD_CLIENT_H__
