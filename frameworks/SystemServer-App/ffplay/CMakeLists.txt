CMAKE_MINIMUM_REQUIRED(VERSION 3.27)
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_C_COMPILER "mips-linux-gnu-gcc")
SET(CMAKE_CXX_COMPILER "mips-linux-gnu-g++")
SET(CMAKE_C_FLAGS "-pthread -fPIC -march=mips32r2")
SET(CMAKE_CXX_FLAGS "-std=c++11 -pthread -fPIC")
PROJECT(ffplayer)


install(FILES ${PROJECT_SOURCE_DIR}/libffplay.so DESTINATION /usr/lib PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

message(STATUS "=================the path is ${PROJECT_SOURCE_DIR}=======================")

add_subdirectory(app)
