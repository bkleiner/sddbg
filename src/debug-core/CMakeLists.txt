cmake_minimum_required(VERSION 3.10)
project(ec2-new)

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

find_package(fmt)

add_library(debug-core STATIC ${SOURCE} ${HEADER})
target_include_directories(debug-core PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR} 
	${CMAKE_CURRENT_SOURCE_DIR}/../ec2drv
	${CMAKE_CURRENT_BINARY_DIR}/../ec2drv
)
target_link_libraries(debug-core PUBLIC fmt::fmt ec2drv)