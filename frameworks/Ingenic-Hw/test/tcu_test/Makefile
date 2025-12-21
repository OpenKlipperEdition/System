TCU_TEST_SRCS=$(TCU_MODULE_SRCS) ./tcu_test.c
TCU_TEST_OBJS=$(patsubst %.c,%.o,$(filter %.c,$(TCU_TEST_SRCS)))
TCU_TEST_TARGET=tcu_test
OBJS+=$(TCU_TEST_OBJS)
TARGETS+=$(TCU_TEST_TARGET)


include ../Makefile


$(TCU_TEST_TARGET):$(TCU_TEST_OBJS)
	$(CC) $(LIBS) $^ -o $@
