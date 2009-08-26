goffice_graph_surfacedir = $(goffice_plugindir)/plot_surface
xmldir	 = $(goffice_graph_surfacedir)
uidir = $(goffice_graph_surfacedir)

goffice_graph_surface_LTLIBRARIES = surface.la
surface_la_LDFLAGS = -module $(GOFFICE_PLUGIN_FLAGS)
surface_la_LIBADD = $(GOFFICE_PLUGIN_LIBADD) 

surface_la_SOURCES = \
	gog-xyz.c		\
	gog-xyz.h		\
	gog-contour.c		\
	gog-contour.h		\
	gog-surface.c		\
	gog-surface.h		\
	gog-xyz-surface.c	\
	gog-xyz-surface.h	\
	xl-surface.c		\
	xl-surface.h

xml_in_files = plugin.xml.in plot-types.xml.in
xml_DATA = $(xml_in_files:.xml.in=.xml)

@INTLTOOL_XML_RULE@

if WITH_GTK
dist_ui_DATA = \
	gog-xyz-prefs.ui	\
	gog-xyz-surface-prefs.ui

surface_la_SOURCES += \
	gog-xyz-prefs.c		\
	gog-xyz-surface-prefs.c
endif

# do not use the intl-tool stuff to merge the text back
# its simpler to just use gettext directly
plot-types.xml : plot-types.xml.in
	cp $< $@

EXTRA_DIST = $(xml_in_files)
DISTCLEANFILES = $(xml_in_files:.xml.in=.xml)

include $(top_srcdir)/goffice-plugins.mk
