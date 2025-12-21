project(sgbm-msa LANGUAGES C CXX)

if (MSA_ENABLE)
  set(TEST_SRC_FILE sgbm-msa.cpp  ${TEST_SRC_FILE})
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -mmsa")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -mmsa")
endif()

add_library(sgbm-msa STATIC ${TEST_SRC_FILE})


