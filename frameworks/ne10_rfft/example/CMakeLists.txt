cmake_minimum_required(VERSION 2.8)
include_directories(inc)

add_executable(capi_example  realfft_capi.c)
add_executable(cxxapi_example  realfft_cxxapi.cpp)

target_link_libraries(capi_example ne10_rfft)
target_link_libraries(cxxapi_example ne10_rfft)
