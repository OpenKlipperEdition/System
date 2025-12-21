project(http LANGUAGES C CXX)
include(CheckIncludeFileCXX)

add_feature_option(ENABLE_HTTP_MANAGEMENT "Enable experimental HTTP management option" OFF)


SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-z,defs -O2 -I=/usr/include/opencv4")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-z,defs -std=c++11 -O2 -I=/usr/include/opencv4 -std=c++11")

if (ENABLE_HTTP_MANAGEMENT)
    add_definitions(-DMANAGMENT)
endif (ENABLE_HTTP_MANAGEMENT)

add_definitions(-D_GNU_SOURCE)
MJPG_STREAMER_PLUGIN_OPTION(output_http "HTTP server output plugin")


check_include_file_cxx(opencv2/core.hpp HAVE_OPENCV_H)
MJPG_STREAMER_PLUGIN_OPTION(output_http "output_http  plugin"  ONLYIF HAVE_OPENCV_H)

find_library(JPEG_LIB jpeg)
if (NOT JPEG_LIB)
    add_definitions(-DNO_LIBJPEG)
endif (NOT JPEG_LIB)

if (JPEG_LIB)
    link_libraries(${JPEG_LIB})
endif (JPEG_LIB)

link_libraries(stereo yaml-cpp calibrate opencv_core ${JPEG_LIB})
MJPG_STREAMER_PLUGIN_COMPILE(output_http
  httpd.c
  output_http.c
  stereo.cpp
  cJSON.c
  stereo_calib.cpp
  jpeg_utils.cpp)
