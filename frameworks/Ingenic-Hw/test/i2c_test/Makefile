I2C_TEST_SRCS=$(I2C_MODULE_SRCS) ./i2c_test.c
I2C_TEST_OBJS=$(patsubst %.c,%.o,$(filter %.c,$(I2C_TEST_SRCS)))
I2C_TEST_TARGET=i2c_test
OBJS+=$(I2C_TEST_OBJS)
TARGETS+=$(I2C_TEST_TARGET)


include ../Makefile


$(I2C_TEST_TARGET):$(I2C_TEST_OBJS)
	$(CC) $(LIBS) $^ -o $@

