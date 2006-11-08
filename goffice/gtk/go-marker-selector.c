/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-marker-selector.c :
 *
 * Copyright (C) 2003-2006 Emmanuel Pacaud (emmanuel.pacaud@lapp.in2p3.fr)
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

#include "go-marker-selector.h"
#include "goffice-gtk.h"

#include <goffice/utils/go-color.h>

typedef struct {
	GOColor outline_color;
	GOColor fill_color;
} GOMarkerSelectorState;

static void
go_marker_palette_render_func (cairo_t *cr,
			       GdkRectangle const *area,
			       int index, 
			       gpointer data)
{
	GOMarker *marker;
	GdkPixbuf *pixbuf;
	GOMarkerSelectorState *state = data;
	int size = 2 * ((int) (MIN (area->width, area->height) * .30));

	if (size < 1)
		return;

	marker = go_marker_new ();
	go_marker_set_fill_color (marker, state->fill_color);
	go_marker_set_outline_color (marker, state->outline_color);
	go_marker_set_size (marker, size);
	go_marker_set_shape (marker, index);

	cairo_set_line_width (cr, 1);
	cairo_set_source_rgb (cr, 1., 1., 1.);
	cairo_rectangle (cr, area->x + .5 , area->y + .5 , area->width - 1, area->height - 1);
	cairo_fill_preserve (cr);
	cairo_set_source_rgb (cr, .75, .75, .75);
	cairo_stroke (cr);

	pixbuf = go_marker_get_pixbuf (marker, 1.0);
	if (pixbuf == NULL)
		return;
	
	go_cairo_set_source_pixbuf (cr, pixbuf, 
				    (int) (area->x + (area->width - gdk_pixbuf_get_width (pixbuf)) / 2.0),
				    (int) (area->y + (area->height - gdk_pixbuf_get_height (pixbuf)) / 2.0));
	cairo_paint (cr);
	g_object_unref (pixbuf);
}

/**
 * go_marker_selector_new:
 * @initial_shape: marker shape intially selected
 * @default_shape: automatic marker shape
 *
 * Creates a new marker selector.
 *
 * returns a new #GtkWidget.
 **/
GtkWidget *
go_marker_selector_new (GOMarkerShape initial_shape,
			GOMarkerShape default_shape)
{
	GtkWidget *palette;
	GtkWidget *selector;
	GOMarkerSelectorState *state;

	state = g_new (GOMarkerSelectorState, 1);
	state->outline_color = RGBA_BLACK;
	state->fill_color = RGBA_WHITE;

	palette = go_palette_new (GO_MARKER_MAX, 1.0, 4, 
				  go_marker_palette_render_func, NULL,
				  state, g_free);
	go_palette_show_automatic (GO_PALETTE (palette), 
				   CLAMP (default_shape, 0, GO_MARKER_MAX -1),
				   NULL); 	
	selector = go_selector_new (GO_PALETTE (palette));
	go_selector_set_active (GO_SELECTOR (selector), 
				CLAMP (initial_shape, 0, GO_MARKER_MAX - 1));
	return selector;
}

/**
 * go_marker_selector_set_colors:
 * @selector: a #GOSelector
 * @outline: outline color
 * @fill: fill color
 *
 * Updates swatch colors of @selector.
 **/
void
go_marker_selector_set_colors (GOSelector *selector, GOColor outline, GOColor fill)
{
	GOMarkerSelectorState *state;

	g_return_if_fail (GO_IS_SELECTOR (selector));

	state = go_selector_get_user_data (selector);
	g_return_if_fail (state != NULL);
	state->outline_color = outline;
	state->fill_color = fill;
	go_selector_update_swatch (selector);
}
