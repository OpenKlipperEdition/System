cmake_minimum_required(VERSION 2.8)
#project(ne10_fft C CXX)

include_directories(inc)

set(CMAKE_C_FLAGS "-march=xburst2 -mmsa")
set(CMAKE_CXX_FLAGS "-march=xburst2 -mmsa")
add_library(ne10_rfft STATIC
  src/NE10_fft.c
  src/NE10_fft_generic_float32.c
  src/NE10_rfft_float32.c
  src/NE10_rfft_float32_mxu.c)

add_subdirectory(example)

INSTALL(TARGETS ne10_rfft DESTINATION /usr/lib)
target_link_libraries(ne10_rfft m)
install(FILES
  inc/NE10.h
  inc/NE10_types.h
  inc/NE10_dsp.h
  inc/fft.h
  DESTINATION /usr/include)
