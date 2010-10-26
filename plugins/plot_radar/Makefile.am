goffice_graph_radardir = $(goffice_plugindir)/plot_radar
xmldir	 = $(goffice_graph_radardir)
uidir = $(goffice_graph_radardir)

goffice_graph_radar_LTLIBRARIES = radar.la
radar_la_LDFLAGS = -module $(GOFFICE_PLUGIN_FLAGS)
radar_la_LIBADD = $(GOFFICE_PLUGIN_LIBADD)

radar_la_SOURCES = \
	gog-radar.c		\
	gog-radar.h		

xml_in_files = plugin.xml.in plot-types.xml.in
xml_DATA = $(xml_in_files:.xml.in=.xml)

ui_DATA = gog-color-polar-prefs.ui gog-polar-prefs.ui

@INTLTOOL_XML_RULE@

# do not use the intl-tool stuff to merge the text back
# its simpler to just use gettext directly
plot-types.xml : plot-types.xml.in
	cp $< $@

EXTRA_DIST = $(xml_in_files) $(ui_DATA)
DISTCLEANFILES = $(xml_in_files:.xml.in=.xml)

include $(top_srcdir)/goffice-plugins.mk
