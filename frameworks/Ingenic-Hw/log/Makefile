CC = mips-linux-gnu-gcc

LDFLAGS =
CFLAGS = -Wall -fPIC
INC = -I../include
SRCS = log.c

OBJS = $(SRCS:.c=.o)

TARGET = libihal_log.so

.PHONY:all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -shared  -o $@ $^

%.o:%.c
	$(CC) $(CFLAGS) $(INC) $(LDFLAGS) -o $@ -c $<
clean:
	rm -f $(OBJS) $(TARGET)
