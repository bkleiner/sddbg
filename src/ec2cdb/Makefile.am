bin_PROGRAMS = ec2cdb

# set the include path found by configure
INCLUDES = -I$(top_srcdir)/src/ec2drv $(all_includes)

# the library search path.
ec2cdb_LDFLAGS = -all-static $(all_libraries)
ec2cdb_SOURCES = break.c cmd.c sdcdb.c simi.c symtab.c SDCCbitv.c SDCChasht.c \
		SDCCset.c NewAlloc.c ihex.c
ec2cdb_LDADD = $(top_builddir)/src/ec2drv/libec2drv.la

noinst_HEADERS = sdcdb.h symtab.h break.h cmd.h newalloc.h simi.h sdccconf.h\
				 SDCCset.h SDCCset.h SDCChasht.h SDCCbitv.h ihex.h common.h \
				 SDCCglobl.h SDCCmem.h SDCCast.h SDCCy.h SDCCicode.h		\
				 SDCClabel.h SDCCBBlock.h SDCCloop.h SDCCcse.h SDCCcflow.h	\
				 SDCCdflow.h SDCCdflow.h SDCClrange.h SDCCptropt.h SDCCopt.h\
				 SDCCglue.h SDCCpeeph.h SDCCdebug.h SDCCutil.h asm.h port.h	\
				 SDCCargs.h SDCCerr.h SDCCsymt.h SDCCval.h
