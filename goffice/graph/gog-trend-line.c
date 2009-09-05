/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-trend-line.c :
 *
 * Copyright (C) 2006 Jean Brefort (jean.brefort@normalesup.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
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

static void
gog_trend_line_init_style (GogStyledObject *gso, GOStyle *style)
{
	style->interesting_fields = GO_STYLE_LINE;
	gog_theme_fillin_style (gog_object_get_theme (GOG_OBJECT (gso)),
		style, GOG_OBJECT (gso), 0, FALSE);
}

static char const *
gog_trend_line_type_name (GogObject const *gobj)
{
	return N_("Trend Line");
}

static void
gog_trend_line_class_init (GogObjectClass *gog_klass)
{
	GogStyledObjectClass *style_klass = (GogStyledObjectClass *) gog_klass;

	style_klass->init_style = gog_trend_line_init_style;
	gog_klass->type_name	= gog_trend_line_type_name;
}

GSF_CLASS (GogTrendLine, gog_trend_line,
	   gog_trend_line_class_init, NULL,
	   GOG_TYPE_STYLED_OBJECT)

GogTrendLine *
gog_trend_line_new_by_type (GogTrendLineType const *type)
{
	GogTrendLine *res;

	g_return_val_if_fail (type != NULL, NULL);

	res = gog_trend_line_new_by_name (type->engine);
	if (res != NULL && type->properties != NULL)
		g_hash_table_foreach (type->properties,
			(GHFunc) gog_object_set_arg, res);
	return res;
}
