/**
 * @file codec.c
 * @author
 * @brief
 * @version 0.1
 * @date 2021-12-02
 *
 * @copyright Copyright ingenic 2021
 *
 */

#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sched.h>
#include <unistd.h>
#include <stdlib.h>

#include <lib_fpga/DmaAllocLinux.h>
#include <lib_common_enc/EncBuffers.h>
#include <lib_codec/lib_codec.h>
#include <lib_common/BufferStreamMeta.h>
#include <lib_codec/BufPool.h>
#include <lib_codec/codec_encoder.h>
#include <lib_fpga/DmaAlloc.h>

#include "codec.h"
#include "log.h"
#define TAG             "codec"

#define MAX_VIDEO_CHAN  8
#define MAX_DSTBUF_NUM  3
#define MAX_PACK_NUM    6
#define VCODEC_STA_RUNING   1
#define VCODEC_STA_STOP     0



static IHAL_UINT32 video_encoder_cnt = 0;

typedef struct __codec_buf_info {
        IHAL_UINT32 *vaddr;
        IHAL_UINT32 *paddr;
        IHAL_INT32  dmafd;
        IHAL_UINT32 size;
        IHAL_UINT32 index;
        AL_TBuffer  *AL_SrcTBuf;
} video_srcbuf_info_t;


typedef struct {
        IHAL_UINT32 seq;
        IHAL_UINT32 packcnt;
        EncoderPack pack[MAX_PACK_NUM];
} encoder_stream_t;

typedef struct __codec_dstbuf_info {
        IHAL_UINT32 *vaddr;
        IHAL_UINT32 size;
        IHAL_UINT32 index;
        IHAL_UINT32  uselength;
        encoder_stream_t stream;
} video_dstbuf_info_t;

typedef struct _codec_run_ctx {
        IHal_CodecParam param;
} video_codec_run_ctx_t;

typedef struct _codec_chan_ctx {
        video_codec_run_ctx_t runCtx;
        AL_CodecEncode          *pCodecEncode;
        AL_CodecParam           codecParam;
        IHAL_INT8               chn_idr_request;
        IHAL_INT8               roi_change_flag;
        IHAL_INT16              num_srcbuf;
        IHAL_INT16              num_dstbuf;
        IHAL_INT16              srcbuf_type;
        IHAL_INT16              dstbuf_type;
        IHAL_INT32              vcodec_run_sta;
        IHAL_UINT64             encoding_count;
        IHAL_UINT64             finishencoding_count;
        IHAL_EncoderRoiAttr             roiAttr[8];
        App_Fifo                dst_queued_FiFo;    //out stream fifo
        App_Fifo                dst_done_FiFo;
        App_Fifo                src_queued_FiFo;
        App_Fifo                src_done_FiFo;
        video_srcbuf_info_t     *srcbuf;
        video_dstbuf_info_t     *dstbuf;
        sem_t                   src_available_cnt_sem;
        sem_t                   dst_available_cnt_sem;
        pthread_mutex_t         mutex;
        pthread_t               encoderTid;
        pthread_t               frameTid;
} Video_CodecCtx_t;

typedef enum {
        IMPP_ENC_PIC_FORMAT_400_8BITS = 0x0088,
        IMPP_ENC_PIC_FORMAT_420_8BITS = 0x0188,
        IMPP_ENC_PIC_FORMAT_422_8BITS = 0x0288,
} IMPEncoderPicFormat;


struct avpu_dma_info {
        uint32_t fd;
        uint32_t size;
        uint32_t phy_addr;
};

struct DmaBuffer {
        /* ioctl structure */
        struct avpu_dma_info info;
        /* given to use with mmap */
        AL_VADDR vaddr;
        size_t offset;
        size_t mmap_offset; /* used by non-dmabuf */
        bool shouldCloseFd;
};

#define MAX_DEVICE_FILE_NAME 30

struct LinuxDmaCtx {
        AL_TLinuxDmaAllocator base;
        char deviceFile[MAX_DEVICE_FILE_NAME];
        int fd;
};

static int fill_one_streambuffer(IHal_CodecHandle_t *handle, AL_TBuffer *pStream);

static IHAL_INT32 al_encoder_init()
{
        return AL_Codec_Create();
}


static void al_encoder_exit(void)
{
        AL_Codec_Destroy();
}

static IHal_CodecHandle_t *vcodec_create(int type)
{
        if (video_encoder_cnt > MAX_VIDEO_CHAN) {
                IMPP_LOGE(TAG, "encoder channel cannt over 8");
                return IHAL_RNULL;
        }
        IHal_CodecHandle_t *handle = (IHal_CodecHandle_t *)malloc(sizeof(IHal_CodecHandle_t));
        if (!handle) {
                IMPP_LOGE(TAG, "malloc handle failed");
                return IHAL_RNULL;
        }
        Video_CodecCtx_t *ctx = (Video_CodecCtx_t *)malloc(sizeof(Video_CodecCtx_t));
        if (!ctx) {
                IMPP_LOGE(TAG, "malloc handle - ctx failed");
                return IHAL_RNULL;
        }
        memset(ctx, 0, sizeof(Video_CodecCtx_t));
        pthread_mutex_init(&ctx->mutex, NULL);
        ctx->encoderTid = 0;
        ctx->frameTid = 0;
        ctx->pCodecEncode = NULL;
        if (type & 0x0f) {
                video_encoder_cnt++;
        }
        handle->codectype = type;
        handle->ctx = (void *)ctx;
        return handle;
}



static int encoder_stream_clean(IHal_CodecHandle_t *handle)
{
        AL_TBuffer *pStream = NULL;
        AL_TBuffer *pSrc = NULL;
        Video_CodecCtx_t *ctx = (Video_CodecCtx_t *)handle->ctx;
        assert(ctx);
        int retry = 1000;
        while ((ctx->encoding_count > ctx->finishencoding_count) && retry--) {
                if (AL_Codec_Encode_GetStream(ctx->pCodecEncode, &pStream, &pSrc) >= 0) {
                        sem_post(&ctx->src_available_cnt_sem);
                        fill_one_streambuffer(handle, pStream);
                        AL_Codec_Encode_ReleaseStream(ctx->pCodecEncode, pStream, pSrc);
                        /* printf("clean stream ok num : %lld \r\n",ctx->finishencoding_count); */
                }
                usleep(100);
        }
        return 0;
}

static int vcodec_destroy(IHal_CodecHandle_t *handle)
{
        unsigned int enc_td_ret = 0;
        Video_CodecCtx_t *ctx = (Video_CodecCtx_t *)handle->ctx;
        assert(ctx);
        int ret = 0;

        // assert pCodec create ok
        if (NULL == ctx->pCodecEncode) {
                ret = pthread_mutex_destroy(&ctx->mutex);
                if (ret) {
                        IMPP_LOGE(TAG, "pthread mutex destroy failed.");
                        return -IHAL_RERR;
                }
                free(ctx);
                ctx = NULL;
                free(handle);
                handle = NULL;
                video_encoder_cnt--;
                return IHAL_ROK;
        }

        /* IMPP_LOGD(TAG, "entry encoding-times = %lld finish-times = %lld \r\n", ctx->encoding_count, ctx->finishencoding_count); */

        if (ctx->encoderTid) {
                pthread_cancel(ctx->encoderTid);
                AL_Codec_Encode_Commit_FilledFifo(ctx->pCodecEncode);
                pthread_join(ctx->encoderTid, &enc_td_ret);
                // printf("%s %d thread-exit ret = %d\r\n", __func__, __LINE__, enc_td_ret);
        }

        if (ctx->frameTid) {
                pthread_cancel(ctx->frameTid);
                pthread_join(ctx->frameTid, &enc_td_ret);
                // printf("%s %d thread-exit-frame ret = %d\r\n", __func__, __LINE__, enc_td_ret);
        }

        pthread_mutex_lock(&ctx->mutex);
        encoder_stream_clean(handle);
        pthread_mutex_unlock(&ctx->mutex);

        if (IMPP_INTERNAL_BUFFER == ctx->dstbuf_type) {
                Fifo_Deinit(&ctx->dst_queued_FiFo);
                Fifo_Deinit(&ctx->dst_done_FiFo);
                sem_destroy(&ctx->dst_available_cnt_sem);

                for (int i = 0; i < ctx->num_dstbuf; i++) {
                        free(ctx->dstbuf[i].vaddr);
                        ctx->dstbuf[i].vaddr = NULL;
                }

                free(ctx->dstbuf);
                ctx->dstbuf = NULL;
        }


        if (IMPP_EXT_DMABUFFER == ctx->srcbuf_type || IMPP_INTERNAL_BUFFER == ctx->srcbuf_type) {
                Fifo_Deinit(&ctx->src_queued_FiFo);
                Fifo_Deinit(&ctx->src_done_FiFo);
                sem_destroy(&ctx->src_available_cnt_sem);
                AL_Codec_Encode_Destroy(ctx->pCodecEncode);
                free(ctx->srcbuf);
                ctx->srcbuf = NULL;
        }

        ret = pthread_mutex_destroy(&ctx->mutex);
        if (ret) {
                IMPP_LOGE(TAG, "pthread mutex destroy failed.");
                return -IHAL_RERR;
        }

        free(ctx);
        ctx = NULL;
        free(handle);
        handle = NULL;
        video_encoder_cnt--;

        return IHAL_ROK;
}

