AUTOMAKE_OPTIONS = subdir-objects

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = \
	--add-include-path="${INTROSPECTION_GIRDIR}" \
	-v --warn-all --accept-unprefixed -DGOFFICE_DISABLE_DEPRECATED \
	--namespace GOffice --nsversion="@GOFFICE_API_VER@" \
	--identifier-filter-cmd="$(top_srcdir)/tools/introspection-identifier-filter" \
	--identifier-prefix="GO"  --identifier-prefix="Go" \
	--symbol-prefix="go" --symbol-prefix="libgoffice"

INTROSPECTION_COMPILER_ARGS = \
	--includedir=$(srcdir) \
	--includedir=. \
	--includedir="${INTROSPECTION_GIRDIR}"

#####################################
#app directory files

noinst_HEADERS = \
	app/file-priv.h				\
	math/go-ryu.h				\
	goffice-debug.h				\
	goffice-multipass.h

app_SOURCES =					\
	app/go-doc.c				\
	app/go-doc-control.c			\
	app/go-view.c				\
						\
	app/go-plugin.c				\
	app/go-plugin-loader.c			\
	app/go-plugin-loader-module.c		\
	app/go-plugin-service.c			\
						\
	app/file.c				\
	app/error-info.c			\
	app/io-context.c			\
	app/go-cmd-context.c			\
	app/go-conf.c

appdir = $(goffice_include_dir)/app
app_api_headers =				\
	app/goffice-app.h			\
						\
	app/go-doc.h				\
	app/go-doc-impl.h			\
	app/go-doc-control.h			\
	app/go-doc-control-impl.h		\
	app/go-view.h				\
	app/go-plugin.h				\
	app/go-plugin-loader.h			\
	app/go-plugin-loader-module.h		\
	app/go-plugin-service.h			\
	app/go-plugin-service-impl.h		\
	app/file.h				\
	app/error-info.h			\
	app/io-context.h			\
	app/io-context-priv.h			\
	app/go-cmd-context.h			\
	app/go-cmd-context-impl.h		\
	app/go-conf.h

app_HEADERS = 	\
	$(app_api_headers) \
	app/module-plugin-defs.h

# This list should be empty when we reach a stable phase.
app_UNUSED = \
	app/go-app.h \
	app/go-error-stack.h \
	app/go-object.c \
	app/go-object.h \
	app/go-service.c \
	app/go-service.h \
	app/go-service-impl.h

EXTRA_DIST = $(app_UNUSED)		\
	app/go-conf-keyfile.c		\
	app/go-conf-gsettings.c		\
	app/go-conf-win32.c

#####################################
# canvas directory files

canvas_SOURCES =	\
	canvas/goc-arc.c		\
	canvas/goc-canvas.c		\
	canvas/goc-circle.c		\
	canvas/goc-ellipse.c		\
	canvas/goc-graph.c		\
	canvas/goc-group.c		\
	canvas/goc-item.c		\
	canvas/goc-image.c		\
	canvas/goc-line.c		\
	canvas/goc-path.c		\
	canvas/goc-pixbuf.c		\
	canvas/goc-polyline.c		\
	canvas/goc-polygon.c		\
	canvas/goc-rectangle.c		\
	canvas/goc-styled-item.c	\
	canvas/goc-text.c		\
	canvas/goc-utils.c

canvas_gtk_SOURCES= \
	canvas/goc-component.c	\
	canvas/goc-widget.c

canvasdir = $(goffice_include_dir)/canvas
canvas_HEADERS =	\
	canvas/goc-arc.h		\
	canvas/goc-canvas.h		\
	canvas/goc-circle.h		\
	canvas/goc-component.h		\
	canvas/goc-ellipse.h		\
	canvas/goc-graph.h		\
	canvas/goc-group.h		\
	canvas/goc-item.h		\
	canvas/goc-image.h		\
	canvas/goc-line.h		\
	canvas/goc-path.h		\
	canvas/goc-pixbuf.h		\
	canvas/goc-polyline.h		\
	canvas/goc-polygon.h		\
	canvas/goc-rectangle.h		\
	canvas/goc-structs.h		\
	canvas/goc-styled-item.h	\
	canvas/goc-text.h		\
	canvas/goc-utils.h		\
	canvas/goc-widget.h		\
	canvas/goffice-canvas.h

