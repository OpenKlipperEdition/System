cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
SET(CMAKE_SYSTEM_PROCESSOR mipsel)
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_CROSS_COMPILER "mips-linux-gnu-")
SET(CMAKE_C_COMPILER "${CMAKE_CROSS_COMPILER}gcc")
SET(CMAKE_CXX_COMPILER "${CMAKE_CROSS_COMPILER}g++")


if(USE_OPENCV STREQUAL "ON")
  set(SYSROOT_DIR "/data1/home/qianliu/work/opencv/x2000/out/product/zebra/sysroot/")
  set(LOCAL_CXX_FLAGS "${LOCAL_CXX_FLAGS} --sysroot=${SYSROOT_DIR} -lopencv_core -lopencv_imgproc -lopencv_imgcodecs -ldl")
  set(LOCAL_CXX_FLAGS "${LOCAL_CXX_FLAGS} -I=/usr/include/opencv4")
endif()

#SET(MIPS_ENABLE "ON")
##############################################
