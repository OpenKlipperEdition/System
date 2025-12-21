#include <stdio.h>
#include <math.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sched.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>
#include <semaphore.h>
#include <alsa/asoundlib.h>
#include "audio.h"
#include "log.h"
#include "abuf.h"

#define TAG "AudioPlay"

static unsigned int full_cnt;

typedef struct
{
        unsigned int size;
        void *data; // audio data start
} ao_data_info_t;

typedef struct
{
        char node[64];
        int bytewidth;
        unsigned int samplerate;
        int channels;
        int bufferDeep;
        unsigned int numPerSample;
        unsigned int ao_thread_run;
        unsigned int ao_pause;
        unsigned int ao_pause_done;
        unsigned int ao_restart;
        double current_vol_multiplier;
        double new_vol_multiplier;
        int volume;

        void *ao_buf_head;
        struct timeval base_tv;
        pthread_t ao_thread_tid;
        sem_t sem_buffull;
        sem_t sem_bufempty;
        pthread_mutex_t ao_lock;
        pthread_mutex_t ao_buf_lock;
} prv_ao_ctx_t;

static int _pcm_soft_vol_set(char *pcmdata, unsigned int datasize, int bytewidth, prv_ao_ctx_t *ctx)
{
        unsigned int i = 0;
        int tmp = 0;
        pthread_mutex_lock(&ctx->ao_lock);
        if (ctx->current_vol_multiplier != ctx->new_vol_multiplier)
        {
                ctx->current_vol_multiplier = ctx->new_vol_multiplier;
        }
        pthread_mutex_unlock(&ctx->ao_lock);
        switch (bytewidth)
        {
        case 1:
        {
                char *pcm = pcmdata;
                for (i = 0; i < datasize; i++)
                {
                        tmp = pcm[i] * ctx->current_vol_multiplier;
                        if (tmp > 127)
                                pcm[i] = 127;
                        else if (tmp < -127)
                                pcm[i] = -127;
                        else
                                pcm[i] = (char)tmp;
                }
        }
        break;
        case 2:
        {
                short *pcm = (short *)pcmdata;
                for (i = 0; i < datasize / 2; i++)
                {
                        tmp = pcm[i] * ctx->current_vol_multiplier;
                        if (tmp > 32767)
                                pcm[i] = 32767;
                        else if (tmp < -32767)
                                pcm[i] = -32767;
                        else
                                pcm[i] = (short)tmp;
                }
        }
        break;
        default:
                return -1;
        }
        return 0;
}

static int __imppfmt_to_alsa(int imppfmt)
{
        int ret = 0;
        switch (imppfmt)
        {
        case IMPP_SAMPLE_FMT_S8:
                ret = SND_PCM_FORMAT_S8;
                break;
        case IMPP_SAMPLE_FMT_U8:
                ret = SND_PCM_FORMAT_U8;
                break;
        case IMPP_SAMPLE_FMT_S16:
                ret = SND_PCM_FORMAT_S16_LE;
                break;
        case IMPP_SAMPLE_FMT_U16:
                ret = SND_PCM_FORMAT_U16_LE;
                break;
        case IMPP_SAMPLE_FMT_S24:
                ret = SND_PCM_FORMAT_S24_3LE;
                break;
        case IMPP_SAMPLE_FMT_U24:
                ret = SND_PCM_FORMAT_U24_3LE;
                break;
        case IMPP_SAMPLE_FMT_S32:
                ret = SND_PCM_FORMAT_S32_LE;
                break;
        case IMPP_SAMPLE_FMT_U32:
                ret = SND_PCM_FORMAT_U32_LE;
                break;
        default:
                ret = -1;
        }
        return ret;
}

