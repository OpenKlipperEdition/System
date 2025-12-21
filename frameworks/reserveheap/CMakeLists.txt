cmake_minimum_required(VERSION 3.8)
project(reserveheap C)

set(LIB_SOURCE_FILES src/reserveheap.c src/ipcsocket.c)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
include_directories(${PROJECT_SOURCE_DIR}/include)

add_library(reserveheap SHARED ${LIB_SOURCE_FILES})


add_executable(test-export test/export-test.c)
target_link_libraries(test-export reserveheap)

add_executable(test-import test/import-test.c)
target_link_libraries(test-import reserveheap)

add_executable(test-reserveheap test/reserveheap-test.c)
target_link_libraries(test-reserveheap reserveheap)

set(EXPORT_INCLUDE_FILES
${PROJECT_SOURCE_DIR}/include/dma-heap.h
${PROJECT_SOURCE_DIR}/include/dma-buf.h
${PROJECT_SOURCE_DIR}/include/ipcsocket.h
${PROJECT_SOURCE_DIR}/include/reserveheap.h
)

install(TARGETS reserveheap DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")

install(TARGETS test-export DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
install(TARGETS test-import DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
install(TARGETS test-reserveheap  DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")

install(FILES ${EXPORT_INCLUDE_FILES} DESTINATION include)
