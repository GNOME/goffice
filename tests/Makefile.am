if WITH_GTK
check_PROGRAMS = pie-demo go-demo shapes-demo mf-demo
endif

include $(top_srcdir)/goffice.mk

AM_CFLAGS = $(GOFFICE_CFLAGS)

pie_demo_LDADD = $(GOFFICE_PLUGIN_LIBADD)
pie_demo_SOURCES = pie-demo.c

go_demo_LDADD = $(GOFFICE_PLUGIN_LIBADD)
go_demo_SOURCES = go-demo.c

shapes_demo_LDADD = $(GOFFICE_PLUGIN_LIBADD)
shapes_demo_SOURCES = shapes-demo.c

mf_demo_LDADD = $(GOFFICE_PLUGIN_LIBADD)
mf_demo_SOURCES = mf-demo.c

EXTRA_DIST = go-demo.ui

