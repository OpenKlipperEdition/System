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
#include <alsa/asoundlib.h>
#include "audio.h"
#include "log.h"
#include "abuf.h"

#ifdef AEC_SUPPORT
#include "audio_aec.h"
#include "audio_agc.h"
#include "audio_ns.h"
#include "audio_hpf.h"
#endif
/* #include "echo_cancellation.h" */
#define TAG "AudioCap"

/* #define SAVE_DEBUG_FILE */
#define ALSA_HCTL_NAME "default"
#define ALSA_HCTL_MONO_ELEM_NAME "name='Left/Right channel select'"
#define AEC_DELAY_MS 135

typedef struct
{
        void *node;
        unsigned int timestamp;
        unsigned int size;
        unsigned int seq;
        void *data; // audio data start
} ai_data_info_t;

typedef struct
{
        char node[64];
        int bytewidth;
        unsigned int samplerate;
        int channels;
        int bufferDeep;
        unsigned int numPerSample;
        IHal_AI_Attr_t attr;
        unsigned int ai_thread_run;
        unsigned short *aec_outbuf;
        void *ref_data;
        void *ai_buf_head;
        void *ref_pcm_handle;
        void *aec_handle;
        void *agc_handle;
        short ai_hpf_coefficient[5];
#ifdef AEC_SUPPORT
        FilterState hpf_handle;
        NsHandle_t ns_handle;
#endif
        int agc_sample_i;
        int ns_cell;
        unsigned int aec_enable;
        unsigned int agc_enable;
        unsigned int ns_enable;
        unsigned int hpf_enable;
        struct timeval base_tv;
        pthread_t ai_thread_tid;
        pthread_t tloop_thread_tid;
        pthread_cond_t aidata_ready_cond;
        pthread_mutex_t ai_lock;
        pthread_mutex_t ai_buf_lock;
        pthread_mutex_t aec_lock;
        pthread_mutex_t agc_lock;
        pthread_mutex_t ns_lock;
        pthread_mutex_t hpf_lock;
} prv_ai_ctx_t;

const int16_t kFilterCoefficients8kHz[5] =
    {3798, -7596, 3798, 7807, -3733};
/* {3875, -7750, 3875, 7737, -3665}; /100
 *                172         * {3768, -7536, 3768, 7511, -3467};//150*/
/* {3665, -7330, 3665, 7285, -3280};//200*/
/* {3565, -7130, 3565, 7061, -3103};//250 */
/* {3279, -6558, 3279, 6394, -2627};//400*/
/* {3101, -6202, 3101, 5957, -2351};//500 */
/* {2331, -4662, 2331, 3862, -1365};//1000 */
const int16_t kFilterCoefficients[5] =
    {3665, -7330, 3665, 7285, -3280}; // 400
/* {3101, -6202, 3101, 5956, -2351};//1000 */
/* {2617, -5234, 2617, 2341, -846}; */
/* {4012, -8024, 4012, 8002, -3913}; */

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

static void __wait_audio_data(IHal_AudioHandle_t *handle)
{
        assert(handle);
        prv_ai_ctx_t *ctx = handle->priv;
        unsigned int timeout_ms = 800;
        struct timespec abstime;
        struct timeval now;
        gettimeofday(&now, NULL);
        long ns = now.tv_usec * 1000 + (timeout_ms % 1000) * 1000000;
        abstime.tv_sec = now.tv_sec + ns / 1000000000 + timeout_ms / 1000;
        abstime.tv_nsec = ns % 1000000000;
        pthread_mutex_lock(&ctx->ai_lock);
        if (pthread_cond_timedwait(&ctx->aidata_ready_cond, &ctx->ai_lock, &abstime) == ETIMEDOUT)
        {
                IMPP_LOGW(TAG, "wait ai data ready time out !!!");
        }
        pthread_mutex_unlock(&ctx->ai_lock);
}

uint64_t timeval_diff(struct timeval *tv0, struct timeval *tv1)
{
        uint64_t time0, time1;
        time0 = tv0->tv_sec * 1000000 + tv0->tv_usec;
        time1 = tv1->tv_sec * 1000000 + tv1->tv_usec;
        return (time1 > time0) ? (time1 - time0) : (time0 - time1);
}

