/*
 * test-canvas.c:  Tests for the goffice canvas.
 *
 * The canvas core is built into libgoffice whether or not Gtk is enabled,
 * so most of the object model, geometry, and drawing can be exercised
 * without a display.  In a Gtk build GocCanvas derives from GtkLayout,
 * which needs a display connection to instantiate, so the canvas-level
 * tests are skipped (with a message) when gtk_init_check() fails.
 *
 * Rendering checks only demand that the ballpark color appears: a fill
 * is expected to dominate a pixel in the middle of a shape, an outline
 * is expected to darken a pixel on the line, and everything else must
 * stay background colored.
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
#include <goffice/canvas/goffice-canvas.h>

#include <math.h>
#include <stdlib.h>

static int failures;
static int skips;
static gboolean have_display = TRUE;

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
check_bounds (GocItem *item, double x0, double y0, double x1, double y1,
	      const char *what)
{
	double gx0, gy0, gx1, gy1;

	goc_item_get_bounds (item, &gx0, &gy0, &gx1, &gy1);
	if (fabs (gx0 - x0) > 1e-9 || fabs (gy0 - y0) > 1e-9 ||
	    fabs (gx1 - x1) > 1e-9 || fabs (gy1 - y1) > 1e-9) {
		g_printerr ("FAIL: %s\n"
			    "    got  %g, %g - %g, %g\n"
			    "    want %g, %g - %g, %g\n",
			    what, gx0, gy0, gx1, gy1, x0, y0, x1, y1);
		failures++;
	}
}

static void
skip (const char *what)
{
	skips++;
	g_print ("test-canvas: skipped %s\n", what);
}

/* ------------------------------------------------------------------------- */

/* Fill-only style: solid `color', no outline.  The auto flags must be
 * cleared or the theme initialization would override the colors.  */
static void
set_fill_style (GocItem *item, GOColor color)
{
	GOStyle *style = go_style_new ();

	style->fill.type = GO_STYLE_FILL_PATTERN;
	style->fill.auto_type = FALSE;
	style->fill.auto_fore = FALSE;
	style->fill.auto_back = FALSE;
	go_pattern_set_solid (&style->fill.pattern, color);

	style->line.dash_type = GO_LINE_NONE;
	style->line.auto_dash = FALSE;
	style->line.auto_color = FALSE;
	style->line.auto_fore = FALSE;
	style->line.width = 0.;

	g_object_set (item, "style", style, NULL);
	g_object_unref (style);
}

/* Outline-only style: solid black line of the given width and cap.  */
static void
set_line_style (GocItem *item, double width, cairo_line_cap_t cap)
{
	GOStyle *style = go_style_new ();

	style->fill.type = GO_STYLE_FILL_NONE;
	style->fill.auto_type = FALSE;

	style->line.dash_type = GO_LINE_SOLID;
	style->line.auto_dash = FALSE;
	style->line.color = GO_COLOR_BLACK;
	style->line.auto_color = FALSE;
	style->line.auto_fore = FALSE;
	style->line.width = width;
	style->line.cap = cap;
	style->line.join = CAIRO_LINE_JOIN_MITER;

	g_object_set (item, "style", style, NULL);
	g_object_unref (style);
}

/* ------------------------------------------------------------------------- */

static GocGroup *
new_group (GocGroup *parent)
{
	return parent != NULL
		? goc_group_new (parent)
		: GOC_GROUP (g_object_new (GOC_TYPE_GROUP, NULL));
}

static GocCanvas *
new_canvas (void)
{
	/* GtkLayout is initially unowned in a Gtk build.  */
	return GOC_CANVAS (g_object_ref_sink (g_object_new (GOC_TYPE_CANVAS, NULL)));
}

/* Swallow expected GLib criticals from deliberately invalid API usage.  */
static void
ignore_log_message (G_GNUC_UNUSED const gchar *log_domain,
		    G_GNUC_UNUSED GLogLevelFlags log_level,
		    G_GNUC_UNUSED const gchar *message,
		    G_GNUC_UNUSED gpointer user_data)
{
}

/* Counts swallowed GLib messages while an expected diagnostic is active.  */
static int log_count;

static void
count_log_message (G_GNUC_UNUSED const gchar *log_domain,
		   G_GNUC_UNUSED GLogLevelFlags log_level,
		   G_GNUC_UNUSED const gchar *message,
		   G_GNUC_UNUSED gpointer user_data)
{
	log_count++;
}

/* Counts notify::scale-line-width emissions.  */
static int notify_count;

static void
count_notify (G_GNUC_UNUSED GObject *obj, G_GNUC_UNUSED GParamSpec *pspec,
	      G_GNUC_UNUSED gpointer data)
{
	notify_count++;
}

static gboolean
group_order_is (GocGroup *group, GocItem **items, unsigned n)
{
	GPtrArray *children = goc_group_get_children (group);
	gboolean ok = children->len == n;
	unsigned ui;

	for (ui = 0; ok && ui < n; ui++)
		ok = g_ptr_array_index (children, ui) == items[ui];

	g_ptr_array_unref (children);
	return ok;
}

/* ------------------------------------------------------------------------- */

static unsigned char *
surface_pixel (cairo_surface_t *surface, int x, int y)
{
	unsigned char *data;
	int stride;

	cairo_surface_flush (surface);
	data = cairo_image_surface_get_data (surface);
	stride = cairo_image_surface_get_stride (surface);
	/* Little-endian ARGB32 is stored as B, G, R, A.  */
	return data + y * stride + 4 * x;
}

static void
check_pixel (cairo_surface_t *surface, int x, int y,
	     int r, int g, int b, int tol, const char *what)
{
	unsigned char *p = surface_pixel (surface, x, y);

	if (abs ((int) p[2] - r) > tol || abs ((int) p[1] - g) > tol ||
	    abs ((int) p[0] - b) > tol || p[3] < 200) {
		g_printerr ("FAIL: %s at (%d,%d)\n"
			    "    got  rgba %u,%u,%u,%u\n"
			    "    want near %d,%d,%d\n",
			    what, x, y, p[2], p[1], p[0], p[3], r, g, b);
		failures++;
	}
}

static void
check_pixel_white (cairo_surface_t *surface, int x, int y, const char *what)
{
	check_pixel (surface, x, y, 255, 255, 255, 5, what);
}

static void
check_pixel_dark (cairo_surface_t *surface, int x, int y, const char *what)
{
	unsigned char *p = surface_pixel (surface, x, y);

	if (p[2] > 100 || p[1] > 100 || p[0] > 100 || p[3] < 200) {
		g_printerr ("FAIL: %s at (%d,%d): rgba %u,%u,%u,%u\n",
			    what, x, y, p[2], p[1], p[0], p[3]);
		failures++;
	}
}

/* Number of clearly dark pixels in column @x between @y0 and @y1.  */
static int
dark_column_height (cairo_surface_t *surface, int x, int y0, int y1)
{
	int y, n = 0;

	for (y = y0; y <= y1; y++) {
		unsigned char *p = surface_pixel (surface, x, y);

		if (p[2] < 128 && p[1] < 128 && p[0] < 128 && p[3] > 128)
			n++;
	}
	return n;
}

/* Bounding box of pixels with a clear red tint; FALSE when there is none.
 * The exact colors are left to cairo's filtering, only the position of the
 * rendered area is checked.  */
static gboolean
red_bbox (cairo_surface_t *surface, int *x0, int *y0, int *x1, int *y1)
{
	unsigned char *data;
	int stride, width, height, x, y;
	gboolean found = FALSE;

	cairo_surface_flush (surface);
	data = cairo_image_surface_get_data (surface);
	stride = cairo_image_surface_get_stride (surface);
	width = cairo_image_surface_get_width (surface);
	height = cairo_image_surface_get_height (surface);
	*x0 = *y0 = G_MAXINT;
	*x1 = *y1 = G_MININT;

	for (y = 0; y < height; y++)
		for (x = 0; x < width; x++) {
			unsigned char *p = data + y * stride + 4 * x;

			if ((int) p[2] - (int) p[1] > 30) {
				found = TRUE;
				if (x < *x0) *x0 = x;
				if (x > *x1) *x1 = x;
				if (y < *y0) *y0 = y;
				if (y > *y1) *y1 = y;
			}
		}

	return found;
}

/* ------------------------------------------------------------------------- */

static void
test_utils (void)
{
	GocPoints *pts, *ref, *copy;
	GocIntArray *arr, *aref, *acopy;

	CHECK (G_TYPE_IS_BOXED (GOC_TYPE_POINTS));
	CHECK (G_TYPE_IS_BOXED (GOC_TYPE_INT_ARRAY));
	CHECK (GOC_TYPE_POINTS == goc_points_get_type ());
	CHECK (GOC_TYPE_INT_ARRAY == goc_int_array_get_type ());

	pts = goc_points_new (3);
	CHECK (pts != NULL);
	CHECK (pts->n == 3);
	CHECK (pts->points != NULL);
	CHECK (pts->points[0].x == 0. && pts->points[0].y == 0.);
	CHECK (pts->points[2].x == 0. && pts->points[2].y == 0.);
	pts->points[0].x = 1.5;
	pts->points[0].y = -2.5;

	ref = goc_points_ref (pts);
	CHECK (ref == pts);
	/* The boxed copy function is the ref function.  */
	copy = g_boxed_copy (GOC_TYPE_POINTS, pts);
	CHECK (copy == pts);
	CHECK (copy->n == 3 && copy->points[0].x == 1.5 && copy->points[0].y == -2.5);
	g_boxed_free (GOC_TYPE_POINTS, copy);
	goc_points_unref (ref);
	goc_points_unref (pts);

	arr = goc_int_array_new (4);
	CHECK (arr != NULL);
	CHECK (arr->n == 4);
	CHECK (arr->vals != NULL);
	CHECK (arr->vals[0] == 0 && arr->vals[3] == 0);
	arr->vals[1] = -7;

	aref = goc_int_array_ref (arr);
	CHECK (aref == arr);
	acopy = g_boxed_copy (GOC_TYPE_INT_ARRAY, arr);
	CHECK (acopy == arr);
	CHECK (acopy->n == 4 && acopy->vals[1] == -7);
	g_boxed_free (GOC_TYPE_INT_ARRAY, acopy);
	goc_int_array_unref (aref);
	goc_int_array_unref (arr);
}

/* ------------------------------------------------------------------------- */

/* A minimal non-styled item.  Its copy method only copies fields, like a
 * well-behaved subclass; dropping stale cached bounds is goc_item_copy()'s
 * job, not the subclass's.  When @set_near is %FALSE its distance method
 * returns a result without setting near_item, like several real items.  */
typedef struct {
	GocItem base;
	double x, y, w, h;
	gboolean set_near;
} SimpleItem;

typedef GocItemClass SimpleItemClass;

static void simple_item_class_init (SimpleItemClass *klass);
static void simple_item_init (SimpleItem *item);
GType simple_item_get_type (void);
G_DEFINE_TYPE (SimpleItem, simple_item, GOC_TYPE_ITEM)

static void
simple_item_init (SimpleItem *item)
{
	item->set_near = TRUE;
}

static void
simple_item_update_bounds (GocItem *item)
{
	SimpleItem *simple = (SimpleItem *) item;

	item->x0 = simple->x;
	item->y0 = simple->y;
	item->x1 = simple->x + simple->w;
	item->y1 = simple->y + simple->h;
}

static double
simple_item_distance (GocItem *item, double x, double y, GocItem **near_item)
{
	SimpleItem *simple = (SimpleItem *) item;
	double dx = MAX (MAX (simple->x - x, x - (simple->x + simple->w)), 0.);
	double dy = MAX (MAX (simple->y - y, y - (simple->y + simple->h)), 0.);

	if (simple->set_near)
		*near_item = item;
	return hypot (dx, dy);
}

static void
simple_item_copy (GocItem *dest, GocItem *source)
{
	SimpleItem *d = (SimpleItem *) dest, *s = (SimpleItem *) source;

	d->x = s->x;
	d->y = s->y;
	d->w = s->w;
	d->h = s->h;
}

static void
simple_item_class_init (SimpleItemClass *klass)
{
	GocItemClass *item_class = (GocItemClass *) klass;

	item_class->update_bounds = simple_item_update_bounds;
	item_class->distance = simple_item_distance;
	item_class->copy = simple_item_copy;
}

static void
set_simple_geometry (SimpleItem *item, double x, double y, double w, double h)
{
	item->x = x;
	item->y = y;
	item->w = w;
	item->h = h;
	goc_item_bounds_changed (GOC_ITEM (item));
}

/* An item that does not chain up its dispose, to drive the brutal clear path
 * of goc_group_clear().  */
typedef struct {
	GocItem base;
} StubbornItem;

typedef GocItemClass StubbornItemClass;

static void stubborn_item_class_init (StubbornItemClass *klass);
static void stubborn_item_init (StubbornItem *item);
GType stubborn_item_get_type (void);
G_DEFINE_TYPE (StubbornItem, stubborn_item, GOC_TYPE_ITEM)

static void
stubborn_item_init (G_GNUC_UNUSED StubbornItem *item)
{
}

static void
stubborn_item_dispose (GObject *obj)
{
	/* deliberately do not chain up */
}

static void
stubborn_item_class_init (StubbornItemClass *klass)
{
	((GObjectClass *) klass)->dispose = stubborn_item_dispose;
}

/* ------------------------------------------------------------------------- */

