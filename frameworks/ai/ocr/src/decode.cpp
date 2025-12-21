#include <iostream>
#include <stdio.h>
#include <iconv.h>
#include <string.h>
using namespace std;
#include "keys.cpp"

int decode(const long long *result_array, int size)
{
  const char *encTo = "UTF-8";
  //const char *encFrom = "UNICODE//TRANSLIT";
  const char *encFrom = "UNICODELITTLE";

  int selected_count = 0;
  size_t selected_size, selected_utf8_size;
  char *selected_utf8_p;
  char32_t *selected_p;
  iconv_t cd ;
  size_t ret;

  if ((selected_p = (char32_t*)calloc(sizeof(char32_t) * size, 1)) == NULL) {
    perror("calloc");
    exit(-1);
  }

  if ((cd = iconv_open(encTo, encFrom)) == (iconv_t) -1) {
    perror("iconv_open");
    exit(-1);
  }

  for (int i = 0; i < size; i++) {
    if (result_array[i] != 0 && !(i > 0 && result_array[i-1] == result_array[i])) {
      int index = (int)(result_array[i] - 1);

      if (index > alphabet_count)
        index = alphabet_count;

      selected_p[selected_count++] = alphabet[index];
    }
  }

  selected_size = selected_count * sizeof(char32_t);
  // max utf-8 encode is 6byte, and append '\0' for each selected word.
  selected_utf8_size = selected_count * 6 + selected_count;

  if ((selected_utf8_p = (char*)calloc(selected_utf8_size, 1)) == NULL) {
    perror("calloc");
    exit(-1);
  }

  char *inbuf = (char*)(selected_p);
  char *outbuf = selected_utf8_p;
  size_t inleft = selected_size;
  size_t outleft = selected_utf8_size;


  if ((ret = iconv(cd, &inbuf, &inleft, &outbuf, &outleft)) == -1)
  {
    perror("iconv");
    exit(-1);
  }

  // show result
  outbuf = selected_utf8_p;
  printf("srclen=%ld, outbuf=%s, outlen=%ld ret %ld\n", inleft, outbuf, outleft, ret);
  printf("test: ");
  for(int i = 0; i <= outleft+1; i++)
  {
    printf("%s", outbuf);
    outbuf  = outbuf + strlen(outbuf)+1;
  }

  printf("\n");

  free(selected_p);
  free(selected_utf8_p);
  iconv_close(cd);

  return 0;
}
