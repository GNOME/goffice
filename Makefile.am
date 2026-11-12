# Makefile.am for goffice

SUBDIRS = po goffice tests plugins mmlitex docs tools

itsdir = $(datadir)/gettext/its
its_DATA = its/goffice-plugin.its its/goffice-plugin.loc

EXTRA_DIST = README NEWS BUGS MAINTAINERS AUTHORS \
	COPYING-gpl2 COPYING-gpl3 \
	goffice.mk \
	goffice-plugins.mk \
	libgoffice.pc.in \
	$(its_DATA)

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
