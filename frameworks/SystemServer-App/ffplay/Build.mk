LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE=ffplay-lib
LOCAL_MODULE_TAGS :=optional
LOCAL_MODULE_GEN_SHARED_FILES := libffplay.so
LOCAL_EXPORT_C_INCLUDE_FILES := include/ffplay.h
LOCAL_MODULE_CONFIG_FILES:= config.log
DEVICE_COMPILER_PREFIX?=mips-linux-gnu

LOCAL_MODULE_CONFIG:= ./configure	\
		--target-os=linux			\
		--arch=mips				\
		--enable-cross-compile			\
		--cross-prefix=$(DEVICE_COMPILER_PREFIX)-		\
		--ranlib=$(DEVICE_COMPILER_PREFIX)-ranlib		\
		--pkg-config=pkg-config \
		--disable-bsfs \
		--disable-mipsdsp			\
		--disable-mipsdspr2			\
		--enable-msa \
		--disable-runtime-cpudetect \
		--disable-avfilter \
		--disable-filters \
		--disable-encoders \
		--disable-hwaccels \
		--disable-dwt \
		--disable-swscale \
		--disable-w32threads     \
		--disable-os2threads     \
		--disable-securetransport \
		--disable-muxers \
		--disable-demuxers \
		--enable-demuxer=aac \
		--enable-demuxer=wav \
		--enable-demuxer=mp3 \
		--enable-demuxer=pcm_s8 \
		--enable-demuxer=sdp \
		--enable-demuxer=h264 \
		--enable-demuxer=mjpeg \
		--disable-indevs \
		--disable-outdevs \
		--disable-decoders \
		--enable-decoder=h264_v4l2m2m \
		--disable-decoder=h264 \
		--enable-decoder=aac \
		--enable-decoder=mp3 \
		--disable-protocols \
		--enable-protocol=rtmp \
		--enable-protocol=file \
		--enable-small \
		--disable-postproc \
		--disable-error-resilience \
		--disable-faan \
		--disable-lsp \
		--disable-ffprobe \
		--disable-doc \
		--enable-v4l2-m2m \
		--enable-swscale \
		--extra-cflags='-fPIC -mfp64' \
		--enable-decoder=pcm_alaw \
		--enable-decoder=pcm_bluray \
		--enable-decoder=pcm_dvd \
		--enable-decoder=pcm_f16le \
		--enable-decoder=pcm_f24le \
		--enable-decoder=pcm_f32be \
		--enable-decoder=pcm_f32le \
		--enable-decoder=pcm_f64le \
		--enable-decoder=pcm_lxf \
		--enable-decoder=pcm_mulaw \
		--enable-decoder=pcm_s16be \
		--enable-decoder=pcm_s16be_planar \
		--enable-decoder=pcm_s16le \
		--enable-decoder=pcm_s24be \
		--enable-decoder=pcm_s24daud \
		--enable-decoder=pcm_s24le \
		--enable-decoder=pcm_s24le_planar \
		--enable-decoder=pcm_s32be \
		--enable-decoder=pcm_s32le \
		--enable-decoder=pcm_s32le_planar \
		--enable-decoder=pcm_s64be \
		--enable-decoder=pcm_s64le \
		--enable-decoder=pcm_s8 \
		--enable-decoder=pcm_s8_planar \
		--enable-decoder=pcm_sga \
		--enable-decoder=pcm_u16be \
		--enable-decoder=pcm_u16le \
		--enable-decoder=pcm_u24be \
		--enable-decoder=pcm_u24le \
		--enable-decoder=pcm_u32be \
		--enable-decoder=pcm_u32le \
		--enable-decoder=pcm_u8 \
		--disable-alsa \
		--disable-stripping \
		--enable-decoder=pcm_vidc
		#--enable-outdev=alsa \
		#--disable-ffmpeg \

# host=$(DEVICE_COMPILER_PREFIX) \
# LDFLAGS=-L$(ABS_DEVICE_SHARED_DIR) --enable-shared
# CFLAGS="-I$(ABS_DEVICE_INCLUDE_DIR) -std=gnu89" \
# LIBS="-ljpeg -lpng -lz"
#LOCAL_MODULE_COMPILE=sed -i 's/!CONFIG_FFPLAY/CONFIG_FFPLAY/g' ffbuild/config.mak;make -j$(MAKE_JLEVEL) EXTRALIBS-ffplay="-Wl,-Bstatic -lSDL2 -Wl,-Bdynamic -L./third_party/lib -ldl -L${ABS_DEVICE_SHARED_DIR} -limpp -lsystemServer"
LOCAL_MODULE_COMPILE=sed -i 's/!CONFIG_FFPLAY/CONFIG_FFPLAY/g' ffbuild/config.mak;make -j$(MAKE_JLEVEL) EXTRALIBS-ffplay="-Wl,-Bstatic -Wl,-Bdynamic -L${ABS_DEVICE_SHARED_DIR} -lSDL2 -ldl -lsystemServer"
LOCAL_MODULE_COMPILE_CLEAN=make distclean

include $(BUILD_THIRDPART)


include $(CLEAR_VARS)
CMAKE_PATH=$(LOCAL_PATH)
LOCAL_MODULE:= ffplayer
LOCAL_MODULE_TAGS := optional
LOCAL_DEPANNER_MODULES := ingenic-systemServer ffplay-lib
#CMAKE_CONF_OPTS := -DCHIP_PLATFORM=$(TARGET_BOARD_PLATFORM)
include $(BUILD_CMAKE_DEVICE)

