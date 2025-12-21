project(update LANGUAGES C)
set(PROJUPDATE update)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -Wall -fPIC -g -Os -Wuninitialized -Wundef")

if(BUILD_MMC_DEVICE)
	add_definitions(-DMMC_DEVICE)
endif()
add_executable(${PROJUPDATE}
	"main.c"
	"update.c"
	"cjson_helper.c"
	)

target_link_libraries(
  ${PROJUPDATE}
  cjson
  m
  dl
  pthread
)

target_include_directories(${PROJUPDATE} PUBLIC
	"${PROJECT_SOURCE_DIR}"
)

install(TARGETS ${PROJUPDATE} DESTINATION "/sbin/")
