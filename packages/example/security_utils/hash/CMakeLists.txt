project(HASH)
cmake_minimum_required(VERSION 2.8.10)

#add_definitions("-DSIMULATOR")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

##############################################


add_executable(hash_test main.c hash_openssl.c hash.c)


SET_TARGET_PROPERTIES("hash_test" PROPERTIES
        OUTPUT_NAME "hash_test"
        )

target_link_libraries(hash_test crypto)
INSTALL(TARGETS hash_test
RUNTIME DESTINATION bin)

