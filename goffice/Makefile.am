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

BUILT_SOURCES =	goffice-paths.h embedded-stuff.c
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

# Note: Files also need to be listed in the relevant subdirectory
embedded_stuff_compress = \
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

embedded_stuff_raw = \
	utils/svg-patterns.xml			\
	graph/bar-none.png 			\
	graph/bar-vplus.png 			\
	graph/bar-vminus.png 			\
	graph/bar-vboth.png			\
	graph/bar-hplus.png 			\
	graph/bar-hminus.png 			\
	graph/bar-hboth.png

embedded_stuff = $(embedded_stuff_compress) $(embedded_stuff_raw)

embedded-stuff.c: $(top_srcdir)/tools/embedder $(embedded_stuff)
	cd $(srcdir) && @PERL@ $(abs_top_srcdir)/tools/embedder \
		--id-prefix=go: \
		--static \
		--register-function=go_register_ui_files \
		--register-file-function=go_rsm_register_file \
		COMPRESS $(embedded_stuff_compress) \
		NOCOMPRESS $(embedded_stuff_raw) \
		>$(abs_builddir)/$@


BUILT_GIRSOURCES = \
	GOffice-@GOFFICE_API_VER@.gir

GOffice-@GOFFICE_API_VER@.gir: list_of_sources
GOffice-@GOFFICE_API_VER@.gir: libgoffice-@GOFFICE_API_VER@.la
GOffice-@GOFFICE_API_VER@.gir: $(G_IR_SCANNER)
	$(AM_V_GEN) $(G_IR_SCANNER) -v --namespace GOffice \
	    --nsversion="@GOFFICE_API_VER@" \
	    --add-include-path=$(srcdir) --add-include-path=. \
	    --include=GObject-2.0 \
	    --include=Gio-2.0 \
	    --include=libxml2-2.0 \
	    --include=cairo-1.0 \
	    --include=Pango-1.0 \
	    --include=Gtk-3.0 \
	    --include=Gsf-1 \
	    --identifier-prefix="GO" \
	    --identifier-prefix="Go" \
	    --symbol-prefix="go" \
	    --symbol-prefix="libgoffice" \
	    --accept-unprefixed \
	    --library=libgoffice-@GOFFICE_API_VER@.la \
	    --libtool="$(LIBTOOL)" \
	    --nsversion="@GOFFICE_API_VER@" \
	    --output $@ \
	    --pkg libgsf-1 \
	    -I$(top_srcdir) \
	    `cat $(srcdir)/list_of_sources`

if HAVE_INTROSPECTION

girdir = $(GIRDIR)
gir_DATA = $(BUILT_GIRSOURCES)

typelibsdir = $(TYPELIBDIR)
typelibs_DATA = $(BUILT_GIRSOURCES:.gir=.typelib)

%.typelib: %.gir $(G_IR_COMPILER)
	$(AM_V_GEN) LD_LIBRARY_PATH=$${LD_LIBRARY_PATH:+$$LD_LIBRARY_PATH:}. $(G_IR_COMPILER) --includedir=$(srcdir) --includedir=. $(G_IR_COMPILER_OPTS) $< -o $(builddir)/$(@F)

endif # HAVE_INTROSPECTION
