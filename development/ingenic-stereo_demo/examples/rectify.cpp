#include <IMat.hpp>
#include <UtilImageIO.hpp>
#include <CalibrateIO.hpp>
#include <StereoDisparity.hpp>
#include <iostream>
#include <ImgWarp.hpp>
#include "undistort.hpp"

using namespace JzStereo;
using namespace std;

#if USE_OPENCV
#ifdef OPENCV_GUI
#include "opencv2/highgui.hpp"
#endif
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

using namespace cv;
#endif

#include "./rectify.hpp"
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


#if USE_OPENCV
void check_rectify(int width, int height, Mat imageL_rect, Mat imageR_rect)
{
  Mat canvas;
  double sf;
  int w, h;

  sf = 600./MAX(width, height);
  w = cvRound(width * sf);
  h = cvRound(height * sf);
  canvas.create(h, w * 2, CV_8UC3);
#ifdef OPENCV_GUI
  namedWindow("left", 1);
  namedWindow("right", 1);
#endif

#if 0
  {
    Mat tr, tl;
    cvtColor(imageL_rect, tl, COLOR_GRAY2BGR);
    cvtColor(imageR_rect, tr, COLOR_GRAY2BGR);
    imageL_rect = tl;
    imageR_rect = tr;
  }
#endif
  Mat canvasPart_1 = canvas(Rect(0, 0, w, h));
  Mat canvasPart_2 = canvas(Rect(w, 0, w, h));
  resize(imageL_rect, canvasPart_1, canvasPart_1.size(), 0, 0, INTER_AREA);
  resize(imageR_rect, canvasPart_2, canvasPart_2.size(), 0, 0, INTER_AREA);

#ifdef OPENCV_GUI
  imshow("left", canvasPart_1);
  imshow("right", canvasPart_2);
#endif

  for (int j = 0; j < canvas.rows; j += 16)
    line (canvas, Point(0,j), Point(canvas.cols, j), Scalar(0, 255, 0), 1, 8);

#ifdef OPENCV_GUI
  namedWindow("rectified", 1);
  imshow("rectified", canvas);

  while((char)waitKey() != 27);
  destroyAllWindows();
#endif
}

void rmap_from_file(IMat &ImapL0, IMat &ImapL1, IMat &ImapR0, IMat &ImapR1)
{
  Mat mapL0, mapL1, mapR0, mapR1;

  string mapfileL("/data1/home/qianliu/work/opencv/examples/mysample/re-write/build2/left.yaml");
  string mapfileR("/data1/home/qianliu/work/opencv/examples/mysample/re-write/build2/right.yaml");
  FileStorage lmap(mapfileL, FileStorage::READ);
  FileStorage rmap(mapfileR, FileStorage::READ);

  lmap["rmapl0"] >> mapL0;
  lmap["rmapl1"] >> mapL1;
  rmap["rmapr0"] >> mapR0;
  rmap["rmapr1"] >> mapR1;

  ImapL0 = IMat(mapL0.ptr<short>(), mapL0.cols, mapL0.rows, IMat_16SC2);
  ImapR0 = IMat(mapR0.ptr<short>(), mapR0.cols, mapR0.rows, IMat_16SC2);
  ImapL1 = IMat(mapL1.ptr<uint16_t>(), mapL1.cols, mapL1.rows, IMat_16U);
  ImapR1 = IMat(mapR1.ptr<uint16_t>(), mapR1.cols, mapR1.rows, IMat_16U);
}

void remap_standard_opencv(const string &leftImage, const string &rightImage,
			   const string &mapfileL, const string &mapfileR,
			   Mat &imageL_rect, Mat &imageR_rect,
			   int is2gray, int check)
{
  Mat imageL, imageR;
  IMat originL = UtilImageIO::loadImage(leftImage);
  IMat originR = UtilImageIO::loadImage(rightImage);

  imageL = Mat(originL.rows, originL.cols, CV_8UC3, originL.ptr<uint8_t>());
  imageR = Mat(originR.rows, originR.cols, CV_8UC3, originR.ptr<uint8_t>());

  Mat img1, img2;
  Mat mapL0, mapL1, mapR0, mapR1;
  FileStorage lmap(mapfileL, FileStorage::READ);
  FileStorage rmap(mapfileR, FileStorage::READ);
  lmap["rmapl0"] >> mapL0;
  lmap["rmapl1"] >> mapL1;
  rmap["rmapr0"] >> mapR0;
  rmap["rmapr1"] >> mapR1;

  // cvtColor(imageL, img1, COLOR_RGB2GRAY);
  // cvtColor(imageR, img2, COLOR_RGB2GRAY);
  remap(imageL, imageL_rect, mapL0, mapL1, INTER_LINEAR);
  remap(imageR, imageR_rect, mapR0, mapR1, INTER_LINEAR);
  if (check)
    {
      int height = imageL_rect.rows, width = imageL_rect.cols;
      check_rectify(width, height, imageL_rect, imageR_rect);
    }
}
#endif

#include "./mapdata.cpp"
// extern int rmap_rows;
// extern int rmap_cols;
// extern int rmap0_type;
// extern int rmap1_type;
// extern short *rmapl0, *rmapr0;
// extern unsigned short *rmapl1, *rmapr1;

