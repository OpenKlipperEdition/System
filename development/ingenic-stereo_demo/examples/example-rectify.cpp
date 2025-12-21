#include <IMat.hpp>
#include <UtilImageIO.hpp>
#include <CalibrateIO.hpp>
#include <StereoDisparity.hpp>
#include <iostream>
#include <ImgWarp.hpp>

using namespace JzStereo;
using namespace std;
#include "./rectify.hpp"

#if USE_OPENCV
#ifdef OPENCV_GUI
#include "opencv2/highgui.hpp"
#endif
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

using namespace cv;
#endif

int main(int argc, char *argv[])
{
  string leftImage;
  string rightImage;
  string mapfileL("/data1/home/qianliu/work/opencv/examples/mysample/re-write/build2/left.yaml");
  string mapfileR("/data1/home/qianliu/work/opencv/examples/mysample/re-write/build2/right.yaml");
  if (argc >=3)
    {
      leftImage = argv[1];
      rightImage = argv[2];
    }
  else
    {
      leftImage = "/data1/home/qianliu/work/zbar/boofcv/boofcv/examples/lc.jpg";
      rightImage = "/data1/home/qianliu/work/zbar/boofcv/boofcv/examples/rc.jpg";
    }

#if USE_OPENCV
  // Mat imageL_rect, imageR_rect;
  // remap_standard_opencv(leftImage, rightImage, mapfileL, mapfileR, imageL_rect, imageR_rect);
#endif
  IMat L_rect, R_rect;

  remap_local_impl(leftImage, rightImage, mapfileL, mapfileR, L_rect, R_rect);
}
