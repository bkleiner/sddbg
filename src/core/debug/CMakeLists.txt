set(SOURCE
  breakpointmgr.cpp 
  contextmgr.cpp 
  linespec.cpp 
  module.cpp    
  symbol.cpp 
  symtypetree.cpp 
  target-dummy.cpp 
  cdbfile.cpp       
  dbgsession.cpp 
  memremap.cpp 
  outformat.cpp 
  symtab.cpp 
  target.cpp      
	targets51.cpp
	targetsilabs.cpp
  ihex.c
)

set(HEADER
  breakpointmgr.h  
  contextmgr.h  
  dbgsession.h  
  linespec.h  
  module.h     
  symbol.h  
  symtypetree.h   
  target.h     
  cdbfile.h       
  ihex.h       
  memremap.h 
  outformat.h 
  symtab.h 
  target-dummy.h 
	targets51.h 
	targetsilabs.h
  types.h
)

set(
  CORE_DEBUG_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR} 
)

find_package(fmt)

add_library(debug-core STATIC ${SOURCE} ${HEADER})
target_compile_features(debug-core PUBLIC cxx_std_17)
target_include_directories(debug-core PUBLIC
	${CORE_DEBUG_INCLUDE_DIRS}
	${DRIVER_INCLUDE_DIRS}
)
target_link_libraries(debug-core PUBLIC fmt::fmt ec2drv)