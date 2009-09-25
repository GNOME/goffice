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

	g_return_val_if_fail (GO_IS_STYLED_OBJECT (so), FALSE);
	style = go_styled_object_get_style (GO_STYLED_OBJECT (so));
	return go_style_set_cairo_line (style, cr);
}

gboolean
go_styled_object_set_cairo_fill (GOStyledObject const *so, cairo_t *cr)
{
	GOStyle const *style;
	cairo_pattern_t *pat = NULL;

	g_return_val_if_fail (GO_IS_STYLED_OBJECT (so), FALSE);
	style = go_styled_object_get_style (GO_STYLED_OBJECT (so));
	if (style->fill.type == GO_STYLE_FILL_NONE)
		return FALSE;
	pat = go_style_create_cairo_pattern (style, cr);
	if (pat) {
		cairo_set_source (cr, pat);
		cairo_pattern_destroy (pat);
	}
	return TRUE;
}

