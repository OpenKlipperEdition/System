#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include "impp.h"
#include "audio.h"
#include "log.h"
#include "fdk-aac/aacdecoder_lib.h"
#include "fdk-aac/aacenc_lib.h"
#include "aaccodec.h"

#define TAG "AAC_CODEC"


typedef struct {
        int channels;
        unsigned int out_buffer_size;
        unsigned int sample_rate;
        short *convertbuf;
        AACENC_InfoStruct aacinfo;
        HANDLE_AACENCODER handle;
} aacEncoder_ctx_t;

typedef struct {
        unsigned int out_buffer_size;
        int channels;
        HANDLE_AACDECODER handle;
} aacDecoder_ctx_t;

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


void *aac_encoder_init(IHal_AudioEnc_Attr_t *attr, aacEnc_prv_attr_t *prv_attr)
{
        assert(attr);
        assert(prv_attr);
        int aot = prv_attr->aot;
        int afterburner = 1;
        int eld_sbr = prv_attr->sbr_enable;
        int vbr = prv_attr->vbr_type;
        int sample_rate = attr->SampleRate;
        int channels = attr->Channels;
        int bitrate = attr->BitRate;
        int transmux = 0;
        CHANNEL_MODE mode;
        aacEncoder_ctx_t *ctx = (aacEncoder_ctx_t *)malloc(sizeof(aacEncoder_ctx_t));
        if (!ctx) {
                return NULL;
        }
        int bytewidth = __imppfmt_to_bytewidth(attr->SampleFmt);
        ctx->out_buffer_size = bytewidth * attr->Channels * attr->numPerSample;

        switch (channels) {
        case 1:
                mode = MODE_1;
                break;
        case 2:
                mode = MODE_2;
                break;
        default:
                IMPP_LOGE(TAG, "aac encoder channels error");
                goto free_ctx;
        }

        switch (prv_attr->transmux) {
        case MP4_RAW:
                transmux = TT_MP4_RAW;
                break;
        case MP4_ADIF:
                transmux = TT_MP4_ADIF;
                break;
        case MP4_ADTS:
                transmux = TT_MP4_ADTS;
                break;
        default:
                transmux = TT_MP4_ADTS;
        }

        if (aacEncOpen(&ctx->handle, 0, channels) != AACENC_OK) {
                IMPP_LOGE(TAG, "Unable to open encoder");
                goto free_ctx;
        }

        if (aacEncoder_SetParam(ctx->handle, AACENC_AOT, aot) != AACENC_OK) {
                IMPP_LOGE(TAG, "Unable to set the AOT");
                goto free_ctx;
        }

        if (aot == 39 && eld_sbr) {
                if (aacEncoder_SetParam(ctx->handle, AACENC_SBR_MODE, 1) != AACENC_OK) {
                        IMPP_LOGE(TAG, "Unable to set SBR mode for ELD");
                        goto free_ctx;
                }
        }

        if (aacEncoder_SetParam(ctx->handle, AACENC_SAMPLERATE, sample_rate) != AACENC_OK) {
                IMPP_LOGE(TAG, "Unable to set the sample_rate");
                goto free_ctx;
        }

        if (aacEncoder_SetParam(ctx->handle, AACENC_CHANNELMODE, mode) != AACENC_OK) {
                IMPP_LOGE(TAG, "Unable to set the channel mode");
                goto free_ctx;
        }

        if (aacEncoder_SetParam(ctx->handle, AACENC_CHANNELORDER, 1) != AACENC_OK) {
                IMPP_LOGE(TAG, "Unable to set the wav channel order");
                goto free_ctx;
        }

        if (vbr) {
                if (aacEncoder_SetParam(ctx->handle, AACENC_BITRATEMODE, vbr) != AACENC_OK) {
                        IMPP_LOGE(TAG, "Unable to set the VBR bitrate mode");
                        goto free_ctx;
                }
        } else {
                if (aacEncoder_SetParam(ctx->handle, AACENC_BITRATE, bitrate) != AACENC_OK) {
                        IMPP_LOGE(TAG, "Unable to set the bitrate");
                        goto free_ctx;
                }
        }

        if (aacEncoder_SetParam(ctx->handle, AACENC_TRANSMUX, transmux) != AACENC_OK) {
                IMPP_LOGE(TAG, "Unable to set the ADTS transmux");
                goto free_ctx;
        }

        if (aacEncoder_SetParam(ctx->handle, AACENC_AFTERBURNER, afterburner) != AACENC_OK) {
                IMPP_LOGE(TAG, "Unable to set the afterburner mode");
                goto free_ctx;
        }

        if (aacEncEncode(ctx->handle, NULL, NULL, NULL, NULL) != AACENC_OK) {
                IMPP_LOGE(TAG, "Unable to initialize the encoder");
                goto free_ctx;
        }

        if (aacEncInfo(ctx->handle, &ctx->aacinfo) != AACENC_OK) {
                IMPP_LOGE(TAG, "Unable to get the encoder info");
                goto free_ctx;
        }

        if (attr->numPerSample < ctx->aacinfo.frameLength) {
                IMPP_LOGW(TAG, "### the channel frame size is less than %d ###", ctx->aacinfo.frameLength);
        }

        ctx->convertbuf = malloc(attr->Channels * ctx->aacinfo.frameLength * 2);
        if (!ctx->convertbuf) {
                IMPP_LOGE(TAG, "malloc convertbuf failed");
                aacEncClose(ctx->handle);
                goto free_ctx;
        }
        return (void *)ctx;
free_ctx:
        free(ctx);
        return NULL;
}