static int set_encoder_gopparam(IHal_CodecHandle_t *handle)
{
        Video_CodecCtx_t *ctx = (Video_CodecCtx_t *)handle->ctx;
        video_codec_run_ctx_t *runctx = &ctx->runCtx;
        switch (handle->codectype) {
        case H264_ENC:
                return AL_Codec_Encode_SetGopLength(ctx->pCodecEncode, runctx->param.codecparam.h264e_param.gop_len);   // test
                break;
        case H265_ENC:
                return AL_Codec_Encode_SetGopLength(ctx->pCodecEncode, runctx->param.codecparam.h265e_param.gop_len);   // test
                break;
        default:
                return 0;
        }
}

static int _encoder_request_idr(IHal_CodecHandle_t *handle)
{
        Video_CodecCtx_t *ctx = (Video_CodecCtx_t *)handle->ctx;
        if (handle->codectype == H264_ENC || handle->codectype == H265_ENC) {
                pthread_mutex_lock(&ctx->mutex);
                ctx->chn_idr_request = 0x01;
                pthread_mutex_unlock(&ctx->mutex);
        }
        return 0;
}

static int _encoder_set_roi(IHal_CodecHandle_t *handle, IHAL_EncoderRoiAttr *roiattr)
{
        Video_CodecCtx_t *ctx = (Video_CodecCtx_t *)handle->ctx;
        if (handle->codectype == H264_ENC || handle->codectype == H265_ENC) {
                if (roiattr->indx > 7) {
                        return -1;
                }
                pthread_mutex_lock(&ctx->mutex);
                if (roiattr->enable) {
                        memset(&ctx->roiAttr[roiattr->indx], 0, sizeof(IHAL_EncoderRoiAttr));
                        memcpy(&ctx->roiAttr[roiattr->indx], roiattr, sizeof(IHAL_EncoderRoiAttr));
                } else {
                        memset(&ctx->roiAttr[roiattr->indx], 0, sizeof(IHAL_EncoderRoiAttr));
                }
                ctx->roi_change_flag = 1;
                pthread_mutex_unlock(&ctx->mutex);
        } else {
                IMPP_LOGW(TAG, "jpeg encoder not support ROI\n");
                return -1;
        }
        return 0;
}


static int _RoiMngr_updateRoi(Video_CodecCtx_t *ctx)
{
        for (int i = 0; i < 8; i++) {
                if (ctx->roiAttr[i].enable == 1) {
                        switch (ctx->roiAttr[i].param.RoiQuality) {
                        case IHAL_ROI_QUALITY_HIGH:
                        case IHAL_ROI_QUALITY_MEDIUM:
                        case IHAL_ROI_QUALITY_LOW:
                        case IHAL_ROI_QUALITY_DONT_CARE:
                        case IHAL_ROI_QUALITY_STATIC:
                        case IHAL_ROI_QUALITY_INTRA:
                                printf("%s =================\r\n", __func__);
                                AL_RoiMngr_AddROI(ctx->pCodecEncode->m_RoiCtx, ctx->roiAttr[i].param.posX,
                                                  ctx->roiAttr[i].param.posY, ctx->roiAttr[i].param.Width,
                                                  ctx->roiAttr[i].param.Height, ctx->roiAttr[i].param.RoiQuality);
                                break;
                        default:
                                return -1;

                        }
                }
        }
        return 0;
}


static IHAL_INT32 set_jpeg_codec_param(IHal_CodecHandle_t *handle, IHal_CodecParam *param)
{
        Video_CodecCtx_t *ctx = (Video_CodecCtx_t *)handle->ctx;
        video_codec_run_ctx_t *runctx = &ctx->runCtx;
        AL_Codec_Encode_SetDefaultParam(&ctx->codecParam);
        if (handle->codectype == JPEG_ENC) {
                // 0 - run time param private save
                memcpy((void *)&runctx->param.codecparam.jpegenc_param, (void *)&param->codecparam.jpegenc_param, sizeof(IHal_JpegEnc_Param));
                // 1 - set param to codec
                ctx->codecParam.m_Settings.tChParam[0].uEncWidth    = param->codecparam.jpegenc_param.enc_width;
                ctx->codecParam.m_Settings.tChParam[0].uEncHeight   = param->codecparam.jpegenc_param.enc_height;
                ctx->codecParam.m_Settings.tChParam[0].uSrcWidth    = param->codecparam.jpegenc_param.src_width;
                ctx->codecParam.m_Settings.tChParam[0].uSrcHeight   = param->codecparam.jpegenc_param.src_height;

                int fmt = param->codecparam.jpegenc_param.src_fmt;
                switch (fmt) {
                case IMPP_PIX_FMT_NV12:
                case IMPP_PIX_FMT_NV21:
                        ctx->codecParam.m_Settings.tChParam[0].ePicFormat = IMPP_ENC_PIC_FORMAT_420_8BITS;
                        ctx->codecParam.m_fourCC    = FOURCC(NV12);
                        break;
                case IMPP_PIX_FMT_NV16:
                        ctx->codecParam.m_Settings.tChParam[0].ePicFormat = IMPP_ENC_PIC_FORMAT_422_8BITS;
                        ctx->codecParam.m_fourCC    = FOURCC(NV16);
                        break;
                /* case IMPP_PIX_FMT_I422: */
                /*  ctx->codecParam.m_Settings.tChParam[0].ePicFormat = IMPP_ENC_PIC_FORMAT_422_8BITS; */
                /*  ctx->codecParam.m_fourCC    = FOURCC(I422);break; */
                /* case IMPP_PIX_FMT_I420: */
                /*  ctx->codecParam.m_Settings.tChParam[0].ePicFormat = IMPP_ENC_PIC_FORMAT_420_8BITS; */
                /*  ctx->codecParam.m_fourCC    = FOURCC(I420);break; */
                default:
                        IMPP_LOGW(TAG, " only support nv12 / nv16");
                        return -IHAL_RERR;
                }

                ctx->codecParam.m_heightAlignVal = -1;
                ctx->codecParam.m_baseStreamSize = param->codecparam.jpegenc_param.base_stream_size;

                ctx->codecParam.m_Settings.tChParam[0].tGopParam.uGopLength = 0;
                ctx->codecParam.m_Settings.tChParam[0].tGopParam.uFreqIDR   = 25;
                ctx->codecParam.m_Settings.tChParam[0].eProfile             = AL_PROFILE_JPEG;
                ctx->codecParam.m_Settings.tChParam[0].uLevel               = 0;
                ctx->codecParam.m_Settings.tChParam[0].uTier                = 0;
                ctx->codecParam.m_Settings.tChParam[0].tRCParam.eRCMode     = AL_RC_CONST_QP;
				ctx->codecParam.m_Settings.tChParam[0].tRCParam.iInitialQP  = param->codecparam.jpegenc_param.quality;
        } else {
                IMPP_LOGW(TAG, "x2500 not support hardware jpeg decoder");
                return -IHAL_RERR;
        }
        return IHAL_ROK;
}

