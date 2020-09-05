set(SOURCE
  breakpoint_mgr.cpp
  cdb_file.cpp
  context_mgr.cpp
  disassembly.cpp
  dbg_session.cpp
  ihex.c
  line_parser.cpp
  line_spec.cpp
  log.cpp
  mem_remap.cpp
  module.cpp
  registers.cpp
  out_format.cpp
  symbol.cpp
  sym_tab.cpp
  sym_type_tree.cpp
  target_cc.cpp
  target.cpp
  target_dummy.cpp
  target_s51.cpp
  target_silabs.cpp
)

set(HEADER
  breakpoint_mgr.h
  cdb_file.h
  context_mgr.h
  disassembly.h
  dbg_session.h
  ihex.h
  line_parser.h
  line_spec.h
  log.h
  mem_remap.h
  module.h
  registers.h
  out_format.h
  symbol.h
  sym_tab.h
  sym_type_tree.h
  target_cc.h
  target_dummy.h
  target.h
  target_s51.h
  target_silabs.h
  types.h
)

find_package(fmt)

add_library(debug-core STATIC ${SOURCE} ${HEADER})
target_compile_features(debug-core PUBLIC cxx_std_17)
target_include_directories(debug-core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR} 
)
target_link_libraries(debug-core PUBLIC fmt::fmt ec2drv ccdrv)