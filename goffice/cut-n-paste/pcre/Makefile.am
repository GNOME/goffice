noinst_LTLIBRARIES = libpcre.la
libpcre_la_SOURCES = pcre_exec.c pcre_compile.c pcre_tables.c pcre_chartables.c pcre_globals.c pcre_xclass.c pcre_info.c pcre_valid_utf8.c pcre_try_flipped.c pcre_ord2utf8.c
EXTRA_DIST = COPYING

libpcre_a_ladir = $(includedir)/libgoffice-1/goffice/cut-n-paste/pcre
libpcre_a_la_HEADERS = 
noinst_HEADERS = pcre.h pcre_internal.h ucp.h

AM_CPPFLAGS = \
	$(GOFFICE_CFLAGS) \
	-I$(top_builddir) \
	-DSUPPORT_UTF8 \
	-DNEWLINE=10 \
	-DPOSIX_MALLOC_THRESHOLD=100 \
	-DLINK_SIZE=2 \
	-DMATCH_LIMIT=10000000

