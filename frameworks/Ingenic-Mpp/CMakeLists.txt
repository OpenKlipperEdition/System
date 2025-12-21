CMAKE_MINIMUM_REQUIRED(VERSION 3.8)

PROJECT(ingenic-mpp)

string(SUBSTRING "${CHIP_PLATFORM}" 0 3 CHIP_PLATFORM_PREFIX)

SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_C_COMPILER "mips-linux-gnu-gcc")
SET(CMAKE_CXX_COMPILER "mips-linux-gnu-g++")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${LOCAL_C_FLAGS} -lpthread -lstdc++ -fPIC")
IF (CHIP_PLATFORM STREQUAL "x2000" OR CHIP_PLATFORM STREQUAL "m300" OR CHIP_PLATFORM_PREFIX STREQUAL "x26")
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -EL -march=mips32r2 -mmsa -flax-vector-conversions -D__MSA__")
ENDIF()
SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${LOCAL_CXX_FLAGS} -lpthread -std=c++11 -fPIC -Wno-dev")

OPTION(CHIP_PLATFORM "option for chip platform")
# IMPP主版本号
SET(IMPP_SOVERSION "1")
# IMPP完整版本号
SET(IMPP_VERSION "1.0.3")

# Default configuration, please modify according to actual needs
OPTION(IMPP_BUILD_AUDIO "Build impp audio module" ON)
IF(IMPP_BUILD_AUDIO)
	OPTION(IMPP_BUILD_AUDIO_AEC "Build impp audio aec module" OFF)
ELSE()
    OPTION(IMPP_BUILD_AUDIO_AEC "Build impp audio aec module" OFF)
ENDIF()
OPTION(IMPP_BUILD_CAMERA "Build impp camera module" ON)
OPTION(IMPP_BUILD_DISPLAY "Build impp display module" ON)
OPTION(IMPP_GENERATE_STATIC_LIB "Generate impp static library" OFF)
# It is not recommended to compile libraries for all platforms on the same platform, as different versions of the compilation toolchain can cause problems.
OPTION(IMPP_COMPILE_ALL_CHIP_SERIES "Compile all chip series platform" OFF)
# IMPP_BUILD_EXAMPLE TODO
OPTION(IMPP_BUILD_EXAMPLE "Build impp examples" OFF)

ADD_SUBDIRECTORY(${PROJECT_SOURCE_DIR}/src)

# install common files
install(FILES ${PROJECT_SOURCE_DIR}/src/audio/audio_conf/webrtc_profile.ini DESTINATION /etc)
install(FILES
    ${PROJECT_SOURCE_DIR}/include/icamera.h
    ${PROJECT_SOURCE_DIR}/include/codec.h
    ${PROJECT_SOURCE_DIR}/include/impp.h
    ${PROJECT_SOURCE_DIR}/include/impp_fifo.h
    ${PROJECT_SOURCE_DIR}/include/log.h
    ${PROJECT_SOURCE_DIR}/include/audio.h
    ${PROJECT_SOURCE_DIR}/include/display.h
    ${PROJECT_SOURCE_DIR}/include/rotater.h
    ${PROJECT_SOURCE_DIR}/include/dpu.h
    ${PROJECT_SOURCE_DIR}/include/dma-buf.h
    ${PROJECT_SOURCE_DIR}/include/dma-heap.h
    ${PROJECT_SOURCE_DIR}/include/dmabuf_allocator.h
    ${PROJECT_SOURCE_DIR}/src/common/codec_common.h
    ${PROJECT_SOURCE_DIR}/test/media-example/rtsp/rtsp_server.h
    ${PROJECT_SOURCE_DIR}/test/media-example/mp4v2/mp4Encoder.h
    ${PROJECT_SOURCE_DIR}/test/codec-example/bs.h
    DESTINATION include)
install(FILES
    ${PROJECT_SOURCE_DIR}/test/media-example/rtsp/librtsp.so
    ${PROJECT_SOURCE_DIR}/test/media-example/mp4v2/libmp4v2.a
    DESTINATION /usr/lib)

# install files from different platforms
IF(CHIP_PLATFORM STREQUAL "x2000" OR CHIP_PLATFORM STREQUAL "m300")
	install(FILES ${PROJECT_SOURCE_DIR}/lib/${CHIP_PLATFORM}/libimpp.so DESTINATION /usr/lib)
	install(FILES ${PROJECT_SOURCE_DIR}/lib/${CHIP_PLATFORM}/libimpp.so.${IMPP_SOVERSION} DESTINATION /usr/lib)
	install(FILES ${PROJECT_SOURCE_DIR}/lib/${CHIP_PLATFORM}/libimpp.so.${IMPP_VERSION} DESTINATION /usr/lib)
    install(FILES ${PROJECT_SOURCE_DIR}/src/common/convert.h DESTINATION include)
	install(FILES ${PROJECT_SOURCE_DIR}/src/x2000/h264e-nl-server/S91h264e DESTINATION /etc/init.d PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ)
	install(FILES ${PROJECT_SOURCE_DIR}/src/x2000/h264e-nl-server/h264e-nl-server DESTINATION /usr/bin/)
    IF(IMPP_GENERATE_STATIC_LIB)
        install(FILES ${PROJECT_SOURCE_DIR}/lib/${CHIP_PLATFORM}/libimpp.a DESTINATION /usr/lib)
    ENDIF()
ELSEIF(CHIP_PLATFORM STREQUAL "x2500")
    install(FILES
        ${PROJECT_SOURCE_DIR}/include/ipu.h
        ${PROJECT_SOURCE_DIR}/include/drawbox.h
        DESTINATION include)
    install(FILES ${PROJECT_SOURCE_DIR}/lib/x2500/libimpp.so DESTINATION /usr/lib)
    install(FILES ${PROJECT_SOURCE_DIR}/lib/x2500/libimpp.so.${IMPP_SOVERSION} DESTINATION /usr/lib)
    install(FILES ${PROJECT_SOURCE_DIR}/lib/x2500/libimpp.so.${IMPP_VERSION} DESTINATION /usr/lib)
    IF(IMPP_GENERATE_STATIC_LIB)
        install(FILES ${PROJECT_SOURCE_DIR}/lib/x2500/libimpp.a DESTINATION /usr/lib)
    ENDIF()
ELSEIF(CHIP_PLATFORM_PREFIX STREQUAL "x26")         # x26xx series
    install(FILES ${PROJECT_SOURCE_DIR}/lib/x26xx/libimpp.so DESTINATION /usr/lib)
    install(FILES ${PROJECT_SOURCE_DIR}/lib/x26xx/libimpp.so.${IMPP_SOVERSION} DESTINATION /usr/lib)
    install(FILES ${PROJECT_SOURCE_DIR}/lib/x26xx/libimpp.so.${IMPP_VERSION} DESTINATION /usr/lib)
    install(FILES ${PROJECT_SOURCE_DIR}/src/common/convert.h DESTINATION include)
    IF(IMPP_GENERATE_STATIC_LIB)
        install(FILES ${PROJECT_SOURCE_DIR}/lib/x26xx/libimpp.a DESTINATION /usr/lib)
    ENDIF()
ELSE()
    MESSAGE(FATAL_ERROR "The ${CHIP_PLATFORM} is not support!!!")
ENDIF()

IF(IMPP_BUILD_EXAMPLE)
    ADD_SUBDIRECTORY(test)
ENDIF()
