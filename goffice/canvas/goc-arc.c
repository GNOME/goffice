/*
 * goc-arc.c :
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
#include <math.h>

/**
 * SECTION:goc-arc
 * @short_description: Simple elliptic arc.
 *
 * #GocArc implements simple elliptic arc drawing in the canvas. The arc can have
 * arrows at the start and/or at the end.
**/

static GocItemClass *parent_class;

enum {
	ARC_PROP_0,
	ARC_PROP_XC,
	ARC_PROP_YC,
	ARC_PROP_XR,
	ARC_PROP_YR,
	ARC_PROP_ANG1,
	ARC_PROP_ANG2,
	ARC_PROP_ROTATION,
	ARC_PROP_TYPE,
	ARC_PROP_START_ARROW,
	ARC_PROP_END_ARROW
};

enum {
	ARC_TYPE_ARC,
	ARC_TYPE_CHORD,
	ARC_TYPE_PIE
};

static void
goc_arc_set_property (GObject *gobject, guint param_id,
		      GValue const *value, GParamSpec *pspec)
{
	GocArc *arc = GOC_ARC (gobject);

	switch (param_id) {
	case ARC_PROP_XC:
		arc->xc = g_value_get_double (value);
		break;

	case ARC_PROP_YC:
		arc->yc = g_value_get_double (value);
		break;

	case ARC_PROP_XR:
		arc->xr = g_value_get_double (value);
		break;

	case ARC_PROP_YR:
		arc->yr = g_value_get_double (value);
		break;

	case ARC_PROP_ANG1:
		arc->ang1 = g_value_get_double (value);
		break;

	case ARC_PROP_ANG2:
		arc->ang2 = g_value_get_double (value);
		break;

	case ARC_PROP_ROTATION:
		arc->rotation = g_value_get_double (value);
		break;

	case ARC_PROP_TYPE:
		arc->type = g_value_get_int (value);
		break;

	case ARC_PROP_START_ARROW:
		arc->start_arrow = *((GOArrow *) g_value_peek_pointer (value));
		break;

	case ARC_PROP_END_ARROW:
		arc->end_arrow = *((GOArrow *) g_value_peek_pointer (value));
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
	goc_item_bounds_changed (GOC_ITEM (gobject));
}

static void
goc_arc_get_property (GObject *gobject, guint param_id,
		      GValue *value, GParamSpec *pspec)
{
	GocArc *arc = GOC_ARC (gobject);

	switch (param_id) {
	case ARC_PROP_XC:
		g_value_set_double (value, arc->xc);
		break;

	case ARC_PROP_YC:
		g_value_set_double (value, arc->yc);
		break;

	case ARC_PROP_XR:
		g_value_set_double (value, arc->xr);
		break;

	case ARC_PROP_YR:
		g_value_set_double (value, arc->yr);
		break;

	case ARC_PROP_ANG1:
		g_value_set_double (value, arc->ang1);
		break;

	case ARC_PROP_ANG2:
		g_value_set_double (value, arc->ang2);
		break;

	case ARC_PROP_ROTATION:
		g_value_set_double (value, arc->rotation);
		break;

	case ARC_PROP_TYPE:
		g_value_set_int (value, arc->type);
		break;

	case ARC_PROP_START_ARROW:
		g_value_set_boxed (value, &arc->start_arrow);
		break;

	case ARC_PROP_END_ARROW:
		g_value_set_boxed (value, &arc->end_arrow);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
}

static void
goc_arc_start (GocItem const *item, double *x, double *y, double *phi)
{
	GocArc *arc = GOC_ARC (item);
	double x1, y1, s;
	double sign = (item->canvas && goc_canvas_get_direction (item->canvas) == GOC_DIRECTION_RTL)? -1.: 1.;

	x1 = arc->xr * cos (atan2 (arc->xr / arc->yr * sin (arc->ang1), cos (arc->ang1)));
	y1 = arc->yr * sin (atan2 (arc->xr / arc->yr * sin (arc->ang1), cos (arc->ang1)));
	s = sqrt (x1 * x1 + y1 * y1);

	(*x) = arc->xc + s * cos (arc->ang1 - arc->rotation) * sign;
	(*y) = arc->yc - s * sin (arc->ang1 - arc->rotation);
	(*phi) = atan2 (arc->yr * arc->yr * x1, arc->xr * arc->xr * y1) + arc->rotation;
	if (-1 == sign) {
		(*phi) += M_PI;
	}
}

static void
goc_arc_end (GocItem const *item, double *x, double *y, double *phi)
{
	GocArc *arc = GOC_ARC (item);
	double x1, y1, s;
	double sign = (item->canvas && goc_canvas_get_direction (item->canvas) == GOC_DIRECTION_RTL)? -1.: 1.;

	x1 = arc->xr * cos (atan2 (arc->xr / arc->yr * sin (arc->ang2), cos (arc->ang2)));
	y1 = arc->yr * sin (atan2 (arc->xr / arc->yr * sin (arc->ang2), cos (arc->ang2)));
	s = sqrt (x1 * x1 + y1 * y1);

	(*x) = arc->xc + s * cos (arc->ang2 - arc->rotation) * sign;
	(*y) = arc->yc - s * sin (arc->ang2 - arc->rotation);
	(*phi) = atan2 (arc->yr * arc->yr * x1, arc->xr * arc->xr * y1) + arc->rotation + M_PI;
	if (-1 == sign) {
		(*phi) += M_PI;
	}
}

static void
prepare_draw_arrow (GocItem const *item, cairo_t *cr, gboolean end, gboolean flag)
{
	double w, x, y, phi;
	GocArc *arc = GOC_ARC (item);
	GOArrow const *arrow;
	GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (item));
	double sign = (item->canvas && goc_canvas_get_direction (item->canvas) == GOC_DIRECTION_RTL)? -1.: 1.;
	double rsign = sign;

	w = style->line.width? style->line.width / 2.0: 0.5;

	if (end) {
		arrow = &arc->end_arrow;
		goc_arc_end (item, &x, &y, &phi);
	} else {
		arrow = &arc->start_arrow;
		goc_arc_start (item, &x, &y, &phi);
	}

	cairo_save (cr);
	if (1 == flag) {
		goc_group_cairo_transform (item->parent, cr, arc->xc, arc->yc);
		sign = 1;
	} else {
		cairo_translate (cr, arc->xc, arc->yc);
		rsign = 1;
	}

#warning "FIXME: this should use go_arrow_draw"

	switch (arrow->typ) {
	case GO_ARROW_KITE:
		cairo_save (cr);
		cairo_translate (cr, (x - arc->xc) * sign, y - arc->yc);
		cairo_rotate (cr, phi * rsign);
		cairo_move_to (cr, -arrow->a * sign,  w);
		cairo_line_to (cr, -arrow->b * sign, w + arrow->c);
		if (w > 0.5) {
			cairo_line_to (cr, 0., w);
			cairo_line_to (cr, 0., -w);
		} else {
			cairo_line_to (cr, 0., 0.);
		}
		cairo_line_to (cr, -arrow->b * sign, -w - arrow->c);
		cairo_line_to (cr, -arrow->a * sign, -w);
		cairo_close_path (cr);
		cairo_restore (cr);
		break;

	case GO_ARROW_OVAL:
		cairo_save (cr);
		cairo_translate (cr, (x - arc->xc) * sign, y - arc->yc);
		cairo_rotate (cr, phi * rsign);
		if (arrow->a > 0 && arrow->b > 0) {
			cairo_scale (cr, arrow->a * sign, arrow->b);
			cairo_move_to (cr, 0., 0.);
			cairo_arc (cr, 0., 0., 1., 0., 2 * M_PI);
		} else
			cairo_move_to (cr, 0., 0.);
		cairo_close_path (cr);
		cairo_restore (cr);
		break;

	default:
		g_assert_not_reached ();
	}

	cairo_restore (cr);

}

static gboolean
goc_arc_prepare_draw (GocItem const *item, cairo_t *cr, gboolean flag)
{
	GocArc *arc = GOC_ARC (item);
	double sign = (goc_canvas_get_direction (item->canvas) == GOC_DIRECTION_RTL)? -1: 1;
	double ecc = 1;
	double rsign = sign;

	if (0 == arc->xr || 0 == arc->yr || arc->ang1 == arc->ang2)
		return FALSE;

	cairo_save (cr);
	_goc_item_transform (item, cr, flag);
	if (1 == flag) {
		goc_group_cairo_transform (item->parent, cr, arc->xc, arc->yc);
		sign = 1;
	} else {
		cairo_translate (cr, arc->xc, arc->yc);
		rsign = 1;
	}
	cairo_rotate (cr, arc->rotation * rsign);
	ecc = arc->xr / arc->yr;
	cairo_scale (cr, arc->xr * sign, arc->yr);
	cairo_arc_negative (cr, 0., 0., 1., -atan2 (ecc * sin (arc->ang1), cos (arc->ang1)), -atan2 (ecc * sin (arc->ang2), cos (arc->ang2)));
	switch (arc->type) {
	case ARC_TYPE_PIE:
		cairo_line_to (cr, 0., 0.);
		/* Fall through */
	case ARC_TYPE_CHORD:
		cairo_close_path (cr);
	default:
		break;
	}
	cairo_restore (cr);

	return TRUE;
}

static void
goc_arc_update_bounds (GocItem *item)
{
	cairo_surface_t *surface;
	cairo_t *cr;
	GocArc *arc = GOC_ARC (item);

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
	cr = cairo_create (surface);

	if (goc_arc_prepare_draw (item, cr, 0)) {
		if (arc->start_arrow.typ != GO_ARROW_NONE)
			prepare_draw_arrow (item, cr, 0, 0);
		if (arc->end_arrow.typ != GO_ARROW_NONE)
			prepare_draw_arrow (item, cr, 1, 0);
		goc_styled_item_set_cairo_line (GOC_STYLED_ITEM (item), cr);
		cairo_stroke_extents (cr, &item->x0, &item->y0, &item->x1, &item->y1);
	}

	cairo_destroy (cr);
	cairo_surface_destroy (surface);
}

static double
goc_arc_distance (GocItem *item, double x, double y, GocItem **near_item)
{
	GocArc *arc = GOC_ARC (item);
	GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (item));
	double tmp_width = 0;
	double res = 20;
	double ppu = goc_canvas_get_pixels_per_unit (item->canvas);
	cairo_surface_t *surface;
	cairo_t *cr;

	if (0 == arc->xr || 0 == arc->yr || arc->ang1 == arc->ang2)
		return res;

	*near_item = item;
	tmp_width = style->line.width;
	if (style->line.width * ppu < 5)
		style->line.width = 5. / (ppu * ppu);
	else
		style->line.width /= ppu;
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
	cr = cairo_create (surface);

	if (goc_styled_item_set_cairo_line (GOC_STYLED_ITEM (item), cr) &&
		(arc->start_arrow.typ != GO_ARROW_NONE || arc->end_arrow.typ != GO_ARROW_NONE)) {
			if (arc->start_arrow.typ != GO_ARROW_NONE)
				prepare_draw_arrow (item, cr, 0, 0);
			if (arc->end_arrow.typ != GO_ARROW_NONE)
				prepare_draw_arrow (item, cr, 1, 0);
			if (cairo_in_fill (cr, x, y)) {
				res = 0;
			}
	}

	if (goc_arc_prepare_draw (item, cr, 0)) {
		/* Filled OR both fill and stroke are none */
		if ((arc->type > 0 && style->fill.type != GO_STYLE_FILL_NONE) ||
			(style->fill.type == GO_STYLE_FILL_NONE && !goc_styled_item_set_cairo_line (GOC_STYLED_ITEM (item), cr))) {
			if (cairo_in_fill (cr, x, y))
				res = 0;
		}
		if (goc_styled_item_set_cairo_line (GOC_STYLED_ITEM (item), cr) && cairo_in_stroke (cr, x, y))
			res = 0;
	}

	cairo_destroy (cr);
	cairo_surface_destroy (surface);
	style->line.width = tmp_width;
	return res;
}

