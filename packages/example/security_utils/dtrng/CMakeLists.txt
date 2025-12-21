project(DTRNG)
cmake_minimum_required(VERSION 2.8.10)

#add_definitions("-DSIMULATOR")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

##############################################


add_executable(dtrng_test app_dtrng.c)


SET_TARGET_PROPERTIES("dtrng_test" PROPERTIES
        OUTPUT_NAME "dtrng_test"
        )

INSTALL(TARGETS dtrng_test
RUNTIME DESTINATION bin)
