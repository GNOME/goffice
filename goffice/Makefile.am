SUBDIRS = app graph utils gtk drawing ms-compat

noinst_LTLIBRARIES = libgoffice.la

libgoffice_la_LIBADD = 				\
	utils/libgoffice-utils.la		\
	app/libgoffice-app.la			\
	gtk/libgoffice-gtk.la			\
	graph/libgoffice-graph.la		\
	drawing/libgoffice-drawing.la		\
	ms-compat/libgoffice-ms-compat.la

BUILT_SOURCES =	goffice-paths.h
libgoffice_la_SOURCES = 	\
	$(BUILT_SOURCES)	\
	goffice.c		\
	goffice.h		\
	goffice-priv.h		\
	goffice-config.h

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
