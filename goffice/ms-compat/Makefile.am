noinst_LTLIBRARIES = libgoffice-ms-compat.la 

libgoffice_ms_compat_la_SOURCES =	\
	go-ms-parser.c
if WITH_GTK
libgoffice_ms_compat_la_SOURCES +=	\
	god-image-ms.c			\
	god-drawing-ms-client-handler.c	\
	god-drawing-ms.c
endif

libgoffice_ms_compat_ladir = $(goffice_include_dir)/ms-compat
libgoffice_ms_compat_la_HEADERS = 	\
	go-ms-parser.h
if WITH_GTK
libgoffice_ms_compat_la_HEADERS +=	\
	god-image-ms.h			\
	god-drawing-ms-client-handler.h	\
	god-drawing-ms.h
endif

include $(top_srcdir)/goffice.mk

LIB_PUBLIC_HDRS = $(libgoffice_ms_compat_la_HEADERS)
