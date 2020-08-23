cmake_minimum_required(VERSION 3.10)
project(ec2-new)

set(SOURCE
  cmdbreakpoints.cpp
  cmdcommon.cpp
  cmddisassemble.cpp
  cmdmaintenance.cpp
  cmdshow.cpp
  main.cpp
  parsecmd.cpp
)
set(HEADER
  cmdbreakpoints.h
  cmdcommon.h
  cmddisassemble.h
  cmdmaintenance.h
  cmdshow.h
  newcdb.h
  parsecmd.h
)

find_package(Threads REQUIRED)

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
target_link_libraries(newcdb PUBLIC debug-core ${CMAKE_THREAD_LIBS_INIT} Boost::regex)