goffice_graph_xydir = $(goffice_plugindir)/plot_xy
xmldir	 = $(goffice_graph_xydir)

goffice_graph_xy_LTLIBRARIES = xy.la
xy_la_LDFLAGS = -module $(GOFFICE_PLUGIN_FLAGS)
xy_la_LIBADD = $(GOFFICE_PLUGIN_LIBADD)

xy_la_SOURCES = \
	gog-xy.c	\
	gog-xy.h \
	gog-xy-dropbar.c	\
	gog-xy-dropbar.h \
	gog-xy-minmax.c	\
	gog-xy-minmax.h

xml_in_files = plugin.xml.in plot-types.xml.in
xml_DATA = $(xml_in_files:.xml.in=.xml)

@INTLTOOL_XML_RULE@

if WITH_GTK
xy_la_SOURCES  += gog-bubble-prefs.c
endif

plugin_resources = bubble.xpm color.xpm dropbar.xpm minmax.xpm scatter.xpm
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
	chart_bubble_1_1.png	\
	chart_bubble_1_2.png	\
	chart_colored_1_1.png	\
	chart_dropbar_1_3.png	\
	chart_dropbar_1_4.png	\
	chart_minmax_1_3.png	\
	chart_minmax_1_4.png	\
	chart_minmax_2_3.png	\
	chart_minmax_2_4.png	\
	chart_scatter_1_1.png	\
	chart_scatter_2_1.png	\
	chart_scatter_2_2.png	\
	chart_scatter_3_1.png	\
	chart_scatter_3_2.png	\
	chart_scatter_3_3.png	\
	chart_scatter_4_1.png	\
	chart_scatter_4_2.png	\
	chart_scatter_4_3.png	\
	chart_scatter_4_4.png

embedded_stuff_compress = \
	gog-bubble-prefs.ui	\
	gog-xy-prefs.ui	\
	gog-xy-color-prefs.ui	\
	gog-xy-series-prefs.ui	\
	gog-xy-dropbar-prefs.ui

embedded_stuff = $(embedded_stuff_compress) $(embedded_stuff_raw)

embedded-stuff.c: $(top_srcdir)/tools/embedder $(embedded_stuff)
	cd $(srcdir) && @PERL@ $(abs_top_srcdir)/tools/embedder \
		--id-prefix=go:plot_xy/ \
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
