goffice_graph_distribdir = $(goffice_plugindir)/plot_distrib
xmldir	 = $(goffice_graph_distribdir)
uidir = $(goffice_graph_distribdir)

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
ui_DATA = \
	gog-boxplot-prefs.ui \
	gog-histogram-prefs.ui \
	gog-double-histogram-prefs.ui

@INTLTOOL_XML_RULE@

# do not use the intl-tool stuff to merge the text back
# its simpler to just use gettext directly
plot-types.xml : plot-types.xml.in
	cp $< $@

EXTRA_DIST = $(xml_in_files) $(ui_DATA)
DISTCLEANFILES = $(xml_in_files:.xml.in=.xml)

include $(top_srcdir)/goffice-plugins.mk