static void
test_groups (void)
{
	GocGroup *root, *sub, *ord, *outer, *inner;
	GocItem *a, *b, *c, *line, *order[3];
	gpointer weak;
	double x0, y0, x1, y1;

	root = new_group (NULL);
	CHECK (root != NULL);
	sub = goc_group_new (root);
	CHECK (GOC_IS_GROUP (sub));
	CHECK (goc_item_get_parent (GOC_ITEM (sub)) == root);
	CHECK (goc_group_get_child (root, 0) == GOC_ITEM (sub));
	CHECK (goc_group_get_child (root, 1) == NULL);
	CHECK (goc_group_find_child (root, GOC_ITEM (sub)) == 0);
	{
		GPtrArray *children = goc_group_get_children (root);
		CHECK (children->len == 1);
		g_ptr_array_unref (children);
	}

	/* Item ordering.  */
	ord = new_group (NULL);
	a = goc_item_new (ord, GOC_TYPE_LINE,
			  "x0", 0., "y0", 0., "x1", 1., "y1", 0., NULL);
	b = goc_item_new (ord, GOC_TYPE_LINE,
			  "x0", 0., "y0", 1., "x1", 1., "y1", 1., NULL);
	c = goc_item_new (ord, GOC_TYPE_LINE,
			  "x0", 0., "y0", 2., "x1", 1., "y1", 2., NULL);
	order[0] = a; order[1] = b; order[2] = c;
	CHECK (group_order_is (ord, order, 3));

	goc_item_raise (a, 1);
	order[0] = b; order[1] = a; order[2] = c;
	CHECK (group_order_is (ord, order, 3));

	goc_item_raise_to_top (b);
	order[0] = a; order[1] = c; order[2] = b;
	CHECK (group_order_is (ord, order, 3));

	goc_item_lower (b, 1);
	order[0] = a; order[1] = b; order[2] = c;
	CHECK (group_order_is (ord, order, 3));

	goc_item_lower_to_bottom (c);
	order[0] = c; order[1] = a; order[2] = b;
	CHECK (group_order_is (ord, order, 3));

	/* The move counts clamp at the ends.  */
	goc_item_raise (c, 100);
	order[0] = a; order[1] = b; order[2] = c;
	CHECK (group_order_is (ord, order, 3));
	goc_item_lower (c, 100);
	order[0] = c; order[1] = a; order[2] = b;
	CHECK (group_order_is (ord, order, 3));

	/* Re-parenting removes from the old group.  */
	goc_group_add_child (sub, c);
	CHECK (goc_item_get_parent (c) == sub);
	CHECK (goc_group_get_child (sub, 0) == c);
	CHECK (goc_group_get_child (ord, 0) == a);
	CHECK (goc_group_get_child (ord, 1) == b);
	CHECK (goc_group_get_child (ord, 2) == NULL);
	CHECK (goc_group_find_child (ord, c) == -1);

	/* Destroying an item removes it and frees it.  */
	g_object_add_weak_pointer (G_OBJECT (c), &weak);
	goc_item_destroy (c);
	CHECK (weak == NULL);
	CHECK (goc_group_get_child (sub, 0) == NULL);

	/* Clearing a group destroys all children.  */
	g_object_add_weak_pointer (G_OBJECT (a), &weak);
	goc_group_clear (ord);
	CHECK (weak == NULL);
	{
		GPtrArray *children = goc_group_get_children (ord);
		CHECK (children->len == 0);
		g_ptr_array_unref (children);
	}
	goc_group_clear (ord);
	g_object_unref (ord);

	/* Group offsets accumulate through the parent chain.  */
	outer = new_group (NULL);
	inner = goc_group_new (outer);
	g_object_set (outer, "x", 10., "y", 20., NULL);
	g_object_set (inner, "x", 5., "y", 6., NULL);
	line = goc_item_new (inner, GOC_TYPE_LINE,
			     "x0", 0., "y0", 0., "x1", 10., "y1", 10., NULL);
	set_line_style (line, 2., CAIRO_LINE_CAP_BUTT);

	check_bounds (line, -1., -1., 11., 11., "line inside nested groups");
	check_bounds (GOC_ITEM (inner), 4., 5., 16., 17., "inner group bounds");
	check_bounds (GOC_ITEM (outer), 14., 25., 26., 37., "outer group bounds");

	x0 = -1.; y0 = -1.; x1 = 11.; y1 = 11.;
	goc_group_adjust_bounds (inner, &x0, &y0, &x1, &y1);
	CHECK (x0 == 14. && y0 == 25. && x1 == 26. && y1 == 37.);

	x0 = 100.; y0 = 100.;
	goc_group_adjust_coords (inner, &x0, &y0);
	CHECK (x0 == 115. && y0 == 126.);

	/* Freezing defers the bounds recalculation.  */
	goc_group_freeze (outer, TRUE);
	g_object_set (line, "x1", 110., NULL);
	goc_item_get_bounds (GOC_ITEM (outer), &x0, &y0, &x1, &y1);
	CHECK (x0 == 14. && y0 == 25. && x1 == 26. && y1 == 37.);
	goc_group_freeze (outer, FALSE);
	goc_item_get_bounds (GOC_ITEM (outer), &x0, &y0, &x1, &y1);
	CHECK (x0 == 14. && y0 == 25. && x1 == 126. && y1 == 37.);

	goc_group_clear (outer);
	g_object_unref (outer);

	/* Hit testing works without a canvas.  */
	{
		GocGroup *g = new_group (NULL);
		GocItem *near = NULL;
		GocItem *child = goc_item_new (g, GOC_TYPE_RECTANGLE,
					       "x", 0., "y", 0., "width", 10., "height", 10., NULL);

		set_fill_style (child, GO_COLOR_RED);
		goc_item_get_bounds (GOC_ITEM (g), &x0, &y0, &x1, &y1);
		CHECK (goc_item_distance (GOC_ITEM (g), 5., 5., &near) == 0.);
		CHECK (near == child);
		CHECK (goc_item_distance (GOC_ITEM (g), 50., 50., &near) != 0.);
		g_object_unref (g);
	}

	/* A child whose distance does not set near_item must not leave
	 * garbage in the caller's pointer.  */
	{
		GocGroup *g = new_group (NULL);
		SimpleItem *child = (SimpleItem *) goc_item_new (g, simple_item_get_type (), NULL);
		GocItem *near = (GocItem *) (gpointer) 0xdeadbeef;

		child->set_near = FALSE;
		set_simple_geometry (child, 0., 0., 10., 10.);
		goc_item_get_bounds (GOC_ITEM (g), &x0, &y0, &x1, &y1);
		CHECK (goc_item_distance (GOC_ITEM (g), 5., 5., &near) == 0.);
		CHECK (near == GOC_ITEM (child));
		g_object_unref (g);
	}

	/* The clip path clips drawing, bounds and hit testing.  */
	{
		GocGroup *root = new_group (NULL);
		GocGroup *g = goc_group_new (root);
		GocGroup *sub = goc_group_new (g);
		GocItem *child;
		GOPath *clip;
		cairo_surface_t *surface;
		cairo_t *cr;
		GocItem *near = NULL;

		g_object_set (g, "x", 5., "y", 5., NULL);
		g_object_set (sub, "x", 10., "y", 10., NULL);
		child = goc_item_new (sub, GOC_TYPE_RECTANGLE,
				      "x", 0., "y", 0., "width", 10., "height", 10., NULL);
		set_fill_style (child, GO_COLOR_RED);

		clip = go_path_new ();
		go_path_rectangle (clip, 0., 0., 5., 5.);
		goc_group_set_clip_path (sub, clip, CAIRO_FILL_RULE_WINDING);
		go_path_free (clip);

		/* bounds are clamped to the clip region; coordinates of @g are
		 * in the root's space */
		check_bounds (GOC_ITEM (g), 15., 15., 20., 20., "clipped group bounds");

		/* only the clipped part is drawn */
		surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 40, 40);
		cr = cairo_create (surface);
		cairo_set_source_rgb (cr, 1., 1., 1.);
		cairo_paint (cr);
		goc_item_draw_region (GOC_ITEM (root), cr, 0., 0., 40., 40.);
		check_pixel (surface, 17, 17, 255, 0, 0, 60, "clipped group draw");
		check_pixel_white (surface, 23, 23, "clipped group draw, outside clip");
		cairo_destroy (cr);
		cairo_surface_destroy (surface);

		/* hit testing follows the clip path too */
		CHECK (goc_item_distance (GOC_ITEM (sub), 12., 12., &near) == 0.);
		CHECK (goc_item_distance (GOC_ITEM (sub), 17., 17., &near) != 0.);
		CHECK (goc_item_distance (GOC_ITEM (g), 17., 17., &near) == 0.);
		CHECK (goc_item_distance (GOC_ITEM (g), 22., 22., &near) != 0.);

		g_object_unref (root);
	}

	/* An unmatched unfreeze must not freeze the group forever.  */
	{
		GocGroup *g = new_group (NULL);
		GocItem *child;

		goc_group_freeze (g, FALSE);
		child = goc_item_new (g, GOC_TYPE_RECTANGLE,
				      "x", 10., "y", 10., "width", 20., "height", 20., NULL);
		set_fill_style (child, GO_COLOR_RED);
		check_bounds (GOC_ITEM (g), 10., 10., 30., 30.,
			      "bounds after unmatched unfreeze");
		g_object_unref (g);
	}

	/* The legacy Xchildren list tracks the first child.  */
	{
		GocGroup *g = new_group (NULL);
		GocItem *c1 = goc_item_new (g, GOC_TYPE_RECTANGLE, NULL);
		GocItem *c2 = goc_item_new (g, GOC_TYPE_RECTANGLE, NULL);

		CHECK (g->Xchildren != NULL && g->Xchildren->data == c1);
		goc_item_raise (c1, 1);
		CHECK (g->Xchildren != NULL && g->Xchildren->data == c2);
		goc_item_lower (c1, 1);
		CHECK (g->Xchildren != NULL && g->Xchildren->data == c1);
		g_object_unref (g);
	}

	/* The brutal clear path refreshes Xchildren.  */
	{
		GocGroup *g = new_group (NULL);
		GocItem *stubborn = goc_item_new (g, stubborn_item_get_type (), NULL);
		GLogLevelFlags old_mask = g_log_set_always_fatal (0);
		GLogFunc old_handler = g_log_set_default_handler (ignore_log_message, NULL);

		CHECK (g->Xchildren != NULL && g->Xchildren->data == stubborn);
		goc_group_clear (g);
		g_log_set_default_handler (old_handler, NULL);
		g_log_set_always_fatal (old_mask);

		CHECK (g->Xchildren == NULL);
		g_object_unref (g);
	}

	/* An item whose bookkeeping is stale must not be detached, raised
	 * or lowered with an invalid index.  */
	{
		GocGroup *g = new_group (NULL);
		GocItem *it = goc_item_new (g, GOC_TYPE_RECTANGLE, NULL);
		GPtrArray *children = goc_group_get_children (g);
		GLogLevelFlags old_mask = g_log_set_always_fatal (0);
		GLogFunc old_handler = g_log_set_default_handler (ignore_log_message, NULL);

		g_ptr_array_remove_index (children, 0);	/* stale */
		goc_group_remove_child (g, it);

		/* Criticals stay fatal here: raise/lower must not feed the
		 * invalid index to the array either.  */
		g_log_set_always_fatal (old_mask);
		goc_item_raise (it, 1);
		goc_item_lower (it, 1);
		g_log_set_default_handler (old_handler, NULL);

		CHECK (it->parent == g);
		g_ptr_array_unref (children);
		it->parent = NULL;
		g_object_unref (it);
		g_object_unref (g);
	}

	/* Reordering an item without a parent reports a critical and returns
	 * instead of crashing.  */
	{
		GocGroup *g = new_group (NULL);
		GLogLevelFlags old_mask = g_log_set_always_fatal (0);
		GLogFunc old_handler =
			g_log_set_default_handler (count_log_message, NULL);

		log_count = 0;
		goc_item_raise (GOC_ITEM (g), 1);
		goc_item_lower (GOC_ITEM (g), 1);
		g_log_set_default_handler (old_handler, NULL);
		g_log_set_always_fatal (old_mask);

		CHECK (log_count == 2);
		g_object_unref (g);
	}

	/* The root owns `sub' and everything below it.  */
	goc_group_clear (root);
	g_object_unref (root);
}

/* ------------------------------------------------------------------------- */

static void
test_item_common (void)
{
	GocGroup *root, *group;
	GocItem *line, *child, *dup, *rect, *rdup, *gl, *gl2;
	GocGroup *gsrc, *gdup;
	cairo_matrix_t m;
	double d;

	CHECK (g_type_is_a (GOC_TYPE_LINE, GOC_TYPE_STYLED_ITEM));
	CHECK (g_type_is_a (GOC_TYPE_RECTANGLE, GOC_TYPE_STYLED_ITEM));
	CHECK (g_type_is_a (GOC_TYPE_STYLED_ITEM, GOC_TYPE_ITEM));
	CHECK (g_type_is_a (GOC_TYPE_GROUP, GOC_TYPE_ITEM));
	CHECK (g_type_is_a (GOC_TYPE_CANVAS, G_TYPE_OBJECT));

	root = new_group (NULL);
	line = goc_item_new (root, GOC_TYPE_LINE,
			     "x0", 1., "y0", 2., "x1", 11., "y1", 4., NULL);

	/* Visibility.  */
	CHECK (goc_item_is_visible (line));
	goc_item_hide (line);
	CHECK (!goc_item_is_visible (line));
	goc_item_show (line);
	CHECK (goc_item_is_visible (line));
	goc_item_set_visible (line, FALSE);
	CHECK (!goc_item_is_visible (line));
	goc_item_set_visible (line, TRUE);

	/* Operator.  */
	goc_item_set_operator (line, CAIRO_OPERATOR_MULTIPLY);
	CHECK (goc_item_get_operator (line) == CAIRO_OPERATOR_MULTIPLY);

	/* Transforms.  */
	cairo_matrix_init_identity (&m);
	goc_item_set_transform (line, &m);
	check_near (_goc_item_transform_scale (line), 1., 1e-9,
		    "identity transform scale");

	cairo_matrix_init_scale (&m, 2., 3.);
	goc_item_set_transform (line, &m);
	check_near (_goc_item_transform_scale (line), 3., 1e-9,
		    "scale (2,3) transform scale");

	cairo_matrix_init_rotate (&m, M_PI / 2.);
	goc_item_set_transform (line, &m);
	check_near (_goc_item_transform_scale (line), 1., 1e-9,
		    "rotation transform scale");

	cairo_matrix_init_translate (&m, 5., 7.);
	goc_item_set_transform (line, &m);
	check_near (_goc_item_transform_scale (line), 1., 1e-9,
		    "translation transform scale");

	/* Ancestor transforms are included.  */
	group = goc_group_new (root);
	child = goc_item_new (group, GOC_TYPE_LINE, NULL);
	cairo_matrix_init_scale (&m, 3., 3.);
	goc_item_set_transform (GOC_ITEM (group), &m);
	cairo_matrix_init_scale (&m, 2., 2.);
	goc_item_set_transform (child, &m);
	check_near (_goc_item_transform_scale (child), 6., 1e-9,
		    "composed transform scale");

	/* The transform is part of the item geometry: changing it must drop
	 * cached bounds and recompute them.  */
	{
		GocItem *r = goc_item_new (root, GOC_TYPE_RECTANGLE,
					   "width", 10., "height", 10., NULL);

		set_fill_style (r, GO_COLOR_RED);
		check_bounds (r, 0., 0., 10., 10., "bounds before transform");
		cairo_matrix_init_scale (&m, 2., 2.);
		goc_item_set_transform (r, &m);
		check_bounds (r, 0., 0., 20., 20., "bounds after transform");
		cairo_matrix_init_identity (&m);
		goc_item_set_transform (r, &m);
		check_bounds (r, 0., 0., 10., 10., "bounds after transform reset");
	}

	/* goc_item_set and property access.  */
	goc_item_set (line, "x0", 7., "y0", 8., NULL);
	CHECK (GOC_LINE (line)->startx == 7. && GOC_LINE (line)->starty == 8.);
	g_object_get (line, "x0", &d, NULL);
	CHECK (d == 7.);
	{
		GocGroup *p = NULL;
		g_object_get (line, "parent", &p, NULL);
		CHECK (p == root);
		if (p != NULL)
			g_object_unref (p);
	}

	/* Styles survive being set and read back.  */
	set_fill_style (line, GO_COLOR_RED);
	{
		GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (line));
		CHECK (style != NULL);
		CHECK (style->fill.type == GO_STYLE_FILL_PATTERN);
		CHECK (style->fill.pattern.back == GO_COLOR_RED);
	}

	/* Duplication copies geometry, behavior fields, and style.  */
	cairo_matrix_init_scale (&m, 2., 2.);
	goc_item_set_transform (line, &m);
	{
		GOArrow arrow;
		go_arrow_init_kite (&arrow, 1., 3., 4.);
		g_object_set (line, "start-arrow", &arrow, NULL);
	}
	goc_item_hide (line);

	dup = goc_item_duplicate (line, root);
	CHECK (dup != NULL && dup != line);
	CHECK (GOC_IS_LINE (dup));
	CHECK (!goc_item_is_visible (dup));
	CHECK (goc_item_get_operator (dup) == CAIRO_OPERATOR_MULTIPLY);
	CHECK (GOC_LINE (dup)->startx == 7. && GOC_LINE (dup)->starty == 8.);
	CHECK (GOC_LINE (dup)->endx == 11. && GOC_LINE (dup)->endy == 4.);
	CHECK (go_arrow_equal (&GOC_LINE (dup)->start_arrow,
			       &GOC_LINE (line)->start_arrow));
	check_near (_goc_item_transform_scale (dup), 2., 1e-9,
		    "duplicate transform scale");
	CHECK (go_styled_object_get_style (GO_STYLED_OBJECT (dup))->fill.pattern.back ==
	       GO_COLOR_RED);

	/* Rectangle duplication.  */
	rect = goc_item_new (root, GOC_TYPE_RECTANGLE,
			     "x", 1., "y", 2., "width", 3., "height", 4.,
			     "rotation", 0.5, "type", 5, "rx", 1., "ry", 2., NULL);
	rdup = goc_item_duplicate (rect, root);
	CHECK (GOC_IS_RECTANGLE (rdup));
	CHECK (GOC_RECTANGLE (rdup)->x == 1. && GOC_RECTANGLE (rdup)->y == 2. &&
	       GOC_RECTANGLE (rdup)->width == 3. && GOC_RECTANGLE (rdup)->height == 4. &&
	       GOC_RECTANGLE (rdup)->rotation == 0.5 &&
	       GOC_RECTANGLE (rdup)->type == 5 &&
	       GOC_RECTANGLE (rdup)->rx == 1. && GOC_RECTANGLE (rdup)->ry == 2.);

	/* Duplicating a group duplicates its subtree.  */
	gsrc = goc_group_new (root);
	gl = goc_item_new (gsrc, GOC_TYPE_LINE,
			   "x0", 1., "y0", 0., "x1", 2., "y1", 0., NULL);
	CHECK (goc_item_get_parent (gl) == gsrc);
	gdup = GOC_GROUP (goc_item_duplicate (GOC_ITEM (gsrc), root));
	CHECK (gdup != NULL && gdup != gsrc && GOC_IS_GROUP (gdup));
	CHECK (goc_group_get_child (gdup, 0) != NULL);
	CHECK (goc_group_get_child (gdup, 1) == NULL);
	gl2 = goc_group_get_child (gdup, 0);
	CHECK (GOC_IS_LINE (gl2));
	CHECK (GOC_LINE (gl2)->startx == 1. && GOC_LINE (gl2)->endx == 2.);

	/* A clone is detached and owned by the caller.  */
	{
		GocItem *clone = goc_item_clone (rect);
		gpointer weak = clone;

		CHECK (clone != NULL && clone != rect);
		if (clone != NULL) {
			CHECK (GOC_IS_RECTANGLE (clone));
			CHECK (clone->parent == NULL && clone->canvas == NULL);
			CHECK (GOC_RECTANGLE (clone)->x == 1. &&
			       GOC_RECTANGLE (clone)->width == 3.);
			g_object_add_weak_pointer (G_OBJECT (clone), &weak);
			g_object_unref (clone);
			CHECK (weak == NULL);
		}
	}

	/* Duplicating with a NULL parent still works, but warns and returns
	 * a caller-owned item.  The warning is capped at three messages.  */
	{
		GLogLevelFlags old_mask = g_log_set_always_fatal (0);
		GLogFunc old_handler =
			g_log_set_default_handler (count_log_message, NULL);
		GocItem *odup;
		gpointer weak;
		unsigned i;

		log_count = 0;
		odup = goc_item_duplicate (rect, NULL);
		CHECK (log_count == 1);
		CHECK (odup != NULL);
		if (odup != NULL) {
			CHECK (odup->parent == NULL && odup->canvas == NULL);
			CHECK (GOC_IS_RECTANGLE (odup));
			CHECK (GOC_RECTANGLE (odup)->x == 1. &&
			       GOC_RECTANGLE (odup)->width == 3.);
			weak = odup;
			g_object_add_weak_pointer (G_OBJECT (odup), &weak);
			g_object_unref (odup);
			CHECK (weak == NULL);
		}

		/* Further calls keep working but stop warning after three.  */
		for (i = 0; i < 5; i++) {
			GocItem *extra = goc_item_duplicate (rect, NULL);

			CHECK (extra != NULL);
			if (extra != NULL)
				g_object_unref (extra);
		}

		g_log_set_default_handler (old_handler, NULL);
		g_log_set_always_fatal (old_mask);

		CHECK (log_count == 3);
	}

	goc_group_clear (root);
	g_object_unref (root);
}

