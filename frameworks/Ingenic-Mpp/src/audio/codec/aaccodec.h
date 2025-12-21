#ifndef __AAC_CODEC_H__
#define __AAC_CODEC_H__

enum {
        MP4_RAW,
        MP4_ADTS,
        MP4_ADIF,
};

typedef struct {
        int aot;
        int sbr_enable;
        int vbr_type;
        int transmux;
} aacEnc_prv_attr_t;

typedef struct {
        int aot;
        int transmux;
} aacDec_prv_attr_t;

void *aac_encoder_init(IHal_AudioEnc_Attr_t *attr, aacEnc_prv_attr_t *prv_attr);

void aac_encoder_deinit(void *prv);

int aac_encode(void *prv, char *input_data, int input_len, void *out_data);

void *aac_decoder_init(IHal_AudioDec_Attr_t *attr, aacDec_prv_attr_t *prv_attr);

int aac_decode(void *prv, char *input_data, int input_len, void *out_data);

int aac_decoder_deinit(void *prv);

#endif
