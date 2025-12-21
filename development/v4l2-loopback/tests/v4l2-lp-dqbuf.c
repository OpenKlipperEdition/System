/*
 * Copyright (C)
 *
 * 	qipengzhen <aric.pzqi@ingenic.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/types.h>
#include <unistd.h>


#include <stdlib.h>

#define COUNT 4
#define sysfail(msg) { printf ("%s failed: %s\n", (msg), strerror (errno)); return -1; }

	void
usage(const char*progname)
{
	printf("usage: %s <videodevice>\n", progname);
	exit(1);
}

	int
main (int argc, char **argv)
{
	struct v4l2_format fmt = { 0 };
	struct v4l2_requestbuffers breq = { 0 };
	struct v4l2_buffer bufs[COUNT];
	void *data[COUNT] = { 0 };
	int fd;
	int i;
	int yuv_fd;
	unsigned long long pts = 0;
	unsigned long frame = 0;

	if(argc<2) usage(argv[0]);

	fd = open (argv[1], O_RDWR);
	if (fd < 0)
		sysfail("open");

	yuv_fd = open("480p.yuv", O_RDONLY);

	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	fmt.fmt.pix.width = 640;
	fmt.fmt.pix.height = 480;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;

	if (ioctl (fd, VIDIOC_S_FMT, &fmt) < 0)
		sysfail ("S_FMT");

	breq.count = COUNT;
	breq.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	breq.memory = V4L2_MEMORY_MMAP;

	if (ioctl (fd, VIDIOC_REQBUFS, &breq) < 0)
		sysfail ("REQBUFS");

	memset (bufs, 0, sizeof (bufs));

	for (i = 0; i < breq.count; i++) {
		int p;

		bufs[i].index = i;
		bufs[i].type = breq.type;
		bufs[i].memory = breq.memory;

		if (ioctl (fd, VIDIOC_QUERYBUF, &bufs[i]) < 0)
			sysfail ("QUERYBUF");

		data[i] = mmap (NULL, bufs[i].length, PROT_WRITE, MAP_SHARED, fd, bufs[i].m.offset);
		if (data[i] == MAP_FAILED)
			sysfail ("mmap");

		printf("data[i]: %x bufs[%d].length: %d, bytesused: %d, offset: %d, userptr: %x\n", data[i], i, bufs[i].length, bufs[i].bytesused, bufs[i].m.offset, bufs[i].m.userptr);
	}

	if (ioctl (fd, VIDIOC_STREAMON, &fmt.type) < 0)
		sysfail ("STREAMON");


	while (1) {
		struct v4l2_buffer buf = { 0 };
		int ret = 0;

		buf.type = breq.type;
		buf.memory = breq.memory;

try_again:
		if (ioctl (fd, VIDIOC_DQBUF, &buf) < 0) {
			usleep(10000);
			goto try_again;
		}
		i = buf.index;

		if ((bufs[i].flags & V4L2_BUF_FLAG_QUEUED) == 0) {
			printf ("BUG #2: Driver should not dequeue a buffer that was not intially queued\n");
		}

		bufs[i] = buf;


		ret = read(yuv_fd, data[i], 640*480*3/2);
		if(ret <= 0) {
			lseek(yuv_fd, 0, SEEK_SET);
			ret = read(yuv_fd, data[i], 640*480*3/2);
			printf("re seek ret: %d\n", ret);
		}

		/*TODO:*/
		//bufs[i].timestamp.tv_sec = pts / 1000000;
		//bufs[i].timestamp.tv_usec = pts % 1000000;
		//bufs[i].sequence = frame++;
//		usleep(20000);

		// 25.fps
		// 1000 000 000 / 25.0 = 40000000.0
		// 1000 000 / 25.0 = 40000.0
		//pts += 20000.0;


		//make 25fps stream.
		//usleep(40000);

		if (ioctl (fd, VIDIOC_QBUF, &bufs[i]) < 0)
			sysfail ("QBUF");

		printf ("\t[%d] QUEUED=%d\tDONE=%d\n",
				i, bufs[i].flags & V4L2_BUF_FLAG_QUEUED,
				bufs[i].flags & V4L2_BUF_FLAG_DONE);

		if ((bufs[i].flags & V4L2_BUF_FLAG_QUEUED) == 0) {
			printf ("BUG #1: Driver should set the QUEUED flag before returning from QBUF\n");
			bufs[i].flags |= V4L2_BUF_FLAG_QUEUED;
		}

	}

	return 0;
}
