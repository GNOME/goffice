noinst_LTLIBRARIES = libgoffice-gtk.la
if WITH_WIN32
    lib_LTLIBRARIES = libgoffice-html-help-stub.la

    libgoffice_html_help_stub_la_SOURCES = htmlhelp-stub.c htmlhelp-stub.h
    libgoffice_html_help_stub_la_LIBADD = -luuid -lhtmlhelp
    libgoffice_html_help_stub_la_LDFLAGS = -no-undefined -Wl,libhtmlhelp-stub.def
endif

libgoffice_gtk_la_SOURCES =		\
	goffice-gtk.c			\
	\
	go-font-sel.c			\
	go-format-sel.c			\
	go-charmap-sel.c		\
	go-locale-sel.c			\
	\
	go-optionmenu.c			\
	go-combo-box.c			\
	go-color-group.c		\
	go-color-palette.c		\
	go-combo-color.c		\
	go-combo-pixmaps.c		\
	go-combo-text.c			\
	\
	go-action-combo-color.c 	\
	go-action-combo-pixmaps.c 	\
	go-action-combo-stack.c 	\
	go-action-combo-text.c	\
	\
	go-graph-widget.c


libgoffice_app_ladir = $(goffice_include_dir)/gtk
libgoffice_app_la_HEADERS = 		\
	goffice-gtk.h			\
	\
	go-font-sel.h			\
	go-format-sel.h			\
	go-charmap-sel.h		\
	go-locale-sel.h			\
	\
	go-optionmenu.h			\
	go-combo-box.h			\
	go-color-group.h		\
	go-color-palette.h		\
	go-combo-color.h		\
	go-combo-pixmaps.h		\
	go-combo-text.h			\
	\
	go-action-combo-color.h 	\
	go-action-combo-pixmaps.h 	\
	go-action-combo-stack.h 	\
	go-action-combo-text.h	\
	\
	go-graph-widget.h

goffice_glade_DATA = \
	go-font-sel.glade 	\
	go-format-sel.glade 

# This list should be empty when we reach a stable phase.
UNUSED = \
	go-dock.c \
	go-dock.h \
	go-dock-band.c \
	go-dock-band.h \
	go-dock-item.c \
	go-dock-item.h \
	go-dock-item-grip.c \
	go-dock-item-grip.h \
	go-dock-layout.c \
	go-dock-layout.h

EXTRA_DIST = htmlhelp-stub.c htmlhelp-stub.h $(goffice_glade_DATA) \
	$(UNUSED)

include $(top_srcdir)/goffice.mk