static IHAL_INT32 set_h264_codec_param(IHal_CodecHandle_t *handle, IHal_CodecParam *param)
{
        Video_CodecCtx_t *ctx = (Video_CodecCtx_t *)handle->ctx;
        video_codec_run_ctx_t *runctx = &ctx->runCtx;
        AL_Codec_Encode_SetDefaultParam(&ctx->codecParam);
        if (handle->codectype == H264_ENC) {
                // 0 - run time param private save
                memcpy((void *)&runctx->param.codecparam.h264e_param, (void *)&param->codecparam.h264e_param, sizeof(IHal_H264E_Param));
                // 1 - set param to codec
                ctx->codecParam.m_Settings.tChParam[0].uEncWidth    = param->codecparam.h264e_param.enc_width;
                ctx->codecParam.m_Settings.tChParam[0].uEncHeight   = param->codecparam.h264e_param.enc_height;
                ctx->codecParam.m_Settings.tChParam[0].uSrcWidth    = param->codecparam.h264e_param.src_width;
                ctx->codecParam.m_Settings.tChParam[0].uSrcHeight   = param->codecparam.h264e_param.src_height;
                ctx->codecParam.m_Settings.tChParam[0].tGopParam.uGopLength     = param->codecparam.h264e_param.gop_len;
                ctx->codecParam.m_Settings.tChParam[0].tGopParam.uFreqIDR       = param->codecparam.h264e_param.freqIDR;
                ctx->codecParam.m_Settings.tChParam[0].eProfile             = AL_PROFILE_AVC_HIGH;
                ctx->codecParam.m_Settings.tChParam[0].uLevel               = param->codecparam.h264e_param.level;

                int fmt = param->codecparam.h264e_param.src_fmt;
                switch (fmt) {
                case IMPP_PIX_FMT_NV12:
                case IMPP_PIX_FMT_NV21:
                        ctx->codecParam.m_fourCC    = FOURCC(NV12);
                        break;
                case IMPP_PIX_FMT_I420:
                        ctx->codecParam.m_fourCC    = FOURCC(I420);
                        break;
                default:
                        IMPP_LOGW(TAG, "x2500 h264/h265 support 420");
                        return -IHAL_RERR;
                }
                ctx->codecParam.m_heightAlignVal = 2;       // default set with out align
                ctx->codecParam.m_baseStreamSize = param->codecparam.h264e_param.base_stream_size;

                ctx->codecParam.m_Settings.tChParam[0].tRCParam.uTargetBitRate  = param->codecparam.h264e_param.target_bitrate * 1000;
                ctx->codecParam.m_Settings.tChParam[0].tRCParam.uMaxBitRate     = param->codecparam.h264e_param.max_bitrate * 1000;
                ctx->codecParam.m_Settings.tChParam[0].tRCParam.uMaxPSNR        = param->codecparam.h264e_param.maxPSNR * 100;
                ctx->codecParam.m_Settings.tChParam[0].tRCParam.uFrameRate      = param->codecparam.h264e_param.frameRateNum;
                ctx->codecParam.m_Settings.tChParam[0].tRCParam.uClkRatio       = param->codecparam.h264e_param.frameRateDen * 1000;
                ctx->codecParam.m_Settings.tChParam[0].tRCParam.iInitialQP      = param->codecparam.h264e_param.initial_Qp;
                ctx->codecParam.m_Settings.tChParam[0].tRCParam.iMinQP          = param->codecparam.h264e_param.mini_Qp;
                ctx->codecParam.m_Settings.tChParam[0].tRCParam.iMaxQP          = param->codecparam.h264e_param.max_Qp;

                /* ctx->codecParam.m_Settings.tChParam[0].tRCParam.pMaxPictureSize[AL_SLICE_I] = param->codecparam.h264e_param.maxPictureSize * 1000; */

                switch (param->codecparam.h264e_param.rc_mode) {
                case IMPP_ENC_RC_MODE_CBR:
                        ctx->codecParam.m_Settings.tChParam[0].tRCParam.eRCMode = AL_RC_CBR;
                        break;
                case IMPP_ENC_RC_MODE_VBR:
                        ctx->codecParam.m_Settings.tChParam[0].tRCParam.eRCMode = AL_RC_VBR;
                        break;
                case IMPP_ENC_RC_MODE_CAPPED_VBR:
                        ctx->codecParam.m_Settings.tChParam[0].tRCParam.eRCMode = AL_RC_CAPPED_VBR;
                        break;
                case IMPP_ENC_RC_MODE_CAPPED_QUALITY:
                        ctx->codecParam.m_Settings.tChParam[0].tRCParam.eRCMode = AL_RC_CAPPED_QUALITY;
                        break;
                default:
                        ctx->codecParam.m_Settings.tChParam[0].tRCParam.eRCMode = AL_RC_CBR;
                        break;
                }
        } else {
                IMPP_LOGW(TAG, "x2500 not support hardware h264 decoder");
                return -IHAL_RERR;
        }

        return IHAL_ROK;
}

static IHAL_INT32 set_hevc_codec_param(IHal_CodecHandle_t *handle, IHal_CodecParam *param)
{
        Video_CodecCtx_t *ctx = (Video_CodecCtx_t *)handle->ctx;
        video_codec_run_ctx_t *runctx = &ctx->runCtx;
        AL_Codec_Encode_SetDefaultParam(&ctx->codecParam);
        if (handle->codectype == H265_ENC) {
                memcpy((void *)&runctx->param.codecparam.h265e_param, (void *)&param->codecparam.h265e_param, sizeof(IHal_H265E_Param));
                // 1 - set param to codec
                ctx->codecParam.m_Settings.tChParam[0].uEncWidth    = param->codecparam.h265e_param.enc_width;
                ctx->codecParam.m_Settings.tChParam[0].uEncHeight   = param->codecparam.h265e_param.enc_height;
                ctx->codecParam.m_Settings.tChParam[0].uSrcWidth    = param->codecparam.h265e_param.src_width;
                ctx->codecParam.m_Settings.tChParam[0].uSrcHeight   = param->codecparam.h265e_param.src_height;
                ctx->codecParam.m_Settings.tChParam[0].tGopParam.uGopLength = param->codecparam.h265e_param.gop_len;
                ctx->codecParam.m_Settings.tChParam[0].tGopParam.uFreqIDR   = param->codecparam.h265e_param.freqIDR;
                ctx->codecParam.m_Settings.tChParam[0].eProfile             = AL_PROFILE_HEVC_MAIN;
                ctx->codecParam.m_Settings.tChParam[0].uLevel               = param->codecparam.h265e_param.level;

                int fmt = param->codecparam.h265e_param.src_fmt;
                switch (fmt) {
                case IMPP_PIX_FMT_NV12:
                case IMPP_PIX_FMT_NV21:
                        ctx->codecParam.m_fourCC    = FOURCC(NV12);
                        break;
                case IMPP_PIX_FMT_I420:
                        ctx->codecParam.m_fourCC    = FOURCC(I420);
                        break;
                default:
                        IMPP_LOGW(TAG, "x2500 vcodec only support nv12/nv21");
                        return -IHAL_RERR;
                }
                /* ctx->codecParam.m_Settings.tChParam[0].ePicFormat    = param->codecparam.h265e_param.src_fmt; */

                ctx->codecParam.m_heightAlignVal = 2;       // default set with out align
                ctx->codecParam.m_baseStreamSize = param->codecparam.h265e_param.base_stream_size;

                ctx->codecParam.m_Settings.tChParam[0].tRCParam.uTargetBitRate  = param->codecparam.h265e_param.target_bitrate * 1000;
                ctx->codecParam.m_Settings.tChParam[0].tRCParam.uMaxBitRate     = param->codecparam.h265e_param.max_bitrate * 1000;
                ctx->codecParam.m_Settings.tChParam[0].tRCParam.uMaxPSNR        = param->codecparam.h265e_param.maxPSNR * 100;
                ctx->codecParam.m_Settings.tChParam[0].tRCParam.uFrameRate      = param->codecparam.h265e_param.frameRateNum;
                ctx->codecParam.m_Settings.tChParam[0].tRCParam.uClkRatio       = param->codecparam.h265e_param.frameRateDen * 1000;
                ctx->codecParam.m_Settings.tChParam[0].tRCParam.iInitialQP      = param->codecparam.h265e_param.initial_Qp;
                ctx->codecParam.m_Settings.tChParam[0].tRCParam.iMinQP          = param->codecparam.h265e_param.mini_Qp;
                ctx->codecParam.m_Settings.tChParam[0].tRCParam.iMaxQP          = param->codecparam.h265e_param.max_Qp;
                /* ctx->codecParam.m_Settings.tChParam[0].tRCParam.pMaxPictureSize[AL_SLICE_I] = param->codecparam.h265e_param.maxPictureSize * 1000; */

                switch (param->codecparam.h265e_param.rc_mode) {
                case IMPP_ENC_RC_MODE_CBR:
                        ctx->codecParam.m_Settings.tChParam[0].tRCParam.eRCMode = AL_RC_CBR;
                        break;
                case IMPP_ENC_RC_MODE_VBR:
                        ctx->codecParam.m_Settings.tChParam[0].tRCParam.eRCMode = AL_RC_VBR;
                        break;
                case IMPP_ENC_RC_MODE_CAPPED_VBR:
                        ctx->codecParam.m_Settings.tChParam[0].tRCParam.eRCMode = AL_RC_CAPPED_VBR;
                        break;
                case IMPP_ENC_RC_MODE_CAPPED_QUALITY:
                        ctx->codecParam.m_Settings.tChParam[0].tRCParam.eRCMode = AL_RC_CAPPED_QUALITY;
                        break;
                default:
                        ctx->codecParam.m_Settings.tChParam[0].tRCParam.eRCMode = AL_RC_CBR;
                        break;
                }
        } else {
                IMPP_LOGW(TAG, "x2500 not support hardware h264 decoder");
                return -IHAL_RERR;
        }

        return IHAL_ROK;
}

