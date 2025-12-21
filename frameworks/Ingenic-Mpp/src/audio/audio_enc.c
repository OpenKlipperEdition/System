#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sched.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#include "audio.h"
#include "log.h"
#include "abuf.h"
#include "adpcm.h"
#include "g711codec.h"
#include "g726codec.h"
#include "aaccodec.h"


#define TAG "AENC"
#define MAX_AENC_CHAN   4
#define AENC_SLEEP_US   8000
#define MIN_BUF_NUM     12
typedef struct {
        pthread_mutex_t lock;
        void *buf_head;
        unsigned int srcsize;
        struct timeval base_tv;
        unsigned long long counter;
        IHal_AudioCodecType_t type;
        void *encoder_prv;
        int (*encoder)(void *encoder_prv, char *indata, char *outdata, int len);
        void (*encoder_deinit)(void *prv);
} audio_encoder_chn_t;

typedef struct {
        unsigned int size;
        char data[0];
} audio_encoder_data_t;

audio_encoder_chn_t audioEncoder[MAX_AENC_CHAN];
static unsigned int chn_cnt = 0;
static pthread_mutex_t module_lock = PTHREAD_MUTEX_INITIALIZER;

static uint64_t timeval_diff(struct timeval *tv0, struct timeval *tv1)
{
        uint64_t time0, time1;
        time0 = tv0->tv_sec * 1000000 + tv0->tv_usec;
        time1 = tv1->tv_sec * 1000000 + tv1->tv_usec;
        return (time1 > time0) ? (time1 - time0) : (time0 - time1);
}

static int _adpcm_encode_frm(void *encoder_prv, char *indata, char *outdata, int len)
{
        return adpcm_encode(outdata, indata, len);
}

static int _g711a_encode_frm(void *encoder_prv, char *indata, char *outdata, int len)
{
        return g711a_encode(outdata, indata, len / 2);
}

static int _g711u_encode_frm(void *encoder_prv, char *indata, char *outdata, int len)
{
        return g711u_encode(outdata, indata, len / 2);
}

static int _g726_encode_frm(void *encoder_prv, char *indata, char *outdata, int len)
{
        // the len is indicate ,length  short type
        return g726_encode(encoder_prv, outdata, (short *)indata, len / 2);
}

static int _aac_encode_frm(void *encoder_prv, char *indata, char *outdata, int len)
{
        return aac_encode(encoder_prv, (short *)indata, len, outdata);
}

static int __imppfmt_to_bytewidth(int imppfmt)
{
        int ret = 0;
        switch (imppfmt) {
        case IMPP_SAMPLE_FMT_S8:
        case IMPP_SAMPLE_FMT_U8:
                ret = 1;
                break;
        case IMPP_SAMPLE_FMT_S16:
        case IMPP_SAMPLE_FMT_U16:
                ret = 2;
                break;
        case IMPP_SAMPLE_FMT_S24:
        case IMPP_SAMPLE_FMT_U24:
                ret = 3;
                break;
        case IMPP_SAMPLE_FMT_S32:
        case IMPP_SAMPLE_FMT_U32:
                ret = 4;
                break;
        default:
                ret = -1;
        }
        return ret;
}

IHal_AudioEnc_Handle_t *IHal_AudioEnc_CreateChn(IHal_AudioEnc_Attr_t *attr)
{
        if (chn_cnt > MAX_AENC_CHAN) {
                IMPP_LOGE(TAG, "max audio encoder chan num is 4");
                return IHAL_RNULL;
        }
        if (attr->bufsize < MIN_BUF_NUM) {
                IMPP_LOGE(TAG, "bufsize (num of enc buffer ) need greater than 12");
                return IHAL_RNULL;
        }
        audio_encoder_chn_t *chn = &audioEncoder[chn_cnt];
        pthread_mutex_init(&chn->lock, NULL);
        chn->encoder_prv = NULL;
        chn->encoder_deinit = NULL;
        aacEnc_prv_attr_t prv_attr;
        switch (attr->type) {
        case Codec_ADPCM:
                chn->encoder_prv = NULL;
                chn->encoder = _adpcm_encode_frm;
                break;
        case Codec_G711A:
                chn->encoder_prv = NULL;
                chn->encoder = _g711a_encode_frm;
                break;
        case Codec_G711U:
                chn->encoder_prv = NULL;
                chn->encoder = _g711u_encode_frm;
                break;
        case Codec_G726:
                chn->encoder_prv = g726_encoder_init(attr->BitRate);
                if (!chn->encoder_prv) {
                        IMPP_LOGE(TAG, "g726_encoder_init failed");
                        return IHAL_RNULL;
                }
                chn->encoder = _g726_encode_frm;
                break;
        case Codec_AAC_ADTS:
                prv_attr.aot = 2;
                prv_attr.sbr_enable = 0;
                prv_attr.vbr_type = 0;
                prv_attr.transmux = MP4_ADTS;
                chn->encoder_prv = aac_encoder_init(attr, &prv_attr);
                chn->encoder_deinit = aac_encoder_deinit;
                chn->encoder = _aac_encode_frm;
                break;
        case Codec_AAC_ADIF:
                prv_attr.aot = 2;
                prv_attr.sbr_enable = 0;
                prv_attr.vbr_type = 0;
                prv_attr.transmux = MP4_ADIF;
                chn->encoder_prv = aac_encoder_init(attr, &prv_attr);
                chn->encoder_deinit = aac_encoder_deinit;
                chn->encoder = _aac_encode_frm;
                break;
        case Codec_AAC_RAW:
                prv_attr.aot = 2;
                prv_attr.sbr_enable = 0;
                prv_attr.vbr_type = 0;
                prv_attr.transmux = MP4_RAW;
                chn->encoder_prv = aac_encoder_init(attr, &prv_attr);
                chn->encoder_deinit = aac_encoder_deinit;
                chn->encoder = _aac_encode_frm;
                break;
        default :
                IMPP_LOGE(TAG, "not support encode type");
                break;
        }
        chn->type = attr->type;
        int bytewidth = __imppfmt_to_bytewidth(attr->SampleFmt);
        if (bytewidth < 0) {
                IMPP_LOGE(TAG, "source pcm data format error");
                return IHAL_RNULL;
        }
        chn->srcsize = bytewidth * attr->Channels * attr->numPerSample;
        pthread_mutex_lock(&chn->lock);
        void *buf = audio_buf_alloc(attr->bufsize, chn->srcsize + sizeof(audio_encoder_data_t), 0);
        if (!buf) {
                IMPP_LOGE(TAG, "alloc audio buffer failed");
                return IHAL_RNULL;
        }
        pthread_mutex_unlock(&chn->lock);
        gettimeofday(&chn->base_tv, NULL);
        chn->counter = 0;
        chn->buf_head = buf;
        chn_cnt += 1;
        return (IHal_AudioEnc_Handle_t *)chn;
}

