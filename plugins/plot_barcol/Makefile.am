goffice_graph_barcoldir = $(goffice_plugindir)/plot_barcol
xmldir	 = $(goffice_graph_barcoldir)

goffice_graph_barcol_LTLIBRARIES = barcol.la
barcol_la_LDFLAGS = -module $(GOFFICE_PLUGIN_FLAGS)
barcol_la_LIBADD = $(GOFFICE_PLUGIN_LIBADD)

barcol_la_SOURCES = gog-1.5d.c 		\
		    gog-1.5d.h		\
		    gog-line.c		\
		    gog-line.h		\
		    gog-barcol.c 	\
		    gog-barcol.h	\
		    gog-dropbar.c	\
		    gog-dropbar.h	\
		    gog-minmax.c	\
		    gog-minmax.h	

xml_in_files = plugin.xml.in plot-types.xml.in
xml_DATA = $(xml_in_files:.xml.in=.xml)

@INTLTOOL_XML_RULE@

if WITH_GTK
barcol_la_SOURCES += gog-barcol-prefs.c
endif

plugin_resources = linegraph.xpm area.xpm bar.xpm column.xpm dropbar.xpm minmax.xpm
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
	chart_area_1_1.png	\
	chart_area_1_2.png	\
	chart_area_1_3.png	\
	chart_bar_1_1.png	\
	chart_bar_1_2.png	\
	chart_bar_1_3.png	\
	chart_bar_2_1.png	\
	chart_bar_2_2.png	\
	chart_bar_2_3.png	\
	chart_column_1_1.png	\
	chart_column_1_2.png	\
	chart_column_1_3.png	\
	chart_column_2_1.png	\
	chart_column_2_2.png	\
	chart_column_2_3.png	\
	chart_column_3_1.png	\
	chart_dropbar_1_1.png	\
	chart_dropbar_1_2.png	\
	chart_line_1_1.png	\
	chart_line_1_2.png	\
	chart_line_1_3.png	\
	chart_line_2_1.png	\
	chart_line_2_2.png	\
	chart_line_2_3.png	\
	chart_line_3_1.png	\
	chart_minmax_1_1.png	\
	chart_minmax_1_2.png	\
	chart_minmax_2_1.png	\
	chart_minmax_2_2.png

embedded_stuff_compress = \
	gog-barcol-prefs.ui	\
	gog-minmax-prefs.ui	\
	gog-area-prefs.ui

embedded_stuff = $(embedded_stuff_compress) $(embedded_stuff_raw)

embedded-stuff.c: $(top_srcdir)/tools/embedder $(embedded_stuff)
	cd $(srcdir) && @PERL@ $(abs_top_srcdir)/tools/embedder \
		--id-prefix=go:plot_barcol/ \
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
