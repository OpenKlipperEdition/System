UART_TEST_SRCS=$(UART_MODULE_SRCS) ./uart_test.c
UART_TEST_OBJS=$(patsubst %.c,%.o,$(filter %.c,$(UART_TEST_SRCS)))
UART_TEST_TARGET=uart_test
OBJS+=$(UART_TEST_OBJS)
TARGETS+=$(UART_TEST_TARGET)


include ../Makefile


$(UART_TEST_TARGET):$(UART_TEST_OBJS)
	$(CC) $(LIBS) $^ -o $@
