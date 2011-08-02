noinst_LTLIBRARIES = libgoffice-component.la

libgoffice_component_la_SOURCES =	\
	go-component.c 		\
	go-component-factory.c	\
	go-component-mime-dialog.c

libgoffice_component_ladir = $(goffice_include_dir)/component
libgoffice_component_la_HEADERS = 	\
	goffice-component.h		\
	go-component.h			\
	go-component-factory.h	\
	go-component-mime-dialog.h

include $(top_srcdir)/goffice.mk

LIB_PUBLIC_HDRS = $(libgoffice_component_la_HEADERS)

