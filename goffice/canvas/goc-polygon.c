/*
 * goc-polygon.c :
 *
 * Copyright (C) 2008 Jean Brefort (jean.brefort@normalesup.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) version 3.
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
 * SECTION:goc-polygon
 * @short_description: Polygon.
 *
 * #GocPolygon implements polygon drawing in the canvas.
**/

enum {
	POLYGON_PROP_0,
	POLYGON_PROP_POINTS,
	POLYGON_PROP_SPLINE,
	POLYGON_PROP_FILL_RULE,
	POLYGON_PROP_SIZES
};

static GocStyledItemClass *parent_class;

static void
goc_polygon_finalize (GObject *obj)
{
	GocPolygon *polygon = GOC_POLYGON (obj);
	g_free (polygon->points);
	g_free (polygon->sizes);
	((GObjectClass *) parent_class)->finalize (obj);
}

static void
goc_polygon_set_property (GObject *gobject, guint param_id,
			  GValue const *value, GParamSpec *pspec)
{
	GocPolygon *polygon = GOC_POLYGON (gobject);

	switch (param_id) {
	case POLYGON_PROP_POINTS: {
		unsigned i;
		GocPoints *points = (GocPoints *) g_value_get_boxed (value);
		polygon->nb_points = points->n;
		g_free (polygon->points);
		if (points->n > 0) {
			polygon->points = g_new (GocPoint, points->n);
			for (i = 0; i < points->n; i++)
				polygon->points[i] = points->points[i];
		} else
			polygon->points = NULL;
		/* reset sizes */
		g_free (polygon->sizes);
		polygon->sizes = NULL;
		polygon->nb_sizes = 0;
		break;
	}
	case POLYGON_PROP_SPLINE:
		polygon->use_spline = g_value_get_boolean (value);
		break;
	case POLYGON_PROP_FILL_RULE:
		polygon->fill_rule = g_value_get_boolean (value);
		break;
	case POLYGON_PROP_SIZES: {
		unsigned i, avail = polygon->nb_points - 3;
		GocIntArray *array = (GocIntArray *) g_value_get_boxed (value);
		g_free (polygon->sizes);
		polygon->sizes = NULL;
		polygon->nb_sizes = 0;
		for (i = 0; i < array->n; i++) {
			if (array->vals[i] < 3 || array->vals[i] > (int) avail)
				break;
			avail -= array->vals[i];
			polygon->nb_sizes++;
		}
		if (polygon->nb_sizes > 0) {
			polygon->sizes = g_new (int, polygon->nb_sizes);
			for (i = 0; i < polygon->nb_sizes; i++)
				polygon->sizes[i] = array->vals[i];
		}
		break;
	}

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
	if (polygon->use_spline && polygon->nb_points) {
		double *x, *y;
		unsigned i;
		x = g_alloca (polygon->nb_points * sizeof (double));
		y = g_alloca (polygon->nb_points * sizeof (double));
		for (i = 0; i < polygon->nb_points; i++) {
			x[i] = polygon->points[i].x - polygon->points[0].x;
			y[i] = polygon->points[i].y - polygon->points[0].y;
		}
		g_object_set_data_full (G_OBJECT (polygon), "spline", go_bezier_spline_init (x, y, polygon->nb_points, TRUE), (GDestroyNotify) go_bezier_spline_destroy);
	}
	goc_item_bounds_changed (GOC_ITEM (gobject));
}

static void
goc_polygon_get_property (GObject *gobject, guint param_id,
				    GValue *value, GParamSpec *pspec)
{
	GocPolygon *polygon = GOC_POLYGON (gobject);

	switch (param_id) {
	case POLYGON_PROP_POINTS: {
		unsigned i;
		GocPoints *points = goc_points_new (polygon->nb_points);
		for (i = 0; i < points->n; i++)
			points->points[i] = polygon->points[i];
		g_value_set_boxed (value, points);
		goc_points_unref (points);
		break;
	}
	case POLYGON_PROP_SPLINE:
		g_value_set_boolean (value, polygon->use_spline);
		break;
	case POLYGON_PROP_FILL_RULE:
		g_value_set_boolean (value, polygon->fill_rule);
		break;
	case POLYGON_PROP_SIZES: {
		unsigned i;
		GocIntArray *array = goc_int_array_new (polygon->nb_sizes);
		for (i = 0; i < array->n; i++)
			array->vals[i] = polygon->sizes[i];
		g_value_set_boxed (value, array);
		goc_int_array_unref (array);
		break;
	}

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
}

static gboolean
goc_polygon_prepare_path (GocItem const *item, cairo_t *cr, gboolean flag)
{
	GocPolygon *polygon = GOC_POLYGON (item);
	unsigned snum;
	int i, j;

	if (polygon->nb_points == 0)
		return FALSE;

	cairo_set_fill_rule (cr, polygon->fill_rule);
	if (1 == flag) {
		goc_group_cairo_transform (item->parent, cr, polygon->points[0].x, polygon->points[0].y);
		cairo_move_to (cr, 0., 0.);
	} else {
		cairo_move_to (cr, polygon->points[0].x, polygon->points[0].y);
	}
	if (polygon->use_spline) {
		GOBezierSpline *spline = (GOBezierSpline *) g_object_get_data (G_OBJECT (polygon), "spline");
		cairo_save (cr);
		if (flag == 0)
			cairo_translate (cr, polygon->points[0].x, polygon->points[0].y);
		go_bezier_spline_to_cairo (spline, cr, item->canvas && goc_canvas_get_direction (item->canvas) == GOC_DIRECTION_RTL);
		cairo_restore (cr);
	} else {
		/* sign is meaningful only for drawing, so if flag == 0, sign must be 1 */
		double sign = (flag && item->canvas && goc_canvas_get_direction (item->canvas) == GOC_DIRECTION_RTL)? -1: 1;
		if (polygon->nb_sizes > 0) {
			snum = 0;
			for (j = 0; j < (int) polygon->nb_sizes; j++) {
				cairo_move_to (cr, (polygon->points[snum].x - polygon->points[0].x * flag) * sign,
					polygon->points[snum].y - polygon->points[0].y * flag);
				for (i = 1; i < polygon->sizes[j]; i++)
					cairo_line_to (cr, (polygon->points[snum + i].x - polygon->points[0].x * flag) * sign,
						polygon->points[snum + i].y - polygon->points[0].y * flag);
				cairo_close_path (cr);
				snum += polygon->sizes[j];
			}
			cairo_move_to (cr, (polygon->points[snum].x - polygon->points[0].x * flag) * sign,
						polygon->points[snum].y - polygon->points[0].y * flag);
			for (i = snum + 1; i < (int) polygon->nb_points; i++)
				cairo_line_to (cr, (polygon->points[i].x - polygon->points[0].x * flag) * sign,
						polygon->points[i].y - polygon->points[0].y * flag);
			cairo_close_path (cr);

		} else {
		    for (i = 1; i < (int) polygon->nb_points; i++)
			cairo_line_to (cr, (polygon->points[i].x - polygon->points[0].x * flag) * sign,
				polygon->points[i].y - polygon->points[0].y * flag);
		    cairo_close_path (cr);
		}
	}

	return TRUE;
}

static void
goc_polygon_update_bounds (GocItem *item)
{
	cairo_surface_t *surface;
	cairo_t *cr;
	int mode = 0;

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
	cr = cairo_create (surface);

	cairo_save (cr);
	_goc_item_transform (item, cr, FALSE);
	if (go_styled_object_set_cairo_line (GO_STYLED_OBJECT (item), cr))
		mode = 1;
	else if (go_style_is_fill_visible (go_styled_object_get_style (GO_STYLED_OBJECT (item))))
		mode = 2;
	if (mode && goc_polygon_prepare_path (item, cr, 0)) {
		cairo_restore (cr);
		if (mode == 1)
			cairo_stroke_extents (cr, &item->x0, &item->y0, &item->x1, &item->y1);
		else
			cairo_fill_extents (cr, &item->x0, &item->y0, &item->x1, &item->y1);
	} else {
		item->x0 = item->y0 = G_MAXDOUBLE;
		item->x1 = item->y1 = -G_MAXDOUBLE;
	}
	cairo_destroy (cr);
	cairo_surface_destroy (surface);
}

static double
goc_polygon_distance (GocItem *item, double x, double y, GocItem **near_item)
{
	GocPolygon *polygon = GOC_POLYGON (item);
	GOStyle *style = go_style_dup (go_styled_object_get_style (GO_STYLED_OBJECT (item)));
	double res = G_MAXDOUBLE;
	double ppu = goc_canvas_get_pixels_per_unit (item->canvas);
	GOLineDashType tmp_dash_type = style->line.dash_type;
	cairo_surface_t *surface;
	cairo_t *cr;

	if (polygon->nb_points == 0)
		return res;

	*near_item = item;
	if (style->line.width * ppu < 5)
		style->line.width = 5. / (ppu * ppu);
	else
		style->line.width /= ppu;
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
	cr = cairo_create (surface);
	cairo_save (cr);
	goc_polygon_prepare_path (item, cr, 0);
	cairo_restore (cr);
	if (go_style_is_fill_visible (style)) {
		if (cairo_in_fill (cr, x, y))
			res = 0;
		else if ((item->x1 - item->x0 < 5 || item->y1 - item->y0 < 5) && style->line.dash_type == tmp_dash_type) {
			style->line.dash_type = GO_LINE_SOLID;
			style->line.auto_dash = FALSE;
		}
	}
	if (res > 0 && style->line.dash_type != GO_LINE_NONE) {
		go_styled_object_set_cairo_line (GO_STYLED_OBJECT (item), cr);
		if (cairo_in_stroke (cr, x, y))
			res = 0;
	}

	g_object_unref (style);
	cairo_destroy (cr);
	cairo_surface_destroy (surface);
	return res;
}

static void
goc_polygon_draw (GocItem const *item, cairo_t *cr)
{
	gboolean scale_line_width = goc_styled_item_get_scale_line_width (GOC_STYLED_ITEM (item));
	gboolean needs_restore;

	cairo_save (cr);
	needs_restore = TRUE;
	_goc_item_transform (item, cr, TRUE);
	if (goc_polygon_prepare_path (item, cr, 1)) {
		go_styled_object_fill (GO_STYLED_OBJECT (item), cr, TRUE);

		if (!scale_line_width) {
			cairo_restore (cr);
			needs_restore = FALSE;
		}
		if (go_styled_object_set_cairo_line (GO_STYLED_OBJECT (item), cr))
			cairo_stroke (cr);
		else
			cairo_new_path (cr);
	}
	if (needs_restore)
		cairo_restore (cr);
}

static void
goc_polygon_copy (GocItem *dest, GocItem *source)
{
	GocPolygon *src = GOC_POLYGON (source), *dst = GOC_POLYGON (dest);
	unsigned ui;
	
	dst->nb_points = src->nb_points;
	dst->use_spline = src->use_spline;
	dst->fill_rule = src->fill_rule;
	if (src->nb_points > 0) {
		dst->points = g_new (GocPoint, src->nb_points);
		for (ui = 0; ui < src->nb_points; ui++)
			dst->points[ui] = src->points[ui];
	} else
		dst->points = NULL;
	if (src->nb_sizes > 0) {
		dst->sizes = g_new (int, src->nb_sizes);
		for (ui = 0; ui < src->nb_sizes; ui++)
			dst->sizes[ui] = src->sizes[ui];
	} else
		dst->sizes = NULL;
	((GocItemClass *) parent_class)->copy (dest, source);
}

static void
goc_polygon_init_style (G_GNUC_UNUSED GocStyledItem *item, GOStyle *style)
{
	style->interesting_fields = GO_STYLE_OUTLINE | GO_STYLE_FILL;
	if (style->line.auto_dash)
		style->line.dash_type = GO_LINE_SOLID;
	if (style->line.auto_color)
		style->line.color = GO_COLOR_BLACK;
	if (style->fill.auto_type)
		style->fill.type  = GO_STYLE_FILL_PATTERN;
	if (style->fill.auto_fore)
		style->fill.pattern.fore = GO_COLOR_BLACK;
	if (style->fill.auto_back)
		style->fill.pattern.back = GO_COLOR_WHITE;
}

static void
goc_polygon_class_init (GocItemClass *item_klass)
{
	GObjectClass *obj_klass = (GObjectClass *) item_klass;
	GocStyledItemClass *gsi_klass = (GocStyledItemClass *) item_klass;

	parent_class = g_type_class_peek_parent (item_klass);

	gsi_klass->init_style = goc_polygon_init_style;

	obj_klass->get_property = goc_polygon_get_property;
	obj_klass->set_property = goc_polygon_set_property;
	obj_klass->finalize = goc_polygon_finalize;

        g_object_class_install_property (obj_klass, POLYGON_PROP_POINTS,
                 g_param_spec_boxed ("points", _("points"), _("The polygon vertices"),
				     GOC_TYPE_POINTS,
				     GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, POLYGON_PROP_SPLINE,
		g_param_spec_boolean ("use-spline",
				      _("Use spline"),
				      _("Use a Bezier closed cubic spline as contour"),
				      FALSE,
				      GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, POLYGON_PROP_FILL_RULE,
		g_param_spec_boolean ("fill-rule",
				      _("Fill rule"),
				      _("Set fill rule to winding or even/odd"),
				      FALSE,
				      GSF_PARAM_STATIC | G_PARAM_READWRITE));
        g_object_class_install_property (obj_klass, POLYGON_PROP_SIZES,
                 g_param_spec_boxed ("sizes", _("sizes"),
				     _("If set, the polygon will be split as several polygons according to the given sizes. "
				         "Each size must be at least 3. Values following an invalid value will be discarded. "
				         "Setting the \"point\" property will reset the sizes."),
				     GOC_TYPE_INT_ARRAY,
				     GSF_PARAM_STATIC | G_PARAM_READWRITE));

	item_klass->update_bounds = goc_polygon_update_bounds;
	item_klass->distance = goc_polygon_distance;
	item_klass->draw = goc_polygon_draw;
	item_klass->copy = goc_polygon_copy;
}

GSF_CLASS (GocPolygon, goc_polygon,
	   goc_polygon_class_init, NULL,
	   GOC_TYPE_STYLED_ITEM)