void aac_encoder_deinit(void *prv)
{
        assert(prv);
        aacEncoder_ctx_t *ctx = (aacEncoder_ctx_t *)prv;
        aacEncClose(&ctx->handle);
        free(ctx->convertbuf);
}

int aac_encode(void *prv, char *input_data, int input_len, void *out_data)
{
        assert(prv);
        aacEncoder_ctx_t *ctx = (aacEncoder_ctx_t *)prv;

        AACENC_BufDesc in_buf = { 0 }, out_buf = { 0 };
        AACENC_InArgs in_args = { 0 };
        AACENC_OutArgs out_args = { 0 };
        int in_identifier = IN_AUDIO_DATA;
        int out_identifier = OUT_BITSTREAM_DATA;
        int in_size, in_elem_size;
        int out_size, out_elem_size;
        AACENC_ERROR err;

        for (int i = 0; i < input_len / 2; i++) {
                const uint8_t *in = &input_data[2 * i];
                ctx->convertbuf[i] = in[0] | (in[1] << 8);
        }


        void *in_ptr = ctx->convertbuf;
        in_size = input_len;
        in_elem_size = 2;

        in_args.numInSamples = input_len <= 0 ? -1 : input_len / 2;
        in_buf.numBufs = 1;
        in_buf.bufs = &in_ptr;
        in_buf.bufferIdentifiers = &in_identifier;
        in_buf.bufSizes = &in_size;
        in_buf.bufElSizes = &in_elem_size;

        void *out_ptr = out_data;
        out_size = ctx->out_buffer_size;
        out_elem_size = 1;

        out_buf.numBufs = 1;
        out_buf.bufs = &out_ptr;
        out_buf.bufferIdentifiers = &out_identifier;
        out_buf.bufSizes = &out_size;
        out_buf.bufElSizes = &out_elem_size;

        err = aacEncEncode(ctx->handle, &in_buf, &out_buf, &in_args, &out_args);
        if (err != AACENC_OK) {
                IMPP_LOGE(TAG, "AAC Encode failed");
                return -1;
        }

        return out_args.numOutBytes;
}


static int sample_rate_to_aac_index(unsigned int sample_rate)
{
        switch (sample_rate) {
        case 96000:
                return 0;
        case 48000:
                return 3;
        case 44100:
                return 4;
        case 32000:
                return 5;
        case 16000:
                return 8;
        case 8000:
                return 11;
        default:
                return -1;
        }
}