static int __imppfmt_to_bytewidth(int imppfmt)
{
        int ret = 0;
        switch (imppfmt)
        {
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

static void *__ao_thread(void *arg)
{
        IHal_AudioHandle_t *handle = (IHal_AudioHandle_t *)arg;
        prv_ao_ctx_t *ctx = (prv_ao_ctx_t *)handle->priv;
        void *aonode = NULL;
        ao_data_info_t *ao_bufinfo;
        int ret = 0;
        unsigned int frames = 0;
        for (;;)
        {
                if (ctx->ao_thread_run)
                {
                        if (ctx->ao_pause == 0x01)
                        {
                                if (ctx->ao_pause_done == 0)
                                {
                                        /* snd_pcm_drop(handle->pcm_handle); */
                                        snd_pcm_drain(handle->pcm_handle);
                                        pthread_mutex_lock(&ctx->ao_lock);
                                        ctx->ao_pause_done = 1;
                                        pthread_mutex_unlock(&ctx->ao_lock);
                                }
                                else
                                {
                                        usleep(10000);
                                        continue;
                                }
                        }

                        if (ctx->ao_restart == 0x01)
                        {
                                snd_pcm_drop(handle->pcm_handle);
                                pthread_mutex_lock(&ctx->ao_buf_lock);
                                audio_buf_clear(ctx->ao_buf_head);
                                ctx->ao_restart = 0x0;
                                pthread_mutex_unlock(&ctx->ao_buf_lock);
                                for (;;)
                                {
                                        ret = sem_trywait(&ctx->sem_buffull);
                                        if (!ret)
                                        {
                                                sem_post(&ctx->sem_bufempty);
                                        }
                                        else
                                        {
                                                break;
                                        }
                                }
                        }

                        sem_wait(&ctx->sem_buffull);
                        pthread_mutex_lock(&ctx->ao_buf_lock);
                        aonode = audio_buf_get_node(ctx->ao_buf_head, AUDIO_BUF_FULL);
                        pthread_mutex_unlock(&ctx->ao_buf_lock);
                        if (!aonode)
                        {
                                IMPP_LOGE(TAG, "play_thread get node failed");
                                return (void *)-1;
                        }
                        pthread_mutex_lock(&ctx->ao_buf_lock);
                        ao_bufinfo = (ao_data_info_t *)audio_buf_node_get_info(aonode);
                        pthread_mutex_unlock(&ctx->ao_buf_lock);
                        frames = ao_bufinfo->size / ctx->bytewidth / ctx->channels;
                        _pcm_soft_vol_set(ao_bufinfo->data, ao_bufinfo->size, ctx->bytewidth, ctx);
                        ret = snd_pcm_writei(handle->pcm_handle, ao_bufinfo->data, frames);
                        if (ret < 0)
                        {
                                // snd_pcm_prepare(handle->pcm_handle);
                                snd_pcm_recover(handle->pcm_handle, ret, 0);
                                ret = snd_pcm_writei(handle->pcm_handle, ao_bufinfo->data, frames);
                                IMPP_LOGW(TAG, "snd pcm EPIPE happend");
                        }
                        pthread_mutex_lock(&ctx->ao_buf_lock);
                        audio_buf_put_node(ctx->ao_buf_head, aonode, AUDIO_BUF_EMPTY);
                        pthread_mutex_unlock(&ctx->ao_buf_lock);
                        sem_post(&ctx->sem_bufempty);
                }
                else
                {
                        usleep(5000);
                }
        }
}

IHal_AudioHandle_t *IHal_AO_ChanCreate(IHal_AO_Attr_t *attr)
{
        snd_pcm_t *handle;
        snd_pcm_hw_params_t *hw_params;
        snd_pcm_sw_params_t *sw_params;
        IHal_AudioHandle_t *pAudio = NULL;
        int ret = 0;
        snd_pcm_uframes_t period_size = 0;
        snd_pcm_uframes_t buffer_size = 0;
        snd_pcm_uframes_t start_threshold, stop_threshold;

        ret = snd_pcm_open(&handle, attr->audio_node, SND_PCM_STREAM_PLAYBACK, SND_PCM_ASYNC);
        if (ret)
        {
                IMPP_LOGE(TAG, "open audio playback  node %s failed", attr->audio_node);
                return IHAL_RNULL;
        }

        snd_pcm_hw_params_alloca(&hw_params);
        ret = snd_pcm_hw_params_any(handle, hw_params);
        if (ret)
        {
                IMPP_LOGE(TAG, "set default hw param failed");
                snd_pcm_close(handle);
                return IHAL_RNULL;
        }

        ret = snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
        if (ret)
        {
                IMPP_LOGE(TAG, "set default access-mode SND_PCM_ACCESS_RW_INTERLEAVED failed");
                snd_pcm_close(handle);
                return IHAL_RNULL;
        }

        int fmt = __imppfmt_to_alsa(attr->SampleFmt);
        if (fmt < 0)
        {
                IMPP_LOGE(TAG, "impp-fmt-to-alsa failed,not support fmt type");
                snd_pcm_close(handle);
                return IHAL_RNULL;
        }

        ret = snd_pcm_hw_params_set_format(handle, hw_params, fmt);
        if (ret)
        {
                IMPP_LOGE(TAG, "set format failed");
                snd_pcm_close(handle);
                return IHAL_RNULL;
        }

        ret = snd_pcm_hw_params_set_channels(handle, hw_params, attr->channels);
        if (ret)
        {
                IMPP_LOGE(TAG, "set channels failed");
                snd_pcm_close(handle);
                return IHAL_RNULL;
        }

        unsigned int rate = attr->SampleRate;
        ret = snd_pcm_hw_params_set_rate_near(handle, hw_params, &rate, NULL);
        if (ret)
        {
                IMPP_LOGE(TAG, "set sample rate failed");
                snd_pcm_close(handle);
                return IHAL_RNULL;
        }

        period_size = attr->numPerSample;
        ret = snd_pcm_hw_params_set_period_size_near(handle, hw_params,
                                                     &period_size, 0);
        if (ret)
        {
                IMPP_LOGE(TAG, "set period size failed");
                snd_pcm_close(handle);
                return IHAL_RNULL;
        }

        unsigned buffer_time = 0;
        ret = snd_pcm_hw_params_get_buffer_time_max(hw_params, &buffer_time, 0);
        if (ret)
        {
                IMPP_LOGE(TAG, "get buffer time max failed");
                snd_pcm_close(handle);
                return IHAL_RNULL;
        }
        if (buffer_time > 500000)
                buffer_time = 500000;
        ret = snd_pcm_hw_params_set_buffer_time_near(handle, hw_params, &buffer_time, 0);
        if (ret)
        {
                IMPP_LOGE(TAG, "set buffer time failed");
                snd_pcm_close(handle);
                return IHAL_RNULL;
        }

        snd_pcm_hw_params_get_buffer_size(hw_params, &buffer_size);
        IMPP_LOGW(TAG, "period_size = %d, buffer_size = %d", period_size, buffer_size);

        ret = snd_pcm_hw_params(handle, hw_params);
        if (ret)
        {
                IMPP_LOGE(TAG, "set last hw-params failed");
                snd_pcm_close(handle);
                return IHAL_RNULL;
        }

        snd_pcm_sw_params_alloca(&sw_params);
        ret = snd_pcm_sw_params_current(handle, sw_params);
        if (ret)
        {
                snd_pcm_close(handle);
                IMPP_LOGE(TAG, "get current sw params failed");
                return IHAL_RNULL;
        }

        ret = snd_pcm_sw_params_set_avail_min(handle, sw_params, period_size);
        if (ret)
        {
                snd_pcm_close(handle);
                IMPP_LOGE(TAG, "set avail min failed");
                return IHAL_RNULL;
        }

        start_threshold = buffer_size;
        stop_threshold = buffer_size;
        ret = snd_pcm_sw_params_set_start_threshold(handle, sw_params, start_threshold);
        if (ret)
        {
                snd_pcm_close(handle);
                IMPP_LOGE(TAG, "set start threshold failed");
                return IHAL_RNULL;
        }
        ret = snd_pcm_sw_params_set_stop_threshold(handle, sw_params, stop_threshold);
        if (ret)
        {
                snd_pcm_close(handle);
                IMPP_LOGE(TAG, "set stop threshold failed");
                return IHAL_RNULL;
        }
        IMPP_LOGI(TAG, "start_threshold = %d, stop_threshold = %d", start_threshold, stop_threshold);

        ret = snd_pcm_sw_params(handle, sw_params);
        if (ret)
        {
                snd_pcm_close(handle);
                IMPP_LOGE(TAG, "set sw params failed");
                return IHAL_RNULL;
        }

        pAudio = (IHal_AudioHandle_t *)calloc(1, sizeof(IHal_AudioHandle_t));
        if (!pAudio)
        {
                IMPP_LOGE(TAG, "malloc audio-handle failed");
                snd_pcm_close(handle);
                return IHAL_RNULL;
        }

        prv_ao_ctx_t *pctx = NULL;
        pctx = (prv_ao_ctx_t *)calloc(1, sizeof(prv_ao_ctx_t));
        if (!pctx)
        {
                IMPP_LOGE(TAG, "malloc prv_ao_ctx_t failed");
                snd_pcm_close(handle);
                free(pAudio);
                return IHAL_RNULL;
        }
        pAudio->pcm_handle = (void *)handle;
        pAudio->priv = (void *)pctx;

        memcpy(pctx->node, attr->audio_node, strlen(attr->audio_node));
        pctx->bytewidth = __imppfmt_to_bytewidth(attr->SampleFmt);
        pctx->samplerate = rate;
        pctx->channels = attr->channels;
        pctx->bufferDeep = attr->bufferDeep;
        pctx->numPerSample = attr->numPerSample;

        pthread_mutex_init(&pctx->ao_lock, NULL);
        pthread_mutex_init(&pctx->ao_buf_lock, NULL);
        sem_init(&pctx->sem_buffull, 0, 0);
        sem_init(&pctx->sem_bufempty, 0, pctx->bufferDeep);
        /** alloc audio buf */
        void *ao_buf = audio_buf_alloc(pctx->bufferDeep, pctx->bytewidth * pctx->channels * pctx->numPerSample, sizeof(ao_data_info_t));
        if (!ao_buf)
        {
                IMPP_LOGE(TAG, "alloc audio buf failed");
                snd_pcm_close(handle);
                free(pAudio);
                free(pctx);
                return IHAL_RNULL;
        }
        pctx->ao_buf_head = ao_buf;
        pctx->current_vol_multiplier = 1.0;
        pctx->new_vol_multiplier = 1.0;
        pctx->volume = 60;
        full_cnt = 0;
        return pAudio;
}

IHAL_INT32 IHal_AO_ChanDestroy(IHal_AudioHandle_t *handle)
{
        int ret = 0;
        assert(handle);
        prv_ao_ctx_t *ctx = handle->priv;
        snd_pcm_t *pcm = (snd_pcm_t *)handle->pcm_handle;
        pthread_mutex_lock(&ctx->ao_lock);
        ctx->ao_thread_run = 0x00;
        pthread_mutex_unlock(&ctx->ao_lock);
        if (ctx->ao_thread_tid)
        {
                pthread_cancel(ctx->ao_thread_tid);
                pthread_join(ctx->ao_thread_tid, NULL);
        }

        ret = snd_pcm_close(pcm);
        if (ret)
        {
                IMPP_LOGE(TAG, "close pcm device failed");
                return -IHAL_RERR;
        }
        pthread_mutex_destroy(&ctx->ao_lock);
        pthread_mutex_destroy(&ctx->ao_buf_lock);
        audio_buf_free(ctx->ao_buf_head);
        free(ctx);
        free(handle);
        return IHAL_ROK;
}

IHAL_INT32 IHal_AO_ChanWriteData(IHal_AudioHandle_t *handle, IHal_AudioFrm_t *frame, IHAL_INT32 waitblock)
{
        int ret = 0;
        assert(handle);
        prv_ao_ctx_t *ctx = handle->priv;
        void *aonode;
        if (ctx->ao_pause == 0x01)
        {
                return -IHAL_RFAILED;
        }
        if (waitblock == IMPP_WAIT_FOREVER)
        {
                ret = sem_wait(&ctx->sem_bufempty);
        }
        else
        {
                ret = sem_trywait(&ctx->sem_bufempty);
        }
        if (ret)
        {
                return -IHAL_RERR;
        }
        pthread_mutex_lock(&ctx->ao_buf_lock);
        aonode = audio_buf_get_node(ctx->ao_buf_head, AUDIO_BUF_EMPTY);
        if (aonode)
        {
                void *aodata = audio_buf_node_get_data(aonode);
                ao_data_info_t *bufinfo = audio_buf_node_get_info(aonode);
                bufinfo->size = frame->datalen;
                bufinfo->data = aodata;
                memcpy((void *)bufinfo->data, frame->vaddr, frame->datalen);
                audio_buf_put_node(ctx->ao_buf_head, aonode, AUDIO_BUF_FULL);
        }
        else
        {
                pthread_mutex_unlock(&ctx->ao_buf_lock);
                IMPP_LOGE(TAG, "AO write data error");
                return -IHAL_RERR;
        }
        pthread_mutex_unlock(&ctx->ao_buf_lock);
        sem_post(&ctx->sem_buffull);
        return IHAL_ROK;
}

IHAL_INT32 IHal_AO_ChanStart(IHal_AudioHandle_t *handle)
{
        int ret = 0;
        assert(handle);
        prv_ao_ctx_t *ctx = handle->priv;
        ret = snd_pcm_prepare(handle->pcm_handle); // alsa pcm start
        if (ret)
        {
                IMPP_LOGE(TAG, "ao start failed");
                return -IHAL_RFAILED;
        }

        ret = pthread_create(&ctx->ao_thread_tid, NULL, __ao_thread, (void *)handle);
        if (ret)
        {
                IMPP_LOGE(TAG, "audio input pthread create failed");
                return -IHAL_RFAILED;
        }
        gettimeofday(&ctx->base_tv, NULL);
        pthread_mutex_lock(&ctx->ao_lock);
        ctx->ao_thread_run = 0x01;
        pthread_mutex_unlock(&ctx->ao_lock);

        return IHAL_ROK;
}

IHAL_INT32 IHal_AO_ChanStop(IHal_AudioHandle_t *handle)
{
        assert(handle);
        prv_ao_ctx_t *ctx = handle->priv;
        int ret = 0;
#if 0
        ret = snd_pcm_drop(handle->pcm_handle);
        if (ret) {
                IMPP_LOGE(TAG, "snd pcm stop failed");
                return -IHAL_RFAILED;
        }
#endif
        pthread_mutex_lock(&ctx->ao_lock);
        ctx->ao_thread_run = 0x00;
        pthread_mutex_unlock(&ctx->ao_lock);

        return IHAL_ROK;
}

IHAL_INT32 IHal_AO_ChanReStart(IHal_AudioHandle_t *handle)
{
        assert(handle);
        prv_ao_ctx_t *ctx = handle->priv;
        int ret = 0;
        int retimes = 0;
        pthread_mutex_lock(&ctx->ao_lock);
        ctx->ao_restart = 0x01;
        pthread_mutex_unlock(&ctx->ao_lock);
        do
        {
                usleep(50 * 1000);
                if (retimes++ > 30)
                {
                        pthread_mutex_lock(&ctx->ao_lock);
                        ctx->ao_restart = 0x00;
                        pthread_mutex_unlock(&ctx->ao_lock);
                        IMPP_LOGE(TAG, "restart time out !!!");
                        return -IHAL_RERR;
                }
        } while (ctx->ao_restart == 1);

        snd_pcm_prepare(handle->pcm_handle);
        return IHAL_ROK;
}

IHAL_INT32 IHal_AO_BufferFlush(IHal_AudioHandle_t *handle)
{
        assert(handle);
        prv_ao_ctx_t *ctx = handle->priv;
        int full_num = audio_buf_get_num(ctx->ao_buf_head, AUDIO_BUF_FULL);
        while (full_num > 0)
        {
                usleep(5000);
                full_num = audio_buf_get_num(ctx->ao_buf_head, AUDIO_BUF_FULL);
        }
        snd_pcm_drain(handle->pcm_handle);
        snd_pcm_prepare(handle->pcm_handle);
        return IHAL_ROK;
}

IHAL_INT32 IHal_AO_Pause(IHal_AudioHandle_t *handle)
{
        // TODO
        assert(handle);
        int retimes = 0;
        prv_ao_ctx_t *ctx = handle->priv;
        int ret = 0;
        pthread_mutex_lock(&ctx->ao_lock);
        ctx->ao_pause_done = 0;
        ctx->ao_pause = 0x01;
        pthread_mutex_unlock(&ctx->ao_lock);
        do
        {
                usleep(50 * 1000);
                if (retimes++ > 30)
                {
                        pthread_mutex_lock(&ctx->ao_lock);
                        ctx->ao_pause_done = 0;
                        ctx->ao_pause = 0x00;
                        pthread_mutex_unlock(&ctx->ao_lock);
                        IMPP_LOGE(TAG, "set pause time out !!!");
                        return -IHAL_RERR;
                }
        } while ((ctx->ao_pause == 1) && (1 != ctx->ao_pause_done));
        return IHAL_ROK;
}

IHAL_INT32 IHal_AO_Resume(IHal_AudioHandle_t *handle)
{
        // TODO
        assert(handle);
        prv_ao_ctx_t *ctx = handle->priv;
        int ret = 0;
        pthread_mutex_lock(&ctx->ao_lock);
        ctx->ao_pause = 0x00;
        snd_pcm_prepare(handle->pcm_handle);
        pthread_mutex_unlock(&ctx->ao_lock);

        return IHAL_ROK;
}

IHAL_INT32 IHal_AO_SetMute(IHal_AudioHandle_t *handle, IHAL_INT32 mute)
{
        // TODO
        return IHAL_ROK;
}

IHAL_INT32 IHal_AO_GetMuteStatus(IHal_AudioHandle_t *handle, IHAL_INT32 *mute)
{
        // TODO
        return IHAL_ROK;
}

// range [-30,120]
IHAL_INT32 IHal_AO_SetVolume(IHal_AudioHandle_t *handle, IHAL_INT32 value)
{
        assert(handle);
        prv_ao_ctx_t *ctx = handle->priv;
        if (value > 100 || value < 0)
        {
                IMPP_LOGE(TAG, "volume set err range[0,100] ,step 1");
                return IHAL_RERR;
        }
        double vol = -30 + 1.5 * value;
        double multiplier = 0.0;
        if (vol == -30)
                multiplier = 0.0;
        else
                multiplier = pow(10, ((((double)vol) * 0.5) - 30.0) / 20.0);
        pthread_mutex_lock(&ctx->ao_lock);
        ctx->volume = value;
        ctx->new_vol_multiplier = multiplier;
        pthread_mutex_unlock(&ctx->ao_lock);

        return IHAL_ROK;
}

IHAL_INT32 IHal_AO_GetVolume(IHal_AudioHandle_t *handle, IHAL_INT32 *value)
{
        assert(handle);
        prv_ao_ctx_t *ctx = handle->priv;
        pthread_mutex_lock(&ctx->ao_lock);
        *value = ctx->volume;
        pthread_mutex_unlock(&ctx->ao_lock);

        return IHAL_ROK;
}

IHAL_INT32 IHal_AO_SetGain(IHal_AudioHandle_t *handle, IHAL_INT32 value)
{
        assert(handle);
        int ret = 0;
        snd_mixer_t *mixer_handle;
        snd_mixer_elem_t *elem;
        prv_ao_ctx_t *ctx = handle->priv;

        ret = snd_mixer_open(&mixer_handle, 0);
        if (ret)
        {
                IMPP_LOGE(TAG, "open mixer failed");
                snd_mixer_close(mixer_handle);
                return -IHAL_RFAILED;
        }

        ret = snd_mixer_attach(mixer_handle, "default");
        if (ret)
        {
                IMPP_LOGE(TAG, "mixer attach failed");
                snd_mixer_close(mixer_handle);
                return -IHAL_RFAILED;
        }

        ret = snd_mixer_selem_register(mixer_handle, NULL, NULL);
        if (ret < 0)
        {
                IMPP_LOGE(TAG, "mixer selem register failed");
                snd_mixer_close(mixer_handle);
                return -IHAL_RFAILED;
        }

        ret = snd_mixer_load(mixer_handle);
        if (ret < 0)
        {
                IMPP_LOGE(TAG, "mixer load failed");
                snd_mixer_close(mixer_handle);
                return -IHAL_RFAILED;
        }
        elem = snd_mixer_first_elem(mixer_handle);

        int found = 0;
        long playback_volume_min = 0;
        long playback_volume_max = 0;
        while (elem)
        {
                // printf("snd_mixer_selem_get_name(elem): %s\n", snd_mixer_selem_get_name(elem));
                if (snd_mixer_selem_is_active(elem))
                {
                        if (snd_mixer_selem_has_common_volume(elem))
                        {
                                // IMPP_LOGW(TAG, "alsa drviers should provide Capture Volume and Playback Volume kcontrols.");
                                /*驱动必须提供 Capture Volume 和 Playback Volume 来调节音量.*/
                        }
                        else
                        {
                                if (snd_mixer_selem_has_playback_volume(elem) && !strcmp(snd_mixer_selem_get_name(elem), "Speaker"))
                                {
                                        ret = snd_mixer_selem_get_playback_volume_range(elem, &playback_volume_min, &playback_volume_max);
                                        if (ret) {
                                            IMPP_LOGE(TAG, "Failed to get playback volume range.");
                                            snd_mixer_close(mixer_handle);
                                            return -IHAL_RERR;
                                        }
                                        IMPP_LOGI(TAG, "The playback volume range is [%d, %d].", playback_volume_min, playback_volume_max);

                                        if (playback_volume_min > value || value > playback_volume_max) {
                                            IMPP_LOGE(TAG, "The playback volume range is [%d, %d], but you have set it to %d.", \
                                                            playback_volume_min, playback_volume_max, value);
                                            snd_mixer_close(mixer_handle);
                                            return -IHAL_RERR;
                                        }

                                        playback_volume_min = 0;
                                        playback_volume_max = 0;
                                        found = 1;
                                        break;
                                }
                        }
                }
                elem = snd_mixer_elem_next(elem);
        }

        if (!found)
        {
                IMPP_LOGE(TAG, "Failed to found Playback Volume kcontrols!\n");
                snd_mixer_close(mixer_handle);
                return -IHAL_RERR;
        }

        int volume = value;
        ret = snd_mixer_selem_set_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, volume);
        ret = snd_mixer_selem_set_playback_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT, volume);
        if (ret)
        {
                IMPP_LOGE(TAG, "set playback volume failed");
                snd_mixer_close(mixer_handle);
                return -IHAL_RFAILED;
        }
        snd_mixer_close(mixer_handle);
        return IHAL_ROK;
}

