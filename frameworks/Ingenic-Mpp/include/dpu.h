#ifndef __DPU_H__
#define __DPU_H__
#ifdef __cplusplus
extern "C" {
#endif
#include "impp.h"

/**
 *      模块说明：
 *      此模块是将DPU用于OSD图像叠加功能或者是CSC格式转换功能
 *      内核配置条件:
 *      1. 必须使能RDMA模式，且显示只能通过RDMA通路
 *      2. 叠加是通过Composer实现的，因此还需将Layer使能并导出至少两个,两层以上的叠加需要更多的导出
 */

/**
 * @defgroup group_Dpu Dpu模块
 * @{
 */

/**
 * @addtogroup group_Dpu_data_type 数据类型定义
 * @{
 */

/**
 * @brief DPU Layer标志
 */
typedef enum {
        Dpu_Layer0 = 0x01,
        Dpu_Layer1 = 0x02,
        Dpu_Layer2 = 0x04,
        Dpu_Layer3 = 0x08,
} IHal_Dpu_Flags_t;

/**
 * @brief DPU 功能类型
 */
typedef enum {
        DPU_OSD_Func = 0,       /*!< OSD功能目前此API软件只支持两层 */
        DPU_CSC_Func,           /*!< [暂不支持]CSC RGB->ARGB  / (NV12/NV21/YUV422)-->RGB */
} IHal_Dpu_Func_t;

/**
 * @brief DPU OSD叠加顺序
 */
typedef enum {
        DPU_OSD_Order0 = 0,     /*!< 最底层 */
        DPU_OSD_Order1,
        DPU_OSD_Order2,
        DPU_OSD_Order3,         /*!< 最顶层 */
} IHal_DpuOSD_Order_t;

/**
 * @brief DPU 颜色顺序
 */
enum {
        DPU_CFG_COLOR_RGB = 0,
        DPU_CFG_COLOR_RBG = 1,
        DPU_CFG_COLOR_GRB = 2,
        DPU_CFG_COLOR_GBR = 3,
        DPU_CFG_COLOR_BRG = 4,
        DPU_CFG_COLOR_BGR = 5,
};

/**
 * @brief DPU 初始化结构标志
 */
typedef struct {
        //IHAL_INT32 layer_flags;               /*!< LayerX初始化标志　*/
        IHAL_INT32 num_outmem;
} IHal_Dpu_InitStruct_t;

/**
 * @brief DPU OSD各层的属性参数
 */
typedef struct {
        IMPP_PIX_FMT srcFmt;            /*!< 源数据格式                                    */
        IHAL_INT32 srcWidth;            /*!< 源数据宽度　　　　　             */
        IHAL_INT32 srcHeight;           /*!< 源数据高度　　　　　             */
        IHAL_INT32 srcCropx;            /*!< crop start x */
        IHAL_INT32 srcCropy;            /*!< crop start y */
        IHAL_INT32 srcCropw;            /*!< crop size width */
        IHAL_INT32 srcCroph;            /*!< crop size height */
        bool scale_enable;              /*!< 使能缩放(最多支持两层)　        */
        IHAL_INT32 scaleHeight;         /*!< 缩放后的图像高度                   */
        IHAL_INT32 scaleWidth;          /*!< 缩放后的图像宽度                   */
        IHAL_INT32 osd_posX;            /*!< 叠加起始坐标X                                */
        IHAL_INT32 osd_posY;            /*!< 叠加起始坐标Y                                */
        IHAL_INT32 osd_order;           /*!< 叠加顺序                                       */
        IHAL_INT32 alpha;                       /*!< 透明度                                          */
        IHAL_UINT32 paddr;                      /*!< LayerBuffer的物理地址         */
        IHAL_UINT32 vaddr;                      /*!< 暂不支持TLB  */
        IHAL_UINT32 uv_vaddr;
        IHAL_UINT32 color_order;
} IHal_DpuOSD_LayerAttr_t;

/**
 * @brief DPU OSD帧结构描述
 */
typedef struct {
        IHAL_INT32 osd_flags;                           /*!< 各层叠加标志                                         */
        IHal_DpuOSD_LayerAttr_t layer[4];       /*!< 各层的参数                                                    */
        IHAL_INT32 wback_enable;                        /*!< 是否写回，如不写回则直接在LCD显示  */
        IMPP_PIX_FMT wback_fmt;                         /*!< 仅支持RGB888 ARGB8888 RGB565 RGB555 */
        IHAL_INT32 outWidth;                            /*!< 叠加后输出图像宽度                                        */
        IHAL_INT32 outHeight;                           /*!< 叠加后输出图像的高度                             */
} IHal_DpuOSD_FrameDesc_t;

/**
 * @brief DPU CSC参数
 */
typedef struct {
        IHAL_INT32 srcFmt;                      /*!< 源图像数据格式                              */
        IHAL_INT32 dstFmt;                      /*!< 输出图像数据格式                   */
        IHAL_INT32 srcWidth;            /*!< 源数据宽度　　　　　             */
        IHAL_INT32 srcHeight;           /*!< 源数据高度　　　　　             */
        bool scale_enable;                      /*!< 使能缩放                       　             */
        IHAL_INT32 dstWidth;            /*!< 输出数据宽度　　　　　          */
        IHAL_INT32 dstHeight;           /*!< 输出据高度　　　　　             */
        IHAL_UINT32 paddr;                      /*!< 源数据Buffer的物理地址，注意: 使用物理地址时， vaddr 需要设置为0 */
        IHAL_UINT32 vaddr;                      /*!< 如果使用TLB, vaddr 为应用malloc或者mmap的内存. paddr 需要设置为0 */
} IHal_DpuCSC_Attr_t;


// CSC support scale && csc
typedef struct {
        IHal_DpuCSC_Attr_t attr;
} IHal_DpuCSC_FrameDesc_t;

/**
 * @brief DPU 帧描述
 */
typedef struct {
        IHAL_INT32 dpu_func;                                    /*!< DPU功能选择            */
        union {
                IHal_DpuOSD_FrameDesc_t osd_frame;      /*!< OSD帧描述                       */
                IHal_DpuCSC_FrameDesc_t csc_frame;      /*!< CSC帧描述                       */
        } func_desc;
} IHal_Dpu_FrameDesc_t;

typedef void IHal_Dpu_Handle_t;

/**
 * @}
 */

/**
 * @addtogroup group_Dpu_API API定义
 * @{
 */

/**
 * @brief DPU功能初始化
 * @param [in] init : 初始化参数
 * @retval IHal_Dpu_Handle_t* 成功
 * @retval IHAL_RNULL              失败
 */
IHal_Dpu_Handle_t *IHal_Dpu_Init(IHal_Dpu_InitStruct_t *init);

/**
 * @brief  DPU功能去初始化
 * @param [in] handle  : DPU Habdle
 * @retval 0            成功
 * @retval 非0                 失败
 */
IHAL_INT32 IHal_Dpu_Deinit(IHal_Dpu_Handle_t *handle);

/**
 * @brief 获取用于屏幕显示的Buffer数量 (RDMA的buffer数量)
 * @param [in] handle : DPU Habdle
 * @retval num of buf   成功
 * @retval -IHAL_RERR   失败
 */
IHAL_INT32 IHal_Dpu_RDMA_Get_BufferNums(IHal_Dpu_Handle_t *handle);

/**
 * @brief 获取RDMA的帧Buffer
 * @param [in] handle : DPU Habdle
 * @param [out] buf   : 帧Buffer信息
 * @retval 0            成功
 * @retval 非0                 失败
 */
IHAL_INT32 IHal_Dpu_RDMA_GetFrame(IHal_Dpu_Handle_t *handle, IMPP_BufferInfo_t *buf);

/**
 * @brief 将帧Buffer归还给RDMA硬件，用于上屏显示
 * @param [in] handle : DPU Habdle
 * @param [in] buf   : 帧Buffer信息
 * @retval 0            成功
 * @retval 非0                 失败
 */
IHAL_INT32 IHal_Dpu_RDMA_PutFrame(IHal_Dpu_Handle_t *handle, IMPP_BufferInfo_t *buf);

/**
 * @brief 设置DPU(OSD/CSC)输出数据Buffer,支持DMA-Buf
 * @param [in] handle : DPU Habdle
 * @param [in] buf   : Buffer信息
 * @param [in] index   : Buffer的编号
 * @retval 0            成功
 * @retval 非0                 失败
 */
IHAL_INT32 IHal_Dpu_Composer_Set_WbackBuffers(IHal_Dpu_Handle_t *handle, IMPP_BufferInfo_t *buf, IHAL_INT32 index);

/**
 * @brief DPU(OSD/CSC)进行一次图像处理,更新composer配置.
 * @param [in] handle  : DPU Habdle
 * @param [in] frame   : 帧描述符
 * @retval 0            成功
 * @retval 非0                 失败
 */
IHAL_INT32 IHal_Dpu_Composer_Process(IHal_Dpu_Handle_t *handle, IHal_Dpu_FrameDesc_t *frame);

/**
 * @brief       获取DPU处理输出数据,writeback 使能时，可以获取composer叠加后的数据.
 * @param [in] handle  : DPU Habdle
 * @param [out] frame   : 输出数据描述
 * @retval 0            成功
 * @retval 非0                 失败
 */
IHAL_INT32 IHal_Dpu_Composer_GetFrame(IHal_Dpu_Handle_t *handle, IMPP_FrameInfo_t *frame);

/**
 * @brief       释放输出数据,writeback使能时，将frame的使用权交还给硬件。（与IHal_Dpu_Composer_GetFrame()配套使用.）
 * @param [in] handle  : DPU Habdle
 * @param [out] frame   : 输出数据描述
 * @retval 0            成功
 * @retval 非0                 失败
 */
IHAL_INT32 IHal_Dpu_Composer_ReleaseFrame(IHal_Dpu_Handle_t *handle, IMPP_FrameInfo_t *frame);


/**
 * @brief       等待vsync结束
 * @param [in] handle  : DPU Habdle
 * @retval 0            成功
 * @retval 非0                 失败
 */
IHAL_INT32 IHal_Dpu_WaitForVsync(IHal_Dpu_Handle_t *handle, int *vsync);

/**
 * @brief       user_addr DMMU映射/解除映射
 * @param [in] handle  : DPU Habdle
 * @param [in] buf  : user buffer 信息
 * @param [in] map : true(映射)/false(解除映射)
 * @retval 0            成功
 * @retval 非0          失败
 */
IHAL_INT32 IHal_Dpu_DmmuOps(IHal_Dpu_Handle_t *handle,IMPP_BufferInfo_t *buf,bool map);


IHAL_INT32 IHal_Dpu_DmmuReleaseAll(IHal_Dpu_Handle_t *handle);
/**
 * @}
 */

/**
 * @}
 */
#ifdef __cplusplus
} // extern "C"
#endif
#endif  // __DPU_H__