static int _set_ctl_elem_value(char *name, int value)
{
        snd_hctl_t *pt_hctl = NULL;
        snd_hctl_elem_t *pt_helem = NULL;
        snd_ctl_elem_id_t *pt_id;
        snd_ctl_elem_value_t *pt_value;
        snd_ctl_elem_value_alloca(&pt_value); // not need free ,because alloc at stack
        snd_ctl_elem_id_alloca(&pt_id);
        if (snd_hctl_open(&pt_hctl, ALSA_HCTL_NAME, 0) < 0)
        {
                IMPP_LOGE(TAG, "snd_hctl_open fail.\n");
                return -1;
        }

        // 2. load hctl
        if (snd_hctl_load(pt_hctl) < 0)
        {
                IMPP_LOGE(TAG, "snd_hctl_load fail.\n");
                goto hctl_fail;
        }

        // 3. parse ctl elem
        if (snd_ctl_ascii_elem_id_parse(pt_id, name) < 0)
        {
                IMPP_LOGE(TAG, "snd_ctl_ascii_elem_id_parse fail.\n");
                goto hctl_fail;
        }

        // 4. find hctl elem
        pt_helem = snd_hctl_find_elem(pt_hctl, pt_id);
        if (!pt_helem)
        {
                IMPP_LOGE(TAG, "snd_hctl_find_elem fail.\n");
                goto hctl_fail;
        }

        if (snd_hctl_elem_read(pt_helem, pt_value) < 0)
        {
                IMPP_LOGE(TAG, "snd_hctl_elem_read fail.\n");
                goto set_value_end;
        }
        // long volume=snd_ctl_elem_value_get_integer(pt_value, 0);
        // set elem value
        snd_ctl_elem_value_set_integer(pt_value, 0, value);
        // 6 write hctl elem
        if (snd_hctl_elem_write(pt_helem, pt_value))
        {
                IMPP_LOGE(TAG, "snd_hctl_elem_write fail.\n");
                goto set_value_end;
        }
        snd_hctl_close(pt_hctl);
        return 0;
set_value_end:
hctl_fail:
        snd_hctl_close(pt_hctl);
        return -1;
}

#ifdef AEC_SUPPORT
#define PI 3.1415926
#define sqrt_2 1.4142
void hpf_gen_filter_coefficients(short *coef, int samplerate, int p_rate)
{
        float cutoff = p_rate;
        float sample_rate = samplerate;
        float omiga_s = tan(cutoff / sample_rate * PI);
        float a = omiga_s * omiga_s + sqrt_2 * omiga_s + 1;
        float b = 2 * omiga_s * omiga_s - 2;
        float c = omiga_s * omiga_s - sqrt_2 * omiga_s + 1;
        b = -b / a;
        c = -c / a;
        float d = 1 / a;
        coef[0] = d * 4096;
        coef[1] = -coef[0] * 2;
        coef[2] = coef[0];
        coef[3] = b * 4096;
        coef[4] = c * 4096;
}

static int _ai_agc_process(prv_ai_ctx_t *ctx, void *indata, void *outdata, int datasize)
{
        int j = datasize / (ctx->agc_sample_i * 2);
        short *p = indata;
        short *q = outdata;
        short *ptmp = NULL;
        short *qtmp = NULL;
        unsigned int outMicLevel;
        unsigned char saturationWarning;
        for (int i = 0; i < j; i++)
        {
                ptmp = p + ctx->agc_sample_i * i;
                qtmp = p + ctx->agc_sample_i * i;
                audio_process_agc_process(ctx->agc_handle,
                                          (const short *const *)&ptmp, 1,
                                          ctx->agc_sample_i, (short *const *)&qtmp,
                                          127, &outMicLevel, 0, &saturationWarning);
        }

        return 0;
}

static int _ai_ns_process(prv_ai_ctx_t *ctx, short *indata, short *outdata, int datasize)
{
        int i, j, k;
        float *sp = ctx->ns_handle.sp;
        float *out = ctx->ns_handle.out;
        k = datasize / (ctx->ns_handle.cell_num * 2);
        for (i = 0; i < k; i++)
        {
                for (j = 0; j < ctx->ns_handle.cell_num; j++)
                {
                        sp[j] = indata[j + i * ctx->ns_handle.cell_num];
                }
                memset(ctx->ns_handle.out, 0, sizeof(float) * ctx->ns_handle.cell_num);
                audio_process_ns_process(ctx->ns_handle.handle, (const float *const *)&sp, 1, (float *const *)&out);
                for (j = 0; j < ctx->ns_handle.cell_num; j++)
                {
                        outdata[j + i * ctx->ns_handle.cell_num] = out[j];
                }
        }

        return 0;
}

static int _ai_hpf_process(prv_ai_ctx_t *ctx, char *data, int datasize)
{
        int k = datasize / 160;
        int samples = datasize / 2 / k;
        char *pdata = data;
        int ret = 0;
        for (int i = 0; i < k; i++)
        {
                ret = audio_process_hpf_process(&ctx->hpf_handle, (short *)(pdata + i * 160), samples);
                if (ret)
                {
                        IMPP_LOGE(TAG, "audio hpf process failed");
                        return -1;
                }
        }
        return 0;
}
#endif

