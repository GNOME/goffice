goffice_graph_piedir = $(goffice_plugindir)/plot_pie
xmldir	 = $(goffice_graph_piedir)
uidir = $(goffice_graph_piedir)

goffice_graph_pie_LTLIBRARIES = pie.la
pie_la_LDFLAGS = -module $(GOFFICE_PLUGIN_FLAGS)
pie_la_LIBADD = $(GOFFICE_PLUGIN_LIBADD)

pie_la_SOURCES = \
	gog-pie.c	\
	gog-pie.h

xml_in_files = plugin.xml.in plot-types.xml.in
xml_DATA = $(xml_in_files:.xml.in=.xml)

@INTLTOOL_XML_RULE@

if WITH_GTK
dist_ui_DATA = gog-pie-prefs.ui gog-ring-prefs.ui gog-pie-series.ui
pie_la_SOURCES += gog-pie-prefs.c
endif

# do not use the intl-tool stuff to merge the text back
# its simpler to just use gettext directly
plot-types.xml : plot-types.xml.in
	cp $< $@

EXTRA_DIST = $(xml_in_files)
DISTCLEANFILES = $(xml_in_files:.xml.in=.xml)

include $(top_srcdir)/goffice-plugins.mk
