SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_CROSS_COMPILER "mips-linux-gnu-")
SET(CMAKE_C_COMPILER "${CMAKE_CROSS_COMPILER}gcc")
SET(CMAKE_CXX_COMPILER "${CMAKE_CROSS_COMPILER}g++")

#set(CMAKE_BUILD_TYPE release)

#add_definitions(-Wall -Wextra)
#add_definitions(-fPIC)
#add_definitions(-Ofast)
#add_definitions(-O3)
#add_definitions(-ffast-math)
#add_definitions(-fvisibility=hidden -fvisibility-inlines-hidden)

SET(LOCAL_C_FLAGS   "-EL  -march=mips32r2")
SET(LOCAL_CXX_FLAGS "-EL  -march=mips32r2")

SET(MSA_ENABLE ON)
##############################################
