#ifndef __CONFIG_H
#define __CONFIG_H
#include <stdlib.h>
#include <sys/time.h>

#define INTER_DATA 1
#define DEBUG(...)

#define WIDTH  120
#define HEIGHT 160

#define BUF_COUNT  6

static inline long long getSystemTime()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000LL + tv.tv_usec;
}

#endif /* __CONFIG_H */
