project(optimize.p2 LANGUAGES C CXX)

set(TEST_SRC_FILE
   forward.cpp main.cpp)

include_directories(inc)


if (MSA_ENABLE)
  set(TEST_SRC_FILE forward-msa.cpp  ${TEST_SRC_FILE})
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -mmsa")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -mmsa")
endif()

if (X86_ENABLE)
  set(TEST_SRC_FILE forward-x86.cpp  ${TEST_SRC_FILE})
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse -msse2 -msse3")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -msse2 -msse3")

endif()

add_executable(sgbm.p2 ${TEST_SRC_FILE})

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

target_link_libraries(sgbm.p2 stereo)

