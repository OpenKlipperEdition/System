project(CameraStereo LANGUAGES C CXX)
cmake_minimum_required(VERSION 2.8.10)

if (MSA_ENABLE)
   SET(LOCAL_C_FLAGS "${LOCAL_C_FLAGS} -mmsa -flax-vector-conversions -D__MSA__")
   SET(LOCAL_CXX_FLAGS "${LOCAL_CXX_FLAGS} -mmsa -flax-vector-conversions -D__MSA__")
endif()

if (OPENCV_ENABLE)
set (LOCAL_CXX_FLAGS "${LOCAL_CXX_FLAGS} -DUSE_OPENCV=1 -I=/usr/include/opencv4 -I=/usr/include")
set (LOCAL_C_FLAGS "${LOCAL_C_FLAGS} -DUSE_OPENCV=1 -I=/usr/include/opencv4 -I=/usr/include")

if (OPENCV_GUI)
  link_libraries(opencv_highgui)
  set (LOCAL_CXX_FLAGS "${LOCAL_CXX_FLAGS} -DOPENCV_GUI=1")
  set (LOCAL_C_FLAGS "${LOCAL_C_FLAGS} -DOPENCV_GUI=1")
endif()

link_libraries( opencv_core opencv_imgcodecs
     opencv_imgproc opencv_calib3d opencv_features2d opencv_flann)

else()
   set (LOCAL_CXX_FLAGS "${LOCAL_CXX_FLAGS} -DUSE_OPENCV=0")
   set (LOCAL_C_FLAGS "${LOCAL_C_FLAGS} -DUSE_OPENCV=0")
endif()

if (MSA_ENABLE)
  SET(LOCAL_C_FLAGS "${LOCAL_C_FLAGS} -mmsa -flax-vector-conversions -D__MSA__")
  SET(LOCAL_CXX_FLAGS "${LOCAL_CXX_FLAGS} -mmsa -flax-vector-conversions -D__MSA__")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -fPIC -Wl,-z,defs -Wno-shadow -Wno-effc++ -Wno-unused-result -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare -Wno-unused-but-set-variable -Wno-unused-value  ${LOCAL_CXX_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fPIC -Wl,-z,defs -O3  ${LOCAL_C_FLAGS}")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")

include_directories(
	base/include
	base/stb
	base/yaml-cpp/include)

if (MSA_ENABLE)
#  add_subdirectory(optimize)
#  link_libraries(pthread)
endif()

add_subdirectory(examples)
add_subdirectory(base/stereo)
add_subdirectory(base/yaml-cpp)
add_subdirectory(base/include)
add_subdirectory(base/calibrate)
#usage
# build for x86; should set opencv prefix through LOCAL_OPENCV_PREFIX
#   cmake -DLINUX_TARGET_ARCH=x86 -DCMAKE_TOOLCHAIN_FILE=../GNUlinux_config.cmake ..
# enable opencv support
#   cmake -DLINUX_TARGET_ARCH=x86 -DCMAKE_TOOLCHAIN_FILE=../GNUlinux_config.cmake -DOPENCV_ENABLE=ON -DLOCAL_OPENCV_PREFIX=</opencv/install/path> ..
#
# build for mips
#   cmake -DCMAKE_TOOLCHAIN_FILE=../GNUlinux_config.cmake ..
#
# build for x2000
#   cmake -DCMAKE_TOOLCHAIN_FILE=../GNUlinux_config.cmake -DMSA_ENABLE=ON ..
#   cmake -DCMAKE_TOOLCHAIN_FILE=../GNUlinux_config.cmake -DMSA_ENABLE=ON -DOPENCV_ENABLE=ON -DLOCAL_OPENCV_PREFIX=</opencv/install/path> ..
