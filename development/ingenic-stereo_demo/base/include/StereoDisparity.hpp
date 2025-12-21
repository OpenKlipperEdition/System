#ifndef __STEREODISPARITY_H__
#define __STEREODISPARITY_H__
#include "CalibrateIO.hpp"
/*
 * parameter for disparity algorithm.
 */
namespace JzStereo {
  typedef enum type {
    STEREO_SGBM,
    STEREO_SAD,
  }StereoAglType;

class DisparityParam {
public:
  
  DisparityParam(){};
  virtual ~DisparityParam() = default;

};

class SgbmDisparityParam: public DisparityParam {
public:
  int minDisparity;
  int numDisparities;
  int SADWindowSize;
  int preFilterCap;
  int uniquenessRatio;
  int P1;
  int P2;
  int speckleWindowSize;
  int speckleRange;
  int disp12MaxDiff;
  int mode;
  SgbmDisparityParam(){};
  ~SgbmDisparityParam(){};
};

class SadDisparityParam: public DisparityParam {
public:
  SadDisparityParam(){};
  ~SadDisparityParam(){};
};

/*
*  called interface for get disparity.
*/
class StereoDisparity {
public:
  static IMat *densDisparity(IMat &rectLeft, IMat &rectRight,
		      DisparityParam &algParam,
		      StereoAglType algtype);

  DisparityParam getDisparityParam() { return algParam; };
  StereoDisparity(){};
  ~StereoDisparity(){};
public:
  DisparityParam algParam;
  StereoParam cameraParam;
};

/*
 * stereo disparity algorithm
 */
class stereoIMatcher {
public:
  stereoIMatcher(){};
  ~stereoIMatcher(){};
};
class StereoSgbm : public stereoIMatcher {
public:
  enum { DISP_SHIFT = 4,
	 DISP_SCALE = (1 << DISP_SHIFT)
  };
  enum
    {
      MODE_SGBM = 0,
      MODE_HH   = 1,
      MODE_SGBM_3WAY = 2,
      MODE_HH4  = 3
    };

  StereoSgbm(){};
  void  compute(IMat& imgl, IMat& imgR, IMat &disp);
  ~StereoSgbm(){};
  SgbmDisparityParam *params;
};

class StereoSad : public stereoIMatcher{
public:
  StereoSad(){};
  ~StereoSad(){};
  SadDisparityParam *params;
};
}
#endif //__STEREODISPARITY_H__
