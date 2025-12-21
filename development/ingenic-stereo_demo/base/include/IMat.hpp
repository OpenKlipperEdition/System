#ifndef __GRAYU8_H__
#define __GRAYU8_H__
#include <stdint.h>

// 000 channel 00 type 000 depth
namespace JzStereo {

#define IMat_8    0
#define IMat_16   1
#define IMat_32   2
#define IMat_64   3
#define IMat_U    0
#define IMat_S    1
#define IMat_F    2
#define IMat_CN_MAX       3
#define IMat_CN_SHIFT     5
#define IMat_TYPE_SHIFT   3

#define IMat_DEPTH_MAX  (1 << IMat_TYPE_SHIFT)
#define IMat_DEPTH_MASK (IMat_DEPTH_MAX - 1)
#define IMat_TYPE_MASK  (((1 << IMat_CN_SHIFT) - 1) & (~IMat_DEPTH_MASK))
#define IMat_CN_MASK  (((1 << (IMat_CN_SHIFT + IMat_CN_MAX)) - 1) & (~IMat_DEPTH_MASK) & (~IMat_TYPE_MASK))

#define IMat_DEPTH(depth) ((depth) & IMat_DEPTH_MASK)
#define IMat_TYPE(type)  ((type) & IMat_TYPE_MASK)
#define IMat_CN(cn)  ((cn) & IMat_CN_MASK)
#define IMat_MAKETYPE(depth, type, cn)  (IMat_DEPTH(depth) | (IMat_TYPE(type << IMat_TYPE_SHIFT)) | (IMat_CN(cn << IMat_CN_SHIFT)))

#define IMat_8U     IMat_MAKETYPE(IMat_8, IMat_U, 1)
#define IMat_8UC2   IMat_MAKETYPE(IMat_8, IMat_U, 2)
#define IMat_8UC3   IMat_MAKETYPE(IMat_8, IMat_U, 3)
#define IMat_8S	    IMat_MAKETYPE(IMat_8, IMat_S, 1)
#define IMat_8SC2   IMat_MAKETYPE(IMat_8, IMat_S, 2)
#define IMat_8SC3   IMat_MAKETYPE(IMat_8, IMat_S, 3)

#define IMat_16U     IMat_MAKETYPE(IMat_16, IMat_U, 1)
#define IMat_16UC2   IMat_MAKETYPE(IMat_16, IMat_U, 2)
#define IMat_16UC3   IMat_MAKETYPE(IMat_16, IMat_U, 3)
#define IMat_16S     IMat_MAKETYPE(IMat_16, IMat_S, 1)
#define IMat_16SC2   IMat_MAKETYPE(IMat_16, IMat_S, 2)
#define IMat_16SC3   IMat_MAKETYPE(IMat_16, IMat_S, 3)

#define IMat_32U     IMat_MAKETYPE(IMat_32, IMat_U, 1)
#define IMat_32UC2   IMat_MAKETYPE(IMat_32, IMat_U, 2)
#define IMat_32UC3   IMat_MAKETYPE(IMat_32, IMat_U, 3)
#define IMat_32S     IMat_MAKETYPE(IMat_32, IMat_S, 1)
#define IMat_32SC2   IMat_MAKETYPE(IMat_32, IMat_S, 2)
#define IMat_32SC3   IMat_MAKETYPE(IMat_32, IMat_S, 3)
#define IMat_32F     IMat_MAKETYPE(IMat_32, IMat_F, 1)
#define IMat_32FC2   IMat_MAKETYPE(IMat_32, IMat_F, 2)
#define IMat_32FC3   IMat_MAKETYPE(IMat_32, IMat_F, 3)

#define IMat_64F     IMat_MAKETYPE(IMat_64, IMat_F, 1)
#define IMat_64FC2   IMat_MAKETYPE(IMat_64, IMat_F, 2)
#define IMat_64FC3   IMat_MAKETYPE(IMat_64, IMat_F, 3)
#define IMat_INVALID (IMat_64FC3 + 1)

#define  channleOfType(_t) ({ IMat_CN(_t) >> IMat_CN_SHIFT; })

#define  esizeOfType(_t) ({ 1 << IMat_DEPTH(_t); })
#define JZ_Assert( expr ) do { if (!(expr)) abort(); } while (0)

typedef struct Size{
	int width;
	int height;
}Size;

class IMat {
 public:
  void *data;
  int width;
  int height;
  int cols;
  int rows;
  int chal;
  int stride;
  int esize;
  int type;

  IMat();
  IMat(void *_data, int _w, int _h, int _t);
  IMat(int _w, int _h, int _t);
  ~IMat(){};

  template<typename T>  T *ptr() const { return (T *)data; }
  template<typename T>  T *ptr(int row) const
  { return (T *) ((uintptr_t)data + row * stride * esize); }
  int elemSize(){return esize; }
  int channels() const { return chal;};
  bool empty() const { return data == nullptr ? true : false; }
  bool reserveBuffer(int size);
  void release();
  Size size();

  template<typename T>  T& at(int row, int col ) {return *(T *) ((intptr_t)data + row * stride * esize + col * esize);};

  void convertTo(IMat &_dst, int _t, double _alpha = 0, double _beta = 0);
private:
  void invalidIMat();
  void setupIMat(int _w, int _h, int _t);
  void copyTo(IMat &_dst);
};


  //A(N x M)* B(M x K) = C(N x K)
template<typename T> void gemm(int M ,int N, int K, T* A , T* B ,T* C){
	for(int i = 0; i < M; ++i) {
		for(int k = 0; k < K; ++k) {
			T sum = 0;
			for(int j = 0; j < N; ++j) {
				sum += A[i * N + j] * B[j * K + k];
			}
			C[i * K + k] = sum;
		}
	}
}

}

#endif //__GRAYU8_H__
