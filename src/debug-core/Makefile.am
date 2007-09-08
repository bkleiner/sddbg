INCLUDES = -I$(top_srcdir)/src/ec2drv $(all_includes)
METASOURCES = AUTO

lib_LTLIBRARIES = libec2debugcore.la

libec2debugcore_la_SOURCES =                                                   \
	cdbfile.cpp cdbfile.h data_types.h ihex.c ihex.h memremap.cpp          \
	memremap.h module.cpp module.h symbol.cpp symbol.h symtab.cpp symtab.h \
	symtypetree.cpp symtypetree.h target.cpp target.h targets51.cpp        \
	targets51.h targetsilabs.cpp targetsilabs.h types.h linespec.h         \
	dbgsession.cpp outformat.cpp outformat.h contextmgr.cpp contextmgr.h   \
	breakpointmgr.cpp breakpointmgr.h linespec.cpp
	
libec2debugcore_la_LIBADD = $(top_builddir)/src/ec2drv/libec2drv.la -lboost_regex
noinst_HEADERS = dbgsession.h

#ec2drv_incdir = @prefix@/include/ec2drv
#ec2drv_inc_HEADERS = ec2drv.h device_enum.h ec2types.h devices.h