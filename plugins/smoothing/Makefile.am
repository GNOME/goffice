goffice_graph_smoothingdir = $(goffice_plugindir)/smoothing
xmldir	 = $(goffice_graph_smoothingdir)

goffice_graph_smoothing_LTLIBRARIES = smoothing.la
smoothing_la_LDFLAGS = -module $(GOFFICE_PLUGIN_FLAGS)
smoothing_la_LIBADD = $(GOFFICE_PLUGIN_LIBADD)

smoothing_la_SOURCES =	\
	gog-moving-avg.c	\
	gog-moving-avg.h	\
	gog-exp-smooth.c	\
	gog-exp-smooth.h	\
	plugin.c

xml_in_files = plugin.xml.in types.xml.in
xml_DATA = $(xml_in_files:.xml.in=.xml)

@INTLTOOL_XML_RULE@

# Do not use the intl-tool stuff to merge the text back;
# it's simpler to just use gettext directly
types.xml : types.xml.in
	cp $< $@

embedded_stuff_raw =

embedded_stuff_compress = \
	gog-moving-avg.ui	\
	gog-exp-smooth.ui

embedded_stuff = $(embedded_stuff_compress) $(embedded_stuff_raw)

embedded-stuff.c: $(top_srcdir)/tools/embedder $(embedded_stuff)
	cd $(srcdir) && @PERL@ $(abs_top_srcdir)/tools/embedder \
		--id-prefix=go:smoothing/ \
		--static \
		--register-function=register_embedded_stuff \
		--unregister-function=unregister_embedded_stuff \
		NOCOMPRESS $(embedded_stuff_raw) \
		COMPRESS $(embedded_stuff_compress) \
		>$(abs_builddir)/$@

BUILT_SOURCES =	embedded-stuff.c
CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = $(xml_in_files) $(embedded_stuff)
DISTCLEANFILES = $(xml_in_files:.xml.in=.xml)

include $(top_srcdir)/goffice-plugins.mk