static void *__ai_thread(void *arg)
{
        IHal_AudioHandle_t *handle = (IHal_AudioHandle_t *)arg;
        prv_ai_ctx_t *ctx = (prv_ai_ctx_t *)handle->priv;
        void *ainode = NULL;
        void *ref_ainode = NULL;
        short *ai_data;
        ai_data_info_t *ai_buf_info;
        ai_data_info_t *ref_buf_info;
        int ret = 0;
        struct timeval cur_tv;
        unsigned long long cnt = 0;
        unsigned int datasize = 0;
        unsigned int ref_datasize = 0;
        short *aec_data = NULL;
        int tloop_ret = 0;
#ifdef SAVE_DEBUG_FILE
        int fd = open("loopsave.pcm", O_RDWR | O_CREAT | O_TRUNC, 0666);
        int fd2 = open("inputsave.pcm", O_RDWR | O_CREAT | O_TRUNC, 0666);
#endif
        int refnum = 0;
        int process_start = 0;
        for (;;)
        {
                if (ctx->ai_thread_run)
                {
                        pthread_mutex_lock(&ctx->ai_buf_lock);
                        ainode = audio_buf_get_node(ctx->ai_buf_head, AUDIO_BUF_EMPTY);
                        pthread_mutex_unlock(&ctx->ai_buf_lock);
                        if (!ainode)
                        {
                                // node all fulled,then get one full node and flush old data
                                pthread_mutex_lock(&ctx->ai_buf_lock);
                                ainode = audio_buf_get_node(ctx->ai_buf_head, AUDIO_BUF_FULL);
                                pthread_mutex_unlock(&ctx->ai_buf_lock);
                                if (!ainode)
                                {
                                        IMPP_LOGE(TAG, "read_thread get node failed");
                                        return (void *)-1;
                                }
                        }
                        pthread_mutex_lock(&ctx->ai_buf_lock);
                        ai_data = (short *)audio_buf_node_get_data(ainode);
                        ai_buf_info = (ai_data_info_t *)audio_buf_node_get_info(ainode);
                        ai_buf_info->data = ai_data;
                        pthread_mutex_unlock(&ctx->ai_buf_lock);

                        ret = snd_pcm_readi(handle->pcm_handle, ai_data, ctx->numPerSample);
                        if (ret <= 0)
                        {
                                /* perror("snd_pcm_readi :"); */
                                IMPP_LOGE(TAG, "overrun happend !!!");
                                snd_pcm_recover(handle->pcm_handle, ret, 0);
                                pthread_mutex_lock(&ctx->ai_buf_lock);
                                audio_buf_put_node(ctx->ai_buf_head, ainode, AUDIO_BUF_EMPTY); // error
                                pthread_mutex_unlock(&ctx->ai_buf_lock);
                                continue;
                                /* return (void*)-2; */
                        }
#ifdef AEC_SUPPORT
                        if (1 == ctx->aec_enable)
                        {
                                tloop_ret = snd_pcm_readi(ctx->ref_pcm_handle, ctx->ref_data, ctx->numPerSample);
                        }
#endif
                        datasize = ret * ctx->bytewidth * ctx->channels; // byte
#ifdef SAVE_DEBUG_FILE
                        write(fd2, ai_data, datasize);
#endif
#ifdef AEC_SUPPORT
                        if ((ctx->aec_enable == 1) && (ctx->ref_pcm_handle != NULL) && (tloop_ret) && (ctx->channels == 1))
                        {
                                pthread_mutex_lock(&ctx->aec_lock);
                                ref_datasize = tloop_ret * ctx->bytewidth;
                                aec_data = ctx->aec_outbuf;
#ifdef SAVE_DEBUG_FILE
                                write(fd, ctx->ref_data, ref_datasize);
#endif
                                if (ref_datasize > 0)
                                {
                                        // do aec process
                                        struct aec_process_param p = {ctx->ref_data, ai_data, ai_data, ref_datasize};
                                        audio_process_aec_process(ctx->aec_handle, &p, AEC_DELAY_MS);
                                        // memcpy(ai_data,ctx->tloop_data,datasize);
                                        // printf("aec process ok \r\n");
                                }
                                pthread_mutex_unlock(&ctx->aec_lock);
                        }
                        if (ctx->ns_enable && ctx->ns_handle.handle)
                        {
                                pthread_mutex_lock(&ctx->ns_lock);
                                _ai_ns_process(ctx, ai_data, ai_data, datasize);
                                pthread_mutex_unlock(&ctx->ns_lock);
                        }
                        if (ctx->agc_enable && ctx->agc_handle)
                        {
                                pthread_mutex_lock(&ctx->agc_lock);
                                _ai_agc_process(ctx, ai_data, ai_data, datasize);
                                pthread_mutex_unlock(&ctx->agc_lock);
                        }
                        if (ctx->hpf_enable)
                        {
                                pthread_mutex_lock(&ctx->hpf_lock);
                                _ai_hpf_process(ctx, ai_data, datasize);
                                pthread_mutex_unlock(&ctx->hpf_lock);
                        }
#endif

                        /* IMPP_LOGD(TAG,"###########readfrm = %d data-size = %d ##########\n",ret,datasize); */
                        gettimeofday(&cur_tv, NULL);

                        ai_buf_info->timestamp = timeval_diff(&ctx->base_tv, &cur_tv);
                        ai_buf_info->node = ainode;
                        ai_buf_info->seq = cnt++;
                        ai_buf_info->size = datasize;
                        pthread_mutex_lock(&ctx->ai_buf_lock);
                        audio_buf_put_node(ctx->ai_buf_head, ainode, AUDIO_BUF_FULL); // error
                        pthread_mutex_unlock(&ctx->ai_buf_lock);

                        pthread_cond_signal(&ctx->aidata_ready_cond);
                }
                else
                {
                        usleep(50000);
                }
        }
}

