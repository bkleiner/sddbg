bin_PROGRAMS = ec2cdb

# set the include path found by configure
INCLUDES = -I$(top_srcdir)/src/ec2drv $(all_includes)

# the library search path.
ec2cdb_LDFLAGS = -all-static $(all_libraries)
ec2cdb_SOURCES = break.c cmd.c sdcdb.c simi.c symtab.c SDCCbitv.c SDCChasht.c \
		SDCCset.c NewAlloc.c ihex.c
ec2cdb_LDADD = $(top_builddir)/src/ec2drv/libec2drv.la
