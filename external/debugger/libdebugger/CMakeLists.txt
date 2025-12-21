set(DEBUGGER debugger)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__STDC_FORMAT_MACROS -DHAVE_SYS_SOCKET_H")

set(DEBUGGER_SOURCE_FILES
  "debugger.cpp"
  )

add_library(${DEBUGGER} SHARED ${DEBUGGER_SOURCE_FILES})

add_dependencies(${DEBUGGER} log)
target_link_libraries(
  ${DEBUGGER}
  log
)
install(TARGETS ${DEBUGGER} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
add_executable(test-debugger test.cpp)
#target_link_libraries(test-debugger debugger)
install(TARGETS test-debugger DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
