goffice_graph_smoothingdir = $(goffice_plugindir)/smoothing
xmldir	 = $(goffice_graph_smoothingdir)
uidir = $(goffice_graph_smoothingdir)

goffice_graph_smoothing_LTLIBRARIES = smoothing.la
smoothing_la_LDFLAGS = -module $(GOFFICE_PLUGIN_FLAGS)
smoothing_la_LIBADD = $(GOFFICE_PLUGIN_LIBADD)

smoothing_la_SOURCES =	\
			gog-moving-avg.c	\
		    gog-moving-avg.h \
			gog-exp-smooth.c	\
		    gog-exp-smooth.h \
			plugin.c

xml_in_files = plugin.xml.in types.xml.in
xml_DATA = $(xml_in_files:.xml.in=.xml)

@INTLTOOL_XML_RULE@

dist_ui_DATA =	\
			gog-moving-avg.ui	\
			gog-exp-smooth.ui

# do not use the intl-tool stuff to merge the text back
# its simpler to just use gettext directly
types.xml : types.xml.in
	cp $< $@

EXTRA_DIST = $(xml_in_files)
DISTCLEANFILES = $(xml_in_files:.xml.in=.xml)

include $(top_srcdir)/goffice-plugins.mk
