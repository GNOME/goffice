INCLUDES = \
    -I$(top_srcdir)					\
    -I$(top_srcdir)/src					\
    -I$(top_builddir)/src				\
    -I$(top_srcdir)/src/cut-n-paste-code		\
    -I$(top_srcdir)/src/cut-n-paste-code/foocanvas	\
    $(GOFFICE_CFLAGS)

GOFFICE_PLUGIN_FLAGS = $(GOFFICE_PLUGIN_LDFLAGS)
