set(SOURCE
  cc_debugger.cpp
)
set(HEADER
  cc_debugger.h
)

add_library(ccdrv STATIC ${SOURCE} ${HEADER})
target_compile_features(ccdrv PUBLIC cxx_std_17)
target_include_directories(ccdrv PUBLIC 
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR} 
)
target_link_libraries(ccdrv PUBLIC driver-core)
