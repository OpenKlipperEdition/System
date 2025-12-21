SPI_TEST_SRCS=$(SPI_MODULE_SRCS) ./spi_test.c
SPI_TEST_OBJS=$(patsubst %.c,%.o,$(filter %.c,$(SPI_TEST_SRCS)))
SPI_TEST_TARGET=spi_test
OBJS+=$(SPI_TEST_OBJS)
TARGETS+=$(SPI_TEST_TARGET)


include ../Makefile


$(SPI_TEST_TARGET):$(SPI_TEST_OBJS)
	$(CC) $(LIBS) $^ -o $@


