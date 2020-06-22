/*
 * goc-text.c:
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
#include <pango/pangocairo.h>

/**
 * SECTION:goc-text
 * @short_description: Text.
 *
 * #GocText implements rich text drawing in the canvas.
**/

enum {
	TEXT_PROP_0,
	TEXT_PROP_X,
	TEXT_PROP_Y,
	TEXT_PROP_ROTATION,
	TEXT_PROP_ANCHOR,
	TEXT_PROP_TEXT,
	TEXT_PROP_ATTRIBUTES,
	TEXT_PROP_CLIP,
	TEXT_PROP_CLIP_WIDTH,
	TEXT_PROP_CLIP_HEIGHT,
	TEXT_PROP_WRAP_WIDTH
};

static GocItemClass *parent_class;

#define GO_TYPE_ANCHOR_TYPE go_anchor_type_get_type ()
GType go_anchor_type_get_type (void);

GType
go_anchor_type_get_type (void)
{
	static GType etype = 0;
	if (etype == 0) {
		static GEnumValue const values[] = {
			{ GO_ANCHOR_CENTER,	"GO_ANCHOR_CENTER",	"center" },
			{ GO_ANCHOR_NORTH_WEST,	"GO_ANCHOR_NORTH_WEST",	"north-west" },
			{ GO_ANCHOR_NORTH_EAST,	"GO_ANCHOR_NORTH_EAST",	"north-east" },
			{ GO_ANCHOR_SOUTH,	"GO_ANCHOR_SOUTH",	"south" },
			{ GO_ANCHOR_SOUTH_WEST,	"GO_ANCHOR_SOUTH_WEST",	"south-west" },
			{ GO_ANCHOR_SOUTH_EAST,	"GO_ANCHOR_SOUTH_EAST",	"south-east" },
			{ GO_ANCHOR_WEST,	"GO_ANCHOR_WEST",	"west" },
			{ GO_ANCHOR_EAST,	"GO_ANCHOR_EAST",	"east" },
			{ GO_ANCHOR_BASELINE_CENTER, "GO_ANCHOR_BASELINE_CENTER", "baseline-centered" },
			{ GO_ANCHOR_BASELINE_WEST, "GO_ANCHOR_BASELINE_WEST", "baseline-west" },
			{ GO_ANCHOR_BASELINE_EAST, "GO_ANCHOR_BASELINE_EAST", "baseline-east" },
			{ GO_ANCHOR_N,	"GO_ANCHOR_N",	"n" },
			{ GO_ANCHOR_NW,	"GO_ANCHOR_NW",	"nw" },
			{ GO_ANCHOR_NE,	"GO_ANCHOR_NE",	"ne" },
			{ GO_ANCHOR_S,	"GO_ANCHOR_S",	"s" },
			{ GO_ANCHOR_SW,	"GO_ANCHOR_SW",	"sw" },
			{ GO_ANCHOR_SE,	"GO_ANCHOR_SE",	"se" },
			{ GO_ANCHOR_W,	"GO_ANCHOR_W",	"w" },
			{ GO_ANCHOR_E,	"GO_ANCHOR_E",	"e" },
			{ GO_ANCHOR_B,	"GO_ANCHOR_B",	"b" },
			{ GO_ANCHOR_BW,	"GO_ANCHOR_BW",	"bw" },
			{ GO_ANCHOR_BE,	"GO_ANCHOR_BE",	"be" },
			{ 0, NULL, NULL }
		};
		etype = g_enum_register_static (g_intern_static_string ("GOAnchorType"), values);
	}
	return etype;
}

