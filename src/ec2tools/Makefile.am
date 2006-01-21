INCLUDES = -I$(top_srcdir)/src/ec2drv $(all_includes)
METASOURCES = AUTO
bin_PROGRAMS = ec2writeflash ec2test ec2readflash ec2-update-fw ec2test-F310
ec2writeflash_LDADD = $(top_builddir)/src/ec2drv/libec2drv.la
ec2writeflash_SOURCES = writeflash.c ihex.c
ec2test_SOURCES = ec2test.c
ec2test_LDADD = $(top_builddir)/src/ec2drv/libec2drv.la
noinst_HEADERS = ihex.h
ec2readflash_SOURCES = ihex.c readflash.c
ec2readflash_LDADD = $(top_builddir)/src/ec2drv/libec2drv.la

ec2test_LDFLAGS = -all-static
ec2writeflash_LDFLAGS = -all-static
ec2_update_fw_SOURCES = ec2fw.c
ec2_update_fw_LDADD = $(top_builddir)/src/ec2drv/libec2drv.la
ec2readflash_LDFLAGS = -all-static
ec2test_F310_SOURCES = ec2test-F310.c
ec2test_F310_LDADD = $(top_builddir)/src/ec2drv/libec2drv.la
