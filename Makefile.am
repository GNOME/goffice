# Makefile.am for goffice

SUBDIRS = goffice tests pixmaps plugins po docs tools

EXTRA_DIST = README NEWS BUGS MAINTAINERS AUTHORS \
	goffice.mk \
	goffice-plugins.mk \
	intltool-extract.in	\
	intltool-update.in	\
	intltool-merge.in

DISTCLEANFILES = intltool-extract intltool-merge intltool-update

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgoffice-0.5.pc

include $(top_srcdir)/goffice.mk
