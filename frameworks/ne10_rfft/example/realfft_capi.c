#include <stdio.h>
#include <stdint.h>
#include <fft.h>

#define FFT_LEN			256
#define ARRAY_GUARD_LEN  4
void fft_test() {

  ne10_fft_r2c_cfg_float32_t cfg;
  float *f = (float*)malloc((FFT_LEN + ARRAY_GUARD_LEN) *  sizeof(float));
  float *out = (float*)malloc((FFT_LEN + ARRAY_GUARD_LEN) *  sizeof(float));
  cfg = ne10_fft_alloc_r2c_float32 ((ne10_int32_t)FFT_LEN);

  // do fft MSA interface
  fft_msa (f, out, cfg);

  ne10_fft_destroy_r2c_float32(cfg);
  free(f);
  free(out);
  return;
}


void ifft_test() {
  ne10_fft_r2c_cfg_float32_t cfg;
  float *f = (float*)malloc((FFT_LEN + ARRAY_GUARD_LEN) *  sizeof(float));
  float *ifft_out = (float*)malloc((FFT_LEN + ARRAY_GUARD_LEN) *  sizeof(float));
  cfg = ne10_fft_alloc_r2c_float32 ((ne10_int32_t)FFT_LEN);

  // call ifft MSA interface
  ifft_msa (f, ifft_out, cfg);

  ne10_fft_destroy_r2c_float32(cfg);
  free(f);
  free(ifft_out);
  return ;
}

int main(int argc, char *argv[]) {
  ifft_test();
  fft_test();
  return 0;
}
