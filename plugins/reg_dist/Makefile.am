goffice_graph_regdistdir = $(goffice_plugindir)/reg_dist
xmldir	 = $(goffice_graph_regdistdir)

goffice_graph_regdist_LTLIBRARIES = distreg.la
distreg_la_LDFLAGS = -module $(GOFFICE_PLUGIN_FLAGS)
distreg_la_LIBADD = $(GOFFICE_PLUGIN_LIBADD)

distreg_la_SOURCES =	\
			gog-gaussian.c	\
		    gog-gaussian.h \
			plugin.c

xml_in_files = plugin.xml.in reg-types.xml.in
xml_DATA = $(xml_in_files:.xml.in=.xml)

@INTLTOOL_XML_RULE@

# do not use the intl-tool stuff to merge the text back
# its simpler to just use gettext directly
reg-types.xml : reg-types.xml.in
	cp $< $@

EXTRA_DIST = $(xml_in_files)
DISTCLEANFILES = $(xml_in_files:.xml.in=.xml)

include $(top_srcdir)/goffice-plugins.mk