void rmap_from_data(IMat &ImapL0, IMat &ImapL1, IMat &ImapR0, IMat &ImapR1)
{
  ImapL0 = IMat(rmapl0, rmap_cols, rmap_rows, rmap0_type);
  ImapR0 = IMat(rmapr0, rmap_cols, rmap_rows, rmap0_type);
  ImapL1 = IMat(rmapl1, rmap_cols, rmap_rows, rmap1_type);
  ImapR1 = IMat(rmapr1, rmap_cols, rmap_rows, rmap1_type);
}

void rmap_computation(IMat &ImapL0, IMat &ImapL1, IMat &ImapR0, IMat &ImapR1){

	JzStereo::Size image_size;
	image_size.width = 640;
	image_size.height = 480;

	double ML_data[]={ 5.2567133048907908e+02, 0., 3.1577822205868762e+02, 0., 5.2845636741586054e+02, 2.4993429540701388e+02, 0., 0. ,1.};
	double DL_data[]={-8.2005378863977771e+00 ,-2.1551980629544037e+01, 1.0110792554016132e-02, -5.4923455176559641e-03,   2.1266194554952170e+02, -8.1045484781952375e+00, -2.2829924456293082e+01 ,2.1691863564839039e+02, 0., 0., 0., 0., 0., 0.};
	double RL_data[]={ 0.9995151842930378, 0.002963797615469154, -0.0309937456811798,-0.002935909284355769, 0.999995243434746, 0.0009452748581362283, 0.0309963998607765, -0.0008538217483352574, 0.9995191314747771};
	double PL_data[]={ 528.7892706041026, 0, 331.5748558044434, 0,0, 528.7892706041026, 257.0511722564697, 0, 0, 0, 1, 0};
	IMat ML(ML_data,3,3,IMat_64F);
	IMat DL(DL_data,1,14,IMat_64F);
	IMat RL(RL_data,3,3,IMat_64F);
	IMat PL(PL_data,4,3,IMat_64F);

	double MR_data[]={5.2392837248626654e+02, 0., 3.1042034820106011e+02, 0., 5.2912217379234471e+02, 2.5275698873853690e+02, 0., 0., 1.};
	double DR_data[]={ -6.1144171878645457e+00, 4.0222131745068026e+00, 9.2876547563129196e-03, -1.3082665662260526e-02,   1.7396542724034344e+01, -6.0595337589842959e+00,   3.6200025784967349e+00, 1.8122533602351631e+01, 0., 0., 0., 0., 0., 0.};
	double RR_data[]={ 9.9923273902751109e-01, 1.0416508868602709e-03, -3.9151605573820546e-02, -1.0796355138485100e-03,   9.9999896683273781e-01 , -9.4906301922910810e-04,  3.9150576531327724e-02, 9.9060430401571925e-04, 9.9923283125624807e-01};
	double PR_data[]={5.6807572277834424e+02, 0. ,3.3157485580444336e+02, -2.0302953046337807e+04, 0., 5.6807572277834424e+02, 2.5705117225646973e+02, 0., 0., 0., 1., 0.};

	IMat MR(MR_data,3,3,IMat_64F);
	IMat DR(DR_data,1,14,IMat_64F);
	IMat RR(RR_data,3,3,IMat_64F);
	IMat PR(PR_data,4,3,IMat_64F);
	initUndistortRectifyMap(ML, DL, RL, PL, image_size, IMat_16SC2, ImapL0, ImapL1);
	initUndistortRectifyMap(MR, DR, RR, PR, image_size, IMat_16SC2, ImapR0, ImapR1);
}


void remap_local_impl(const string &leftImage, const string &rightImage,
		      const string &mapfileL, const string &mapfileR,
		      IMat &L_rect, IMat &R_rect, int is2gray,
		      int check)
{
   // Get image as R G B opencv imread stored as B G R
  IMat ImapL0, ImapL1, ImapR0, ImapR1;
  IMat originL, originR;

  if (is2gray)
  {
    IMat img1 = UtilImageIO::loadImage(leftImage);
    IMat img2 = UtilImageIO::loadImage(rightImage);
    originL = UtilImageIO::rgbResize2Gray(img1, img1.width, img1.height);
    originR = UtilImageIO::rgbResize2Gray(img2, img2.width, img2.height);
  } else {
    originL = UtilImageIO::loadImage(leftImage);
    originR = UtilImageIO::loadImage(rightImage);
  }

  //rmap_from_data(ImapL0, ImapL1, ImapR0, ImapR1);
  rmap_computation(ImapL0, ImapL1, ImapR0, ImapR1);
  JzStereo_internal::remap(originL, L_rect, ImapL0, ImapL1, JzStereo_internal::INTER_LINEAR);
  JzStereo_internal::remap(originR, R_rect, ImapR0, ImapR1, JzStereo_internal::INTER_LINEAR);

#if USE_OPENCV
  if (check)
    {
      Mat imageL_rect, imageR_rect;
      imageL_rect = Mat(L_rect.rows, L_rect.cols, L_rect.type == IMat_8UC3 ? CV_8UC3 : CV_8UC1,
			L_rect.ptr<uint8_t>());
      imageR_rect = Mat(R_rect.rows, R_rect.cols, R_rect.type == IMat_8UC3 ? CV_8UC3 : CV_8UC1,
			R_rect.ptr<uint8_t>());
      int height = imageL_rect.rows, width = imageL_rect.cols;
      check_rectify(width, height, imageL_rect, imageR_rect);
    }
#endif
}
