GPIO_TEST_SRCS=$(GPIO_MODULE_SRCS) ./gpio_test.c
GPIO_TEST_OBJS=$(patsubst %.c,%.o,$(filter %.c,$(GPIO_TEST_SRCS)))
GPIO_TEST_TARGET=gpio_test
OBJS+=$(GPIO_TEST_OBJS)
TARGETS+=$(GPIO_TEST_TARGET)


include ../Makefile


$(GPIO_TEST_TARGET):$(GPIO_TEST_OBJS)
	$(CC) $(LIBS) $^ -o $@