static int _init_ref_pcm_device(prv_ai_ctx_t *ctx)
{
        snd_pcm_t *handle;
        snd_pcm_hw_params_t *hw_params;
        int ret = 0;
        ret = snd_pcm_open(&handle, "plug:tloop_cap", SND_PCM_STREAM_CAPTURE, SND_PCM_ASYNC);
        // ret = snd_pcm_open(&handle,"hw:0,2",SND_PCM_STREAM_CAPTURE,SND_PCM_ASYNC);
        if (ret)
        {
                IMPP_LOGE(TAG, "open tloop  node %s failed", ctx->attr.audio_node);
                return -1;
        }

        snd_pcm_hw_params_alloca(&hw_params);
        ret = snd_pcm_hw_params_any(handle, hw_params);
        if (ret)
        {
                IMPP_LOGE(TAG, "set default hw param failed");
                snd_pcm_close(handle);
                return -1;
        }

        ret = snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
        if (ret)
        {
                IMPP_LOGE(TAG, "set default access-mode SND_PCM_ACCESS_RW_INTERLEAVED failed");
                snd_pcm_close(handle);
                return IHAL_RNULL;
        }

        int fmt = __imppfmt_to_alsa(ctx->attr.SampleFmt);
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

        unsigned int rate = ctx->attr.SampleRate;
        ret = snd_pcm_hw_params_set_rate_near(handle, hw_params, &rate, 0);
        if (ret)
        {
                IMPP_LOGE(TAG, "set sample rate failed");
                snd_pcm_close(handle);
                return IHAL_RNULL;
        }

        ret = snd_pcm_hw_params_set_channels(handle, hw_params, 1); // tloop only mono
        if (ret)
        {
                IMPP_LOGE(TAG, "set channels failed");
                snd_pcm_close(handle);
                return IHAL_RNULL;
        }

        ret = snd_pcm_hw_params(handle, hw_params);
        if (ret)
        {
                IMPP_LOGE(TAG, "set last hw-params failed");
                snd_pcm_close(handle);
                return IHAL_RNULL;
        }

        ctx->ref_data = malloc(ctx->attr.numPerSample * ctx->bytewidth * 4);
        ctx->ref_pcm_handle = handle;
        snd_pcm_prepare(handle);
        return 0;
}

static int _ref_device_deinit(prv_ai_ctx_t *ctx)
{
        pthread_mutex_lock(&ctx->aec_lock);
        snd_pcm_close(ctx->ref_pcm_handle);
        ctx->ref_pcm_handle = NULL;
        free(ctx->ref_data);
        pthread_mutex_unlock(&ctx->aec_lock);
        return 0;
}

