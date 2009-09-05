/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * goc-styled-item.c :
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

enum {
	STYLED_ITEM_PROP_0,
	STYLED_ITEM_PROP_STYLE,
	STYLED_ITEM_SCALE_LINE_WIDTH
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

	case STYLED_ITEM_PROP_STYLE :
		resize = go_styled_object_set_style (GO_STYLED_OBJECT (gsi),
	      			g_value_get_object (value));
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
	if (resize) {
		goc_item_invalidate (item);
		goc_item_bounds_changed (item);
	}
	goc_item_invalidate (item);
}

static void
goc_styled_item_get_property (GObject *obj, guint param_id,
				GValue *value, GParamSpec *pspec)
{
	GocStyledItem *gsi = GOC_STYLED_ITEM (obj);

	switch (param_id) {
	case STYLED_ITEM_PROP_STYLE :
		g_value_set_object (value, gsi->style);
		break;

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

	(*parent_klass->finalize) (obj);
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
goc_styled_item_class_init (GocItemClass *goc_klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) goc_klass;
	GocStyledItemClass *style_klass = (GocStyledItemClass *) goc_klass;

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
	goc_klass->parent_changed   = goc_styled_item_parent_changed;
	style_klass->init_style	    = goc_styled_item_init_style;

	g_object_class_install_property (gobject_klass, STYLED_ITEM_PROP_STYLE,
		g_param_spec_object ("style",
			_("Style"),
			_("A pointer to the GOStyle object"),
			go_style_get_type (),
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, STYLED_ITEM_SCALE_LINE_WIDTH,
		g_param_spec_boolean ("scale-line-width",
			_("Scale line width"),
			_("Whether to scale the line width or not when zooming"),
			TRUE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
}

static void
goc_styled_item_init (GocStyledItem *gsi)
{
	gsi->style = GO_STYLE (g_object_new (go_style_get_type (), NULL)); /* use the defaults */
	gsi->scale_line_width = TRUE;
}

static gboolean
goc_styled_item_set_style (GOStyledObject *gsi,
			     GOStyle *style)
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
	g_signal_emit (G_OBJECT (gsi),
		goc_styled_item_signals [STYLE_CHANGED], 0, GOC_STYLED_ITEM (gsi)->style);
}

static GODoc*
goc_styled_item_get_document (GOStyledObject *gsi)
{
	return goc_canvas_get_document (GOC_ITEM (gsi)->canvas);
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
	   GOC_TYPE_ITEM, 0,
	   GSF_INTERFACE (goc_styled_item_so_init, GO_TYPE_STYLED_OBJECT))

gboolean
goc_styled_item_set_cairo_line  (GocStyledItem const *gsi, cairo_t *cr)
{
	double width = 0.;
	gboolean result;
	g_return_val_if_fail (GOC_IS_STYLED_ITEM (gsi), FALSE);

	/* scale the line width */
	if (gsi->scale_line_width) {
		width = gsi->style->line.width;
		gsi->style->line.width *= goc_canvas_get_pixels_per_unit (GOC_ITEM (gsi)->canvas);
	}
	result = go_styled_object_set_cairo_line (GO_STYLED_OBJECT (gsi), cr);
	/* restore the line width */
	if (gsi->scale_line_width)
		gsi->style->line.width = width;
	return result;
}
