## Process this file with automake to produce Makefile.in

MKHTML_OPTIONS=--path="$(abs_builddir)"

DOC_MODULE=goffice-@GOFFICE_API_VER@

DOC_MAIN_SGML_FILE=goffice-docs.xml

DOC_SOURCE_DIR=$(top_srcdir)/goffice

SCANGOBJ_OPTIONS=

SCAN_OPTIONS= --deprecated-guards="GOFFICE_DISABLE_DEPRECATED" --rebuild-types

MKDB_OPTIONS=--xml-mode --output-format=xml

MKTMPL_OPTIONS=

FIXXREF_OPTIONS=

HFILE_GLOB=$(top_srcdir)/goffice/*.h $(top_srcdir)/goffice/*/*.h
CFILE_GLOB=$(top_srcdir)/goffice/*.c $(top_srcdir)/goffice/*/*.c

IGNORE_HFILES=	\
		goffice-priv.h 		 		\
		goffice-config.h        		\
		goffice-features.h      		\
		goffice-paths.h				\
		goffice-debug.h				\
		io-context-priv.h       		\
		go-app.h       				\
		go-object.h       			\
		go-service.h       			\
		go-service-impl.h

if GOFFICE_WITH_LASEM
else
IGNORE_HFILES += gog-equation.h
endif

HTML_IMAGES=

content_files =
expand_content_files=

GTKDOC_CFLAGS = -I$(top_srcdir) -I$(top_builddir) $(GOFFICE_CFLAGS)
GTKDOC_LIBS = $(top_builddir)/goffice/libgoffice-@GOFFICE_API_VER@.la $(GOFFICE_LIBS)

include $(top_srcdir)/gtk-doc.make

EXTRA_DIST += version.xml.in

CLEANFILES += \
	goffice-@GOFFICE_API_VER@-decl-list.txt	\
	goffice-@GOFFICE_API_VER@-unused.txt	\
	goffice-@GOFFICE_API_VER@-decl.txt   	\
	goffice-@GOFFICE_API_VER@-undeclared.txt	\
	goffice-@GOFFICE_API_VER@-overrides.txt	\
	goffice-@GOFFICE_API_VER@-undocumented.txt	\
	goffice-@GOFFICE_API_VER@.types	\
	html/* tmpl/* xml/*
