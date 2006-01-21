SUBDIRS = utils data app graph gtk drawing ms-compat cut-n-paste component

lib_LTLIBRARIES = libgoffice-0.la

if WITH_INCLUDED_PCRE
pcre_la = cut-n-paste/pcre/libpcre.la
else
pcre_la =
endif

libgoffice_0_la_LIBADD = 			\
	$(pcre_la)				\
	utils/libgoffice-utils.la		\
	app/libgoffice-app.la			\
	data/libgoffice-data.la			\
	gtk/libgoffice-gtk.la			\
	cut-n-paste/foocanvas/libfoocanvas.la	\
	graph/libgoffice-graph.la		\
	drawing/libgoffice-drawing.la		\
	ms-compat/libgoffice-ms-compat.la	\
	component/libgoffice-component.la	\
	$(GOFFICE_LIBS)

libgoffice_0_la_LDFLAGS = -version-info $(VERSION_INFO)
if WITH_WIN32
libgoffice_0_la_LDFLAGS += -no-undefined -Wl,-export-all-symbols
libgoffice_0_la_LIBADD += utils/libgoffice-win32-stub.la
endif

BUILT_SOURCES =	goffice-paths.h
CLEANFILES = $(BUILT_SOURCES)

libgoffice_0_la_SOURCES = 	\
	$(BUILT_SOURCES)	\
	goffice.c		\
	goffice-priv.h

libgoffice_0_ladir = $(goffice_include_dir)
libgoffice_0_la_HEADERS = 	\
	goffice-features.h	\
	goffice.h

# Depends on this Makefile, because it uses make variables.
goffice-paths.h: Makefile
	@echo 'creating $@'
	@( \
	echo '/* This file has been automatically generated.  Do not edit. */'; \
	echo ''; \
	echo '#ifndef GOFFICE_PATHS_H'; \
	echo '#define GOFFICE_PATHS_H'; \
	echo ''; \
	echo '#define GOFFICE_DATADIR "$(goffice_datadir)"'; \
	echo '#define GOFFICE_LIBDIR "$(goffice_libdir)"'; \
	echo '#define GOFFICE_ICONDIR "$(goffice_icondir)"'; \
	echo '#define GOFFICE_LOCALEDIR "$(goffice_localedir)"'; \
	echo ''; \
	echo '#endif /* GOFFICE_PATHS_H */'; \
	) >$@

include $(top_srcdir)/goffice.mk
