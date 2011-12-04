goffice_graph_linregdir = $(goffice_plugindir)/reg_linear
xmldir	 = $(goffice_graph_linregdir)

goffice_graph_linreg_LTLIBRARIES = linreg.la
linreg_la_LDFLAGS = -module $(GOFFICE_PLUGIN_FLAGS)
linreg_la_LIBADD = $(GOFFICE_PLUGIN_LIBADD)

linreg_la_SOURCES =	\
			gog-lin-reg.c	\
			gog-lin-reg.h	\
			gog-polynom-reg.c	\
			gog-polynom-reg.h	\
			gog-log-reg.c	\
			gog-log-reg.h	\
			gog-exp-reg.c	\
			gog-exp-reg.h	\
			gog-power-reg.c	\
			gog-power-reg.h

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
