set(SOURCE
  main.cpp
)
set(HEADER

)

find_package(fmt REQUIRED)

add_executable(cc-tool ${SOURCE} ${HEADER})
target_compile_features(cc-tool PUBLIC cxx_std_17)
target_include_directories(cc-tool PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR} 
)
target_link_libraries(cc-tool PUBLIC debug-core driver-core ccdrv fmt::fmt)