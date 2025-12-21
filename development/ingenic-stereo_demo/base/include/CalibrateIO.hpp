#ifndef __CALIBRATEIO_H__
#define __CALIBRATEIO_H__

#include "IMat.hpp"
#include "StereoParam.hpp"

class CalibrateIO {
public:
  static std::string MODEL_PINHOLE;
  static std::string MODEL_STEREO;
  static StereoParam &load(std::string paramFile);
  template<typename T>   static T &load(YAML::Node top);

};

#endif //__CALIBRATEIO_H__
