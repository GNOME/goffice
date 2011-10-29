noinst_LTLIBRARIES = libgoffice-canvas.la

libgoffice_canvas_la_SOURCES =	\
	goc-arc.c		\
	goc-canvas.c		\
	goc-circle.c		\
	goc-ellipse.c		\
	goc-graph.c		\
	goc-group.c		\
	goc-item.c		\
	goc-image.c		\
	goc-line.c		\
	goc-path.c		\
	goc-pixbuf.c		\
	goc-polyline.c		\
	goc-polygon.c		\
	goc-rectangle.c		\
	goc-styled-item.c	\
	goc-text.c		\
	goc-utils.c

if  WITH_GTK
libgoffice_canvas_la_SOURCES += \
	goc-component.c	\
	goc-widget.c
endif

libgoffice_canvas_ladir = $(goffice_include_dir)/canvas
libgoffice_canvas_la_HEADERS =	\
	goc-arc.h		\
	goc-canvas.h		\
	goc-circle.h		\
	goc-component.h		\
	goc-ellipse.h		\
	goc-graph.h		\
	goc-group.h		\
	goc-item.h		\
	goc-image.h		\
	goc-line.h		\
	goc-path.h		\
	goc-pixbuf.h		\
	goc-polyline.h		\
	goc-polygon.h		\
	goc-rectangle.h		\
	goc-structs.h		\
	goc-styled-item.h	\
	goc-text.h		\
	goc-utils.h		\
	goc-widget.h		\
	goffice-canvas.h

include $(top_srcdir)/goffice.mk

LIB_PUBLIC_HDRS = $(libgoffice_canvas_la_HEADERS)
