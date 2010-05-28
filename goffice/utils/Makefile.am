patternsdir = $(goffice_datadir)/patterns
dist_patterns_DATA = svg-patterns.xml

noinst_LTLIBRARIES = libgoffice-utils.la
libgoffice_utils_la_SOURCES =	\
	go-marshalers.list	\
	\
	go-color.c		\
	go-file.c		\
	go-font.c		\
	\
	go-cairo.c		\
	go-gradient.c		\
	go-image.c		\
	go-line.c		\
	go-locale.c		\
	go-marker.c		\
	go-path.c		\
	go-pattern.c		\
	go-geometry.c		\
	go-string.c		\
	go-undo.c		\
	datetime.c		\
	formats.c		\
	go-format.c		\
	regutf8.c 		\
	\
	go-glib-extras.c	\
	go-libxml-extras.c	\
	go-pango-extras.c	\
	go-persist.c		\
	go-bezier.c		\
	go-editor.c		\
	go-style.c		\
	go-styled-object.c

libgoffice_utils_ladir = $(goffice_include_dir)/utils
libgoffice_utils_la_HEADERS = 	\
	goffice-utils.h		\
	go-cairo.h		\
	go-color.h		\
	go-file.h		\
	go-font.h		\
	go-gradient.h		\
	go-image.h		\
	go-line.h		\
	go-locale.h		\
	go-marker.h		\
	go-path.h		\
	go-pattern.h		\
	go-units.h		\
	go-geometry.h		\
	go-string.h		\
	go-undo.h		\
	datetime.h		\
	go-format.h		\
	regutf8.h 		\
	go-glib-extras.h	\
	go-libxml-extras.h	\
	go-pango-extras.h	\
	go-persist.h		\
	go-bezier.h		\
	go-editor.h		\
	go-style.h		\
	go-styled-object.h

uidir = $(goffice_datadir)/ui
dist_ui_DATA = \
	go-style-prefs.ui

CLEANFILES =			\
	go-marshalers.h		\
	go-marshalers.c

# A hint is needed to build the header first:
BUILT_SOURCES = go-marshalers.h

# Another hint, see bugs #172211 and #172212:
non-intermediate: go-marshalers.c

GENMARSHAL_COMMAND = $(GLIB_GENMARSHAL) --prefix=go_

.list.h:
	$(GENMARSHAL_COMMAND) --header $< >$@

.list.c:
	(echo '/* This file has been automatically generated.  Do not edit. */' && \
	echo '#include <goffice/goffice-config.h>' && \
	echo '#include "$*.h"' && \
	$(GENMARSHAL_COMMAND) --body $< ) >$@

include $(top_srcdir)/goffice.mk

LIB_PUBLIC_HDRS = $(libgoffice_utils_la_HEADERS)
