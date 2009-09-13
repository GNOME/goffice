## Process this file with automake to produce Makefile.in

DOC_MODULE=goffice-@GOFFICE_API_VER@

DOC_MAIN_SGML_FILE=goffice-docs.sgml

DOC_SOURCE_DIR=../../goffice

SCANGOBJ_OPTIONS= --nogtkinit --type-init-func="g_type_init ()"

SCAN_OPTIONS=--rebuild-types

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
		go-app.h       				\
		go-object.h       			\
		go-service.h       			\
		go-service-impl.h      			\
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
		god-drawing-ms-client-handler.h

if GOFFICE_WITH_LASEM
else
IGNORE_HFILES += gog-equation.h
endif

HTML_IMAGES=

content_files = \
        $(top_srcdir)/docs/reference/version.xml

expand_content_files=

GTKDOC_CFLAGS = -I$(top_srcdir) -I$(top_builddir) $(GOFFICE_CFLAGS)
GTKDOC_LIBS = $(top_builddir)/goffice/libgoffice-@GOFFICE_API_VER@.la $(GOFFICE_LIBS)

include $(top_srcdir)/gtk-doc.make

# To work around bug #314105, we have to distribute the generated file, too.
EXTRA_DIST += version.xml.in version.xml

CLEANFILES += \
	goffice-@GOFFICE_API_VER@-decl-list.txt	\
	goffice-@GOFFICE_API_VER@-unused.txt	\
	goffice-@GOFFICE_API_VER@-decl.txt   	\
	goffice-@GOFFICE_API_VER@-undeclared.txt	\
	goffice-@GOFFICE_API_VER@-overrides.txt	\
	goffice-@GOFFICE_API_VER@-undocumented.txt	\
	goffice-@GOFFICE_API_VER@.types	\
	html/* tmpl/* xml/*

$(top_srcdir)/docs/reference/version.xml:
	cp $(top_builddir)/docs/reference/version.xml $@