/* ------------------------------------------------------------------------- */

static void
test_line (void)
{
	GocGroup *root = new_group (NULL);
	GocItem *line, *dot, *dup;
	GocItem *near = NULL;
	GOArrow arrow;
	double d;

	line = goc_item_new (root, GOC_TYPE_LINE,
			     "x0", 1., "y0", 2., "x1", 11., "y1", 2., NULL);

	/* The default (zero width) line is stroked as a hairline with a
	 * half-unit slack; the hairline is pixel snapped, so the bounds sit
	 * on the snapped position.  */
	check_bounds (line, 0.5, 2.0, 11.5, 3.0, "line bounds, default style");

	set_line_style (line, 4., CAIRO_LINE_CAP_BUTT);
	check_bounds (line, -1., 0., 13., 4., "line bounds, width 4, butt cap");

	set_line_style (line, 4., CAIRO_LINE_CAP_SQUARE);
	check_bounds (line, -1., 0., 13., 4., "line bounds, width 4, square cap");

	/* Distances.  Widths up to 5 get a 2.5 hit tolerance; larger
	 * widths get half the width.  */
	set_line_style (line, 0., CAIRO_LINE_CAP_BUTT);
	d = goc_item_distance (line, 5., 2., &near);
	CHECK (d == 0. && near == line);
	CHECK (goc_item_distance (line, 5., 3., &near) == 0.);
	check_near (goc_item_distance (line, 5., 6., &near), 1.5, 1e-12,
		    "line distance, 4 above");
	check_near (goc_item_distance (line, 5., 20., &near), 15.5, 1e-12,
		    "line distance, 18 above");
	check_near (goc_item_distance (line, 15., 2., &near), 4., 1e-12,
		    "line distance, 4 past the end");
	check_near (goc_item_distance (line, -1., 2., &near), 2., 1e-12,
		    "line distance, 2 before the start");

	set_line_style (line, 6., CAIRO_LINE_CAP_BUTT);
	check_near (goc_item_distance (line, 5., 7., &near), 2., 1e-12,
		    "line distance, width 6");

	/* A degenerate line is just a point.  */
	dot = goc_item_new (root, GOC_TYPE_LINE,
			    "x0", 5., "y0", 5., "x1", 5., "y1", 5., NULL);
	check_bounds (dot, 4.5, 4.5, 5.5, 5.5, "degenerate line bounds");
	check_near (goc_item_distance (dot, 5., 10., &near), 5., 1e-12,
		    "degenerate line distance");
	check_near (goc_item_distance (dot, 8., 9., &near), 5., 1e-12,
		    "degenerate line distance 2");

	/* Arrows expand the bounds by hypot(b,c) for a kite.  */
	set_line_style (line, 4., CAIRO_LINE_CAP_BUTT);
	go_arrow_init_kite (&arrow, 1., 3., 4.);
	g_object_set (line, "start-arrow", &arrow, NULL);
	check_bounds (line, -6., -5., 18., 9., "line bounds with kite arrow");
	CHECK (go_arrow_equal (&GOC_LINE (line)->start_arrow, &arrow));

	dup = goc_item_duplicate (line, root);
	CHECK (go_arrow_equal (&GOC_LINE (dup)->start_arrow,
			       &GOC_LINE (line)->start_arrow));

	/* A long arrow is covered by its tip extent a.  */
	go_arrow_init_kite (&arrow, 20., 2., 2.);
	g_object_set (line, "start-arrow", &arrow, NULL);
	check_bounds (line, -21., -20., 33., 24., "line bounds with long arrow");
	CHECK (goc_item_distance (line, 19., 2., &near) == 0.);
	CHECK (goc_item_distance (line, 19., 6., &near) != 0.);

	/* Setting the properties to NULL clears the arrows instead of
	 * dereferencing a NULL boxed value.  */
	g_object_set (line, "start-arrow", NULL, NULL);
	CHECK (GOC_LINE (line)->start_arrow.typ == GO_ARROW_NONE);
	CHECK (GOC_LINE (line)->start_arrow.a == 0.);
	g_object_set (line, "end-arrow", NULL, NULL);
	CHECK (GOC_LINE (line)->end_arrow.typ == GO_ARROW_NONE);
	check_bounds (line, -1., 0., 13., 4., "line bounds after clearing arrows");

	/* Non-integer coordinates are pixel snapped: the bounds must cover
	 * the snapped ink.  */
	{
		GocItem *snap = goc_item_new (root, GOC_TYPE_LINE,
					      "x0", 10.4, "y0", 20., "x1", 30.4, "y1", 20., NULL);
		set_line_style (snap, 1., CAIRO_LINE_CAP_BUTT);
		check_bounds (snap, 9.5, 20., 30.5, 21., "snapped line bounds");
	}

	/* The item transform is part of the geometry.  */
	{
		cairo_matrix_t sm;
		GocItem *tr = goc_item_new (root, GOC_TYPE_LINE,
					    "x0", 0., "y0", 0., "x1", 20., "y1", 0., NULL);
		set_line_style (tr, 4., CAIRO_LINE_CAP_BUTT);
		cairo_matrix_init_scale (&sm, 2., 2.);
		goc_item_set_transform (tr, &sm);
		check_bounds (tr, -4., -4., 44., 4., "transformed line bounds");
		check_near (goc_item_distance (tr, 30., 5., &near), .5, 1e-12,
			    "transformed line distance");
		CHECK (goc_item_distance (tr, 30., 3., &near) == 0.);
	}

	/* Draw smoke test.  */
	{
		cairo_surface_t *surface =
			cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 20, 10);
		cairo_t *cr = cairo_create (surface);

		cairo_set_source_rgb (cr, 1., 1., 1.);
		cairo_paint (cr);
		goc_item_draw (line, cr);
		check_pixel_dark (surface, 5, 2, "line draw on the line");
		check_pixel_white (surface, 15, 8, "line draw off the line");

		cairo_destroy (cr);
		cairo_surface_destroy (surface);
	}

	/* The kite heads are not reversed: each tip sits at the line end
	 * and the kite widens back along the line.  */
	{
		GocItem *arrowed = goc_item_new (root, GOC_TYPE_LINE,
						 "x0", 10., "y0", 20., "x1", 40., "y1", 20., NULL);
		cairo_surface_t *surface =
			cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 50, 40);
		cairo_t *cr = cairo_create (surface);
		GOArrow kite;

		set_line_style (arrowed, 2., CAIRO_LINE_CAP_BUTT);
		go_arrow_init_kite (&kite, 6., 4., 4.);
		g_object_set (arrowed, "start-arrow", &kite,
			      "end-arrow", &kite, NULL);

		cairo_set_source_rgb (cr, 1., 1., 1.);
		cairo_paint (cr);
		goc_item_draw (arrowed, cr);

		/* Start head: nothing before the tip at the line start
		 * (x=10), widest at x=13.  */
		CHECK (dark_column_height (surface, 9, 10, 30) == 0);
		CHECK (dark_column_height (surface, 13, 10, 30) >= 6);
		CHECK (dark_column_height (surface, 11, 10, 30) <
		       dark_column_height (surface, 13, 10, 30));
		/* End head: widest at x=36, narrowing to the tip at the line
		 * end (x=40), nothing beyond it.  */
		CHECK (dark_column_height (surface, 36, 10, 30) >= 6);
		CHECK (dark_column_height (surface, 39, 10, 30) <
		       dark_column_height (surface, 36, 10, 30));
		CHECK (dark_column_height (surface, 40, 10, 30) == 0);
		CHECK (dark_column_height (surface, 41, 10, 30) == 0);

		cairo_destroy (cr);
		cairo_surface_destroy (surface);
	}

	g_object_unref (root);
}

/* ------------------------------------------------------------------------- */

static void
test_rectangle (void)
{
	GocGroup *root = new_group (NULL);
	GocItem *rect, *empty, *rdup;
	GocItem *near = NULL;
	double x0, y0, x1, y1;

	rect = goc_item_new (root, GOC_TYPE_RECTANGLE,
			     "x", 10., "y", 10., "width", 40., "height", 40., NULL);

	/* The default style fills and strokes with a hairline, hence the
	 * half-unit slack.  */
	check_bounds (rect, 9.5, 9.5, 50.5, 50.5, "rectangle bounds, default style");

	set_fill_style (rect, GO_COLOR_RED);
	check_bounds (rect, 10., 10., 50., 50., "rectangle bounds, fill only");

	/* Hit testing lives in canvas coordinates even off the origin.  */
	CHECK (goc_item_distance (rect, 30., 30., &near) == 0.);
	CHECK (goc_item_distance (rect, 100., 100., &near) == 20.);

	set_line_style (rect, 4., CAIRO_LINE_CAP_BUTT);
	check_bounds (rect, 8., 8., 52., 52., "rectangle bounds, width 4 outline");

	/* Rounded corners leave the fill extents alone.  */
	set_fill_style (rect, GO_COLOR_RED);
	g_object_set (rect, "type", GOC_RECTANGLE_ALL_CORNERS_ROUND,
		      "rx", 5., "ry", 5., NULL);
	check_bounds (rect, 10., 10., 50., 50., "rounded rectangle bounds");

	/* A 90 degree rotation happens around the top left corner, moving
	 * the box to the left of its position.  */
	g_object_set (rect, "type", 0, "rx", 0., "ry", 0.,
		      "rotation", M_PI / 2., NULL);
	check_bounds (rect, -30., 10., 10., 50., "rotated rectangle bounds");

	/* Properties round trip.  */
	g_object_set (rect, "rotation", 0.25, "type", 5, "rx", 1., "ry", 2., NULL);
	g_object_get (rect, "x", &x0, "y", &y0, "width", &x1, "height", &y1, NULL);
	CHECK (x0 == 10. && y0 == 10. && x1 == 40. && y1 == 40.);
	CHECK (GOC_RECTANGLE (rect)->rotation == 0.25);
	CHECK (GOC_RECTANGLE (rect)->type == 5);
	CHECK (GOC_RECTANGLE (rect)->rx == 1. && GOC_RECTANGLE (rect)->ry == 2.);

	/* Duplication copies the shape.  */
	rdup = goc_item_duplicate (rect, root);
	CHECK (GOC_IS_RECTANGLE (rdup));
	CHECK (GOC_RECTANGLE (rdup)->rotation == 0.25 &&
	       GOC_RECTANGLE (rdup)->type == 5 &&
	       GOC_RECTANGLE (rdup)->rx == 1. &&
	       GOC_RECTANGLE (rdup)->ry == 2.);

	/* A zero-sized rectangle is not drawn and is never hit.  */
	empty = goc_item_new (root, GOC_TYPE_RECTANGLE, NULL);
	CHECK (goc_item_distance (empty, 0., 0., &near) == 20.);

	/* Draw smoke test.  */
	set_fill_style (rect, GO_COLOR_RED);
	g_object_set (rect, "type", 0, "rx", 0., "ry", 0., "rotation", 0., NULL);
	{
		cairo_surface_t *surface =
			cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 60, 60);
		cairo_t *cr = cairo_create (surface);

		cairo_set_source_rgb (cr, 1., 1., 1.);
		cairo_paint (cr);
		goc_item_draw (rect, cr);
		check_pixel (surface, 30, 30, 255, 0, 0, 60, "rectangle fill draw");
		check_pixel_white (surface, 5, 5, "rectangle draw outside");

		cairo_destroy (cr);
		cairo_surface_destroy (surface);
	}

	/* The corner names must round the corners they name.  */
	{
		static const struct {
			int type;
			int cx, cy;	/* pixel to be rounded away */
			int fx, fy;	/* pixel to stay filled */
			const char *rounded;
			const char *filled;
		} cases[] = {
			{ GOC_RECTANGLE_NW_CORNER_ROUND, 5, 5, 24, 24,
			  "NW corner rounded", "NW case, SE filled" },
			{ GOC_RECTANGLE_NE_CORNER_ROUND, 24, 5, 5, 24,
			  "NE corner rounded", "NE case, SW filled" },
			{ GOC_RECTANGLE_SW_CORNER_ROUND, 5, 24, 24, 5,
			  "SW corner rounded", "SW case, NE filled" },
			{ GOC_RECTANGLE_SE_CORNER_ROUND, 24, 24, 5, 5,
			  "SE corner rounded", "SE case, NW filled" },
		};
		GocItem *corner = goc_item_new (root, GOC_TYPE_RECTANGLE,
						"x", 5., "y", 5., "width", 20., "height", 20.,
						"rx", 6., "ry", 6., NULL);
		cairo_surface_t *surface =
			cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 30, 30);
		cairo_t *cr = cairo_create (surface);
		unsigned i;

		set_fill_style (corner, GO_COLOR_RED);
		for (i = 0; i < G_N_ELEMENTS (cases); i++) {
			g_object_set (corner, "type", cases[i].type, NULL);

			cairo_set_source_rgb (cr, 1., 1., 1.);
			cairo_paint (cr);
			goc_item_draw (corner, cr);

			check_pixel_white (surface, cases[i].cx, cases[i].cy,
					   cases[i].rounded);
			check_pixel (surface, cases[i].fx, cases[i].fy, 255, 0, 0, 60,
				     cases[i].filled);
		}

		cairo_destroy (cr);
		cairo_surface_destroy (surface);
	}

	/* Fractional sizes are kept: the bounds, the hit region and the ink
	 * agree with the declared box.  */
	{
		GocItem *narrow = goc_item_new (root, GOC_TYPE_RECTANGLE,
						"x", 10., "y", 10., "width", 0.5, "height", 10., NULL);
		GocItem *wide = goc_item_new (root, GOC_TYPE_RECTANGLE,
					      "x", 10., "y", 10., "width", 30.7, "height", 10., NULL);

		set_fill_style (narrow, GO_COLOR_RED);
		goc_item_get_bounds (narrow, &x0, &y0, &x1, &y1);
		check_near (x0, 10., 1e-9, "narrow rectangle x0");
		check_near (y0, 10., 1e-9, "narrow rectangle y0");
		check_near (x1, 10.5, 1e-2, "narrow rectangle x1");
		check_near (y1, 20., 1e-9, "narrow rectangle y1");
		CHECK (goc_item_distance (narrow, 10.25, 15., &near) == 0.);
		CHECK (goc_item_distance (narrow, 10.75, 15., &near) != 0.);
		{
			cairo_surface_t *surface =
				cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 24, 24);
			cairo_t *cr = cairo_create (surface);
			int rx0, ry0, rx1, ry1;

			cairo_set_source_rgb (cr, 1., 1., 1.);
			cairo_paint (cr);
			goc_item_draw (narrow, cr);
			CHECK (red_bbox (surface, &rx0, &ry0, &rx1, &ry1));
			CHECK (rx0 == 10 && rx1 == 10 && ry0 == 10 && ry1 == 19);

			cairo_destroy (cr);
			cairo_surface_destroy (surface);
		}

		set_fill_style (wide, GO_COLOR_RED);
		goc_item_get_bounds (wide, &x0, &y0, &x1, &y1);
		check_near (x0, 10., 1e-9, "wide rectangle x0");
		check_near (y0, 10., 1e-9, "wide rectangle y0");
		check_near (x1, 40.7, 1e-2, "wide rectangle x1");
		check_near (y1, 20., 1e-9, "wide rectangle y1");
	}

	g_object_unref (root);
}

