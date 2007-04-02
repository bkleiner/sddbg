bin_PROGRAMS = newcdb
newcdb_SOURCES = target.cpp symtab.cpp cdbfile.cpp symbol.cpp parsecmd.cpp \
				cmdcommon.cpp cmdbreakpoints.cpp targetsilabs.cpp targets51.cpp breakpointmgr.cpp \
				linespec.cpp ihex.c module.cpp cmddisassemble.cpp cmdmaintenance.cpp contextmgr.cpp \
				main.cpp memremap.cpp symtypetree.cpp outformat.cpp

# set the include path found by configure
INCLUDES = -I$(top_srcdir)/src/ec2drv $(all_includes)

# the library search path.
newcdb_LDFLAGS = $(all_libraries) 
noinst_HEADERS = target.h symtab.h cdbfile.h symbol.h parsecmd.h cmdshow.h \
		cmdcommon.h cmdbreakpoints.h targetsilabs.h targets51.h breakpointmgr.h linespec.h \
		ihex.h module.h types.h cmddisassemble.h cmdmaintenance.h contextmgr.h \
		memremap.h symtypetree.h outformat.h
newcdb_LDADD = $(top_builddir)/src/ec2drv/libec2drv.la -lpthread

##AUTOMAKE_OPTIONS = dejagnu
