CMAKE_MINIMUM_REQUIRED(VERSION 3.8)

PROJECT(LTP-INGENIC)


SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_C_COMPILER "mips-linux-gnu-gcc")
SET(CMAKE_CXX_COMPILER "mips-linux-gnu-g++")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${LOCAL_C_FLAGS} -lpthread -lstdc++ -fPIC")

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)


install(FILES
	${PROJECT_SOURCE_DIR}/include/gpio.h
	${PROJECT_SOURCE_DIR}/include/i2c.h
	${PROJECT_SOURCE_DIR}/include/ihal.h
	${PROJECT_SOURCE_DIR}/include/ihal_fifo.h
	${PROJECT_SOURCE_DIR}/include/ihal_info.h
	${PROJECT_SOURCE_DIR}/include/iio_sadc.h
	${PROJECT_SOURCE_DIR}/include/keybord.h
	${PROJECT_SOURCE_DIR}/include/hw_log.h
	${PROJECT_SOURCE_DIR}/include/pwm.h
	${PROJECT_SOURCE_DIR}/include/rtc.h
	${PROJECT_SOURCE_DIR}/include/sadc.h
	${PROJECT_SOURCE_DIR}/include/spi.h
	${PROJECT_SOURCE_DIR}/include/spi_slv.h
	${PROJECT_SOURCE_DIR}/include/tcu.h
	${PROJECT_SOURCE_DIR}/include/tpc.h
	${PROJECT_SOURCE_DIR}/include/uart.h
	${PROJECT_SOURCE_DIR}/include/watchdog.h
	DESTINATION include
)


ADD_LIBRARY(ingenichw SHARED ${PROJECT_SOURCE_DIR}/src/gpio/gpio.c
			${PROJECT_SOURCE_DIR}/src/i2c/i2c.c
			${PROJECT_SOURCE_DIR}/src/iio_sadc/iio_sadc.c
			${PROJECT_SOURCE_DIR}/src/input/keyboard.c
			${PROJECT_SOURCE_DIR}/src/pwm/pwm.c
			${PROJECT_SOURCE_DIR}/src/rtc/rtc.c
			${PROJECT_SOURCE_DIR}/src/sadc/sadc.c
			${PROJECT_SOURCE_DIR}/src/spi/spi.c
			${PROJECT_SOURCE_DIR}/src/spi_slv/spi_slv.c
			${PROJECT_SOURCE_DIR}/src/tcu/tcu.c
			${PROJECT_SOURCE_DIR}/src/uart/uart.c
			${PROJECT_SOURCE_DIR}/src/watchdog/watchdog.c
			${PROJECT_SOURCE_DIR}/base/ihal_fifo.c
			${PROJECT_SOURCE_DIR}/log/log.c
)

install(TARGETS ingenichw DESTINATION lib)