void *aac_decoder_init(IHal_AudioDec_Attr_t *attr, aacDec_prv_attr_t *prv_attr)
{
        int transmux;
        aacDecoder_ctx_t *ctx = malloc(sizeof(aacDecoder_ctx_t));
        if (!ctx) {
                IMPP_LOGE(TAG, "init decoder ctx failed");
                return NULL;
        }

        switch (prv_attr->transmux) {
        case MP4_RAW:
                transmux = TT_MP4_RAW;
                break;
        case MP4_ADIF:
                transmux = TT_MP4_ADIF;
                break;
        case MP4_ADTS:
                transmux = TT_MP4_ADTS;
                break;
        default:
                transmux = TT_MP4_RAW;
        }

        int rate_idx = sample_rate_to_aac_index(attr->SampleRate);
        if (rate_idx < 0) {
                IMPP_LOGE(TAG, "sample_rate of raw data not support");
                goto free_ctx;
        }

        ctx->handle = aacDecoder_Open(transmux, 1);
        if (!ctx->handle) {
                IMPP_LOGE(TAG, "open aac decoder failed");
                goto free_ctx;
        }
        int conceal_method = 2; //0 muting 1 noise 2 interpolation
        unsigned short confnum = 0;
        confnum |= (prv_attr->aot <<  11);
        confnum |= (rate_idx << 7);
        confnum |= (attr->Channels << 3);

        printf("confnum = 0x%x ####\r\n", confnum);
        printf("rate_idx = %d prv_attr->aot = %d chns = %d ####\r\n", rate_idx, prv_attr->aot, attr->Channels);

        UCHAR ld_conf[] = {((confnum & 0xff00) >> 8), confnum & 0xff, 0x00, 0x00};
        /* UCHAR ld_conf[] = {0x14,0x10}; */

        printf("ld_conf = 0x%x 0x%x #######\r\n", ld_conf[0], ld_conf[1]);
        UCHAR *conf[] = { ld_conf };
        UINT conf_len = sizeof(ld_conf);
        AAC_DECODER_ERROR err;
        if (transmux == TT_MP4_RAW) {
                err = aacDecoder_ConfigRaw(ctx->handle, conf, &conf_len);
                if (err != AAC_DEC_OK) {
                        IMPP_LOGE(TAG, "aac decoder config failed");
                        goto close_dec;
                }
        }
        aacDecoder_SetParam(ctx->handle, AAC_CONCEAL_METHOD, conceal_method);
        /* aacDecoder_SetParam(ctx->handle,AAC_PCM_MAX_OUTPUT_CHANNELS,attr->Channels); */
        /* aacDecoder_SetParam(ctx->handle,AAC_PCM_MIN_OUTPUT_CHANNELS,attr->Channels); */

        ctx->out_buffer_size = sizeof(short) * attr->AO_numPerSample * attr->Channels;
        ctx->channels = attr->Channels;

        printf("out buffer size  = %d \r\n", ctx->out_buffer_size);

        return (void *)ctx;
close_dec:
        aacDecoder_Close(ctx->handle);
free_ctx:
        free(ctx);
        return NULL;
}

int aac_decode(void *prv, char *input_data, int input_len, void *out_data)
{
        assert(prv);
        aacDecoder_ctx_t *ctx = (aacDecoder_ctx_t *)prv;
        unsigned int valid = input_len;
        AAC_DECODER_ERROR err;
        err = aacDecoder_Fill(ctx->handle, (UCHAR *)&input_data, (UINT *)&input_len, &valid);
        if (err != AAC_DEC_OK) {
                IMPP_LOGE(TAG, "aac decoder fill data error");
                return -1;
        }
        err = aacDecoder_DecodeFrame(ctx->handle, (INT_PCM *)out_data, ctx->out_buffer_size / sizeof(short), 0);
        if (err) {
                if (err == AAC_DEC_NOT_ENOUGH_BITS) {
                        IMPP_LOGW(TAG, "aac decoder not have enouth data to dec");
                        return 0;
                }
                IMPP_LOGE(TAG, "aac decode failed err = 0x%x #########", err);
                return 0;
        }
        CStreamInfo *info = aacDecoder_GetStreamInfo(ctx->handle);
        /* return info.frameSize * ctx->channels * sizeof(short); */
        return info->frameSize * ctx->channels * 2;
}

int aac_decoder_deinit(void *prv)
{
        assert(prv);
        aacDecoder_ctx_t *ctx = (aacDecoder_ctx_t *)prv;
        aacDecoder_Close(ctx->handle);
        return 0;
}












