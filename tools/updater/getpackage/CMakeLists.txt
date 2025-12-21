project(getpackage LANGUAGES C)
set(PROJGETPACKAGE getpackage)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -Wall -fPIC -g -Os -Wuninitialized -Wundef")


add_definitions(-DMMC_INDEX=${MMC_INDEX})

add_executable(${PROJGETPACKAGE}
	"main.c"
	"cjson_helper.c"
	)

target_link_libraries(
  ${PROJGETPACKAGE}
  cjson
  m
  dl
  pthread
)

target_include_directories(${PROJGETPACKAGE} PUBLIC
	"${PROJECT_SOURCE_DIR}"
)

install(TARGETS ${PROJGETPACKAGE} DESTINATION "/sbin/")
