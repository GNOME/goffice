goffice_lasemdir = $(goffice_plugindir)/lasem
xmldir	 = $(goffice_lasemdir)

goffice_lasem_LTLIBRARIES = lasem.la
lasem_la_LDFLAGS = -module $(GOFFICE_PLUGIN_FLAGS)
lasem_la_LIBADD = $(GOFFICE_PLUGIN_LIBADD)

lasem_la_SOURCES =	\
	component.c

xml_in_files = plugin.xml.in
xml_DATA = $(xml_in_files:.xml.in=.xml)

@INTLTOOL_XML_RULE@

EXTRA_DIST = $(xml_in_files)
DISTCLEANFILES = $(xml_in_files:.xml.in=.xml)

include $(top_srcdir)/goffice-plugins.mk
