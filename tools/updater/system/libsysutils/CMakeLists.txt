set(PROJNAME sysutils)

set(SYSUTILS_SOURCE_FILES
  src/FrameworkClient.cpp
  src/FrameworkListener.cpp
  src/SocketClient.cpp
  src/FrameworkCommand.cpp
  src/ServiceManager.cpp
  src/SocketListener.cpp
  src/ClientListener.cpp
  src/MyClientListener.cpp
  src/MyFrameworkListener.cpp
  )

add_library(${PROJNAME} SHARED ${SYSUTILS_SOURCE_FILES})
target_link_libraries(${PROJNAME}
  icutils
  pthread
  )
#add_dependencies(${PROJNAME} ipc dict hal gevent)

include_directories(../include)

install(DIRECTORY ../include/sysutils DESTINATION include)
install(TARGETS ${PROJNAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
# install(FILES   S22debuggerd DESTINATION "/etc/init.d/"
#   PERMISSIONS
#   OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ)

# install(FILES debugger.sh DESTINATION "/etc/profile.d/"
#   PERMISSIONS
#   OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ)
