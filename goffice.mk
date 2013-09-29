AM_CPPFLAGS = \
	-I$(top_builddir)		\
	-I$(top_srcdir)			\
	$(GOFFICE_CFLAGS)		\
	$(GTK_MAC_CFLAGS)		\
	-DGOFFICE_COMPILATION

GOFFICE_PLUGIN_FLAGS = $(GOFFICE_PLUGIN_LDFLAGS)
goffice_include_dir = $(includedir)/libgoffice-@GOFFICE_API_VER@/goffice
