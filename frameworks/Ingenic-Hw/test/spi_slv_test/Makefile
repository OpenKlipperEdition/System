SPI_SLV_TEST_SRCS=$(SPI_SLV_MODULE_SRCS) ./spi_slv_test.c
SPI_SLV_TEST_OBJS=$(patsubst %.c,%.o,$(filter %.c,$(SPI_SLV_TEST_SRCS)))
SPI_SLV_TEST_TARGET=spi_slv_test
OBJS+=$(SPI_SLV_TEST_OBJS)
TARGETS+=$(SPI_SLV_TEST_TARGET)


include ../Makefile


$(SPI_SLV_TEST_TARGET):$(SPI_SLV_TEST_OBJS)
	$(CC) $(LIBS) $^ -o $@


