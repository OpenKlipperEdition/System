set(PROJLIBNAME backtrace)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__STDC_FORMAT_MACROS")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -D__STDC_FORMAT_MACROS")

set(BACKTRACE_SOURCE_FILES
  "BacktraceImpl.cpp"
  "BacktraceMap.cpp"
  "BacktraceThread.cpp"
  "UnwindCurrent.cpp"
  "UnwindMap.cpp"
  "UnwindPtrace.cpp"
  "thread_utils.c"
  )
include_directories (
  ${PROJECT_SOURCE_DIR}/libunwind/include
  ${PROJECT_SOURCE_DIR}/include

  )

add_library(${PROJLIBNAME} STATIC
        ${BACKTRACE_SOURCE_FILES}
    )
