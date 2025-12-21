LOCAL_PATH := $(my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE:= v4l2loopback-ctl
LOCAL_MODULE_TAGS :=optional

LOCAL_SRC_FILES:=utils/v4l2loopback-ctl.c
LOCAL_C_INCLUDES:= .

LOCAL_LDLIBS := -lc -lstdc++

include $(BUILD_EXECUTABLE)

#=================examples.=======================
include $(CLEAR_VARS)
LOCAL_MODULE:= yuv420_infiniteloop
LOCAL_MODULE_TAGS :=optional

LOCAL_SRC_FILES:=examples/yuv420_infiniteloop.c
LOCAL_C_INCLUDES:= .

LOCAL_LDLIBS := -lc -lstdc++

include $(BUILD_EXECUTABLE)

#================================================

include $(CLEAR_VARS)
LOCAL_MODULE:= ondemandcam
LOCAL_MODULE_TAGS :=optional

LOCAL_SRC_FILES:=examples/ondemandcam.c
LOCAL_C_INCLUDES:= .

LOCAL_LDLIBS := -lc -lstdc++ -lpthread

include $(BUILD_EXECUTABLE)

#===============================================
include $(CLEAR_VARS)
LOCAL_MODULE:= yuv4mpeg_to_v4l2
LOCAL_MODULE_TAGS :=optional

LOCAL_SRC_FILES:=examples/yuv4mpeg_to_v4l2.c
LOCAL_C_INCLUDES:= .

LOCAL_LDLIBS := -lc -lstdc++

include $(BUILD_EXECUTABLE)

#===============================================
include $(CLEAR_VARS)
LOCAL_MODULE:= v4l2-lp-dqbuf
LOCAL_MODULE_TAGS :=optional

LOCAL_SRC_FILES:=tests/v4l2-lp-dqbuf.c
LOCAL_C_INCLUDES:= .

LOCAL_LDLIBS := -lc -lstdc++

include $(BUILD_EXECUTABLE)