IHAL_INT32 IHal_AudioEnc_DestroyChn(IHal_AudioEnc_Handle_t *handle)
{
        assert(handle);
        audio_encoder_chn_t *chn = (audio_encoder_chn_t *)handle;
        pthread_mutex_lock(&chn->lock);
        if (chn->encoder_deinit && chn->encoder_prv) {
                chn->encoder_deinit(chn->encoder_prv);
                chn->encoder_deinit = NULL;
        }
        if (chn->encoder_prv) {
                free(chn->encoder_prv);
                chn->encoder_prv = NULL;
        }
        chn->encoder = NULL;
        audio_buf_free(chn->buf_head);
        pthread_mutex_unlock(&chn->lock);
        pthread_mutex_destroy(&chn->lock);
        pthread_mutex_lock(&module_lock);
        chn_cnt -= 1;
        pthread_mutex_unlock(&module_lock);
        return IHAL_ROK;
}

IHAL_INT32 IHal_AudioEnc_PutFrame(IHal_AudioEnc_Handle_t *handle, IHal_AudioStream_t *frame, IHAL_INT32 block)
{
        assert(handle);
        audio_encoder_chn_t *chn = (audio_encoder_chn_t *)handle;
        int ret = 0;
        void *node = NULL;
        assert(chn->buf_head);
        if (frame->vaddr == 0 || frame->size <= 0) {
                return -IHAL_RFAILED;
        }
        do {
                pthread_mutex_lock(&chn->lock);
                node = audio_buf_get_node(chn->buf_head, AUDIO_BUF_EMPTY);
                pthread_mutex_unlock(&chn->lock);
                if (node) {
                        pthread_mutex_lock(&chn->lock);
                        audio_encoder_data_t *data = audio_buf_node_get_data(node);
                        pthread_mutex_unlock(&chn->lock);
                        data->size = chn->srcsize;

                        if (chn->encoder) {
                                data->size = chn->encoder(chn->encoder_prv, frame->vaddr, data->data, frame->size);
                        }

                        pthread_mutex_lock(&chn->lock);
                        audio_buf_put_node(chn->buf_head, node, AUDIO_BUF_FULL);
                        pthread_mutex_unlock(&chn->lock);

                        return IHAL_ROK;
                } else {
                        printf("get node failed\r\n");
                }

                usleep(AENC_SLEEP_US);

        } while (block == IMPP_WAIT_FOREVER);

        return -IHAL_RFAILED;
}

IHAL_INT32 IHal_AudioEnc_GetStream(IHal_AudioEnc_Handle_t *handle, IHal_AudioStream_t *stream, IHAL_INT32 block)
{
        assert(handle);
        audio_encoder_chn_t *chn = (audio_encoder_chn_t *)handle;
        void *node = NULL;
        struct timeval cur_tv;
        if (stream == NULL) {
                return -IHAL_RFAILED;
        }
        do {
                pthread_mutex_lock(&chn->lock);
                node = audio_buf_get_node(chn->buf_head, AUDIO_BUF_FULL);
                pthread_mutex_unlock(&chn->lock);
                if (node) {
                        pthread_mutex_lock(&chn->lock);
                        audio_encoder_data_t *data = audio_buf_node_get_data(node);
                        pthread_mutex_unlock(&chn->lock);
                        gettimeofday(&cur_tv, NULL);
                        stream->node = node;
                        stream->vaddr = data->data;
                        stream->size = data->size;
                        stream->seq = chn->counter++;
                        stream->timestamp = timeval_diff(&chn->base_tv, &cur_tv);
                        return IHAL_ROK;
                }

                usleep(AENC_SLEEP_US);

        } while (block == IMPP_WAIT_FOREVER);

        return -IHAL_RFAILED;
}

IHAL_INT32 IHal_AudioEnc_ReleaseStream(IHal_AudioEnc_Handle_t *handle, IHal_AudioStream_t *stream)
{
        assert(handle);
        audio_encoder_chn_t *chn = (audio_encoder_chn_t *)handle;
        void *node = NULL;
        if (stream->node) {
                pthread_mutex_lock(&chn->lock);
                audio_buf_put_node(chn->buf_head, stream->node, AUDIO_BUF_EMPTY);
                pthread_mutex_unlock(&chn->lock);
        } else {
                return -IHAL_RFAILED;
        }
        return IHAL_ROK;
}

