TARGETS=test yuv4mpeg_to_v4l2 ondemandcam yuv420_infiniteloop

CC:=mips-linux-gnu-gcc

.PHONY: all clean

all: $(TARGETS)

ondemandcam: ondemandcam.c
	$(CC) -o ondemandcam ondemandcam.c -lrt -lpthread

clean:
	-rm $(TARGETS)
