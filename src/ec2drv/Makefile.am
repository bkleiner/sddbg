INCLUDES = $(all_includes)
METASOURCES = AUTO
lib_LTLIBRARIES = libec2drv.la
libec2drv_la_SOURCES = ec2drv.c devices.c device_table.c

libec2drv_la_LIBADD = -lusb
noinst_HEADERS = devices.h ec2drv.h
noinst_SCRIPTS = csv2c.py
EXTRA_DIST = csv2c.py
