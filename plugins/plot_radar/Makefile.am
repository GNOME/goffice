goffice_graph_radardir = $(goffice_plugindir)/plot_radar
xmldir	 = $(goffice_graph_radardir)

goffice_graph_radar_LTLIBRARIES = radar.la
radar_la_LDFLAGS = -module $(GOFFICE_PLUGIN_FLAGS)
radar_la_LIBADD = $(GOFFICE_PLUGIN_LIBADD)

radar_la_SOURCES = \
	gog-radar.c		\
	gog-radar.h		

xml_in_files = plugin.xml.in plot-types.xml.in
xml_DATA = $(xml_in_files:.xml.in=.xml)

@INTLTOOL_XML_RULE@

plugin_resources = radar.xpm polar.xpm
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
	chart_color_polar_1_1.png	\
	chart_polar_1_1.png		\
	chart_radar_1_1.png		\
	chart_radar_1_2.png		\
	chart_radar_1_3.png

embedded_stuff_compress = \
	gog-color-polar-prefs.ui \
	gog-polar-prefs.ui


embedded_stuff = $(embedded_stuff_compress) $(embedded_stuff_raw)

embedded-stuff.c: $(top_srcdir)/tools/embedder $(embedded_stuff)
	cd $(srcdir) && @PERL@ $(abs_top_srcdir)/tools/embedder \
		--id-prefix=go:plot_radar/ \
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
