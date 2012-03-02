goffice_graph_distribdir = $(goffice_plugindir)/plot_distrib
xmldir	 = $(goffice_graph_distribdir)

goffice_graph_distrib_LTLIBRARIES = distrib.la
distrib_la_LDFLAGS = -module $(GOFFICE_PLUGIN_FLAGS)
distrib_la_LIBADD = $(GOFFICE_PLUGIN_LIBADD)

distrib_la_SOURCES = gog-boxplot.c	\
		     gog-boxplot.h	\
		     gog-histogram.c	\
		     gog-histogram.h	\
			 gog-probability-plot.c	\
			 gog-probability-plot.h \
			 plugin.c

xml_in_files = plugin.xml.in plot-types.xml.in
xml_DATA = $(xml_in_files:.xml.in=.xml)

if WITH_GTK
  distrib_la_SOURCES +=	\
	go-distribution-prefs.c	\
	go-distribution-prefs.h
endif
@INTLTOOL_XML_RULE@

plugin_resources = dist.xpm
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
	chart_boxplot_1_1.png	\
	chart_boxplot_1_2.png	\
	chart_boxplot_2_1.png	\
	chart_boxplot_2_2.png	\
	chart_histogram_1_1.png	\
	chart_histogram_1_2.png	\
	chart_histogram_1_3.png	\
	chart_histogram_1_4.png	\
	chart_histogram_2_1.png	\
	chart_prob_1_1.png

embedded_stuff_compress = \
	gog-boxplot-prefs.ui \
	gog-histogram-prefs.ui \
	gog-double-histogram-prefs.ui

embedded_stuff = $(embedded_stuff_compress) $(embedded_stuff_raw)

embedded-stuff.c: $(top_srcdir)/tools/embedder $(embedded_stuff)
	cd $(srcdir) && @PERL@ $(abs_top_srcdir)/tools/embedder \
		--id-prefix=go:plot_distrib/ \
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