/* ------------------------------------------------------------------------- */

static void
test_polyline (void)
{
	GocGroup *root = new_group (NULL);
	GocItem *pl, *dup;
	GocItem *near = NULL;
	GocPoints *pts, *got;

	pl = goc_item_new (root, GOC_TYPE_POLYLINE, NULL);
	pts = goc_points_new (3);
	pts->points[0].x = 0.; pts->points[0].y = 0.;
	pts->points[1].x = 10.; pts->points[1].y = 0.;
	pts->points[2].x = 10.; pts->points[2].y = 10.;
	g_object_set (pl, "points", pts, NULL);
	CHECK (GOC_POLYLINE (pl)->nb_points == 3);

	/* The item copies the points.  */
	pts->points[1].x = 100.;
	goc_points_unref (pts);

	got = NULL;
	g_object_get (pl, "points", &got, NULL);
	CHECK (got != NULL);
	CHECK (got->n == 3);
	CHECK (got->points[1].x == 10. && got->points[1].y == 0.);
	CHECK (got->points[2].x == 10. && got->points[2].y == 10.);
	g_boxed_free (GOC_TYPE_POINTS, got);

	/* Stroked bounds: the outside of the corner join adds room to the
	 * vertex box, and the line width must survive the computation.  */
	set_line_style (pl, 4., CAIRO_LINE_CAP_BUTT);
	check_bounds (pl, 0., -2., 12., 10., "polyline bounds, width 4");
	CHECK (go_styled_object_get_style (GO_STYLED_OBJECT (pl))->line.width == 4.);

	/* Hit testing covers the stroke plus half the line width.  */
	CHECK (goc_item_distance (pl, 5., 0., &near) == 0.);
	CHECK (goc_item_distance (pl, 10., 5., &near) == 0.);
	CHECK (goc_item_distance (pl, 5., 2., &near) == 0.);
	CHECK (goc_item_distance (pl, 5., 4., &near) == 20.);
	CHECK (goc_item_distance (pl, 20., 20., &near) == 20.);
	CHECK (go_styled_object_get_style (GO_STYLED_OBJECT (pl))->line.width == 4.);

	/* An empty polyline has inverted (empty) bounds and is never hit.  */
	{
		GocItem *empty = goc_item_new (root, GOC_TYPE_POLYLINE, NULL);
		double x0, y0, x1, y1;

		goc_item_get_bounds (empty, &x0, &y0, &x1, &y1);
		CHECK (x0 == G_MAXDOUBLE && y0 == G_MAXDOUBLE);
		CHECK (x1 == -G_MAXDOUBLE && y1 == -G_MAXDOUBLE);
		CHECK (goc_item_distance (empty, 1., 1., &near) == 20.);
	}

	/* The spline variant accepts points and produces sane bounds.  */
	{
		GocItem *spline = goc_item_new (root, GOC_TYPE_POLYLINE,
						"use-spline", TRUE, NULL);
		GocPoints *sp = goc_points_new (4);
		double x0, y0, x1, y1;

		sp->points[0].x = 0.; sp->points[0].y = 0.;
		sp->points[1].x = 10.; sp->points[1].y = 0.;
		sp->points[2].x = 10.; sp->points[2].y = 10.;
		sp->points[3].x = 0.; sp->points[3].y = 10.;
		g_object_set (spline, "points", sp, NULL);
		goc_points_unref (sp);
		CHECK (GOC_POLYLINE (spline)->use_spline);

		goc_item_get_bounds (spline, &x0, &y0, &x1, &y1);
		CHECK (x0 <= 0. && y0 <= 0. && x1 >= 10. && y1 >= 10.);
		CHECK (x0 >= -10. && y0 >= -10. && x1 <= 20. && y1 <= 20.);

		dup = goc_item_duplicate (spline, root);
		CHECK (GOC_POLYLINE (dup)->use_spline);
		CHECK (GOC_POLYLINE (dup)->nb_points == 4);
	}

	/* Draw smoke test.  */
	{
		cairo_surface_t *surface =
			cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 20, 20);
		cairo_t *cr = cairo_create (surface);

		cairo_set_source_rgb (cr, 1., 1., 1.);
		cairo_paint (cr);
		goc_item_draw (pl, cr);
		check_pixel_dark (surface, 5, 0, "polyline draw, horizontal");
		check_pixel_dark (surface, 10, 5, "polyline draw, vertical");
		check_pixel_white (surface, 15, 8, "polyline draw outside");

		cairo_destroy (cr);
		cairo_surface_destroy (surface);
	}

	g_object_unref (root);
}

/* ------------------------------------------------------------------------- */

static void
test_polygon (void)
{
	GocGroup *root = new_group (NULL);
	GocItem *poly;
	GocPoints *pts;
	GocItem *near = NULL;
	GOStyle *style;
	cairo_matrix_t m;

	poly = goc_item_new (root, GOC_TYPE_POLYGON, NULL);
	pts = goc_points_new (4);
	pts->points[0].x = 0.; pts->points[0].y = 0.;
	pts->points[1].x = 10.; pts->points[1].y = 0.;
	pts->points[2].x = 10.; pts->points[2].y = 10.;
	pts->points[3].x = 0.; pts->points[3].y = 10.;
	g_object_set (poly, "points", pts, NULL);
	goc_points_unref (pts);
	CHECK (GOC_POLYGON (poly)->nb_points == 4);

	/* Fill only: bounds are the vertex box.  */
	set_fill_style (poly, GO_COLOR_RED);
	check_bounds (poly, 0., 0., 10., 10., "polygon bounds, fill only");
	CHECK (goc_item_distance (poly, 5., 5., &near) == 0.);
	CHECK (goc_item_distance (poly, 50., 50., &near) != 0.);

	/* Stroke bounds use the real line width; distance covers the stroke
	 * and leaves the style alone.  */
	set_line_style (poly, 4., CAIRO_LINE_CAP_BUTT);
	check_bounds (poly, -2., -2., 12., 12., "polygon bounds, width 4");
	CHECK (goc_item_distance (poly, 11., 5., &near) == 0.);
	CHECK (goc_item_distance (poly, 5., 5., &near) != 0.);
	style = go_styled_object_get_style (GO_STYLED_OBJECT (poly));
	CHECK (style->line.width == 4.);
	CHECK (style->line.dash_type == GO_LINE_SOLID);

	/* The item transform is part of the hit region.  */
	set_fill_style (poly, GO_COLOR_RED);
	cairo_matrix_init_scale (&m, 0.5, 0.5);
	goc_item_set_transform (poly, &m);
	CHECK (goc_item_distance (poly, 3., 3., &near) == 0.);
	CHECK (goc_item_distance (poly, 6., 6., &near) != 0.);
	check_bounds (poly, 0., 0., 5., 5., "polygon bounds, scaled");
	cairo_matrix_init_identity (&m);
	goc_item_set_transform (poly, &m);

	/* A tiny dashed polygon is hit-tested on its outline anyway, and the
	 * dash style survives the hit test.  */
	{
		GocItem *tiny = goc_item_new (root, GOC_TYPE_POLYGON, NULL);
		GocPoints *tp = goc_points_new (4);

		tp->points[0].x = 0.; tp->points[0].y = 0.;
		tp->points[1].x = 2.; tp->points[1].y = 0.;
		tp->points[2].x = 2.; tp->points[2].y = 2.;
		tp->points[3].x = 0.; tp->points[3].y = 2.;
		g_object_set (tiny, "points", tp, NULL);
		goc_points_unref (tp);
		set_fill_style (tiny, GO_COLOR_RED);
		style = go_styled_object_get_style (GO_STYLED_OBJECT (tiny));
		style->line.dash_type = GO_LINE_DASH;
		style->line.auto_dash = FALSE;
		goc_item_bounds_changed (tiny);
		CHECK (goc_item_distance (tiny, 1., -1., &near) == 0.);
		CHECK (style->line.dash_type == GO_LINE_DASH);
		CHECK (!style->line.auto_dash);
	}

	/* An empty polygon has no hit area and needs no canvas.  */
	{
		GocItem *empty = goc_item_new (root, GOC_TYPE_POLYGON, NULL);
		CHECK (goc_item_distance (empty, 1., 1., &near) == 20.);
	}

	g_object_unref (root);
}

/* ------------------------------------------------------------------------- */

static void
test_text (void)
{
	GocGroup *root = new_group (NULL);
	GocText *text;
	GocItem *fresh;
	GocItem *near = NULL;
	cairo_surface_t *surface;
	cairo_t *cr;
	double x0, y0, x1, y1, w, h;

	/* Empty text: inverted bounds, not a box at the origin.  */
	{
		GocItem *empty = goc_item_new (root, GOC_TYPE_TEXT, NULL);
		goc_item_get_bounds (empty, &x0, &y0, &x1, &y1);
		CHECK (x0 == G_MAXDOUBLE && y0 == G_MAXDOUBLE);
		CHECK (x1 == -G_MAXDOUBLE && y1 == -G_MAXDOUBLE);
	}

	text = GOC_TEXT (goc_item_new (root, GOC_TYPE_TEXT,
				       "text", "MMMMMM", "x", 10., "y", 20.,
				       "anchor", GO_ANCHOR_NORTH_WEST, NULL));
	goc_item_get_bounds (GOC_ITEM (text), &x0, &y0, &x1, &y1);
	CHECK (x0 == 10. && y0 == 20.);
	CHECK (x1 == 10. + text->w && y1 == 20. + text->h);

	/* Hit testing covers the box.  */
	CHECK (goc_item_distance (GOC_ITEM (text), 10. + text->w / 2,
				  20. + text->h / 2, &near) == 0.);
	CHECK (goc_item_distance (GOC_ITEM (text), 60. + text->w,
				  20., &near) != 0.);

	/* Drawing measures the layout even without a prior bounds pass.  */
	fresh = goc_item_new (root, GOC_TYPE_TEXT,
			      "text", "MMMMMM", "x", 10., "y", 20.,
			      "anchor", GO_ANCHOR_NORTH_WEST, NULL);
	surface = cairo_recording_surface_create (CAIRO_CONTENT_COLOR_ALPHA, NULL);
	cr = cairo_create (surface);
	goc_item_draw (fresh, cr);
	cairo_recording_surface_ink_extents (surface, &x0, &y0, &w, &h);
	cairo_destroy (cr);
	cairo_surface_destroy (surface);
	CHECK (w > 0. && h > 0.);
	CHECK (x0 >= 9. && y0 >= 19.);
	CHECK (x0 + w <= 10. + text->w + 1. && y0 + h <= 20. + text->h + 1.);

	/* A first draw of a centre-anchored layout must measure it, or the
	 * anchor offset would be wrong.  */
	fresh = goc_item_new (root, GOC_TYPE_TEXT,
			      "text", "MMMMMM", "x", 10., "y", 20.,
			      "anchor", GO_ANCHOR_CENTER, NULL);
	surface = cairo_recording_surface_create (CAIRO_CONTENT_COLOR_ALPHA, NULL);
	cr = cairo_create (surface);
	goc_item_draw (fresh, cr);
	cairo_recording_surface_ink_extents (surface, &x0, &y0, &w, &h);
	cairo_destroy (cr);
	cairo_surface_destroy (surface);
	CHECK (w > 0. && h > 0.);
	CHECK (x0 >= 10. - text->w / 2. - 1. && x0 + w <= 10. + text->w / 2. + 1.);

	/* Aliased and self text sets must not read freed memory.  */
	{
		GValue v = G_VALUE_INIT;
		char *saved = g_strdup (text->text);

		g_value_init (&v, G_TYPE_STRING);
		g_value_set_static_string (&v, text->text);
		g_object_set_property (G_OBJECT (text), "text", &v);
		g_value_unset (&v);
		CHECK (g_strcmp0 (text->text, saved) == 0);
		g_free (saved);
	}
	goc_item_copy (GOC_ITEM (text), GOC_ITEM (text));
	CHECK (g_strcmp0 (text->text, "MMMMMM") == 0);

	g_object_unref (root);
}

/* ------------------------------------------------------------------------- */

static void
test_pixbuf (void)
{
	GocGroup *root = new_group (NULL);
	GocItem *item, *dup;
	GocPixbuf *pixbuf;
	GdkPixbuf *image;
	GocItem *near = NULL;
	cairo_matrix_t m;
	double x0, y0, x1, y1;

	image = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, 2, 1);

	item = goc_item_new (root, GOC_TYPE_PIXBUF,
			     "pixbuf", image, "x", 10., "y", 20., NULL);
	pixbuf = GOC_PIXBUF (item);

	/* The default size (0) means nothing is drawn or hit.  */
	goc_item_get_bounds (item, &x0, &y0, &x1, &y1);
	CHECK (x0 == G_MAXDOUBLE && y0 == G_MAXDOUBLE);
	CHECK (x1 == -G_MAXDOUBLE && y1 == -G_MAXDOUBLE);
	CHECK (goc_item_distance (item, 10., 20., &near) != 0.);

	/* Explicit size.  */
	g_object_set (item, "width", 30., "height", 40., NULL);
	check_bounds (item, 10., 20., 40., 60., "pixbuf bounds");
	CHECK (goc_item_distance (item, 25., 40., &near) == 0.);
	CHECK (goc_item_distance (item, 100., 100., &near) != 0.);

	/* -1 asks for the native size.  */
	g_object_set (item, "width", -1., "height", -1., NULL);
	check_bounds (item, 10., 20., 12., 21., "pixbuf native bounds");

	/* Item transforms apply to the hit region.  */
	g_object_set (item, "width", 30., "height", 40., NULL);
	cairo_matrix_init_scale (&m, 0.5, 0.5);
	goc_item_set_transform (item, &m);
	check_bounds (item, 5., 10., 20., 30., "pixbuf scaled bounds");
	CHECK (goc_item_distance (item, 8., 15., &near) == 0.);
	CHECK (goc_item_distance (item, 35., 55., &near) != 0.);
	cairo_matrix_init_identity (&m);
	goc_item_set_transform (item, &m);

	/* Copying keeps the image and the geometry.  */
	dup = goc_item_duplicate (item, root);
	CHECK (dup != NULL);
	if (dup != NULL) {
		CHECK (GOC_PIXBUF (dup)->pixbuf == pixbuf->pixbuf);
		CHECK (GOC_PIXBUF (dup)->x == 10. && GOC_PIXBUF (dup)->y == 20.);
		CHECK (GOC_PIXBUF (dup)->width == 30. && GOC_PIXBUF (dup)->height == 40.);
		goc_item_get_bounds (dup, &x0, &y0, &x1, &y1);
		CHECK (x0 == 10. && y0 == 20. && x1 == 40. && y1 == 60.);
	}

	g_object_unref (image);
	g_object_unref (root);
}

/* ------------------------------------------------------------------------- */