#####################################
# component directory files

component_SOURCES =				\
	component/go-component.c		\
	component/go-component-factory.c	\
	component/go-component-mime-dialog.c

componentdir = $(goffice_include_dir)/component
component_HEADERS =				\
	component/goffice-component.h		\
	component/go-component.h		\
	component/go-component-factory.h	\
	component/go-component-mime-dialog.h

#####################################
# data directory files

data_SOURCES =	\
	data/go-data.c 		\
	data/go-data-simple.c

go_datadir = $(goffice_include_dir)/data
go_data_HEADERS =				\
	data/goffice-data.h			\
	data/go-data.h				\
	data/go-data-impl.h			\
	data/go-data-simple.h

#####################################
# graph directory files

graph_SOURCES =					\
	graph/gog-object.c			\
	graph/gog-object-xml.c			\
	graph/gog-styled-object.c		\
	graph/gog-outlined-object.c		\
	graph/gog-view.c			\
						\
	graph/gog-graph.c			\
	graph/gog-chart.c			\
						\
	graph/gog-chart-map.c			\
	graph/gog-chart-map-3d.c		\
	graph/gog-3d-box.c			\
						\
	graph/gog-axis.c			\
	graph/gog-axis-line.c			\
	graph/gog-axis-color-map.c		\
	graph/gog-legend.c			\
	graph/gog-color-scale.c			\
	graph/gog-label.c			\
	graph/gog-grid.c			\
	graph/gog-grid-line.c			\
						\
	graph/gog-theme.c			\
						\
	graph/gog-plot.c			\
	graph/gog-plot-engine.c			\
	graph/gog-series.c			\
	graph/gog-error-bar.c			\
	graph/gog-trend-line.c			\
	graph/gog-reg-curve.c			\
	graph/gog-smoothed-curve.c		\
	graph/gog-series-labels.c		\
	graph/gog-series-lines.c		\
	graph/gog-data-set.c			\
						\
	graph/gog-renderer.c

graph_gtk_SOURCES =				\
	graph/gog-guru.c			\
	graph/gog-data-allocator.c		\
	graph/gog-child-button.c

graph_lasem_SOURCES = \
	graph/gog-equation.c

graphdir = $(goffice_include_dir)/graph
graph_HEADERS =					\
	graph/goffice-graph.h			\
	graph/gog-object.h			\
	graph/gog-object-xml.h			\
	graph/gog-styled-object.h		\
	graph/gog-outlined-object.h		\
	graph/gog-view.h			\
	graph/gog-graph.h			\
	graph/gog-graph-impl.h			\
	graph/gog-chart.h			\
	graph/gog-chart-impl.h			\
	graph/gog-chart-map.h			\
	graph/gog-chart-map-3d.h		\
	graph/gog-3d-box.h			\
	graph/gog-axis.h			\
	graph/gog-axis-line.h			\
	graph/gog-axis-color-map.h		\
	graph/gog-legend.h			\
	graph/gog-color-scale.h			\
	graph/gog-label.h			\
	graph/gog-grid.h			\
	graph/gog-grid-line.h			\
	graph/gog-theme.h			\
	graph/gog-plot.h			\
	graph/gog-plot-impl.h			\
	graph/gog-plot-engine.h			\
	graph/gog-series.h			\
	graph/gog-series-impl.h			\
	graph/gog-error-bar.h			\
	graph/gog-trend-line.h			\
	graph/gog-reg-curve.h			\
	graph/gog-smoothed-curve.h		\
	graph/gog-series-labels.h		\
	graph/gog-series-lines.h		\
	graph/gog-data-set.h			\
	graph/gog-renderer.h

graph_gtkdir = $(goffice_include_dir)/graph
graph_gtk_HEADERS =				\
	graph/gog-guru.h			\
	graph/gog-data-allocator.h		\
	graph/gog-child-button.h

graph_lasemdir = $(goffice_include_dir)/graph
graph_lasem_HEADERS = \
	graph/gog-equation.h

