#ifndef __RECTIFY_H__
#define __RECTIFY_H__

#include <IMat.hpp>
#include <UtilImageIO.hpp>
#include <CalibrateIO.hpp>
#include <StereoDisparity.hpp>
#include <iostream>
#include <ImgWarp.hpp>

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
#ifdef OPENVC_GUI
#include "opencv2/highgui.hpp"
#endif
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"

void rmap_from_file(JzStereo::IMat &ImapL0, JzStereo::IMat &ImapL1,
		    JzStereo::IMat &ImapR0, JzStereo::IMat &ImapR1);

void check_rectify(int width, int height, cv::Mat imageL_rect, cv::Mat imageR_rect);
void remap_standard_opencv(const std::string &leftImage, const std::string &rightImage,
			   const std::string &mapfileL, const std::string &mapfileR,
			   cv::Mat &imageL_rect, cv::Mat &imageR_rect,
			   int is2gray = 1, int check = 0);
#endif

void rmap_from_data(JzStereo::IMat &ImapL0, JzStereo::IMat &ImapL1,
		    JzStereo::IMat &ImapR0, JzStereo::IMat &ImapR1);

void remap_local_impl(const std::string &leftImage, const std::string &rightImage,
		      const std::string &mapfileL, const std::string &mapfileR,
		      JzStereo::IMat &L_rect, JzStereo::IMat &R_rect,
		      int is2gray = 1, int check = 0);

#endif
