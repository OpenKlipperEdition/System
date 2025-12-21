#
#  Copyright 2013-16 ARM Limited and Contributors.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in the
#      documentation and/or other materials provided with the distribution.
#    * Neither the name of ARM Limited nor the
#      names of its contributors may be used to endorse or promote products
#      derived from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY ARM LIMITED AND CONTRIBUTORS "AS IS" AND
#  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
#  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#  DISCLAIMED. IN NO EVENT SHALL ARM LIMITED AND CONTRIBUTORS BE LIABLE FOR ANY
#  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
#  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
#  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

#  Usage:
#   $ mkdir build && cd build
#   $ cmake -DLINUX_TARGET_ARCH=x86 -DCMAKE_TOOLCHAIN_FILE=../GNUlinux_config.cmake ..
#   $ make
#

if(LINUX_TARGET_ARCH STREQUAL "x86")
	set(CMAKE_C_COMPILER gcc)
	set(CMAKE_CXX_COMPILER g++)
	set(CMAKE_ASM_COMPILER as)
	find_program(CMAKE_AR NAMES "ar")
	find_program(CMAKE_RANLIB NAMES "ranlib")

	# link_libraries(opencv_videoio opencv_core opencv_imgcodecs opencv_highgui
	# 	opencv_imgproc opencv_calib3d opencv_features2d opencv_flann)

	# need set LOCAL_OPENCV_PREFIX to use opencv
	# set(LOCAL_OPENCV_PREFIX "")
	include_directories(${LOCAL_OPENCV_PREFIX}/include/opencv4)
	include_directories(${LOCAL_OPENCV_PREFIX}/include/)
	LINK_DIRECTORIES(${LOCAL_OPENCV_PREFIX}/lib)

	SET(X86_ENABLE ON)
	set(TARGET_ARCH "x86")
else()

	set(CMAKE_C_COMPILER mips-linux-gnu-gcc)
	set(CMAKE_CXX_COMPILER mips-linux-gnu-g++)
	set(CMAKE_ASM_COMPILER mips-linux-gnu-as)
	find_program(CMAKE_AR NAMES "mips-linux-gnu-ar")
	find_program(CMAKE_RANLIB NAMES "mips-linux-gnu-ranlib")

        # need set LOCAL_OPENCV_PREFIX for use opencv
	# set(LOCAL_OPENCV_PREFIX "")
	if (LOCAL_OPENCV_PREFIX)
	   include_directories(${LOCAL_OPENCV_PREFIX}/usr/include/opencv4)
	   LINK_DIRECTORIES(${LOCAL_OPENCV_PREFIX}/lib)
	   SET(LOCAL_C_FLAGS   "--sysroot=${LOCAL_OPENCV_PREFIX}")
	   SET(LOCAL_CXX_FLAGS "--sysroot=${LOCAL_OPENCV_PREFIX}")
	endif()
        set(TARGET_ARCH "mips")
	SET(LOCAL_C_FLAGS   "${LOCAL_C_FLAGS} -EL  -march=mips32r2")
	SET(LOCAL_CXX_FLAGS "${LOCAL_CXX_FLAGS} -EL  -march=mips32r2")
endif()

if(LIBTYPE STREQUAL "static")
	set(STATIC_FLAG "--static")
endif()

#option(KERNEL "set default kernel version 3.0.8" ON)

mark_as_advanced(CMAKE_AR)
mark_as_advanced(CMAKE_RANLIB)