noinst_HEADERS += \
	graph/gog-axis-line-impl.h

#####################################
# gtk directory files

gtk_SOURCES =					\
	gtk/goffice-gtk.c			\
						\
	gtk/go-font-sel.c			\
	gtk/go-font-sel-dialog.c		\
	gtk/go-format-sel.c			\
	gtk/go-rotation-sel.c			\
	gtk/go-charmap-sel.c			\
	gtk/go-locale-sel.c			\
	gtk/go-3d-rotation-sel.c		\
	gtk/go-arrow-sel.c			\
						\
	gtk/go-optionmenu.c			\
	gtk/go-combo-box.c			\
	gtk/go-color-group.c			\
	gtk/go-color-palette.c			\
	gtk/go-combo-color.c			\
	gtk/go-combo-pixmaps.c			\
						\
	gtk/go-action-combo-color.c		\
	gtk/go-action-combo-pixmaps.c		\
	gtk/go-action-combo-stack.c		\
	gtk/go-action-combo-text.c		\
						\
	gtk/go-calendar-button.c		\
	gtk/go-palette.c			\
	gtk/go-selector.c			\
	gtk/go-color-selector.c			\
	gtk/go-gradient-selector.c		\
	gtk/go-line-selector.c			\
	gtk/go-marker-selector.c		\
	gtk/go-pattern-selector.c		\
						\
	gtk/go-graph-widget.c			\
	gtk/go-image-sel.c				\
	gtk/go-math-editor.c

gtkdir = $(goffice_include_dir)/gtk
gtk_HEADERS =					\
	gtk/goffice-gtk.h			\
						\
	gtk/go-font-sel.h			\
	gtk/go-font-sel-dialog.h		\
	gtk/go-format-sel.h			\
	gtk/go-rotation-sel.h			\
	gtk/go-charmap-sel.h			\
	gtk/go-locale-sel.h			\
	gtk/go-3d-rotation-sel.h		\
	gtk/go-arrow-sel.h			\
						\
	gtk/go-optionmenu.h			\
	gtk/go-combo-box.h			\
	gtk/go-color-group.h			\
	gtk/go-color-palette.h			\
	gtk/go-combo-color.h			\
	gtk/go-combo-pixmaps.h			\
						\
	gtk/go-action-combo-color.h		\
	gtk/go-action-combo-pixmaps.h		\
	gtk/go-action-combo-stack.h		\
	gtk/go-action-combo-text.h		\
						\
	gtk/go-calendar-button.h		\
	gtk/go-palette.h			\
	gtk/go-selector.h			\
	gtk/go-color-selector.h			\
	gtk/go-gradient-selector.h		\
	gtk/go-line-selector.h			\
	gtk/go-marker-selector.h		\
	gtk/go-pattern-selector.h		\
						\
	gtk/go-graph-widget.h			\
	gtk/go-image-sel.h				\
	gtk/go-math-editor.h

#####################################
# maths directory files

math_SOURCES =					\
	math/go-accumulator.c			\
	math/go-dtoa.c				\
	math/go-math.c				\
	math/go-rangefunc.c			\
	math/go-regression.c			\
	math/go-cspline.c			\
	math/go-complex.c			\
	math/go-fft.c				\
	math/go-matrix.c			\
	math/go-matrix3x3.c			\
	math/go-quad.c				\
	math/go-R.c				\
	math/go-ryu.c				\
	math/go-distribution.c

mathdir = $(goffice_include_dir)/math
math_HEADERS =					\
	math/goffice-math.h			\
	math/go-accumulator.h			\
	math/go-dtoa.h				\
	math/go-math.h				\
	math/go-rangefunc.h			\
	math/go-regression.h			\
	math/go-cspline.h			\
	math/go-complex.h			\
	math/go-fft.h				\
	math/go-matrix.h			\
	math/go-matrix3x3.h			\
	math/go-quad.h				\
	math/go-R.h				\
	math/go-distribution.h

if GOFFICE_WITH_DECIMAL64
math_SOURCES += math/go-decimal.c
math_HEADERS += math/go-decimal.h
endif

