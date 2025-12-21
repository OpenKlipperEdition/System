/**
 * @file ihal_info.h
 * @author ingenic-team
 * @brief
 * @version 1.0
 * @date 2023-10-12
 *
 * @copyright Copyright ingenic (c) 2023
 *
 */

#ifndef __IHAL_INFO_H__
#define __IHAL_INFO_H__


/**
 * @defgroup group_IHAL 通用数据类型定义
 * @{
 */

#define IHAL_ROK        0
#define IHAL_RERR       1
#define IHAL_RFAILED    2
#define IHAL_RNULL      NULL


/* ihal wait type */
#define IHAL_NO_WAIT            0                       /*!< 非阻塞等待   */
#define IHAL_WAIT_FOREVER   0xFFFFFFFF                  /*!< 阻塞等待     */

typedef char            IHAL_INT8;
typedef unsigned char   IHAL_UINT8;
typedef short           IHAL_INT16;
typedef unsigned short  IHAL_UINT16;
typedef int             IHAL_INT32;
typedef unsigned int    IHAL_UINT32;
typedef long long             IHAL_INT64;
typedef unsigned long long    IHAL_UINT64;


/**
 * @brief 缓冲区类型
 */
typedef enum __ihal_buffer_type {
        IHAL_INTERNAL_BUFFER,				/*!< 内部缓冲区           */
        IHAL_EXT_DMABUFFER,				/*!< 外部dma-buf          */
        IHAL_EXT_USERBUFFER,                            /*!< 外部用户空间缓冲区   */
} IHAL_BUFFER_TYPE;

/**
 * @brief 缓冲区信息
 */
typedef struct {
        IHAL_INT32 fd;                                  /*!< 用于dma-buf         */
        IHAL_UINT32 paddr;				/*!< 物理地址            */
        IHAL_UINT32 vaddr;                              /*!< 虚拟地址            */
        IHAL_UINT32 size;                               /*!< 缓冲区大小          */
        IHAL_UINT32 byteused;
        IHAL_INT32 index;
} IHAL_BufferInfo_t;

/**
 * @}
 */

#endif  // __IHAL_INFO_H__
