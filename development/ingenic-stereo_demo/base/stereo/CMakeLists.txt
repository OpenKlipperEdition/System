cmake_minimum_required(VERSION 2.6)
project(Stereo LANGUAGES C CXX)

set(STEREO_SRC_FILE
    IMat.cpp
    UtilImageIO.cpp
    CalibrateIO.cpp
    StereoParam.cpp
    StereoDisparity.cpp
    StereoSgbm.cpp
    ImgWarp.cpp
    persistence.cpp
	undistort.cpp
	)

set(STEREO_LIBRARIES "libstereo"  INTERNAL "stereo libraries" PARENT_SCOPE)
include_directories(internal/)

add_library(stereo SHARED ${STEREO_SRC_FILE})
add_library(stereo_static STATIC ${STEREO_SRC_FILE})
if (MSA_ENABLE)
   add_subdirectory(sgbm)
   target_link_libraries(stereo yaml-cpp sgbm-msa)
else()
   target_link_libraries(stereo yaml-cpp)
endif()

install (TARGETS stereo
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static)
