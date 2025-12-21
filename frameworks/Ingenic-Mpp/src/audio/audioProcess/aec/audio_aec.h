#ifndef AUDIO_AEC_H
#define AUDIO_AEC_H

#ifdef __cplusplus
extern "C" {
#endif

struct aec_process_param {
        char *far;
        char *near;
        char *filter;
        int size;
};

void *audio_process_aec_create(int sample_rate);

int audio_process_aec_process(void *aec_handel, struct aec_process_param *param, int delayms);

int audio_process_aec_free(void *aec_handel);

#ifdef __cplusplus
}

#endif
#endif

