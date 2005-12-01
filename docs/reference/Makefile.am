## Process this file with automake to produce Makefile.in

DOC_MODULE=goffice

DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml

DOC_SOURCE_DIR=../../goffice

SCANGOBJ_OPTIONS= --nogtkinit --type-init-func="g_type_init ()"

SCAN_OPTIONS= 

MKDB_OPTIONS=--sgml-mode --output-format=xml

MKTMPL_OPTIONS=

FIXXREF_OPTIONS=

HFILE_GLOB=$(top_srcdir)/goffice/*.h $(top_srcdir)/goffice/*/*.h
CFILE_GLOB=$(top_srcdir)/goffice/*.c $(top_srcdir)/goffice/*/*.c

IGNORE_HFILES=	\
		goffice-priv.h 		 		\
		goffice-config.h        		\
		goffice-features.h      		\
		goffice-paths.h				\
		io-context-priv.h       		\
		god-anchor.h				\
		god-default-attributes.h		\
		god-drawing-group.h			\
		god-drawing-renderer-gdk.h		\
		god-drawing-renderer-cairo.h		\
		god-drawing-view.h			\
		god-drawing.h				\
		god-image-store.h			\
		god-image.h				\
		god-paragraph-attributes.h		\
		god-property-table.h			\
		god-shape.h				\
		god-text-model.h			\
		god-image-ms.h				\
		god-drawing-ms.h			\
		god-drawing-ms-client-handler.h 	\
		foo-canvas-line.h			\
		foo-canvas-pixbuf.h			\
		foo-canvas-polygon.h			\
		foo-canvas-rect-ellipse.h		\
		foo-canvas-text.h			\
		foo-canvas-util.h			\
		foo-canvas-widget.h			\
		foo-canvas.h				\
		libfoocanvas.h				\
		foo-canvas-i18n.h			\
		foo-canvas-marshal.h			\
		pcre.h					\
		pcre_internal.h				\
		ucp.h
	
HTML_IMAGES=

content_files=

expand_content_files=

INCLUDES = -I$(top_srcdir) $(GOFFICE_CFLAGS)
GTKDOC_LIBS = $(top_builddir)/goffice/libgoffice-1.la $(GOFFICE_LIBS)

include $(top_srcdir)/gtk-doc.make

EXTRA_DIST += 
