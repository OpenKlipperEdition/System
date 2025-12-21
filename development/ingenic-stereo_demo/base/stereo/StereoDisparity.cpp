#include <StereoDisparity.hpp>
namespace JzStereo {
  IMat *StereoDisparity::densDisparity(IMat &rectLeft, IMat &rectRight,
		      DisparityParam &algParam,
		      StereoAglType algtype)
{
  IMat *disp = new IMat(rectLeft.width, rectLeft.height, IMat_16S);

  if (algtype == STEREO_SGBM)
    {
      StereoSgbm alg;
      // maybe need copy??
      alg.params = dynamic_cast<SgbmDisparityParam *>(&algParam);
      alg.compute(rectLeft, rectRight, *disp);
    }
  else if (algtype == STEREO_SAD)
    {
      StereoSad alg;
      // alg.params = &algParam;
      // alg.compute(rectLeft, rectRight, *disp);
    }
  return disp;
}

}