IHal_AudioHandle_t *IHal_AI_ChanCreate(IHal_AI_Attr_t *attr)
{
        snd_pcm_t *handle;
        snd_pcm_hw_params_t *hw_params;
        snd_pcm_sw_params_t *sw_params;
        IHal_AudioHandle_t *pAudio = NULL;
        int ret = 0;
        snd_pcm_uframes_t period_size = 0;
        snd_pcm_uframes_t buffer_size = 0;
        snd_pcm_uframes_t start_threshold, stop_threshold;
        if (attr->bufferDeep < 4)
        {
                return IHAL_RNULL;
        }
        ret = snd_pcm_open(&handle, attr->audio_node, SND_PCM_STREAM_CAPTURE, SND_PCM_ASYNC);
        if (ret)
        {
                IMPP_LOGE(TAG, "open audio capture  node %s failed", attr->audio_node);
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

        unsigned int rate = attr->SampleRate;
        ret = snd_pcm_hw_params_set_rate_near(handle, hw_params, &rate, 0);
        if (ret)
        {
                IMPP_LOGE(TAG, "set sample rate failed");
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

        period_size = attr->numPerSample;
        ret = snd_pcm_hw_params_set_period_size_near(handle, hw_params, &period_size, 0);
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
        IMPP_LOGW(TAG, "period_size:%d,buffer_size:%d", period_size, buffer_size);

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
                IMPP_LOGE(TAG, "get current sw-params failed");
                snd_pcm_close(handle);
                return IHAL_RNULL;
        }

        ret = snd_pcm_sw_params_set_avail_min(handle, sw_params, period_size);
        if (ret)
        {
                IMPP_LOGE(TAG, "set avail-min failed");
                snd_pcm_close(handle);
                return IHAL_RNULL;
        }

        start_threshold = 1;
        stop_threshold = buffer_size;
        ret = snd_pcm_sw_params_set_start_threshold(handle, sw_params, start_threshold);
        if (ret)
        {
                IMPP_LOGE(TAG, "set start-threshold failed");
                snd_pcm_close(handle);
                return IHAL_RNULL;
        }

        ret = snd_pcm_sw_params_set_stop_threshold(handle, sw_params, stop_threshold);
        if (ret)
        {
                IMPP_LOGE(TAG, "set stop-threshold failed");
                snd_pcm_close(handle);
                return IHAL_RNULL;
        }

        IMPP_LOGI(TAG, "start_threshold:%d,stop_threshold:%d", start_threshold, stop_threshold);

        ret = snd_pcm_sw_params(handle, sw_params);
        if (ret)
        {
                IMPP_LOGE(TAG, "set last sw-params failed");
                snd_pcm_close(handle);
                return IHAL_RNULL;
        }

        pAudio = (IHal_AudioHandle_t *)calloc(1, sizeof(IHal_AudioHandle_t));
        if (!pAudio)
        {
                IMPP_LOGE(TAG, "malloc audio-handle failed");
                snd_pcm_close(handle);
                return IHAL_RNULL;
        }

        prv_ai_ctx_t *pctx = NULL;
        pctx = calloc(1, sizeof(prv_ai_ctx_t));
        if (!pctx)
        {
                IMPP_LOGE(TAG, "malloc prv_ai_ctx_t failed");
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
        pthread_mutex_init(&pctx->ai_lock, NULL);
        pthread_mutex_init(&pctx->ai_buf_lock, NULL);
        pthread_mutex_init(&pctx->aec_lock, NULL);
        pthread_mutex_init(&pctx->agc_lock, NULL);
        pthread_mutex_init(&pctx->ns_lock, NULL);
        pthread_mutex_init(&pctx->hpf_lock, NULL);

        /** alloc audio buf */
        void *ai_buf = audio_buf_alloc(pctx->bufferDeep, pctx->bytewidth * pctx->channels * pctx->numPerSample, sizeof(ai_data_info_t));
        if (!ai_buf)
        {
                IMPP_LOGE(TAG, "alloc audio buf failed");
                snd_pcm_close(handle);
                free(pAudio);
                free(pctx);
                return IHAL_RNULL;
        }
        pctx->ai_buf_head = ai_buf;
        memcpy(&pctx->attr, attr, sizeof(IHal_AI_Attr_t));
#ifdef AEC_SUPPORT
        ret = _init_ref_pcm_device(pctx);
        if (ret)
        {
                IMPP_LOGE(TAG, "ref device init failed");
                return -IHAL_RERR;
        }
#endif
        return pAudio;
}

IHAL_INT32 IHal_AI_ChanDestroy(IHal_AudioHandle_t *handle)
{
        int ret = 0;
        assert(handle);
        prv_ai_ctx_t *ctx = handle->priv;
        snd_pcm_t *pcm = (snd_pcm_t *)handle->pcm_handle;
        if (ctx->ai_thread_tid)
        {
                pthread_cancel(ctx->ai_thread_tid);
                pthread_join(ctx->ai_thread_tid, NULL);
        }

        if (ctx->tloop_thread_tid)
        {
                pthread_cancel(ctx->tloop_thread_tid);
                pthread_join(ctx->tloop_thread_tid, NULL);
        }

        ret = snd_pcm_close(pcm);
        if (ret)
        {
                IMPP_LOGE(TAG, "close pcm device failed");
                return -IHAL_RERR;
        }
#ifdef AEC_SUPPORT
        _ref_device_deinit(ctx);
#endif
        pthread_mutex_destroy(&ctx->ai_lock);
        pthread_mutex_destroy(&ctx->ai_buf_lock);
        pthread_mutex_destroy(&ctx->aec_lock);
        pthread_mutex_destroy(&ctx->agc_lock);
        pthread_mutex_destroy(&ctx->ns_lock);
        pthread_mutex_destroy(&ctx->hpf_lock);
        audio_buf_free(ctx->ai_buf_head);
        free(ctx);
        free(handle);
        return IHAL_ROK;
}

IHAL_INT32 IHal_AI_ChanStart(IHal_AudioHandle_t *handle)
{
        int ret = 0;
        assert(handle);
        prv_ai_ctx_t *ctx = handle->priv;
        ret = snd_pcm_prepare(handle->pcm_handle); // alsa pcm start
        if (ret)
        {
                IMPP_LOGE(TAG, "ai start failed");
                return -IHAL_RFAILED;
        }

        ret = pthread_create(&ctx->ai_thread_tid, NULL, __ai_thread, (void *)handle);
        if (ret)
        {
                IMPP_LOGE(TAG, "audio input pthread create failed");
                return -IHAL_RFAILED;
        }

        gettimeofday(&ctx->base_tv, NULL);
        pthread_mutex_lock(&ctx->ai_lock);
        ctx->ai_thread_run = 0x01;
        pthread_mutex_unlock(&ctx->ai_lock);

        return IHAL_ROK;
}

IHAL_INT32 IHal_AI_ChanStop(IHal_AudioHandle_t *handle)
{
        assert(handle);
        prv_ai_ctx_t *ctx = handle->priv;
        int ret = 0;
#if 0
        ret = snd_pcm_drop(handle->pcm_handle);
        if (ret) {
                IMPP_LOGE(TAG, "snd pcm stop failed");
                return -IHAL_RFAILED;
        }
#endif
        pthread_mutex_lock(&ctx->ai_lock);
        ctx->ai_thread_run = 0x00;
        pthread_mutex_unlock(&ctx->ai_lock);

        return IHAL_ROK;
}

IHAL_INT32 IHal_AI_GetBuffer(IHal_AudioHandle_t *handle, IHal_AudioBuffer_t *buf, IHAL_INT32 wait)
{
        assert(handle);
        prv_ai_ctx_t *ctx = handle->priv;
        int ret = 0;
        void *ainode = NULL;
        if (!buf)
        {
                return -IHAL_RERR;
        }
        if (ctx->ai_thread_run == 0)
        {
                return -IHAL_RERR;
        }
        do
        {
                pthread_mutex_lock(&ctx->ai_buf_lock);
                ainode = audio_buf_get_node(ctx->ai_buf_head, AUDIO_BUF_FULL);
                pthread_mutex_unlock(&ctx->ai_buf_lock);
                if (ainode)
                {
                        ai_data_info_t *aidata_info = audio_buf_node_get_info(ainode);
                        buf->vaddr = (unsigned int *)aidata_info->data;
                        buf->datalen = aidata_info->size;
                        buf->timestamp = aidata_info->timestamp;
                        buf->seq = aidata_info->seq;
                        buf->node = ainode;
                        return 0;
                }

                if (wait == IMPP_WAIT_FOREVER)
                {
                        __wait_audio_data(handle);
                }
        } while (wait == IMPP_WAIT_FOREVER);

        return -IHAL_RFAILED;
}

IHAL_INT32 IHal_AI_ReleaseBuffer(IHal_AudioHandle_t *handle, IHal_AudioBuffer_t *buf)
{
        assert(handle);
        prv_ai_ctx_t *ctx = handle->priv;
        int ret = 0;
        if (!buf->node)
        {
                return -IHAL_RERR;
        }
        pthread_mutex_lock(&ctx->ai_buf_lock);
        if (ctx->ai_thread_run)
        {
                audio_buf_put_node(ctx->ai_buf_head, buf->node, AUDIO_BUF_EMPTY);
        }
        pthread_mutex_unlock(&ctx->ai_buf_lock);
        return IHAL_ROK;
}


IHAL_INT32 IHal_AI_SetVolume(IHal_AudioHandle_t *handle, IHAL_INT32 value)
{
	/* TODO : Recording does not support software tuning. */
	IMPP_LOGW(TAG, "Recording does not support software tuning.");

	return IHAL_ROK;
}

IHAL_INT32 IHal_AI_GetVolume(IHal_AudioHandle_t *handle, IHAL_INT32 *value)
{
	/* TODO : Recording does not support software tuning. */
	IMPP_LOGW(TAG, "Recording does not support software tuning.");

	return IHAL_ROK;
}

IHAL_INT32 IHal_AI_SetGain(IHal_AudioHandle_t *handle, IHAL_INT32 value)
{
        assert(handle);
        int ret = 0;
        snd_mixer_t *mixer_handle;
        snd_mixer_elem_t *elem;
        prv_ai_ctx_t *ctx = handle->priv;

        ret = snd_mixer_open(&mixer_handle, 0);
        if (ret)
        {
                IMPP_LOGE(TAG, "open mixer failed");
                snd_mixer_close(mixer_handle);
                return -IHAL_RFAILED;
        }

        if (!strcmp(ctx->node, "plug:cap_dmic"))
        {
                // dmic cannt set volume
                IMPP_LOGW(TAG, "dmic not need set gain");
                snd_mixer_close(mixer_handle);
                return IHAL_ROK;
        }
        else
        {
                ret = snd_mixer_attach(mixer_handle, "default");
        }
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
		int capture_volume_min = 0;
		int capture_volume_max = 0;
        while (elem)
        {
			// printf("snd_mixer_selem_get_name(elem): %s\n", snd_mixer_selem_get_name(elem));
                if (snd_mixer_selem_is_active(elem))
                {
                        if (snd_mixer_selem_has_common_volume(elem))
                        {
                        }
                        else
                        {
                                if (snd_mixer_selem_has_capture_volume(elem))
                                {
									ret = snd_mixer_selem_get_capture_volume_range(elem, &capture_volume_min, &capture_volume_max);
									if (ret) {
										IMPP_LOGE(TAG, "Failed to get capture volume range.");
										snd_mixer_close(mixer_handle);
										return -IHAL_RERR;
									}

									IMPP_LOGI(TAG, "The capture volume range is [%d, %d].", capture_volume_min, capture_volume_max);

									if (capture_volume_min > value || value > capture_volume_max) {
										IMPP_LOGE(TAG, "The capture volume range is [%d, %d], but you have set it to %d.", \
														capture_volume_min, capture_volume_max, value);
										snd_mixer_close(mixer_handle);
										return -IHAL_RERR;
									}

									capture_volume_min = 0;
									capture_volume_max = 0;

                                        found = 1;
                                        break;
                                }
                        }
                }
                elem = snd_mixer_elem_next(elem);
        }

        if (!found)
        {
                IMPP_LOGE(TAG, "Failed to found Capture Volume kcontrols.");
                snd_mixer_close(mixer_handle);
                return -IHAL_RERR;
        }

        int volume = value;
        ret = snd_mixer_selem_set_capture_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, volume);
        ret = snd_mixer_selem_set_capture_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT, volume);
        if (ret)
        {
                IMPP_LOGE(TAG, "set capture volume failed");
                snd_mixer_close(mixer_handle);
                return -IHAL_RFAILED;
        }
        snd_mixer_close(mixer_handle);
        return IHAL_ROK;
}

