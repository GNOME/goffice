/*
 * gog-series-lines.c :
 *
 * Copyright (C) 2005 Jean Brefort (jean.brefort@normalesup.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <goffice/goffice-config.h>
#include <goffice/goffice.h>

#include <gsf/gsf-impl-utils.h>
#include <string.h>

typedef GogStyledObjectClass GogSeriesLinesClass;
struct _GogSeriesLines {
	GogStyledObject base;

	gboolean	use_markers;
};

static void
gog_series_lines_init_style (GogStyledObject *gso, GOStyle *style)
{
	style->interesting_fields = (GOG_SERIES_LINES (gso)->use_markers)
		? GO_STYLE_LINE | GO_STYLE_MARKER
		: GO_STYLE_LINE;
	gog_theme_fillin_style (gog_object_get_theme (GOG_OBJECT (gso)),
		style, GOG_OBJECT (gso), 0, style->interesting_fields);
}

static void
gog_series_lines_update (GogObject *obj)
{
	gog_object_request_update (obj->parent);
}

static void
gog_series_lines_changed (GogObject *obj, gboolean size)
{
	gog_object_emit_changed (obj->parent, size);
}

static void
gog_series_lines_class_init (GogObjectClass *klass)
{
	GogStyledObjectClass *style_klass = (GogStyledObjectClass *) klass;

	klass->update = gog_series_lines_update;
	klass->changed = gog_series_lines_changed;
	style_klass->init_style = gog_series_lines_init_style;
}

GSF_CLASS (GogSeriesLines, gog_series_lines,
	   gog_series_lines_class_init, NULL,
	   GOG_TYPE_STYLED_OBJECT)

static void
path_move_to (void *closure, GOPathPoint const *point)
{
	gog_renderer_draw_marker (GOG_RENDERER (closure), point->x, point->y);
}

static void
path_curve_to (void *closure,
	       GOPathPoint const *point0,
	       GOPathPoint const *point1,
	       GOPathPoint const *point2)
{
	gog_renderer_draw_marker (GOG_RENDERER (closure), point2->x, point2->y);
}

static void
path_close_path (void *closure)
{
}

void gog_series_lines_stroke (GogSeriesLines *lines, GogRenderer *rend,
		GogViewAllocation const *bbox, GOPath *path, gboolean invert)
{
	GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (lines));

	if (invert) {
		GOMarker *marker;
		GOColor color;

		style = go_style_dup (style);
		style->line.color ^= 0xffffff00;
		marker = style->marker.mark;
		color = go_marker_get_outline_color (marker);
		go_marker_set_outline_color (marker, color ^ 0xffffff00);
		color = go_marker_get_fill_color (marker);
		go_marker_set_fill_color (marker, color ^ 0xffffff00);
	}
	gog_renderer_push_style (rend, style);
	gog_renderer_stroke_serie (rend, path);
	if ((style->interesting_fields & GO_STYLE_MARKER) != 0)
		go_path_interpret (path, GO_PATH_DIRECTION_FORWARD,
				   path_move_to,
				   path_move_to,
				   path_curve_to,
				   path_close_path,
				   rend);
	gog_renderer_pop_style (rend);
	if (invert)
		g_object_unref (style);
}

void
gog_series_lines_use_markers (GogSeriesLines *lines, gboolean use_markers)
{
	lines->use_markers = use_markers;
}