static void
test_image (void)
{
	GocGroup *root = new_group (NULL);
	GocItem *item, *dup;
	GocImage *img;
	GOImage *goimg;
	GdkPixbuf *pixbuf;
	GocItem *near = NULL;
	cairo_matrix_t m;
	double x0, y0, x1, y1;

	pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, 8, 8);
	goimg = go_pixbuf_new_from_pixbuf (pixbuf);
	g_object_unref (pixbuf);

	/* No image: nothing to draw.  */
	item = goc_item_new (root, GOC_TYPE_IMAGE, "x", 10., "y", 20., NULL);
	goc_item_get_bounds (item, &x0, &y0, &x1, &y1);
	CHECK (x0 == G_MAXDOUBLE && y0 == G_MAXDOUBLE);
	CHECK (x1 == -G_MAXDOUBLE && y1 == -G_MAXDOUBLE);
	CHECK (goc_item_distance (item, 10., 20., &near) != 0.);

	/* Setting the image draws at the native size by default.  */
	g_object_set (item, "image", goimg, NULL);
	img = GOC_IMAGE (item);
	CHECK (img->width == -1. && img->height == -1.);
	check_bounds (item, 10., 20., 18., 28., "image native bounds");
	CHECK (goc_item_distance (item, 14., 24., &near) == 0.);
	CHECK (goc_item_distance (item, 100., 100., &near) != 0.);

	/* Explicit size; 0 means nothing.  */
	g_object_set (item, "width", 30., "height", 40., NULL);
	check_bounds (item, 10., 20., 40., 60., "image explicit bounds");
	CHECK (goc_item_distance (item, 25., 40., &near) == 0.);
	g_object_set (item, "width", 0., NULL);
	goc_item_get_bounds (item, &x0, &y0, &x1, &y1);
	CHECK (x0 == G_MAXDOUBLE && y0 == G_MAXDOUBLE);
	CHECK (goc_item_distance (item, 25., 40., &near) != 0.);

	/* Cropping shrinks the native size.  */
	g_object_set (item, "width", -1., "height", -1.,
		      "crop-left", 0.25, "crop-top", 0.25, NULL);
	check_bounds (item, 10., 20., 16., 26., "image cropped bounds");
	CHECK (goc_item_distance (item, 12., 22., &near) == 0.);
	CHECK (goc_item_distance (item, 17., 25., &near) != 0.);
	g_object_set (item, "crop-left", 0., "crop-top", 0., NULL);

	/* Rotation is part of the item geometry now.  */
	g_object_set (item, "width", 10., "height", 20., "rotation", G_PI / 2., NULL);
	check_bounds (item, -10., 20., 10., 30., "image rotated bounds");
	CHECK (goc_item_distance (item, 0., 25., &near) == 0.);
	CHECK (goc_item_distance (item, 15., 30., &near) != 0.);

	/* Item transforms apply, too.  */
	cairo_matrix_init_scale (&m, 0.5, 0.5);
	goc_item_set_transform (item, &m);
	check_bounds (item, -5., 10., 5., 15., "image scaled+rotated bounds");
	CHECK (goc_item_distance (item, 0., 12.5, &near) == 0.);
	CHECK (goc_item_distance (item, 20., 40., &near) != 0.);
	cairo_matrix_init_identity (&m);
	goc_item_set_transform (item, &m);

	/* Copying keeps the image, the geometry and the rotation.  */
	dup = goc_item_duplicate (item, root);
	CHECK (dup != NULL);
	if (dup != NULL) {
		CHECK (GOC_IMAGE (dup)->image == img->image);
		CHECK (GOC_IMAGE (dup)->x == 10. && GOC_IMAGE (dup)->y == 20.);
		CHECK (GOC_IMAGE (dup)->width == 10. && GOC_IMAGE (dup)->height == 20.);
		CHECK (GOC_IMAGE (dup)->rotation == G_PI / 2.);
		check_bounds (dup, -10., 20., 10., 30., "image copy bounds");
	}

	g_object_unref (goimg);
	g_object_unref (root);
}

/* ------------------------------------------------------------------------- */

static void
test_path (void)
{
	GocGroup *root = new_group (NULL);
	GocItem *item;
	GOPath *path, *got = NULL;
	GocItem *near = NULL;
	double x0, y0, x1, y1;

	item = goc_item_new (root, GOC_TYPE_PATH, NULL);

	/* A fresh item has no path; reading the property must return NULL
	 * rather than a copy of the member address, and the bounds must be
	 * empty.  */
	g_object_get (item, "path", &got, NULL);
	CHECK (got == NULL);
	goc_item_get_bounds (item, &x0, &y0, &x1, &y1);
	CHECK (x0 == G_MAXDOUBLE && y0 == G_MAXDOUBLE);
	CHECK (x1 == -G_MAXDOUBLE && y1 == -G_MAXDOUBLE);
	CHECK (goc_item_distance (item, 0., 0., &near) != 0.);

	path = go_path_new ();
	go_path_move_to (path, 1., 2.);
	go_path_line_to (path, 11., 2.);
	go_path_line_to (path, 11., 12.);
	go_path_close (path);
	g_object_set (item, "path", path, NULL);

	/* The getter returns a reference to the same path.  */
	got = NULL;
	g_object_get (item, "path", &got, NULL);
	CHECK (got == path);
	CHECK (GOC_PATH (item)->path == path);
	if (got != NULL)
		g_boxed_free (GO_TYPE_PATH, got);

	/* The stored path actually drives the item bounds.  */
	set_line_style (item, 4., CAIRO_LINE_CAP_BUTT);
	goc_item_get_bounds (item, &x0, &y0, &x1, &y1);
	CHECK (x0 <= 1. && y0 <= 2. && x1 >= 11. && y1 >= 12.);
	CHECK (x0 >= -3. && y0 >= -3. && x1 <= 14. && y1 <= 15.);

	/* Copying onto an item that already owns a path must release the
	 * old one instead of leaking it.  */
	{
		GocItem *dst = goc_item_new (root, GOC_TYPE_PATH, NULL);
		GOPath *p = go_path_new ();
		go_path_move_to (p, 0., 0.);
		go_path_line_to (p, 1., 1.);
		g_object_set (dst, "path", p, NULL);
		go_path_free (p);

		goc_item_copy (dst, item);
		CHECK (GOC_PATH (dst)->path != NULL);
		CHECK (GOC_PATH (dst)->path != GOC_PATH (item)->path);
		goc_item_get_bounds (dst, &x0, &y0, &x1, &y1);
		CHECK (x0 <= 1. && y0 <= 2. && x1 >= 11. && y1 >= 12.);
	}

	/* An open path whose line is invisible draws nothing, so it must not
	 * keep the bounds of a default stroke.  */
	{
		GocItem *open = goc_item_new (root, GOC_TYPE_PATH,
					      "x", 10., "y", 10., NULL);
		GOPath *p = go_path_new ();
		go_path_move_to (p, 0., 0.);
		go_path_line_to (p, 40., 0.);
		g_object_set (open, "path", p, NULL);
		go_path_free (p);
		set_fill_style (open, GO_COLOR_BLUE);
		goc_item_get_bounds (open, &x0, &y0, &x1, &y1);
		CHECK (x0 == G_MAXDOUBLE && y0 == G_MAXDOUBLE);
		CHECK (x1 == -G_MAXDOUBLE && y1 == -G_MAXDOUBLE);
		CHECK (goc_item_distance (open, 10., 10., &near) != 0.);
	}

	/* A closed fill-only path uses the fill extents.  */
	{
		GocItem *filled = goc_item_new (root, GOC_TYPE_PATH, NULL);
		GOPath *p = go_path_new ();
		go_path_rectangle (p, 0., 0., 20., 10.);
		g_object_set (filled, "path", p, "closed", TRUE, NULL);
		go_path_free (p);
		set_fill_style (filled, GO_COLOR_BLUE);
		check_bounds (filled, 0., 0., 20., 10., "path fill-only bounds");
		CHECK (goc_item_distance (filled, 10., 5., &near) == 0.);
		CHECK (goc_item_distance (filled, 30., 5., &near) != 0.);
	}

	/* With even/odd, the inner contour of a doubly wound path is a hole;
	 * hit testing must use the same rule as drawing.  */
	{
		GocItem *donut = goc_item_new (root, GOC_TYPE_PATH, NULL);
		GOPath *p = go_path_new ();
		cairo_surface_t *surface;
		cairo_t *cr;

		go_path_rectangle (p, 0., 0., 20., 20.);
		go_path_rectangle (p, 5., 5., 10., 10.);
		g_object_set (donut, "path", p, "closed", TRUE, NULL);
		go_path_free (p);
		set_fill_style (donut, GO_COLOR_BLUE);

		/* Winding fills the inner square.  */
		CHECK (goc_item_distance (donut, 10., 10., &near) == 0.);

		g_object_set (donut, "fill-rule", TRUE, NULL);
		CHECK (goc_item_distance (donut, 10., 10., &near) != 0.);
		CHECK (goc_item_distance (donut, 2., 10., &near) == 0.);
		check_bounds (donut, 0., 0., 20., 20., "path even-odd bounds");

		surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 20, 20);
		cr = cairo_create (surface);
		cairo_set_source_rgb (cr, 1., 1., 1.);
		cairo_paint (cr);
		goc_item_draw (donut, cr);
		check_pixel_white (surface, 10, 10, "path even-odd hole");
		check_pixel (surface, 2, 10, 0, 0, 255, 60, "path even-odd ring");
		cairo_destroy (cr);
		cairo_surface_destroy (surface);
	}

	/* Clearing the property must drop the path without warnings.  */
	g_object_set (item, "path", NULL, NULL);
	CHECK (GOC_PATH (item)->path == NULL);
	got = NULL;
	g_object_get (item, "path", &got, NULL);
	CHECK (got == NULL);
	goc_item_get_bounds (item, &x0, &y0, &x1, &y1);
	CHECK (x0 == G_MAXDOUBLE && y0 == G_MAXDOUBLE);
	CHECK (x1 == -G_MAXDOUBLE && y1 == -G_MAXDOUBLE);

	/* Our own reference; the item holds one until it is destroyed.  */
	go_path_free (path);
	g_object_unref (root);
}

#ifdef GOFFICE_WITH_GTK
/* GOComponentSnapshot lives in goffice/component/go-component.c and is the
 * only concrete component available here.  Without snapshot data it renders
 * nothing, which is exactly what this test wants.  */
GType go_component_snapshot_get_type (void);

/* A 1x1 opaque red PNG, used for rendering tests.  */
static const unsigned char red_png[] = {
	0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
	0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
	0x08, 0x02, 0x00, 0x00, 0x00, 0x90, 0x77, 0x53, 0xde, 0x00, 0x00, 0x00,
	0x0c, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x63, 0xf8, 0xcf, 0xc0, 0x00,
	0x00, 0x03, 0x01, 0x01, 0x00, 0xc9, 0xfe, 0x92, 0xef, 0x00, 0x00, 0x00,
	0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82,
};

static void
test_component (void)
{
	GocGroup *root = new_group (NULL);
	GocItem *item;
	GocItem *near = NULL;
	GOComponent *component;
	cairo_surface_t *surface;
	cairo_t *cr;
	cairo_matrix_t m;
	double x0, y0, x1, y1;

	component = g_object_new (go_component_snapshot_get_type (), NULL);
	CHECK (GO_IS_COMPONENT (component));

	item = goc_item_new (root, GOC_TYPE_COMPONENT, NULL);
	goc_item_set (item, "object", component, "x", 10., "y", 20., NULL);
	goc_item_set (item, "width", 30., "height", 40., NULL);
	g_object_unref (component);

	/* Without a canvas the item must still produce sane bounds in canvas
	 * units instead of dereferencing a NULL canvas.  */
	goc_item_get_bounds (item, &x0, &y0, &x1, &y1);
	CHECK (x0 == 10. && y0 == 20. && x1 == 40. && y1 == 60.);

	/* Drawing must not crash either.  */
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 60, 70);
	cr = cairo_create (surface);
	cairo_set_source_rgb (cr, 1., 1., 1.);
	cairo_paint (cr);
	goc_item_draw (item, cr);
	check_pixel_white (surface, 25, 30, "component draw without canvas");
	cairo_destroy (cr);
	cairo_surface_destroy (surface);

	/* The item transform is part of the geometry and hit testing.  */
	cairo_matrix_init_scale (&m, 2., 2.);
	goc_item_set_transform (item, &m);
	goc_item_get_bounds (item, &x0, &y0, &x1, &y1);
	CHECK (x0 == 20. && y0 == 40. && x1 == 80. && y1 == 120.);
	CHECK (goc_item_distance (item, 70., 100., &near) == 0.);
	CHECK (goc_item_distance (item, 15., 25., &near) != 0.);
	cairo_matrix_init_identity (&m);
	goc_item_set_transform (item, &m);
	goc_item_get_bounds (item, &x0, &y0, &x1, &y1);
	CHECK (x0 == 10. && y0 == 20. && x1 == 40. && y1 == 60.);

	/* Components have no copy method, so they cannot be duplicated.  */
	CHECK (goc_item_duplicate (item, root) == NULL);
	CHECK (goc_item_clone (item) == NULL);

	g_object_unref (root);
}

/* Drawing a component: canvas scale, scrolling and RTL all belong in the
 * group transform; bounds and hit testing do not depend on the direction.  */
static void
test_component_render (void)
{
	GocCanvas *canvas = new_canvas ();
	GocGroup *root = goc_canvas_get_root (canvas);
	GocItem *item;
	GOComponent *component;
	cairo_surface_t *surface;
	cairo_t *cr;
	double x0, y0, x1, y1;

	canvas->width = 200;
	canvas->height = 150;

	component = g_object_new (go_component_snapshot_get_type (), NULL);
	component->snapshot_type = GO_SNAPSHOT_PNG;
	component->snapshot_data = (void *) red_png;
	component->snapshot_length = sizeof (red_png);

	item = goc_item_new (root, GOC_TYPE_COMPONENT, NULL);
	goc_item_set (item, "object", component, "x", 10., "y", 20., NULL);
	goc_item_set (item, "width", 20., "height", 30., NULL);
	g_object_unref (component);

	goc_canvas_set_pixels_per_unit (canvas, 2.);
	goc_canvas_scroll_to (canvas, 10., 0.);

	/* LTR: the content is anchored at device (0,40) and stays inside the
	 * 40x60 device box.  */
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 200, 150);
	cr = cairo_create (surface);
	cairo_set_source_rgb (cr, 1., 1., 1.);
	cairo_paint (cr);
	goc_item_draw (item, cr);
	{
		int bx0, by0, bx1, by1;

		CHECK (red_bbox (surface, &bx0, &by0, &bx1, &by1));
		CHECK (bx0 >= 0 && by0 >= 40 && bx1 <= 39 && by1 <= 99);
		CHECK (bx0 <= 1 && by0 <= 41 && bx1 >= 10 && by1 >= 50);
	}
	check_pixel_white (surface, 100, 60, "component draw, outside");
	check_pixel_white (surface, 20, 10, "component draw, above");
	cairo_destroy (cr);
	cairo_surface_destroy (surface);

	/* Bounds and hit testing are direction and scroll independent.  */
	goc_canvas_set_direction (canvas, GOC_DIRECTION_RTL);
	goc_item_get_bounds (item, &x0, &y0, &x1, &y1);
	CHECK (x0 == 10. && y0 == 20. && x1 == 30. && y1 == 50.);
	CHECK (goc_canvas_get_item_at (canvas, 15., 25.) == item);
	CHECK (goc_canvas_get_item_at (canvas, 90., 25.) == NULL);

	/* RTL: anchored at the mirrored right edge, device (160,40); the
	 * contents themselves are not mirrored.  */
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 200, 150);
	cr = cairo_create (surface);
	cairo_set_source_rgb (cr, 1., 1., 1.);
	cairo_paint (cr);
	goc_item_draw (item, cr);
	{
		int bx0, by0, bx1, by1;

		CHECK (red_bbox (surface, &bx0, &by0, &bx1, &by1));
		CHECK (bx0 >= 160 && by0 >= 40 && bx1 <= 199 && by1 <= 99);
		CHECK (bx0 <= 161 && by0 <= 41 && bx1 >= 170 && by1 >= 50);
	}
	check_pixel_white (surface, 100, 60, "component draw, RTL outside");
	cairo_destroy (cr);
	cairo_surface_destroy (surface);

	g_object_unref (canvas);
}

