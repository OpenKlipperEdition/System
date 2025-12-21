set(PROJDEBUGGER debuggerd)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__STDC_FORMAT_MACROS -DHAVE_SYS_SOCKET_H -DHAVE_LINUX_LOCAL_SOCKET_NAMESPACE")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__STDC_FORMAT_MACROS -DHAVE_SYS_SOCKET_H -DHAVE_LINUX_LOCAL_SOCKET_NAMESPACE")

set(DEBUGGER_SOURCE_FILES
    "backtrace.cpp"
    "debuggerd.cpp"
    "tombstone.cpp"
    "utility.cpp"
	"socket_local_client.c"
	"socket_local_server.c"
	"debugger.c"
	)

set(DEBUGGER_SOURCE_FILES_arm ${DEBUGGER_SOURCE_FILES} "arm/machine.cpp")
set(DEBUGGER_SOURCE_FILES_arm64 ${DEBUGGER_SOURCE_FILES} "arm64/machine.cpp")
set(DEBUGGER_SOURCE_FILES_mips ${DEBUGGER_SOURCE_FILES} "mips/machine.cpp")
set(DEBUGGER_SOURCE_FILES_mips64 ${DEBUGGER_SOURCE_FILES} "mips/machine.cpp")
set(DEBUGGER_SOURCE_FILES_x86 ${DEBUGGER_SOURCE_FILES} "x86/machine.cpp")
set(DEBUGGER_SOURCE_FILES_x86_64 ${DEBUGGER_SOURCE_FILES} "x86_64/machine.cpp")
#link_libraries(syslog)

add_executable(${PROJDEBUGGER} ${DEBUGGER_SOURCE_FILES} ${DEBUGGER_SOURCE_FILES_${TARGET_ARCH}})

add_dependencies(${PROJDEBUGGER} log backtrace unwind unwind-ptrace)

target_link_libraries(
  ${PROJDEBUGGER}
  log
  backtrace
  unwind-ptrace
  unwind
  pthread
)

install(TARGETS ${PROJDEBUGGER} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
install(FILES   S22debuggerd DESTINATION "/etc/init.d/"
  PERMISSIONS
  OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ)

install(FILES debugger.sh DESTINATION "/etc/profile.d/"
  PERMISSIONS
  OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ)
