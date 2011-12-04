goffice_graph_logfitdir = $(goffice_plugindir)/reg_logfit
xmldir	 = $(goffice_graph_logfitdir)

goffice_graph_logfit_LTLIBRARIES = logfit.la
logfit_la_LDFLAGS = -module $(GOFFICE_PLUGIN_FLAGS)
logfit_la_LIBADD = $(GOFFICE_PLUGIN_LIBADD)

logfit_la_SOURCES =	\
			gog-logfit.c	\
			gog-logfit.h

xml_in_files = plugin.xml.in reg-types.xml.in
xml_DATA = $(xml_in_files:.xml.in=.xml)

@INTLTOOL_XML_RULE@

# Do not use the intl-tool stuff to merge the text back;
# it's simpler to just use gettext directly
reg-types.xml : reg-types.xml.in
	cp $< $@

EXTRA_DIST = $(xml_in_files)
DISTCLEANFILES = $(xml_in_files:.xml.in=.xml)

include $(top_srcdir)/goffice-plugins.mk
