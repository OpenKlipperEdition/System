#ifndef AUDIO_HPF_H
#define AUDIO_HPF_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct {
        int16_t y[4];
        int16_t x[2];
        const int16_t *ba;
} FilterState;

void audio_process_hpf_create(int16_t *vector_x, int16_t *vector_y, int16_t set_value_x,
                              int16_t set_value_y, int vector_length_x, int vector_length_y);

int audio_process_hpf_process(void *filter, short *data, int length);

void audio_process_hpf_free();

#ifdef __cplusplus
}
#endif
#endif