/* Text is positioned in the mirror of its LTR spot under RTL (glyphs stay
 * upright), the box is direction independent, and clipping only happens
 * when the "clip" property asks for it.  */
static void
test_text_render (void)
{
	GocCanvas *canvas = new_canvas ();
	GocGroup *root = goc_canvas_get_root (canvas);
	GocText *text;
	cairo_surface_t *surface;
	cairo_t *cr;
	double x0, y0, iw, ih, w, h;

	canvas->width = 200;
	canvas->height = 50;

	text = GOC_TEXT (goc_item_new (root, GOC_TYPE_TEXT,
				       "text", "MMMMMMMM", "x", 10., "y", 20.,
				       "anchor", GO_ANCHOR_CENTER, NULL));
	goc_item_get_bounds (GOC_ITEM (text), &x0, &y0, &iw, &ih);
	w = text->w;
	h = text->h;

	/* LTR: the box is centred on (10,20).  */
	surface = cairo_recording_surface_create (CAIRO_CONTENT_COLOR_ALPHA, NULL);
	cr = cairo_create (surface);
	goc_item_draw (GOC_ITEM (text), cr);
	cairo_recording_surface_ink_extents (surface, &x0, &y0, &iw, &ih);
	cairo_destroy (cr);
	cairo_surface_destroy (surface);
	CHECK (iw > 0. && ih > 0.);
	CHECK (x0 >= 10. - w / 2. - 1. && x0 + iw <= 10. + w / 2. + 1.);
	CHECK (y0 >= 20. - h / 2. - 1. && y0 + ih <= 20. + h / 2. + 1.);

	/* Bounds are direction independent.  */
	goc_canvas_set_direction (canvas, GOC_DIRECTION_RTL);
	goc_item_bounds_changed (GOC_ITEM (text));
	goc_item_get_bounds (GOC_ITEM (text), &x0, &y0, &iw, &ih);
	CHECK (x0 == 10. - w / 2. && y0 == 20. - h / 2.);
	CHECK (iw == 10. + w / 2. && ih == 20. + h / 2.);

	/* RTL: the box lands on the mirror of its LTR position.  */
	surface = cairo_recording_surface_create (CAIRO_CONTENT_COLOR_ALPHA, NULL);
	cr = cairo_create (surface);
	goc_item_draw (GOC_ITEM (text), cr);
	cairo_recording_surface_ink_extents (surface, &x0, &y0, &iw, &ih);
	cairo_destroy (cr);
	cairo_surface_destroy (surface);
	CHECK (iw > 0. && ih > 0.);
	CHECK (x0 >= 200. - 10. - w / 2. - 1. && x0 + iw <= 200. - 10. + w / 2. + 1.);
	CHECK (y0 >= 20. - h / 2. - 1. && y0 + ih <= 20. + h / 2. + 1.);

	/* Clipping happens only when "clip" is set.  The anchor offset is
	 * computed from the clipped size, so the box starts at 10-w/4.  */
	{
		double clipped_w;

		goc_canvas_set_direction (canvas, GOC_DIRECTION_LTR);
		g_object_set (text, "clip-width", w / 2., "clip", TRUE, NULL);
		surface = cairo_recording_surface_create (CAIRO_CONTENT_COLOR_ALPHA, NULL);
		cr = cairo_create (surface);
		goc_item_draw (GOC_ITEM (text), cr);
		cairo_recording_surface_ink_extents (surface, &x0, &y0, &iw, &ih);
		cairo_destroy (cr);
		cairo_surface_destroy (surface);
		CHECK (iw <= w / 2. + 2.);
		CHECK (x0 >= 10. - w / 4. - 1. && x0 + iw <= 10. + w / 4. + 1.);
		clipped_w = iw;

		g_object_set (text, "clip", FALSE, NULL);
		surface = cairo_recording_surface_create (CAIRO_CONTENT_COLOR_ALPHA, NULL);
		cr = cairo_create (surface);
		goc_item_draw (GOC_ITEM (text), cr);
		cairo_recording_surface_ink_extents (surface, &x0, &y0, &iw, &ih);
		cairo_destroy (cr);
		cairo_surface_destroy (surface);
		CHECK (iw > clipped_w && x0 + iw > 10. + w / 4. + 1.);
	}

	g_object_unref (canvas);
}

/* RTL anchors a native-sized pixbuf on the mirrored right edge.  */
static void
test_pixbuf_render (void)
{
	GocCanvas *canvas = new_canvas ();
	GocGroup *root = goc_canvas_get_root (canvas);
	GocItem *item;
	GdkPixbuf *image;
	cairo_surface_t *surface;
	cairo_t *cr;

	canvas->width = 200;
	canvas->height = 50;

	image = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, 8, 8);
	gdk_pixbuf_fill (image, 0xff0000ff);

	item = goc_item_new (root, GOC_TYPE_PIXBUF,
			     "pixbuf", image, "x", 10., "y", 20.,
			     "width", -1., "height", -1., NULL);
	g_object_unref (image);

	goc_canvas_set_pixels_per_unit (canvas, 2.);
	goc_canvas_scroll_to (canvas, 10., 0.);

	/* LTR: device (0,40) to (16,56).  */
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 200, 60);
	cr = cairo_create (surface);
	cairo_set_source_rgb (cr, 1., 1., 1.);
	cairo_paint (cr);
	goc_item_draw (item, cr);
	check_pixel (surface, 8, 48, 255, 0, 0, 60, "pixbuf draw");
	check_pixel_white (surface, 150, 48, "pixbuf draw, outside");
	cairo_destroy (cr);
	cairo_surface_destroy (surface);

	/* RTL: the native size mirrors to device (184,40) to (200,56).  */
	goc_canvas_set_direction (canvas, GOC_DIRECTION_RTL);
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 200, 60);
	cr = cairo_create (surface);
	cairo_set_source_rgb (cr, 1., 1., 1.);
	cairo_paint (cr);
	goc_item_draw (item, cr);
	check_pixel (surface, 192, 48, 255, 0, 0, 60, "pixbuf draw, RTL");
	check_pixel_white (surface, 2, 48, "pixbuf draw, RTL outside");
	cairo_destroy (cr);
	cairo_surface_destroy (surface);

	g_object_unref (canvas);
}

/* RTL arcs mirror their arrow geometry; hit testing must follow the drawn
 * side.  Needs a canvas for the RTL direction.  */
static void
test_arc_render (void)
{
	GocCanvas *canvas = new_canvas ();
	GocGroup *root = goc_canvas_get_root (canvas);
	GocItem *item;
	GOArrow arrow;
	cairo_surface_t *surface;
	cairo_t *cr;
	GocItem *near = NULL;
	double x0, y0, x1, y1;

	canvas->width = 200;
	canvas->height = 100;
	goc_canvas_set_direction (canvas, GOC_DIRECTION_RTL);

	go_arrow_init_kite (&arrow, 8., 6., 6.);
	item = goc_item_new (root, GOC_TYPE_ARC,
			     "xc", 100., "yc", 50., "xr", 40., "yr", 40.,
			     "ang1", 0., "ang2", G_PI, "type", 0,
			     "start-arrow", &arrow, NULL);
	set_line_style (item, 2., CAIRO_LINE_CAP_BUTT);

	/* The start arrow mirrors to the left of the start endpoint; the
	 * bounds and the hit region must cover that side.  */
	goc_item_get_bounds (item, &x0, &y0, &x1, &y1);
	CHECK (x0 > 50. && x0 < 52.);
	CHECK (x1 > 140. && x1 < 142.);
	CHECK (goc_item_distance (item, 54., 45., &near) == 0.);
	CHECK (goc_item_distance (item, 146., 45., &near) != 0.);

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 200, 100);
	cr = cairo_create (surface);
	cairo_set_source_rgb (cr, 1., 1., 1.);
	cairo_paint (cr);
	goc_item_draw (item, cr);
	check_pixel_dark (surface, 54, 45, "arc RTL start arrow");
	check_pixel_white (surface, 146, 45, "arc RTL arrow, mirror side");
	cairo_destroy (cr);
	cairo_surface_destroy (surface);

	g_object_unref (canvas);
}

/* RTL paths mirror their geometry; bounds and hit testing must follow the
 * drawn side.  Needs a canvas for the RTL direction.  */static void
test_path_render (void)
{
	GocCanvas *canvas = new_canvas ();
	GocGroup *root = goc_canvas_get_root (canvas);
	GocItem *item;
	GOPath *path;
	cairo_surface_t *surface;
	cairo_t *cr;
	GocItem *near = NULL;
	double x0, y0, x1, y1;

	canvas->width = 200;
	canvas->height = 100;
	goc_canvas_set_direction (canvas, GOC_DIRECTION_RTL);

	path = go_path_new ();
	go_path_move_to (path, 0., 0.);
	go_path_line_to (path, 10., 0.);
	item = goc_item_new (root, GOC_TYPE_PATH,
			     "x", 100., "y", 30., "rotation", G_PI / 2.,
			     "path", path, NULL);
	go_path_free (path);
	set_line_style (item, 4., CAIRO_LINE_CAP_BUTT);

	/* The rotated segment is drawn upwards from the anchor; the bounds
	 * and the hit region must cover that side.  */
	goc_item_get_bounds (item, &x0, &y0, &x1, &y1);
	CHECK (y0 > 19. && y0 < 21.);
	CHECK (y1 > 29. && y1 < 31.);
	CHECK (goc_item_distance (item, 100., 21., &near) == 0.);
	CHECK (goc_item_distance (item, 100., 39., &near) != 0.);

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 200, 100);
	cr = cairo_create (surface);
	cairo_set_source_rgb (cr, 1., 1., 1.);
	cairo_paint (cr);
	goc_item_draw (item, cr);
	check_pixel_dark (surface, 100, 25, "path RTL draw");
	check_pixel_white (surface, 100, 35, "path RTL draw, mirror side");
	cairo_destroy (cr);
	cairo_surface_destroy (surface);

	g_object_unref (canvas);
}

/* The line hit tolerance and hairline width follow the canvas scale.
 * Needs a canvas for the pixels per unit.  */
static void
test_line_render (void)
{
	GocCanvas *canvas = new_canvas ();
	GocGroup *root = goc_canvas_get_root (canvas);
	GocItem *item;
	GocItem *near = NULL;

	canvas->width = 200;
	canvas->height = 100;

	item = goc_item_new (root, GOC_TYPE_LINE,
			     "x0", 0., "y0", 50., "x1", 100., "y1", 50., NULL);
	set_line_style (item, 0., CAIRO_LINE_CAP_BUTT);

	/* At scale 2 a hairline is half an item unit wide and the 5 device
	 * pixel tolerance is 2.5 item units.  */
	goc_canvas_set_pixels_per_unit (canvas, 2.);
	check_bounds (item, -0.25, 50.25, 100.25, 50.75, "line bounds at ppu 2");
	CHECK (goc_item_distance (item, 50., 51., &near) == 0.);
	CHECK (goc_item_distance (item, 50., 52., &near) != 0.);

	g_object_unref (canvas);
}

/* Tooltips on a graph without a renderer must not touch a NULL renderer or
 * view.  Needs a realized canvas for the bin window.  */
static void
test_graph_tooltip (void)
{
	GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	GocCanvas *canvas = new_canvas ();
	GocGroup *root = goc_canvas_get_root (canvas);
	GocItem *item;

	gtk_window_set_default_size (GTK_WINDOW (window), 200, 100);
	gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (canvas));
	gtk_widget_show_all (window);
	while (gtk_events_pending ())
		gtk_main_iteration ();

	item = goc_item_new (root, GOC_TYPE_GRAPH,
			     "width", 100., "height", 50., NULL);
	CHECK (GOC_GRAPH (item)->renderer == NULL);

	GOC_ITEM_GET_CLASS (item)->motion (item, 10., 10.);
	g_usleep (200000);
	while (g_main_context_iteration (NULL, FALSE))
		;

	gtk_widget_destroy (window);
	g_object_unref (canvas);
}

/* The widget item forwards events to its parent item, so use a group
 * subclass to record the coordinates that arrive.  */
typedef struct {
	GocGroup base;
	double last_x, last_y;
	int last_button;
} RecordGroup;

typedef GocGroupClass RecordGroupClass;

static void record_group_class_init (RecordGroupClass *klass);
static void record_group_init (RecordGroup *group);
GType record_group_get_type (void);
G_DEFINE_TYPE (RecordGroup, record_group, GOC_TYPE_GROUP)

static gboolean
record_group_enter (GocItem *item, double x, double y)
{
	RecordGroup *group = (RecordGroup *) item;

	group->last_x = x;
	group->last_y = y;
	return TRUE;
}

static gboolean
record_group_button (GocItem *item, int button, double x, double y)
{
	RecordGroup *group = (RecordGroup *) item;

	group->last_button = button;
	group->last_x = x;
	group->last_y = y;
	return TRUE;
}

static void
record_group_class_init (RecordGroupClass *klass)
{
	GocItemClass *item_class = (GocItemClass *) klass;

	item_class->enter_notify = record_group_enter;
	item_class->button_pressed = record_group_button;
}

static void
record_group_init (G_GNUC_UNUSED RecordGroup *group)
{
}

/* Exercises the widget item's forwarding of embedded-widget events, which
 * must produce canvas coordinates for any ppu, scroll, group offset, widget
 * scale and canvas direction.  */
static void
test_widget (void)
{
	GocCanvas *canvas = new_canvas ();
	GocGroup *root = goc_canvas_get_root (canvas);
	RecordGroup *record;
	GocWidget *item;
	GtkWidget *embedded;
	GdkEvent *event;
	gboolean ret;

	/* Give the unrealized canvas a size so the widget gets an offscreen
	 * box; the embedded widgets work without being realized.  */
	canvas->width = 400;
	canvas->height = 300;

	record = (RecordGroup *) goc_item_new (root, record_group_get_type (),
					       "x", 5., "y", 7., NULL);
	goc_canvas_set_pixels_per_unit (canvas, 2.);
	goc_canvas_scroll_to (canvas, 10., 20.);

	/* Big enough that the widget is not scaled to fit.  */
	embedded = gtk_button_new ();
	item = GOC_WIDGET (goc_item_new (GOC_GROUP (record), GOC_TYPE_WIDGET,
					 "x", 3., "y", 4.,
					 "width", 500., "height", 400., NULL));
	goc_item_set (GOC_ITEM (item), "widget", embedded, NULL);
	CHECK (item->ofbox != NULL);
	CHECK (item->scale == 1.);

	/* LTR: canvas coordinates are group offset + item position + event.  */
	event = gdk_event_new (GDK_ENTER_NOTIFY);
	event->crossing.x = 1.5;
	event->crossing.y = 2.5;
	ret = FALSE;
	g_signal_emit_by_name (embedded, "enter-notify-event", event, &ret);
	check_near (record->last_x, 5. + 3. + 1.5, 1e-12, "widget enter x");
	check_near (record->last_y, 7. + 4. + 2.5, 1e-12, "widget enter y");
	gdk_event_free (event);

	event = gdk_event_new (GDK_BUTTON_PRESS);
	event->button.button = 3;
	event->button.x = 0.5;
	event->button.y = 1.5;
	ret = FALSE;
	g_signal_emit_by_name (embedded, "button-press-event", event, &ret);
	CHECK (record->last_button == 3);
	check_near (record->last_x, 5. + 3. + 0.5, 1e-12, "widget button x");
	check_near (record->last_y, 7. + 4. + 1.5, 1e-12, "widget button y");
	gdk_event_free (event);

	/* A leave event must clear cur_event again; otherwise the canvas
	 * exposes an event that is no longer valid.  */
	{
		GdkEvent *ev = gdk_event_new (GDK_LEAVE_NOTIFY);

		canvas->last_item = GOC_ITEM (record);
		ret = FALSE;
		g_signal_emit_by_name (canvas, "leave-notify-event", ev, &ret);
		CHECK (canvas->last_item == NULL);
		CHECK (goc_canvas_get_cur_event (canvas) == NULL);
		gdk_event_free (ev);
	}

	/* RTL mirrors the widget inside the canvas.  */
	goc_canvas_set_direction (canvas, GOC_DIRECTION_RTL);
	goc_widget_set_bounds (item, 3., 4., 500., 400.);
	event = gdk_event_new (GDK_ENTER_NOTIFY);
	event->crossing.x = 1.5;
	event->crossing.y = 2.5;
	ret = FALSE;
	g_signal_emit_by_name (embedded, "enter-notify-event", event, &ret);
	check_near (record->last_x, 5. + 3. + 500. - 1.5, 1e-12, "widget enter x, RTL");
	check_near (record->last_y, 7. + 4. + 2.5, 1e-12, "widget enter y, RTL");
	gdk_event_free (event);

	g_object_unref (canvas);
}