IHAL_INT32 IHal_AI_GetGain(IHal_AudioHandle_t *handle, IHAL_INT32 *value)
{
        assert(handle);
        int ret = 0;
        snd_mixer_t *mixer_handle;
        snd_mixer_elem_t *elem;
        prv_ai_ctx_t *ctx = handle->priv;
        long right = 0, left = 0;

        ret = snd_mixer_open(&mixer_handle, 0);
        if (ret)
        {
                IMPP_LOGE(TAG, "open mixer failed");
                snd_mixer_close(mixer_handle);
                return -IHAL_RFAILED;
        }

        if (!strcmp(ctx->node, "plug:cap_dmic"))
        {
                // dmic cannt set volume
                IMPP_LOGW(TAG, "dmic not need set gain");
                snd_mixer_close(mixer_handle);
                return IHAL_ROK;
        }
        else
        {
                ret = snd_mixer_attach(mixer_handle, "default");
        }
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
						if (snd_mixer_selem_has_capture_volume(elem)) {
							found = 1;
							break;
						}
					}
				}

                elem = snd_mixer_elem_next(elem);
        }

		if (!found) {
			IMPP_LOGE(TAG, "Failed to found Capture Volume kcontrols!\n");
			snd_mixer_close(mixer_handle);
			return -IHAL_RERR;
		}

        ret = snd_mixer_selem_get_capture_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &left);
        ret = snd_mixer_selem_get_capture_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT, &right);
        if (ret || (left != right))
        {
                IMPP_LOGE(TAG, "set capture volume failed");
                snd_mixer_close(mixer_handle);
                return -IHAL_RFAILED;
        }

        *value = left;
        snd_mixer_close(mixer_handle);
        return IHAL_ROK;
}

