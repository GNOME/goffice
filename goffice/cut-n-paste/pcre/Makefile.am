noinst_LTLIBRARIES = libpcre.la
libpcre_la_SOURCES = maketables.c get.c study.c pcre.c pcreposix.c
EXTRA_DIST = printint.c

libpcre_a_ladir = $(includedir)/libgoffice-1/goffice/cut-n-paste/pcre
libpcre_a_la_HEADERS = pcreposix.h
noinst_HEADERS = internal.h pcre.h

INCLUDES = \
	$(GOFFICE_CFLAGS) \
	-I$(top_builddir) \
	-DSUPPORT_UTF8 \
	-DNEWLINE=10 \
	-DPOSIX_MALLOC_THRESHOLD=100 \
	-DLINK_SIZE=2 \
	-DMATCH_LIMIT=10000000