IHAL_INT32 IHal_AO_GetGain(IHal_AudioHandle_t *handle, IHAL_INT32 *value)
{
        assert(handle);
        int ret = 0;
        snd_mixer_t *mixer_handle;
        snd_mixer_elem_t *elem;
        prv_ao_ctx_t *ctx = handle->priv;
        long right = 0, left = 0;

        ret = snd_mixer_open(&mixer_handle, 0);
        if (ret)
        {
                IMPP_LOGE(TAG, "open mixer failed");
                snd_mixer_close(mixer_handle);
                return -IHAL_RFAILED;
        }

        ret = snd_mixer_attach(mixer_handle, "default");
        if (ret)
        {
                IMPP_LOGE(TAG, "mixer attach failed");
                snd_mixer_close(mixer_handle);
                return -IHAL_RFAILED;
        }

        ret = snd_mixer_selem_register(mixer_handle, NULL, NULL);
        if (ret < 0)
        {
                IMPP_LOGE(TAG, "mixer selem register failed");
                snd_mixer_close(mixer_handle);
                return -IHAL_RFAILED;
        }

        ret = snd_mixer_load(mixer_handle);
        if (ret < 0)
        {
                IMPP_LOGE(TAG, "mixer load failed");
                snd_mixer_close(mixer_handle);
                return -IHAL_RFAILED;
        }

        elem = snd_mixer_first_elem(mixer_handle);

        int found = 0;
        while (elem)
        {
                if (snd_mixer_selem_is_active(elem))
                {
                      if (snd_mixer_selem_has_common_volume(elem)) {
                        // IMPP_LOGW(TAG, "alsa drviers should provide Capture Volume and Playback Volume kcontrols.");
                      } else {
                        if (snd_mixer_selem_has_playback_volume(elem) && !strcmp(snd_mixer_selem_get_name(elem), "Speaker")) {
                            found = 1;
                            break;
                        }
                      }
                }
                elem = snd_mixer_elem_next(elem);
        }

        if (!found) {
            IMPP_LOGE(TAG, "Failed to found Playback Volume kcontrols!\n");
            snd_mixer_close(mixer_handle);
            return -IHAL_RERR;
        }

        ret = snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &left);
        ret = snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT, &right);
        if (ret || (left != right))
        {
                IMPP_LOGE(TAG, "get playback volume failed");
                snd_mixer_close(mixer_handle);
                return -IHAL_RFAILED;
        }

        *value = left;
        snd_mixer_close(mixer_handle);
        return IHAL_ROK;
}
