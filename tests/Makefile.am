check_PROGRAMS=test-quad test-math test-format test-dtoa test-decimal constants
if WITH_GTK
check_PROGRAMS += pie-demo go-demo mf-demo
endif

include $(top_srcdir)/goffice.mk

AM_CFLAGS = $(GOFFICE_CFLAGS)

TESTS = test-quad test-math test-format test-dtoa test-decimal

constants_LDADD = $(GOFFICE_PLUGIN_LIBADD)
constants_SOURCES = constants.c

pie_demo_LDADD = $(GOFFICE_PLUGIN_LIBADD)
pie_demo_SOURCES = pie-demo.c

go_demo_LDADD = $(GOFFICE_PLUGIN_LIBADD)
go_demo_SOURCES = go-demo.c

shapes_demo_LDADD = $(GOFFICE_PLUGIN_LIBADD)
shapes_demo_SOURCES = shapes-demo.c

mf_demo_LDADD = $(GOFFICE_PLUGIN_LIBADD)
mf_demo_SOURCES = mf-demo.c

test_quad_LDADD = $(GOFFICE_PLUGIN_LIBADD)
test_quad_SOURCES = test-quad.c

test_math_LDADD = $(GOFFICE_PLUGIN_LIBADD)
test_math_SOURCES = test-math.c

test_decimal_LDADD = $(GOFFICE_PLUGIN_LIBADD)
test_decimal_SOURCES = test-decimal.c

test_format_LDADD = $(GOFFICE_PLUGIN_LIBADD)
test_format_SOURCES = test-format.c

test_dtoa_LDADD = $(GOFFICE_PLUGIN_LIBADD)
test_dtoa_SOURCES = test-dtoa.c

EXTRA_DIST = go-demo.ui
