/*
 * goc-polyline.c :
 *
 * Copyright (C) 2009 Jean Brefort (jean.brefort@normalesup.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#include <goffice/goffice-config.h>
#include <goffice/goffice.h>
#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

/**
 * SECTION:goc-polyline
 * @short_description: Multi-segment line.
 *
 * #GocPolyline implements muti-segment line drawing in the canvas.
**/

enum {
	POLYLINE_PROP_0,
	POLYLINE_PROP_POINTS,
	POLYLINE_PROP_SPLINE
};

static GObjectClass *parent_class;

static void
goc_polyline_set_property (GObject *gobject, guint param_id,
				    GValue const *value, GParamSpec *pspec)
{
	GocPolyline *polyline = GOC_POLYLINE (gobject);

	switch (param_id) {
	case POLYLINE_PROP_POINTS: {
		unsigned i;
		GocPoints *points = (GocPoints *) g_value_get_boxed (value);
		polyline->nb_points = points->n;
		g_free (polyline->points);
		if (points->n > 0) {
			polyline->points = g_new (GocPoint, points->n);
			for (i = 0; i < points->n; i++)
				polyline->points[i] = points->points[i];
		} else
			polyline->points = NULL;
		break;
	}
	case POLYLINE_PROP_SPLINE:
		polyline->use_spline = g_value_get_boolean (value);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
	if (polyline->use_spline && polyline->nb_points) {
		double *x, *y;
		unsigned i;
		x = g_alloca (polyline->nb_points * sizeof (double));
		y = g_alloca (polyline->nb_points * sizeof (double));
		for (i = 0; i < polyline->nb_points; i++) {
			x[i] = polyline->points[i].x - polyline->points[0].x;
			y[i] = polyline->points[i].y - polyline->points[0].y;
		}
		g_object_set_data_full (G_OBJECT (polyline), "spline", go_bezier_spline_init (x, y, polyline->nb_points, FALSE), (GDestroyNotify) go_bezier_spline_destroy);
	}
	goc_item_bounds_changed (GOC_ITEM (gobject));
}

static void
goc_polyline_get_property (GObject *gobject, guint param_id,
				    GValue *value, GParamSpec *pspec)
{
	GocPolyline *polyline = GOC_POLYLINE (gobject);

	switch (param_id) {
	case POLYLINE_PROP_POINTS: {
		unsigned i;
		GocPoints *points = goc_points_new (polyline->nb_points);
		for (i = 0; i < points->n; i++)
			points->points[i] = polyline->points[i];
		g_value_set_boxed (value, points);
		goc_points_unref (points);
		break;
	}
	case POLYLINE_PROP_SPLINE:
		g_value_set_boolean (value, polyline->use_spline);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
}

static gboolean
goc_polyline_prepare_draw (GocItem const *item, cairo_t *cr, gboolean flag)
{
	GocPolyline *polyline = GOC_POLYLINE (item);
	unsigned i;
	gboolean scale_line_width = goc_styled_item_get_scale_line_width (GOC_STYLED_ITEM (item));
	gboolean needs_restore;

	cairo_save (cr);
	needs_restore = TRUE;

	_goc_item_transform (item, cr, flag);
	if (polyline->nb_points == 0)
		return FALSE;

	if (1 == flag) {
		goc_group_cairo_transform (item->parent, cr, polyline->points[0].x, polyline->points[0].y);
		cairo_move_to (cr, 0., 0.);
	} else {
		cairo_move_to (cr, polyline->points[0].x, polyline->points[0].y);
	}
	if (polyline->use_spline) {
		GOBezierSpline *spline = (GOBezierSpline *) g_object_get_data (G_OBJECT (polyline), "spline");
		cairo_save (cr);
		if (flag == 0)
			cairo_translate (cr, polyline->points[0].x, polyline->points[0].y);
		go_bezier_spline_to_cairo (spline, cr, item->canvas && goc_canvas_get_direction (item->canvas) == GOC_DIRECTION_RTL);
		cairo_restore (cr);
	} else {
		double sign = (flag && item->canvas && goc_canvas_get_direction (item->canvas) == GOC_DIRECTION_RTL)? -1: 1;
		gboolean prev_valid = TRUE;
		for (i = 1; i < polyline->nb_points; i++) {
			if (go_finite (polyline->points[i].x)) {
				if (prev_valid)
					cairo_line_to (cr, (polyline->points[i].x - polyline->points[0].x * flag) * sign,
						polyline->points[i].y - polyline->points[0].y * flag);
				else {
					cairo_move_to (cr, (polyline->points[i].x - polyline->points[0].x * flag) * sign,
						polyline->points[i].y - polyline->points[0].y * flag);
					prev_valid = TRUE;
				}
			} else
			    prev_valid = FALSE;
		}
	}

	if (!scale_line_width) {
		cairo_restore (cr);
		needs_restore = FALSE;
	}
	if (goc_styled_item_set_cairo_line (GOC_STYLED_ITEM (item), cr)) {
		if (needs_restore)
			cairo_restore (cr);
		return TRUE;
	}

	if (needs_restore)
		cairo_restore (cr);
	return FALSE;
}

static void
goc_polyline_update_bounds (GocItem *item)
{
	cairo_surface_t *surface;
	cairo_t *cr;

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
	cr = cairo_create (surface);

	if (goc_polyline_prepare_draw (item, cr, 0)) {
		cairo_stroke_extents (cr, &item->x0, &item->y0, &item->x1, &item->y1);
	} else {
		item->x0 = item->y0 = G_MAXDOUBLE;
		item->x1 = item->y1 = -G_MAXDOUBLE;
	}

	cairo_destroy (cr);
	cairo_surface_destroy (surface);
}

static double
goc_polyline_distance (GocItem *item, double x, double y, GocItem **near_item)
{
	GocPolyline *polyline = GOC_POLYLINE (item);
	GOStyle *style = go_style_dup (go_styled_object_get_style (GO_STYLED_OBJECT (item)));
	double res = 20;
	double ppu = goc_canvas_get_pixels_per_unit (item->canvas);
	cairo_surface_t *surface;
	cairo_t *cr;

	if (polyline->nb_points == 0)
		return res;

	*near_item = item;
	if (style->line.width * ppu < 5)
		style->line.width = 5. / (ppu * ppu);
	else
		style->line.width /= ppu;
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
	cr = cairo_create (surface);

	if (goc_polyline_prepare_draw (item, cr, 0)) {
		if (cairo_in_stroke (cr, x, y))
			res = 0;
	}

	g_object_unref (style);
	cairo_destroy (cr);
	cairo_surface_destroy (surface);
	return res;
}

static void
goc_polyline_draw (GocItem const *item, cairo_t *cr)
{
	if (goc_polyline_prepare_draw (item, cr, 1)) {
		cairo_stroke (cr);
	}
}

static void
goc_polyline_copy (GocItem *dest, GocItem *source)
{
	GocPolyline *src = GOC_POLYLINE (source), *dst = GOC_POLYLINE (dest);
	
	dst->nb_points = src->nb_points;
	dst->use_spline = src->use_spline;
	if (src->nb_points > 0) {
		unsigned ui;
		dst->points = g_new (GocPoint, src->nb_points);
		for (ui = 0; ui < src->nb_points; ui++)
			dst->points[ui] = src->points[ui];
	} else
		dst->points = NULL;
	((GocItemClass *) parent_class)->copy (dest, source);
}

static void
goc_polyline_init_style (G_GNUC_UNUSED GocStyledItem *item, GOStyle *style)
{
	style->interesting_fields = GO_STYLE_LINE;
	if (style->line.auto_dash)
		style->line.dash_type = GO_LINE_SOLID;
	if (style->line.auto_color)
		style->line.color = GO_COLOR_BLACK;
}

static void
goc_polyline_finalize (GObject *obj)
{
	GocPolyline *polyline = GOC_POLYLINE (obj);
	g_free (polyline->points);
	parent_class->finalize (obj);
}

static void
goc_polyline_class_init (GocItemClass *item_klass)
{
	GObjectClass *obj_klass = (GObjectClass *) item_klass;
	GocStyledItemClass *gsi_klass = (GocStyledItemClass *) item_klass;
	parent_class = g_type_class_peek_parent (item_klass);

	gsi_klass->init_style = goc_polyline_init_style;

	obj_klass->finalize = goc_polyline_finalize;
	obj_klass->get_property = goc_polyline_get_property;
	obj_klass->set_property = goc_polyline_set_property;
        g_object_class_install_property (obj_klass, POLYLINE_PROP_POINTS,
                 g_param_spec_boxed ("points", _("points"), _("The polyline vertices"),
				     GOC_TYPE_POINTS,
				     GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, POLYLINE_PROP_SPLINE,
		g_param_spec_boolean ("use-spline",
				      _("Use spline"),
				      _("Use a Bezier cubic spline as line"),
				      FALSE,
				      GSF_PARAM_STATIC | G_PARAM_READWRITE));

	item_klass->update_bounds = goc_polyline_update_bounds;
	item_klass->distance = goc_polyline_distance;
	item_klass->draw = goc_polyline_draw;
	item_klass->copy = goc_polyline_copy;
}

GSF_CLASS (GocPolyline, goc_polyline,
	   goc_polyline_class_init, NULL,
	   GOC_TYPE_STYLED_ITEM)