IHAL_INT32 IHal_AI_SingleChannelSet(IHal_AudioHandle_t *handle, IHal_Audio_MonoType_t mono)
{
        assert(handle);
        prv_ai_ctx_t *ctx = handle->priv;
        int ret = 0;
        ret = _set_ctl_elem_value(ALSA_HCTL_MONO_ELEM_NAME, mono);
        if (ret)
        {
                return -IHAL_RERR;
        }
        return IHAL_ROK;
}

#ifdef AEC_SUPPORT
IHAL_INT32 IHal_AI_EnableAec(IHal_AudioHandle_t *handle)
{
        assert(handle);
        prv_ai_ctx_t *ctx = handle->priv;
        int ret = 0;
        if (!strcmp(ctx->node, "plug:cap_dmic"))
        {
                /* dmic cannot enable AEC */
                IMPP_LOGE(TAG, "Dmic cannot enable AEC.");
                return -IHAL_RERR;
        }

        // WebRtcAec_Create();
        ctx->aec_handle = audio_process_aec_create(ctx->attr.SampleRate);
        if (!ctx->aec_handle)
        {
                IMPP_LOGE(TAG, "aec handle init failed");
                return -IHAL_RERR;
        }
        ctx->aec_outbuf = malloc(ctx->attr.numPerSample * ctx->bytewidth * 4);
        pthread_mutex_lock(&ctx->aec_lock);
        ctx->aec_enable = 1;
        pthread_mutex_unlock(&ctx->aec_lock);
        return IHAL_ROK;
}

IHAL_INT32 IHal_AI_DisableAec(IHal_AudioHandle_t *handle)
{
        assert(handle);
        prv_ai_ctx_t *ctx = handle->priv;
        int ret = 0;
        if (!strcmp(ctx->node, "plug:cap_dmic"))
        {
                IMPP_LOGE(TAG, "The dmic cannot support the AEC.");
                return -IHAL_RERR;
        }

        pthread_mutex_lock(&ctx->aec_lock);
        ctx->aec_enable = 0;
        pthread_mutex_unlock(&ctx->aec_lock);
        free(ctx->aec_outbuf);
        audio_process_aec_free(ctx->aec_handle);
        ctx->aec_handle = NULL;
        return IHAL_ROK;
}

