project(heads LANGUAGES C CXX)
cmake_minimum_required(VERSION 2.8.10)

install(FILES
	CalibrateIO.hpp
	cvdefs.hpp
	IMat.hpp
	ImgWarp.hpp
	StereoDisparity.hpp
	StereoParam.hpp
	test.hpp
	undistort.hpp
	UtilImageIO.hpp
	calibrate.hpp
	DESTINATION include/stereo)
