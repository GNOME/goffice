INCLUDES = \
	-I$(top_srcdir) 				\
	-I$(top_builddir)				\
	$(WARN_CFLAGS) 					\
	$(GOFFICE_CFLAGS)				\
        -DFOOCANVASLIBDIR=\""$(libdir)"\" 		\
        -DFOOCANVASDATADIR=\""$(datadir)"\" 		\
        -DFOOCANVASPIXMAPDIR=\""$(datadir)/pixmaps"\"	\
        -DFOOCANVASBINDIR=\""$(bindir)"\" 		\
        -DFOOCANVASLOCALSTATEDIR=\""$(localstatedir)"\"	\
        -DFOOCANVASLOCALEDIR=\""$(gnomelocaledir)"\" 	\
	-DG_LOG_DOMAIN=\"Foocanvas\"			\
	-DVERSION=\"$(VERSION)\"

noinst_LTLIBRARIES = libfoocanvas.la

libfoocanvas_la_LIBADD = \
	$(RENDER_LIBS)				\
	$(LIBFOOCANVAS_LIBS)

libfoocanvasincludedir = $(includedir)/libgoffice-1/goffice/cut-n-paste/foocanvas/

libfoocanvasinclude_HEADERS =			\
	foo-canvas-line.h			\
	foo-canvas-pixbuf.h			\
	foo-canvas-polygon.h			\
	foo-canvas-rect-ellipse.h		\
	foo-canvas-text.h			\
	foo-canvas-util.h			\
	foo-canvas-widget.h			\
	foo-canvas.h				\
	libfoocanvas.h

libfoocanvas_la_SOURCES =			\
	$(libfoocanvasinclude_HEADERS)		\
	foo-canvas-marshal.list			\
	foo-canvas-i18n.h			\
	foo-canvas-line.c			\
	foo-canvas-pixbuf.c			\
	foo-canvas-polygon.c			\
	foo-canvas-rect-ellipse.c		\
	foo-canvas-text.c			\
	foo-canvas-util.c			\
	foo-canvas-widget.c			\
	foo-canvas.c				\
	libfoocanvastypes.c

GENMARSHAL_COMMAND = $(GLIB_GENMARSHAL) --prefix=foo_canvas_marshal
SUFFIXES = .list

.list.h: $(GLIB_GENMARSHAL)
	$(GENMARSHAL_COMMAND) --header $< >$@

.list.c: $(GLIB_GENMARSHAL)
	(echo '/* This file has been automatically generated.  Do not edit. */' && \
	echo '#include "$*.h"' && \
	$(GENMARSHAL_COMMAND) --body $< ) >$@

# A hint is needed to build the header first:
BUILT_SOURCES = foo-canvas-marshal.h

CLEANFILES = foo-canvas-marshal.h foo-canvas-marshal.c

