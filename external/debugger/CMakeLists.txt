cmake_minimum_required(VERSION 2.6)
project(Debugerd LANGUAGES C CXX)
# ASM
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -std=gnu++11 -Wall -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -std=gnu99 -Wall -fPIC")
execute_process(COMMAND bash "-c" "${CMAKE_C_COMPILER} -v 2>&1 | grep 'Target' | awk '{print $2}' | awk -F '-' '{printf(\"%s\",$1)}'" OUTPUT_VARIABLE TARGET_ARCH)
function(set_file_to_c files)
  set(FLAGS
    "-x assembler-with-cpp" )
  foreach(file ${${files}})
    set_property (SOURCE ${file} PROPERTY LANGUAGE C)
    set_source_files_properties(
      ${file} PROPERTIES COMPILE_FLAGS
      ${FLAGS}
      )
  endforeach(file)
endfunction()

include_directories (include)
add_subdirectory(libbacktrace)
add_subdirectory(libunwind)
add_subdirectory(syslog)
add_subdirectory(debuggerd)
add_subdirectory(libdebugger)
