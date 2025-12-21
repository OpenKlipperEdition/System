project(example LANGUAGES C CXX)
set(DISPARITY_SRC_FILE
   example-disparity.cpp)

#add_executable(dilate test.cpp)


add_executable(disparity ${DISPARITY_SRC_FILE} rectify.cpp)
add_executable(rectify   example-rectify.cpp rectify.cpp)
add_executable(yamlread  "example-readxml.cpp")
add_executable(rectifyMap "example-rectifyMap.cpp")

target_link_libraries(disparity stereo)
target_link_libraries(rectify stereo)
target_link_libraries(yamlread stereo yaml-cpp)
target_link_libraries(rectifyMap stereo)

install(TARGETS  disparity rectify yamlread rectifyMap
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static)
