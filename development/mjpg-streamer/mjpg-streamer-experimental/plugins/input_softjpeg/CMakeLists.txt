check_include_files(syncframe.h HAVE_SYNCFRAME_H)

MJPG_STREAMER_PLUGIN_OPTION(input_softjpeg "syncframes input plugin"
    ONLYIF HAVE_SYNCFRAME_H)

add_definitions(-DLINUX -D_GNU_SOURCE)
link_libraries(syncframe jpeg)

MJPG_STREAMER_PLUGIN_COMPILE(input_softjpeg input_syncframes.c softjpeg.c)
