#include <IMat.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <saturate_cast.hpp>
#include <iostream>
namespace JzStereo {

  // convert float to _Td
  template <typename _Ts, typename _Td>  static inline void
  cvt_32f(const _Ts *src, _Td *dst, int width, int height, float a, float b)
  {
    int dstep = width, sstep = width;

    for (int i = 0; i < height; i++, src += sstep, dst += dstep)
      for (int j = 0; j < width; j++)
	{
	  dst[j] = internal::saturate_cast<_Td>(src[j] * a + b);
	}
  }

  void IMat::invalidIMat()
  {
    cols = width = 0;
    rows = height = 0;
    chal = 0;
    stride = 0;
    esize = 0;
    type = IMat_INVALID;
  }

  void IMat::setupIMat(int _w, int _h, int _t)
  {
    cols = width = _w;
    rows = height = _h;
    chal = channleOfType(_t);
    stride = width * chal;
    esize = esizeOfType(_t);
    type = _t;
  }

  IMat::IMat()
    :data(nullptr),
     width(0),
     height(0),
     cols(0),
     rows(0),
     chal(0),
     stride(0),
     esize(0),
     type(0)
  {
  }

  IMat::IMat(void *_data, int _w, int _h, int _t)
    :data(_data)
  {
    if (_t < IMat_INVALID)
      setupIMat(_w, _h, _t);
    else
      invalidIMat();
  }

  IMat::IMat(int _w, int _h, int _t)
  {
    if (_t < IMat_INVALID)
      {
		  setupIMat(_w, _h, _t);
		  data = malloc(_w * _h * chal * esize);
      }
    else
      invalidIMat();
  }

  bool IMat::reserveBuffer(int size)
  {
    if (data == nullptr)
      data = malloc(size);
    else
      {
	free(data);
	data = malloc (size);
      }

    if (data == nullptr)
      {
	invalidIMat();
	return false;
      }
    else
      {
	setupIMat(size, 1, IMat_8U);
	return true;
      }
  }


  void IMat::copyTo(IMat &_dst)
  {
    if (_dst.empty())
      _dst = IMat(width, height, type);

    memcpy (_dst.data, data, height * stride * esize);
  }

  void IMat::release()
  {
    setupIMat(0, 0, 0);
    if (!empty())
      free(data);
  }


  void IMat::convertTo(IMat &_dst, int _t, double _alpha, double _beta)
  {

	  if (this->chal != channleOfType(_t))
	  {
		  printf("Different channel to convert\n");
		  abort();
	  }
	  else
	  {
		  if (_t < 0 || _t >= IMat_INVALID)
			  _t = type;
		  else
			  _t = IMat_MAKETYPE(IMat_DEPTH(_t), IMat_TYPE(_t), channels());

		  bool noScale = fabs(_alpha-1) < DBL_EPSILON && fabs(_beta) < DBL_EPSILON;

		  int sdepth = IMat_DEPTH(type), ddepth = IMat_DEPTH(_t);

		  if (sdepth == ddepth && noScale)
		  {
			  copyTo(_dst);
			  return;
		  }
		  _dst.release();
		  _dst = IMat(this->width, this->height, _t);

		  short *sdata = this->ptr<short>();
		  uint8_t *ddata = _dst.ptr<unsigned char>();
		  cvt_32f<short, uint8_t>(sdata, ddata, this->width, this->height, _alpha, _beta);
	  }
  }

}
