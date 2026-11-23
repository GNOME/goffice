check_PROGRAMS=test-quad test-math test-format test-dtoa test-decimal	\
	test-classify test-complex test-canvas test-graph constants
if WITH_GTK
check_PROGRAMS += pie-demo go-demo shapes-demo mf-demo
endif

include $(top_srcdir)/goffice.mk

AM_CFLAGS = $(GOFFICE_CFLAGS)

# Directories a test may need to find build-tree data (the graph tests
# load the plot-type plugins from here).
TESTS_ENVIRONMENT = 						\
	GOFFICE_TEST_SRCDIR='$(abs_top_srcdir)'			\
	GOFFICE_TEST_BUILDDIR='$(abs_top_builddir)'

TSCRIPTS = t2000-canvas-headless.pl t2001-canvas-xvfb.pl	\
	t2002-graph-headless.pl t2003-graph-xvfb.pl t8000-multipass.pl

TESTS = test-quad test-math test-format test-dtoa test-decimal	\
	test-classify test-complex					\
	$(TSCRIPTS)

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

test_classify_LDADD = $(GOFFICE_PLUGIN_LIBADD)
test_classify_SOURCES = test-classify.c

test_complex_LDADD = $(GOFFICE_PLUGIN_LIBADD)
test_complex_SOURCES = test-complex.c

test_canvas_LDADD = $(GOFFICE_PLUGIN_LIBADD)
test_canvas_SOURCES = test-canvas.c

test_graph_LDADD = $(GOFFICE_PLUGIN_LIBADD)
test_graph_SOURCES = test-graph.c

test_format_LDADD = $(GOFFICE_PLUGIN_LIBADD)
test_format_SOURCES = test-format.c

test_dtoa_LDADD = $(GOFFICE_PLUGIN_LIBADD)
test_dtoa_SOURCES = test-dtoa.c

EXTRA_DIST = go-demo.ui GofficeTest.pm common.supp			\
	t2000-canvas-headless.supp t2001-canvas-xvfb.supp		\
	t2002-graph-headless.supp t2003-graph-xvfb.supp			\
	$(TSCRIPTS)