static IHAL_INT32 set_audio_codec_param(IHal_CodecHandle_t *handle, IHal_CodecParam *param)
{
        // TODO
        return IHAL_ROK;
}



static IHAL_INT32 video_codec_createbuffer(IHal_CodecHandle_t *handle, IMPP_BUFFER_TYPE type, IHAL_INT32 nums, IHAL_INT32 is_src)
{
        IHAL_INT32 ret = 0;
        Video_CodecCtx_t *ctx = (Video_CodecCtx_t *)handle->ctx;
        assert(ctx);
        video_codec_run_ctx_t *runctx = &ctx->runCtx;
        assert(runctx);
        if (is_src) {
                if (type == IMPP_EXT_DMABUFFER) {
                        ctx->codecParam.m_bUseExtraSrcBuffer = true;
                        ctx->num_srcbuf = nums;
                        ctx->srcbuf = (video_srcbuf_info_t *)malloc(nums * sizeof(video_srcbuf_info_t));
                        if (!ctx->srcbuf) {
                                IMPP_LOGE(TAG, "jpeg mallc srcbuf info failed");
                                return -IHAL_RERR;
                        }
                        ret = AL_Codec_Encode_Create(&ctx->pCodecEncode,  &ctx->codecParam);
                        if ((ret < 0) || !ctx->pCodecEncode) {
                                IMPP_LOGE(TAG, "codec Create failed");
                                free(ctx->srcbuf);
                                return -IHAL_RERR;
                        }
                        set_encoder_gopparam(handle);       // make sure gop_param valid

                        ctx->srcbuf_type = IMPP_EXT_DMABUFFER;

                        // vcodec buffer mannager
                        sem_init(&ctx->src_available_cnt_sem, 0, 0);    // at start ,src bufs are all not available
                        Fifo_Init(&ctx->src_done_FiFo, nums);
                        Fifo_Init(&ctx->src_queued_FiFo, nums);
                        return ctx->num_srcbuf;
                } else if (type == IMPP_INTERNAL_BUFFER) {
                        ctx->codecParam.m_bUseExtraSrcBuffer = false;
                        ret = AL_Codec_Encode_Create(&ctx->pCodecEncode,  &ctx->codecParam);
                        if ((ret < 0) || !ctx->pCodecEncode) {
                                IMPP_LOGE(TAG, "codec Create failed");
                                free(ctx->srcbuf);
                                return -IHAL_RERR;
                        }
                        set_encoder_gopparam(handle);       // make sure gop_param valid

                        IHAL_UINT32 fcnt = 0, fsize = 0;
                        AL_Codec_Encode_GetSrcFrameCntAndSize(ctx->pCodecEncode, &fcnt, &fsize);
                        IMPP_LOGD(TAG, "fcnt = %d  fsize = %d", fcnt, fsize);
                        ctx->num_srcbuf = fcnt;
                        ctx->srcbuf = (video_srcbuf_info_t *)malloc(fcnt * sizeof(video_srcbuf_info_t));
                        if (!ctx->srcbuf) {
                                IMPP_LOGE(TAG, "codec mallc src buf info failed");
                                return -IHAL_RERR;
                        }
                        ctx->srcbuf_type = IMPP_INTERNAL_BUFFER;

                        sem_init(&ctx->src_available_cnt_sem, 0, ctx->num_srcbuf);      // at start ,src bufs are all available
                        Fifo_Init(&ctx->src_done_FiFo, ctx->num_srcbuf);
                        Fifo_Init(&ctx->src_queued_FiFo, ctx->num_srcbuf);
                        struct DmaBuffer *dmabuf = NULL;
                        App_Fifo *fifo = &ctx->pCodecEncode->m_SrcBufPool.fifo;
                        for (int i = 0; i < ctx->num_srcbuf; i++) {
                                AL_TBuffer *pBuf = (AL_TBuffer *)Fifo_Dequeue(fifo, AL_WAIT_FOREVER);
                                dmabuf = (struct DmaBuffer *)pBuf->hBufs[0];
                                ctx->srcbuf[i].AL_SrcTBuf = pBuf;
                                ctx->srcbuf[i].vaddr = (IHAL_UINT32 *)AL_Allocator_GetVirtualAddr(ctx->pCodecEncode->m_pCodec->m_pAllocator, pBuf->hBufs[0]);
                                ctx->srcbuf[i].paddr = (IHAL_UINT32 *)AL_Allocator_GetPhysicalAddr(ctx->pCodecEncode->m_pCodec->m_pAllocator, pBuf->hBufs[0]);
                                ctx->srcbuf[i].dmafd = dmabuf->info.fd;
                                ctx->srcbuf[i].size = fsize;
                                ctx->srcbuf[i].index = i;
                                Fifo_Queue(fifo, pBuf, AL_WAIT_FOREVER);

                                // add buf to src done fifo
                                Fifo_Queue(&ctx->src_done_FiFo, &ctx->srcbuf[i], AL_WAIT_FOREVER);
                                IMPP_LOGD(TAG, "DmaBuffer info .phy_addr = 0x%x vaddr = 0x%x fd = %d", dmabuf->info.phy_addr, dmabuf->vaddr, dmabuf->info.fd);
                                IMPP_LOGD(TAG, "get src vaddr = 0x%x paddr = 0x%x dmafd = %d ", ctx->srcbuf[i].vaddr, ctx->srcbuf[i].paddr, ctx->srcbuf[i].dmafd);
                        }
                        return fcnt;
                } else {
                        IMPP_LOGE(TAG, "%s x2500 encoder src just support ext-dmabuf or internal-buf");
                        return -IHAL_RERR;
                }
        } else {
                if (type == IMPP_INTERNAL_BUFFER) {
                        IHAL_UINT32 stream_cnt = 0, stream_size = 0;
                        ret = AL_Codec_Encode_GetSrcStreamCntAndSize(ctx->pCodecEncode, &stream_cnt, &stream_size);
                        IMPP_LOGD(TAG, "stream cnt = %d stream size = %d \n", stream_cnt, stream_size);
                        if (ret < 0) {
                                IMPP_LOGW(TAG, "get stream cnt && size failed,set default as 1M");
                                if (handle->codectype == JPEG_ENC) {
                                        stream_size   = 512 * 1024;
                                } else {
                                        stream_size   = 1024 * 1024;
                                }
                        }
                        if (nums > MAX_DSTBUF_NUM) {
                                nums = MAX_DSTBUF_NUM;
                        }
                        ctx->num_dstbuf = nums;
                        ctx->dstbuf = (video_dstbuf_info_t *)malloc(nums * sizeof(video_dstbuf_info_t));
                        if (!ctx->dstbuf) {
                                IMPP_LOGE(TAG, "mallc dst buf info failed");
                                return -IHAL_RERR;
                        }
                        //stream_size = stream_size - stream_size % 4 + 4;

                        //IMPP_LOGD(TAG, "4align stream_size = %d ", stream_size);
                        for (int i = 0; i < nums; i++) {
                                ctx->dstbuf[i].vaddr = (IHAL_UINT32 *)malloc(stream_size);
                                if (!ctx->dstbuf[i].vaddr) {
                                        IMPP_LOGE(TAG, "malloc user space dst buffer failed \n");
                                        free(ctx->dstbuf);
                                        return -IHAL_RERR;
                                }
                                ctx->dstbuf[i].size = stream_size;
                                ctx->dstbuf[i].index = i;
                                ctx->dstbuf[i].uselength = 0;
                        }
                        ctx->dstbuf_type = IMPP_INTERNAL_BUFFER;
                        sem_init(&ctx->dst_available_cnt_sem, 0, 0);    // when start the dst is not available
                        Fifo_Init(&ctx->dst_done_FiFo, nums);
                        Fifo_Init(&ctx->dst_queued_FiFo, nums);
                        for (int i = 0; i < nums; i++) {
                                Fifo_Queue(&ctx->dst_queued_FiFo, &ctx->dstbuf[i], AL_WAIT_FOREVER);
                        }
                        return nums;
                } else {
                        IMPP_LOGE(TAG, "%s x2500 encoder dst just support internal-buf");
                        return -IHAL_RERR;
                }
        }
}

