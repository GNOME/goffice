SUBDIRS = utils data app graph gtk drawing ms-compat cut-n-paste

lib_LTLIBRARIES = libgoffice-1.la

libgoffice_1_la_LIBADD = 			\
	cut-n-paste/pcre/libpcre.la		\
	utils/libgoffice-utils.la		\
	app/libgoffice-app.la			\
	data/libgoffice-data.la			\
	gtk/libgoffice-gtk.la			\
	graph/libgoffice-graph.la		\
	drawing/libgoffice-drawing.la		\
	ms-compat/libgoffice-ms-compat.la

libgoffice_1_la_LDFLAGS = -version-info $(GOFFICE_LIB_VERSON)
if PLATFORM_WIN32
libgoffice_1_la_LDFLAGS += $(no_undefined)
endif

BUILT_SOURCES =	goffice-paths.h
libgoffice_1_la_SOURCES = 	\
	$(BUILT_SOURCES)	\
	goffice.c		\
	goffice-priv.h		\
	goffice-config.h
libgoffice_1_ladir = $(goffice_include_dir)
libgoffice_1_la_HEADERS = 	\
	goffice.h

goffice-paths.h: goffice-paths.sh
	@sh $< > $@.tmp ;					\
	if cmp -s $@.tmp $@; then				\
	    rm $@.tmp;						\
	else							\
	    mv $@.tmp $@ ; echo updated goffice-paths.h ;	\
	fi

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA= libgoffice-1.pc

EXTRA_DIST = libgoffice-1.pc.in

include $(top_srcdir)/goffice.mk
