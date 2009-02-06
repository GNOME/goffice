SUBDIRS = utils data app graph math ms-compat cut-n-paste

lib_LTLIBRARIES = libgoffice-@GOFFICE_API_VER@.la

libgoffice_@GOFFICE_API_VER@_la_LIBADD = 			\
	utils/libgoffice-utils.la		\
	app/libgoffice-app.la			\
	data/libgoffice-data.la			\
	graph/libgoffice-graph.la		\
	math/libgoffice-math.la		\
	ms-compat/libgoffice-ms-compat.la	\
	$(GOFFICE_LIBS)

if WITH_GTK
SUBDIRS += gtk drawing component
libgoffice_@GOFFICE_API_VER@_la_LIBADD += 			\
	gtk/libgoffice-gtk.la			\
	drawing/libgoffice-drawing.la		\
	component/libgoffice-component.la	\
	cut-n-paste/foocanvas/libfoocanvas.la
endif

libgoffice_@GOFFICE_API_VER@_la_LDFLAGS = -version-info $(VERSION_INFO)
if WITH_WIN32
libgoffice_@GOFFICE_API_VER@_la_DEPENDENCIES = goffice.def
libgoffice_@GOFFICE_API_VER@_la_LDFLAGS += -no-undefined -export-symbols goffice.def
endif

BUILT_SOURCES =	goffice-paths.h
CLEANFILES = $(BUILT_SOURCES)

libgoffice_@GOFFICE_API_VER@_la_SOURCES = 	\
	goffice.c		\
	goffice-priv.h

libgoffice_@GOFFICE_API_VER@_ladir = $(goffice_include_dir)
libgoffice_@GOFFICE_API_VER@_la_HEADERS = 	\
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

if WITH_WIN32
LIB_PUBLIC_HDRS = $(libgoffice_@GOFFICE_API_VER@_la_HEADERS)

goffice.def: local.def app/local.def \
	cut-n-paste/foocanvas/local.def \
	component/local.def \
	data/local.def drawing/local.def graph/local.def \
	gtk/local.def  math/local.def \
	ms-compat/local.def utils/local.def
	echo EXPORTS > $@ && \
	cat $^ | sort >> $@

if HAVE_LIBEXE
goffice_mslibdir = $(DESTDIR)$(libdir)
goffice_mslib_DATA = goffice-0.lib

goffice-0.lib: libgoffice-@GOFFICE_API_VER@.la goffice.def
	lib -name:libgoffice-0-$(VERSION_IFACE).dll -def:goffice.def -out:$@
endif
endif
