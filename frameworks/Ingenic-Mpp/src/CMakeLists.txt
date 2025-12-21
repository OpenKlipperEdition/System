INCLUDE_DIRECTORIES(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/audio
    ${PROJECT_SOURCE_DIR}/src/audio/audioProcess/aec
    ${PROJECT_SOURCE_DIR}/src/audio/audioProcess/agc
    ${PROJECT_SOURCE_DIR}/src/audio/audioProcess/ns
    ${PROJECT_SOURCE_DIR}/src/audio/audioProcess/hpf
    ${PROJECT_SOURCE_DIR}/src/audio/audioProcess/aec/profiles
    ${PROJECT_SOURCE_DIR}/src/common
    ${PROJECT_SOURCE_DIR}/src/external/webrtc/include
    ${PROJECT_SOURCE_DIR}/src/external/webrtc/include/webrtc_audio_processing
    ${PROJECT_SOURCE_DIR}/src/external/webrtc/include/webrtc_audio_processing/webrtc
    ${PROJECT_SOURCE_DIR}/src/audio/codec
    ${PROJECT_SOURCE_DIR}/src/audio/codec/fdk-aac
    ${PROJECT_SOURCE_DIR}/src/external/speexdsp/speex
    ${PROJECT_SOURCE_DIR}/src/x2500/header
    )

SET(BASE_SRC_FILES
    ${PROJECT_SOURCE_DIR}/src/log/log.c
    ${PROJECT_SOURCE_DIR}/src/base/impp_fifo.c
)

SET(DMABUF_SRC_FILES
    ${PROJECT_SOURCE_DIR}/src/common/dmabuf_allocator.c
)

IF(IMPP_BUILD_AUDIO)
    SET(AUDIO_SRC_FILES
        ${PROJECT_SOURCE_DIR}/src/audio/ai.c
        ${PROJECT_SOURCE_DIR}/src/audio/ao.c
        ${PROJECT_SOURCE_DIR}/src/audio/abuf.c
        ${PROJECT_SOURCE_DIR}/src/audio/audio_enc.c
        ${PROJECT_SOURCE_DIR}/src/audio/audio_dec.c
        ${PROJECT_SOURCE_DIR}/src/audio/audio_resampler.c
        ${PROJECT_SOURCE_DIR}/src/audio/codec/adpcm.c
        ${PROJECT_SOURCE_DIR}/src/audio/codec/g711codec.c
        ${PROJECT_SOURCE_DIR}/src/audio/codec/g726codec.c
        ${PROJECT_SOURCE_DIR}/src/audio/codec/aaccodec.c
    )
ENDIF()

IF(IMPP_BUILD_CAMERA)
    SET(CAMERA_SRC_FILES
        ${PROJECT_SOURCE_DIR}/src/camera/icamera.c
        )
ENDIF()

IF(IMPP_BUILD_DISPLAY)
    SET(DISPLAY_SRC_FILES
        ${PROJECT_SOURCE_DIR}/src/dpu/sample_fb.c
        ${PROJECT_SOURCE_DIR}/src/dpu/dpu.c
        )
ENDIF()

SET(X2000_SRC_FILES
    ${BASE_SRC_FILES}
    ${DMABUF_SRC_FILES}
    ${AUDIO_SRC_FILES}
    ${CAMERA_SRC_FILES}
    ${DISPLAY_SRC_FILES}
    ${PROJECT_SOURCE_DIR}/src/x2000/codec.c
    ${PROJECT_SOURCE_DIR}/src/common/h264dec_v1.c
    ${PROJECT_SOURCE_DIR}/src/common/h264enc_v1.c
    ${PROJECT_SOURCE_DIR}/src/common/jpegd_v1.c
    ${PROJECT_SOURCE_DIR}/src/common/jpege_v1.c
	${PROJECT_SOURCE_DIR}/src/common/convert.c
    ${PROJECT_SOURCE_DIR}/src/x2000/rotater.c
)

SET(X2500_SRC_FILES
    ${BASE_SRC_FILES}
    ${DMABUF_SRC_FILES}
    ${AUDIO_SRC_FILES}
    ${CAMERA_SRC_FILES}
    ${DISPLAY_SRC_FILES}
    ${PROJECT_SOURCE_DIR}/src/x2500/codec.c
    ${PROJECT_SOURCE_DIR}/src/x2500/rotater.c
    ${PROJECT_SOURCE_DIR}/src/x2500/ipu.c
    ${PROJECT_SOURCE_DIR}/src/x2500/drawbox.c
)