static void
goc_text_set_property (GObject *gobject, guint param_id,
		       GValue const *value, GParamSpec *pspec)
{
	GocText *text = GOC_TEXT (gobject);

	switch (param_id) {
	case TEXT_PROP_X:
		text->x = g_value_get_double (value);
		break;

	case TEXT_PROP_Y:
		text->y = g_value_get_double (value);
		break;

	case TEXT_PROP_ROTATION:
		text->rotation = g_value_get_double (value);
		break;

	case TEXT_PROP_ANCHOR:
		text->anchor =  (GOAnchorType) g_value_get_enum (value);
		break;

	case TEXT_PROP_TEXT:
		g_free (text->text);
		text->text = g_value_dup_string (value);
		break;

	case TEXT_PROP_ATTRIBUTES: {
		PangoAttrList *attrs = (PangoAttrList *) g_value_get_boxed (value);
		if (text->attributes)
			pango_attr_list_unref (text->attributes);
		text->attributes = (attrs)? pango_attr_list_copy (attrs): pango_attr_list_new ();
		break;
	}

	case TEXT_PROP_CLIP:
		text->clipped = g_value_get_boolean (value);
		break;

	case TEXT_PROP_CLIP_WIDTH:
		text->clip_width = g_value_get_double (value);
		break;

	case TEXT_PROP_CLIP_HEIGHT:
		text->clip_height = g_value_get_double (value);
		break;

	case TEXT_PROP_WRAP_WIDTH:
		text->wrap_width = g_value_get_double (value);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
	goc_item_bounds_changed (GOC_ITEM (gobject));
}

static void
goc_text_get_property (GObject *gobject, guint param_id,
		       GValue *value, GParamSpec *pspec)
{
	GocText *text = GOC_TEXT (gobject);

	switch (param_id) {
	case TEXT_PROP_X:
		g_value_set_double (value, text->x);
		break;

	case TEXT_PROP_Y:
		g_value_set_double (value, text->y);
		break;

	case TEXT_PROP_ROTATION:
		g_value_set_double (value, text->rotation);
		break;

	case TEXT_PROP_ANCHOR:
		g_value_set_enum (value, text->anchor);
		break;

	case TEXT_PROP_TEXT:
		if (text->text)
			g_value_set_string (value, text->text);
		break;

	case TEXT_PROP_ATTRIBUTES:
		if (text->attributes)
			g_value_set_boxed (value, text->attributes);
		break;

	case TEXT_PROP_CLIP:
		g_value_set_boolean (value, text->clipped);
		break;

	case TEXT_PROP_CLIP_WIDTH:
		break;
		g_value_set_double (value, text->clip_width);

	case TEXT_PROP_CLIP_HEIGHT:
		g_value_set_double (value, text->clip_height);
		break;

	case TEXT_PROP_WRAP_WIDTH:
		g_value_set_double (value, text->wrap_width);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
}

static void
goc_text_finalize (GObject *gobject)
{
	GocText *text = GOC_TEXT (gobject);

	g_free (text->text);
	if (text->attributes)
		pango_attr_list_unref (text->attributes);
	((GObjectClass *) parent_class)->finalize (gobject);
}

static void
goc_text_prepare_draw (GocItem *item, cairo_t *cr, gboolean flag)
{
	GocText *text = GOC_TEXT (item);
	PangoRectangle rect;
	double sign = (item->canvas && goc_canvas_get_direction (item->canvas) == GOC_DIRECTION_RTL)? -1.: 1.;
	double w, h, dx, dy;
	PangoLayout *pl;
	GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (item));
	if (!text->text)
		return;
	w = (text->clip_width > 0.)? MIN (text->clip_width, text->w): text->w;
	h = (text->clip_height > 0.)? MIN (text->clip_height, text->h): text->h;
	pl = pango_cairo_create_layout (cr);
	pango_layout_set_font_description (pl, style->font.font->desc);
	pango_layout_set_text (pl, text->text, -1);
	if (text->wrap_width > 0) {
		pango_layout_set_width (pl, text->wrap_width * PANGO_SCALE);
		pango_layout_set_wrap (pl, PANGO_WRAP_WORD_CHAR);
	}
	if (text->attributes)
		pango_layout_set_attributes (pl, text->attributes);

	pango_layout_get_extents (pl, NULL, &rect);
	text->w = (double) rect.width / PANGO_SCALE;
	text->h = (double) rect.height / PANGO_SCALE;
	item->x0 = (item->canvas && goc_canvas_get_direction (item->canvas) == GOC_DIRECTION_RTL)? text->x + text->w: text->x;
	item->y0 = text->y;
	dx = dy = 0;
	w = (text->clip_width > 0.)? MIN (text->clip_width, text->w): text->w;
	h = (text->clip_height > 0.)? MIN (text->clip_height, text->h): text->h;
	/* adjust horizontally */
	switch (text->anchor) {
	case GO_ANCHOR_CENTER:
	case GO_ANCHOR_NORTH:
	case GO_ANCHOR_SOUTH:
	case GO_ANCHOR_BASELINE_CENTER:
		dx = -w / 2.;
		break;
	case GO_ANCHOR_NORTH_WEST:
	case GO_ANCHOR_SOUTH_WEST:
	case GO_ANCHOR_WEST:
	case GO_ANCHOR_BASELINE_WEST:
		break;
	case GO_ANCHOR_NORTH_EAST:
	case GO_ANCHOR_SOUTH_EAST:
	case GO_ANCHOR_EAST:
	case GO_ANCHOR_BASELINE_EAST:
		dx = -w;
		break;
	default: /* should not occur */
		break;
	}
	/* adjust vertically */
	switch (text->anchor) {
	case GO_ANCHOR_NORTH:
	case GO_ANCHOR_NORTH_WEST:
	case GO_ANCHOR_NORTH_EAST:
		break;
	case GO_ANCHOR_CENTER:
	case GO_ANCHOR_WEST:
	case GO_ANCHOR_EAST:
		dy = -h / 2.;
		break;
	case GO_ANCHOR_SOUTH:
	case GO_ANCHOR_SOUTH_WEST:
	case GO_ANCHOR_SOUTH_EAST:
		dy = -h;
		break;
	case GO_ANCHOR_BASELINE_CENTER:
	case GO_ANCHOR_BASELINE_WEST:
	case GO_ANCHOR_BASELINE_EAST: {
		PangoLayoutIter* iter = pango_layout_get_iter (pl);
		dy -= (double) pango_layout_iter_get_baseline (iter) / PANGO_SCALE;
		pango_layout_iter_free (iter);
		break;
	}
	default: /* should not occur */
		break;
	}
	cairo_save (cr);
	_goc_item_transform (item, cr, flag);
	cairo_translate (cr, item->x0, item->y0);
	cairo_rotate (cr, text->rotation * sign);
	cairo_rectangle (cr, dx, dy, w, h);
	cairo_restore (cr);
	g_object_unref (pl);
}

static void
goc_text_update_bounds (GocItem *item)
{
	cairo_surface_t *surface;
	cairo_t *cr;

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
	cr = cairo_create (surface);
	goc_text_prepare_draw (item, cr, FALSE);
	cairo_stroke_extents (cr, &item->x0, &item->y0, &item->x1, &item->y1);
	cairo_destroy (cr);
	cairo_surface_destroy (surface);
}

static double
goc_text_distance (GocItem *item, double x, double y, GocItem **near_item)
{
	cairo_surface_t *surface;
	cairo_t *cr;
	double res = 20;

	*near_item = item;

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
	cr = cairo_create (surface);
	goc_text_prepare_draw (item, cr, FALSE);
	if (cairo_in_fill (cr, x, y) || cairo_in_stroke (cr, x, y))
		res = 0;
	cairo_destroy (cr);
	cairo_surface_destroy (surface);

	return res;
}

static void
goc_text_draw (GocItem const *item, cairo_t *cr)
{
	GocText *text = GOC_TEXT (item);
	double x = (item->canvas && goc_canvas_get_direction (item->canvas) == GOC_DIRECTION_RTL)? text->x + text->w: text->x,
	       y = text->y, dx = 0., dy = 0., h, w;
	double sign = (item->canvas && goc_canvas_get_direction (item->canvas) == GOC_DIRECTION_RTL)? -1.: 1.;

	PangoLayout *pl;
	GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (item));
	if (!text->text)
		return;
	w = (text->clip_width > 0.)? MIN (text->clip_width, text->w): text->w;
	h = (text->clip_height > 0.)? MIN (text->clip_height, text->h): text->h;
	pl = pango_cairo_create_layout (cr);
	pango_layout_set_font_description (pl, style->font.font->desc);
	pango_layout_set_text (pl, text->text, -1);
	if (text->wrap_width > 0) {
		pango_layout_set_width (pl, text->wrap_width * PANGO_SCALE);
		pango_layout_set_wrap (pl, PANGO_WRAP_WORD_CHAR);
	}
	if (text->attributes)
		pango_layout_set_attributes (pl, text->attributes);
	/* adjust horizontally */
	switch (text->anchor) {
	case GO_ANCHOR_CENTER:
	case GO_ANCHOR_NORTH:
	case GO_ANCHOR_SOUTH:
	case GO_ANCHOR_BASELINE_CENTER:
		dx = -w / 2.;
		break;
	case GO_ANCHOR_NORTH_WEST:
	case GO_ANCHOR_SOUTH_WEST:
	case GO_ANCHOR_WEST:
	case GO_ANCHOR_BASELINE_WEST:
		break;
	case GO_ANCHOR_NORTH_EAST:
	case GO_ANCHOR_SOUTH_EAST:
	case GO_ANCHOR_EAST:
	case GO_ANCHOR_BASELINE_EAST:
		dx = -w;
		break;
	default: /* should not occur */
		break;
	}
	/* adjust vertically */
	switch (text->anchor) {
	case GO_ANCHOR_NORTH:
	case GO_ANCHOR_NORTH_WEST:
	case GO_ANCHOR_NORTH_EAST:
		break;
	case GO_ANCHOR_CENTER:
	case GO_ANCHOR_WEST:
	case GO_ANCHOR_EAST:
		dy = -h / 2.;
		break;
	case GO_ANCHOR_SOUTH:
	case GO_ANCHOR_SOUTH_WEST:
	case GO_ANCHOR_SOUTH_EAST:
		dy = -h;
		break;
	case GO_ANCHOR_BASELINE_CENTER:
	case GO_ANCHOR_BASELINE_WEST:
	case GO_ANCHOR_BASELINE_EAST: {
		PangoLayoutIter* iter = pango_layout_get_iter (pl);
		dy -= (double) pango_layout_iter_get_baseline (iter) / PANGO_SCALE;
		pango_layout_iter_free (iter);
		break;
	}
	default: /* should not occur */
		break;
	}
	cairo_save (cr);
	_goc_item_transform (item, cr, TRUE);
//	cairo_set_source_rgba (cr, GO_COLOR_TO_CAIRO (style->font.color));
	cairo_set_source_rgb (cr, 0.,0.,0.);
	goc_group_cairo_transform (item->parent, cr, x, y);
	cairo_rotate (cr, text->rotation * sign);
	cairo_translate (cr, dx, dy);
	cairo_rectangle (cr, 0., 0., w, h);
	cairo_clip (cr);
	pango_cairo_show_layout (cr, pl);
	cairo_new_path (cr);
	cairo_restore (cr);
	g_object_unref (pl);
}