/* Duplicating an item on a realized canvas must not keep bounds cached for
 * the duplicate's default geometry.  */
static void
test_duplicate_realized (void)
{
	GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	GocCanvas *canvas = new_canvas ();
	GocGroup *root = goc_canvas_get_root (canvas);
	SimpleItem *item;
	GocItem *dup, *hit;
	double x0, y0, x1, y1;

	gtk_window_set_default_size (GTK_WINDOW (window), 400, 300);
	gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (canvas));
	gtk_widget_show_all (window);
	while (gtk_events_pending ())
		gtk_main_iteration ();
	CHECK (goc_canvas_get_realized (canvas));

	/* The fresh item's default geometry gets cached by goc_item_new().  */
	item = (SimpleItem *) goc_item_new (root, simple_item_get_type (), NULL);
	set_simple_geometry (item, 10., 10., 40., 40.);
	goc_item_get_bounds (GOC_ITEM (item), &x0, &y0, &x1, &y1);
	CHECK (x0 == 10. && y0 == 10. && x1 == 50. && y1 == 50.);

	dup = goc_item_duplicate (GOC_ITEM (item), root);
	CHECK (dup != NULL);
	goc_item_get_bounds (dup, &x0, &y0, &x1, &y1);
	CHECK (x0 == 10. && y0 == 10. && x1 == 50. && y1 == 50.);

	/* `dup' was added last, so it is on top and must be hit.  */
	hit = goc_canvas_get_item_at (canvas, 30., 30.);
	CHECK (hit == dup);

	gtk_widget_destroy (window);
	g_object_unref (canvas);
}
#endif /* GOFFICE_WITH_GTK */

/* ------------------------------------------------------------------------- */

static void
test_graph (void)
{
	GocGroup *root = new_group (NULL);
	GocItem *item;
	GogRenderer *renderer = NULL;
	GogGraph *graph;
	cairo_surface_t *surface;
	cairo_t *cr;
	double x0, y0, x1, y1;

	/* No canvas and no renderer: bounds must be available and drawing
	 * must be a no-op instead of dereferencing a NULL canvas.  */
	item = goc_item_new (root, GOC_TYPE_GRAPH,
			     "width", 100., "height", 50., NULL);
	goc_item_get_bounds (item, &x0, &y0, &x1, &y1);
	CHECK (x0 == 0. && y0 == 0. && x1 == 100. && y1 == 50.);

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 60, 60);
	cr = cairo_create (surface);
	goc_item_draw (item, cr);
	cairo_destroy (cr);
	cairo_surface_destroy (surface);

	/* A NULL "graph" must clear the renderer rather than create one with
	 * a NULL model, which crashed on the first update.  */
	g_object_set (item, "graph", NULL, NULL);
	g_object_get (item, "renderer", &renderer, NULL);
	CHECK (renderer == NULL);
	if (renderer != NULL)
		g_object_unref (renderer);
	goc_item_get_bounds (item, &x0, &y0, &x1, &y1);
	CHECK (x0 == 0. && y0 == 0. && x1 == 100. && y1 == 50.);

	/* A graph with a model can be copied: the copy gets its own model
	 * and renderer.  */
	graph = GOG_GRAPH (g_object_new (GOG_TYPE_GRAPH, NULL));
	{
		GocItem *src, *dup;
		GogGraph *dup_model = NULL;
		GogRenderer *dup_renderer = NULL;

		src = goc_item_new (root, GOC_TYPE_GRAPH,
				    "graph", graph, "x", 5., "y", 6.,
				    "width", 70., "height", 40., NULL);
		dup = goc_item_duplicate (src, root);
		CHECK (dup != NULL);
		if (dup != NULL) {
			CHECK (GOC_GRAPH (dup)->x == 5. && GOC_GRAPH (dup)->y == 6.);
			CHECK (GOC_GRAPH (dup)->w == 70. && GOC_GRAPH (dup)->h == 40.);
			CHECK (GOC_GRAPH (dup)->renderer != NULL);
			CHECK (GOC_GRAPH (dup)->renderer != GOC_GRAPH (src)->renderer);
			g_object_get (dup, "renderer", &dup_renderer, NULL);
			CHECK (dup_renderer != NULL);
			if (dup_renderer != NULL) {
				g_object_get (dup_renderer, "model", &dup_model, NULL);
				CHECK (dup_model != NULL && dup_model != graph);
				if (dup_model != NULL)
					g_object_unref (dup_model);
				g_object_unref (dup_renderer);
			}
			goc_item_get_bounds (dup, &x0, &y0, &x1, &y1);
			CHECK (x0 == 5. && y0 == 6. && x1 == 75. && y1 == 46.);
		}

		/* A group copy keeps the graph, too.  */
		{
			GocGroup *g = new_group (root);
			GocGroup *gdup;
			GocItem *copied;

			goc_item_new (g, GOC_TYPE_GRAPH,
				      "graph", graph, "width", 70., "height", 40., NULL);
			gdup = GOC_GROUP (goc_item_duplicate (GOC_ITEM (g), root));
			CHECK (gdup != NULL);
			if (gdup != NULL) {
				copied = goc_group_get_child (gdup, 0);
				CHECK (copied != NULL && GOC_IS_GRAPH (copied));
				CHECK (copied != NULL && GOC_GRAPH (copied)->renderer != NULL);
			}
		}
	}

	g_object_unref (root);
	g_object_unref (graph);
}

/* ------------------------------------------------------------------------- */

static void
test_circle_ellipse (void)
{
	GocGroup *root = new_group (NULL);
	GocItem *circle, *ellipse, *dup;
	GocItem *near = NULL;

	circle = goc_item_new (root, GOC_TYPE_CIRCLE,
			       "x", 10., "y", 20., "radius", 5., NULL);

	/* The default style strokes with a hairline, hence the half-unit
	 * slack.  */
	check_bounds (circle, 4.5, 14.5, 15.5, 25.5, "circle bounds, default style");

	set_fill_style (circle, GO_COLOR_BLUE);
	check_bounds (circle, 5., 15., 15., 25., "circle bounds, fill only");
	CHECK (goc_item_distance (circle, 15., 20., &near) == 0.);
	CHECK (goc_item_distance (circle, 11., 20., &near) == 0.);
	check_near (goc_item_distance (circle, 18., 20., &near), 3., 1e-12,
		    "circle distance, 3 outside");

	set_line_style (circle, 4., CAIRO_LINE_CAP_BUTT);
	check_bounds (circle, 3., 13., 17., 27., "circle bounds, width 4");
	check_near (goc_item_distance (circle, 20., 20., &near), 3., 1e-12,
		    "circle distance, width 4");

	dup = goc_item_duplicate (circle, root);
	CHECK (GOC_CIRCLE (dup)->x == 10. && GOC_CIRCLE (dup)->y == 20. &&
	       GOC_CIRCLE (dup)->radius == 5.);

	/* The item transform scales the drawn circle; the outline does not
	 * scale with it.  */
	{
		cairo_matrix_t sm;
		cairo_surface_t *surface;
		cairo_t *cr;

		cairo_matrix_init_scale (&sm, 2., 2.);
		goc_item_set_transform (circle, &sm);
		check_bounds (circle, 8., 28., 32., 52., "transformed circle bounds");
		CHECK (goc_item_distance (circle, 30., 40., &near) == 0.);
		check_near (goc_item_distance (circle, 33., 40., &near), .5, 1e-12,
			    "transformed circle distance");

		surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 60, 70);
		cr = cairo_create (surface);
		cairo_set_source_rgb (cr, 1., 1., 1.);
		cairo_paint (cr);
		goc_item_draw (circle, cr);
		check_pixel_dark (surface, 30, 40, "transformed circle draw");
		check_pixel_white (surface, 35, 40, "transformed circle draw outside");
		cairo_destroy (cr);
		cairo_surface_destroy (surface);

		cairo_matrix_init_identity (&sm);
		goc_item_set_transform (circle, &sm);
	}

	ellipse = goc_item_new (root, GOC_TYPE_ELLIPSE,
				"x", 0., "y", 0., "width", 20., "height", 10., NULL);

	set_fill_style (ellipse, GO_COLOR_GREEN);
	check_bounds (ellipse, 0., 0., 20., 10., "ellipse bounds, fill only");
	CHECK (goc_item_distance (ellipse, 10., 5., &near) == 0.);
	CHECK (goc_item_distance (ellipse, 40., 5., &near) == 20.);

	set_line_style (ellipse, 4., CAIRO_LINE_CAP_BUTT);
	check_bounds (ellipse, -2., -2., 22., 12., "ellipse bounds, width 4");

	/* Rotated by 90 degrees the half axes swap, plus the outline.  */
	g_object_set (ellipse, "rotation", M_PI / 2., NULL);
	check_bounds (ellipse, 3., -7., 17., 17., "rotated ellipse bounds");

	/* scale-line-width is part of the copied state, and the C setter
	 * goes through the property machinery.  */
	{
		GocItem *src = goc_item_new (root, GOC_TYPE_ELLIPSE,
					     "x", 0., "y", 0., "width", 20., "height", 10., NULL);
		GocItem *dup;
		cairo_matrix_t sm;

		set_line_style (src, 4., CAIRO_LINE_CAP_BUTT);
		cairo_matrix_init_scale (&sm, 2., 2.);
		goc_item_set_transform (src, &sm);
		check_bounds (src, -4., -4., 44., 24., "scaled ellipse bounds");

		notify_count = 0;
		g_signal_connect (src, "notify::scale-line-width",
				  G_CALLBACK (count_notify), NULL);
		goc_styled_item_set_scale_line_width (GOC_STYLED_ITEM (src), FALSE);
		CHECK (notify_count == 1);
		CHECK (!GOC_STYLED_ITEM (src)->scale_line_width);
		check_bounds (src, -2., -2., 42., 22.,
			      "ellipse bounds, no line scaling");

		dup = goc_item_duplicate (src, root);
		CHECK (dup != NULL);
		CHECK (dup != NULL && !GOC_STYLED_ITEM (dup)->scale_line_width);
	}

	/* Draw smoke tests.  */
	set_fill_style (circle, GO_COLOR_BLUE);
	set_fill_style (ellipse, GO_COLOR_GREEN);
	g_object_set (ellipse, "rotation", 0., NULL);
	{
		cairo_surface_t *surface =
			cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 30, 30);
		cairo_t *cr = cairo_create (surface);

		cairo_set_source_rgb (cr, 1., 1., 1.);
		cairo_paint (cr);
		goc_item_draw (circle, cr);
		goc_item_draw (ellipse, cr);
		check_pixel (surface, 10, 20, 0, 0, 255, 60, "circle fill draw");
		check_pixel (surface, 10, 5, 0, 255, 0, 60, "ellipse fill draw");
		check_pixel_white (surface, 0, 0, "shape draw outside");

		cairo_destroy (cr);
		cairo_surface_destroy (surface);
	}

	dup = goc_item_duplicate (ellipse, root);
	CHECK (GOC_ELLIPSE (dup)->width == 20. && GOC_ELLIPSE (dup)->height == 10.);

	g_object_unref (root);
}

/* ------------------------------------------------------------------------- */

static void
test_arc (void)
{
	GocGroup *root = new_group (NULL);
	GocItem *item;
	GOArrow arrow;
	cairo_matrix_t m;
	GocItem *near = NULL;
	double x0, y0, x1, y1;

	/* Set and clear the arrow properties; clearing must not dereference
	 * NULL.  */
	go_arrow_init_kite (&arrow, 4., 3., 3.);
	item = goc_item_new (root, GOC_TYPE_ARC,
			     "xc", 100., "yc", 50., "xr", 40., "yr", 40.,
			     "ang1", 0., "ang2", G_PI, "type", 0,
			     "start-arrow", &arrow, NULL);
	CHECK (GOC_ARC (item)->start_arrow.typ == GO_ARROW_KITE);
	CHECK (GOC_ARC (item)->start_arrow.a == 4.);
	g_object_set (item, "start-arrow", NULL, NULL);
	CHECK (GOC_ARC (item)->start_arrow.typ == GO_ARROW_NONE);
	CHECK (GOC_ARC (item)->start_arrow.a == 0.);
	g_object_set (item, "end-arrow", NULL, NULL);
	CHECK (GOC_ARC (item)->end_arrow.typ == GO_ARROW_NONE);

	/* A plain stroked semicircle.  */
	set_line_style (item, 2., CAIRO_LINE_CAP_BUTT);
	check_bounds (item, 59., 9., 141., 50., "arc bounds");
	CHECK (goc_item_distance (item, 100., 10., &near) == 0.);
	CHECK (goc_item_distance (item, 100., 30., &near) != 0.);

	/* The start arrow extends the bounds on the start side.  */
	go_arrow_init_kite (&arrow, 8., 6., 6.);
	g_object_set (item, "start-arrow", &arrow, NULL);
	goc_item_get_bounds (item, &x0, &y0, &x1, &y1);
	CHECK (x0 < 59.1);
	CHECK (x1 > 148. && x1 < 149.);
	CHECK (goc_item_distance (item, 146., 45., &near) == 0.);
	g_object_set (item, "start-arrow", NULL, NULL);

	/* scale-line-width: transforms scale the stroke width.  */
	set_line_style (item, 10., CAIRO_LINE_CAP_BUTT);
	check_bounds (item, 55., 5., 145., 50., "arc bounds, width 10");

	cairo_matrix_init_scale (&m, 2., 2.);
	goc_item_set_transform (item, &m);
	check_bounds (item, 110., 10., 290., 100., "arc scaled bounds");
	CHECK (goc_item_distance (item, 288., 95., &near) == 0.);
	CHECK (goc_item_distance (item, 292., 95., &near) != 0.);

	/* The drawn stroke is scaled as well: the top of the dome is at
	 * (200,20) and the half width grows from 5 to 10.  */
	{
		cairo_surface_t *surface =
			cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 300, 150);
		cairo_t *cr = cairo_create (surface);

		cairo_set_source_rgb (cr, 1., 1., 1.);
		cairo_paint (cr);
		goc_item_draw (item, cr);
		check_pixel_dark (surface, 200, 12, "arc scaled stroke draw");
		check_pixel_white (surface, 200, 6, "arc scaled stroke draw, outside");
		cairo_destroy (cr);
		cairo_surface_destroy (surface);
	}

	cairo_matrix_init_identity (&m);
	goc_item_set_transform (item, &m);

	g_object_unref (root);
}

/* ------------------------------------------------------------------------- */

static void
test_render (void)
{
	GocGroup *root = new_group (NULL);
	GocGroup *empty = new_group (NULL);
	GocItem *rect, *line;
	cairo_surface_t *surface;
	cairo_t *cr;

	rect = goc_item_new (root, GOC_TYPE_RECTANGLE,
			     "x", 10., "y", 10., "width", 40., "height", 40., NULL);
	set_fill_style (rect, GO_COLOR_RED);

	line = goc_item_new (root, GOC_TYPE_LINE,
			     "x0", 0., "y0", 55., "x1", 60., "y1", 55., NULL);
	set_line_style (line, 4., CAIRO_LINE_CAP_BUTT);

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 60, 60);
	cr = cairo_create (surface);
	cairo_set_source_rgb (cr, 1., 1., 1.);
	cairo_paint (cr);

	CHECK (goc_item_draw_region (GOC_ITEM (root), cr, 0., 0., 60., 60.));
	check_pixel (surface, 30, 30, 255, 0, 0, 60, "group render, fill");
	check_pixel_dark (surface, 5, 55, "group render, line");
	check_pixel_white (surface, 0, 0, "group render, corner");

	/* An empty group reports success and draws nothing.  */
	cairo_set_source_rgb (cr, 1., 1., 1.);
	cairo_paint (cr);
	CHECK (goc_item_draw_region (GOC_ITEM (empty), cr, 0., 0., 60., 60.));
	check_pixel_white (surface, 30, 30, "empty group render");

	cairo_destroy (cr);
	cairo_surface_destroy (surface);

	goc_group_clear (empty);
	g_object_unref (empty);
	g_object_unref (root);
}

