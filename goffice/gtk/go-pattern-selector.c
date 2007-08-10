/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-pattern-selector.c :
 *
 * Copyright (C) 2006 Emmanuel Pacaud (emmanuel.pacaud@lapp.in2p3.fr)
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

#include "go-pattern-selector.h"
#include "goffice-gtk.h"

#include <goffice/utils/go-color.h>
#include <libart_lgpl/libart.h>

typedef struct {
	GOColor foreground;
	GOColor background;
} GOPatternSelectorState;

static void
go_pattern_palette_render_func (cairo_t *cr,
				GdkRectangle const *area,
				int index, 
				gpointer data)
{
	int const W = 8, H = 8;
	GOPattern pattern;
	GdkPixbuf *pixbuf;
	GOPatternSelectorState *state = data;
	ArtVpath path[6];
	ArtSVP *svp;

	path[0].code = ART_MOVETO;
	path[1].code = ART_LINETO;
	path[2].code = ART_LINETO;
	path[3].code = ART_LINETO;
	path[4].code = ART_LINETO;
	path[5].code = ART_END;
	path[0].x = path[1].x = path[4].x = 0;
	path[2].x = path[3].x = W;
	path[0].y = path[3].y = path[4].y = 0;
	path[1].y = path[2].y = H;
	
	svp = art_svp_from_vpath (path);

	if (state) {
		pattern.fore = state->foreground;
		pattern.back = state->background;
	} else {
		pattern.fore = RGBA_BLACK;
		pattern.back = RGBA_WHITE;
	}
	pattern.pattern = index;

	pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, W, H);
	gdk_pixbuf_fill (pixbuf, 0xffffffff);
	go_pattern_render_svp (&pattern, svp, 0, 0, W, H,
			       gdk_pixbuf_get_pixels (pixbuf),
			       gdk_pixbuf_get_rowstride (pixbuf));
	
	art_svp_free (svp);

	go_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);
	cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_REPEAT);
	cairo_paint (cr);
	cairo_rectangle (cr, area->x + .5 , area->y + .5 , area->width - 1, area->height - 1);
	cairo_set_line_width (cr, 1);
	cairo_set_source_rgb (cr, .75, .75, .75);
	cairo_stroke (cr);

	g_object_unref (pixbuf);
}

/**
 * go_pattern_selector_new:
 * @initial_type: pattern type initially selected
 * @default_type: automatic pattern type
 *
 * Creates a new pattern selector.
 *
 * Returns: a new #GtkWidget.
 **/
GtkWidget *
go_pattern_selector_new (GOPatternType initial_type,
			 GOPatternType default_type)
{
	GtkWidget *selector;
	GtkWidget *palette;
	GOPatternSelectorState *state;

	state = g_new (GOPatternSelectorState, 1);
	state->foreground = RGBA_WHITE;
	state->background = RGBA_BLACK;

	palette = go_palette_new (GO_PATTERN_MAX, 1.0, 5,
				  go_pattern_palette_render_func, NULL,
				  state, g_free);
	go_palette_show_automatic (GO_PALETTE (palette), 
				   CLAMP (default_type, 0, GO_PATTERN_MAX - 1),
				   NULL);

	selector = go_selector_new (GO_PALETTE (palette));
	go_selector_set_active (GO_SELECTOR (selector), 
				     CLAMP (initial_type, 0, GO_PATTERN_MAX -1));

	return selector;
}

/**
 * go_pattern_selector_set_colors:
 * @selector: a pattern #GOSelector
 * @foreground: foreground color
 * @background: background color
 *
 * Updates swatch colors of @selector.
 **/
void
go_pattern_selector_set_colors (GOSelector *selector, 
				GOColor foreground, 
				GOColor background)
{
	GOPatternSelectorState *state;

	g_return_if_fail (GO_IS_SELECTOR (selector));

	state = go_selector_get_user_data (selector);
	g_return_if_fail (state != NULL);
	state->foreground = foreground;
	state->background = background;
	go_selector_update_swatch (selector);
}
