set(SOURCE
  cmdbreakpoints.cpp
  cmdcommon.cpp
  cmddisassemble.cpp
  cmdlist.cpp
  cmdmaintenance.cpp
  cmdshow.cpp
  dap_server.cpp
  sddbg.cpp
  parsecmd.cpp
)
set(HEADER
  cmdbreakpoints.h
  cmdcommon.h
  cmddisassemble.h
  cmdlist.h
  cmdmaintenance.h
  cmdshow.h
  dap_server.h
  sddbg.h
  parsecmd.h
)

find_package(PkgConfig)
pkg_check_modules(readline REQUIRED readline)

find_package(Threads REQUIRED)
find_package(fmt REQUIRED)

add_executable(sddbg ${SOURCE} ${HEADER})
target_compile_features(sddbg PUBLIC cxx_std_17)
target_include_directories(sddbg PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(sddbg PUBLIC debug-core ${CMAKE_THREAD_LIBS_INIT} fmt::fmt cppdap)

if (${readline_FOUND})
  target_compile_definitions(sddbg PRIVATE
    HAVE_LIBREADLINE
    HAVE_READLINE_READLINE_H
    HAVE_READLINE_HISTORY
    HAVE_READLINE_HISTORY_H
  )
  target_link_libraries(sddbg PUBLIC ${readline_LIBRARIES})
endif()