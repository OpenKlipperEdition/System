project(AES)
cmake_minimum_required(VERSION 2.8.10)

#add_definitions("-DSIMULATOR")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

##############################################


add_executable(aes_test main.c gen_key_base.c aes.c)


SET_TARGET_PROPERTIES("aes_test" PROPERTIES
        OUTPUT_NAME "aes_test"
        )

target_link_libraries(aes_test crypto)
INSTALL(TARGETS aes_test
RUNTIME DESTINATION bin)
