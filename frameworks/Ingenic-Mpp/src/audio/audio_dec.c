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

#define TAG "ADEC"
#define MAX_ADEC_CHAN   2
#define AENC_SLEEP_US   8000

typedef struct {
        pthread_mutex_t lock;
        void *buf_head;
        unsigned int dstsize;
        IHal_AudioCodecType_t type;
        void *decoder_prv;
        int (*decoder)(void *decoder_prv, char *indata, char *outdata, int len);
        void (*decoder_deinit)(void *prv);
} audio_decoder_chn_t;

typedef struct {
        unsigned int size;
        char data[0];
} audio_decoder_data_t;


audio_decoder_chn_t audioDecoder[MAX_ADEC_CHAN];
static unsigned int chn_cnt = 0;
static pthread_mutex_t module_lock = PTHREAD_MUTEX_INITIALIZER;

static int _adpcm_decode_frm(void *decoder_prv, char *indata, char *outdata, int len)
{
        return adpcm_decode(outdata, indata, len);
}

static int _g711a_decode_frm(void *decoder_prv, char *indata, char *outdata, int len)
{
        int ret = 0;
        ret = g711a_decode(outdata, indata, len);
        return ret * 2;
}

static int _g711u_decode_frm(void *decoder_prv, char *indata, char *outdata, int len)
{
        int ret = g711u_decode(outdata, indata, len);
        return ret * 2;
}

static int _g726_decode_frm(void *decoder_prv, char *indata, char *outdata, int len)
{
        int ret = 0;
        ret = g726_decode(decoder_prv, outdata, indata, len);
        return ret * 2;     // out data is short
}

static int _aac_decode_frm(void *decoder_prv, char *indata, char *outdata, int len)
{
        return aac_decode(decoder_prv, indata, len, outdata);
}


IHal_AudioDec_Handle_t *IHal_AudioDec_CreateChn(IHal_AudioDec_Attr_t *attr)
{
        if (chn_cnt > MAX_ADEC_CHAN) {
                IMPP_LOGE(TAG, "max audio decoder chan num is 2");
                return IHAL_RNULL;
        }
        audio_decoder_chn_t *chn = &audioDecoder[chn_cnt];

        pthread_mutex_init(&chn->lock, NULL);
        chn->decoder_deinit = NULL;
        aacDec_prv_attr_t aac_prattr;
        switch (attr->type) {
        case Codec_ADPCM:
                chn->decoder_prv = NULL;
                chn->decoder = _adpcm_decode_frm;
                break;
        case Codec_G711A:
                chn->decoder_prv = NULL;
                chn->decoder = _g711a_decode_frm;
                break;
        case Codec_G711U:
                chn->decoder_prv = NULL;
                chn->decoder = _g711u_decode_frm;
                break;
        case Codec_G726:
                chn->decoder_prv = g726_decoder_init(attr->BitRate);
                if (!chn->decoder_prv) {
                        IMPP_LOGE(TAG, "g726_decoder_init failed");
                        return IHAL_RNULL;
                }
                chn->decoder = _g726_decode_frm;
                break;
        case Codec_AAC_RAW:
                aac_prattr.aot = 2;
                aac_prattr.transmux = MP4_RAW;
                chn->decoder_prv = aac_decoder_init(attr, &aac_prattr);
                if (!chn->decoder_prv) {
                        IMPP_LOGE(TAG, "aac raw decoder failed");
                        return IHAL_RNULL;
                }
                chn->decoder = _aac_decode_frm;
                chn->decoder_deinit = aac_decoder_deinit;
                break;
        case Codec_AAC_ADTS:
                aac_prattr.aot = 2;
                aac_prattr.transmux = MP4_ADTS;
                chn->decoder_prv = aac_decoder_init(attr, &aac_prattr);
                if (!chn->decoder_prv) {
                        IMPP_LOGE(TAG, "aac raw decoder failed");
                        return IHAL_RNULL;
                }
                chn->decoder = _aac_decode_frm;
                chn->decoder_deinit = aac_decoder_deinit;
                break;
        default :
                IMPP_LOGE(TAG, "not support encode type");
                break;
        }
        chn->type = attr->type;
        chn->dstsize =  sizeof(short) * attr->AO_numPerSample * attr->Channels;
        void *buf = audio_buf_alloc(attr->bufsize, chn->dstsize + sizeof(audio_decoder_data_t), 0);
        if (!buf) {
                IMPP_LOGE(TAG, "alloc audio buffer failed");
                return IHAL_RNULL;
        }
        pthread_mutex_unlock(&chn->lock);
        chn->buf_head = buf;
        chn_cnt += 1;
        return (IHal_AudioDec_Handle_t *)chn;
}

