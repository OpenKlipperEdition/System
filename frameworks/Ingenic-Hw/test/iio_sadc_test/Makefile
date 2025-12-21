IIO_SADC_TEST_SRCS=$(IIO_SADC_MODULE_SRCS) ./iio_sadc_test.c
IIO_SADC_TEST_OBJS=$(patsubst %.c,%.o,$(filter %.c,$(IIO_SADC_TEST_SRCS)))
IIO_SADC_TEST_TARGET=iio_sadc_test
OBJS+=$(IIO_SADC_TEST_OBJS)
TARGETS+=$(IIO_SADC_TEST_TARGET)


include ../Makefile


$(IIO_SADC_TEST_TARGET):$(IIO_SADC_TEST_OBJS)
	$(CC) $(LIBS) $^ -o $@