static IHAL_INT32 audio_codec_createbuffer(IHal_CodecHandle_t *handle, IMPP_BUFFER_TYPE type, IHAL_INT32 nums, IHAL_INT32 is_src)
{
        // TODO
        return IHAL_ROK;
}



static int vcodec_setSrcBuf(IHal_CodecHandle_t *handle, int index, IMPP_BufferInfo_t *share)
{
        assert(handle->ctx);
        Video_CodecCtx_t *ctx = (Video_CodecCtx_t *)handle->ctx;
        if (index > ctx->num_srcbuf) {
                IMPP_LOGE(TAG, "###### index error #######");
                return -IHAL_RERR;
        }
        if (share->fd <= 0) {
                IMPP_LOGE(TAG, "x2500 video encoder only support dma-buf input ");
                return -IHAL_RERR;
        }
        if (ctx->srcbuf_type == IMPP_EXT_DMABUFFER) {
                struct LinuxDmaCtx *pCtx = (struct LinuxDmaCtx *)ctx->pCodecEncode->m_pCodec->m_pAllocator;
                if (pCtx == NULL) {
                        IMPP_LOGE(TAG, "codec set src-buffer : get pCtx failed");
                        return -IHAL_RERR;
                }
                AL_TLinuxDmaAllocator *pAllocator = (AL_TLinuxDmaAllocator *)&pCtx->base;
                struct  DmaBuffer *pDmaBuffer = NULL;
                pDmaBuffer = AL_LinuxDmaAllocator_ImportFromFd(pAllocator, share->fd);
                if (!pDmaBuffer) {
                        IMPP_LOGE(TAG, "import dma-buf fd failed");
                        return -IHAL_RERR;
                }
                ctx->srcbuf[index].dmafd = share->fd;
                ctx->srcbuf[index].index = index;
                ctx->srcbuf[index].size  = share->size;
                ctx->srcbuf[index].paddr = (IHAL_UINT32 *)pDmaBuffer->info.phy_addr;
                //ctx->srcbuf[index].vaddr = (IHAL_UINT32*)AL_Allocator_GetVirtualAddr((AL_TAllocator*)pCtx,pDmaBuffer); // something wrong
                //IMPP_LOGD(TAG,"set src buf vaddr = 0x%x paddr = 0x%x .....",ctx->srcbuf[index].vaddr,ctx->srcbuf[index].paddr);
                // add buf to src done fifo
                /* Fifo_Queue(&ctx->src_done_FiFo,&ctx->srcbuf[index],AL_WAIT_FOREVER); */
                free(pDmaBuffer);
                pDmaBuffer = NULL;
                return IHAL_ROK;
        } else {
                IMPP_LOGE(TAG, "x2500 video encoder only support set dma-buf ");
                return -IHAL_RFAILED;
        }
}

static IHAL_INT32 vcodec_getSrcBuf(IHal_CodecHandle_t *handle, int index, IMPP_BufferInfo_t *share)
{
        int ret = 0;
        assert(handle->ctx);
        Video_CodecCtx_t *ctx = (Video_CodecCtx_t *)handle->ctx;
        if (ctx->srcbuf_type == IMPP_INTERNAL_BUFFER) {
                // TODO
                if (index > ctx->num_srcbuf) {
                        IMPP_LOGE(TAG, "the src buf max index = %d", ctx->num_srcbuf - 1);
                        return -IHAL_RERR;
                }
                assert(share);
                ret = sem_trywait(&ctx->src_available_cnt_sem);
                if (ret) {
                        IMPP_LOGW(TAG, "the buf[%d] has been get out \n", index);
                        return -IHAL_RFAILED;
                }
                video_srcbuf_info_t *srcbuf = Fifo_Dequeue(&ctx->src_done_FiFo, AL_NO_WAIT);
                if (!srcbuf) {
                        IMPP_LOGW(TAG, "the buf[%d] has been get out \n", index);
                        return -IHAL_RFAILED;
                }
                share->vaddr = srcbuf->vaddr;
                share->paddr = srcbuf->paddr;
                share->fd = srcbuf->dmafd;
                share->size = ctx->srcbuf->size;
                share->index = srcbuf->index;
                return IHAL_ROK;
        } else {
                IMPP_LOGE(TAG, "only codec buffer is internall buffer can get");
                return -IHAL_RFAILED;
        }
}

