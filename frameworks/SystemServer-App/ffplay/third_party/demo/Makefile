CC = mips-linux-gnu-gcc
RM = rm
CP = cp
RF = -rf

CFLAG = -Wall

TOP_PATH = $(shell pwd)

COMM_SRC = ffplay_impp_demo.c

LIBS := -L../lib/impp_x2000 -limpp
LIBS += -L../.. -lffplay
LIBS += -pthread

INCLUDE = -I ./ -I ../include/ \
		  -I ../include/libimpp \
		  -I ../include/SDL2/ \

OUTDIR = $(TOP_PATH)/

TARGET = ffplay_impp_demo

.PHONY : all install clean
all:
	$(CC) $(COMM_SRC) $(MAIN_SRC) -o $(TARGET) $(LIBS) $(INCLUDE) $(CFLAG)

clean:
	$(RM) $(RF) $(COMM_OBJ) $(TARGET)


