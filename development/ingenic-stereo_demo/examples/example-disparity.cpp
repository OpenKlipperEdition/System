#include <IMat.hpp>
#include <UtilImageIO.hpp>
#include <CalibrateIO.hpp>
#include <StereoDisparity.hpp>
#include <iostream>
#include <ImgWarp.hpp>
#include "./rectify.hpp"

#if USE_OPENCV
#ifdef OPENCV_GUI
#include "opencv2/highgui.hpp"
#endif
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

using namespace cv;
#endif

using namespace JzStereo;
using namespace std;
#include <test.hpp>

#define out_put(filename, stride, height, esize, srcp)	\
  do{							\
    FILE *fp = fopen(filename, "wb");			\
    if (fp == NULL)					\
      {							\
	printf("** Error: open %s fail\n", filename);	\
	exit(-1);					\
      }							\
    fwrite(srcp, esize, stride* height, fp);		\
    fclose(fp);						\
  }while(0)

int calculate_depth(StereoParam &stereoParam, int px, int py, int min, IMat *disparity)
{
  double *Q = stereoParam.getQ();
  double cx = *(Q + 1*3);
  double cy = *(Q + 1*4 + 3);
  double f  = *(Q + 2*4 + 3);
  double Tx = *(Q + 3*4 + 2);
  double sw = *(Q + 3*4 + 3);

  int32_t d = disparity->at<short>(py, px)/16 + min;
  double w = d * Tx + sw;
  double z = f/w;
  double y = (py + cy)/w;
  double x = (px + cx)/w;

#if 0
  printf("Q: cx = %f cy = %f f = %f Tx = %f sw = %f \n", cx, cy, f, Tx, sw);
  printf("[%d, %d]:\n",px, py);
  printf("d = %d, X = %f, Y = %f, Z = %f W = %f\n", d, px + cx, py + cy, f, w);
  printf("x = %f y = %f z = %f \n", x, y, z);
#endif
  printf("\t(%4d %4d [%4d]) --> (%+-7f, %+-7f, %+-7f)\n", px, py, d, x, y, z);

  return 0;
}

/*
 * ./disparity [istogray] [is check depth] [left] [right]
 */

int main(int argc, char *argv[])
{
  /*
   *      load rectify images
   * here get data RGB888, opencv save bgr888
   * IMat test= IMat(rectL.height, rectL.width, CV_8UC3, rectL.data);
   * imwrite("right.jpg", test);
   * out_put("right.rgb", rectL.width * 3, rectL.height, rectL.data);
   */
    
  // IMat originL = UtilImageIO::loadImage("/data1/home/qianliu/work/opencv/examples/mysample/re-write/resource/aloeL.jpg");
  // IMat originR = UtilImageIO::loadImage("/data1/home/qianliu/work/opencv/examples/mysample/re-write/resource/aloeR.jpg");

  string leftImage;
  string rightImage;
  leftImage = "/data1/home/qianliu/work/zbar/boofcv/boofcv/examples/lc.jpg";
  rightImage = "/data1/home/qianliu/work/zbar/boofcv/boofcv/examples/rc.jpg";

  int check_depth = 0;
  int is2gray = 1;
  
  switch ( argc ) {
  case 5:
    leftImage = argv[3];
    rightImage = argv[4];
  case 3:
    check_depth = atoi(argv[2]);
  case 2:
    is2gray = atoi(argv[1]);
    break;
  default:
    break;
  }

  IMat rectL, rectR;
  long long time_remap, time_match;
  GET_TIME(
	   time_remap,
	   {remap_local_impl(leftImage, rightImage, "", "", rectL, rectR, is2gray);};
	   );

  /*
   *   load paramma
   * StereoParam.loadParam
   */
  string yamlFile("/data1/home/qianliu/work/zbar/boofcv/boofcv/examples/stereo-demo.yaml");
  StereoParam stereoParam = CalibrateIO::load(yamlFile);
  StereoAglType algtype = STEREO_SGBM;
  DisparityParam *dispParam;
  int range_disparity, min_disparity;

  if (algtype == STEREO_SGBM)
    {
      SgbmDisparityParam *sgbmParam = new SgbmDisparityParam();
      // default value
      sgbmParam->minDisparity = 0;
      sgbmParam->numDisparities = 120;
      min_disparity = 0;
      range_disparity = sgbmParam->numDisparities;
      sgbmParam->SADWindowSize = 5;
      sgbmParam->preFilterCap = 15;
      sgbmParam->uniquenessRatio = 18;
      sgbmParam->P1 = 40;
      sgbmParam->P2 = 800;
      sgbmParam->speckleWindowSize = 11;
      sgbmParam->speckleRange = 17;
      sgbmParam->disp12MaxDiff = 2;
      sgbmParam->mode = 0;

      // reset to opencv stereo_match's
      sgbmParam->minDisparity = 0;
      sgbmParam->numDisparities = 128;
      min_disparity = 0;
      range_disparity = sgbmParam->numDisparities;
      sgbmParam->SADWindowSize = 5;
      sgbmParam->preFilterCap = 63;
      sgbmParam->uniquenessRatio = 10;
      sgbmParam->P1 = 600;
      sgbmParam->P2 = 2400;
      sgbmParam->speckleWindowSize = 100;
      sgbmParam->speckleRange = 32;
      sgbmParam->disp12MaxDiff = 1;
      sgbmParam->mode = 0;
      dispParam = sgbmParam;
    }
  else
    {
      dispParam = new SadDisparityParam();
    }
    
  // get dendisparity
  IMat *disparity;
  GET_TIME( time_match,
	    {disparity = StereoDisparity::densDisparity (rectL, rectR, * dispParam, STEREO_SGBM);};
	    );

  printf("time_remap %lld ; time_match %lld\n", time_remap, time_match);
#if USE_OPENCV
  // set point
  Mat disp8;
  Mat disp(disparity->rows, disparity->cols, CV_16S, disparity->ptr<short>());

  //  TODO:: Using opencv mediaBlur, remove in future.
  medianBlur(disp, disp, 3);
  disp.convertTo(disp8, IMat_8U, 255/((range_disparity + min_disparity) *16.));
#ifdef OPENCV_GUI
  namedWindow("demo-disparity", 1);
  imshow("demo-disparity", disp8);
  while ((waitKey(1) & 0xff) != 27)
    continue;
#endif
  imwrite("demo-disparity.jpg", disp8);
#else
  IMat disp8;
  disparity->convertTo(disp8, IMat_8U, 255/((range_disparity + min_disparity) *16.));
  out_put("disp8", disp8.stride, disp8.height, disp8.esize, disp8.ptr<unsigned char>());
#endif

  if (check_depth)
    {
      /*
       * For rectify coordinate X axis is cols-index and Y axis is rows-index.
       *      ___________x
       *     |
       *     |
       *     |
       *     |
       *     |
       *   Y
       * So, when get disparity must be row * stride + col;
       */
      do {
	int strx, stry;
	cout << "enter x y(col row): ";
	if (!scanf("%d %d", &strx, &stry))
	  break;

	int x = strx;
	int y = stry;
	if (strx > disparity->cols || stry > disparity->rows)
	  {
	    printf("**Error: out of range (%d %d), please retry!\n",
		   disparity->cols, disparity->rows);
	    continue;
	  }

	calculate_depth (stereoParam, x, y, min_disparity, disparity);
      } while (1);
      // show dendisparity
      // ShowImage
    }
  return 0;
}