#####################################
# utils directory files
utils_SOURCES =					\
	utils/go-marshalers.list		\
						\
	utils/goffice-utils.c			\
	utils/go-color.c			\
	utils/go-file.c				\
	utils/go-font.c				\
						\
	utils/go-cairo.c			\
	utils/go-gradient.c			\
	utils/go-image.c			\
	utils/go-pixbuf.c			\
	utils/go-emf.c				\
	utils/go-spectre.c			\
	utils/go-line.c				\
	utils/go-locale.c			\
	utils/go-marker.c			\
	utils/go-mml-to-itex.c		\
	utils/go-path.c				\
	utils/go-pattern.c			\
	utils/go-geometry.c			\
	utils/go-rsm.c				\
	utils/go-string.c			\
	utils/go-undo.c				\
	utils/go-unit.c				\
	utils/datetime.c			\
	utils/formats.c				\
	utils/go-format.c			\
	utils/regutf8.c				\
						\
	utils/go-glib-extras.c			\
	utils/go-libxml-extras.c		\
	utils/go-pango-extras.c			\
	utils/go-gdk-pixbuf.c			\
	utils/go-persist.c			\
	utils/go-bezier.c			\
	utils/go-editor.c			\
	utils/go-style.c			\
	utils/go-styled-object.c

if WITH_LIBRSVG
utils_SOURCES += utils/go-svg.c
endif

utilsdir = $(goffice_include_dir)/utils
utils_HEADERS =					\
	utils/goffice-utils.h			\
	utils/go-cairo.h			\
	utils/go-color.h			\
	utils/go-file.h				\
	utils/go-font.h				\
	utils/go-gradient.h			\
	utils/go-image.h			\
	utils/go-pixbuf.h			\
	utils/go-svg.h				\
	utils/go-emf.h				\
	utils/go-spectre.h			\
	utils/go-line.h				\
	utils/go-locale.h			\
	utils/go-marker.h			\
	utils/go-mml-to-itex.h		\
	utils/go-path.h				\
	utils/go-pattern.h			\
	utils/go-unit.h				\
	utils/go-units.h			\
	utils/go-geometry.h			\
	utils/go-string.h			\
	utils/go-undo.h				\
	utils/datetime.h			\
	utils/go-format.h			\
	utils/regutf8.h				\
	utils/go-glib-extras.h			\
	utils/go-libxml-extras.h		\
	utils/go-pango-extras.h			\
	utils/go-gdk-pixbuf.h			\
	utils/go-persist.h			\
	utils/go-bezier.h			\
	utils/go-editor.h			\
	utils/go-rsm.h				\
	utils/go-style.h			\
	utils/go-styled-object.h

# A hint is needed to build the header first:
BUILT_SOURCES = utils/go-marshalers.h

# Another hint, see bugs #172211 and #172212:
non-intermediate: utils/go-marshalers.c

GENMARSHAL_COMMAND = $(GLIB_GENMARSHAL) --prefix=go_

.list.h:
	if ! [ -x utils ]; then mkdir utils; fi && \
	$(GENMARSHAL_COMMAND) --header $< >$@

.list.c:
	(echo '/* This file has been automatically generated.  Do not edit. */' && \
	echo '#include <goffice/goffice-config.h>' && \
	echo '#include "$*.h"' && \
	$(GENMARSHAL_COMMAND) --body $< ) >$@

lib_LTLIBRARIES = libgoffice-@GOFFICE_API_VER@.la

libgoffice_@GOFFICE_API_VER@_la_LIBADD = 			\
	$(GOFFICE_LIBS)	\
	$(GTK_MAC_LIBS)

libgoffice_@GOFFICE_API_VER@_la_LDFLAGS = -version-info $(VERSION_INFO)
if WITH_WIN32
libgoffice_@GOFFICE_API_VER@_la_DEPENDENCIES = goffice.def
libgoffice_@GOFFICE_API_VER@_la_LDFLAGS += -no-undefined -export-symbols goffice.def
endif

BUILT_SOURCES +=	goffice-paths.h embedded-stuff.c
CLEANFILES = $(BUILT_SOURCES)		\
	utils/go-marshalers.c