IHAL_INT32 IHal_AudioDec_DestroyChn(IHal_AudioDec_Handle_t *handle)
{
        assert(handle);
        audio_decoder_chn_t *chn = (audio_decoder_chn_t *)handle;
        pthread_mutex_lock(&chn->lock);

        if (chn->decoder_deinit && chn->decoder_prv) {
                chn->decoder_deinit(chn->decoder_prv);
        }

        if (chn->decoder_prv) {
                free(chn->decoder_prv);
                chn->decoder_prv = NULL;
        }
        audio_buf_free(chn->buf_head);
        pthread_mutex_unlock(&chn->lock);
        pthread_mutex_destroy(&chn->lock);
        pthread_mutex_lock(&module_lock);
        chn_cnt -= 1;
        pthread_mutex_unlock(&module_lock);
        return IHAL_ROK;

}

IHAL_INT32 IHal_AudioDec_PutStream(IHal_AudioDec_Handle_t *handle, IHal_AudioStream_t *stream, IHAL_INT32 block)
{
        assert(handle);
        audio_decoder_chn_t *chn = (audio_decoder_chn_t *)handle;
        int ret = 0;
        void *node = NULL;
        assert(chn->buf_head);
        if (stream->vaddr == 0 || stream->size <= 0) {
                return -IHAL_RFAILED;
        }

        do {
                pthread_mutex_lock(&chn->lock);
                node = audio_buf_get_node(chn->buf_head, AUDIO_BUF_EMPTY);
                pthread_mutex_unlock(&chn->lock);
                if (node) {
                        pthread_mutex_lock(&chn->lock);
                        audio_decoder_data_t *data = audio_buf_node_get_data(node);
                        pthread_mutex_unlock(&chn->lock);
                        /* data->size = ; */

                        if (chn->decoder) {
                                data->size = chn->decoder(chn->decoder_prv, stream->vaddr, data->data, stream->size);
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

IHAL_INT32 IHal_AudioDec_GetFrame(IHal_AudioEnc_Handle_t *handle, IHal_AudioStream_t *frame, IHAL_INT32 block)
{
        assert(handle);
        audio_decoder_chn_t *chn = (audio_decoder_chn_t *)handle;
        void *node = NULL;
        if (frame == NULL) {
                return -IHAL_RFAILED;
        }
        do {
                pthread_mutex_lock(&chn->lock);
                node = audio_buf_get_node(chn->buf_head, AUDIO_BUF_FULL);
                pthread_mutex_unlock(&chn->lock);
                if (node) {
                        pthread_mutex_lock(&chn->lock);
                        audio_decoder_data_t *data = audio_buf_node_get_data(node);
                        pthread_mutex_unlock(&chn->lock);
                        frame->node = node;
                        frame->vaddr = data->data;
                        frame->size = data->size;

                        return IHAL_ROK;
                }

                usleep(AENC_SLEEP_US);

        } while (block == IMPP_WAIT_FOREVER);

        return -IHAL_RFAILED;
}

IHAL_INT32 IHal_AudioDec_ReleaseFrame(IHal_AudioEnc_Handle_t *handle, IHal_AudioStream_t *frame)
{
        assert(handle);
        audio_decoder_chn_t *chn = (audio_decoder_chn_t *)handle;
        void *node = NULL;
        if (frame->node) {
                pthread_mutex_lock(&chn->lock);
                audio_buf_put_node(chn->buf_head, frame->node, AUDIO_BUF_EMPTY);
                pthread_mutex_unlock(&chn->lock);
        } else {
                return -IHAL_RFAILED;
        }
        return IHAL_ROK;

}

