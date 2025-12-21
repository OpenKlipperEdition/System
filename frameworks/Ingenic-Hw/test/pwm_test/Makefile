PWM_TEST_SRCS=$(PWM_MODULE_SRCS) ./pwm_test.c
PWM_TEST_OBJS=$(patsubst %.c,%.o,$(filter %.c,$(PWM_TEST_SRCS)))
PWM_TEST_TARGET=pwm_test
OBJS+=$(PWM_TEST_OBJS)
TARGETS+=$(PWM_TEST_TARGET)


include ../Makefile


$(PWM_TEST_TARGET):$(PWM_TEST_OBJS)
	$(CC) $(LIBS) $^ -o $@


