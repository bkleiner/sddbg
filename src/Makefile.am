
# set the include path found by configure
INCLUDES= $(all_includes)

# the library search path.
noinst_HEADERS = ec2drv.h
lib_LTLIBRARIES = libec2drv.la
libec2drv_la_SOURCES = ec2drv.c
bin_PROGRAMS = ec2test ec2writeflash
ec2test_SOURCES = ec2test.c
ec2test_LDADD = $(top_builddir)/src/libec2drv.la
ec2writeflash_SOURCES = writeflash.c
ec2writeflash_LDADD = $(top_builddir)/src/libec2drv.la
