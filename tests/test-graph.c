/*
 * test-graph.c: Functional tests for the goffice graph (gog) library.
 *
 * These tests pin behaviour, not appearance.  Model state, geometry
 * computed through the APIs (axis maps, ticks, GOPath, bounds),
 * persistence and export are checked precisely; rendering is exercised
 * only at the "something was drawn where it should be, nothing where it
 * should not be, and the result is reproducible" level.  There are no
 * reference images, no expected pixel coordinates, and there will not
 * be.
 *
 * Most of this works without a display.  The plot-type plugins are
 * needed and are loaded from the directories in the environment
 * variable GOFFICE_TEST_PLUGIN_DIRS (the wrapper script sets it);
 * without them the plugin-dependent groups are skipped.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) version 3.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA.
 */

#include <goffice/goffice-config.h>
#include <goffice/goffice.h>
#include <goffice/graph/goffice-graph.h>
#include <goffice/graph/gog-axis.h>
#include <goffice/graph/gog-axis-color-map.h>
#include <goffice/graph/gog-chart-map-3d.h>
#include <goffice/graph/gog-chart-map.h>
#include <goffice/graph/gog-color-scale.h>
#include <goffice/graph/gog-object-xml.h>
#include <goffice/graph/gog-axis-line-impl.h>
#include <goffice/graph/gog-plot-engine.h>
#include <goffice/graph/gog-plot-impl.h>
#include <goffice/graph/gog-reg-curve.h>
#include <goffice/graph/gog-series-impl.h>
#include <goffice/graph/gog-trend-line.h>
#include <goffice/data/go-data-simple.h>

#ifdef GOFFICE_WITH_GTK
#include <goffice/graph/gog-guru.h>
#include <goffice/gtk/go-graph-widget.h>
#endif

#include <cairo.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;
static int skips;
static gboolean have_plugins = FALSE;
static gboolean have_display = FALSE;
static GString *passes;

/* ------------------------------------------------------------------------- */

static void
check (gboolean ok, const char *what)
{
	if (!ok) {
		g_printerr ("FAIL: %s\n", what);
		failures++;
	}
}

#define CHECK(cond) check ((cond) ? TRUE : FALSE, #cond)

static void
check_near (double got, double want, double tol, const char *what)
{
	if (!(fabs (got - want) <= tol)) {
		g_printerr ("FAIL: %s\n    got  %.17g\n    want %.17g\n",
			    what, got, want);
		failures++;
	}
}

static void
skip (const char *what)
{
	skips++;
	g_print ("test-graph: skipped %s\n", what);
}

/* ------------------------------------------------------------------------- */

/* Coarse colour comparison; no test should depend on exact colours.  */
static gboolean
color_near (GOColor got, GOColor want, unsigned tol)
{
	unsigned dr = abs ((int) GO_COLOR_UINT_R (got) -
			   (int) GO_COLOR_UINT_R (want));
	unsigned dg = abs ((int) GO_COLOR_UINT_G (got) -
			   (int) GO_COLOR_UINT_G (want));
	unsigned db = abs ((int) GO_COLOR_UINT_B (got) -
			   (int) GO_COLOR_UINT_B (want));
	return dr <= tol && dg <= tol && db <= tol;
}

/* ------------------------------------------------------------------------- */

/* Raster helpers.  These deliberately answer coarse questions: "is there
 * any drawing in this rectangle?" and "is this rectangle still the
 * background?".  There are no position- or colour-specific pixel checks.  */

static unsigned
region_ink (cairo_surface_t *surface, int x0, int y0, int x1, int y1)
{
	unsigned char *data = cairo_image_surface_get_data (surface);
	int stride = cairo_image_surface_get_stride (surface);
	unsigned count = 0;
	int x, y;

	g_return_val_if_fail (data != NULL, 0);
	for (y = y0; y < y1; y++)
		for (x = x0; x < x1; x++) {
			guint32 pixel = *(guint32 *) (data + y * stride + 4 * x);
			if ((pixel & 0xffffff) != 0xffffff)
				count++;
		}
	return count;
}

/* Bounding box of pixels close to @color, or %FALSE when there is none.  */
static gboolean
color_bbox (cairo_surface_t *surface, GOColor color,
	    unsigned *x0, unsigned *y0, unsigned *x1, unsigned *y1)
{
	unsigned char *data = cairo_image_surface_get_data (surface);
	int stride = cairo_image_surface_get_stride (surface);
	int w = cairo_image_surface_get_width (surface);
	int h = cairo_image_surface_get_height (surface);
	unsigned want_r = GO_COLOR_UINT_R (color);
	unsigned want_g = GO_COLOR_UINT_G (color);
	unsigned want_b = GO_COLOR_UINT_B (color);
	gboolean found = FALSE;
	int x, y;

	cairo_surface_flush (surface);
	g_return_val_if_fail (data != NULL, FALSE);
	for (y = 0; y < h; y++)
		for (x = 0; x < w; x++) {
			guint32 p = *(guint32 *) (data + y * stride + 4 * x);
			unsigned r = (p >> 16) & 0xff;
			unsigned g = (p >> 8) & 0xff;
			unsigned b = p & 0xff;

			if (abs ((int) r - (int) want_r) > 32 ||
			    abs ((int) g - (int) want_g) > 32 ||
			    abs ((int) b - (int) want_b) > 32)
				continue;
			if (!found) {
				*x0 = *x1 = x;
				*y0 = *y1 = y;
				found = TRUE;
			} else {
				if (x < (int) *x0) *x0 = x;
				if (x > (int) *x1) *x1 = x;
				if (y < (int) *y0) *y0 = y;
				if (y > (int) *y1) *y1 = y;
			}
		}
	return found;
}

/* Summary of a GOPath: segment count and first/last points.  */
typedef struct {
	unsigned n_segments;
	double first_x, first_y, last_x, last_y;
} PathSummary;

static void
summary_move_to (gpointer closure, GOPathPoint const *p)
{
	PathSummary *s = closure;
	if (s->n_segments == 0) {
		s->first_x = p->x;
		s->first_y = p->y;
	}
	s->last_x = p->x;
	s->last_y = p->y;
	s->n_segments++;
}

static void
summary_line_to (gpointer closure, GOPathPoint const *p)
{
	PathSummary *s = closure;
	if (s->n_segments == 0) {
		s->first_x = p->x;
		s->first_y = p->y;
	}
	s->last_x = p->x;
	s->last_y = p->y;
	s->n_segments++;
}

static void
summary_curve_to (gpointer closure, GOPathPoint const *p0,
		  G_GNUC_UNUSED GOPathPoint const *p1,
		  GOPathPoint const *p2)
{
	PathSummary *s = closure;
	if (s->n_segments == 0) {
		s->first_x = p0->x;
		s->first_y = p0->y;
	}
	s->last_x = p2->x;
	s->last_y = p2->y;
	s->n_segments++;
}

static void
summary_close (G_GNUC_UNUSED gpointer closure)
{
}

static PathSummary
path_summary (GOPath const *path)
{
	PathSummary s = { 0, 0., 0., 0., 0. };

	if (path != NULL)
		go_path_interpret (path, GO_PATH_DIRECTION_FORWARD,
				   summary_move_to, summary_line_to,
				   summary_curve_to, summary_close, &s);
	return s;
}

/* ------------------------------------------------------------------------- */

static GogGraph *
new_graph (double width, double height)
{
	GogGraph *graph = GOG_GRAPH (g_object_new (GOG_TYPE_GRAPH, NULL));
	gog_graph_set_size (graph, width, height);
	return graph;
}

static GogChart *
graph_add_chart (GogGraph *graph)
{
	return GOG_CHART (gog_object_add_by_name2 (GOG_OBJECT (graph),
						  "Chart", NULL));
}

/* Create a plot from the named family/type and add it to the chart.
 * The type name is the one in plot-types.xml (e.g. "XY"), not the
 * engine id used by gog_plot_new_by_name().  */
static GogPlot *
chart_add_plot (GogChart *chart, char const *family_name, char const *type_name)
{
	GogPlotFamily *family;
	GogPlotType *type;
	GogPlot *plot;

	if (chart == NULL)
		return NULL;
	family = gog_plot_family_by_name (family_name);
	if (family == NULL)
		return NULL;
	type = g_hash_table_lookup (family->types, type_name);
	if (type == NULL)
		return NULL;
	plot = gog_plot_new_by_type (type);
	if (plot == NULL)
		return NULL;
	if (gog_object_add_by_name2 (GOG_OBJECT (chart), "Plot",
				    GOG_OBJECT (plot)) == NULL)
		return NULL;
	return plot;
}

static GogSeries *
plot_add_xy_series (GogPlot *plot, double const *x, double const *y,
		    unsigned n)
{
	GogSeries *series;

	if (plot == NULL)
		return NULL;
	series = gog_plot_new_series (plot);
	if (series == NULL)
		return NULL;
	gog_dataset_set_dim (GOG_DATASET (series), 0,
			     go_data_vector_val_new_copy ((double *) x, n),
			     NULL);
	gog_dataset_set_dim (GOG_DATASET (series), 1,
			     go_data_vector_val_new_copy ((double *) y, n),
			     NULL);

	/* Series caches (e.g. num_elements) are filled by the update
	 * vfunc, which normally runs from an idle.  Run it now so the
	 * tests can rely on it.  */
	gog_object_request_update (GOG_OBJECT (series));
	gog_object_update (GOG_OBJECT (series));

	return series;
}

static cairo_surface_t *
render_graph (GogGraph *graph, double width, double height, gboolean *ok)
{
	cairo_surface_t *surface;
	cairo_t *cr;
	GogRenderer *renderer;

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					      (int) width, (int) height);
	cr = cairo_create (surface);
	cairo_set_source_rgb (cr, 1., 1., 1.);
	cairo_paint (cr);

	renderer = gog_renderer_new (graph);
	*ok = gog_renderer_render_to_cairo (renderer, cr, width, height);
	g_object_unref (renderer);

	cairo_destroy (cr);
	cairo_surface_flush (surface);
	return surface;
}

static GsfOutput *
graph_save_to_memory (GogGraph *graph)
{
	GsfOutput *output = gsf_output_memory_new ();
	GsfXMLOut *xml = gsf_xml_out_new (output);

	gog_object_write_xml_sax (GOG_OBJECT (graph), xml, NULL);
	g_object_unref (xml);
	gsf_output_close (output);
	return output;
}

static GogObject *
graph_load_from_memory (GsfOutput *output)
{
	GsfInput *input;
	GogObject *loaded;

	input = gsf_input_memory_new_clone (gsf_output_memory_get_bytes (
						    GSF_OUTPUT_MEMORY (output)),
					    gsf_output_size (output));
	loaded = gog_object_new_from_input (input, NULL);
	g_object_unref (input);
	return loaded;
}

/* ------------------------------------------------------------------------- */

static void
test_plugins (void)
{
	GogPlotFamily *family;

	if (!have_plugins) {
		skip ("plot plugins (no GOFFICE_TEST_PLUGIN_DIRS)");
		return;
	}

	CHECK (g_hash_table_size ((GHashTable *) gog_plot_families ()) > 0);
	family = gog_plot_family_by_name ("XY");
	CHECK (family != NULL);
	if (family != NULL)
		CHECK (g_hash_table_lookup (family->types, "XY") != NULL);
}

static void
test_model (void)
{
	GogGraph *graph = new_graph (200., 100.);
	GogGraph *dup;
	GogChart *chart;
	GogPlot *plot;
	GogSeries *series;

	CHECK (GOG_IS_GRAPH (graph));
	CHECK (gog_graph_get_theme (graph) != NULL);

	chart = graph_add_chart (graph);
	CHECK (GOG_IS_CHART (chart));
	CHECK (gog_object_get_child_by_name (GOG_OBJECT (graph), "Chart") ==
	       GOG_OBJECT (chart));

	plot = chart_add_plot (chart, "XY", "XY");
	if (plot == NULL) {
		skip ("XY plot model tests (plugin not loaded)");
		g_object_unref (graph);
		return;
	}
	CHECK (gog_plot_get_axis (plot, GOG_AXIS_X) != NULL);
	CHECK (gog_plot_get_axis (plot, GOG_AXIS_Y) != NULL);

	series = plot_add_xy_series (plot,
				     (double const[]) { 0, 1, 2, 3, 4 },
				     (double const[]) { 1, 3, 2, 5, 4 },
				     5);
	CHECK (GOG_IS_SERIES (series));
	if (series != NULL)
		CHECK (gog_series_num_elements (series) == 5);

	dup = gog_graph_dup (graph);
	CHECK (dup != NULL);
	if (dup != NULL) {
		CHECK (gog_object_get_child_by_name (GOG_OBJECT (dup),
						     "Chart") != NULL);
		g_object_unref (dup);
	}

	g_object_unref (graph);
}

