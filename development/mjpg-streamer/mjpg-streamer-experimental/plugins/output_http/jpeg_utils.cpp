
#include <linux/types.h>          /* for videodev2.h */
#include <linux/videodev2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jpeglib.h>
#include "stereo.hpp"
int compress_yuv_to_jpeg(unsigned char **outbuffer, unsigned char *inbuffer,
			 int width, int height, int fmt, int quality)
{
    struct jpeg_compress_struct jcs;
    struct jpeg_error_mgr jerr;
    unsigned char* tmpbuffer = NULL; //tmp buffer
    int row_stride;

    JSAMPROW row_pointer[1];
#if JPEG_LIB_VERSION >= 90
    size_t outSize = 0;
#else
    unsigned long outSize = 0;
#endif
    jcs.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&jcs);
    jpeg_mem_dest(&jcs, &tmpbuffer, &outSize);

    jcs.image_width = width;
    jcs.image_height = height;
    jcs.input_components = 1;
    if (fmt == V4L2_PIX_FMT_Y4) {
	jcs.in_color_space = JCS_GRAYSCALE;
	row_stride = jcs.image_width;
    } else {
	OPRINT("**Error unsupport compress tyep!\n");
	return 0;
    }

    jpeg_set_defaults(&jcs);
    jpeg_set_quality(&jcs, quality, TRUE);
    jpeg_start_compress(&jcs, TRUE);

    if (fmt == V4L2_PIX_FMT_Y4) {
	while (jcs.next_scanline < jcs.image_height) {
	    row_pointer[0] = &inbuffer[jcs.next_scanline * row_stride];
	    (void) jpeg_write_scanlines(&jcs, row_pointer, 1);
	}
    }

    jpeg_finish_compress(&jcs);

    if(*outbuffer != NULL) {
	free(*outbuffer);
    }
    *outbuffer = (unsigned char *)malloc(outSize + 1 );
    memcpy(*outbuffer, tmpbuffer, outSize);
    outbuffer[outSize + 1] = 0;

    free(tmpbuffer);
    jpeg_destroy_compress(&jcs);
    return outSize;
}
