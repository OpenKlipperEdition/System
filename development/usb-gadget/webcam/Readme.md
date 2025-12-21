## uvc-gadget

## How to use

	Usage: ./webcam_gadget [options]
	Available options are
		-b             Use bulk mode
		-d             Do not use any real V4L2 capture device
		-h             Print this help screen and exit
		-i image       MJPEG image
		-m             Streaming mult for ISOC (b/w 0 and 2)
		-n             Number of Video buffers (b/w 2 and 32)
		-o <IO method> Select UVC IO method:
			0 = MMAP
			1 = USER_PTR
		-r <resolution> Select frame resolution:
			0 = 360p, VGA (640x360)
			1 = 720p, WXGA (1280x720)
		-s <speed>     Select USB bus speed (b/w 0 and 2)
			0 = Full Speed (FS)
			1 = High Speed (HS)
			2 = Super Speed (SS)
		-t             Streaming burst (b/w 0 and 15)
		-u device      UVC Video Output device
		-v device      V4L2 Video Capture device

note:
	Currently x2000 isp only supports the V4L2_PIX_FMT_NV12 format, which is set as the default in the test code.
	and the output format of uvc is set to V4L2_PIX_FMT_YUYV format by default.

1.Ways to use static pictures to simulate cameras:

	example:./webcam_gadget -d -i ttt_1280x720.yuv -r 1 -u /dev/video0	/* uvc video node(/dev/video0) */

2.Ways to use the camera:

	format:./webcam_gadget -u /dev/video<uvc video node #> -v /dev/video<vivid video node #>

	example:./webcam_gadget -u /dev/video0 -v /dev/video1 -r 1


3.The computer takes the linux system as an example

	Video tools: xawtv
	Video tools: cheese webcam booth

	If you use cheese webcam booth, please set the video resolution or photo resolution;

