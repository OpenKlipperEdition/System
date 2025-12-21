include(ExternalProject)

set(GFLAG_ROOT          ${CMAKE_BINARY_DIR}/host-tools/)
set(GFLAG_LIB_DIR       ${GFLAG_ROOT}/lib)
set(GFLAG_INCLUDE_DIR   ${GFLAG_ROOT}/include)
set(GFLAG_URL           ${GFLAG_ROOT})

if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux")
    set(INSERD_COMMAND "")
else()
    set(INSERD_COMMAND -G "MinGW Makefiles")
endif()

set(GFLAG_CONFIGURE     cd ${GFLAG_ROOT} && ${CMAKE_COMMAND} -D CMAKE_INSTALL_PREFIX=${GFLAG_ROOT} ${INSERD_COMMAND} ${CMAKE_SOURCE_DIR}/host-tools)
set(GFLAG_MAKE          cd ${GFLAG_ROOT} && $(MAKE) --silent)
set(GFLAG_INSTALL       cd ${GFLAG_ROOT} && $(MAKE) install)

ExternalProject_Add(ghost-tools
  SOURCE_DIR  ${CMAKE_SOURCE_DIR}/host-tools
  PREFIX                ${GFLAG_ROOT}
  BUILD_ALWAYS                 1
  CONFIGURE_COMMAND     ${GFLAG_CONFIGURE}
  BUILD_COMMAND         ${GFLAG_MAKE}
  COMMENT               "External project host-tools build"
  INSTALL_COMMAND       ""#${GFLAG_INSTALL}
)