# X26xx series
SET(X26XX_SRC_FILES
    ${BASE_SRC_FILES}
    ${DMABUF_SRC_FILES}
    ${AUDIO_SRC_FILES}
    ${CAMERA_SRC_FILES}
    ${DISPLAY_SRC_FILES}
    ${PROJECT_SOURCE_DIR}/src/x2600/codec.c
    ${PROJECT_SOURCE_DIR}/src/common/h264dec_v1.c
    ${PROJECT_SOURCE_DIR}/src/common/jpegd_v2.c
    ${PROJECT_SOURCE_DIR}/src/common/jpege_v2.c
	${PROJECT_SOURCE_DIR}/src/common/convert.c
)

FUNCTION(GENERATE_SHARED_LIB CHIP_SERIES LIB_SRC_FILES)
    SET(PLATFORM_DEFINES)

    IF(IMPP_BUILD_AUDIO)
        SET(AUDIO_LIBRARIES
            ${PROJECT_SOURCE_DIR}/src/audio/codec/fdk-aac/libfdk-aac.a
            ${PROJECT_SOURCE_DIR}/src/external/speexdsp/lib/libspeexdsp.a
        )

        IF (IMPP_BUILD_AUDIO_AEC)
            SET(AUDIO_AEC_LIBRARIES
                ${PROJECT_SOURCE_DIR}/src/audio/audioProcess/lib/libaudioProcess.a
                ${PROJECT_SOURCE_DIR}/src/audio/audioProcess/lib/libwebrtc_audio_processing.a
            )
            SET(PLATFORM_DEFINES ${PLATFORM_DEFINES} -DAEC_SUPPORT)
        ENDIF()
    ENDIF()

    IF(CHIP_SERIES STREQUAL "x2500")
        SET(CODEC_LIBRARIES
            ${PROJECT_SOURCE_DIR}/src/x2500/codec_libs/liballegro_encode.a
            ${PROJECT_SOURCE_DIR}/src/x2500/codec_libs/liballegro_codec.a
        )
        SET(PLATFORM_DEFINES ${PLATFORM_DEFINES} -DX2500)
    ENDIF()

    ADD_LIBRARY(impp_${CHIP_SERIES} SHARED ${LIB_SRC_FILES})
    TARGET_COMPILE_DEFINITIONS(impp_${CHIP_SERIES} PRIVATE ${PLATFORM_DEFINES})

    TARGET_LINK_LIBRARIES(impp_${CHIP_SERIES}
        "-Wl,--whole-archive"
        ${AUDIO_LIBRARIES}
        ${AUDIO_AEC_LIBRARIES}
        "-Wl,--no-whole-archive"
        ${CODEC_LIBRARIES}
		asound
    )

    SET_TARGET_PROPERTIES(impp_${CHIP_SERIES} PROPERTIES
        OUTPUT_NAME "impp"
		VERSION ${IMPP_VERSION}
		SOVERSION ${IMPP_SOVERSION}
        LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib/${CHIP_SERIES}
    )

ENDFUNCTION()

IF(IMPP_GENERATE_STATIC_LIB)
FUNCTION(GENERATE_STATIC_LIB CHIP_SERIES LIB_SRC_FILES DEPENDS_TARGET NAME_TARGET)
    SET(PLATFORM_DEFINES)

    IF(IMPP_BUILD_AUDIO)
        SET(EXTRACT_AUDIO_FILES_CMD1 ar -x ${PROJECT_SOURCE_DIR}/src/audio/libasound.a)
        SET(EXTRACT_AUDIO_FILES_CMD2 ar -x ${PROJECT_SOURCE_DIR}/src/audio/codec/fdk-aac/libfdk-aac.a)
        SET(EXTRACT_AUDIO_FILES_CMD3 ar -x ${PROJECT_SOURCE_DIR}/src/external/speexdsp/lib/libspeexdsp.a)

        IF (IMPP_BUILD_AUDIO_AEC)
            SET(EXTRACT_AUDIO_AEC_FILES_CMD1 ar -x ${PROJECT_SOURCE_DIR}/src/audio/audioProcess/lib/libaudioProcess.a)
            SET(EXTRACT_AUDIO_AEC_FILES_CMD2 ar -x ${PROJECT_SOURCE_DIR}/src/audio/audioProcess/lib/libwebrtc_audio_processing.a)
            SET(PLATFORM_DEFINES ${PLATFORM_DEFINES} -DAEC_SUPPORT)
        ENDIF()
    ENDIF()

    IF(CHIP_SERIES STREQUAL "x2500")
        SET(EXTRACT_CODEC_FILES_CMD1 ar -x ${PROJECT_SOURCE_DIR}/src/x2500/codec_libs/liballegro_codec.a)
        SET(EXTRACT_CODEC_FILES_CMD2 ar -x ${PROJECT_SOURCE_DIR}/src/x2500/codec_libs/liballegro_encode.a)
        SET(PLATFORM_DEFINES ${PLATFORM_DEFINES} -DX2500)
    ENDIF()

    ADD_LIBRARY(${DEPENDS_TARGET} STATIC ${LIB_SRC_FILES})
    TARGET_COMPILE_DEFINITIONS(${DEPENDS_TARGET} PRIVATE ${PLATFORM_DEFINES})

    ADD_CUSTOM_COMMAND(
        OUTPUT libimpp_${CHIP_SERIES}.a
        COMMAND ${EXTRACT_AUDIO_FILES_CMD1}
        COMMAND ${EXTRACT_AUDIO_AEC_FILES_CMD1}
        COMMAND ${EXTRACT_AUDIO_AEC_FILES_CMD2}
        COMMAND ${EXTRACT_AUDIO_FILES_CMD2}
        COMMAND ${EXTRACT_AUDIO_FILES_CMD3}
        COMMAND ${EXTRACT_CODEC_FILES_CMD1}
        COMMAND ${EXTRACT_CODEC_FILES_CMD2}
        COMMAND ar -x $<TARGET_FILE:${DEPENDS_TARGET}>
        COMMAND ar -crs ${PROJECT_SOURCE_DIR}/lib/${CHIP_SERIES}/libimpp.a *.o
        DEPENDS ${DEPENDS_TARGET}
    )

    ADD_CUSTOM_TARGET(${NAME_TARGET} ALL DEPENDS libimpp_${CHIP_SERIES}.a)