static int fill_one_streambuffer(IHal_CodecHandle_t *handle, AL_TBuffer *pStream)
{
        int iNumFrame = 0;
        unsigned int offset = 0;
        Video_CodecCtx_t *ctx = (Video_CodecCtx_t *)handle->ctx;
        video_dstbuf_info_t *dst = Fifo_Dequeue(&ctx->dst_queued_FiFo, AL_WAIT_FOREVER);
        unsigned char *bufViraddr = (unsigned char *)dst->vaddr;
        AL_TStreamMetaData *pStreamMeta = (AL_TStreamMetaData *)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);
        AL_TStreamSection *pCurSection = NULL;
        unsigned char *pData = NULL;
        ctx->finishencoding_count += 1;
        if (handle->codectype == JPEG_ENC) {
                pData = AL_Buffer_GetData(pStream);
                for (int curSection = 0; curSection < pStreamMeta->uNumSection; curSection++) {
                        pCurSection = &pStreamMeta->pSections[curSection];
                        memcpy((void *)(bufViraddr + offset), (void *)(pData + pCurSection->uOffset), pCurSection->uLength);
                        offset += pCurSection->uLength;
                }
                dst->uselength = offset;
                Fifo_Queue(&ctx->dst_done_FiFo, dst, AL_WAIT_FOREVER);
                sem_post(&ctx->dst_available_cnt_sem);
        } else {
                dst->stream.packcnt = 0;
                for (int curSection = 0; curSection < pStreamMeta->uNumSection; ++curSection) {
                        EncoderPack *pack = &dst->stream.pack[curSection];
                        memset(pack, 0, sizeof(EncoderPack));
                        if (pStreamMeta->pSections[curSection].eFlags & AL_SECTION_END_FRAME_FLAG) {
                                ++iNumFrame;
                        }
                        pCurSection = &pStreamMeta->pSections[curSection];
                        pData = AL_Buffer_GetData(pStream);
                        if (pCurSection->uLength > 0) {
                                unsigned int uRemSize = AL_Buffer_GetSize(pStream) - pCurSection->uOffset;
                                pack->length = pCurSection->uLength;
                                pack->frameEnd = pCurSection->eFlags & AL_SECTION_END_FRAME_FLAG;
                                pack->sliceType = pCurSection->uSliceType;
                                pack->nalType = (EncoderNalType)pCurSection->uNut;
                                //printf("%s sliece type = %d nalutype = %d\r\n",__func__,pCurSection->uSliceType,pack->nalType);

                                if (uRemSize < pCurSection->uLength) {
                                        memcpy((void *)(bufViraddr + offset), (void *)(pData + pCurSection->uOffset), uRemSize);
                                        offset += uRemSize;
                                        memcpy((void *)(bufViraddr + offset), (void *)(pData), pCurSection->uLength - uRemSize);
                                        offset += (pCurSection->uLength - uRemSize);
                                } else {
                                        memcpy((void *)(bufViraddr + offset), (void *)(pData + pCurSection->uOffset), pCurSection->uLength);
                                        offset += pCurSection->uLength;
                                }
                                dst->stream.packcnt += 1;
                        }
                }
                dst->uselength = offset;
                offset = 0;
                for (int i = 0; i < pStreamMeta->uNumSection; i++) {
                        EncoderPack *pack = &dst->stream.pack[i];
                        pack->offset = offset;
                        offset += pack->length;
                }
                Fifo_Queue(&ctx->dst_done_FiFo, dst, AL_WAIT_FOREVER);
                sem_post(&ctx->dst_available_cnt_sem);
        }
        return 0;
}




void *frame_update_thread(void *arg)
{
        int ret = 0;
        IHal_CodecHandle_t *handle = (IHal_CodecHandle_t *)arg;
        Video_CodecCtx_t *ctx = (Video_CodecCtx_t *)handle->ctx;
        video_srcbuf_info_t *src = NULL;
        AL_TBuffer *pStream = NULL;
        AL_TBuffer *pSrc = NULL;
        AL_CodecFrame frame;

        while (1) {
                memset(&frame, 0, sizeof(frame));
                if (ctx->vcodec_run_sta == VCODEC_STA_RUNING) {
                        pthread_mutex_lock(&ctx->mutex);
                        if (ctx->chn_idr_request == 1) {
                                AL_Codec_Encode_RestartGop(ctx->pCodecEncode);
                                ctx->chn_idr_request = 0;
                        }
                        if (ctx->roi_change_flag == 1) {
                                AL_RoiMngr_Clear(ctx->pCodecEncode->m_RoiCtx);
                                _RoiMngr_updateRoi(ctx);
                                ctx->roi_change_flag = 0;
                        }
                        pthread_mutex_unlock(&ctx->mutex);
                        if (ctx->srcbuf_type == IMPP_EXT_DMABUFFER) {
                                src = Fifo_Dequeue(&ctx->src_queued_FiFo, AL_WAIT_FOREVER); //
                                if (!src) {
                                        IMPP_LOGE(TAG, "dequeue src_queued_FiFo failed");
                                        continue;
                                }
                                frame.frameInfo.phyAddr = (IHAL_UINT32)src->paddr;
                                frame.frameInfo.virAddr = (IHAL_UINT32)src->vaddr;//
                                frame.frameInfo.pixfmt = ctx->codecParam.m_fourCC;  //  fix fmt
                                ret = AL_Codec_Encode_Process(ctx->pCodecEncode, &frame);
                                Fifo_Queue(&ctx->src_done_FiFo, src, AL_NO_WAIT);
                        } else {
                                // INTERNAL buffer
                                src = Fifo_Dequeue(&ctx->src_queued_FiFo, AL_WAIT_FOREVER); //
                                if (!src) {
                                        IMPP_LOGE(TAG, "dequeue src_queued_FiFo failed");
                                        continue;
                                }
                                pSrc = src->AL_SrcTBuf;
                                AL_BufPool_GetBuffer(&ctx->pCodecEncode->m_SrcBufPool, AL_BUF_MODE_NONBLOCK);
                                ret = AL_Codec_Encode_Process_V2(ctx->pCodecEncode, pSrc);
                                Fifo_Queue(&ctx->src_done_FiFo, src, AL_NO_WAIT);
                        }
                        pthread_mutex_lock(&ctx->mutex);
                        ctx->encoding_count += 1;
                        pthread_mutex_unlock(&ctx->mutex);
                } else {
                        /* usleep(5000); */
                        return (void *)0;
                }
        }
}


void *vcodec_enc_thread(void *arg)
{
        int ret = 0;
        IHal_CodecHandle_t *handle = (IHal_CodecHandle_t *)arg;
        Video_CodecCtx_t *ctx = (Video_CodecCtx_t *)handle->ctx;
        AL_TBuffer *pStream = NULL;
        AL_TBuffer *pSrc = NULL;

        while (1) {
                if (ctx->vcodec_run_sta == VCODEC_STA_RUNING) {
                        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
                        if (AL_Codec_Encode_GetStream(ctx->pCodecEncode, &pStream, &pSrc) >= 0) {
                                sem_post(&ctx->src_available_cnt_sem);

                                pthread_mutex_lock(&ctx->mutex);
                                fill_one_streambuffer(handle, pStream);
                                pthread_mutex_unlock(&ctx->mutex);
                                AL_Codec_Encode_ReleaseStream(ctx->pCodecEncode, pStream, pSrc);
                        } else {
                                IMPP_LOGW(TAG, "get stream failed");
                        }
                        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
                } else {
                        /* usleep(50000); */
                        /* IMPP_LOGD(TAG,"encode stop sta"); */
                        return (void *)0;
                }
        }
}

static int vcodec_start(IHal_CodecHandle_t *handle)
{
        assert(handle);
        assert(handle->ctx);
        Video_CodecCtx_t *ctx = (Video_CodecCtx_t *)handle->ctx;
        pthread_mutex_lock(&ctx->mutex);
        ctx->vcodec_run_sta = VCODEC_STA_RUNING;
        ctx->encoding_count = 0;
        ctx->finishencoding_count = 0;
        pthread_mutex_unlock(&ctx->mutex);
        if (!ctx->encoderTid) {
                IHAL_INT32 ret = pthread_create(&ctx->encoderTid, NULL, vcodec_enc_thread, (void *)handle);
                if (ret) {
                        perror("pthread Create : ");
                        IMPP_LOGE(TAG, "create encoder thread failed ret = %d", ret);
                        return IHAL_RERR;
                }
        }
        if (!ctx->frameTid) {
                IHAL_INT32 ret = pthread_create(&ctx->frameTid, NULL, frame_update_thread, (void *)handle);
                if (ret) {
                        perror("pthread Create : ");
                        IMPP_LOGE(TAG, "create frame thread failed ret = %d", ret);
                        return IHAL_RERR;
                }
        }
        return 0;
}

static int vcodec_stop(IHal_CodecHandle_t *handle)
{
        assert(handle);
        assert(handle->ctx);
        Video_CodecCtx_t *ctx = (Video_CodecCtx_t *)handle->ctx;
        pthread_mutex_lock(&ctx->mutex);
        ctx->vcodec_run_sta = VCODEC_STA_STOP;
        pthread_mutex_unlock(&ctx->mutex);
        return IHAL_ROK;
}

static int audio_codec_start(IHal_CodecHandle_t *handle)
{
        // TODO
        return 0;
}

static int audio_codec_stop(IHal_CodecHandle_t *handle)
{
        //TODO
        return 0;
}

static int vcodec_queueSrc(IHal_CodecHandle_t *handle, IHAL_INT32 idx)
{
        assert(handle->ctx);
        Video_CodecCtx_t *ctx = (Video_CodecCtx_t *)handle->ctx;
        Fifo_Queue(&ctx->src_queued_FiFo, &ctx->srcbuf[idx], AL_WAIT_FOREVER);
        return IHAL_ROK;
}