libgoffice_@GOFFICE_API_VER@_la_SOURCES = 	\
	goffice.c		\
	outoflinedocs.c		\
	goffice-priv.h \
	$(app_SOURCES) \
	$(canvas_SOURCES) \
	$(data_SOURCES) \
	$(graph_SOURCES) \
	$(math_SOURCES) \
	$(utils_SOURCES)

if WITH_GTK
libgoffice_@GOFFICE_API_VER@_la_SOURCES += 	\
	$(canvas_gtk_SOURCES) \
	$(component_SOURCES) \
	$(graph_gtk_SOURCES) \
	$(gtk_SOURCES)
else
EXTRA_DIST += \
	$(canvas_gtk_SOURCES) \
	$(component_SOURCES) \
	$(graph_gtk_SOURCES) \
	$(gtk_SOURCES)
endif

if GOFFICE_WITH_LASEM
libgoffice_@GOFFICE_API_VER@_la_SOURCES += 	\
	$(graph_lasem_SOURCES)
else
EXTRA_DIST += \
	$(graph_lasem_SOURCES)
endif

libgoffice_@GOFFICE_API_VER@_ladir = $(goffice_include_dir)
libgoffice_@GOFFICE_API_VER@_la_HEADERS = 	\
	goffice-features.h	\
	goffice.h

# Depends on this Makefile, because it uses make variables.
goffice-paths.h: Makefile
	@echo 'creating $@'
	@( \
	echo '/* This file has been automatically generated.  Do not edit. */'; \
	echo ''; \
	echo '#ifndef GOFFICE_PATHS_H'; \
	echo '#define GOFFICE_PATHS_H'; \
	echo ''; \
	echo '#define GOFFICE_DATADIR "$(goffice_datadir)"'; \
	echo '#define GOFFICE_LIBDIR "$(goffice_libdir)"'; \
	echo '#define GOFFICE_ICONDIR "$(goffice_icondir)"'; \
	echo '#define GOFFICE_LOCALEDIR "$(goffice_localedir)"'; \
	echo '#define GOFFICE_EXTERNPLUGINDIR "$(goffice_externplugindir)"'; \
	echo ''; \
	echo '#endif /* GOFFICE_PATHS_H */'; \
	) >$@

include $(top_srcdir)/goffice.mk

ALL_HDRS = goffice.h $(app_HEADERS) \
	$(canvas_HEADERS) \
	$(go_data_HEADERS) \
	$(graph_HEADERS) \
	$(math_HEADERS) \
	$(utils_HEADERS)

if WITH_GTK
ALL_HDRS += \
	$(canvas_gtk_HEADERS) \
	$(component_HEADERS) \
	$(graph_gtk_HEADERS) \
	$(gtk_HEADERS)
endif

if GOFFICE_WITH_LASEM
ALL_HDRS += \
	$(graph_lasem_HEADERS)
endif


if WITH_WIN32
LIB_PUBLIC_HDRS = $(libgoffice_@GOFFICE_API_VER@_la_HEADERS)


goffice.def: $(ALL_HDRS) $(top_srcdir)/tools/dumpdef.pl
	hdrs='$(ALL_HDRS)'; \
	hdrs_list=''; \
	for hdr in $$hdrs; do \
	  if test -f $(srcdir)/$$hdr; then \
	    hdrs_list="$$hdrs_list $(srcdir)/$$hdr"; \
	  else \
	    hdrs_list="$$hdrs_list $$hdr"; \
	  fi; \
	done; \
	cat $(top_builddir)/goffice/goffice-config.h $$hdrs_list | \
	sed -e 's/^#[ 	]*include[ 	]\+.*$$//g' | \
	$(CPP) $(AM_CPPFLAGS) "-DGO_VAR_DECL=__declspec(dllexport)" -P - > xgen-localdef.1
	@PERL@ $(top_srcdir)/tools/dumpdef.pl \
		xgen-localdef.1 > xgen-localdef.2
	(cmp -s xgen-localdef.2 local.def || cp xgen-localdef.2 local.def)
	rm -f xgen-localdef.1 xgen-localdef.2
	echo EXPORTS > $@ && \
	grep -E -v '^(go_plugin_init|go_plugin_shutdown)$$' local.def | sort >> $@