IHAL_INT32 IHal_AI_EnableAgc(IHal_AudioHandle_t *handle, IHal_AudioAgcConfig_t *config)
{
        prv_ai_ctx_t *ctx = handle->priv;
        int ret = 0;
        AgcConfig cfg;
        cfg.targetLevelDbfs = config->DbLevel;
        cfg.compressionGaindB = config->MaxGainDb;
        cfg.limiterEnable = 1;
        ctx->agc_handle = audio_process_agc_create();
        if (!ctx->agc_handle)
        {
                IMPP_LOGE(TAG, "create agc processer failed");
                return -IHAL_RERR;
        }
        ret = audio_process_agc_set_config(ctx->agc_handle, 0, 255, AgcModeAdaptiveDigital, ctx->samplerate, cfg);
        if (ret)
        {
                audio_process_agc_free(ctx->agc_handle);
                IMPP_LOGE(TAG, "config agc failed");
                return -IHAL_RERR;
        }
        if (ctx->samplerate == 8000)
        {
                ctx->agc_sample_i = 80;
        }
        else
        {
                ctx->agc_sample_i = 160;
        }
        ctx->agc_enable = 1;
        return IHAL_ROK;
}

IHAL_INT32 IHal_AI_DisableAgc(IHal_AudioHandle_t *handle)
{
        prv_ai_ctx_t *ctx = handle->priv;
        int ret = 0;
        pthread_mutex_lock(&ctx->agc_lock);
        ctx->agc_enable = 0;
        audio_process_agc_free(ctx->agc_handle);
        pthread_mutex_unlock(&ctx->agc_lock);
        return IHAL_ROK;
}

IHAL_INT32 IHal_AI_EnableNs(IHal_AudioHandle_t *handle, int mode)
{
        if ((mode > 3) || (mode < 0))
        {
                IMPP_LOGW(TAG, "ns mode error regin [0,3] default set 3");
                mode = 3;
        }
        prv_ai_ctx_t *ctx = handle->priv;
        ctx->ns_handle.handle = audio_process_ns_create();
        if (!ctx->ns_handle.handle)
        {
                IMPP_LOGE(TAG, "create ns processer failed");
                return -IHAL_RFAILED;
        }
        audio_process_ns_set_config(ctx->ns_handle.handle, ctx->attr.SampleRate, mode);
        if (ctx->attr.SampleRate == 8000)
        {
                ctx->ns_handle.cell_num = 80;
        }
        else
        {
                ctx->ns_handle.cell_num = 160;
        }
        ctx->ns_handle.sp = (float *)malloc(ctx->ns_handle.cell_num * sizeof(float));
        memset(ctx->ns_handle.sp, 0, sizeof(float) * ctx->ns_handle.cell_num);
        ctx->ns_handle.out = (float *)malloc(ctx->ns_handle.cell_num * sizeof(float));
        memset(ctx->ns_handle.out, 0, sizeof(float) * ctx->ns_handle.cell_num);
        pthread_mutex_lock(&ctx->ns_lock);
        ctx->ns_enable = 1;
        pthread_mutex_unlock(&ctx->ns_lock);

        return IHAL_ROK;
}

IHAL_INT32 IHal_AI_DisableNs(IHal_AudioHandle_t *handle)
{
        prv_ai_ctx_t *ctx = handle->priv;
        pthread_mutex_lock(&ctx->ns_lock);
        ctx->ns_enable = 0;
        audio_process_ns_free(ctx->ns_handle.handle);
        ctx->ns_handle.handle = NULL;
        pthread_mutex_unlock(&ctx->ns_lock);

        return IHAL_ROK;
}

IHAL_INT32 IHal_AI_EnableHpf(IHal_AudioHandle_t *handle, unsigned int cutoff_Freq)
{
        prv_ai_ctx_t *ctx = handle->priv;
        if (cutoff_Freq != 0)
        {
                hpf_gen_filter_coefficients(ctx->ai_hpf_coefficient, ctx->attr.SampleRate, cutoff_Freq);
                ctx->hpf_handle.ba = ctx->ai_hpf_coefficient;
        }
        else
        {
                if (ctx->attr.SampleRate == 8000)
                {
                        ctx->hpf_handle.ba = kFilterCoefficients8kHz;
                }
                else
                {
                        ctx->hpf_handle.ba = kFilterCoefficients;
                }
        }
        audio_process_hpf_create(ctx->hpf_handle.x, ctx->hpf_handle.y, 0, 0, 2, 4);
        pthread_mutex_lock(&ctx->hpf_lock);
        ctx->hpf_enable = 1;
        pthread_mutex_unlock(&ctx->hpf_lock);
        return IHAL_ROK;
}

IHAL_INT32 IHal_AI_DisableHpf(IHal_AudioHandle_t *handle)
{
        prv_ai_ctx_t *ctx = handle->priv;
        pthread_mutex_lock(&ctx->hpf_lock);
        ctx->hpf_enable = 0;
        pthread_mutex_unlock(&ctx->hpf_lock);
        return 0;
}
#endif
