SUBDIRS = utils data app graph math canvas

lib_LTLIBRARIES = libgoffice-@GOFFICE_API_VER@.la

libgoffice_@GOFFICE_API_VER@_la_LIBADD = 			\
	utils/libgoffice-utils.la		\
	app/libgoffice-app.la			\
	data/libgoffice-data.la			\
	graph/libgoffice-graph.la		\
	math/libgoffice-math.la		\
	$(GOFFICE_LIBS)	\
	$(GTK_MAC_LIBS)

if WITH_GTK
SUBDIRS += gtk component
libgoffice_@GOFFICE_API_VER@_la_LIBADD += 			\
	gtk/libgoffice-gtk.la			\
	component/libgoffice-component.la	\
	canvas/libgoffice-canvas.la
endif

libgoffice_@GOFFICE_API_VER@_la_LDFLAGS = -version-info $(VERSION_INFO)
if WITH_WIN32
libgoffice_@GOFFICE_API_VER@_la_DEPENDENCIES = goffice.def
libgoffice_@GOFFICE_API_VER@_la_LDFLAGS += -no-undefined -export-symbols goffice.def
endif

BUILT_SOURCES =	goffice-paths.h embedded-ui.c
CLEANFILES = $(BUILT_SOURCES)

libgoffice_@GOFFICE_API_VER@_la_SOURCES = 	\
	goffice.c		\
	goffice-priv.h

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

if WITH_WIN32
LIB_PUBLIC_HDRS = $(libgoffice_@GOFFICE_API_VER@_la_HEADERS)


if WITH_GTK
GTK_DEFS = gtk/local.def drawing/local.def component/local.def \
	canvas/local.def
else
GTK_DEFS =
endif

goffice.def: local.def app/local.def \
	data/local.def graph/local.def \
	math/local.def \
	ms-compat/local.def utils/local.def \
	$(GTK_DEFS)
	echo EXPORTS > $@ && \
	cat $^ | sort >> $@

if HAVE_LIBEXE
goffice_mslibdir = $(DESTDIR)$(libdir)
goffice_mslib_DATA = goffice-0.lib

goffice-0.lib: libgoffice-@GOFFICE_API_VER@.la goffice.def
	lib -name:libgoffice-0-$(VERSION_IFACE).dll -def:goffice.def -out:$@
endif
endif

embedded_uis = \
	graph/gog-3d-box-prefs.ui		\
	graph/gog-axis-prefs.ui			\
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
	gtk/go-3d-rotation-sel.ui		\
	gtk/go-font-sel.ui			\
	gtk/go-format-sel.ui			\
	gtk/go-image-save-dialog-extra.ui	\
	gtk/go-image-sel.ui			\
	gtk/go-rotation-sel.ui			\
	utils/go-style-prefs.ui

embedded_raw = utils/svg-patterns.xml

embedded-ui.c: $(top_srcdir)/tools/embedder $(embedded_uis) $(embedded_raw)
	cd $(srcdir) && @PERL@ $(abs_top_srcdir)/tools/embedder \
		--id-prefix=go: \
		--static \
		--register-function=go_register_ui_files \
		--register-file-function=go_rsm_register_file \
		COMPRESS $(embedded_uis) \
		NOCOMPRESS $(embedded_raw) \
		>$(abs_builddir)/$@
