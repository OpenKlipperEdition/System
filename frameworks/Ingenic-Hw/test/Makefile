CC=mips-linux-gnu-gcc
CXX=mips-linux-gnu-g++

LIBS:=-lpthread

CFLAGS=-Wall -g
# IHAL库的头文件
CFLAGS+=-I../../include

# 测试用例目录下的头文件
CFLAGS+=-I.

#判断芯片平台
ifeq ($(TARGET_DEVICE),x2600Ehalley)
	CFLAGS += -DX2600E_HALLEY
else ifeq ($(TARGET_DEVICE),X2600halley)
	CFLAGS += -DX2600_HALLEY
else ifeq ($(TARGET_DEVICE),x2670halley)
	CFLAGS += -DX2670_HALLEY
else ifeq ($(TARGET_DEVICE),X2670Mhare)
	CFLAGS += -DX2670M_HARE
else ifeq ($(TARGET_DEVICE),x2600halley7)
	CFLAGS += -DX2600_HALLEY7
else ifeq ($(TARGET_DEVICE),x2670hare)
	CFLAGS += -DX2670_HARE
else ifeq ($(TARGET_DEVICE),hippo)
	CFLAGS += -DX2500_HIPPO
else ifeq ($(TARGET_DEVICE),halley5)
	CFLAGS += -DX2000_HALLEY5
else ifeq ($(TARGET_DEVICE),halley6)
	CFLAGS += -DX1600_HALLEY6
endif

# Watchdog模块源文件用到的文件
WATCHDOG_MODULE_SRCS:=../../src/watchdog/watchdog.c ../../log/log.c
I2C_MODULE_SRCS:=../../src/i2c/i2c.c ../../log/log.c
PWM_MODULE_SRCS:=../../src/pwm/pwm.c ../../log/log.c
GPIO_MODULE_SRCS:=../../src/gpio/gpio.c ../../log/log.c
SADC_MODULE_SRCS:=../../src/sadc/sadc.c ../../log/log.c
SPI_MODULE_SRCS:=../../src/spi/spi.c ../../log/log.c
UART_MODULE_SRCS:=../../src/uart/uart.c ../../log/log.c
SPI_SLV_MODULE_SRCS:=../../src/spi_slv/spi_slv.c ../../log/log.c
TCU_MODULE_SRCS:=../../src/tcu/tcu.c ../../log/log.c  ../../src/gpio/gpio.c
IIO_SADC_MODULE_SRCS:=../../src/iio_sadc/iio_sadc.c ../../log/log.c

RM=rm -f

.PHONY:all clean



all:$(TARGETS)


clean:
	@$(RM) $(OBJS) $(TARGETS)