static void
goc_arc_draw (GocItem const *item, cairo_t *cr)
{
	GocArc *arc = GOC_ARC (item);


	cairo_save(cr);

	if (go_styled_object_set_cairo_line (GO_STYLED_OBJECT (item), cr)) {
		if (arc->start_arrow.typ != GO_ARROW_NONE)
			prepare_draw_arrow (item, cr, 0, 1);
		if (arc->end_arrow.typ != GO_ARROW_NONE)
			prepare_draw_arrow (item, cr, 1, 1);
		cairo_fill_preserve (cr);
		cairo_stroke (cr);
	}

	if (goc_arc_prepare_draw (item, cr, 1)) {
		if (arc->type > 0)
			go_styled_object_fill (GO_STYLED_OBJECT (item), cr, TRUE);
		if (goc_styled_item_set_cairo_line (GOC_STYLED_ITEM (item), cr)) {
			cairo_stroke (cr);
		} else {
			cairo_new_path (cr);
		}
	}
	cairo_restore(cr);
}

static void
goc_arc_copy (GocItem *dest, GocItem *source)
{
	GocArc *src = GOC_ARC (source), *dst = GOC_ARC (dest);

	dst->xc = src->xc;
	dst->yc = src->yc;
	dst->xr = src->xr;
	dst->yr = src->yr;
	dst->ang1 = src->ang1;
	dst->ang2 = src->ang2;
	dst->type = src->type;
	dst->start_arrow = src->start_arrow;
	dst->end_arrow = src->end_arrow;
	parent_class->copy (dest, source);
}