/* ------------------------------------------------------------------------- */

static void
test_canvas (void)
{
	GocCanvas *canvas = new_canvas ();
	GocGroup *root = goc_canvas_get_root (canvas);
	GocItem *rect, *line;
	double x, y, x0, y0, x1, y1;
	int wx, wy;

	CHECK (GOC_IS_GROUP (root));
	CHECK (goc_canvas_get_pixels_per_unit (canvas) == 1.);
	goc_canvas_get_scroll_position (canvas, &x, &y);
	CHECK (x == 0. && y == 0.);
	CHECK (goc_canvas_get_direction (canvas) == GOC_DIRECTION_LTR);
	CHECK (goc_canvas_get_width (canvas) == 0);
	CHECK (goc_canvas_get_height (canvas) == 0);
	CHECK (goc_canvas_get_grabbed_item (canvas) == NULL);
	CHECK (goc_canvas_get_document (canvas) == NULL);

	goc_canvas_set_pixels_per_unit (canvas, 2.);
	CHECK (goc_canvas_get_pixels_per_unit (canvas) == 2.);
	goc_canvas_scroll_to (canvas, 10., 20.);
	goc_canvas_get_scroll_position (canvas, &x, &y);
	CHECK (x == 10. && y == 20.);

	goc_canvas_w2c (canvas, 4, 6, &x, &y);
	CHECK (x == 12. && y == 23.);
	goc_canvas_w2c (canvas, 4, 6, NULL, NULL);
	goc_canvas_c2w (canvas, 12., 23., &wx, &wy);
	CHECK (wx == 4 && wy == 6);

	/* Right to left mirrors around the widget width (zero here).  */
	goc_canvas_set_direction (canvas, GOC_DIRECTION_RTL);
	CHECK (goc_canvas_get_direction (canvas) == GOC_DIRECTION_RTL);
	goc_canvas_w2c (canvas, 4, 6, &x, &y);
	CHECK (x == 8. && y == 23.);
	goc_canvas_c2w (canvas, 8., 23., &wx, &wy);
	CHECK (wx == 4 && wy == 6);
	goc_canvas_set_direction (canvas, GOC_DIRECTION_LTR);

	/* Bounds and hit testing.  */
	rect = goc_item_new (root, GOC_TYPE_RECTANGLE,
			     "x", 0., "y", 0., "width", 10., "height", 10., NULL);
	set_fill_style (rect, GO_COLOR_RED);
	{
		GocItem *top = goc_item_new (root, GOC_TYPE_RECTANGLE,
					     "x", 5., "y", 5., "width", 10., "height", 10., NULL);
		set_fill_style (top, GO_COLOR_BLUE);

		/* Prime the cached bounds; group hit testing uses them.  */
		goc_canvas_get_bounds (canvas, &x0, &y0, &x1, &y1);
		CHECK (x0 == 0. && y0 == 0. && x1 == 15. && y1 == 15.);

		CHECK (goc_canvas_get_item_at (canvas, 6., 6.) == top);
		CHECK (goc_canvas_get_item_at (canvas, 1., 1.) == rect);
		CHECK (goc_canvas_get_item_at (canvas, 50., 50.) == NULL);

		goc_item_lower_to_bottom (top);
		CHECK (goc_canvas_get_item_at (canvas, 6., 6.) == rect);
		goc_item_hide (rect);
		CHECK (goc_canvas_get_item_at (canvas, 6., 6.) == top);
		goc_item_show (rect);
	}

	/* A thin line is hit only close to it.  */
	line = goc_item_new (root, GOC_TYPE_LINE,
			     "x0", 20., "y0", 20., "x1", 40., "y1", 20., NULL);
	set_line_style (line, 0., CAIRO_LINE_CAP_BUTT);
	goc_canvas_get_bounds (canvas, &x0, &y0, &x1, &y1);
	CHECK (goc_canvas_get_item_at (canvas, 30., 20.) == line);
	CHECK (goc_canvas_get_item_at (canvas, 30., 30.) == NULL);

	/* Grabbing.  */
	goc_item_grab (line);
	CHECK (goc_canvas_get_grabbed_item (canvas) == line);
	goc_item_ungrab (line);
	CHECK (goc_canvas_get_grabbed_item (canvas) == NULL);
	goc_item_grab (line);
	goc_item_destroy (line);
	CHECK (goc_canvas_get_grabbed_item (canvas) == NULL);

	/* Pixel-per-unit dependent distances.  */
	{
		GocItem *r2 = goc_item_new (root, GOC_TYPE_RECTANGLE,
					    "x", 100., "y", 100.,
					    "width", 20., "height", 10., NULL);
		GocItem *near = NULL;
		GOStyle *style;

		set_fill_style (r2, GO_COLOR_RED);
		CHECK (goc_item_distance (r2, 110., 105., &near) == 0.);
		CHECK (goc_item_distance (r2, 200., 200., &near) == 20.);
		/* The distance code must restore the style line width.  */
		style = go_styled_object_get_style (GO_STYLED_OBJECT (r2));
		CHECK (style->line.width == 0.);
	}
	{
		GocItem *p2 = goc_item_new (root, GOC_TYPE_POLYLINE, NULL);
		GocPoints *pts = goc_points_new (3);
		GocItem *near = NULL;

		pts->points[0].x = 100.; pts->points[0].y = 100.;
		pts->points[1].x = 110.; pts->points[1].y = 100.;
		pts->points[2].x = 110.; pts->points[2].y = 110.;
		g_object_set (p2, "points", pts, NULL);
		goc_points_unref (pts);
		set_line_style (p2, 0., CAIRO_LINE_CAP_BUTT);

		CHECK (goc_item_distance (p2, 105., 100., &near) == 0.);
		CHECK (goc_item_distance (p2, 110., 105., &near) == 0.);
		CHECK (goc_item_distance (p2, 200., 200., &near) == 20.);
	}

	/* Invalidation is a no-op while the canvas is unrealized, but it
	 * must not crash or leave stale state behind.  */
	goc_canvas_invalidate (canvas, 0., 0., 10., 10.);
	{
		cairo_region_t *region = cairo_region_create ();
		goc_canvas_invalidate_region (canvas, GOC_ITEM (root), region);
		cairo_region_destroy (region);
	}

	/* Optional out parameters and NULL guards.  */
	goc_canvas_get_bounds (canvas, NULL, NULL, NULL, NULL);
	goc_canvas_get_bounds (canvas, &x0, NULL, NULL, NULL);
	CHECK (x0 == 0.);

	{
		GLogLevelFlags old_mask = g_log_set_always_fatal (0);
		GLogFunc old_handler = g_log_set_default_handler (ignore_log_message, NULL);
		cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
		cairo_t *cr = cairo_create (surface);

		/* Invalid scales must be rejected.  */
		goc_canvas_set_pixels_per_unit (canvas, 0.);
		goc_canvas_set_pixels_per_unit (canvas, -2.);
		goc_canvas_set_pixels_per_unit (canvas, NAN);
		CHECK (goc_canvas_get_pixels_per_unit (canvas) == 2.);

		/* NULL canvas guards must not crash.  */
		CHECK (goc_canvas_get_item_at (NULL, 0., 0.) == NULL);
		goc_canvas_get_bounds (NULL, &x0, &y0, &x1, &y1);
		goc_canvas_w2c (NULL, 0, 0, &x, &y);
		goc_canvas_c2w (NULL, 0., 0., &wx, &wy);
		goc_canvas_render (NULL, cr, 0., 0., 1., 1.);
		goc_canvas_invalidate (NULL, 0., 0., 1., 1.);

		cairo_destroy (cr);
		cairo_surface_destroy (surface);
		g_log_set_default_handler (old_handler, NULL);
		g_log_set_always_fatal (old_mask);
	}

	g_object_unref (canvas);
}

static void
test_canvas_render (void)
{
	GocCanvas *canvas = new_canvas ();
	GocGroup *root = goc_canvas_get_root (canvas);
	GocItem *rect;
	cairo_surface_t *surface;
	cairo_t *cr;

	rect = goc_item_new (root, GOC_TYPE_RECTANGLE,
			     "x", 10., "y", 10., "width", 40., "height", 40., NULL);
	set_fill_style (rect, GO_COLOR_RED);

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 60, 60);
	cr = cairo_create (surface);

	cairo_set_source_rgb (cr, 1., 1., 1.);
	cairo_paint (cr);
	goc_canvas_render (canvas, cr, 0., 0., 60., 60.);
	check_pixel (surface, 30, 30, 255, 0, 0, 60, "canvas render");
	check_pixel_white (surface, 5, 5, "canvas render outside");

	/* Scrolling moves the canvas contents inside the widget.  */
	cairo_set_source_rgb (cr, 1., 1., 1.);
	cairo_paint (cr);
	goc_canvas_scroll_to (canvas, 10., 10.);
	goc_canvas_render (canvas, cr, 10., 10., 70., 70.);
	check_pixel (surface, 20, 30, 255, 0, 0, 60, "canvas render, scrolled");
	check_pixel_white (surface, 50, 10, "canvas render, scrolled outside");

	cairo_destroy (cr);
	cairo_surface_destroy (surface);
	g_object_unref (canvas);
}

/* ------------------------------------------------------------------------- */

/* Exercise the canvas draw handler directly: a missing current event and
 * several clip rectangles must not confuse it, the clip rectangles are
 * interpreted in widget coordinates, and the invalidated-region path is
 * used.  Needs a realized canvas for the adjustments.  */
static void
test_canvas_draw (void)
{
	GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	GocCanvas *canvas = new_canvas ();
	GocGroup *root = goc_canvas_get_root (canvas);
	GocItem *rect, *ants;
	GtkWidgetClass *widget_klass;
	GtkAdjustment *hadj, *vadj;
	cairo_surface_t *surface;
	cairo_t *cr;
	cairo_region_t *region;
	cairo_rectangle_int_t r = { 0, 0, 10, 10 };

	gtk_window_set_default_size (GTK_WINDOW (window), 200, 100);
	gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (canvas));
	gtk_widget_show_all (window);
	while (gtk_events_pending ())
		gtk_main_iteration ();

	canvas->width = 200;
	canvas->height = 100;

	/* Deterministic adjustments: the canvas is scrolled by 10.  The
	 * GtkLayout does not own them, so they must stay alive here.  */
	hadj = g_object_ref_sink (gtk_adjustment_new (0., 0., 1000., 1., 10., 100.));
	vadj = g_object_ref_sink (gtk_adjustment_new (0., 0., 1000., 1., 10., 100.));
	gtk_scrollable_set_hadjustment (GTK_SCROLLABLE (canvas), hadj);
	gtk_scrollable_set_vadjustment (GTK_SCROLLABLE (canvas), vadj);
	/* GtkLayout reconfigures the adjustment to its own size, so the
	 * value has to be forced afterwards.  */
	gtk_adjustment_configure (hadj, 10., 0., 1000., 1., 10., 100.);

	widget_klass = (GtkWidgetClass *) G_OBJECT_GET_CLASS (canvas);

	/* A red rectangle at item x 98..128.  With the canvas scrolled by 10
	 * it spans device x 88..118.  */
	rect = goc_item_new (root, GOC_TYPE_RECTANGLE,
			     "x", 98., "y", 10., "width", 30., "height", 20., NULL);
	set_fill_style (rect, GO_COLOR_RED);

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 200, 100);
	cr = cairo_create (surface);
	cairo_set_source_rgb (cr, 1., 1., 1.);
	cairo_paint (cr);
	/* Two clip rectangles; only the second one covers the item.  There
	 * is no current event here, so the handler must cope with NULL.  */
	cairo_rectangle (cr, 0., 0., 10., 10.);
	cairo_rectangle (cr, 80., 5., 40., 35.);
	cairo_clip (cr);
	CHECK (widget_klass->draw (GTK_WIDGET (canvas), cr));
	check_pixel (surface, 115, 20, 255, 0, 0, 60, "canvas draw, second clip");
	check_pixel_white (surface, 5, 5, "canvas draw, first clip");
	cairo_destroy (cr);
	cairo_surface_destroy (surface);

	/* The invalidated-region path draws the single item directly and
	 * resets the cached state on the next draw.  */
	ants = goc_item_new (root, GOC_TYPE_RECTANGLE,
			     "x", 0., "y", 0., "width", 5., "height", 5., NULL);
	set_fill_style (ants, GO_COLOR_BLUE);
	region = cairo_region_create_rectangle (&r);
	goc_canvas_invalidate_region (canvas, ants, region);
	cairo_region_destroy (region);

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 10, 10);
	cr = cairo_create (surface);
	cairo_set_source_rgb (cr, 1., 1., 1.);
	cairo_paint (cr);
	cairo_rectangle (cr, 0., 0., 10., 10.);
	cairo_clip (cr);
	CHECK (widget_klass->draw (GTK_WIDGET (canvas), cr));
	check_pixel (surface, 2, 2, 0, 0, 255, 60, "canvas draw, invalidated item");
	cairo_destroy (cr);
	cairo_surface_destroy (surface);

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 10, 10);
	cr = cairo_create (surface);
	cairo_set_source_rgb (cr, 1., 1., 1.);
	cairo_paint (cr);
	cairo_rectangle (cr, 0., 0., 10., 10.);
	cairo_clip (cr);
	CHECK (widget_klass->draw (GTK_WIDGET (canvas), cr));
	check_pixel_white (surface, 2, 2, "canvas draw, cleared region");
	cairo_destroy (cr);
	cairo_surface_destroy (surface);

	gtk_widget_destroy (window);
	g_object_unref (canvas);
	g_object_unref (hadj);
	g_object_unref (vadj);
}

/* ------------------------------------------------------------------------- */

int
main (int argc, char **argv)
{
	GString *passes;

	(void) argc;
	(void) argv;

#ifdef GOFFICE_WITH_GTK
	/* A GtkLayout based canvas needs a display to instantiate.  Not
	 * having one is not a test failure, but report what was skipped.  */
	have_display = gtk_init_check (NULL, NULL);
#endif

	libgoffice_init ();

	passes = g_string_new ("utils");
	test_utils ();

	g_string_append (passes, ", groups");
	test_groups ();

	g_string_append (passes, ", items");
	test_item_common ();

	g_string_append (passes, ", line");
	test_line ();

	g_string_append (passes, ", rectangle");
	test_rectangle ();

	g_string_append (passes, ", polyline");
	test_polyline ();

	g_string_append (passes, ", polygon");
	test_polygon ();

	g_string_append (passes, ", text");
	test_text ();

	g_string_append (passes, ", pixbuf");
	test_pixbuf ();

	g_string_append (passes, ", image");
	test_image ();

	g_string_append (passes, ", path");
	test_path ();

#ifdef GOFFICE_WITH_GTK
	g_string_append (passes, ", component");
	test_component ();
#endif

	g_string_append (passes, ", graph");
	test_graph ();

	g_string_append (passes, ", circle/ellipse");
	test_circle_ellipse ();

	g_string_append (passes, ", arc");
	test_arc ();

	g_string_append (passes, ", render");
	test_render ();

	if (have_display) {
		g_string_append (passes, ", canvas");
		test_canvas ();
		test_canvas_render ();
		g_string_append (passes, ", canvas draw");
		test_canvas_draw ();
#ifdef GOFFICE_WITH_GTK
		g_string_append (passes, ", widget");
		test_widget ();
		g_string_append (passes, ", duplicate");
		test_duplicate_realized ();
		g_string_append (passes, ", component render");
		test_component_render ();
		g_string_append (passes, ", text render");
		test_text_render ();
		g_string_append (passes, ", pixbuf render");
		test_pixbuf_render ();
		g_string_append (passes, ", arc render");
		test_arc_render ();
		g_string_append (passes, ", path render");
		test_path_render ();
		g_string_append (passes, ", line render");
		test_line_render ();
		g_string_append (passes, ", graph tooltip");
		test_graph_tooltip ();
#endif
	} else
		skip ("canvas tests (no display connection)");

	g_print ("test-canvas: exercised %s\n", passes->str);
	g_string_free (passes, TRUE);

	libgoffice_shutdown ();

	if (skips)
		g_print ("test-canvas: %d group(s) skipped.\n", skips);
	if (failures)
		g_printerr ("%d canvas test(s) failed.\n", failures);

	return failures ? 1 : 0;
}