static void
test_axis (void)
{
	GogGraph *graph = new_graph (200., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY");
	GogAxis *axis = plot ? gog_plot_get_axis (plot, GOG_AXIS_X) : NULL;
	GogAxisMap *map;
	GogAxisTick *ticks;
	double min, max;

	if (axis == NULL) {
		skip ("axis map tests (plugin not loaded)");
		g_object_unref (graph);
		return;
	}

	gog_axis_set_bounds (axis, 0., 10.);
	CHECK (gog_axis_get_bounds (axis, &min, &max));
	check_near (min, 0., 1e-12, "axis bound minimum");
	check_near (max, 10., 1e-12, "axis bound maximum");

	map = gog_axis_map_new (axis, 0., 100.);
	CHECK (map != NULL);
	CHECK (gog_axis_map_is_valid (map));
	check_near (gog_axis_map_to_view (map, 0.), 0., 1e-9,
		    "map to_view minimum");
	check_near (gog_axis_map_to_view (map, 10.), 100., 1e-9,
		    "map to_view maximum");
	check_near (gog_axis_map_from_view (map, 50.), 5., 1e-9,
		    "map from_view midpoint");
	check_near (gog_axis_map_from_view (map, gog_axis_map_to_view (map, 3.7)),
		    3.7, 1e-9, "map round trip");
	gog_axis_map_free (map);

	/* The tick array is borrowed; no free.  */
	CHECK (gog_axis_get_ticks (axis, &ticks) > 0);
	CHECK (ticks != NULL);

	g_object_unref (graph);
}

static void
test_color_map (void)
{
	GOColor colors[3] = { GO_COLOR_RED, GO_COLOR_GREEN, GO_COLOR_BLUE };
	GogAxisColorMap *map = gog_axis_color_map_from_colors (
		"test-map", 3, colors, GO_RESOURCE_RW);
	GogAxisColorMap *dup;

	CHECK (map != NULL);
	if (map == NULL)
		return;

	CHECK (gog_axis_color_map_get_max (map) == 2);
	CHECK (color_near (gog_axis_color_map_get_color (map, 0.), colors[0], 1));
	CHECK (color_near (gog_axis_color_map_get_color (map, 2.), colors[2], 1));
	/* Midpoint is interpolated; allow generous rounding.  */
	CHECK (color_near (gog_axis_color_map_get_color (map, 1.), colors[1], 16));

	dup = gog_axis_color_map_dup (map);
	CHECK (dup != NULL);
	CHECK (dup != map);
	if (dup != NULL)
		g_object_unref (dup);

	/* A map id from a document that is not installed is an empty map;
	 * the core accessors must cope (the editor used to crash on it).  */
	{
		GogAxisColorMap const *empty =
			gog_axis_color_map_get_from_id ("no-such-colour-map");
		CHECK (empty != NULL);
		if (empty != NULL) {
			CHECK (gog_axis_color_map_get_max (empty) == 0);
			CHECK (color_near (gog_axis_color_map_get_color (empty, 0.),
					   (GOColor) 0, 0));
		}
	}

	g_object_unref (map);
}

static void
test_series (void)
{
	static const double xv[] = { 0, 1, 2, 3, 4 };
	static const double yv[] = { 1, 3, 2, 5, 4 };
	GogGraph *graph = new_graph (200., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY");
	GogSeries *series = plot_add_xy_series (plot, xv, yv, 5);

	if (series == NULL) {
		skip ("series tests (plugin not loaded)");
		g_object_unref (graph);
		return;
	}

	CHECK (gog_series_num_elements (series) == 5);
	{
		double const *x = NULL, *y = NULL;
		unsigned n = gog_series_get_xy_data (series, &x, &y);
		CHECK (n == 5);
		if (n == 5 && x != NULL && y != NULL) {
			check_near (x[0], 0., 1e-12, "series x first");
			check_near (y[0], 1., 1e-12, "series y first");
			check_near (y[4], 4., 1e-12, "series y last");
		}
	}

	g_object_unref (graph);
}

static void
test_persistence (void)
{
	GogGraph *graph = new_graph (300., 200.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY");
	GsfOutput *output;
	GogObject *loaded;

	if (plot == NULL) {
		skip ("persistence tests (plugin not loaded)");
		g_object_unref (graph);
		return;
	}
	plot_add_xy_series (plot, (double const[]) { 0, 1, 2 },
			    (double const[]) { 1, 0, 1 }, 3);

	output = graph_save_to_memory (graph);
	CHECK (gsf_output_size (output) > 0);
	CHECK (g_strstr_len ((char const *) gsf_output_memory_get_bytes (
				     GSF_OUTPUT_MEMORY (output)),
			     gsf_output_size (output), "GogGraph") != NULL);

	loaded = graph_load_from_memory (output);
	CHECK (loaded != NULL);
	if (loaded != NULL) {
		GogObject *lchart = gog_object_get_child_by_name (loaded, "Chart");
		CHECK (lchart != NULL);
		if (lchart != NULL)
			CHECK (gog_object_get_child_by_name (lchart, "Plot") != NULL);
		g_object_unref (loaded);
	}

	g_object_unref (output);
	g_object_unref (graph);
}

/* Defined with the robustness test below; used to keep the property
 * sweep quiet while it pokes at interacting properties.  */
static void
quiet_log (const gchar *domain, GLogLevelFlags level, const gchar *message,
	   gpointer data);

/* Give a persistent property a different, valid value.  Only types with
 * an obvious "other" value are handled; strings (often enumeration-like)
 * are left alone.  */
static gboolean
perturb_value (GParamSpec *ps, GValue *val)
{
	GType type = G_TYPE_FUNDAMENTAL (G_PARAM_SPEC_VALUE_TYPE (ps));

	switch (type) {
	case G_TYPE_BOOLEAN:
		g_value_set_boolean (val, !g_value_get_boolean (val));
		return TRUE;
	case G_TYPE_INT: {
		GParamSpecInt *p = G_PARAM_SPEC_INT (ps);
		gint v = g_value_get_int (val);
		if (v < p->maximum) v++;
		else if (v > p->minimum) v--;
		else return FALSE;
		g_value_set_int (val, v);
		return TRUE;
	}
	case G_TYPE_UINT: {
		GParamSpecUInt *p = G_PARAM_SPEC_UINT (ps);
		guint v = g_value_get_uint (val);
		if (v < p->maximum) v++;
		else if (v > p->minimum) v--;
		else return FALSE;
		g_value_set_uint (val, v);
		return TRUE;
	}
	case G_TYPE_LONG: {
		GParamSpecLong *p = G_PARAM_SPEC_LONG (ps);
		glong v = g_value_get_long (val);
		if (v < p->maximum) v++;
		else if (v > p->minimum) v--;
		else return FALSE;
		g_value_set_long (val, v);
		return TRUE;
	}
	case G_TYPE_ULONG: {
		GParamSpecULong *p = G_PARAM_SPEC_ULONG (ps);
		gulong v = g_value_get_ulong (val);
		if (v < p->maximum) v++;
		else if (v > p->minimum) v--;
		else return FALSE;
		g_value_set_ulong (val, v);
		return TRUE;
	}
	case G_TYPE_INT64: {
		GParamSpecInt64 *p = G_PARAM_SPEC_INT64 (ps);
		gint64 v = g_value_get_int64 (val);
		if (v < p->maximum) v++;
		else if (v > p->minimum) v--;
		else return FALSE;
		g_value_set_int64 (val, v);
		return TRUE;
	}
	case G_TYPE_UINT64: {
		GParamSpecUInt64 *p = G_PARAM_SPEC_UINT64 (ps);
		guint64 v = g_value_get_uint64 (val);
		if (v < p->maximum) v++;
		else if (v > p->minimum) v--;
		else return FALSE;
		g_value_set_uint64 (val, v);
		return TRUE;
	}
	case G_TYPE_DOUBLE: {
		GParamSpecDouble *p = G_PARAM_SPEC_DOUBLE (ps);
		double v = g_value_get_double (val);
		double c;

		if (!go_finite (v))
			c = 1.;
		else {
			c = v + 1.;
			if (!(c <= p->maximum) || c == v)
				c = v - 1.;
		}
		if (!(c >= p->minimum) || !(c <= p->maximum) || c == v)
			return FALSE;
		g_value_set_double (val, c);
		return TRUE;
	}
	case G_TYPE_ENUM: {
		GEnumClass *klass = G_PARAM_SPEC_ENUM (ps)->enum_class;
		gint v = g_value_get_enum (val);
		guint i;

		for (i = 0; i < klass->n_values; i++)
			if (klass->values[i].value != v) {
				g_value_set_enum (val, klass->values[i].value);
				return TRUE;
			}
		return FALSE;
	}
	case G_TYPE_FLAGS: {
		GFlagsClass *klass = G_PARAM_SPEC_FLAGS (ps)->flags_class;
		guint v = g_value_get_flags (val);
		guint i;

		for (i = 0; i < klass->n_values; i++)
			if ((v & klass->values[i].value) == 0) {
				g_value_set_flags (val, v | klass->values[i].value);
				return TRUE;
			}
		for (i = 0; i < klass->n_values; i++)
			if ((v & klass->values[i].value) != 0) {
				g_value_set_flags (val, v & ~klass->values[i].value);
				return TRUE;
			}
		return FALSE;
	}
	default:
		return FALSE;
	}
}

/* Set every persistent, writable property of @obj to a non-default value.
 * Returns the number of properties that actually changed.  */
static unsigned
perturb_persistent_props (GObject *obj)
{
	GParamSpec **pspecs;
	guint n, i;
	unsigned changed = 0;

	pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (obj), &n);
	for (i = 0; i < n; i++) {
		GParamSpec *ps = pspecs[i];
		GValue val = G_VALUE_INIT;

		if (!(ps->flags & GO_PARAM_PERSISTENT) ||
		    (ps->flags & G_PARAM_CONSTRUCT_ONLY) ||
		    !(ps->flags & G_PARAM_WRITABLE))
			continue;
		/* Ids are assigned by the parent; leave them alone.  */
		if (strcmp (ps->name, "id") == 0)
			continue;
		/* Virtual axes force the tick flags off in
		 * gog_axis_set_atype(); those states are unreachable and
		 * the forced values differ from the pspec defaults.  */
		if (GOG_IS_AXIS (obj) &&
		    gog_axis_get_atype (GOG_AXIS (obj)) >= GOG_AXIS_VIRTUAL &&
		    (g_str_has_prefix (ps->name, "major-tick-") ||
		     g_str_has_prefix (ps->name, "minor-tick-")))
			continue;
		g_value_init (&val, G_PARAM_SPEC_VALUE_TYPE (ps));
		g_object_get_property (obj, ps->name, &val);
		if (perturb_value (ps, &val)) {
			g_object_set_property (obj, ps->name, &val);
			changed++;
		}
		g_value_unset (&val);
	}
	g_free (pspecs);
	return changed;
}

static char **
filtered_lines (GsfOutput *output, gboolean skip_ids)
{
	char **raw = g_strsplit ((char const *) gsf_output_memory_get_bytes (
					 GSF_OUTPUT_MEMORY (output)),
				 "\n", -1);
	GString *str = g_string_new (NULL);
	unsigned i;

	for (i = 0; raw[i] != NULL; i++) {
		if (skip_ids && strstr (raw[i], "name=\"id\"") != NULL)
			continue;
		g_string_append (str, raw[i]);
		g_string_append_c (str, '\n');
	}
	g_strfreev (raw);
	raw = g_strsplit (str->str, "\n", -1);
	g_string_free (str, TRUE);
	return raw;
}

static void
check_output_equal (GsfOutput *a, GsfOutput *b, gboolean skip_ids,
		    const char *what)
{
	char **la = filtered_lines (a, skip_ids);
	char **lb = filtered_lines (b, skip_ids);
	unsigned i;

	for (i = 0; la[i] != NULL || lb[i] != NULL; i++) {
		if (g_strcmp0 (la[i], lb[i]) != 0) {
			g_printerr ("FAIL: %s\n    line %u:\n      %.100s\n      %.100s\n",
				    what, i + 1,
				    la[i] != NULL ? la[i] : "(none)",
				    lb[i] != NULL ? lb[i] : "(none)");
			failures++;
			break;
		}
	}
	g_strfreev (la);
	g_strfreev (lb);
}

/* A duplicate must be indistinguishable from its original, both as XML
 * (ignoring ids) and as pixels.  */
static void
test_graph_dup_roundtrip (void)
{
	GogGraph *graph = new_graph (300., 200.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY Lines");
	GogGraph *dup;
	GsfOutput *orig_xml, *dup_xml;
	cairo_surface_t *surf_a, *surf_b;
	gboolean ok;

	if (plot == NULL) {
		skip ("graph duplicate test (no XY plot)");
		g_object_unref (graph);
		return;
	}
	plot_add_xy_series (plot, (double const[]) { 0., 1., 2., 3., 4. },
			    (double const[]) { 1., 3., 2., 5., 4. }, 5);
	plot_add_xy_series (plot, (double const[]) { 0., 1., 2., 3., 4. },
			    (double const[]) { 4., 3., 5., 2., 1. }, 5);
	g_object_set (chart, "columns", 2, NULL);
	g_object_set (gog_plot_get_axis (plot, GOG_AXIS_X),
		      "invert-axis", TRUE, NULL);

	dup = gog_graph_dup (graph);
	CHECK (dup != NULL);
	if (dup == NULL) {
		g_object_unref (graph);
		return;
	}

	orig_xml = graph_save_to_memory (graph);
	dup_xml = graph_save_to_memory (dup);
	check_output_equal (orig_xml, dup_xml, TRUE,
			    "a duplicate matches its original");
	g_object_unref (orig_xml);
	g_object_unref (dup_xml);

	surf_a = render_graph (graph, 400., 300., &ok);
	CHECK (ok);
	surf_b = render_graph (dup, 400., 300., &ok);
	CHECK (ok);
	if (surf_a != NULL && surf_b != NULL) {
		int stride_a = cairo_image_surface_get_stride (surf_a);
		int stride_b = cairo_image_surface_get_stride (surf_b);

		check (stride_a == stride_b &&
		       memcmp (cairo_image_surface_get_data (surf_a),
			       cairo_image_surface_get_data (surf_b),
			       (size_t) stride_a * 300) == 0,
		       "a duplicate renders like its original");
	}
	if (surf_a != NULL)
		cairo_surface_destroy (surf_a);
	if (surf_b != NULL)
		cairo_surface_destroy (surf_b);

	g_object_unref (dup);
	g_object_unref (graph);
}

/* Every persistent property of the objects in the tree must survive a
 * save/load round trip.  The check is a byte comparison of the XML before
 * and after; the writer is deterministic.  */
static void
test_persistent_property_roundtrip (void)
{
	GogGraph *graph = new_graph (300., 200.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY Lines");
	GogSeries *series;
	GsfOutput *before, *after;
	GogObject *loaded;
	GLogLevelFlags old_fatal;
	guint handler;
	unsigned changed = 0;

	if (plot == NULL) {
		skip ("persistent property round trip (no XY plot)");
		g_object_unref (graph);
		return;
	}
	series = plot_add_xy_series (plot, (double const[]) { 0., 1., 2. },
				     (double const[]) { 1., 0., 1. }, 3);

	/* Some persistent properties interact (e.g. axis bounds), so
	 * setting them one by one can complain; only the state that
	 * survives the round trip is of interest here.  */
	old_fatal = g_log_set_always_fatal ((GLogLevelFlags) 0);
	handler = g_log_set_handler (NULL,
				     G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL,
				     quiet_log, NULL);

	changed += perturb_persistent_props (G_OBJECT (graph));
	changed += perturb_persistent_props (G_OBJECT (chart));
	changed += perturb_persistent_props (G_OBJECT (plot));
	if (series != NULL)
		changed += perturb_persistent_props (G_OBJECT (series));
	changed += perturb_persistent_props (
		G_OBJECT (gog_plot_get_axis (plot, GOG_AXIS_X)));
	changed += perturb_persistent_props (
		G_OBJECT (gog_plot_get_axis (plot, GOG_AXIS_Y)));

	g_log_remove_handler (NULL, handler);
	g_log_set_always_fatal (old_fatal);

	check (changed >= 10,
	       "persistent properties suitable for the sweep were found");

	/* Auto style fields serialize their last resolved value, so resolve
	 * them on both sides before comparing.  */
	gog_object_request_update (GOG_OBJECT (graph));
	gog_object_update (GOG_OBJECT (graph));

	before = graph_save_to_memory (graph);
	loaded = graph_load_from_memory (before);
	CHECK (loaded != NULL);
	if (loaded != NULL) {
		gog_object_request_update (loaded);
		gog_object_update (loaded);
		after = graph_save_to_memory (GOG_GRAPH (loaded));
		check_output_equal (before, after, FALSE,
				    "persistent properties survive a round trip");
		g_object_unref (after);
		g_object_unref (loaded);
	}
	g_object_unref (before);
	g_object_unref (graph);
}

/* Every registered plot type must preserve its persistent properties
 * across a save/load round trip.  Types whose instance default differs
 * from the pspec default (like the radar area's fill flag) need
 * GOG_PARAM_FORCE_SAVE, or the writer elides the value.  */
static void
test_persistent_plot_type_roundtrip (void)
{
	GHashTableIter fiter, titer;
	gpointer fkey, fval, tkey, tval;
	unsigned n_types = 0;

	g_hash_table_iter_init (&fiter, (GHashTable *) gog_plot_families ());
	while (g_hash_table_iter_next (&fiter, &fkey, &fval)) {
		GogPlotFamily *family = fval;

		g_hash_table_iter_init (&titer, family->types);
		while (g_hash_table_iter_next (&titer, &tkey, &tval)) {
			GogPlotType *type = tval;
			GogGraph *graph = new_graph (300., 200.);
			GogChart *chart = graph_add_chart (graph);
			GogPlot *plot = gog_plot_new_by_type (type);
			GogSeries *series = NULL;
			GsfOutput *before = NULL, *after = NULL;
			GogObject *loaded = NULL;
			GLogLevelFlags old_fatal;
			guint handler;
			unsigned i;

			CHECK (chart != NULL && plot != NULL);
			if (plot == NULL ||
			    gog_object_add_by_name2 (GOG_OBJECT (chart), "Plot",
						     GOG_OBJECT (plot)) == NULL) {
				g_object_unref (graph);
				continue;
			}
			n_types++;
			series = gog_plot_new_series (plot);

			/* Some properties interact, and some combinations can
			 * complain; only the surviving state matters.  */
			old_fatal = g_log_set_always_fatal ((GLogLevelFlags) 0);
			handler = g_log_set_handler (
				NULL, G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL,
				quiet_log, NULL);

			perturb_persistent_props (G_OBJECT (graph));
			perturb_persistent_props (G_OBJECT (chart));
			perturb_persistent_props (G_OBJECT (plot));
			if (series != NULL)
				perturb_persistent_props (G_OBJECT (series));
			for (i = 0; i < GOG_AXIS_TYPES; i++) {
				GogAxis *axis = gog_chart_get_axis (chart, i);
				if (axis != NULL)
					perturb_persistent_props (G_OBJECT (axis));
			}

			g_log_remove_handler (NULL, handler);
			g_log_set_always_fatal (old_fatal);

			/* Resolve auto styles on both sides before saving.  */
			gog_object_request_update (GOG_OBJECT (graph));
			gog_object_update (GOG_OBJECT (graph));
			before = graph_save_to_memory (graph);
			loaded = graph_load_from_memory (before);
			if (loaded != NULL) {
				gog_object_request_update (loaded);
				gog_object_update (loaded);
				after = graph_save_to_memory (GOG_GRAPH (loaded));
			}

			if (after != NULL) {
				char *msg = g_strdup_printf (
					"%.60s properties survive a round trip",
					(char const *) tkey);

				check_output_equal (before, after, FALSE, msg);
				g_free (msg);
			} else
				check (FALSE, "the swept plot type loads back");

			if (after != NULL)
				g_object_unref (after);
			if (loaded != NULL)
				g_object_unref (loaded);
			g_object_unref (before);
			g_object_unref (graph);
		}
	}
	check (n_types >= 10, "the registered plot types were swept");
}

/* An explicitly empty label format must survive a round trip: the
 * constructed default is plot-dependent ("%j" for value dimensions),
 * so the writer must not elide "".  */
static void
test_label_format_roundtrip (void)
{
	GogGraph *graph = new_graph (300., 200.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY");
	GogSeries *series = plot != NULL? plot_add_xy_series (
		plot, (double const[]) { 0, 1, 2 },
		(double const[]) { 3, 1, 2 }, 3): NULL;
	GogObject *labels, *point = NULL, *loaded = NULL;
	GsfOutput *out;
	char *fmt = NULL;

	CHECK (series != NULL);
	if (series == NULL) {
		skip ("label format round trip (XY plot not loaded)");
		g_object_unref (graph);
		return;
	}
	labels = gog_object_add_by_name2 (GOG_OBJECT (series), "Data labels", NULL);
	CHECK (labels != NULL);
	if (labels != NULL) {
		point = gog_object_add_by_name2 (labels, "Point", NULL);
		CHECK (point != NULL);
	}

	if (labels != NULL) {
		g_object_get (labels, "format", &fmt, NULL);
		check (fmt != NULL && fmt[0] != 0,
		       "the constructed label format is non-empty");
		g_free (fmt);
		fmt = NULL;
		g_object_set (labels, "format", "", NULL);
	}
	if (point != NULL) {
		g_object_get (point, "format", &fmt, NULL);
		check (fmt != NULL && fmt[0] != 0,
		       "the constructed per-point format is non-empty");
		g_free (fmt);
		fmt = NULL;
		g_object_set (point, "format", "", NULL);
	}

	gog_object_request_update (GOG_OBJECT (graph));
	gog_object_update (GOG_OBJECT (graph));
	out = graph_save_to_memory (graph);
	loaded = graph_load_from_memory (out);
	g_object_unref (out);
	CHECK (loaded != NULL);
	if (loaded != NULL) {
		GogObject *lchart, *lplot, *lseries;

		gog_object_request_update (loaded);
		gog_object_update (loaded);
		lchart = gog_object_get_child_by_name (loaded, "Chart");
		lplot = lchart != NULL?
			gog_object_get_child_by_name (lchart, "Plot"): NULL;
		lseries = lplot != NULL?
			gog_object_get_child_by_name (lplot, "Series"): NULL;
		labels = lseries != NULL?
			gog_object_get_child_by_name (lseries, "Data labels"): NULL;
		CHECK (labels != NULL);
		if (labels != NULL) {
			g_object_get (labels, "format", &fmt, NULL);
			check (fmt != NULL && fmt[0] == 0,
			       "an empty label format survives the round trip");
			g_free (fmt);
			fmt = NULL;
			point = gog_object_get_child_by_name (labels, "Point");
			CHECK (point != NULL);
			if (point != NULL) {
				g_object_get (point, "format", &fmt, NULL);
				check (fmt != NULL && fmt[0] == 0,
				       "an empty per-point format survives the round trip");
				g_free (fmt);
				fmt = NULL;
			}

			/* A NULL format behaves like an empty one and must
			 * not crash the update.  */
			g_object_set (labels, "format", NULL, NULL);
			if (point != NULL)
				g_object_set (point, "format", NULL, NULL);
			gog_object_request_update (loaded);
			gog_object_update (loaded);
			g_object_get (labels, "format", &fmt, NULL);
			check (fmt != NULL && fmt[0] == 0,
			       "a NULL label format is kept as empty");
			g_free (fmt);
			fmt = NULL;
		}
		g_object_unref (loaded);
	}
	g_object_unref (graph);
}

/* "0" is a valid padding or swatch size and must survive a round trip;
 * the constructed defaults are non-zero (0.25 cm / 0.2 cm).  */
static void
test_outlined_padding_roundtrip (void)
{
	GogGraph *graph = new_graph (300., 200.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY");
	GogObject *legend = NULL, *loaded;
	GsfOutput *out;
	double padding = -1., swatch_size = -1., swatch_padding = -1.;

	if (plot == NULL) {
		skip ("padding round trip (XY plot not loaded)");
		g_object_unref (graph);
		return;
	}
	legend = gog_object_add_by_name2 (GOG_OBJECT (chart), "Legend", NULL);
	CHECK (legend != NULL);
	g_object_set (graph, "padding-pts", 0., NULL);
	if (legend != NULL) {
		g_object_set (legend, "swatch-size-pts", 0., NULL);
		g_object_set (legend, "swatch-padding-pts", 0., NULL);
	}

	gog_object_request_update (GOG_OBJECT (graph));
	gog_object_update (GOG_OBJECT (graph));
	out = graph_save_to_memory (graph);
	loaded = graph_load_from_memory (out);
	g_object_unref (out);
	CHECK (loaded != NULL);
	if (loaded != NULL) {
		GogObject *lchart, *llegend;

		gog_object_request_update (loaded);
		gog_object_update (loaded);
		lchart = gog_object_get_child_by_name (loaded, "Chart");
		llegend = lchart != NULL?
			gog_object_get_child_by_name (lchart, "Legend"): NULL;
		g_object_get (loaded, "padding-pts", &padding, NULL);
		check (padding == 0., "padding-pts 0 survives the round trip");
		CHECK (llegend != NULL);
		if (llegend != NULL) {
			g_object_get (llegend, "swatch-size-pts", &swatch_size, NULL);
			g_object_get (llegend, "swatch-padding-pts", &swatch_padding, NULL);
			check (swatch_size == 0.,
			       "swatch-size-pts 0 survives the round trip");
			check (swatch_padding == 0.,
			       "swatch-padding-pts 0 survives the round trip");
		}
		g_object_unref (loaded);
	}
	g_object_unref (graph);
}

/* Bad input must be rejected, without crashing.  Deliberate parse errors
 * are noisy, so silence the XML domain while testing them.  */
static void
quiet_log (G_GNUC_UNUSED const gchar *domain, G_GNUC_UNUSED GLogLevelFlags level,
	   G_GNUC_UNUSED const gchar *message, G_GNUC_UNUSED gpointer data)
{
}

static void
test_robustness (void)
{
	static const char *bad[] = { "", "<GogObject", NULL };
	GLogLevelFlags old_fatal;
	guint handler;
	unsigned i;

	/* Feeding bad input is the point of this test: quiet the XML
	 * domain and do not let the harness's fatal-criticals trap fire
	 * on the expected parse errors.  */
	old_fatal = g_log_set_always_fatal ((GLogLevelFlags) 0);
	handler = g_log_set_handler ("XML",
		G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL, quiet_log, NULL);

	for (i = 0; bad[i] != NULL; i++) {
		GsfInput *input = gsf_input_memory_new (
			(guint8 const *) bad[i], strlen (bad[i]), FALSE);
		GogObject *obj = gog_object_new_from_input (input, NULL);
		g_object_unref (input);
		CHECK (obj == NULL);
		if (obj != NULL)
			g_object_unref (obj);
	}

	g_log_remove_handler ("XML", handler);
	g_log_set_always_fatal (old_fatal);
}

static void
test_render (void)
{
	GogGraph *graph = new_graph (400., 300.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY");
	cairo_surface_t *empty, *a, *b;
	gboolean ok;
	unsigned empty_ink, data_ink;

	if (plot == NULL) {
		skip ("render tests (plugin not loaded)");
		g_object_unref (graph);
		return;
	}

	/* The chart without data.  */
	empty = render_graph (graph, 400., 300., &ok);
	CHECK (ok);

	plot_add_xy_series (plot, (double const[]) { 0, 1, 2, 3, 4 },
			    (double const[]) { 1, 3, 2, 5, 4 }, 5);

	a = render_graph (graph, 400., 300., &ok);
	CHECK (ok);
	b = render_graph (graph, 400., 300., &ok);
	CHECK (ok);

	/* Functional containment: adding data must add drawing in the plot
	 * area.  Nothing here looks at individual pixel positions.  */
	empty_ink = region_ink (empty, 120, 60, 280, 240);
	data_ink = region_ink (a, 120, 60, 280, 240);
	CHECK (data_ink > empty_ink);

	/* Rendering the same graph twice must give the same image.  */
	{
		int stride_a = cairo_image_surface_get_stride (a);
		int stride_b = cairo_image_surface_get_stride (b);
		CHECK (stride_a == stride_b);
		CHECK (memcmp (cairo_image_surface_get_data (a),
			       cairo_image_surface_get_data (b),
			       (size_t) stride_a * 300) == 0);
	}

	cairo_surface_destroy (empty);
	cairo_surface_destroy (a);
	cairo_surface_destroy (b);
	g_object_unref (graph);
}

static void
test_export (void)
{
	GogGraph *graph = new_graph (200., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY");
	GsfOutput *output;
	const guint8 *bytes;

	if (plot == NULL) {
		skip ("export tests (plugin not loaded)");
		g_object_unref (graph);
		return;
	}
	plot_add_xy_series (plot, (double const[]) { 0, 1, 2 },
			    (double const[]) { 1, 0, 1 }, 3);

	output = gsf_output_memory_new ();
	CHECK (gog_graph_export_image (graph, GO_IMAGE_FORMAT_PNG, output,
				       96., 96.));
	gsf_output_close (output);
	bytes = gsf_output_memory_get_bytes (GSF_OUTPUT_MEMORY (output));
	CHECK (gsf_output_size (output) > 8);
	CHECK (bytes != NULL && bytes[0] == 0x89 && bytes[1] == 'P' &&
	       bytes[2] == 'N' && bytes[3] == 'G');
	g_object_unref (output);

	output = gsf_output_memory_new ();
	CHECK (gog_graph_export_image (graph, GO_IMAGE_FORMAT_SVG, output,
				       96., 96.));
	gsf_output_close (output);
	bytes = gsf_output_memory_get_bytes (GSF_OUTPUT_MEMORY (output));
	CHECK (bytes != NULL &&
	       g_strstr_len ((char const *) bytes, gsf_output_size (output),
			     "<svg") != NULL);
	g_object_unref (output);

	g_object_unref (graph);
}

static char const *
image_format_name (GOImageFormat format)
{
	switch (format) {
	case GO_IMAGE_FORMAT_SVG: return "svg";
	case GO_IMAGE_FORMAT_PNG: return "png";
	case GO_IMAGE_FORMAT_JPG: return "jpg";
	case GO_IMAGE_FORMAT_PDF: return "pdf";
	case GO_IMAGE_FORMAT_PS:  return "ps";
	case GO_IMAGE_FORMAT_EMF: return "emf";
	case GO_IMAGE_FORMAT_WMF: return "wmf";
	case GO_IMAGE_FORMAT_EPS: return "eps";
	default: return "unknown";
	}
}

/* Every advertised format must export, with content recognizable as that
 * format.  Note that GO_IMAGE_FORMAT_SVG is 0, i.e. a NULL list element.  */
static void
test_export_format_sweep (void)
{
	GogGraph *graph = new_graph (200., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY Lines");
	GSList *formats, *ptr;
	unsigned n = 0;

	if (plot == NULL) {
		skip ("export format sweep (no XY plot)");
		g_object_unref (graph);
		return;
	}
	plot_add_xy_series (plot, (double const[]) { 0., 1., 2. },
			    (double const[]) { 1., 0., 1. }, 3);

	formats = gog_graph_get_supported_image_formats ();
	for (ptr = formats; ptr != NULL; ptr = ptr->next) {
		GOImageFormat format = GPOINTER_TO_UINT (ptr->data);
		GsfOutput *output = gsf_output_memory_new ();
		const guint8 *bytes;
		gsize size;
		gboolean ok;
		char *msg;

		ok = gog_graph_export_image (graph, format, output, 96., 96.);
		gsf_output_close (output);
		bytes = gsf_output_memory_get_bytes (GSF_OUTPUT_MEMORY (output));
		size = gsf_output_size (output);

		msg = g_strdup_printf ("%s export succeeds",
				       image_format_name (format));
		check (ok && size > 8, msg);
		g_free (msg);
		if (ok && size > 8) {
			gboolean magic_ok = TRUE;

			switch (format) {
			case GO_IMAGE_FORMAT_PNG:
				magic_ok = bytes[0] == 0x89 &&
					   memcmp (bytes + 1, "PNG", 3) == 0;
				break;
			case GO_IMAGE_FORMAT_JPG:
				magic_ok = bytes[0] == 0xFF && bytes[1] == 0xD8;
				break;
			case GO_IMAGE_FORMAT_PDF:
				magic_ok = memcmp (bytes, "%PDF", 4) == 0;
				break;
			case GO_IMAGE_FORMAT_PS:
			case GO_IMAGE_FORMAT_EPS:
				magic_ok = memcmp (bytes, "%!", 2) == 0;
				break;
			case GO_IMAGE_FORMAT_SVG:
				magic_ok = g_strstr_len ((char const *) bytes,
							 size, "<svg") != NULL;
				break;
			default:
				break;
			}
			msg = g_strdup_printf ("%s export has recognizable content",
					       image_format_name (format));
			check (magic_ok, msg);
			g_free (msg);
			n++;
		}
		g_object_unref (output);
	}
	g_slist_free (formats);
	check (n >= 5, "the supported format list was swept");

	g_object_unref (graph);
}

/* ------------------------------------------------------------------------- */

/* Synthetic dimension data for the engine smoke test.  Matrix dimensions
 * need data proxies, so the few engines needing them are skipped.  */
static GOData *
make_dim_data (GogSeriesDimDesc const *dim, unsigned n)
{
	double *v;
	unsigned i;

	if (dim->val_type == GOG_DIM_MATRIX)
		return NULL;
	if (dim->ms_type == GOG_MS_DIM_LABELS) {
		char const *strs[5] = { "a", "b", "c", "d", "e" };
		return go_data_vector_str_new_copy (strs, MIN (n, 5));
	}
	v = g_new (double, n);
	for (i = 0; i < n; i++)
		v[i] = i + 1;
	return go_data_vector_val_new (v, n, g_free);
}

/* Render every registered plot type with generic data.  This is a crash,
 * critical and "nothing was drawn" net, not a looks test.  */
static void
test_engine_smoke (void)
{
	GHashTableIter fiter, titer;
	gpointer fkey, fval, tkey, tval;
	unsigned rendered = 0, skipped = 0;

	if (!have_plugins) {
		skip ("engine smoke tests (no plugins)");
		return;
	}

	g_hash_table_iter_init (&fiter, (GHashTable *) gog_plot_families ());
	while (g_hash_table_iter_next (&fiter, &fkey, &fval)) {
		GogPlotFamily *family = fval;

		g_hash_table_iter_init (&titer, family->types);
		while (g_hash_table_iter_next (&titer, &tkey, &tval)) {
			GogPlotType *type = tval;
			GogGraph *graph = new_graph (200., 150.);
			GogChart *chart = graph_add_chart (graph);
			GogPlot *plot = gog_plot_new_by_type (type);
			GogPlotDesc const *desc;
			GogSeries *series;
			cairo_surface_t *surface;
			gboolean matrix = FALSE, ok = FALSE;
			unsigned i, ink;

			if (plot == NULL) {
				g_printerr ("FAIL: %s / %s: no plot object\n",
					    family->name, type->name);
				failures++;
				g_object_unref (graph);
				continue;
			}
			if (gog_object_add_by_name2 (GOG_OBJECT (chart), "Plot",
						     GOG_OBJECT (plot)) == NULL) {
				g_printerr ("FAIL: %s / %s: plot not accepted\n",
					    family->name, type->name);
				failures++;
				g_object_unref (graph);
				continue;
			}
			series = gog_plot_new_series (plot);
			desc = gog_plot_description (plot);
			for (i = 0; series != NULL && i < desc->series.num_dim; i++) {
				GOData *dat = make_dim_data (&desc->series.dim[i], 5);
				if (dat == NULL) {
					matrix = TRUE;
					break;
				}
				gog_dataset_set_dim (GOG_DATASET (series), i, dat, NULL);
			}
			if (matrix) {
				skipped++;
				g_object_unref (graph);
				continue;
			}
			if (series != NULL) {
				gog_object_request_update (GOG_OBJECT (series));
				gog_object_update (GOG_OBJECT (series));
			}
			surface = render_graph (graph, 200., 150., &ok);
			ink = region_ink (surface, 0, 0, 200, 150);
			cairo_surface_destroy (surface);
			if (!ok || ink == 0) {
				g_printerr ("FAIL: %s / %s (engine %s): %s%s\n",
					    family->name, type->name, type->engine,
					    ok ? "" : "render failed; ",
					    ink == 0 ? "nothing drawn" : "");
				failures++;
			} else
				rendered++;
			g_object_unref (graph);
		}
	}

	g_print ("test-graph: %u plot engine case(s) rendered, "
		 "%u matrix case(s) skipped.\n", rendered, skipped);
}

/* ------------------------------------------------------------------------- */

static gboolean
polar_odf_spline_renders (const char *family_name, const char *type_name)
{
	GogGraph *graph = new_graph (200., 150.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, family_name, type_name);
	GogSeries *series;
	cairo_surface_t *surface;
	gboolean ok = FALSE;

	if (plot != NULL) {
		series = plot_add_xy_series (plot,
					     (double const[]) { 0, 1, 2, 3, 4 },
					     (double const[]) { 1, 3, 2, 5, 4 },
					     5);
		if (series != NULL) {
			g_object_set (series, "interpolation", "odf-spline",
				      NULL);
			gog_object_request_update (GOG_OBJECT (series));
			gog_object_update (GOG_OBJECT (series));
			surface = render_graph (graph, 200., 150., &ok);
			cairo_surface_destroy (surface);
		}
	}

	g_object_unref (graph);
	return ok;
}

static void
test_regression_polar_odf_spline (void)
{
	gboolean ok = polar_odf_spline_renders ("Polar", "Polar") &&
		      polar_odf_spline_renders ("Radar", "Radar");

	check (ok, "polar/radar ODF-spline render does not abort");
}

static void
test_regression_color_map_wrap (void)
{
	GOColor colors[3] = { GO_COLOR_RED, GO_COLOR_GREEN, GO_COLOR_BLUE };
	GogAxisColorMap *map;
	GOColor got, expected;

	map = gog_axis_color_map_from_colors ("wrap-map", 3, colors,
					      GO_RESOURCE_RW);
	if (map == NULL) {
		skip ("colour map wrap regression (no map)");
		return;
	}

	/* A value in (max, max+1) falls in the wrap-around gap: it should
	 * interpolate cyclically from the last stop to the first.  */
	expected = GO_COLOR_INTERPOLATE (colors[2], colors[0], 0.5);
	got = gog_axis_color_map_get_color (map, 2.5);
	check (color_near (got, expected, 64),
	       "colour-map value in (max, max+1) wraps");

	g_object_unref (map);
}

static void
test_regression_label_overrides (void)
{
	GogGraph *graph = new_graph (200., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY");
	GogSeries *series;
	GogObject *labels_obj;
	GogDataLabel *l0, *l1;
	GogSeriesLabelElt const *e0, *e1;
	gboolean ok;

	if (plot == NULL) {
		skip ("label override regression (no plot)");
		g_object_unref (graph);
		return;
	}

	series = plot_add_xy_series (plot, (double const[]) { 0, 1, 2, 3, 4 },
				     (double const[]) { 1, 3, 2, 5, 4 }, 5);
	labels_obj = series != NULL
		? gog_object_add_by_name2 (GOG_OBJECT (series),
					  "Data labels", NULL)
		: NULL;
	if (labels_obj == NULL) {
		skip ("label override regression (no labels object)");
		g_object_unref (graph);
		return;
	}
	l0 = GOG_DATA_LABEL (gog_object_add_by_name2 (labels_obj,
						     "Point", NULL));
	l1 = GOG_DATA_LABEL (gog_object_add_by_name2 (labels_obj,
						     "Point", NULL));
	if (l0 == NULL || l1 == NULL) {
		skip ("label override regression (no data labels)");
		g_object_unref (graph);
		return;
	}
	g_object_set (l0, "format", "LBL0", NULL);
	g_object_set (l1, "format", "LBL1", NULL);

	gog_object_request_update (labels_obj);
	gog_object_update (labels_obj);

	e0 = gog_series_labels_vector_get_element (GOG_SERIES_LABELS (labels_obj),
						   0);
	e1 = gog_series_labels_vector_get_element (GOG_SERIES_LABELS (labels_obj),
						   1);
	ok = e0 != NULL && e0->str != NULL &&
	     g_strcmp0 (e0->str->str, "LBL0") == 0 &&
	     e1 != NULL && e1->str != NULL &&
	     g_strcmp0 (e1->str->str, "LBL1") == 0;
	if (!ok)
		g_print ("      labels: [0]=%s [1]=%s\n",
			 e0 != NULL && e0->str != NULL ? e0->str->str : "(none)",
			 e1 != NULL && e1->str != NULL ? e1->str->str : "(none)");
	check (ok, "series-label overrides applied to every index");

	g_object_unref (graph);
}

static const char *
first_trend_line_engine_name (void)
{
	GHashTableIter iter;
	gpointer key, value;

	g_hash_table_iter_init (&iter, (GHashTable *) gog_trend_line_types ());
	return g_hash_table_iter_next (&iter, &key, &value)
		? ((GogTrendLineType *) value)->engine : NULL;
}

/* gog-plot.c:683: trend lines hidden from the legend must not be
 * subtracted from a vary-style-by-element cardinality.  Safe while the
 * bug is present: the underflow only bites when enumerating.  */
static void
test_regression_vary_cardinality (void)
{
	GogGraph *graph = new_graph (200., 150.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "Bubble", "Bubble");
	GogSeries *series;
	GogPlotDesc const *desc;
	char const *tl_name;
	unsigned full = 0, visible = 0, i;

	if (plot == NULL) {
		skip ("vary-style cardinality regression (no Bubble plot)");
		g_object_unref (graph);
		return;
	}

	series = gog_plot_new_series (plot);
	desc = gog_plot_description (plot);
	for (i = 0; series != NULL && i < desc->series.num_dim; i++) {
		GOData *dat = make_dim_data (&desc->series.dim[i], 1);
		if (dat != NULL)
			gog_dataset_set_dim (GOG_DATASET (series), i, dat, NULL);
	}
	g_object_set (plot, "vary-style-by-element", TRUE, NULL);

	tl_name = first_trend_line_engine_name ();
	for (i = 0; i < 2 && tl_name != NULL; i++) {
		GogTrendLine *tl = gog_trend_line_new_by_name (tl_name);
		if (tl == NULL)
			break;
		if (gog_object_add_by_name2 (GOG_OBJECT (series), "Trend line",
					    GOG_OBJECT (tl)) == NULL)
			break;
		/* After insertion: setting it earlier crashes.  */
		g_object_set (tl, "has-legend", FALSE, NULL);
	}
	if (i == 2) {
		gog_object_request_update (GOG_OBJECT (series));
		gog_object_update (GOG_OBJECT (series));
		gog_plot_update_cardinality (plot, 0);
		gog_plot_get_cardinality (plot, &full, &visible);
		check (full >= 1 && visible <= full,
				 "vary-style cardinality stays consistent "
				 "(gog-plot.c:683)");
	} else
		skip ("vary-style cardinality regression (no trend lines)");

	g_object_unref (graph);
}

/* gog-color-scale.c:259: an empty "axis" string must not read before
 * the buffer.  */
static void
test_regression_color_scale_empty_axis (void)
{
	GogGraph *graph = new_graph (200., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogObject *scale = gog_object_add_by_name2 (GOG_OBJECT (chart),
						   "Color-Scale", NULL);

	if (scale == NULL) {
		skip ("empty colour-scale axis regression (no colour scale)");
		g_object_unref (graph);
		return;
	}
	g_object_set (scale, "axis", "", NULL);
	check (gog_color_scale_get_axis (GOG_COLOR_SCALE (scale)) == NULL,
	       "empty colour-scale axis string leaves the axis unset "
	       "(gog-color-scale.c:259)");
	g_object_unref (graph);
}

/* Trend-line types are freed at plugin shutdown.  This runs at the very
 * end of the suite since it releases the plugin state; it catches the
 * use-after-free in gog-plot-engine.c under valgrind.  */
static void
test_trend_line_type_shutdown (void)
{
	GSList *states;

	if (!have_plugins) {
		skip ("trend-line shutdown test (no plugins)");
		return;
	}
	if (g_hash_table_size ((GHashTable *) gog_trend_line_types ()) == 0) {
		skip ("trend-line shutdown test (no trend-line types)");
		return;
	}

	states = go_plugins_shutdown ();
	g_slist_free_full (states, g_free);

	check (g_hash_table_size ((GHashTable *) gog_trend_line_types ()) == 0,
	       "trend-line types released cleanly at plugin shutdown");
}

/* ------------------------------------------------------------------------- */

/* A graph + chart + XY plot + X axis, in one go.  */
static GogAxis *
new_xy_axis (GogGraph **graph_out, GogChart **chart_out, GogPlot **plot_out)
{
	GogGraph *graph = new_graph (200., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY");

	*graph_out = graph;
	*chart_out = chart;
	*plot_out = plot;
	return plot != NULL ? gog_plot_get_axis (plot, GOG_AXIS_X) : NULL;
}

static void
test_object_child_by_name (void)
{
	GogGraph *graph = new_graph (200., 100.);

	graph_add_chart (graph);
	check (gog_object_get_child_by_name (GOG_OBJECT (graph),
						       "NoSuchRole") == NULL,
			 "unknown role name yields no child (gog-object.c:1336)");
	g_object_unref (graph);
}

static void
test_object_getter_defaults (void)
{
	GogGraph *graph;
	GogChart *chart;
	GogPlot *plot;
	GogAxis *axis = new_xy_axis (&graph, &chart, &plot);
	char *str;

	(void) chart;
	(void) plot;
	if (axis == NULL) {
		skip ("object getter defaults (no XY plot)");
		g_object_unref (graph);
		return;
	}

	str = NULL;
	g_object_get (axis, "compass", &str, NULL);
	check (str != NULL,
			 "\"compass\" getter always sets a string "
			 "(gog-object.c:394)");
	g_free (str);

	str = NULL;
	g_object_get (axis, "anchor", &str, NULL);
	check (str != NULL,
			 "\"anchor\" getter always sets a string "
			 "(gog-object.c:413)");
	g_free (str);

	g_object_unref (graph);
}

static void
test_metrics_unit_property (void)
{
	GogGraph *graph;
	GogChart *chart;
	GogPlot *plot;
	GogAxis *axis = new_xy_axis (&graph, &chart, &plot);
	char *str = NULL;

	(void) chart;
	(void) plot;
	if (axis == NULL) {
		skip ("metrics-unit property (no XY plot)");
		g_object_unref (graph);
		return;
	}

	g_object_set (axis, "metrics-unit", "in", NULL);
	g_object_get (axis, "metrics-unit", &str, NULL);
	check (g_strcmp0 (str, "in") == 0,
			 "\"metrics-unit\" round-trips (gog-axis.c:3543)");
	g_free (str);
	g_object_unref (graph);
}

static void
test_series_data_null_contract (void)
{
	GogGraph *graph;
	GogChart *chart;
	GogPlot *plot;
	GogSeries *series;
	double const *x = (double const *) (void *) 1;
	double const *y = (double const *) (void *) 1;

	new_xy_axis (&graph, &chart, &plot);
	(void) chart;
	if (plot == NULL) {
		skip ("series data contract (no XY plot)");
		g_object_unref (graph);
		return;
	}
	series = gog_plot_new_series (plot);
	CHECK (series != NULL);
	CHECK (gog_series_get_xy_data (series, &x, &y) == 0);
	check (x == NULL && y == NULL,
			 "get_xy_data clears its outputs for an empty series "
			 "(gog-series.c:1308)");

	g_object_unref (graph);
}

static void
test_unparented_series_interpolation (void)
{
	GType type = g_type_from_name ("GogXYSeries");
	GogObject *series;

	if (type == 0) {
		skip ("unparented interpolation (plugin type missing)");
		return;
	}
	series = g_object_new (type, "interpolation", "spline", NULL);
	check (series != NULL,
	       "interpolation on an unparented series does not crash "
	       "(gog-series.c:439)");
	if (series != NULL)
		g_object_unref (series);
}

static void
test_renderer_update_null_model (void)
{
	GogRenderer *renderer = gog_renderer_new (NULL);

	CHECK (renderer != NULL);
	gog_renderer_update (renderer, 100., 100.);
	check (renderer != NULL,
	       "gog_renderer_update with no model does not crash "
	       "(gog-renderer.c:1575)");
	g_object_unref (renderer);
}

static void
test_data_label_position (void)
{
	GogGraph *graph;
	GogChart *chart;
	GogPlot *plot;
	GogSeries *series;
	GogObject *labels_obj;
	GogDataLabel *label;

	new_xy_axis (&graph, &chart, &plot);
	(void) chart;
	if (plot == NULL) {
		skip ("data label position (no XY plot)");
		g_object_unref (graph);
		return;
	}

	series = plot_add_xy_series (plot, (double const[]) { 0, 1, 2 },
				     (double const[]) { 1, 0, 1 }, 3);
	labels_obj = gog_object_add_by_name2 (GOG_OBJECT (series),
					     "Data labels", NULL);
	label = labels_obj != NULL
		? GOG_DATA_LABEL (gog_object_add_by_name2 (labels_obj,
							  "Point", NULL))
		: NULL;
	if (label == NULL) {
		skip ("data label position (no data label)");
		g_object_unref (graph);
		return;
	}

	check (gog_data_label_get_position (label) ==
			 GOG_SERIES_LABELS_TOP,
			 "data label position resolves the series default "
			 "(gog-series-labels.c:1072)");

	g_object_unref (graph);
}

static void
test_graph_auto_placement (void)
{
	GogGraph *graph = new_graph (400., 200.);
	GogChart *c1 = graph_add_chart (graph);
	GogChart *c2;
	unsigned x1, y1, w1, h1, x2, y2, w2, h2;
	gboolean overlap;

	g_object_set (c1, "rows", 2, NULL);
	c2 = graph_add_chart (graph);
	CHECK (gog_chart_get_position (c1, &x1, &y1, &w1, &h1));
	CHECK (gog_chart_get_position (c2, &x2, &y2, &w2, &h2));
	overlap = x1 < x2 + w2 && x2 < x1 + w1 &&
		  y1 < y2 + h2 && y2 < y1 + h1;
	check (!overlap,
			 "auto-placed charts do not overlap "
			 "(gog-graph.c:386)");
	g_object_unref (graph);
}

static void
test_chart_get_axis_order (void)
{
	GogGraph *graph;
	GogChart *chart;
	GogPlot *plot;
	GogAxis *first, *second;

	new_xy_axis (&graph, &chart, &plot);
	if (plot == NULL) {
		skip ("chart axis order (no XY plot)");
		g_object_unref (graph);
		return;
	}
	first = gog_plot_get_axis (plot, GOG_AXIS_X);
	second = GOG_AXIS (gog_object_add_by_name2 (GOG_OBJECT (chart),
						   "X-Axis", NULL));
	CHECK (first != NULL && second != NULL && first != second);
	if (second != NULL)
		g_object_set (second, "type", GOG_AXIS_X, NULL);

	check (gog_chart_get_axis (chart, GOG_AXIS_X) == first,
			 "gog_chart_get_axis returns the first match "
			 "(gog-chart.c:1204)");

	g_object_unref (graph);
}

static void
test_chart_axis_after_removal (void)
{
	GogGraph *graph = new_graph (200., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot_xy = chart_add_plot (chart, "XY", "XY");
	GogPlot *plot_col = chart_add_plot (chart, "ColoredXY", "Map");
	GogAxis *axis;
	GogPlot *plot2;

	if (plot_xy == NULL || plot_col == NULL) {
		skip ("axis recreation after removal (no colored plot)");
		g_object_unref (graph);
		return;
	}
	axis = gog_chart_get_axis (chart, GOG_AXIS_COLOR);
	if (axis == NULL) {
		skip ("axis recreation after removal (no color axis)");
		g_object_unref (graph);
		return;
	}

	/* The colored plot goes first so the axis becomes deletable; the
	 * XY plot keeps the chart non-empty so that the axis set is not
	 * reset.  */
	gog_object_clear_parent (GOG_OBJECT (plot_col));
	g_object_unref (plot_col);
	gog_object_clear_parent (GOG_OBJECT (axis));
	g_object_unref (axis);
	plot2 = chart_add_plot (chart, "ColoredXY", "Map");
	check (plot2 != NULL &&
	       gog_plot_get_axis (plot2, GOG_AXIS_COLOR) != NULL,
	       "a deleted axis is recreated for a later plot "
	       "(gog-chart.c:570)");

	g_object_unref (graph);
}

static void
test_position_masks (void)
{
	check ((GOG_POSITION_ANCHOR & GOG_POSITION_PADDING) == 0 &&
			 (GOG_POSITION_ANY_MANUAL &
			  GOG_POSITION_PADDING) == 0,
			 "anchor/manual masks do not overlap padding "
			 "(goffice-graph.h:194)");
}

static gboolean
count_changed (GogObject *obj, gboolean size, gpointer data)
{
	(void) obj;
	(void) size;
	(*(int *) data)++;
	return FALSE;
}

static void
test_reg_curve_getter_changed (void)
{
	GogGraph *graph;
	GogChart *chart;
	GogPlot *plot;
	GogSeries *series;
	GogTrendLine *tl = NULL;
	GHashTableIter iter;
	gpointer key, value;
	int changed = 0;
	char *str = NULL;

	new_xy_axis (&graph, &chart, &plot);
	(void) chart;
	if (plot == NULL) {
		skip ("reg-curve getter (no XY plot)");
		g_object_unref (graph);
		return;
	}
	series = plot_add_xy_series (plot, (double const[]) { 0, 1, 2 },
				     (double const[]) { 1, 0, 1 }, 3);

	g_hash_table_iter_init (&iter, (GHashTable *) gog_trend_line_types ());
	while (tl == NULL && g_hash_table_iter_next (&iter, &key, &value)) {
		GogTrendLine *cand = gog_trend_line_new_by_name (
			((GogTrendLineType *) value)->engine);
		if (cand != NULL && GOG_IS_REG_CURVE (cand))
			tl = cand;
		else if (cand != NULL)
			g_object_unref (cand);
	}
	if (tl == NULL) {
		skip ("reg-curve getter (no regression engine)");
		g_object_unref (graph);
		return;
	}
	if (gog_object_add_by_name2 (GOG_OBJECT (series), "Regression curve",
				     GOG_OBJECT (tl)) == NULL) {
		skip ("reg-curve getter (curve not accepted)");
		g_object_unref (graph);
		return;
	}
	g_signal_connect (G_OBJECT (tl), "changed",
			  G_CALLBACK (count_changed), &changed);
	g_object_get (G_OBJECT (tl), "drawing-bounds", &str, NULL);
	g_free (str);
	check (changed == 0,
			 "reading \"drawing-bounds\" does not emit changed "
			 "(gog-reg-curve.c:222)");

	g_object_unref (graph);
}

static void
test_single_stop_color_map (void)
{
	GOColor one[1] = { GO_COLOR_RED };
	GogAxisColorMap *map = gog_axis_color_map_from_colors ("one-stop", 1,
							       one,
							       GO_RESOURCE_RW);
	cairo_surface_t *surface;
	cairo_t *cr;
	unsigned char *data;
	int stride;
	guint32 pixel;
	unsigned r, g, b;

	CHECK (map != NULL);
	if (map == NULL)
		return;
	CHECK (gog_axis_color_map_get_max (map) == 0);
	CHECK (color_near (gog_axis_color_map_get_color (map, 0.),
			   GO_COLOR_RED, 1));

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 40, 10);
	cr = cairo_create (surface);
	cairo_set_source_rgb (cr, 1., 1., 1.);
	cairo_paint (cr);
	gog_axis_color_map_to_cairo (map, cr, 0, TRUE, 40., 10.);
	cairo_destroy (cr);
	cairo_surface_flush (surface);

	data = cairo_image_surface_get_data (surface);
	stride = cairo_image_surface_get_stride (surface);
	pixel = *(guint32 *) (data + 5 * stride + 20 * 4);
	r = (pixel >> 16) & 0xff;
	g = (pixel >> 8) & 0xff;
	b = pixel & 0xff;
	check (r > 200 && g < 60 && b < 60,
	       "single-stop colour map paints its colour "
	       "(gog-axis-color-map.c:593)");

	cairo_surface_destroy (surface);
	g_object_unref (map);
}

static void
test_trend_line_pre_insert_legend (void)
{
	char const *engine = first_trend_line_engine_name ();
	GogTrendLine *tl;

	if (engine == NULL) {
		skip ("trend-line pre-insert legend (no engine)");
		return;
	}
	tl = gog_trend_line_new_by_name (engine);
	check (tl != NULL,
	       "\"has-legend\" before insertion does not crash "
	       "(gog-trend-line.c:44)");
	if (tl != NULL) {
		g_object_set (tl, "has-legend", FALSE, NULL);
		g_object_unref (tl);
	}
}

static void
test_chart_map_3d_non_xyz (void)
{
	GogGraph *graph = new_graph (200., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY");
	GogView *view;
	GogViewAllocation area;
	GogChartMap3D *map;
	guint handler;

	if (plot == NULL) {
		skip ("3D map on a non-XYZ chart (no XY plot)");
		g_object_unref (graph);
		return;
	}

	view = gog_object_new_view (GOG_OBJECT (chart), NULL);
	CHECK (view != NULL);
	if (view == NULL) {
		g_object_unref (graph);
		return;
	}

	area.x = area.y = 0.;
	area.w = 200.;
	area.h = 100.;

	/* The unimplemented axis set warns.  */
	handler = g_log_set_handler (NULL, G_LOG_LEVEL_WARNING, quiet_log, NULL);
	map = gog_chart_map_3d_new (view, &area,
				    gog_plot_get_axis (plot, GOG_AXIS_X),
				    gog_plot_get_axis (plot, GOG_AXIS_Y),
				    NULL);
	g_log_remove_handler (NULL, handler);

	CHECK (map != NULL);
	if (map != NULL) {
		CHECK (!gog_chart_map_3d_is_valid (map));
		CHECK (gog_chart_map_3d_get_axis_map (map, 0) == NULL);
		CHECK (gog_chart_map_3d_get_axis_map (map, 1) == NULL);
		CHECK (gog_chart_map_3d_get_axis_map (map, 2) == NULL);
		gog_chart_map_3d_free (map);
	}

	g_object_unref (view);
	g_object_unref (graph);
}

static void
test_renderer_label_scale (void)
{
	GogGraph *graph;
	GogChart *chart;
	GogPlot *plot;
	GogSeries *series;
	GogObject *labels_obj;
	GOStyle *style;
	cairo_surface_t *surface;
	gboolean ok = FALSE;
	unsigned rx0, ry0, rx1, ry1, bx0, by0, bx1, by1;

	graph = new_graph (100., 100.);
	chart = graph_add_chart (graph);
	plot = chart_add_plot (chart, "XY", "XY Lines");
	if (plot == NULL) {
		skip ("label scale test (no XY plot)");
		g_object_unref (graph);
		return;
	}
	series = plot_add_xy_series (plot,
				     (double const[]) { 20., 50., 80. },
				     (double const[]) { 60., 70., 80. }, 3);

	/* Plain black line, no marker or fill, so that the only red or blue
	 * pixels in the render come from the data labels below.  */
	style = go_style_new ();
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 1.;
	style->line.auto_dash = FALSE;
	style->line.auto_color = FALSE;
	style->line.auto_fore = FALSE;
	style->line.auto_width = FALSE;
	style->line.color = GO_COLOR_BLACK;
	style->marker.auto_shape = FALSE;
	go_marker_set_shape (style->marker.mark, GO_MARKER_NONE);
	go_styled_object_set_style (GO_STYLED_OBJECT (series), style);
	g_object_unref (style);

	/* Labels: red background, blue 24 pt text.  */
	labels_obj = gog_object_add_by_name2 (GOG_OBJECT (series),
					     "Data labels", NULL);
	CHECK (labels_obj != NULL);
	style = go_style_new ();
	style->fill.type = GO_STYLE_FILL_PATTERN;
	style->fill.auto_type = FALSE;
	style->fill.auto_fore = FALSE;
	style->fill.auto_back = FALSE;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_RED);
	style->line.dash_type = GO_LINE_NONE;
	style->line.width = 0.;
	style->line.auto_dash = FALSE;
	style->line.auto_color = FALSE;
	style->line.auto_fore = FALSE;
	style->font.auto_font = FALSE;
	go_style_set_font (style, go_font_new_by_name ("Sans 24"));
	style->font.auto_color = FALSE;
	style->font.color = GO_COLOR_BLUE;
	go_styled_object_set_style (GO_STYLED_OBJECT (labels_obj), style);
	g_object_unref (style);

	gog_object_request_update (labels_obj);
	gog_object_update (labels_obj);

	/* Render at 4x scale; the background must frame the text.  The bug
	 * is that the background origin is not scaled, so the frame is
	 * displaced and the text spills out.  */
	surface = render_graph (graph, 400., 400., &ok);
	CHECK (ok);
	CHECK (color_bbox (surface, GO_COLOR_RED, &rx0, &ry0, &rx1, &ry1));
	CHECK (color_bbox (surface, GO_COLOR_BLUE, &bx0, &by0, &bx1, &by1));
	check (bx0 >= rx0 - 1 && by0 >= ry0 - 1 &&
	       bx1 <= rx1 + 1 && by1 <= ry1 + 1,
	       "data-label background frames the text at 4x scale "
	       "(gog-renderer.c:1362)");

	cairo_surface_destroy (surface);
	g_object_unref (graph);
}

/* Data-label style: red fill, blue text, no line interest, but a line
 * width that must be ignored when the line is not interesting.  */
static GOStyle *
make_label_style (double line_width)
{
	GOStyle *style = go_style_new ();

	style->fill.type = GO_STYLE_FILL_PATTERN;
	style->fill.auto_type = FALSE;
	style->fill.auto_fore = FALSE;
	style->fill.auto_back = FALSE;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_RED);
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = line_width;
	style->line.auto_dash = FALSE;
	style->line.auto_color = FALSE;
	style->line.auto_fore = FALSE;
	style->line.auto_width = FALSE;
	style->interesting_fields &= ~GO_STYLE_LINE;
	style->font.auto_font = FALSE;
	go_style_set_font (style, go_font_new_by_name ("Sans 12"));
	style->font.auto_color = FALSE;
	style->font.color = GO_COLOR_BLUE;

	return style;
}

static void
test_renderer_label_line_width (void)
{
	GogGraph *graph;
	GogChart *chart;
	GogPlot *plot;
	GogSeries *series;
	GogObject *labels_obj;
	GOStyle *style;
	cairo_surface_t *surface;
	gboolean ok = FALSE;
	unsigned rx0, ry0, rx1, ry1, qx0, qy0, qx1, qy1;

	graph = new_graph (100., 100.);
	chart = graph_add_chart (graph);
	plot = chart_add_plot (chart, "XY", "XY Lines");
	if (plot == NULL) {
		skip ("label line-width test (no XY plot)");
		g_object_unref (graph);
		return;
	}
	series = plot_add_xy_series (plot,
				     (double const[]) { 20., 50., 80. },
				     (double const[]) { 60., 70., 80. }, 3);

	style = go_style_new ();
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 1.;
	style->line.auto_dash = FALSE;
	style->line.auto_color = FALSE;
	style->line.auto_fore = FALSE;
	style->line.auto_width = FALSE;
	style->line.color = GO_COLOR_BLACK;
	style->marker.auto_shape = FALSE;
	go_marker_set_shape (style->marker.mark, GO_MARKER_NONE);
	go_styled_object_set_style (GO_STYLED_OBJECT (series), style);
	g_object_unref (style);

	labels_obj = gog_object_add_by_name2 (GOG_OBJECT (series),
					     "Data labels", NULL);
	CHECK (labels_obj != NULL);

	/* The line width is not an interesting field, so changing it must
	 * not move the label.  */
	style = make_label_style (2.);
	go_styled_object_set_style (GO_STYLED_OBJECT (labels_obj), style);
	g_object_unref (style);
	gog_object_request_update (labels_obj);
	gog_object_update (labels_obj);
	surface = render_graph (graph, 400., 400., &ok);
	CHECK (ok);
	CHECK (color_bbox (surface, GO_COLOR_RED, &rx0, &ry0, &rx1, &ry1));
	cairo_surface_destroy (surface);

	style = make_label_style (12.);
	go_styled_object_set_style (GO_STYLED_OBJECT (labels_obj), style);
	g_object_unref (style);
	gog_object_request_update (labels_obj);
	gog_object_update (labels_obj);
	surface = render_graph (graph, 400., 400., &ok);
	CHECK (ok);
	CHECK (color_bbox (surface, GO_COLOR_RED, &qx0, &qy0, &qx1, &qy1));
	cairo_surface_destroy (surface);

	check (abs ((int) qx0 - (int) rx0) <= 1 &&
	       abs ((int) qy0 - (int) ry0) <= 1 &&
	       abs ((int) qx1 - (int) rx1) <= 1 &&
	       abs ((int) qy1 - (int) ry1) <= 1,
	       "uninteresting line width does not move the label "
	       "(gog-renderer.c:1326)");

	g_object_unref (graph);
}

static void
test_renderer_class_macro (void)
{
	GogGraph *graph = new_graph (50., 50.);
	GogRenderer *renderer = gog_renderer_new (graph);
	GogRendererClass *klass = GOG_RENDERER_GET_CLASS (renderer);

	CHECK (klass != NULL);
	g_object_unref (renderer);
	g_object_unref (graph);
}

static void
test_chart_map_close_paths (void)
{
	GogGraph *graph = new_graph (100., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY Lines");
	GogAxis *x_axis, *y_axis;
	GogAxisMap *x_map, *y_map;
	GogViewAllocation area;
	GogChartMap *map;
	GOPath *path;
	PathSummary s;
	double ext, expect;
	static const double xs[3] = { 1., 2., 3. };
	static const double ys[3] = { 4., 6., 5. };

	if (plot == NULL) {
		skip ("close-path test (no XY plot)");
		g_object_unref (graph);
		return;
	}
	x_axis = gog_plot_get_axis (plot, GOG_AXIS_X);
	y_axis = gog_plot_get_axis (plot, GOG_AXIS_Y);
	gog_axis_set_bounds (x_axis, 0., 10.);
	gog_axis_set_bounds (y_axis, 0., 10.);
	area.x = area.y = 0.;
	area.w = area.h = 100.;
	map = gog_chart_map_new (chart, &area, x_axis, y_axis, NULL, FALSE);
	CHECK (map != NULL && gog_chart_map_is_valid (map));
	if (map == NULL) {
		g_object_unref (graph);
		return;
	}
	x_map = gog_chart_map_get_axis_map (map, 0);
	y_map = gog_chart_map_get_axis_map (map, 1);

	/* The X_AXIS_MIN closing line is vertical, at the x-axis minimum.  */
	path = gog_chart_map_make_close_path (map, xs, ys, 3,
					      GOG_SERIES_FILL_TYPE_X_AXIS_MIN);
	CHECK (path != NULL);
	if (path != NULL) {
		s = path_summary (path);
		gog_axis_map_get_real_bounds (x_map, &ext, NULL);
		expect = gog_axis_map_to_view (x_map, ext);
		check_near (s.first_x, expect, 1e-9, "X_AXIS_MIN u (first)");
		check_near (s.last_x, expect, 1e-9, "X_AXIS_MIN u (last)");
		check_near (s.first_y, gog_axis_map_to_view (y_map, 4.), 1e-9,
			    "X_AXIS_MIN v (first)");
		check_near (s.last_y, gog_axis_map_to_view (y_map, 5.), 1e-9,
			    "X_AXIS_MIN v (last)");
		go_path_free (path);
	}

	path = gog_chart_map_make_close_path (map, xs, ys, 3,
					      GOG_SERIES_FILL_TYPE_X_AXIS_MAX);
	CHECK (path != NULL);
	if (path != NULL) {
		s = path_summary (path);
		gog_axis_map_get_real_bounds (x_map, NULL, &ext);
		expect = gog_axis_map_to_view (x_map, ext);
		check_near (s.first_x, expect, 1e-9, "X_AXIS_MAX u (first)");
		check_near (s.last_x, expect, 1e-9, "X_AXIS_MAX u (last)");
		check_near (s.first_y, gog_axis_map_to_view (y_map, 4.), 1e-9,
			    "X_AXIS_MAX v (first)");
		check_near (s.last_y, gog_axis_map_to_view (y_map, 5.), 1e-9,
			    "X_AXIS_MAX v (last)");
		go_path_free (path);
	}

	gog_chart_map_free (map);
	g_object_unref (graph);
}

static void
test_chart_map_inverted_cspline (void)
{
	GogGraph *graph = new_graph (100., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY Lines");
	GogAxis *x_axis, *y_axis;
	GogViewAllocation area;
	GogChartMap *map;
	GOPath *path;
	PathSummary s;
	static const double xs[4] = { 0., 3., 6., 9. };
	static const double ys[4] = { 1., 4., 2., 5. };

	if (plot == NULL) {
		skip ("inverted cspline test (no XY plot)");
		g_object_unref (graph);
		return;
	}
	x_axis = gog_plot_get_axis (plot, GOG_AXIS_X);
	y_axis = gog_plot_get_axis (plot, GOG_AXIS_Y);
	gog_axis_set_bounds (x_axis, 0., 10.);
	gog_axis_set_bounds (y_axis, 0., 10.);
	g_object_set (x_axis, "invert-axis", TRUE, NULL);
	area.x = area.y = 0.;
	area.w = area.h = 100.;
	map = gog_chart_map_new (chart, &area, x_axis, y_axis, NULL, FALSE);
	CHECK (map != NULL && gog_chart_map_is_valid (map));
	if (map == NULL) {
		g_object_unref (graph);
		return;
	}

	/* View-space x decreases on an inverted axis; the cubic spline
	 * path must still be built.  */
	path = gog_chart_map_make_path (map, xs, ys, 4,
					GO_LINE_INTERPOLATION_CUBIC_SPLINE,
					FALSE, NULL);
	CHECK (path != NULL);
	s = path_summary (path);
	check (s.n_segments > 0,
	       "inverted-axis cubic spline is not empty "
	       "(gog-chart-map.c:428)");
	go_path_free (path);

	gog_chart_map_free (map);
	g_object_unref (graph);
}

/* First and last tangent of a path, recovered from the Bézier control
 * points.  The curve callback provides (first control, second control,
 * end); the start of the path comes from its initial move.  */
typedef struct {
	gboolean have_start, have_first;
	double start_x, start_y;
	double first_cx, first_cy;
	double last_cx, last_cy, last_x, last_y;
} ClampedTangents;

static void
clamped_tangents_move (gpointer closure, GOPathPoint const *p)
{
	ClampedTangents *ct = closure;

	if (!ct->have_start) {
		ct->have_start = TRUE;
		ct->start_x = p->x;
		ct->start_y = p->y;
	}
}

static void
clamped_tangents_line (G_GNUC_UNUSED gpointer closure,
		       G_GNUC_UNUSED GOPathPoint const *p)
{
}

static void
clamped_tangents_curve (gpointer closure, GOPathPoint const *p0,
			GOPathPoint const *p1, GOPathPoint const *p2)
{
	ClampedTangents *ct = closure;

	if (!ct->have_first) {
		ct->have_first = TRUE;
		ct->first_cx = p0->x;
		ct->first_cy = p0->y;
	}
	ct->last_cx = p1->x;
	ct->last_cy = p1->y;
	ct->last_x = p2->x;
	ct->last_y = p2->y;
}

static void
clamped_tangents_close (G_GNUC_UNUSED gpointer closure)
{
}

static ClampedTangents
clamped_tangents (GOPath const *path)
{
	ClampedTangents ct = { FALSE, FALSE, 0., 0., 0., 0., 0., 0., 0., 0. };

	go_path_interpret (path, GO_PATH_DIRECTION_FORWARD,
			   clamped_tangents_move, clamped_tangents_line,
			   clamped_tangents_curve, clamped_tangents_close,
			   &ct);
	return ct;
}

static double
expected_view_slope (GogAxisMap *x_map, GogAxisMap *y_map,
		     double deriv, double x, double y)
{
	double du = gog_axis_map_derivative_to_view (x_map, x);
	double dv = gog_axis_map_derivative_to_view (y_map, y);

	return deriv * dv / du;
}

/* gog-chart-map.c:424: the clamped spline's end derivatives are data
 * space slopes; convert them using the data coordinates of the end
 * points, swapping them when the view coordinates get reversed.  */
static void
test_chart_map_clamped_derivs (void)
{
	GogGraph *graph = new_graph (100., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY Lines");
	GogAxis *x_axis, *y_axis;
	GogAxisMap *x_map, *y_map;
	GogViewAllocation area;
	GogChartMap *map;
	GOPath *path;
	ClampedTangents ct;
	double derivs[2] = { 5., -5. };
	static const double xs[3] = { 1., 2., 3. };
	static const double ys[3] = { 1., 2., 3. };

	if (plot == NULL) {
		skip ("clamped derivative test (no XY plot)");
		g_object_unref (graph);
		return;
	}
	x_axis = gog_plot_get_axis (plot, GOG_AXIS_X);
	y_axis = gog_plot_get_axis (plot, GOG_AXIS_Y);
	/* A log axis makes the mapping derivative depend on the value.  */
	g_object_set (x_axis, "map-name", "Log", NULL);
	gog_axis_set_bounds (x_axis, 0.5, 4.);
	gog_axis_set_bounds (y_axis, 0., 4.);
	area.x = area.y = 0.;
	area.w = area.h = 100.;
	map = gog_chart_map_new (chart, &area, x_axis, y_axis, NULL, FALSE);
	CHECK (map != NULL && gog_chart_map_is_valid (map));
	if (map == NULL) {
		g_object_unref (graph);
		return;
	}
	x_map = gog_chart_map_get_axis_map (map, 0);
	y_map = gog_chart_map_get_axis_map (map, 1);

	/* The first data point is first in view space too.  */
	path = gog_chart_map_make_path (map, xs, ys, 3,
					GO_LINE_INTERPOLATION_CLAMPED_CUBIC_SPLINE,
					TRUE, derivs);
	CHECK (path != NULL);
	if (path != NULL) {
		ct = clamped_tangents (path);
		CHECK (ct.have_start && ct.have_first);
		if (ct.have_start && ct.have_first) {
			check_near ((ct.first_cy - ct.start_y) /
				    (ct.first_cx - ct.start_x),
				    expected_view_slope (x_map, y_map,
							 derivs[0], xs[0], ys[0]),
				    1e-9, "clamped start tangent");
			check_near ((ct.last_y - ct.last_cy) /
				    (ct.last_x - ct.last_cx),
				    expected_view_slope (x_map, y_map,
							 derivs[1], xs[2], ys[2]),
				    1e-9, "clamped end tangent");
		}
		go_path_free (path);
	}
	gog_chart_map_free (map);

	/* An inverted axis reverses the view coordinates: the first data
	 * point is at the end of the path, so the clamped derivatives
	 * swap.  */
	g_object_set (x_axis, "invert-axis", TRUE, NULL);
	map = gog_chart_map_new (chart, &area, x_axis, y_axis, NULL, FALSE);
	CHECK (map != NULL && gog_chart_map_is_valid (map));
	if (map != NULL) {
		x_map = gog_chart_map_get_axis_map (map, 0);
		y_map = gog_chart_map_get_axis_map (map, 1);

		path = gog_chart_map_make_path (map, xs, ys, 3,
						GO_LINE_INTERPOLATION_CLAMPED_CUBIC_SPLINE,
						TRUE, derivs);
		CHECK (path != NULL);
		if (path != NULL) {
			ct = clamped_tangents (path);
			CHECK (ct.have_start && ct.have_first);
			if (ct.have_start && ct.have_first) {
				check_near ((ct.first_cy - ct.start_y) /
					    (ct.first_cx - ct.start_x),
					    expected_view_slope (x_map, y_map,
								 derivs[1], xs[2], ys[2]),
					    1e-9, "clamped start tangent "
					    "(inverted axis)");
				check_near ((ct.last_y - ct.last_cy) /
					    (ct.last_x - ct.last_cx),
					    expected_view_slope (x_map, y_map,
								 derivs[0], xs[0], ys[0]),
					    1e-9, "clamped end tangent "
					    "(inverted axis)");
			}
			go_path_free (path);
		}
		gog_chart_map_free (map);
	}

	g_object_unref (graph);
}

#define PATH_WALK_MAX_SEGS 32

typedef struct {
	gboolean have_current;
	double cur_x, cur_y;
	gboolean closed;
	unsigned n;
	struct {
		gboolean is_curve;
		double sx, sy;
		double c1x, c1y, c2x, c2y;
		double ex, ey;
	} segs[PATH_WALK_MAX_SEGS];
} PathWalk;

static void
path_walk_move (gpointer closure, GOPathPoint const *p)
{
	PathWalk *w = closure;

	w->have_current = TRUE;
	w->cur_x = p->x;
	w->cur_y = p->y;
}

static void
path_walk_line (gpointer closure, GOPathPoint const *p)
{
	PathWalk *w = closure;

	if (w->n < PATH_WALK_MAX_SEGS) {
		w->segs[w->n].is_curve = FALSE;
		w->segs[w->n].sx = w->cur_x;
		w->segs[w->n].sy = w->cur_y;
		w->segs[w->n].ex = p->x;
		w->segs[w->n].ey = p->y;
		w->n++;
	}
	w->cur_x = p->x;
	w->cur_y = p->y;
}

static void
path_walk_curve (gpointer closure, GOPathPoint const *p0,
		 GOPathPoint const *p1, GOPathPoint const *p2)
{
	PathWalk *w = closure;

	if (w->n < PATH_WALK_MAX_SEGS) {
		w->segs[w->n].is_curve = TRUE;
		w->segs[w->n].sx = w->cur_x;
		w->segs[w->n].sy = w->cur_y;
		w->segs[w->n].c1x = p0->x;
		w->segs[w->n].c1y = p0->y;
		w->segs[w->n].c2x = p1->x;
		w->segs[w->n].c2y = p1->y;
		w->segs[w->n].ex = p2->x;
		w->segs[w->n].ey = p2->y;
		w->n++;
	}
	w->cur_x = p2->x;
	w->cur_y = p2->y;
}

static void
path_walk_close (gpointer closure)
{
	((PathWalk *) closure)->closed = TRUE;
}

static PathWalk
path_walk (GOPath const *path)
{
	PathWalk w;

	memset (&w, 0, sizeof (w));
	go_path_interpret (path, GO_PATH_DIRECTION_FORWARD,
			   path_walk_move, path_walk_line, path_walk_curve,
			   path_walk_close, &w);
	return w;
}

/* Interpolation-specific properties of the built paths: the spline
 * variants pass through the data points and are C1 continuous, and the
 * step variants consist of axis-aligned segments of a known count.  */
static void
test_chart_map_path_properties (void)
{
	GogGraph *graph = new_graph (100., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY Lines");
	GogAxis *x_axis, *y_axis;
	GogAxisMap *x_map, *y_map;
	GogViewAllocation area;
	GogChartMap *map;
	GOPath *path;
	PathWalk w;
	double derivs[2] = { 0.5, -0.5 };
	static const double xs[5] = { 0., 1., 2., 3., 4. };
	static const double ys[5] = { 1., 3., 2., 5., 4. };
	struct {
		GOLineInterpolation type;
		gboolean clamped;
		char const *name;
	} splines[] = {
		{ GO_LINE_INTERPOLATION_SPLINE, FALSE, "spline" },
		{ GO_LINE_INTERPOLATION_CUBIC_SPLINE, FALSE, "cspline" },
		{ GO_LINE_INTERPOLATION_PARABOLIC_CUBIC_SPLINE, FALSE, "parabolic" },
		{ GO_LINE_INTERPOLATION_CUBIC_CUBIC_SPLINE, FALSE, "cubic" },
		{ GO_LINE_INTERPOLATION_CLAMPED_CUBIC_SPLINE, TRUE, "clamped" }
	};
	struct {
		GOLineInterpolation type;
		unsigned segs;
		char const *name;
	} steps[] = {
		{ GO_LINE_INTERPOLATION_STEP_START, 2 * 4, "step-start" },
		{ GO_LINE_INTERPOLATION_STEP_END, 2 * 4, "step-end" },
		{ GO_LINE_INTERPOLATION_STEP_CENTER_X, 3 * 4, "step-center-x" },
		{ GO_LINE_INTERPOLATION_STEP_CENTER_Y, 3 * 4, "step-center-y" }
	};
	unsigned i;

	if (plot == NULL) {
		skip ("path property test (no XY plot)");
		g_object_unref (graph);
		return;
	}
	x_axis = gog_plot_get_axis (plot, GOG_AXIS_X);
	y_axis = gog_plot_get_axis (plot, GOG_AXIS_Y);
	gog_axis_set_bounds (x_axis, 0., 4.);
	gog_axis_set_bounds (y_axis, 0., 6.);
	area.x = area.y = 0.;
	area.w = area.h = 100.;
	map = gog_chart_map_new (chart, &area, x_axis, y_axis, NULL, FALSE);
	CHECK (map != NULL && gog_chart_map_is_valid (map));
	if (map == NULL) {
		g_object_unref (graph);
		return;
	}
	x_map = gog_chart_map_get_axis_map (map, 0);
	y_map = gog_chart_map_get_axis_map (map, 1);

	for (i = 0; i < G_N_ELEMENTS (splines); i++) {
		gboolean knots_ok = TRUE, c1_ok = TRUE;
		char *msg;
		unsigned j;

		path = gog_chart_map_make_path (map, xs, ys, 5, splines[i].type,
						TRUE,
						splines[i].clamped ? derivs : NULL);
		CHECK (path != NULL);
		w = path_walk (path);

		if (w.n != 4) {
			knots_ok = c1_ok = FALSE;
		} else {
			/* The path passes through the mapped data points.  */
			if (fabs (w.segs[0].sx -
				  gog_axis_map_to_view (x_map, xs[0])) > 1e-9 ||
			    fabs (w.segs[0].sy -
				  gog_axis_map_to_view (y_map, ys[0])) > 1e-9)
				knots_ok = FALSE;
			for (j = 0; j < w.n; j++) {
				double ex = gog_axis_map_to_view (x_map, xs[j + 1]);
				double ey = gog_axis_map_to_view (y_map, ys[j + 1]);

				if (!w.segs[j].is_curve ||
				    fabs (w.segs[j].ex - ex) > 1e-9 ||
				    fabs (w.segs[j].ey - ey) > 1e-9)
					knots_ok = FALSE;
			}
			/* Tangents agree across the joins.  */
			for (j = 1; j < w.n; j++) {
				double end_slope =
					(w.segs[j - 1].ey - w.segs[j - 1].c2y) /
					(w.segs[j - 1].ex - w.segs[j - 1].c2x);
				double start_slope =
					(w.segs[j].c1y - w.segs[j].sy) /
					(w.segs[j].c1x - w.segs[j].sx);

				if (fabs (end_slope - start_slope) > 1e-6)
					c1_ok = FALSE;
			}
		}
		msg = g_strdup_printf ("%s path passes through the data points",
				       splines[i].name);
		check (knots_ok, msg);
		g_free (msg);
		msg = g_strdup_printf ("%s path is C1 continuous",
				       splines[i].name);
		check (c1_ok, msg);
		g_free (msg);
		go_path_free (path);
	}

	for (i = 0; i < G_N_ELEMENTS (steps); i++) {
		gboolean aligned_ok = TRUE, end_ok = FALSE;
		char *msg;
		unsigned j;

		path = gog_chart_map_make_path (map, xs, ys, 5, steps[i].type,
						TRUE, NULL);
		CHECK (path != NULL);
		w = path_walk (path);

		if (w.n != steps[i].segs) {
			aligned_ok = FALSE;
		} else {
			for (j = 0; j < w.n; j++) {
				double dx = w.segs[j].ex - w.segs[j].sx;
				double dy = w.segs[j].ey - w.segs[j].sy;

				if (w.segs[j].is_curve ||
				    !(fabs (dx) < 1e-9 || fabs (dy) < 1e-9))
					aligned_ok = FALSE;
			}
			end_ok = fabs (w.segs[w.n - 1].ex -
				       gog_axis_map_to_view (x_map, xs[4])) < 1e-9 &&
				 fabs (w.segs[w.n - 1].ey -
				       gog_axis_map_to_view (y_map, ys[4])) < 1e-9;
		}
		msg = g_strdup_printf ("%s path has %u axis-aligned line segments",
				       steps[i].name, steps[i].segs);
		check (aligned_ok, msg);
		g_free (msg);
		msg = g_strdup_printf ("%s path ends at the last data point",
				       steps[i].name);
		check (end_ok, msg);
		g_free (msg);
		go_path_free (path);
	}

	gog_chart_map_free (map);
	g_object_unref (graph);
}

static void
test_chart_map_polar_step (void)
{
	GogGraph *graph = new_graph (100., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "Polar", "Polar");
	GogAxis *circular, *radial;
	GogViewAllocation area;
	GogChartMap *map;
	GOPath *path;
	PathSummary s;
	double expect_x, expect_y;
	static const double xs[5] = { 0., 1., 2., 3., 4. };
	static const double ys[5] = { 1., 2., 3., 4., 5. };

	if (plot == NULL) {
		skip ("polar step test (no Polar plot)");
		g_object_unref (graph);
		return;
	}
	circular = gog_plot_get_axis (plot, GOG_AXIS_CIRCULAR);
	radial = gog_plot_get_axis (plot, GOG_AXIS_RADIAL);
	gog_axis_set_bounds (circular, 0., 4.);
	gog_axis_set_bounds (radial, 0., 10.);
	area.x = area.y = 0.;
	area.w = area.h = 100.;
	map = gog_chart_map_new (chart, &area, circular, radial, NULL, FALSE);
	CHECK (map != NULL && gog_chart_map_is_valid (map));
	if (map == NULL) {
		g_object_unref (graph);
		return;
	}

	path = gog_chart_map_make_path (map, xs, ys, 5,
					GO_LINE_INTERPOLATION_STEP_START,
					FALSE, NULL);
	CHECK (path != NULL);
	s = path_summary (path);
	gog_chart_map_2D_to_view (map, 4., 5., &expect_x, &expect_y);
	check_near (s.last_x, expect_x, 1e-9,
		    "polar STEP_START reaches the last point (x)");
	check_near (s.last_y, expect_y, 1e-9,
		    "polar STEP_START reaches the last point (y)");
	go_path_free (path);

	gog_chart_map_free (map);
	g_object_unref (graph);
}

static void
test_chart_map_unimplemented (void)
{
	GogGraph *graph = new_graph (100., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "Surface", "XYZSurface");
	GogAxis *x_axis, *y_axis, *z_axis;
	GogViewAllocation area;
	GogChartMap *map;
	guint handler;
	static const double xs[4] = { 0., 3., 6., 9. };
	static const double ys[4] = { 1., 4., 2., 5. };

	if (plot == NULL) {
		skip ("unimplemented-axis-set test (no 3D plot)");
		g_object_unref (graph);
		return;
	}
	x_axis = gog_plot_get_axis (plot, GOG_AXIS_X);
	y_axis = gog_plot_get_axis (plot, GOG_AXIS_Y);
	z_axis = gog_plot_get_axis (plot, GOG_AXIS_Z);
	gog_axis_set_bounds (x_axis, 0., 10.);
	gog_axis_set_bounds (y_axis, 0., 10.);
	gog_axis_set_bounds (z_axis, 0., 10.);
	area.x = area.y = 0.;
	area.w = area.h = 100.;

	/* Unimplemented axis set: the map must be inert, not jump
	 * through an uninitialized function pointer.  */
	handler = g_log_set_handler (NULL, G_LOG_LEVEL_WARNING, quiet_log, NULL);
	map = gog_chart_map_new (chart, &area, x_axis, y_axis, z_axis, FALSE);
	CHECK (map != NULL);
	if (map != NULL) {
		CHECK (!gog_chart_map_is_valid (map));
		CHECK (gog_chart_map_make_path (map, xs, ys, 4,
						GO_LINE_INTERPOLATION_LINEAR,
						FALSE, NULL) == NULL);
		CHECK (gog_chart_map_make_close_path (map, xs, ys, 4,
						      GOG_SERIES_FILL_TYPE_Y_ORIGIN) == NULL);
		gog_chart_map_free (map);
	}
	g_log_remove_handler (NULL, handler);

	g_object_unref (graph);
}

static void
test_axis_date_singleton (void)
{
	GogGraph *graph = new_graph (100., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY Lines");
	GogAxis *axis;
	GOFormat *fmt;
	double min, max;

	if (plot == NULL) {
		skip ("date singleton test (no XY plot)");
		g_object_unref (graph);
		return;
	}
	plot_add_xy_series (plot, (double const[]) { 45000. },
			    (double const[]) { 1. }, 1);
	axis = gog_plot_get_axis (plot, GOG_AXIS_X);
	fmt = go_format_new_from_XL ("yyyy-mm-dd");
	CHECK (go_format_is_date (fmt) > 0);
	CHECK (gog_axis_set_format (axis, fmt));
	/* The axis asks the plot for its bounds, and the plot serves them
	 * from a model refreshed by its own update vfunc.  */
	gog_object_request_update (GOG_OBJECT (plot));
	gog_object_update (GOG_OBJECT (plot));
	gog_object_request_update (GOG_OBJECT (axis));
	gog_object_update (GOG_OBJECT (axis));

	/* A single date must give a non-empty range.  */
	CHECK (gog_axis_get_bounds (axis, &min, &max));
	check (min < max, "single-date axis has a non-empty range "
	       "(gog-axis.c:730)");

	g_object_unref (graph);
}

static void
test_axis_log_tick_cap (void)
{
	GogGraph *graph = new_graph (100., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY Lines");
	GogSeries *series;
	GogAxis *axis;
	GogAxisTick *ticks;
	unsigned n;

	if (plot == NULL) {
		skip ("log tick cap test (no XY plot)");
		g_object_unref (graph);
		return;
	}
	series = plot_add_xy_series (plot, (double const[]) { 1., 2. },
				     (double const[]) { 1., 2. }, 2);
	axis = gog_plot_get_axis (plot, GOG_AXIS_X);
	g_object_set (axis, "map-name", "Log", NULL);
	gog_axis_set_bounds (axis, 1e-30, 1e30);
	gog_dataset_set_dim (GOG_DATASET (axis), GOG_AXIS_ELEM_MAJOR_TICK,
			     go_data_scalar_val_new (10.), NULL);
	gog_dataset_set_dim (GOG_DATASET (axis), GOG_AXIS_ELEM_MINOR_TICK,
			     go_data_scalar_val_new (89.), NULL);
	gog_object_request_update (GOG_OBJECT (series));
	gog_object_update (GOG_OBJECT (series));
	gog_object_request_update (GOG_OBJECT (axis));
	gog_object_update (GOG_OBJECT (axis));

	/* 60 decades at 10 per major tick: the automatic minor tick count
	 * would give far more ticks than the cap unless the ladder is
	 * retried at the next lower step.  */
	n = gog_axis_get_ticks (axis, &ticks);
	check (n > 0 && n < 250,
	       "log axis respects the tick cap (gog-axis.c:1525)");

	g_object_unref (graph);
}

static void
test_chart_is_3d_superset (void)
{
	GogGraph *graph = new_graph (100., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "Radar", "Radar");
	GogAxisSet superset;

	if (plot == NULL) {
		skip ("3D superset test (no Radar plot)");
		g_object_unref (graph);
		return;
	}
	/* Assigning a 3D set to a chart that already has radar axes folds
	 * their types into the chart's set, making it a superset.  */
	superset = GOG_AXIS_SET_XYZ | (1 << GOG_AXIS_CIRCULAR) |
		(1 << GOG_AXIS_RADIAL);
	gog_chart_axis_set_assign (chart, superset);
	CHECK (gog_chart_get_axis_set (chart) == superset);
	check (gog_chart_is_3d (chart),
	       "a superset axis set containing XYZ is still 3D "
	       "(gog-chart.c:1268)");

	g_object_unref (graph);
}

/* gog-series-labels.c:875/1353: an out-of-range %<digit> must not read
 * past the series values array and a format ending in '%' must not read
 * past the format string.  */
static void
test_regression_label_format_bounds (void)
{
	GogGraph *graph = new_graph (200., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY");
	GogSeries *series;
	GogObject *labels;
	GogSeriesLabelElt const *elt;
	gboolean ok = TRUE;

	if (plot == NULL) {
		skip ("label format bounds regression (no XY plot)");
		g_object_unref (graph);
		return;
	}
	series = plot_add_xy_series (plot, (double const[]) { 1., 2. },
				     (double const[]) { 3., 4. }, 2);
	labels = gog_object_add_by_name2 (GOG_OBJECT (series),
					 "Data labels", NULL);
	if (labels == NULL) {
		skip ("label format bounds regression (no data labels)");
		g_object_unref (graph);
		return;
	}

	/* The XY plot has two data dimensions; %5 is out of range.  */
	g_object_set (labels, "format", "%5", NULL);
	gog_object_request_update (labels);
	gog_object_update (labels);
	elt = gog_series_labels_vector_get_element (
		GOG_SERIES_LABELS (labels), 0);
	ok = ok && elt != NULL && elt->str != NULL &&
	     elt->str->str[0] == '\0';

	/* A trailing '%' is an unexpected string end.  */
	g_object_set (labels, "format", "%", NULL);
	gog_object_request_update (labels);
	gog_object_update (labels);
	elt = gog_series_labels_vector_get_element (
		GOG_SERIES_LABELS (labels), 0);
	ok = ok && elt != NULL && elt->str != NULL &&
	     elt->str->str[0] == '\0';

	check (ok, "out-of-range and trailing '%' label formats are handled "
	       "(gog-series-labels.c:875)");
	g_object_unref (graph);
}

/* A minimal host document for feeding theme XML through the theme's
 * GOPersist interface, i.e. the re-parse path.  */
static void
theme_reparse_start (GsfXMLIn *xin, xmlChar const **attrs)
{
	go_persist_prep_sax (GO_PERSIST (xin->user_state), xin, attrs);
}

static GsfXMLInNode const theme_reparse_dtd[] = {
	GSF_XML_IN_NODE (THEME_REPARSE, THEME_REPARSE, -1, "GogTheme",
			 GSF_XML_NO_CONTENT, theme_reparse_start, NULL),
	GSF_XML_IN_NODE_END
};

static void
theme_reparse_parse (GogTheme *theme, char const *xml)
{
	GsfInput *input = gsf_input_memory_new ((guint8 const *) xml,
						(gsf_off_t) strlen (xml), FALSE);
	GsfXMLInDoc *doc = gsf_xml_in_doc_new (theme_reparse_dtd, NULL);
	gboolean ok;

	ok = gsf_xml_in_doc_parse (doc, input, theme);
	check (ok, "the theme XML parses");
	gsf_xml_in_doc_free (doc);
	g_object_unref (input);
}

static unsigned
theme_reparse_palette_size (GogTheme *theme)
{
	GsfOutput *output = gsf_output_memory_new ();
	GsfXMLOut *xml_out = gsf_xml_out_new (output);
	char const *bytes;
	gsize len, off = 0;
	unsigned count = 0;

	gsf_xml_out_start_element (xml_out, "root");
	go_persist_sax_save (GO_PERSIST (theme), xml_out);
	gsf_xml_out_end_element (xml_out);
	g_object_unref (xml_out);
	gsf_output_close (output);

	bytes = (char const *) gsf_output_memory_get_bytes (
		GSF_OUTPUT_MEMORY (output));
	len = gsf_output_size (output);
	while (off < len) {
		char const *found = g_strstr_len (bytes + off, len - off,
						  "class=\"GogSeries\"");
		if (found == NULL)
			break;
		count++;
		off = found - bytes + 1;
	}
	g_object_unref (output);
	return count;
}

/* gog-theme.c:703: re-parsing a theme replaces its palette instead of
 * appending to it.  */
static void
test_theme_palette_reparse (void)
{
	static char const * const xml_two =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<GogTheme>\n"
		"  <GOStyle class=\"GogSeries\"/>\n"
		"  <GOStyle class=\"GogSeries\"/>\n"
		"</GogTheme>\n";
	static char const * const xml_one =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<GogTheme>\n"
		"  <GOStyle class=\"GogSeries\"/>\n"
		"</GogTheme>\n";
	GogTheme *theme = gog_theme_registry_lookup ("palette-reparse-test");

	theme_reparse_parse (theme, xml_two);
	check (theme_reparse_palette_size (theme) == 2,
	       "the parsed palette has two entries");

	theme_reparse_parse (theme, xml_one);
	check (theme_reparse_palette_size (theme) == 1,
	       "a re-parse replaces the palette (gog-theme.c:703)");
}

/* gog-theme.c:1411: the Default theme's data-label style must keep its
 * line and font values when serialized.  */
static void
test_regression_theme_data_label_style (void)
{
	GogGraph *graph = new_graph (200., 100.);
	GogTheme *theme = gog_graph_get_theme (graph);
	GsfOutput *output;
	char const *bytes;
	gsize len;
	char const *elem, *end;
	gboolean ok = FALSE;

	CHECK (theme != NULL);
	output = gsf_output_memory_new ();
	{
		GsfXMLOut *xml = gsf_xml_out_new (output);
		go_persist_sax_save (GO_PERSIST (theme), xml);
		g_object_unref (xml);
	}
	gsf_output_close (output);
	bytes = (char const *) gsf_output_memory_get_bytes (
		GSF_OUTPUT_MEMORY (output));
	len = gsf_output_size (output);

	elem = g_strstr_len (bytes, len, "class=\"GogDataLabel\"");
	if (elem != NULL) {
		end = g_strstr_len (elem, bytes + len - elem, "</GOStyle>");
		if (end != NULL) {
			gsize elem_len = end - elem;
			ok = g_strstr_len (elem, elem_len,
					   "font=\"Sans 6\"") != NULL &&
			     g_strstr_len (elem, elem_len,
					   "dash=\"none\"") != NULL;
		}
	}
	check (ok, "Default theme data-label style survives serialization "
	       "(gog-theme.c:1411)");

	g_object_unref (output);
	g_object_unref (graph);
}

/* gog-grid-line.c:762: a grid line attached to an axis line without
 * custom ticks must fall back to the axis ticks instead of vanishing.  */
static void
test_regression_grid_line_axis_ticks (void)
{
	GogGraph *graph = new_graph (100., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY Lines");
	GogAxis *axis;
	GogObject *axis_line, *grid;
	GOStyle *style;
	cairo_surface_t *surface;
	unsigned x0, y0, x1, y1;
	gboolean ok = FALSE;

	if (plot == NULL) {
		skip ("axis-line grid regression (no XY plot)");
		g_object_unref (graph);
		return;
	}
	plot_add_xy_series (plot, (double const[]) { 0., 1., 2., 3., 4. },
			    (double const[]) { 1., 3., 2., 5., 4. }, 5);
	axis = gog_plot_get_axis (plot, GOG_AXIS_X);
	axis_line = gog_object_add_by_name2 (GOG_OBJECT (axis), "AxisLine", NULL);
	grid = axis_line != NULL
		? gog_object_add_by_name2 (axis_line, "MajorGrid", NULL) : NULL;
	if (grid == NULL) {
		skip ("axis-line grid regression (no grid line)");
		g_object_unref (graph);
		return;
	}
	style = go_style_new ();
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 3.;
	style->line.color = GO_COLOR_RED;
	style->line.auto_dash = FALSE;
	style->line.auto_color = FALSE;
	style->line.auto_width = FALSE;
	go_styled_object_set_style (GO_STYLED_OBJECT (grid), style);
	g_object_unref (style);
	gog_object_request_update (GOG_OBJECT (plot));
	gog_object_update (GOG_OBJECT (plot));
	gog_object_request_update (GOG_OBJECT (axis));
	gog_object_update (GOG_OBJECT (axis));
	gog_object_request_update (grid);
	gog_object_update (grid);

	surface = render_graph (graph, 400., 400., &ok);
	CHECK (ok);
	check (color_bbox (surface, GO_COLOR_RED, &x0, &y0, &x1, &y1),
	       "grid line on an axis line without custom ticks is drawn "
	       "(gog-grid-line.c:762)");
	cairo_surface_destroy (surface);
	g_object_unref (graph);
}

/* gog-color-scale.c:563: a colour scale whose axis is gone must not
 * render with uninitialized bounds.  */
static void
test_regression_color_scale_null_axis (void)
{
	GogGraph *graph = new_graph (100., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY Lines");
	GogObject *scale;
	cairo_surface_t *surface;
	gboolean ok = FALSE;

	if (plot == NULL) {
		skip ("colour-scale null-axis regression (no XY plot)");
		g_object_unref (graph);
		return;
	}
	plot_add_xy_series (plot, (double const[]) { 0., 1., 2. },
			    (double const[]) { 1., 0., 1. }, 3);
	scale = gog_object_add_by_name2 (GOG_OBJECT (chart),
					"Color-Scale", NULL);
	if (scale == NULL) {
		skip ("colour-scale null-axis regression (no colour scale)");
		g_object_unref (graph);
		return;
	}
	/* No colour axis exists in an XY chart.  */
	g_object_set (scale, "axis", "", NULL);
	{
		char *axis_str = NULL;
		g_object_get (scale, "axis", &axis_str, NULL);
		check (g_strcmp0 (axis_str, "") == 0,
		       "colour-scale axis property is empty without an axis "
		       "(gog-color-scale.c:295)");
		g_free (axis_str);
	}
	surface = render_graph (graph, 400., 400., &ok);
	cairo_surface_destroy (surface);
	check (ok, "colour scale without an axis renders "
	       "(gog-color-scale.c:563)");
	g_object_unref (graph);
}

/* gog-color-scale.c:444/568/752/790: discrete axes have major ticks
 * without strings; they must not be dereferenced.  */
static void
test_regression_color_scale_discrete_axis (void)
{
	GogGraph *graph = new_graph (100., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogAxis *axis;
	GogObject *scale;
	cairo_surface_t *surface;
	gboolean ok = FALSE;

	/* A pseudo-3D axis is discrete, so its major ticks have no label
	 * strings.  */
	gog_chart_axis_set_assign (chart, GOG_AXIS_SET_XY_pseudo_3d);
	axis = gog_chart_get_axis (chart, GOG_AXIS_PSEUDO_3D);
	if (axis == NULL) {
		skip ("colour-scale discrete-axis regression (no 3D axis)");
		g_object_unref (graph);
		return;
	}
	gog_axis_set_bounds (axis, 1., 4.);
	gog_object_request_update (GOG_OBJECT (axis));
	gog_object_update (GOG_OBJECT (axis));
	scale = gog_object_add_by_name2 (GOG_OBJECT (chart),
					"Color-Scale", NULL);
	if (scale == NULL) {
		skip ("colour-scale discrete-axis regression (no colour scale)");
		g_object_unref (graph);
		return;
	}
	gog_color_scale_set_axis (GOG_COLOR_SCALE (scale), axis);
	surface = render_graph (graph, 400., 400., &ok);
	cairo_surface_destroy (surface);
	check (ok, "colour scale on a discrete axis renders "
	       "(gog-color-scale.c:444)");
	g_object_unref (graph);
}

/* gog-grid-line.c:215/252: with an odd tick count the closing stripe
 * must follow the data maximum, which is on the other edge for an
 * inverted axis.  */
static void
test_regression_grid_stripe_inverted (void)
{
	GogGraph *graph = new_graph (100., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY Lines");
	GogAxis *axis;
	GogObject *axis_line, *grid;
	GOStyle *style;
	cairo_surface_t *surface;
	unsigned nx0, ny0, nx1, ny1, ix0, iy0, ix1, iy1;
	gboolean ok = FALSE;

	if (plot == NULL) {
		skip ("inverted stripe regression (no XY plot)");
		g_object_unref (graph);
		return;
	}
	plot_add_xy_series (plot, (double const[]) { 0., 1., 2., 3., 4. },
			    (double const[]) { 1., 3., 2., 5., 4. }, 5);
	axis = gog_plot_get_axis (plot, GOG_AXIS_X);
	gog_axis_set_bounds (axis, 0., 9.);
	gog_dataset_set_dim (GOG_DATASET (axis), GOG_AXIS_ELEM_MAJOR_TICK,
			     go_data_scalar_val_new (2.), NULL);
	axis_line = gog_object_add_by_name2 (GOG_OBJECT (axis), "AxisLine", NULL);
	grid = axis_line != NULL
		? gog_object_add_by_name2 (axis_line, "MajorGrid", NULL) : NULL;
	if (grid == NULL) {
		skip ("inverted stripe regression (no grid line)");
		g_object_unref (graph);
		return;
	}
	style = go_style_new ();
	style->fill.type = GO_STYLE_FILL_PATTERN;
	style->fill.auto_type = FALSE;
	style->fill.auto_fore = FALSE;
	style->fill.auto_back = FALSE;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_RED);
	style->line.dash_type = GO_LINE_NONE;
	style->line.auto_dash = FALSE;
	style->line.width = 0.;
	style->line.auto_width = FALSE;
	go_styled_object_set_style (GO_STYLED_OBJECT (grid), style);
	g_object_unref (style);
	gog_object_request_update (GOG_OBJECT (plot));
	gog_object_update (GOG_OBJECT (plot));
	gog_object_request_update (GOG_OBJECT (axis));
	gog_object_update (GOG_OBJECT (axis));
	gog_object_request_update (grid);
	gog_object_update (grid);

	surface = render_graph (graph, 400., 400., &ok);
	CHECK (ok);
	CHECK (color_bbox (surface, GO_COLOR_RED, &nx0, &ny0, &nx1, &ny1));
	cairo_surface_destroy (surface);

	g_object_set (axis, "invert-axis", TRUE, NULL);
	gog_object_request_update (GOG_OBJECT (axis));
	gog_object_update (GOG_OBJECT (axis));
	surface = render_graph (graph, 400., 400., &ok);
	CHECK (ok);
	CHECK (color_bbox (surface, GO_COLOR_RED, &ix0, &iy0, &ix1, &iy1));
	cairo_surface_destroy (surface);

	check (abs ((int) ix0 - (int) nx0) <= 2,
	       "inverted closing stripe follows the data maximum "
	       "(gog-grid-line.c:215)");

	g_object_unref (graph);
}

/* gog-object.c:294: a NULL "position" string must be ignored.  */
static void
test_regression_object_position_null (void)
{
	GogGraph *graph = new_graph (100., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogObjectPosition before;

	CHECK (chart != NULL);
	before = gog_object_get_position_flags (GOG_OBJECT (chart),
						GOG_POSITION_MANUAL);
	g_object_set (chart, "position", NULL, NULL);
	check (gog_object_get_position_flags (GOG_OBJECT (chart),
					      GOG_POSITION_MANUAL) == before,
	       "a NULL position string is ignored (gog-object.c:294)");
	g_object_unref (graph);
}

/* gog-renderer.c:1844: an unknown image format has no format info; it
 * must be rejected instead of dereferenced.  */
static void
test_regression_export_unknown_format (void)
{
	GogGraph *graph = new_graph (100., 100.);
	GogRenderer *renderer = gog_renderer_new (graph);
	GsfOutput *output = gsf_output_memory_new ();
	guint handler;
	gboolean result;

	handler = g_log_set_handler (NULL, G_LOG_LEVEL_WARNING,
				     quiet_log, NULL);
	result = gog_renderer_export_image (renderer,
					    GO_IMAGE_FORMAT_UNKNOWN,
					    output, 96., 96.);
	g_log_remove_handler (NULL, handler);
	check (!result, "an unknown image format is rejected "
	       "(gog-renderer.c:1844)");
	g_object_unref (output);
	g_object_unref (renderer);
	g_object_unref (graph);
}

/* gog-series-labels.c:1669: the boxed copy/free of a label element must
 * cope with a NULL point.  */
static void
test_regression_series_label_elt_null_point (void)
{
	GogSeriesLabelElt elt;
	GogSeriesLabelElt *copy;

	memset (&elt, 0, sizeof (elt));
	elt.str = go_string_new ("x");
	copy = g_boxed_copy (gog_series_label_elt_get_type (), &elt);
	CHECK (copy != NULL);
	check (copy != NULL && copy->point == NULL,
	       "label element boxed copy handles a NULL point "
	       "(gog-series-labels.c:1669)");
	g_boxed_free (gog_series_label_elt_get_type (), copy);
	go_string_unref (elt.str);
}

/* gog-axis.c:4138: ABSOLUTE metrics is a legitimate state; get_ref_axis
 * must return NULL instead of criticaling.  */
static void
test_regression_axis_ref_axis_absolute (void)
{
	GogAxis *axis = GOG_AXIS (g_object_new (GOG_TYPE_AXIS,
						"type", GOG_AXIS_X, NULL));

	CHECK (axis != NULL);
	g_object_set (axis, "metrics", "absolute", NULL);
	check (gog_axis_get_metrics (axis) == GOG_AXIS_METRICS_ABSOLUTE,
	       "absolute metrics can be set");
	check (gog_axis_get_ref_axis (axis) == NULL,
	       "an absolute-metrics axis has no reference axis "
	       "(gog-axis.c:4138)");
	g_object_unref (axis);
}

/* gog-theme.c:1784: deleting a theme without a URI must fail cleanly
 * instead of building a GFile from NULL.  */
static void
test_regression_theme_delete_no_uri (void)
{
	GogTheme *theme = gog_theme_registry_lookup ("Default");

	if (theme == NULL ||
	    gog_theme_get_resource_type (theme) != GO_RESOURCE_NATIVE) {
		skip ("theme delete without URI (no native Default theme)");
		return;
	}
	check (!gog_theme_delete (theme),
	       "deleting a theme without a URI fails cleanly "
	       "(gog-theme.c:1784)");
}

/* gog_object_add_by_name2()/add_by_role2() have strict transfer-full
 * semantics: the child is consumed on success and on every failure.  */
static void
test_add2_ownership (void)
{
	GogGraph *graph = new_graph (100., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogLabel *kept;
	GogLabel *consumed;

	/* Success: the parent holds the reference.  */
	kept = GOG_LABEL (g_object_new (GOG_TYPE_LABEL, NULL));
	g_object_add_weak_pointer (G_OBJECT (kept), (gpointer *) &kept);
	CHECK (gog_object_add_by_name2 (GOG_OBJECT (chart), "Title",
					GOG_OBJECT (kept)) != NULL);
	check (kept != NULL,
	       "add_by_name2 keeps the child alive on success");
	if (kept != NULL)
		g_object_remove_weak_pointer (G_OBJECT (kept),
					      (gpointer *) &kept);

	/* Unknown role: the reference is consumed.  */
	consumed = GOG_LABEL (g_object_new (GOG_TYPE_LABEL, NULL));
	g_object_add_weak_pointer (G_OBJECT (consumed), (gpointer *) &consumed);
	check (gog_object_add_by_name2 (GOG_OBJECT (chart), "Bogus",
					GOG_OBJECT (consumed)) == NULL &&
	       consumed == NULL,
	       "add_by_name2 consumes the child for an unknown role");

	g_object_unref (graph);
}

/* gog-object-xml.c:511: an element whose type cannot be attached to its
 * role must not leak the created object (caught under valgrind).  */
static void
test_regression_xml_unattachable (void)
{
	static char const * const xml =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<GogObject type=\"GogGraph\">\n"
		"  <GogObject role=\"Chart\">\n"
		"    <GogObject type=\"GogLabel\" role=\"Bogus\"/>\n"
		"  </GogObject>\n"
		"</GogObject>\n";
	GsfInput *input = gsf_input_memory_new ((guint8 const *) xml,
						(gsf_off_t) strlen (xml), FALSE);
	GogObject *loaded = gog_object_new_from_input (input, NULL);

	CHECK (loaded != NULL);
	check (loaded != NULL &&
	       gog_object_get_child_by_name (loaded, "Chart") != NULL,
	       "a document with an unattachable element still loads "
	       "(gog-object-xml.c:511)");
	if (loaded != NULL)
		g_object_unref (loaded);
	g_object_unref (input);
}

/* gog-object.c:1507: a rejected "move up" must be a quiet no-op that
 * leaves the child list untouched.  */
static void
test_regression_reorder_head (void)
{
	GogGraph *graph = new_graph (100., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogObject *axis1, *axis2, *axis3, *head, *second, *after;
	GSList *before, *p, *q;
	gboolean unchanged;

	axis1 = gog_object_add_by_name2 (GOG_OBJECT (chart), "X-Axis", NULL);
	axis2 = gog_object_add_by_name2 (GOG_OBJECT (chart), "X-Axis", NULL);
	axis3 = gog_object_add_by_name2 (GOG_OBJECT (chart), "X-Axis", NULL);
	if (axis1 == NULL || axis2 == NULL || axis3 == NULL) {
		skip ("reorder regression (cannot add axes)");
		g_object_unref (graph);
		return;
	}

	before = g_slist_copy (GOG_OBJECT (chart)->children);
	head = before->data;
	check (gog_object_reorder (head, TRUE, FALSE) == NULL,
	       "a rejected move up returns NULL (gog-object.c:1507)");
	unchanged = TRUE;
	for (p = before, q = GOG_OBJECT (chart)->children;
	     p != NULL && q != NULL; p = p->next, q = q->next)
		if (p->data != q->data) {
			unchanged = FALSE;
			break;
		}
	if (p != NULL || q != NULL)
		unchanged = FALSE;
	check (unchanged,
	       "a rejected move up leaves the child list untouched "
	       "(gog-object.c:1507)");
	g_slist_free (before);

	/* The normal one-position move still works.  */
	second = GOG_OBJECT (chart)->children->next->data;
	after = gog_object_reorder (second, TRUE, FALSE);
	check (after == NULL && GOG_OBJECT (chart)->children->data == second,
	       "moving up one position still works");

	g_object_unref (graph);
}

static void
count_enum_cb (G_GNUC_UNUSED unsigned i, G_GNUC_UNUSED gpointer style,
	       G_GNUC_UNUSED char const *label,
	       G_GNUC_UNUSED PangoAttrList *markup, gpointer data)
{
	(*(unsigned *) data)++;
}

/* gog-plot.c:773: with only_visible == FALSE, legend-hidden trend lines
 * must be enumerated too.  */
static void
test_regression_plot_foreach_legend (void)
{
	GogGraph *graph = new_graph (200., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY");
	GogSeries *series;
	GogTrendLine *tl;
	char const *engine = first_trend_line_engine_name ();
	unsigned all = 0, visible = 0;

	if (plot == NULL || engine == NULL) {
		skip ("plot foreach legend regression (no plot or engine)");
		g_object_unref (graph);
		return;
	}
	series = plot_add_xy_series (plot, (double const[]) { 0., 1., 2. },
				     (double const[]) { 1., 0., 1. }, 3);
	tl = gog_trend_line_new_by_name (engine);
	if (tl == NULL) {
		skip ("plot foreach legend regression (no trend line)");
		g_object_unref (graph);
		return;
	}
	if (gog_object_add_by_name2 (GOG_OBJECT (series), "Trend line",
				     GOG_OBJECT (tl)) == NULL) {
		skip ("plot foreach legend regression (trend line rejected)");
		g_object_unref (graph);
		return;
	}
	g_object_set (tl, "has-legend", FALSE, NULL);
	g_object_set (series, "has-legend", TRUE, NULL);
	gog_object_request_update (GOG_OBJECT (plot));
	gog_object_update (GOG_OBJECT (plot));
	gog_plot_update_cardinality (plot, 0);

	gog_plot_foreach_elem (plot, FALSE, count_enum_cb, &all);
	gog_plot_foreach_elem (plot, TRUE, count_enum_cb, &visible);
	check (all >= 2 && all > visible,
	       "legend-hidden trend lines are enumerated when "
	       "only_visible is FALSE (gog-plot.c:773)");

	g_object_unref (graph);
}

/* gog-chart-map.c:581: an ODF spline with no points must not touch the
 * data arrays (caught under valgrind when the pointers are one past a
 * heap block) and must yield an empty path.  */
static void
test_regression_make_path_empty (void)
{
	GogGraph *graph = new_graph (100., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY Lines");
	GogAxis *x_axis, *y_axis;
	GogViewAllocation area;
	GogChartMap *map;
	GOPath *path;
	PathSummary s;
	double *xs, *ys;

	if (plot == NULL) {
		skip ("empty ODF-spline path regression (no XY plot)");
		g_object_unref (graph);
		return;
	}
	x_axis = gog_plot_get_axis (plot, GOG_AXIS_X);
	y_axis = gog_plot_get_axis (plot, GOG_AXIS_Y);
	gog_axis_set_bounds (x_axis, 0., 10.);
	gog_axis_set_bounds (y_axis, 0., 10.);
	area.x = area.y = 0.;
	area.w = area.h = 100.;
	map = gog_chart_map_new (chart, &area, x_axis, y_axis, NULL, FALSE);
	CHECK (map != NULL && gog_chart_map_is_valid (map));
	if (map == NULL) {
		g_object_unref (graph);
		return;
	}

	/* With zero points the arrays must not be dereferenced: use
	 * pointers one past a heap block so that a violation shows up
	 * under valgrind instead of reading addressable stack bytes.  */
	xs = g_new (double, 1);
	ys = g_new (double, 1);
	xs[0] = ys[0] = 1.;
	path = gog_chart_map_make_path (map, xs + 1, ys + 1, 0,
					GO_LINE_INTERPOLATION_ODF_SPLINE,
					FALSE, NULL);
	CHECK (path != NULL);
	s = path_summary (path);
	check (s.n_segments == 0,
	       "an empty ODF spline yields an empty path "
	       "(gog-chart-map.c:581)");
	go_path_free (path);
	g_free (xs);
	g_free (ys);

	gog_chart_map_free (map);
	g_object_unref (graph);
}

/* gog-axis.c:902: a tiny major tick combined with a huge display factor
 * underflows the step to zero, which used to make the tick adjustment
 * loop spin forever.  (The property range prevents zero or non-finite
 * factors, so this is the reachable spelling of the bug.)  */
static void
test_regression_display_factor_underflow (void)
{
	GogGraph *graph = new_graph (200., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY Lines");
	GogSeries *series;
	GogAxis *axis;
	GogAxisTick *ticks;
	unsigned n;

	if (plot == NULL) {
		skip ("display factor underflow regression (no XY plot)");
		g_object_unref (graph);
		return;
	}
	series = plot_add_xy_series (plot, (double const[]) { 1., 2. },
				     (double const[]) { 1., 2. }, 2);
	axis = gog_plot_get_axis (plot, GOG_AXIS_X);
	gog_axis_set_bounds (axis, 0., 1e290);
	gog_dataset_set_dim (GOG_DATASET (axis), GOG_AXIS_ELEM_MAJOR_TICK,
			     go_data_scalar_val_new (1e-30), NULL);
	g_object_set (axis, "display-factor", 1e308, NULL);
	gog_object_request_update (GOG_OBJECT (series));
	gog_object_update (GOG_OBJECT (series));
	gog_object_request_update (GOG_OBJECT (plot));
	gog_object_update (GOG_OBJECT (plot));
	gog_object_request_update (GOG_OBJECT (axis));
	gog_object_update (GOG_OBJECT (axis));

	n = gog_axis_get_ticks (axis, &ticks);
	check (n > 0 && n < 250,
	       "an underflowing display factor still gives bounded ticks "
	       "(gog-axis.c:902)");

	g_object_unref (graph);
}

/* gog-object-xml.c:367: a property element without a name must be
 * ignored, not applied to the previous property.  (The anchor lives on
 * a roled child: position flags are refused for the role-less graph
 * root.)  */
static void
test_regression_xml_property_no_name (void)
{
	static char const * const xml =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<GogObject type=\"GogGraph\">\n"
		"  <GogObject role=\"Chart\">\n"
		"    <GogObject role=\"Title\">\n"
		"      <property name=\"anchor\">center</property>\n"
		"      <property>top-left</property>\n"
		"    </GogObject>\n"
		"  </GogObject>\n"
		"</GogObject>\n";
	GsfInput *input = gsf_input_memory_new ((guint8 const *) xml,
						(gsf_off_t) strlen (xml), FALSE);
	GogObject *loaded, *chart, *title;
	char *anchor = NULL;
	guint handler;

	handler = g_log_set_handler (NULL, G_LOG_LEVEL_WARNING, quiet_log, NULL);
	loaded = gog_object_new_from_input (input, NULL);
	g_log_remove_handler (NULL, handler);
	g_object_unref (input);

	CHECK (loaded != NULL);
	chart = loaded != NULL ?
		gog_object_get_child_by_name (loaded, "Chart") : NULL;
	title = chart != NULL ?
		gog_object_get_child_by_name (chart, "Title") : NULL;
	CHECK (title != NULL);
	if (title != NULL) {
		g_object_get (title, "anchor", &anchor, NULL);
		check (g_strcmp0 (anchor, "center") == 0,
		       "a nameless property is ignored "
		       "(gog-object-xml.c:367)");
		g_free (anchor);
	}
	if (loaded != NULL)
		g_object_unref (loaded);
}

/* gog-axis-line.c:2469: the label position out-param must be written even
 * when the function bails out early.  */
static void
test_view_label_position_zeroed (void)
{
	GogGraph *graph = new_graph (100., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogAxis *axis;
	GogView *view;
	GogViewAllocation bbox = { 0., 0., 100., 100. };
	GogViewAllocation pos = { 1., 2., 3., 4. };

	gog_chart_axis_set_assign (chart, GOG_AXIS_SET_XY_pseudo_3d);
	axis = gog_chart_get_axis (chart, GOG_AXIS_PSEUDO_3D);
	if (axis == NULL) {
		skip ("label position regression (no pseudo-3D axis)");
		g_object_unref (graph);
		return;
	}
	view = gog_object_new_view (GOG_OBJECT (axis), NULL);
	if (view == NULL) {
		skip ("label position regression (no view)");
		g_object_unref (graph);
		return;
	}
	gog_axis_base_view_label_position_request (view, &bbox, &pos);
	check (pos.x == 0. && pos.y == 0. && pos.w == 0. && pos.h == 0.,
	       "the label position out-param is cleared on early return "
	       "(gog-axis-line.c:2469)");
	g_object_unref (view);
	g_object_unref (graph);
}

/* gog-axis-line.c:2662: custom tick positions without a labels vector
 * must be formatted with the axis line's format, not shown raw.  */
static void
test_axis_line_position_labels_format (void)
{
	GogGraph *graph = new_graph (200., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY Lines");
	GogAxis *axis;
	GogObject *line;
	GogAxisTick *ticks;
	unsigned n;

	if (plot == NULL) {
		skip ("axis-line tick format (no XY plot)");
		g_object_unref (graph);
		return;
	}

	axis = gog_plot_get_axis (plot, GOG_AXIS_X);
	line = gog_object_add_by_name2 (GOG_OBJECT (axis), "AxisLine", NULL);
	CHECK (line != NULL);
	if (line == NULL) {
		g_object_unref (graph);
		return;
	}

	g_object_set (line, "assigned-format-string-XL", "0.00", NULL);
	gog_dataset_set_dim (GOG_DATASET (line),
			     GOG_AXIS_ELEM_CROSS_POINT + 1,
			     go_data_vector_val_new ((double[]) { 1.5, 2.5,
								  3.5 },
						     3, NULL),
			     NULL);

	n = gog_axis_line_get_ticks (GOG_AXIS_LINE (line), &ticks);
	CHECK (n == 3);
	if (n == 3)
		check (ticks[0].str != NULL &&
		       strcmp (ticks[0].str->str, "1.50") == 0,
		       "tick labels from positions use the axis format "
		       "(gog-axis-line.c:2662)");

	g_object_unref (graph);
}

/* gog-graph.c:1371: after the view tree is rebuilt the selection must
 * be re-resolvable by asking for the same object again.  */
static void
test_graph_view_selection_restore (void)
{
	GogGraph *graph = new_graph (200., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogRenderer *renderer;
	GogGraphView *gview;
	GogView *view;

	CHECK (chart != NULL);
	renderer = gog_renderer_new (graph);
	gview = GOG_GRAPH_VIEW (g_object_new (GOG_TYPE_GRAPH_VIEW,
					      "renderer", renderer,
					      "model", graph, NULL));

	view = gog_view_find_child_view (GOG_VIEW (gview), GOG_OBJECT (chart));
	CHECK (view != NULL);
	gog_graph_view_set_selection (gview, GOG_OBJECT (chart));
	check (gog_graph_view_get_selection (gview) == view,
	       "the selected view is resolved");

	/* Simulate a rebuilt view tree: destroy the chart view so the
	 * selection's weak reference clears, then build a fresh one.  */
	GOG_VIEW (gview)->children =
		g_slist_remove (GOG_VIEW (gview)->children, view);
	g_object_unref (view);
	check (gog_graph_view_get_selection (gview) == NULL,
	       "the selected view is cleared when the view goes away");

	view = gog_object_new_view (GOG_OBJECT (chart), GOG_VIEW (gview));
	CHECK (view != NULL);
	g_object_unref (view);

	gog_graph_view_set_selection (gview, GOG_OBJECT (chart));
	check (gog_graph_view_get_selection (gview) == view &&
	       view == gog_view_find_child_view (GOG_VIEW (gview),
						 GOG_OBJECT (chart)),
	       "the selection is restored after the view was rebuilt "
	       "(gog-graph.c:1371)");

	g_object_unref (gview);
	g_object_unref (renderer);
	g_object_unref (graph);
}

/* gog-view.c:233: the first drag of a not-yet-manual object must measure
 * against the parent view's origin, or the object jumps.  */
static void
test_tool_move_object_offset (void)
{
	GogGraph *graph = new_graph (400., 200.);
	GogChart *chart = graph_add_chart (graph);
	GogRenderer *renderer;
	GogView *gview, *view;
	GSList const *ptr;
	GogTool *move_tool = NULL;
	GogToolAction *action;
	GogViewAllocation parent = { 100., 50., 400., 200. };
	GogViewAllocation child = { 150., 90., 300., 120. };
	GogViewAllocation pos;

	CHECK (chart != NULL);
	renderer = gog_renderer_new (graph);
	gview = g_object_new (GOG_TYPE_GRAPH_VIEW,
			      "renderer", renderer, "model", graph, NULL);
	view = gog_view_find_child_view (gview, GOG_OBJECT (chart));
	CHECK (view != NULL);
	if (view == NULL) {
		g_object_unref (gview);
		g_object_unref (renderer);
		g_object_unref (graph);
		return;
	}

	for (ptr = gog_view_get_toolkit (view); ptr != NULL; ptr = ptr->next) {
		GogTool *tool = ptr->data;
		if (g_strcmp0 (tool->name, "Move") == 0)
			move_tool = tool;
	}
	if (move_tool == NULL) {
		skip ("move tool not available");
		g_object_unref (gview);
		g_object_unref (renderer);
		g_object_unref (graph);
		return;
	}

	gview->allocation = parent;
	view->allocation = child;

	action = gog_tool_action_new (view, move_tool, 0., 0.);
	gog_tool_action_move (action, 0., 0.);
	gog_object_get_manual_position (GOG_OBJECT (chart), &pos);
	check (fabs (parent.x + pos.x * parent.w - child.x) < 1e-6 &&
	       fabs (parent.y + pos.y * parent.h - child.y) < 1e-6,
	       "the first drag does not jump by the parent origin "
	       "(gog-view.c:233)");
	gog_tool_action_free (action);

	g_object_unref (gview);
	g_object_unref (renderer);
	g_object_unref (graph);
}

#ifdef GOFFICE_WITH_GTK
/* Display-only tests.  They build Gtk dialogs and widgets, pump the
 * pending events and tear them down again; no events are synthesized.  */
static void
test_display_guru (void)
{
	GClosure *closure = g_closure_new_simple (sizeof (GClosure), NULL);
	GtkWidget *guru = gog_guru (NULL, NULL, NULL, closure);

	check (guru != NULL, "the graph guru dialog is built");
	if (guru != NULL) {
		while (g_main_context_pending (NULL))
			g_main_context_iteration (NULL, FALSE);
		gtk_widget_destroy (guru);
	}
	g_closure_unref (closure);
}

static void
test_display_graph_widget (void)
{
	GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	GtkWidget *widget;
	GogChart *chart;

	gtk_window_resize (GTK_WINDOW (window), 300, 200);
	widget = go_graph_widget_new (NULL);
	gtk_container_add (GTK_CONTAINER (window), widget);
	chart = go_graph_widget_get_chart (GO_GRAPH_WIDGET (widget));
	CHECK (chart != NULL);
	if (chart != NULL)
		(void) chart_add_plot (chart, "XY", "XY Lines");
	gtk_widget_show_all (window);
	while (g_main_context_pending (NULL))
		g_main_context_iteration (NULL, FALSE);
	check (gtk_widget_get_realized (widget),
	       "the graph widget is realized and rendered");
	gtk_widget_destroy (window);
}

/* ------------------------------------------------------------------------- */
/* Editor harness: a minimal GogDataAllocator handing out spin buttons that
 * implement GogDataEditor.  Editors are built and torn down without
 * synthesizing any events.  */

static GtkWidget *
find_widget_named (GtkWidget *widget, char const *name)
{
	GtkWidget *found = NULL;
	GList *children, *ptr;

	if (g_strcmp0 (gtk_buildable_get_name (GTK_BUILDABLE (widget)),
		       name) == 0)
		return widget;
	if (!GTK_IS_CONTAINER (widget))
		return NULL;
	children = gtk_container_get_children (GTK_CONTAINER (widget));
	for (ptr = children; ptr != NULL && found == NULL; ptr = ptr->next)
		found = find_widget_named (GTK_WIDGET (ptr->data), name);
	g_list_free (children);
	return found;
}

typedef struct {
	GtkSpinButton		 parent;
} TestDataEditor;

typedef struct {
	GtkSpinButtonClass	 parent;
} TestDataEditorClass;

GType test_data_editor_get_type (void);
GType test_allocator_get_type (void);

static void
test_data_editor_set_format (G_GNUC_UNUSED GogDataEditor *editor,
			     G_GNUC_UNUSED GOFormat const *fmt)
{
}

static void
test_data_editor_set_value_double (GogDataEditor *editor, double val,
				   G_GNUC_UNUSED GODateConventions const *date_conv)
{
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (editor), val);
}

static void
test_data_editor_iface_init (GogDataEditorClass *iface)
{
	iface->set_format = test_data_editor_set_format;
	iface->set_value_double = test_data_editor_set_value_double;
}

G_DEFINE_TYPE_WITH_CODE (TestDataEditor, test_data_editor,
			 GTK_TYPE_SPIN_BUTTON,
			 G_IMPLEMENT_INTERFACE (GOG_TYPE_DATA_EDITOR,
						test_data_editor_iface_init))

static void
test_data_editor_class_init (G_GNUC_UNUSED TestDataEditorClass *klass)
{
}

static void
test_data_editor_init (G_GNUC_UNUSED TestDataEditor *editor)
{
}

typedef struct {
	GObject			 parent;
} TestAllocator;

typedef struct {
	GObjectClass		 parent;
} TestAllocatorClass;

static GogDataEditor *
test_allocator_editor (G_GNUC_UNUSED GogDataAllocator *allocator,
		       G_GNUC_UNUSED GogDataset *set,
		       G_GNUC_UNUSED int dim_i,
		       G_GNUC_UNUSED GogDataType data_type)
{
	return GOG_DATA_EDITOR (g_object_new (test_data_editor_get_type (),
					      NULL));
}

static void
test_allocator_allocate (G_GNUC_UNUSED GogDataAllocator *allocator,
			 G_GNUC_UNUSED GogPlot *plot)
{
}

static void
test_allocator_iface_init (GogDataAllocatorClass *iface)
{
	iface->allocate = test_allocator_allocate;
	iface->editor = test_allocator_editor;
}

G_DEFINE_TYPE_WITH_CODE (TestAllocator, test_allocator, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GOG_TYPE_DATA_ALLOCATOR,
						test_allocator_iface_init))

static void
test_allocator_class_init (G_GNUC_UNUSED TestAllocatorClass *klass)
{
}

static void
test_allocator_init (G_GNUC_UNUSED TestAllocator *allocator)
{
}

/* gog-axis-line.c:2691: the axis-line editor must not gain pages when the
 * base class bails out for an absent or invisible axis.  */
static void
test_display_axis_line_editor_guard (void)
{
	GogGraph *graph = new_graph (200., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY Lines");
	GogAxis *axis;
	GogObject *line;
	GogDataAllocator *dalloc;
	GOEditor *editor;
	GtkWidget *notebook;
	gboolean found = FALSE;
	int i, n;

	if (plot == NULL) {
		skip ("axis-line editor guard (no XY plot)");
		g_object_unref (graph);
		return;
	}
	axis = gog_plot_get_axis (plot, GOG_AXIS_X);
	line = gog_object_add_by_name2 (GOG_OBJECT (axis), "AxisLine", NULL);
	CHECK (line != NULL);
	if (line == NULL) {
		g_object_unref (graph);
		return;
	}

	gog_object_set_invisible (GOG_OBJECT (axis), TRUE);

	dalloc = g_object_new (test_allocator_get_type (), NULL);
	editor = go_editor_new ();
	(GOG_OBJECT_GET_CLASS (line)->populate_editor) (line, editor, dalloc,
							NULL);
	notebook = go_editor_get_notebook (editor);
	n = gtk_notebook_get_n_pages (GTK_NOTEBOOK (notebook));
	for (i = 0; i < n; i++)
		if (find_widget_named
		    (gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), i),
		     "custom-ticks-grid") != NULL)
			found = TRUE;
	check (!found,
	       "no editor pages when the axis is invisible "
	       "(gog-axis-line.c:2691)");

	g_object_ref_sink (notebook);
	gtk_widget_destroy (notebook);
	g_object_unref (notebook);
	go_editor_free (editor);
	g_object_unref (dalloc);
	g_object_unref (graph);
}

/* gog-axis-line.c:624: the axis editor state must live as long as the
 * page, not the cross-axis combo that can be destroyed first.  */
static void
test_display_axis_editor_state_lifetime (void)
{
	GogGraph *graph = new_graph (200., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY Lines");
	GogAxis *axis;
	GogDataAllocator *dalloc;
	GOEditor *editor;
	GtkWidget *notebook, *window;
	GtkWidget *page = NULL, *combo = NULL, *cross = NULL, *radio = NULL;
	int i, n;

	if (plot == NULL) {
		skip ("axis editor state lifetime (no XY plot)");
		g_object_unref (graph);
		return;
	}
	axis = gog_plot_get_axis (plot, GOG_AXIS_X);

	dalloc = g_object_new (test_allocator_get_type (), NULL);
	editor = go_editor_new ();
	(GOG_OBJECT_GET_CLASS (axis)->populate_editor) (GOG_OBJECT (axis),
							editor, dalloc, NULL);

	notebook = go_editor_get_notebook (editor);
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_container_add (GTK_CONTAINER (window), notebook);

	n = gtk_notebook_get_n_pages (GTK_NOTEBOOK (notebook));
	for (i = 0; i < n && combo == NULL; i++) {
		page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), i);
		combo = find_widget_named (page, "cross-axis-combo");
	}
	CHECK (combo != NULL);
	if (combo != NULL) {
		GtkWidget *grid = find_widget_named (page, "cross-at-grid");
		GList *children, *ptr;

		CHECK (grid != NULL);
		children = grid != NULL ?
			gtk_container_get_children (GTK_CONTAINER (grid)) :
			NULL;
		for (ptr = children; ptr != NULL && cross == NULL; ptr = ptr->next)
			if (GOG_IS_DATA_EDITOR (ptr->data))
				cross = GTK_WIDGET (ptr->data);
		g_list_free (children);
		radio = find_widget_named (page, "axis-cross");
	}
	CHECK (cross != NULL);
	CHECK (radio != NULL);

	if (combo != NULL && cross != NULL && radio != NULL) {
		/* Destroy only the combo; the page and its other widgets live
		 * on, so the editor state must remain usable.  */
		gtk_widget_destroy (combo);
		g_signal_emit_by_name (cross, "changed");
		check (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radio)),
		       "the axis editor state outlives the combo "
		       "(gog-axis-line.c:624)");
	}

	gtk_widget_destroy (window);
	go_editor_free (editor);
	g_object_unref (dalloc);
	g_object_unref (graph);
}

/* go-editor.c:286: go_editor_get_page must return the page with the
 * requested name, not the first page with a different one.  */
static void
test_display_editor_get_page (void)
{
	GOEditor *editor = go_editor_new ();
	GtkWidget *page1 = g_object_ref_sink (gtk_label_new ("one"));
	GtkWidget *page2 = g_object_ref_sink (gtk_label_new ("two"));

	go_editor_add_page (editor, page1, "one");
	go_editor_add_page (editor, page2, "two");
	check (go_editor_get_page (editor, "one") == page1 &&
	       go_editor_get_page (editor, "two") == page2 &&
	       go_editor_get_page (editor, "three") == NULL,
	       "go_editor_get_page finds pages by name (go-editor.c:286)");
	go_editor_free (editor);
	g_object_unref (page1);
	g_object_unref (page2);
}

static void
count_critical (G_GNUC_UNUSED const gchar *log_domain,
		G_GNUC_UNUSED GLogLevelFlags log_level,
		G_GNUC_UNUSED const gchar *message, gpointer data)
{
	(*(unsigned *) data)++;
}

typedef struct {
	GtkTreeIter	 iters[4];
	GogObject	*objs[4];
	unsigned	 n;
} GuruSeriesScan;

static gboolean
cb_guru_collect_series (GtkTreeModel *model, G_GNUC_UNUSED GtkTreePath *path,
			GtkTreeIter *iter, GuruSeriesScan *scan)
{
	GogObject *obj;

	/* Column 1 is the guru's object pointer column.  */
	gtk_tree_model_get (model, iter, 1, &obj, -1);
	if (obj != NULL && GOG_IS_SERIES (obj) &&
	    scan->n < G_N_ELEMENTS (scan->iters)) {
		scan->iters[scan->n] = *iter;
		scan->objs[scan->n] = obj;
		scan->n++;
	}
	return FALSE;
}

static GtkWidget *
find_widget_type (GtkWidget *widget, GType type)
{
	GtkWidget *found = NULL;
	GList *children, *ptr;

	if (G_TYPE_CHECK_INSTANCE_TYPE (widget, type))
		return widget;
	if (!GTK_IS_CONTAINER (widget))
		return NULL;
	children = gtk_container_get_children (GTK_CONTAINER (widget));
	for (ptr = children; ptr != NULL && found == NULL; ptr = ptr->next)
		found = find_widget_type (GTK_WIDGET (ptr->data), type);
	g_list_free (children);
	return found;
}

static int
tree_row_index (GtkTreeModel *model, GtkTreeIter *iter)
{
	GtkTreePath *path = gtk_tree_model_get_path (model, iter);
	int depth = gtk_tree_path_get_depth (path);
	int index = gtk_tree_path_get_indices (path)[depth - 1];
	gtk_tree_path_free (path);
	return index;
}

/* gog-guru.c:724: a search that misses - the object is not in the tree
 * model - must not leave search_target behind; the next reorder would
 * fail its reentrancy guard and stop working silently.  */
static void
test_display_guru_reorder (void)
{
	GogGraph *graph = new_graph (200., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY Lines");
	GClosure *closure = g_closure_new_simple (sizeof (GClosure), NULL);
	GogDataAllocator *dalloc;
	GtkWidget *guru, *attr_window, *inc_button;
	GtkTreeView *view;
	GtkTreeModel *model;
	GuruSeriesScan scan;
	guint handler;
	unsigned criticals = 0;
	gboolean ok;

	if (plot == NULL) {
		skip ("guru reorder test (no XY plot)");
		g_closure_unref (closure);
		g_object_unref (graph);
		return;
	}

	(void) plot_add_xy_series (plot, (double const[]) { 1., 2. },
				   (double const[]) { 1., 2. }, 2);
	(void) plot_add_xy_series (plot, (double const[]) { 2., 3. },
				   (double const[]) { 3., 4. }, 2);
	(void) plot_add_xy_series (plot, (double const[]) { 3., 4. },
				   (double const[]) { 5., 6. }, 2);

	dalloc = g_object_new (test_allocator_get_type (), NULL);
	guru = gog_guru (graph, dalloc, NULL, closure);
	CHECK (guru != NULL);
	if (guru == NULL) {
		g_object_unref (dalloc);
		g_closure_unref (closure);
		g_object_unref (graph);
		return;
	}
	while (g_main_context_pending (NULL))
		g_main_context_iteration (NULL, FALSE);

	attr_window = find_widget_named (guru, "attr_window");
	view = attr_window != NULL ?
		GTK_TREE_VIEW (find_widget_type (attr_window,
						 GTK_TYPE_TREE_VIEW)) : NULL;
	CHECK (view != NULL);
	if (view == NULL) {
		gtk_widget_destroy (guru);
		g_object_unref (dalloc);
		g_closure_unref (closure);
		g_object_unref (graph);
		return;
	}
	model = gtk_tree_view_get_model (view);

	memset (&scan, 0, sizeof (scan));
	gtk_tree_model_foreach (model,
				(GtkTreeModelForeachFunc) cb_guru_collect_series,
				&scan);
	check (scan.n == 3, "the guru property tree lists all series");

	inc_button = find_widget_named (guru, "inc_precedence");
	CHECK (inc_button != NULL);

	if (scan.n == 3 && inc_button != NULL) {
		GtkTreeSelection *selection = gtk_tree_view_get_selection (view);

		/* Select the second series; "increase" is possible for it.  */
		gtk_tree_selection_select_iter (selection, &scan.iters[1]);
		check (gtk_widget_get_sensitive (inc_button),
		       "the precedence buttons are live for a movable series");

		/* Make the third series leave the tree, then emit a signal
		 * for it: the tree search for it now misses.  */
		/* Keep the series alive, then release the reference its
		 * parent used to hold.  */
		g_object_ref (scan.objs[2]);
		ok = gog_object_clear_parent (scan.objs[2]);
		g_object_unref (scan.objs[2]);
		g_signal_emit_by_name (scan.objs[2], "name-changed");
		g_object_unref (scan.objs[2]);
		CHECK (ok);

		check (tree_row_index (model, &scan.iters[0]) <
		       tree_row_index (model, &scan.iters[1]),
		       "series are listed in graph order before the move");

		handler = g_log_set_handler (NULL, G_LOG_LEVEL_CRITICAL,
					     count_critical, &criticals);
		g_signal_emit_by_name (inc_button, "clicked");
		g_log_remove_handler (NULL, handler);

		check (criticals == 0,
		       "a missed search does not disable reordering "
		       "(gog-guru.c:724)");
		check (tree_row_index (model, &scan.iters[1]) <
		       tree_row_index (model, &scan.iters[0]),
		       "the increase button moves the series in the tree");
	}

	gtk_widget_destroy (guru);
	g_object_unref (dalloc);
	g_closure_unref (closure);
	g_object_unref (graph);
}

/* gog-series.c:730: the fill-type combo sensitivity must follow the
 * interpolation, not the way the editor was opened.  */
static void
test_display_series_fill_sensitivity (void)
{
	GogGraph *graph = new_graph (200., 100.);
	GogChart *chart = graph_add_chart (graph);
	GogPlot *plot = chart_add_plot (chart, "XY", "XY Lines");
	GogSeries *series;
	GogDataAllocator *dalloc;
	GtkWidget *notebook, *combo;

	if (plot == NULL) {
		skip ("series fill sensitivity (no XY plot)");
		g_object_unref (graph);
		return;
	}
	series = plot_add_xy_series (plot, (double const[]) { 1., 2. },
				     (double const[]) { 1., 2. }, 2);
	CHECK (series != NULL);
	if (series == NULL) {
		g_object_unref (graph);
		return;
	}

	dalloc = g_object_new (test_allocator_get_type (), NULL);

	/* A clamped cubic spline skips invalid points automatically.  */
	g_object_set (series, "interpolation", "clamped-cspline", NULL);
	notebook = gog_object_get_editor (GOG_OBJECT (series), dalloc, NULL);
	CHECK (notebook != NULL);
	combo = notebook != NULL ?
		find_widget_named (notebook, "fill_type_combo") : NULL;
	CHECK (combo != NULL);
	if (combo != NULL)
		check (!gtk_widget_get_sensitive (combo),
		       "the fill-type combo is insensitive for an "
		       "auto-skipping interpolation (gog-series.c:730)");
	if (notebook != NULL) {
		g_object_ref_sink (notebook);
		gtk_widget_destroy (notebook);
		g_object_unref (notebook);
	}

	/* A linear interpolation keeps it sensitive.  */
	g_object_set (series, "interpolation", "linear", NULL);
	notebook = gog_object_get_editor (GOG_OBJECT (series), dalloc, NULL);
	CHECK (notebook != NULL);
	combo = notebook != NULL ?
		find_widget_named (notebook, "fill_type_combo") : NULL;
	CHECK (combo != NULL);
	if (combo != NULL)
		check (gtk_widget_get_sensitive (combo),
		       "the fill-type combo is sensitive for a normal "
		       "interpolation (gog-series.c:730)");
	if (notebook != NULL) {
		g_object_ref_sink (notebook);
		gtk_widget_destroy (notebook);
		g_object_unref (notebook);
	}

	g_object_unref (dalloc);
	g_object_unref (graph);
}
#endif

/* ------------------------------------------------------------------------- */

/* Internal to gog-plot-engine.c; needed here to exercise the service
 * finalize chain before any plugin has been loaded.  */
GType gog_trend_line_service_get_type (void);

int
main (int argc, char **argv)
{
	char const *env;
	int i;

	(void) argc;
	(void) argv;

#ifdef GOFFICE_WITH_GTK
	/* The "display" group needs a toplevel; everything else does not.
	 * t2003-graph-xvfb.pl supplies a virtual display.  */
	have_display = gtk_init_check (NULL, NULL);
#endif

	libgoffice_init ();

	/* Finalizing a trend-line service must not depend on the plot-type
	 * service class existing yet; exercise it before any plugin loads.  */
	{
		GObject *service = g_object_new (gog_trend_line_service_get_type (),
						 NULL);
		g_object_unref (service);
	}

	env = g_getenv ("GOFFICE_TEST_PLUGIN_DIRS");
	if (env != NULL && *env != '\0') {
		char **dirs = g_strsplit (env, G_SEARCHPATH_SEPARATOR_S, -1);
		GSList *plugin_dirs = NULL;

		/* go_plugins_init takes ownership of the list nodes and of
		 * the strings; go_plugins_shutdown frees them.  The vector
		 * itself is ours.  */
		for (i = 0; dirs[i] != NULL; i++)
			plugin_dirs = g_slist_append (plugin_dirs, dirs[i]);
		if (plugin_dirs != NULL)
			go_plugins_init (NULL, NULL, NULL, plugin_dirs, TRUE,
					 GO_TYPE_PLUGIN_LOADER_MODULE);
		g_free (dirs);
		have_plugins = g_hash_table_size ((GHashTable *)
						  gog_plot_families ()) > 0;
	}

	passes = g_string_new ("plugins");
	test_plugins ();

	g_string_append (passes, ", model");
	test_model ();
	test_graph_dup_roundtrip ();

	g_string_append (passes, ", axis");
	test_axis ();

	g_string_append (passes, ", color map");
	test_color_map ();

	g_string_append (passes, ", series");
	test_series ();

	g_string_append (passes, ", persistence");
	test_persistence ();
	test_persistent_property_roundtrip ();
	test_persistent_plot_type_roundtrip ();
	test_label_format_roundtrip ();
	test_outlined_padding_roundtrip ();

	g_string_append (passes, ", robustness");
	test_robustness ();

	g_string_append (passes, ", render");
	test_render ();

	g_string_append (passes, ", export");
	test_export ();
	test_export_format_sweep ();

	g_string_append (passes, ", engine smoke");
	test_engine_smoke ();

	g_string_append (passes, ", regressions");
	test_regression_polar_odf_spline ();
	test_regression_color_map_wrap ();
	test_regression_label_overrides ();
	test_regression_vary_cardinality ();
	test_regression_color_scale_empty_axis ();

	g_string_append (passes, ", medium");
	test_object_child_by_name ();
	test_object_getter_defaults ();
	test_metrics_unit_property ();
	test_series_data_null_contract ();
	test_unparented_series_interpolation ();
	test_renderer_update_null_model ();
	test_data_label_position ();
	test_graph_auto_placement ();
	test_chart_get_axis_order ();
	test_chart_axis_after_removal ();
	test_position_masks ();
	test_reg_curve_getter_changed ();
	test_single_stop_color_map ();
	test_trend_line_pre_insert_legend ();
	test_chart_map_3d_non_xyz ();
	test_renderer_label_scale ();
	test_renderer_label_line_width ();
	test_renderer_class_macro ();
	test_chart_map_close_paths ();
	test_chart_map_clamped_derivs ();
	test_chart_map_path_properties ();
	test_chart_map_inverted_cspline ();
	test_chart_map_polar_step ();
	test_chart_map_unimplemented ();
	test_axis_date_singleton ();
	test_axis_log_tick_cap ();
	test_chart_is_3d_superset ();
	test_regression_label_format_bounds ();
	test_regression_theme_data_label_style ();
	test_regression_grid_line_axis_ticks ();
	test_regression_color_scale_null_axis ();
	test_regression_color_scale_discrete_axis ();
	test_regression_grid_stripe_inverted ();

	g_string_append (passes, ", low");
	test_regression_object_position_null ();
	test_regression_export_unknown_format ();
	test_regression_series_label_elt_null_point ();
	test_regression_axis_ref_axis_absolute ();
	test_regression_theme_delete_no_uri ();
	test_theme_palette_reparse ();
	test_add2_ownership ();
	test_regression_xml_unattachable ();
	test_regression_reorder_head ();
	test_regression_plot_foreach_legend ();
	test_regression_make_path_empty ();
	test_regression_display_factor_underflow ();
	test_regression_xml_property_no_name ();
	test_view_label_position_zeroed ();
	test_axis_line_position_labels_format ();
	test_graph_view_selection_restore ();
	test_tool_move_object_offset ();

	if (have_display) {
		g_string_append (passes, ", display");
#ifdef GOFFICE_WITH_GTK
		test_display_guru ();
		test_display_graph_widget ();
		test_display_axis_line_editor_guard ();
		test_display_axis_editor_state_lifetime ();
		test_display_editor_get_page ();
		test_display_guru_reorder ();
		test_display_series_fill_sensitivity ();
#endif
	} else
		skip ("graph display tests (no display connection)");

	/* Run last: it releases the plugin state.  */
	g_string_append (passes, ", trend-line shutdown");
	test_trend_line_type_shutdown ();

	g_print ("test-graph: exercised %s\n", passes->str);
	g_string_free (passes, TRUE);

	libgoffice_shutdown ();

	if (skips)
		g_print ("test-graph: %d group(s) skipped.\n", skips);
	if (failures)
		g_printerr ("%d graph test(s) failed.\n", failures);

	return failures ? 1 : 0;
}
