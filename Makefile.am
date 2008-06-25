# Makefile.am for goffice

SUBDIRS = goffice tests pixmaps plugins po docs tools

EXTRA_DIST = README NEWS BUGS MAINTAINERS AUTHORS \
	goffice.mk \
	goffice-plugins.mk \
	libgoffice.pc.in \
	acinclude.m4		\
	intltool-extract.in	\
	intltool-update.in	\
	intltool-merge.in

DISTCLEANFILES = intltool-extract intltool-merge intltool-update doltcompile doltlibtool

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgoffice-@GOFFICE_API_VER@.pc

%-@GOFFICE_API_VER@.pc: %.pc
	cp $< $@

CLEANFILES = $(pkgconfig_DATA)

include $(top_srcdir)/goffice.mk
