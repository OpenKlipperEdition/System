SADC_TEST_SRCS=$(SADC_MODULE_SRCS) ./sadc_test.c
SADC_TEST_OBJS=$(patsubst %.c,%.o,$(filter %.c,$(SADC_TEST_SRCS)))
SADC_TEST_TARGET=sadc_test
OBJS+=$(SADC_TEST_OBJS)
TARGETS+=$(SADC_TEST_TARGET)


include ../Makefile


$(SADC_TEST_TARGET):$(SADC_TEST_OBJS)
	$(CC) $(LIBS) $^ -o $@
