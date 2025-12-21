#include <stdio.h>
#include <stdint.h>
#include <fft.h>

#define FFT_LEN          128
#define ARRAY_GUARD_LEN  4
void fft_test() {
  float *f = (float*)malloc((FFT_LEN + ARRAY_GUARD_LEN) *  sizeof(float));
  float *out = (float*)malloc((FFT_LEN + ARRAY_GUARD_LEN) *  sizeof(float));
  realfft r(FFT_LEN);

  r.fft_msa(f, out);

  free(f);
  free(out);
  return;

}


void ifft_test() {
  float *f = (float*)malloc((FFT_LEN + ARRAY_GUARD_LEN) *  sizeof(float));
  float *out = (float*)malloc((FFT_LEN + ARRAY_GUARD_LEN) *  sizeof(float));
  realfft r(FFT_LEN);

  r.ifft_msa(f, out);

  free(f);
  free(out);
  return;
}

int main(int argc, char *argv[]) {

  fft_test();
  ifft_test();
  return 0;
}
