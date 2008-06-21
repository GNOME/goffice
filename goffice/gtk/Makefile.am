noinst_LTLIBRARIES = libgoffice-gtk.la

libgoffice_gtk_la_SOURCES =		\
	goffice-gtk.c			\
	\
	go-font-sel.c			\
	go-format-sel.c			\
	go-rotation-sel.c	 	\
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
	go-palette.c			\
	go-selector.c			\
	go-color-selector.c		\
	go-gradient-selector.c		\
	go-line-selector.c		\
	go-marker-selector.c		\
	go-pattern-selector.c		\
	\
	go-pixbuf.c			\
	\
	go-graph-widget.c		


libgoffice_gtk_ladir = $(goffice_include_dir)/gtk
libgoffice_gtk_la_HEADERS = 		\
	goffice-gtk.h			\
	\
	go-font-sel.h			\
	go-format-sel.h			\
	go-rotation-sel.h	 	\
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
	go-action-combo-text.h		\
	\
	go-palette.h			\
	go-selector.h			\
	go-color-selector.h		\
	go-gradient-selector.h		\
	go-line-selector.h		\
	go-marker-selector.h		\
	go-pattern-selector.h		\
	\
	go-pixbuf.h			\
	\
	go-graph-widget.h

gladedir = $(goffice_datadir)/glade
dist_glade_DATA = \
	go-rotation-sel.glade			\
	go-font-sel.glade 			\
	go-format-sel.glade 			\
	go-image-save-dialog-extra.glade

EXTRA_DIST = $(UNUSED)

include $(top_srcdir)/goffice.mk

LIB_PUBLIC_HDRS = $(libgoffice_gtk_la_HEADERS)
