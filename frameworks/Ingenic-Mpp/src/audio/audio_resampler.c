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
#include "speex_resampler.h"

#define TAG "Resamlper"

typedef struct {
        IHal_ResamplerAttr_t attr;
        short *outbuffer;
        short *processInbuffer;
        short *processOutbuffer;
        pthread_mutex_t lock;
        SpeexResamplerState *state;
} resamlper_ctx_t;

int get_num_samples_10ms(unsigned int sampele_rate)
{
#if 0
        switch (sampele_rate) {
        case 8000:
                return 80;
        case 16000:
                return 160;
        case 24000:
                return 240;
        case 32000:
                return 320;
        case 44100:
                return 441;
        default:
                return -1;
        }
#endif
        return sampele_rate / 100;
}


IHal_Resampler_Handle_t *IHal_ResamlperInit(IHal_ResamplerAttr_t *attr)
{
        resamlper_ctx_t *ctx = (resamlper_ctx_t *)malloc(sizeof(resamlper_ctx_t));
        if (ctx == NULL) {
                IMPP_LOGE(TAG, "init ctx failed");
                return IHAL_RNULL;
        }
        int numPerSampleIn_10ms = get_num_samples_10ms(attr->InSamplerate);
        int numPerSampleOut_10ms = get_num_samples_10ms(attr->OutSamplerate);
        int maxnum10ms = attr->MaxNumPersamples / get_num_samples_10ms(attr->InSamplerate);
        ctx->outbuffer = malloc(maxnum10ms * get_num_samples_10ms(attr->OutSamplerate) * attr->Channels * sizeof(short));
        ctx->processOutbuffer = malloc(numPerSampleOut_10ms * attr->Channels * sizeof(short));
        ctx->processInbuffer = malloc(numPerSampleIn_10ms * attr->Channels * sizeof(short));
        int ret = 0;
        ctx->state = speex_resampler_init(attr->Channels, attr->InSamplerate, attr->OutSamplerate, attr->Quality, &ret);
        if (ctx->state == NULL) {
                IMPP_LOGE(TAG, "init Resamlper failed");
                goto free_buffer;
        }
        speex_resampler_set_rate(ctx->state, attr->InSamplerate, attr->OutSamplerate);
        pthread_mutex_init(&ctx->lock, NULL);
        memcpy(&ctx->attr, attr, sizeof(IHal_ResamplerAttr_t));
        return (IHal_Resampler_Handle_t *)ctx;
free_buffer:
        free(ctx->outbuffer);
        free(ctx->processInbuffer);
        free(ctx->processOutbuffer);
        free(ctx);
        return IHAL_RNULL;
}

IHAL_INT32 IHal_ResamlperSetRate(IHal_Resampler_Handle_t *handle, IHAL_UINT32 in_samplerate, IHAL_UINT32 out_samplerate)
{
        assert(handle);
        resamlper_ctx_t *ctx = (resamlper_ctx_t *)handle;
        if (ctx->state) {
                pthread_mutex_lock(&ctx->lock);
                speex_resampler_set_rate(ctx->state, in_samplerate, out_samplerate);
                free(ctx->processInbuffer);
                free(ctx->processOutbuffer);
                ctx->processInbuffer = malloc(get_num_samples_10ms(in_samplerate) * ctx->attr.Channels * sizeof(short));
                ctx->processOutbuffer = malloc(get_num_samples_10ms(out_samplerate) * ctx->attr.Channels * sizeof(short));
                ctx->attr.InSamplerate = in_samplerate;
                ctx->attr.OutSamplerate = out_samplerate;
                int maxnum10ms = ctx->attr.MaxNumPersamples / get_num_samples_10ms(ctx->attr.InSamplerate);
                ctx->outbuffer = malloc(maxnum10ms * get_num_samples_10ms(ctx->attr.OutSamplerate) * ctx->attr.Channels * sizeof(short));
                pthread_mutex_unlock(&ctx->lock);
        } else {
                IMPP_LOGE(TAG, "Resamlper state error");
                return -IHAL_RERR;
        }
}