ENDFUNCTION()
ENDIF()

############################ Generate Library ############################
IF(IMPP_COMPILE_ALL_CHIP_SERIES)
    GENERATE_SHARED_LIB(m300 "${X2000_SRC_FILES}")
    IF(IMPP_GENERATE_STATIC_LIB)
        GENERATE_STATIC_LIB(m300 "${X2000_SRC_FILES}" mid_m300 MERGE_M300)
    ENDIF()

    GENERATE_SHARED_LIB(x2000 "${X2000_SRC_FILES}")
    IF(IMPP_GENERATE_STATIC_LIB)
        GENERATE_STATIC_LIB(x2000 "${X2000_SRC_FILES}" mid_x2000 MERGE_X2000)
    ENDIF()

    GENERATE_SHARED_LIB(x2500 "${X2500_SRC_FILES}")
    IF(IMPP_GENERATE_STATIC_LIB)
        GENERATE_STATIC_LIB(x2500 "${X2500_SRC_FILES}" mid_x2500 MERGE_X2500)
    ENDIF()

    GENERATE_SHARED_LIB(x26xx "${X26XX_SRC_FILES}")
    IF(IMPP_GENERATE_STATIC_LIB)
        GENERATE_STATIC_LIB(x26xx "${X26XX_SRC_FILES}" mid_x26xx MERGE_X26XX)
    ENDIF()
ELSE()
	IF(CHIP_PLATFORM STREQUAL "m300")
		GENERATE_SHARED_LIB(m300 "${X2000_SRC_FILES}")
		IF(IMPP_GENERATE_STATIC_LIB)
			GENERATE_STATIC_LIB(m300 "${X2000_SRC_FILES}" mid_m300 MERGE_M300)
		ENDIF()
	ELSEIF(CHIP_PLATFORM STREQUAL "x2000")
        GENERATE_SHARED_LIB(x2000 "${X2000_SRC_FILES}")
        IF(IMPP_GENERATE_STATIC_LIB)
            GENERATE_STATIC_LIB(x2000 "${X2000_SRC_FILES}" mid_x2000 MERGE_X2000)
        ENDIF()
    ELSEIF(CHIP_PLATFORM STREQUAL "x2500")
        GENERATE_SHARED_LIB(x2500 "${X2500_SRC_FILES}")
        IF(IMPP_GENERATE_STATIC_LIB)
            GENERATE_STATIC_LIB(x2500 "${X2500_SRC_FILES}" mid_x2500 MERGE_X2500)
        ENDIF()
    ELSEIF(CHIP_PLATFORM_PREFIX STREQUAL "x26")                     # x26xx series
        GENERATE_SHARED_LIB(x26xx "${X26XX_SRC_FILES}")
        IF(IMPP_GENERATE_STATIC_LIB)
            GENERATE_STATIC_LIB(x26xx "${X26XX_SRC_FILES}" mid_x26xx MERGE_X26XX)
        ENDIF()
    ELSE()
        MESSAGE(FATAL_ERROR "The ${CHIP_PLATFORM} is not support!!!")
    ENDIF()
ENDIF()
