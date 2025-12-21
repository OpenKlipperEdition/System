project(SECURITY_TEST)
cmake_minimum_required(VERSION 2.8.10)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

##############################################


add_subdirectory(aes)
add_subdirectory(rsa)
add_subdirectory(hash)
add_subdirectory(dtrng)