static void
goc_arc_init_style (G_GNUC_UNUSED GocStyledItem *item, GOStyle *style)
{
	GocArc *arc = GOC_ARC(item);

	style->interesting_fields = GO_STYLE_OUTLINE | GO_STYLE_FILL;
	if (style->line.auto_dash)
		style->line.dash_type = GO_LINE_SOLID;
	if (style->line.auto_color)
		style->line.color = GO_COLOR_BLACK;
	if (style->line.auto_fore)
		style->line.fore  = 0;
	if (arc->type > 0) {
		if (style->fill.auto_type)
			style->fill.type  = GO_STYLE_FILL_PATTERN;
		if (style->fill.auto_fore)
			style->fill.pattern.fore = GO_COLOR_BLACK;
		if (style->fill.auto_back)
			style->fill.pattern.back = GO_COLOR_WHITE;
	}
}

static void
goc_arc_class_init (GocItemClass *item_klass)
{
	GObjectClass *obj_klass = (GObjectClass *) item_klass;
	GocStyledItemClass *gsi_klass = (GocStyledItemClass *) item_klass;
	parent_class = g_type_class_peek_parent (item_klass);

	gsi_klass->init_style = goc_arc_init_style;

	obj_klass->get_property = goc_arc_get_property;
	obj_klass->set_property = goc_arc_set_property;
	g_object_class_install_property (obj_klass, ARC_PROP_XC,
		g_param_spec_double ("xc",
			_("xc"),
			_("The arc center x coordinate"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, ARC_PROP_YC,
		g_param_spec_double ("yc",
			_("yc"),
			_("The arc center y coordinate"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, ARC_PROP_XR,
		g_param_spec_double ("xr",
			_("xr"),
			_("The arc x radius"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, ARC_PROP_YR,
		g_param_spec_double ("yr",
			_("yr"),
			_("The arc y radius"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, ARC_PROP_ANG1,
		g_param_spec_double ("ang1",
			_("ang1"),
			_("The arc start angle"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, ARC_PROP_ANG2,
		g_param_spec_double ("ang2",
			_("ang2"),
			_("The arc end angle"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, ARC_PROP_ROTATION,
		g_param_spec_double ("rotation",
			_("Rotation"),
			_("The rotation around center position"),
			0., 2 * M_PI, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, ARC_PROP_TYPE,
		g_param_spec_int ("type",
			_("Type"),
			_("The type of arc: arc, chord or pie"),
			0, 2, 0,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));

	g_object_class_install_property (obj_klass, ARC_PROP_START_ARROW,
		g_param_spec_boxed ("start-arrow",
			_("Start Arrow"),
			_("Arrow for line's start"),
			GO_ARROW_TYPE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, ARC_PROP_END_ARROW,
		g_param_spec_boxed ("end-arrow",
			_("End Arrow"),
			_("Arrow for line's end"),
			GO_ARROW_TYPE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));

	item_klass->update_bounds = goc_arc_update_bounds;
	item_klass->distance = goc_arc_distance;
	item_klass->draw = goc_arc_draw;
	item_klass->copy = goc_arc_copy;
}

GSF_CLASS (GocArc, goc_arc,
	   goc_arc_class_init, NULL,
	   GOC_TYPE_STYLED_ITEM)
