project(RSA)
cmake_minimum_required(VERSION 2.8.10)

#add_definitions("-DSIMULATOR")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

#SET(CMAKE_C_FLAGS "-EL -march=mips32r2 -mfp64")
#SET(CMAKE_CXX_FLAGS "-EL -march=mips32r2 -mfp64")

link_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/gmplib
	)

##############################################

add_executable(rsa_test main.c gen_key_base.c rsa.c)

SET_TARGET_PROPERTIES("rsa_test" PROPERTIES
        OUTPUT_NAME "rsa_test"
        )

target_link_libraries(rsa_test crypto)
INSTALL(TARGETS rsa_test
RUNTIME DESTINATION bin)

