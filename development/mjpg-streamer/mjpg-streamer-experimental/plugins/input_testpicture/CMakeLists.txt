
check_include_files(linux/videodev2.h HAVE_LINUX_VIDEODEV2_H)
check_include_files(testpicture.h HAVE_LINUX_TESTPICTURE_H)

MJPG_STREAMER_PLUGIN_OPTION(input_testpicture "Video 4 Linux input plugin"
                            ONLYIF HAVE_LINUX_VIDEODEV2_H)

add_definitions(-DLINUX -D_GNU_SOURCE)
MJPG_STREAMER_PLUGIN_COMPILE(input_testpicture input_testpicture.c)