static int vcodec_dequeueSrc(IHal_CodecHandle_t *handle, IMPP_BufferInfo_t *buf)
{
        assert(handle->ctx);
        Video_CodecCtx_t *ctx = (Video_CodecCtx_t *)handle->ctx;
        video_srcbuf_info_t *pbuf = Fifo_Dequeue(&ctx->src_done_FiFo, AL_NO_WAIT);
        if (!pbuf) {
                IMPP_LOGE(TAG, "dequeue src done fifo failed");
                return -IHAL_RFAILED;
        }
        //pthread_mutex_lock(&ctx->mutex);
        buf->vaddr = pbuf->vaddr;
        buf->paddr = pbuf->paddr;
        buf->fd      = pbuf->dmafd;
        buf->size  = pbuf->size;
        buf->index   = pbuf->index;
        //pthread_mutex_unlock(&ctx->mutex);

        return IHAL_ROK;
}

static int vcodec_waitSrcAvailable(IHal_CodecHandle_t *handle, IHAL_INT32 wait)
{
        assert(handle->ctx);
        Video_CodecCtx_t *ctx = (Video_CodecCtx_t *)handle->ctx;
        if (wait == IMPP_NO_WAIT) {
                return sem_trywait(&ctx->src_available_cnt_sem);
        } else {
                return sem_wait(&ctx->src_available_cnt_sem);
        }
}

static int vcodec_waitDstAvailable(IHal_CodecHandle_t *handle, IHAL_INT32 wait)
{
        assert(handle->ctx);
        Video_CodecCtx_t *ctx = (Video_CodecCtx_t *)handle->ctx;
        if (wait == IMPP_NO_WAIT) {
                return sem_trywait(&ctx->dst_available_cnt_sem);
        } else {
                return sem_wait(&ctx->dst_available_cnt_sem);
        }

}

static int vcodec_queueDst(IHal_CodecHandle_t *handle, IHAL_INT32 idx)
{
        assert(handle->ctx);
        Video_CodecCtx_t *ctx = (Video_CodecCtx_t *)handle->ctx;
        Fifo_Queue(&ctx->dst_queued_FiFo, &ctx->dstbuf[idx], AL_WAIT_FOREVER);
        return 0;
}

static int vcodec_dequeueDst(IHal_CodecHandle_t *handle, IHAL_CodecStreamInfo_t *buf)
{
        assert(handle->ctx);
        Video_CodecCtx_t *ctx = (Video_CodecCtx_t *)handle->ctx;
        video_dstbuf_info_t *out = NULL;
        out = Fifo_Dequeue(&ctx->dst_done_FiFo, AL_NO_WAIT);
        if (!out) {
                IMPP_LOGE(TAG, "dst_done_FiFo dequeue failed");
                return -1;
        }
        //pthread_mutex_lock(&ctx->mutex);
        buf->vaddr = out->vaddr;
        buf->index   = out->index;
        buf->size = out->uselength;
        buf->pack.seq = out->stream.seq;
        buf->pack.packcnt = out->stream.packcnt;
        buf->pack.pack = &out->stream.pack[0];
        //pthread_mutex_unlock(&ctx->mutex);
        return 0;
}

static int _encoder_set_qpbounds(IHal_CodecHandle_t *handle,IHal_CodecControlParam_t *p)
{
	int MinQp = (int)p->priv[0];
	int MaxQp = (int)p->priv[1];
    Video_CodecCtx_t *ctx = (Video_CodecCtx_t *)handle->ctx;
	if(MinQp < 0 || MinQp > 51 || MaxQp < 0 || MaxQp > 51){
		IMPP_LOGE(TAG,"invalid QP value \n");
		return -1;
	}
	printf("%s minqp = %d maxqp = %d \n",__func__,MinQp,MaxQp);
	return AL_Codec_Encode_SetQpBounds(ctx->pCodecEncode,MinQp,MaxQp);
}

static _encoder_set_bitrate(IHal_CodecHandle_t *handle,IHal_CodecControlParam_t *p)
{
	int tar_bitrate = (int)p->priv[0];
	int max_bitrate = (int)p->priv[1];
    Video_CodecCtx_t *ctx = (Video_CodecCtx_t *)handle->ctx;
	if(tar_bitrate <= 0 || max_bitrate <= 0){
		IMPP_LOGE(TAG,"invalid bitrate value \n");
		return -1;
	}
	/* printf("%s tar_bitrate = %d max_bitrate = %d \n",__func__,tar_bitrate,max_bitrate); */
	return AL_Codec_Encode_SetBitRate(ctx->pCodecEncode,tar_bitrate * 1000,max_bitrate * 1000);
}

IHal_CodecHandle_t *IHal_CodecCreate(CODEC_TYPE type)
{
        return vcodec_create(type);
}

IHAL_INT32 IHal_CodecInit(void)
{
        int ret = al_encoder_init();
        if (ret) {
                IMPP_LOGE(TAG, "codec (x2500) init failed");
                return -IHAL_RERR;
        }
        return IHAL_ROK;
}

IHAL_INT32 IHal_CodecDeInit(void)
{
        al_encoder_exit();
        return IHAL_ROK;
}

IHAL_INT32 IHal_CodecDestroy(IHal_CodecHandle_t *handle)
{
        assert(handle);
        assert(handle->ctx);
        return vcodec_destroy(handle);
}

IHAL_INT32 IHal_Codec_SetParams(IHal_CodecHandle_t *handle, IHal_CodecParam *param)
{
        IHAL_INT32 ret = 0;
        assert(handle);
        assert(handle->ctx);
        switch (handle->codectype) {
        case JPEG_DEC:
        case JPEG_ENC:
                ret = set_jpeg_codec_param(handle, param);
                break;
        case H264_DEC:
        case H264_ENC:
                ret = set_h264_codec_param(handle, param);
                break;
        case H265_DEC:
        case H265_ENC:
                ret = set_hevc_codec_param(handle, param);
                break;
        case AUDIO_G711A:
                ret = set_audio_codec_param(handle, param);
                break;
        default:
                IMPP_LOGE(TAG, "not support codec type\n");
                return -IHAL_RERR;
        }

        if (ret) {
                IMPP_LOGE(TAG, "set codec param failed");
                return -IHAL_RFAILED;
        }

        return -IHAL_ROK;
}

IHAL_INT32 IHal_Codec_GetParams(IHal_CodecHandle_t *ctx, IHal_CodecParam *param)
{
        // TODO
        return IHAL_ROK;
}
IHAL_INT32 IHal_Codec_SetSrcBuffer(IHal_CodecHandle_t *handle, int index, IMPP_BufferInfo_t *sharebuf)
{
        assert(handle);
        return vcodec_setSrcBuf(handle, index, sharebuf);
}

IHAL_INT32 IHal_Codec_GetSrcBuffer(IHal_CodecHandle_t *handle, int index, IMPP_BufferInfo_t *sharebuf)
{
        assert(handle);
        return vcodec_getSrcBuf(handle, index, sharebuf);
}

IHAL_INT32 IHal_Codec_CreateSrcBuffers(IHal_CodecHandle_t *handle, IMPP_BUFFER_TYPE buftype, IHAL_INT32 create_num)
{
        IHAL_INT32 ret = 0;
        assert(handle);
        assert(handle->ctx);
        switch (handle->codectype) {
        case JPEG_ENC:
        case H264_ENC:
        case H265_ENC:
                ret = video_codec_createbuffer(handle, buftype, create_num, 1);
                break;
        case AUDIO_G711A:
                ret = audio_codec_createbuffer(handle, buftype, create_num, 1);
                break;
        default:
                IMPP_LOGE(TAG, "not support codec type\n");
                return -IHAL_RERR;
        }

        if (ret < 0) {
                IMPP_LOGE(TAG, "codec buffer create failed");
                return -IHAL_RFAILED;
        }

        return ret;
}

