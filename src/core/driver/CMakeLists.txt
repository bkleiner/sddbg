set(SOURCE
  serial.cpp 
)

set(HEADER
  serial.h  
)

find_package(fmt)

add_library(driver-core STATIC ${SOURCE} ${HEADER})
target_compile_features(driver-core PUBLIC cxx_std_17)
target_include_directories(driver-core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR} 
)
target_link_libraries(driver-core PUBLIC fmt::fmt)