INCLUDES = $(all_includes)
METASOURCES = AUTO
lib_LTLIBRARIES = libec2drv.la
libec2drv_la_SOURCES = ec2drv.c devices.c device_table.c

libec2drv_la_LDFLAGS = -all-static
noinst_HEADERS = devices.h ec2drv.h
libec2drv_la_LIBADD = -lusb
