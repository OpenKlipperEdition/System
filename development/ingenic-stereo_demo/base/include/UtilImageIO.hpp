#ifndef __UTILEIMAGEIO_H__
#define __UTILEIMAGEIO_H__
#include "IMat.hpp"
#include <string>

namespace JzStereo {
class UtilImageIO {
public:
  // static IMat loadImage(std::string path, std::string filename)
  // {
  //   IMat image;
  //   return image;
  // };
  static IMat &loadImage(std::string filename);
  static IMat &rgb2Gray (IMat &src);
  static IMat &rgbResize2Gray (IMat &src, int _width, int _height);

  // static IMat saveImage(IMat data, std::string filename)
  // {
  //   IMat image;
  //   return image;
  // };
  UtilImageIO(){};
  ~UtilImageIO(){};
};

}
#endif //__UTILEIMAGEIO_H__ 
