AM_CPPFLAGS = \
	-I$(top_builddir)		\
	-I$(top_srcdir)			\
	$(GOFFICE_CFLAGS)

GOFFICE_PLUGIN_FLAGS = $(GOFFICE_PLUGIN_LDFLAGS)
goffice_include_dir = $(includedir)/libgoffice-0.3/goffice
