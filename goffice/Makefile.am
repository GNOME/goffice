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
libgoffice_0_la_DEPENDENCIES = goffice.def
libgoffice_0_la_LDFLAGS += -no-undefined -export-symbols goffice.def
libgoffice_0_la_LIBADD += -luuid -lhtmlhelp -lurlmon
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

if WITH_WIN32
LIB_PUBLIC_HDRS = $(libgoffice_1_la_HEADERS)

goffice.def: local.def app/local.def \
	cut-n-paste/foocanvas/local.def \
	data/local.def drawing/local.def graph/local.def \
	gtk/local.def ms-compat/local.def utils/local.def
	echo EXPORTS > $@ && \
	cat $^ | sort >> $@

if HAVE_LIBEXE
goffice_mslibdir = $(DESTDIR)$(libdir)
goffice_mslib_DATA = goffice-0.lib

goffice-0.lib: libgoffice-0.la goffice.def
	lib -name:libgoffice-0-$(VERSION_IFACE).dll -def:goffice.def -out:$@
endif
endif