IHAL_INT32 IHal_ResamlperGetRate(IHal_Resampler_Handle_t *handle, IHAL_UINT32 *in_samplerate, IHAL_UINT32 *out_samplerate)
{
        assert(handle);
        resamlper_ctx_t *ctx = (resamlper_ctx_t *)handle;
        speex_resampler_get_rate(ctx->state, in_samplerate, out_samplerate);
        return IHAL_ROK;
}

IHAL_INT32 IHal_ResamlperSetQuality(IHal_Resampler_Handle_t *handle, IHAL_UINT32 target_quality)
{
        assert(handle);
        resamlper_ctx_t *ctx = (resamlper_ctx_t *)handle;
        int ret = speex_resampler_set_quality(ctx->state, target_quality);
        if (ret) {
                IMPP_LOGE(TAG, "set Quality error");
                return -IHAL_RERR;
        }
        return IHAL_ROK;
}

IHAL_INT32 IHal_ResamlperProcess(IHal_Resampler_Handle_t *handle, IHAL_INT16 *indata, IHAL_INT16 **outdata, IHAL_INT32 insize, IHAL_INT32 *outsize)
{
        assert(handle);
        resamlper_ctx_t *ctx = (resamlper_ctx_t *)handle;
        int inlen = get_num_samples_10ms(ctx->attr.InSamplerate);
        int outlen = get_num_samples_10ms(ctx->attr.OutSamplerate);
        int t = insize / inlen / sizeof(short) / ctx->attr.Channels;
        int ret = 0;
        int osize = 0;
        //IMPP_LOGD(TAG,"inlen = %d outlen = %d t = %d ##########",inlen,outlen,t);
        int k, j;
        short *p = NULL;
        for (k = 0; k < t; k++) {
                inlen = get_num_samples_10ms(ctx->attr.InSamplerate);
                outlen = get_num_samples_10ms(ctx->attr.OutSamplerate);
                //printf("k = %d \r\n",k);
                p = (short *)(indata + k * inlen * ctx->attr.Channels);
                for (j = 0; j < inlen * ctx->attr.Channels; j++) {
                        ctx->processInbuffer[j] = (short)p[j];
                }

                if (ctx->attr.Channels == 1) {
                        ret = speex_resampler_process_int(ctx->state, 0, (const short *)ctx->processInbuffer, &inlen, ctx->processOutbuffer, &outlen);
                } else {
                        ret = speex_resampler_process_interleaved_int(
                                      ctx->state, (const short *)ctx->processInbuffer,
                                      &inlen, ctx->processOutbuffer, &outlen);
                }

                if (!ret) {
                        p = (short *)(ctx->outbuffer + k * outlen * ctx->attr.Channels);
                        for (j = 0; j < outlen * ctx->attr.Channels; j++) {
                                //ctx->outbuffer[j + k * outlen * ctx->attr.Channels] = ctx->processOutbuffer[j];
                                p[j] = ctx->processOutbuffer[j];
                        }
                        osize += outlen * sizeof(short) * ctx->attr.Channels;
                } else {
                        IMPP_LOGE(TAG, "Resamlper process failed");
                        return -IHAL_RERR;
                }
        }
        *outsize = osize;
        *outdata = ctx->outbuffer;
        return IHAL_ROK;
}

IHAL_INT32 IHal_ResamlperDeInit(IHal_Resampler_Handle_t *handle)
{
        assert(handle);
        resamlper_ctx_t *ctx = (resamlper_ctx_t *)handle;
        pthread_mutex_lock(&ctx->lock);
        free(ctx->outbuffer);
        free(ctx->processInbuffer);
        free(ctx->processOutbuffer);
        speex_resampler_destroy(ctx->state);
        pthread_mutex_unlock(&ctx->lock);

        pthread_mutex_destroy(&ctx->lock);
        free(ctx);
        return IHAL_ROK;

}

