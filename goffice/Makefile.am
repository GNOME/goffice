SUBDIRS = app graph utils gtk drawing ms-compat

noinst_LTLIBRARIES = libgoffice.la

libgoffice_la_LIBADD = 				\
	utils/libgoffice-utils.la		\
	app/libgoffice-app.la			\
	gtk/libgoffice-gtk.la			\
	graph/libgoffice-graph.la		\
	drawing/libgoffice-drawing.la		\
	ms-compat/libgoffice-ms-compat.la

libgoffice_la_SOURCES = 	\
	goffice.c		\
	goffice.h		\
	goffice-priv.h		\
	goffice-config.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA= libgoffice-1.pc

include $(top_srcdir)/goffice.mk
