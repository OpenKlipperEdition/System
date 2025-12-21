#ifndef __UNDISTORT_H__
#define __UNDISTORT_H__

#include "IMat.hpp"
namespace JzStereo {
void initUndistortRectifyMap( IMat &cameraMatrix, IMat &distCoeffs,
			IMat &matR, IMat &newCameraMatrix,
			Size size, int m1type, IMat &map1, IMat &map2 );
}

#endif //__UNDISTORT_H__
