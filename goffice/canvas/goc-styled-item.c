/*
 * goc-styled-item.c:
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

#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>
#include <string.h>

/**
 * SECTION:goc-styled-item
 * @short_description: Styled items
 *
 * The virtual base object for canvas items with style.
 **/

/**
 * GocStyledItemClass:
 * @init_style: style initialization handler.
 * @reserved1: reserved for future expansion
 * @reserved2: reserved for future expansion
 **/
enum {
	STYLED_ITEM_PROP_0,
	STYLED_ITEM_PROP_STYLE,
	STYLED_ITEM_PROP_SCALE_LINE_WIDTH
};

enum {
	STYLE_CHANGED,
	LAST_SIGNAL
};

static void goc_styled_item_style_changed (GOStyledObject *obj);

static gulong goc_styled_item_signals [LAST_SIGNAL] = { 0, };
static GObjectClass *parent_klass;

static void
goc_styled_item_set_property (GObject *obj, guint param_id,
				GValue const *value, GParamSpec *pspec)
{
	GocStyledItem *gsi = GOC_STYLED_ITEM (obj);
	GocItem *item = GOC_ITEM (obj);
	gboolean resize = FALSE;

	switch (param_id) {

	case STYLED_ITEM_PROP_STYLE:
		resize = go_styled_object_set_style (GO_STYLED_OBJECT (gsi),
	      			g_value_get_object (value));
		break;

	case STYLED_ITEM_PROP_SCALE_LINE_WIDTH:
		gsi->scale_line_width =  g_value_get_boolean (value);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
	if (resize)
		goc_item_bounds_changed (item);
	goc_item_invalidate (item);
}

static void
goc_styled_item_get_property (GObject *obj, guint param_id,
				GValue *value, GParamSpec *pspec)
{
	GocStyledItem *gsi = GOC_STYLED_ITEM (obj);

	switch (param_id) {
	case STYLED_ITEM_PROP_STYLE:
		g_value_set_object (value, gsi->style);
		break;

	case STYLED_ITEM_PROP_SCALE_LINE_WIDTH:
		g_value_set_boolean (value, gsi->scale_line_width);

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
goc_styled_item_finalize (GObject *obj)
{
	GocStyledItem *gsi = GOC_STYLED_ITEM (obj);

	if (gsi->style != NULL) {
		g_object_unref (gsi->style);
		gsi->style = NULL;
	}

	parent_klass->finalize (obj);
}

static void
goc_styled_item_parent_changed (GocItem *item)
{
	GocStyledItem *gsi = GOC_STYLED_ITEM (item);
	go_styled_object_apply_theme (GO_STYLED_OBJECT (gsi), gsi->style);
}

static void
goc_styled_item_init_style (GocStyledItem *gsi, GOStyle *style)
{
	style->interesting_fields = GO_STYLE_OUTLINE | GO_STYLE_FILL; /* default */
}

static void
goc_styled_item_copy (GocItem *dest, GocItem *source)
{
	GOStyle *style = go_style_dup (((GocStyledItem *) source)->style);
	g_object_set (dest, "style", style, NULL);
	g_object_unref (style);
}

static void
goc_styled_item_class_init (GocItemClass *goc_klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) goc_klass;
	GocStyledItemClass *style_klass = (GocStyledItemClass *) goc_klass;

/**
 * GocStyledItem::style-changed:
 * @gsi: the object on which the signal is emitted
 * @style: the new #GOStyle.
 *
 * The ::style-changed signal is emitted when a new style
 * has been set on a styled item.
 **/
	goc_styled_item_signals [STYLE_CHANGED] = g_signal_new ("style-changed",
		G_TYPE_FROM_CLASS (goc_klass),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GocStyledItemClass, style_changed),
		NULL, NULL,
		g_cclosure_marshal_VOID__OBJECT,
		G_TYPE_NONE,
		1, G_TYPE_OBJECT);

	parent_klass = g_type_class_peek_parent (goc_klass);

	gobject_klass->set_property = goc_styled_item_set_property;
	gobject_klass->get_property = goc_styled_item_get_property;
	gobject_klass->finalize	    = goc_styled_item_finalize;
	style_klass->init_style	    = goc_styled_item_init_style;
	goc_klass->copy	    		= goc_styled_item_copy;

	g_object_class_install_property (gobject_klass, STYLED_ITEM_PROP_STYLE,
		g_param_spec_object ("style",
			_("Style"),
			_("A pointer to the GOStyle object"),
			go_style_get_type (),
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, STYLED_ITEM_PROP_SCALE_LINE_WIDTH,
		g_param_spec_boolean ("scale-line-width",
			_("Scale line width"),
			_("Whether to scale the line width when zooming"),
			TRUE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
}

static void
goc_styled_item_init (GocStyledItem *gsi)
{
	gsi->style = GO_STYLE (g_object_new (go_style_get_type (), NULL)); /* use the defaults */
	gsi->scale_line_width = TRUE;
	g_signal_connect (G_OBJECT (gsi), "notify::parent", G_CALLBACK (goc_styled_item_parent_changed), NULL);
}

static gboolean
goc_styled_item_set_style (GOStyledObject *gsi, GOStyle *style)
{
	gboolean resize;
	GocStyledItem *ggsi;

	g_return_val_if_fail (GO_IS_STYLED_OBJECT (gsi), FALSE);
	ggsi = GOC_STYLED_ITEM (gsi);
	if (ggsi->style == style)
		return FALSE;
	style = go_style_dup (style);

	/* which fields are we interested in for this object */
	go_styled_object_apply_theme (gsi, style);
	resize = go_style_is_different_size (ggsi->style, style);
	if (ggsi->style != NULL)
		g_object_unref (ggsi->style);
	ggsi->style = style;
	go_styled_object_style_changed (gsi);

	return resize;
}

static GOStyle *
goc_styled_item_get_style (GOStyledObject *gsi)
{
	g_return_val_if_fail (GO_IS_STYLED_OBJECT (gsi), NULL);
	return GOC_STYLED_ITEM (gsi)->style;
}

static GOStyle *
goc_styled_item_get_auto_style (GOStyledObject *gsi)
{
	GOStyle *res = go_style_dup (GOC_STYLED_ITEM (gsi)->style);
	go_style_force_auto (res);
	go_styled_object_apply_theme (gsi, res);
	return res;
}

static void
goc_styled_item_apply_theme (GOStyledObject *gsi, GOStyle *style)
{
	/* FIXME: do we need themes? */
	GocStyledItemClass *klass = GOC_STYLED_ITEM_GET_CLASS (gsi);
	(klass->init_style) (GOC_STYLED_ITEM (gsi), style);
}

static void
goc_styled_item_style_changed (GOStyledObject *gsi)
{
	GocItem *item = GOC_ITEM (gsi);
	goc_item_bounds_changed (item); /* FIXME: should not be there, remove when we branch */
	g_signal_emit (G_OBJECT (gsi),
		goc_styled_item_signals [STYLE_CHANGED], 0, GOC_STYLED_ITEM (gsi)->style);
	goc_item_invalidate (item);
}

static GODoc*
goc_styled_item_get_document (GOStyledObject *gsi)
{
	return (GOC_ITEM (gsi)->canvas)? goc_canvas_get_document (GOC_ITEM (gsi)->canvas): NULL;
}

static void
goc_styled_item_so_init (GOStyledObjectClass *iface)
{
	iface->set_style = goc_styled_item_set_style;
	iface->get_style = goc_styled_item_get_style;
	iface->get_auto_style = goc_styled_item_get_auto_style;
	iface->style_changed = goc_styled_item_style_changed;
	iface->apply_theme = goc_styled_item_apply_theme;
	iface->get_document = goc_styled_item_get_document;
}

GSF_CLASS_FULL (GocStyledItem, goc_styled_item, NULL, NULL,
	   goc_styled_item_class_init, NULL, goc_styled_item_init,
	   GOC_TYPE_ITEM, G_TYPE_FLAG_ABSTRACT,
	   GSF_INTERFACE (goc_styled_item_so_init, GO_TYPE_STYLED_OBJECT))

/**
 * goc_styled_item_set_cairo_line:
 * @gsi: #GocStyledItem
 * @cr: #cairo_t
 *
 * Prepares the cairo context @cr to draw a line according to the
 * item style and canvas scale. The line width is scaled only if
 * the scale-line-width property is set to %TRUE. This function calls
 * go_styled_object_set_cairo_line().
 *
 * If the item drawing used goc_group_cairo_transform(),
 * scale-line-width should be %FALSE to avoid scaling twice, or
 * go_styled_object_set_cairo_line() should be called directly instead.
 * Returns: %TRUE if the line is not invisible
 **/
gboolean
goc_styled_item_set_cairo_line  (GocStyledItem const *gsi, cairo_t *cr)
{
	double width = 0.;
	gboolean result;
	g_return_val_if_fail (GOC_IS_STYLED_ITEM (gsi), FALSE);

	/* scale the line width */
	if (gsi->scale_line_width && GOC_ITEM (gsi)->canvas) {
		width = gsi->style->line.width;
		gsi->style->line.width *= goc_canvas_get_pixels_per_unit (GOC_ITEM (gsi)->canvas);
	}
	result = go_styled_object_set_cairo_line (GO_STYLED_OBJECT (gsi), cr);
	/* restore the line width */
	if (gsi->scale_line_width)
		gsi->style->line.width = width;
	return result;
}

/**
 * goc_styled_item_get_scale_line_width:
 * @gsi: #GocStyledItem
 *
 * This function returns %TRUE if the line width needs to be scaled. It will
 * always return %FALSE if the line width is 0.
 * Returns: whether the line width needs to be scaled.
 **/
gboolean  goc_styled_item_get_scale_line_width  (GocStyledItem const *gsi)
{
	g_return_val_if_fail (GOC_IS_STYLED_ITEM (gsi), FALSE);
	return gsi->scale_line_width && gsi->style->line.width > 0.;
}

/**
 * goc_styled_item_set_scale_line_width:
 * @gsi: #GocStyledItem
 * @scale_line_width: boolean
 *
 * Sets whether the line width needs to be scaled according to the current
 * canvas resolution and the item transformation. It will be ignored if the
 * line width is 0. Default value is %TRUE.
 **/
void  goc_styled_item_set_scale_line_width  (GocStyledItem *gsi, gboolean scale_line_width)
{
	g_return_if_fail (GOC_IS_STYLED_ITEM (gsi));
	gsi->scale_line_width = scale_line_width;
}
