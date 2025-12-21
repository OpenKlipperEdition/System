project(cpp_calibrate CXX )

#aux_source_directory(. CALIB_SRC_FILE)
set (CALIB_SRC_FILE calibrate.cpp stereo_calibrate.cpp)

INCLUDE_DIRECTORIES(.)

if (OPENCV_ENABLE)

add_library(calibrate SHARED ${CALIB_SRC_FILE})
add_executable(calib.exe  "example-calibrate.cpp")
target_link_libraries(calib.exe calibrate)

endif()

install(TARGETS  calibrate calib.exe
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static)
