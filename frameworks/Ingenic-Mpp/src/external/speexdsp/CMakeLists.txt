SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_C_COMPILER mips-linux-gnu-gcc)
SET(CMAKE_CXX_COMPILER mips-linux-gnu-g++)
add_definitions(-DNONX64)

cmake_minimum_required(VERSION 3.0)
project(libspeexdsp)
add_definitions(-DHAVE_CONFIG_H)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -std=gnu11 -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=gnu++11 -ffunction-sections -fdata-sections")
set(HOME_BASE "..")

# 头文件目录

#include_directories(${HOME_BASE}/speex)
#include_directories(${HOME_BASE}/src)

include_directories(./)
include_directories(speex)
include_directories(src)

# 发现目录下的源文件

#aux_source_directory(./src DIR_SRCS)


SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
add_library(speex  SHARED    src/buffer.c 
    src/fftwrap.c 
    src/filterbank.c 
    src/jitter.c 
    src/kiss_fft.c 
    src/kiss_fftr.c 
    src/mdf.c 
    src/preprocess.c 
    src/resample.c 
    src/scal.c 
    src/smallft.c)

add_library(speexdsp  STATIC   src/buffer.c 
    src/fftwrap.c 
    src/filterbank.c 
    src/jitter.c 
    src/kiss_fft.c 
    src/kiss_fftr.c 
    src/mdf.c 
    src/preprocess.c 
    src/resample.c 
    src/scal.c 
    src/smallft.c)

MESSAGE(STATUS ${PROJECT_SOURCE_DIR})