static void
goc_text_copy (GocItem *dest, GocItem *source)
{
	GocText *src = GOC_TEXT (source), *dst = GOC_TEXT (dest);

	dst->x = src->x;
	dst->y = src->y;
	dst->w = src->w;
	dst->h = src->h;
	dst->rotation = src->rotation;
	dst->clipped = src->clipped;
	dst->clip_width = src->clip_width;
	dst->clip_height = src->clip_height;
	dst->wrap_width = src->wrap_width;
	dst->text = g_strdup (src->text);
	dst->anchor = src->anchor;
	dst->attributes = pango_attr_list_copy (src->attributes);
	parent_class->copy (dest, source);
}

static void
goc_text_init_style (G_GNUC_UNUSED GocStyledItem *item, GOStyle *style)
{
	style->interesting_fields = GO_STYLE_FONT;
	/* we might add more to display some extra decoration
	  without using a separate rectangle item */
}

static void
goc_text_style_changed (GOStyledObject *obj)
{
	goc_item_bounds_changed (GOC_ITEM (obj));
}

static void
goc_text_class_init (GocItemClass *item_klass)
{
	GObjectClass *obj_klass = (GObjectClass *) item_klass;
	GocStyledItemClass *gsi_klass = (GocStyledItemClass *) item_klass;
	parent_class = g_type_class_peek_parent (item_klass);

	obj_klass->finalize = goc_text_finalize;
	obj_klass->get_property = goc_text_get_property;
	obj_klass->set_property = goc_text_set_property;
	g_object_class_install_property (obj_klass, TEXT_PROP_X,
		g_param_spec_double ("x",
			_("x"),
			_("The text horizontal position"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, TEXT_PROP_Y,
		g_param_spec_double ("y",
			_("y"),
			_("The text position"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, TEXT_PROP_ROTATION,
		g_param_spec_double ("rotation",
			_("Rotation"),
			_("The rotation around the anchor"),
			0., 2 * M_PI, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, TEXT_PROP_ANCHOR,
		g_param_spec_enum ("anchor",
			_("Anchor"),
			_("The anchor point for the text"),
			GO_TYPE_ANCHOR_TYPE, GO_ANCHOR_CENTER,
		        GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, TEXT_PROP_TEXT,
		g_param_spec_string ("text",
			_("Text"),
			_("The text to display"), NULL,
 			GSF_PARAM_STATIC | G_PARAM_READWRITE));
       g_object_class_install_property (obj_klass, TEXT_PROP_ATTRIBUTES,
                 g_param_spec_boxed ("attributes", _("Attributes"), _("The attributes list as a PangoAttrList"),
			PANGO_TYPE_ATTR_LIST,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, TEXT_PROP_CLIP,
		g_param_spec_boolean ("clip",
			_("Clip"),
			_("Whether to clip or not"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, TEXT_PROP_CLIP_WIDTH,
		g_param_spec_double ("clip-width",
			_("Clip width"),
			_("Clip width for the text"),
			0., G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, TEXT_PROP_CLIP_HEIGHT,
		g_param_spec_double ("clip-height",
			_("Clip height"),
			_("Clip height for the text"),
			0., G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, TEXT_PROP_WRAP_WIDTH,
		g_param_spec_double ("wrap-width",
			_("Wrap width"),
			_("Wrap width for the text"),
			0., G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));

	gsi_klass->init_style = goc_text_init_style;

	item_klass->update_bounds = goc_text_update_bounds;
	item_klass->distance = goc_text_distance;
	item_klass->draw = goc_text_draw;
	item_klass->copy = goc_text_copy;
}

static void
goc_text_item_so_init (GOStyledObjectClass *iface)
{
	iface->style_changed = goc_text_style_changed;
	/* Rest of interface is defined by parent class.  */
}

GSF_CLASS_FULL (GocText, goc_text, NULL, NULL,
		goc_text_class_init, NULL, NULL,
		GOC_TYPE_STYLED_ITEM, 0,
		GSF_INTERFACE (goc_text_item_so_init, GO_TYPE_STYLED_OBJECT))
