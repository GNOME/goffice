goffice_graph_surfacedir = $(goffice_plugindir)/plot_surface
xmldir	 = $(goffice_graph_surfacedir)

goffice_graph_surface_LTLIBRARIES = surface.la
surface_la_LDFLAGS = -module $(GOFFICE_PLUGIN_FLAGS)
surface_la_LIBADD = $(GOFFICE_PLUGIN_LIBADD)

surface_la_SOURCES = \
	gog-xyz.c		\
	gog-xyz.h		\
	gog-contour.c		\
	gog-contour.h		\
	gog-matrix.c		\
	gog-matrix.h		\
	gog-surface.c		\
	gog-surface.h		\
	gog-xyz-surface.c	\
	gog-xyz-surface.h	\
	xl-surface.c		\
	xl-surface.h

xml_in_files = plugin.xml.in plot-types.xml.in
xml_DATA = $(xml_in_files:.xml.in=.xml)

@INTLTOOL_XML_RULE@

if WITH_GTK
surface_la_SOURCES += \
	gog-xyz-prefs.c		\
	gog-xyz-surface-prefs.c
endif

plugin_resources = contour.xpm matrix.xpm surface.xpm
pofiles = $(wildcard $(top_srcdir)/po/*.po)

plugin.xml: plugin.xml.in $(plugin_resources) $(pofiles) $(top_srcdir)/tools/resource-embedder
	LC_ALL=C $(INTLTOOL_MERGE) -x -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@.tmp
	@PERL@ $(abs_top_srcdir)/tools/resource-embedder --file-dir $(srcdir) \
		$@.tmp $@
	rm -f $@.tmp

# Do not use the intl-tool stuff to merge the text back;
# it's simpler to just use gettext directly
plot-types.xml : plot-types.xml.in
	cp $< $@

embedded_stuff_raw = \
	chart_contour_1_1.png	\
	chart_contour_2_1.png	\
	chart_contour_2_2.png	\
	chart_matrix_1_1.png	\
	chart_matrix_1_2.png	\
	chart_matrix_1_3.png	\
	chart_surface_2_1.png	\
	chart_surface_2_2.png	\
	chart_surface_2_3.png

embedded_stuff_compress = \
	gog-xyz-prefs.ui	\
	gog-xyz-surface-prefs.ui

embedded_stuff = $(embedded_stuff_compress) $(embedded_stuff_raw)

embedded-stuff.c: $(top_srcdir)/tools/embedder $(embedded_stuff)
	cd $(srcdir) && @PERL@ $(abs_top_srcdir)/tools/embedder \
		--id-prefix=go:plot_surface/ \
		--static \
		--register-function=register_embedded_stuff \
		--unregister-function=unregister_embedded_stuff \
		NOCOMPRESS $(embedded_stuff_raw) \
		COMPRESS $(embedded_stuff_compress) \
		>$(abs_builddir)/$@

BUILT_SOURCES =	embedded-stuff.c
CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = $(xml_in_files) $(embedded_stuff) $(plugin_resources)
DISTCLEANFILES = $(xml_in_files:.xml.in=.xml)

include $(top_srcdir)/goffice-plugins.mk
