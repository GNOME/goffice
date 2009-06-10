noinst_LTLIBRARIES = libgoffice-app.la

noinst_HEADERS = \
	file-priv.h

libgoffice_app_la_SOURCES = \
	go-doc.c 			\
	go-doc-control.c 		\
	\
	go-plugin.c 			\
	go-plugin-loader.c 		\
	go-plugin-loader-module.c 	\
	go-plugin-service.c 		\
	\
	file.c				\
	error-info.c			\
	io-context.c			\
	go-cmd-context.c		\
	go-conf.c

libgoffice_app_ladir = $(goffice_include_dir)/app
libgoffice_app_la_api_headers = 	\
	goffice-app.h			\
	\
	go-doc.h 			\
	go-doc-impl.h 			\
	go-doc-control.h 		\
	go-doc-control-impl.h 		\
	go-plugin.h 			\
	go-plugin-loader.h 		\
	go-plugin-loader-module.h 	\
	go-plugin-service.h 		\
	go-plugin-service-impl.h	\
	file.h				\
	error-info.h			\
	io-context.h			\
	io-context-priv.h		\
	go-cmd-context.h		\
	go-cmd-context-impl.h	\
	go-conf.h

libgoffice_app_la_HEADERS = 	\
	$(libgoffice_app_la_api_headers) \
	module-plugin-defs.h

# This list should be empty when we reach a stable phase.
UNUSED = \
	go-app.h \
	go-error-stack.h \
	go-object.c \
	go-object.h \
	go-service.c \
	go-service.h \
	go-service-impl.h

EXTRA_DIST = $(UNUSED)		\
	go-conf-gconf.c			\
	go-conf-keyfile.c		\
	go-conf-win32.c

include $(top_srcdir)/goffice.mk

LIB_PUBLIC_HDRS = $(libgoffice_app_la_api_headers)
