#ifndef __TEST_HPP__
#define __TEST_HPP__

#include <sys/time.h>
static long int GetMicrosecondCount()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec*1000000 + tv.tv_usec;
}
#undef GET_TIME

#define GET_TIME(time, code) {		\
    (time) = GetMicrosecondCount();			\
    code;						\
    (time) = GetMicrosecondCount() - (time);		\
  }

#endif