if HAVE_LIBEXE
goffice_mslibdir = $(DESTDIR)$(libdir)
goffice_mslib_DATA = goffice-0.lib

goffice-0.lib: libgoffice-@GOFFICE_API_VER@.la goffice.def
	lib -name:libgoffice-0-$(VERSION_IFACE).dll -def:goffice.def -out:$@
endif
endif

# Note: Files also need to be listed in the relevant subdirectory
embedded_stuff_compress = \
	graph/gog-3d-box-prefs.ui		\
	graph/gog-axis-prefs.ui			\
	graph/gog-axis-color-map-prefs.ui	\
	graph/gog-color-scale-prefs.ui	\
	graph/gog-equation-prefs.ui		\
	graph/gog-error-bar-prefs.ui		\
	graph/gog-graph-prefs.ui		\
	graph/gog-guru-type-selector.ui		\
	graph/gog-guru.ui			\
	graph/gog-object-prefs.ui		\
	graph/gog-plot-prefs.ui			\
	graph/gog-reg-curve-prefs.ui		\
	graph/gog-reg-eqn-prefs.ui		\
	graph/gog-series-labels-prefs.ui	\
	graph/gog-series-prefs.ui		\
	graph/gog-theme-editor.ui		\
	graph/new-theme-prefs.ui		\
	gtk/go-3d-rotation-sel.ui		\
	gtk/go-arrow-sel.ui			\
	gtk/go-font-sel.ui			\
	gtk/go-format-sel.ui			\
	gtk/go-image-save-dialog-extra.ui	\
	gtk/go-image-sel.ui			\
	gtk/go-rotation-sel.ui			\
	utils/go-style-prefs.ui

embedded_stuff_raw = \
	gtk/goffice-old.css			\
	gtk/goffice.css				\
	utils/svg-patterns.xml			\
	utils/unknown_image.png			\
	graph/bar-none.png 			\
	graph/bar-vplus.png 			\
	graph/bar-vminus.png 			\
	graph/bar-vboth.png			\
	graph/bar-hplus.png 			\
	graph/bar-hminus.png 			\
	graph/bar-hboth.png

embedded_stuff = $(embedded_stuff_compress) $(embedded_stuff_raw)

EXTRA_DIST += $(embedded_stuff)

embedded-stuff.c: $(top_srcdir)/tools/embedder $(embedded_stuff)
	cd $(srcdir) && @PERL@ $(abs_top_srcdir)/tools/embedder \
		--id-prefix=go: \
		--static \
		--register-function=go_register_ui_files \
		--register-file-function=go_rsm_register_file \
		COMPRESS $(embedded_stuff_compress) \
		NOCOMPRESS $(embedded_stuff_raw) \
		>$(abs_builddir)/$@


if HAVE_INTROSPECTION

INTROSPECTION_GIRS += GOffice-@GOFFICE_API_VER@.gir

GOffice-@GOFFICE_API_VER@.gir: \
	$(INTROSPECTION_SCANNER) \
	libgoffice-@GOFFICE_API_VER@.la
GOffice_@GOFFICE_API_VER_@_gir_INCLUDES = \
	GObject-2.0 \
	Gio-2.0 \
	libxml2-2.0 \
	cairo-1.0 \
	Pango-1.0 \
	Gtk-3.0 \
	Gsf-1
GOffice_@GOFFICE_API_VER_@_gir_CFLAGS = $(AM_CPPFLAGS)
GOffice_@GOFFICE_API_VER_@_gir_LIBS = libgoffice-@GOFFICE_API_VER@.la
GOffice_@GOFFICE_API_VER_@_gir_FILES = \
	$(libgoffice_@GOFFICE_API_VER@_la_SOURCES) \
	$(ALL_HDRS)

girdir = $(GIRDIR)
gir_DATA = $(INTROSPECTION_GIRS)

typelibsdir = $(TYPELIBDIR)
typelibs_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelibs_DATA)

endif # HAVE_INTROSPECTION
