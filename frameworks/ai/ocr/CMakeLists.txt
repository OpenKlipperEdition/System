cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(customer_ocr)

set(SOURCE_FILES src/feature.cpp
	  src/decode.cpp
	  src/maxpool2d_permute.cpp
	  src/permute.cpp
  )

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${LOCAL_C_FLAGS} -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LOCAL_CXX_FLAGS} -O2 -std=c++14")

set(LOCAL_C_FLAGS "${LOCAL_C_FLAGS} -mmsa -DMSA_ENABLE=1 -flax-vector-conversions")
set(LOCAL_CXX_FLAGS "${LOCAL_CXX_FLAGS} -mmsa -DMSA_ENABLE=1 -flax-vector-conversions")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${LOCAL_C_FLAGS} -march=xburst2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LOCAL_CXX_FLAGS} -march=xburst2")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -march=xburst2")

set(SOURCE_FILES ${SOURCE_FILES}
    src/bidirectionalLSTM_f16_msa.cpp
    src/bidirectionalLSTM_fixpoint_msa.cpp
    )
set(SOURCE_FILES ${SOURCE_FILES}
    src/linear_f16_msa.cpp
    src/linear_fixpoint_msa.cpp
    )
set(SOURCE_FILES ${SOURCE_FILES}
    src/groupconv2d_relu_permute_msa.cpp
    src/groupconv2d_norm_relu_permute_msa.cpp)
set(SOURCE_FILES ${SOURCE_FILES}
    src/conv2d_norm_fixpoint_permute_msa.cpp
    src/conv2d_fixpoint_permute_msa.cpp)

message(${SOURCE_FILES})

include_directories(include)
include_directories(src)

add_executable(lstm_v2_tiny ${SOURCE_FILES})
find_package(Threads REQUIRED)

target_link_libraries(lstm_v2_tiny Threads::Threads)
install(FILES models/lite.data DESTINATION /usr/bin)
