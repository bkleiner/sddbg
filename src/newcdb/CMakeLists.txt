cmake_minimum_required(VERSION 3.10)
project(ec2-new)

set(SOURCE
  cmdbreakpoints.cpp
  cmdcommon.cpp
  cmddisassemble.cpp
  cmdlist.cpp
  cmdmaintenance.cpp
  cmdshow.cpp
  main.cpp
  parsecmd.cpp
)
set(HEADER
  cmdbreakpoints.h
  cmdcommon.h
  cmddisassemble.h
  cmdlist.h
  cmdmaintenance.h
  cmdshow.h
  newcdb.h
  parsecmd.h
)

find_package(PkgConfig)
pkg_check_modules(readline REQUIRED readline)

find_package(Threads REQUIRED)
find_package(fmt REQUIRED)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS regex)

add_executable(newcdb ${SOURCE} ${HEADER})
target_compile_features(newcdb PUBLIC cxx_std_17)
target_include_directories(newcdb PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../ec2drv
  ${CMAKE_CURRENT_SOURCE_DIR}/../debug-core
)
target_link_libraries(newcdb PUBLIC debug-core ${CMAKE_THREAD_LIBS_INIT} Boost::regex fmt::fmt)

if (${readline_FOUND})
  target_compile_definitions(newcdb PRIVATE
    HAVE_LIBREADLINE
    HAVE_READLINE_READLINE_H
    HAVE_READLINE_HISTORY
    HAVE_READLINE_HISTORY_H
  )
  target_link_libraries(newcdb PUBLIC ${readline_LIBRARIES})
endif()