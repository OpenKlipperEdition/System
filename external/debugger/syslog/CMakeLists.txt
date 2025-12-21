set(PROJLOG log)
set(SYSLOG_SOURCE_FILES
  "src/log.c"
  )
include_directories (
  ${PROJECT_SOURCE_DIR}/include
)
add_library(${PROJLOG} STATIC
        ${SYSLOG_SOURCE_FILES}
    )


  set(PROJNAME testlog)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -std=c++11")

  add_executable(${PROJNAME} "test/main.cpp")

  target_link_libraries (
	${PROJNAME}
        ${PROJLOG}
		)
 set_target_properties(${PROJNAME} PROPERTIES LINKER_LANGUAGE C)
