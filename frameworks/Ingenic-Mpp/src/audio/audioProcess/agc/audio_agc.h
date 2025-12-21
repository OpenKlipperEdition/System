#ifndef AUDIO_AGC_H
#define AUDIO_AGC_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

enum {
        AgcModeUnchanged,
        AgcModeAdaptiveAnalog,
        AgcModeAdaptiveDigital,
        AgcModeFixedDigital
};

enum {
        AgcFalse = 0,
        AgcTrue
};

typedef struct {
        int16_t targetLevelDbfs;
        int16_t compressionGaindB;
        uint8_t limiterEnable;
} AgcConfig;

void *audio_process_agc_create();

int audio_process_agc_set_config(void *handle_agc, int32_t minLevel, int32_t maxLevel,
                                 int32_t agcMode, int samplerate, AgcConfig config);

void audio_process_agc_get_config(void *handle_agc, AgcConfig *config);

int audio_process_agc_process(void *agcInst,
                              const int16_t *const *inNear,
                              size_t num_bands,
                              size_t samples,
                              int16_t *const *out,
                              int32_t inMicLevel,
                              int32_t *outMicLevel,
                              int16_t echo,
                              uint8_t *saturationWarning);

void audio_process_agc_free(void *handle_agc);

#ifdef __cplusplus
}
#endif
#endif