IHAL_INT32 IHal_Codec_CreateDstBuffer(IHal_CodecHandle_t *handle, IMPP_BUFFER_TYPE buftype, IHAL_INT32 create_num)
{
        IHAL_INT32 ret = 0;
        assert(handle);
        assert(handle->ctx);
        switch (handle->codectype) {
        case JPEG_ENC:
        case H264_ENC:
        case H265_ENC:
                ret = video_codec_createbuffer(handle, buftype, create_num, 0);
                break;
        case AUDIO_G711A:
                ret = audio_codec_createbuffer(handle, buftype, create_num, 0);
                break;
        default:
                IMPP_LOGE(TAG, "not support codec type\n");
                return -IHAL_RERR;
        }

        if (ret < 0) {
                IMPP_LOGE(TAG, "codec buffer create failed");
                return -IHAL_RFAILED;
        }

        return ret;

}

IHAL_INT32 IHal_Codec_SetDstBuffer(IHal_CodecHandle_t *handle, int index, IMPP_BufferInfo_t *sharebuf)
{
        if (handle->codectype & 0x0f) {
                IMPP_LOGW(TAG, "x2500 video encoder dst buf just support internal-buf,so cannot set dstbuf");
                return -IHAL_RFAILED;
        } else {
                // TODO
                IMPP_LOGE(TAG, "not support codec type yet");
                return IHAL_ROK;
        }
}

IHAL_INT32 IHal_Codec_GetDstBuffer(IHal_CodecHandle_t *handle, int index, IMPP_BufferInfo_t *sharebuf)
{
        if (handle->codectype & 0x0f) {
                IMPP_LOGW(TAG, "x2500 video encoder dst buf just support internal-buf,so cannot set dstbuf");
                return -IHAL_RFAILED;
        } else {
                // TODO
                IMPP_LOGE(TAG, "not support codec type yet");
                return IHAL_ROK;
        }
}

IHAL_INT32 IHal_Codec_Start(IHal_CodecHandle_t *handle)
{
        IHAL_INT32 ret = 0;
        assert(handle);
        assert(handle->ctx);
        switch (handle->codectype) {
        case JPEG_ENC:
        case H264_ENC:
        case H265_ENC:
                ret = vcodec_start(handle);
                break;
        case AUDIO_G711A:
                ret = audio_codec_start(handle);
                break;
        default:
                IMPP_LOGE(TAG, "not support codec type\n");
                return -IHAL_RERR;
        }
        if (ret) {
                IMPP_LOGE(TAG, "IHal_Codec_Start error");
                return -IHAL_RERR;
        }

        return IHAL_ROK;
}

IHAL_INT32 IHal_Codec_Stop(IHal_CodecHandle_t *handle)
{
        assert(handle);
        assert(handle->ctx);
        switch (handle->codectype) {
        case JPEG_ENC:
        case H264_ENC:
        case H265_ENC:
                vcodec_stop(handle);
                break;
        case AUDIO_G711A:
                audio_codec_stop(handle);
                break;
        default:
                IMPP_LOGE(TAG, "not support codec type\n");
                return -IHAL_RERR;
        }
        return IHAL_ROK;
}

IHAL_INT32 IHal_Codec_WaitSrcAvailable(IHal_CodecHandle_t *handle, IHAL_INT32 wait_type)
{
        IHAL_INT32 ret = 0;
        assert(handle);
        assert(handle->ctx);
        switch (handle->codectype) {
        case JPEG_ENC:
        case H264_ENC:
        case H265_ENC:
                ret = vcodec_waitSrcAvailable(handle, wait_type);
                break;
        case AUDIO_G711A:
        default:
                IMPP_LOGE(TAG, "not support codec type %d \n", handle->codectype);
                return -IHAL_RERR;
        }

        if (ret) {
                return -IHAL_RERR;
        }

        return IHAL_ROK;
}

IHAL_INT32 IHal_Codec_QueueSrcBuffer(IHal_CodecHandle_t *handle, IMPP_BufferInfo_t *buf)
{
        assert(handle);
        int index = buf->index;
        assert(index >= 0);
        IHAL_INT32 ret = 0;
        switch (handle->codectype) {
        case JPEG_ENC:
        case H264_ENC:
        case H265_ENC:
                ret = vcodec_queueSrc(handle, index);
                break;
        case AUDIO_G711A:
        default:
                IMPP_LOGE(TAG, "not support codec type %d \n", handle->codectype);
                return -IHAL_RERR;
        }

        if (ret) {
                return -IHAL_RERR;
        }

        return IHAL_ROK;

}

IHAL_INT32 IHal_Codec_DequeueSrcBuffer(IHal_CodecHandle_t *handle, IMPP_BufferInfo_t *buf)
{
        IHAL_INT32 ret = 0;
        assert(handle);
        switch (handle->codectype) {
        case JPEG_ENC:
        case H264_ENC:
        case H265_ENC:
                ret = vcodec_dequeueSrc(handle, buf);
                break;
        case AUDIO_G711A:
        default:
                IMPP_LOGE(TAG, "not support codec type %d \n", handle->codectype);
                return -IHAL_RERR;
        }

        if (ret) {
                return -IHAL_RERR;
        }

        return IHAL_ROK;
}

IHAL_INT32 IHal_Codec_WaitDstAvailable(IHal_CodecHandle_t *handle, IHAL_INT32 wait_type)
{
        IHAL_INT32 ret = 0;
        assert(handle);
        assert(handle->ctx);
        switch (handle->codectype) {
        case JPEG_ENC:
        case H264_ENC:
        case H265_ENC:
                ret = vcodec_waitDstAvailable(handle, wait_type);
                break;
        case AUDIO_G711A:
        default:
                IMPP_LOGE(TAG, "not support codec type %d \n", handle->codectype);
                return -IHAL_RERR;
        }

        if (ret) {
                return -IHAL_RERR;
        }

        return IHAL_ROK;
}

IHAL_INT32 IHal_Codec_QueueDstBuffer(IHal_CodecHandle_t *handle, IHAL_CodecStreamInfo_t *buf)
{
        IHAL_INT32 ret = 0;
        IHAL_INT32 index = buf->index;
        assert(index >= 0);
        assert(handle);
        switch (handle->codectype) {
        case JPEG_ENC:
        case H264_ENC:
        case H265_ENC:
                ret = vcodec_queueDst(handle, index);
                break;
        case AUDIO_G711A:
        default:
                IMPP_LOGE(TAG, "not support codec type %d \n", handle->codectype);
                return -IHAL_RERR;
        }

        if (ret) {
                return -IHAL_RERR;
        }

        return IHAL_ROK;
}

IHAL_INT32 IHal_Codec_DequeueDstBuffer(IHal_CodecHandle_t *handle, IHAL_CodecStreamInfo_t *buf)
{
        IHAL_INT32 ret = 0;
        assert(handle);
        assert(buf);
        switch (handle->codectype) {
        case JPEG_ENC:
        case H264_ENC:
        case H265_ENC:
                ret = vcodec_dequeueDst(handle, buf);
                break;
        case AUDIO_G711A:
        default:
                IMPP_LOGE(TAG, "not support codec type %d \n", handle->codectype);
                return -IHAL_RERR;
        }

        if (ret) {
                return -IHAL_RERR;
        }

        return IHAL_ROK;
}



IHAL_INT32 IHal_Codec_Control(IHal_CodecHandle_t *handle, IHal_CodecControlParam_t *param)
{
        assert(handle);
        int ret = 0;
        switch (param->cmd) {
        case ENCODER_IDR_REQUEST:
                ret = _encoder_request_idr(handle);
                break;
		case ENCODER_SET_BITRATE:
				ret = _encoder_set_bitrate(handle,param);
				break;
		case ENCODER_SET_QP_BOUNDS:
				ret = _encoder_set_qpbounds(handle,param);
				break;
        default :
                IMPP_LOGE(TAG, "not support cmd type");
                return -IHAL_RERR;
        }
        if (ret) {
                return -IHAL_RERR;
        }
        return IHAL_ROK;
}

IHAL_INT32 IHal_EncoderRoiSet(IHal_CodecHandle_t *handle, IHAL_EncoderRoiAttr *attr)
{
        assert(handle);
        int ret = 0;
        ret =  _encoder_set_roi(handle, attr);
        if (ret) {
                return -IHAL_RERR;
        }
        return IHAL_ROK;
}
