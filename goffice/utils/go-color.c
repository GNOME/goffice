/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-color.c :
 *
 * Copyright (C) 2003-2004 Jody Goldberg (jody@gnome.org)
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

#include <stdio.h>

/**
 * go_color_from_str :
 * @str: String to parse
 * @res: (out): the parsed color
 *
 * Returns: %TRUE if @str can be parsed as a color of the form R:G:B:A and the
 * 	result is stored in @res.
 **/
gboolean
go_color_from_str (gchar const *str, GOColor *res)
{
	unsigned r, g, b, a;

	if (sscanf (str, "%X:%X:%X:%X", &r, &g, &b, &a) == 4) {
		*res = GO_COLOR_FROM_RGBA (r, g, b, a);
		return TRUE;
	}
#ifdef GOFFICE_WITH_GTK
	 else {
		GdkRGBA color;
		if (gdk_rgba_parse (&color, str)) {
			*res = GO_COLOR_FROM_GDK_RGBA (color);
			return TRUE;
		}
	}
#endif
	*res = 0;
	return FALSE;
}

gchar *
go_color_as_str (GOColor color)
{
	unsigned r, g, b, a;

	GO_COLOR_TO_RGBA (color, &r, &g, &b, &a);
	return g_strdup_printf ("%X:%X:%X:%X", r, g, b, a);
}

/**
 * go_color_to_pango: (skip)
 * @color: #GOColor
 * @is_fore: %TRUE for foreground, %FALSE for background
 *
 * Returns: (transfer full): the newly created #PangoAttribute.
 **/
PangoAttribute *
go_color_to_pango (GOColor color, gboolean is_fore)
{
	guint16 r, g, b;
	r  = GO_COLOR_UINT_R (color);
	r |= (r << 8);
	g  = GO_COLOR_UINT_G (color);
	g |= (g << 8);
	b  = GO_COLOR_UINT_B (color);
	b |= (b << 8);

	if (is_fore)
		return pango_attr_foreground_new (r, g, b);
	else
		return pango_attr_background_new (r, g, b);
}

#ifdef GOFFICE_WITH_GTK
#include <gdk/gdk.h>

/**
 * go_color_to_gdk_rgba:
 * @color: #GOColor
 * @res: %GdkRGBA to change
 *
 * Returns: (transfer none): @res
 **/
GdkRGBA *
go_color_to_gdk_rgba (GOColor color, GdkRGBA *res)
{
	res->red    = GO_COLOR_DOUBLE_R (color);
	res->green  = GO_COLOR_DOUBLE_G (color);
	res->blue   = GO_COLOR_DOUBLE_B (color);
	res->alpha  = GO_COLOR_DOUBLE_A (color);

	return res;
}

/**
 * go_color_from_gdk_rgba:
 * @rgbacolor: #GdkRGBA to convert
 * @res: (out) (optional): resulting color
 *
 * Returns: resulting color
 **/
GOColor
go_color_from_gdk_rgba (GdkRGBA const *rgbacolor, GOColor *res)
{
	GOColor color;
	gint r, g, b, a;

	g_return_val_if_fail (rgbacolor != NULL, 0);

	r = CLAMP (rgbacolor->red * 256, 0, 255);
	g = CLAMP (rgbacolor->green * 256, 0, 255);
	b = CLAMP (rgbacolor->blue * 256, 0, 255);
	a = CLAMP (rgbacolor->alpha * 256, 0, 255);

	color = GO_COLOR_FROM_RGBA (r,g,b,a);

	if (res)
		*res = color;
	return color;
}
#endif /* GOFFICE_WITH_GTK */
