noinst_LTLIBRARIES = libgoffice-utils.la

libgoffice_utils_la_SOURCES =	\
	go-marshalers.list	\
	\
	go-color.c		\
	go-file.c		\
	go-font.c		\
	\
	go-gradient.c		\
	go-line.c		\
	go-marker.c		\
	go-pattern.c		\
	go-locale.c		\
	go-math.c		\
	datetime.c		\
	format.c		\
	formats.c		\
	go-format.c		\
	go-format-match.c	\
	regutf8.c 		\
	\
	go-glib-extras.c	\
	go-libxml-extras.c

libgoffice_utils_ladir = $(goffice_include_dir)/utils
libgoffice_utils_la_HEADERS = 	\
	goffice-utils.h		\
	go-color.h		\
	go-file.h		\
	go-font.h		\
	go-gradient.h		\
	go-line.h		\
	go-marker.h		\
	go-pattern.h		\
	go-locale.h		\
	go-units.h		\
	go-math.h		\
	datetime.h		\
	format.h		\
	format-impl.h		\
	go-format.h		\
	go-format-match.h	\
	regutf8.h 		\
	go-glib-extras.h	\
	go-libxml-extras.h
CLEANFILES =			\
	go-marshalers.h		\
	go-marshalers.c

# A hint is needed to build the header first:
BUILT_SOURCES = go-marshalers.h

GENMARSHAL_COMMAND = $(GLIB_GENMARSHAL) --prefix=go_
SUFFIXES = .list

.list.h: $(GLIB_GENMARSHAL)
	$(GENMARSHAL_COMMAND) --header $< >$@

.list.c: $(GLIB_GENMARSHAL)
	(echo '/* This file has been automatically generated.  Do not edit. */' && \
	echo '#include <goffice/goffice-config.h>' && \
	echo '#include "$*.h"' && \
	$(GENMARSHAL_COMMAND) --body $< ) >$@

include $(top_srcdir)/goffice.mk
