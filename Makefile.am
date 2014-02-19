# Makefile.am for goffice

SUBDIRS = goffice tests plugins mmlitex po docs tools

EXTRA_DIST = README NEWS BUGS MAINTAINERS AUTHORS \
	COPYING-gpl2 COPYING-gpl3 \
	goffice.mk \
	goffice-plugins.mk \
	libgoffice.pc.in \
	intltool-extract.in	\
	intltool-update.in	\
	intltool-merge.in

DISTCLEANFILES = intltool-extract intltool-merge intltool-update

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc \
	--enable-introspection=auto \
	--with-gir-dir=\$${datadir}/gir-1.0 \
	--with-typelib-dir=\$${libdir}/girepository-1.0

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgoffice-@GOFFICE_API_VER@.pc

%-@GOFFICE_API_VER@.pc: %.pc
	cp $< $@

CLEANFILES = $(pkgconfig_DATA)

include $(top_srcdir)/goffice.mk
