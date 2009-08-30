/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-styled-object.c :  
 *
 * Copyright (C) 2009 JJean Brefort (jean.brefort@normalesup.org)
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
#include <goffice/utils/go-styled-object.h>

GType
go_styled_object_get_type (void)
{
	static GType go_styled_object_type = 0;

	if (!go_styled_object_type) {
		static GTypeInfo const go_styled_object_info = {
			sizeof (GOStyledObjectClass),	/* class_size */
			NULL,		/* base_init */
			NULL,		/* base_finalize */
		};

		go_styled_object_type = g_type_register_static (G_TYPE_INTERFACE,
			"GOStyledObject", &go_styled_object_info, 0);
	}

	return go_styled_object_type;
}

/**
 * go_styled_object_set_style:
 * @gso: a #GOStyledObject
 * @style: a #GOStyle
 *
 * Sets a new style for @gso, and emits "style-changed" signal. This function
 * does not take ownership of @style.
 *
 * return value: %TRUE if new style may lead to change of object size, which
 * happens when changing font size for example. 
 **/
gboolean
go_styled_object_set_style (GOStyledObject *gso, GOStyle *style)
{
	GOStyledObjectClass *klass = GO_STYLED_OBJECT_GET_CLASS (gso);
	g_return_val_if_fail (klass != NULL, FALSE);
	return (klass->set_style)?
		klass->set_style (gso, style): FALSE;
}

/**
 * go_styled_object_get_style:
 * @gso: a #GOStyledObject
 *
 * Simply an accessor function that returns @gso->style, without referencing it.
 * 
 * return value: the styled object's #GOStyle
 **/
GOStyle*
go_styled_object_get_style(GOStyledObject *gso)
{
	GOStyledObjectClass *klass = GO_STYLED_OBJECT_GET_CLASS (gso);
	g_return_val_if_fail (klass != NULL, NULL);
	return (klass->get_style)?
		klass->get_style (gso): NULL;
}

/**
 * go_styled_object_get_auto_style:
 * @gso: a #GOStyledObject
 *
 * This function returns a new style that is initialized with the auto values for @gso.
 * Caller is responsible for the result.
 *
 * return value: a new #GOStyle
 **/
GOStyle*
go_styled_object_get_auto_style (GOStyledObject *gso)
{
	GOStyledObjectClass *klass = GO_STYLED_OBJECT_GET_CLASS (gso);
	g_return_val_if_fail (klass != NULL, NULL);
	return (klass->get_auto_style)?
		klass->get_auto_style (gso): NULL;
}

/**
 * go_styled_object_style_changed:
 * @gso: a #GOStyledObject
 *
 * Called when the style changed. Might emit a signal if meaningful.
 *
 **/
void
go_styled_object_style_changed (GOStyledObject *gso)
{
	GOStyledObjectClass *klass = GO_STYLED_OBJECT_GET_CLASS (gso);
	g_return_if_fail (klass != NULL);
	if (klass->style_changed)
		klass->style_changed (gso);
}

/**
 * go_styled_object_apply_theme:
 * @gso: a #GOStyledObject
 * @style: a #GOStyle that will be themed
 *
 * Apply appropriate theme @style if meaningful, i.e. properties with 
 * auto flag set to %TRUE should be changed to default theme value.
 *
 **/
void
go_styled_object_apply_theme (GOStyledObject *gso, GOStyle *style)
{
	GOStyledObjectClass *klass = GO_STYLED_OBJECT_GET_CLASS (gso);
	g_return_if_fail (klass != NULL);
	if (klass->apply_theme)
		klass->apply_theme (gso, style);
}

GODoc*
go_styled_object_get_document (GOStyledObject *gso)
{
	GOStyledObjectClass *klass = GO_STYLED_OBJECT_GET_CLASS (gso);
	g_return_val_if_fail (klass != NULL, NULL);
	return (klass->get_document)?
		klass->get_document (gso): NULL;
}

gboolean
go_styled_object_set_cairo_line (GOStyledObject const *so, cairo_t *cr)
{
	GOStyle const *style;
	double width;
	GOLineDashSequence *line_dash;

	g_return_val_if_fail (GO_IS_STYLED_OBJECT (so), FALSE);
	style = go_styled_object_get_style (GO_STYLED_OBJECT (so));
	if (style->line.dash_type == GO_LINE_NONE)
		return FALSE;
	width = (style->line.width)? style->line.width: 1.;
	cairo_set_line_width (cr, width);
	cairo_set_line_cap (cr, style->line.cap);
	cairo_set_line_join (cr, style->line.join);
	cairo_set_miter_limit (cr, style->line.miter_limit);
	line_dash = go_line_dash_get_sequence (style->line.dash_type, width);
	if (style->line.cap == CAIRO_LINE_CAP_BUTT && style->line.dash_type != GO_LINE_SOLID) {
		unsigned i;
		for (i = 0; i < line_dash->n_dash; i++)
			if (line_dash->dash[i] == 0.)
				line_dash->dash[i] = width;
	}
	if (line_dash != NULL)
		cairo_set_dash (cr,
				line_dash->dash,
				line_dash->n_dash,
				line_dash->offset);
	else
		cairo_set_dash (cr, NULL, 0, 0);
	switch (style->line.pattern) {
	case GO_PATTERN_SOLID:
		cairo_set_source_rgba (cr, GO_COLOR_TO_CAIRO (style->line.color));
		break;
	case GO_PATTERN_FOREGROUND_SOLID:
		cairo_set_source_rgba (cr, GO_COLOR_TO_CAIRO (style->line.fore));
		break;
	default: {
		GOPattern pat;
		double scalex = 1., scaley = 1.;
		cairo_pattern_t *cp;
		cairo_matrix_t mat;
		pat.fore = style->line.fore;
		pat.back = style->line.color;
		pat.pattern = style->line.pattern;
		cp = go_pattern_create_cairo_pattern (&pat, cr);
		cairo_user_to_device_distance (cr, &scalex, &scaley);
		cairo_matrix_init_scale (&mat, scalex, scaley);
		cairo_pattern_set_matrix (cp, &mat);
		cairo_set_source (cr, cp);
		break;
	}
	}
	return TRUE;
}

gboolean
go_styled_object_set_cairo_fill (GOStyledObject const *so, cairo_t *cr)
{
	GOStyle const *style;
	cairo_pattern_t *pat = NULL;

	g_return_val_if_fail (GO_IS_STYLED_OBJECT (so), FALSE);
	style = go_styled_object_get_style (GO_STYLED_OBJECT (so));
	pat = go_style_create_cairo_pattern (style, cr);
	if (pat) {
		cairo_set_source (cr, pat);
		cairo_pattern_destroy (pat);
	}
	return TRUE;
}

