INCLUDES = -I$(top_builddir)			\
	   -I$(top_srcdir)			\
	   -I$(top_srcdir)/../foocanvas		\
	    $(GOFFICE_CFLAGS)

GOFFICE_PLUGIN_FLAGS = $(GOFFICE_PLUGIN_LDFLAGS)
goffice_include_dir = $(includedir)/libgoffice-1/goffice
