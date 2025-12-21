set(PROJNAME icutils)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++11 -Wall -fPIC -g -Wno-unused-variable")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -std=gnu99 -Wall -fPIC -g")


include_directories(../include)
set(CUTILS_SOURCE_FILES
  system_properties.cpp
  system_properties_compat.c
  properties.c
  socket_local_client.c
  socket_local_server.c
  socket_inaddr_any_server.c
  )

add_library(${PROJNAME} SHARED ${CUTILS_SOURCE_FILES})

#add_dependencies(${PROJNAME} ipc dict hal gevent)


install(DIRECTORY ../include/icutils DESTINATION include)
install(TARGETS ${PROJNAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
# install(FILES   S22debuggerd DESTINATION "/etc/init.d/"
#   PERMISSIONS
#   OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ)

# install(FILES debugger.sh DESTINATION "/etc/profile.d/"
#   PERMISSIONS
#   OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ)
