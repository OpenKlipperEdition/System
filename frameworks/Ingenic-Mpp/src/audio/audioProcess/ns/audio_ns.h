#ifndef AUDIO_NS_H
#define AUDIO_NS_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct NsHandleT NsHandle;
typedef struct {
        NsHandle *handle;
        int cell_num;
        float *sp, *out;
} NsHandle_t;

NsHandle *audio_process_ns_create();

int audio_process_ns_set_config(NsHandle *handle, int samplerate, int mode);

void audio_process_ns_get_config();

void audio_process_ns_process(NsHandle *handle, const float *const *spframe, int num_bands, float *const *outframe);

void audio_process_ns_free(NsHandle *handle);

#ifdef __cplusplus
}
#endif
#endif
