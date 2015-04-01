/*
 * go-marker-selector.c :
 *
 * Copyright (C) 2003-2006 Emmanuel Pacaud (emmanuel.pacaud@lapp.in2p3.fr)
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


typedef struct {
	GOColor outline_color;
	GOColor fill_color;
	gboolean auto_fill;
} GOMarkerSelectorState;

static void
go_marker_palette_render_func (cairo_t *cr,
			       GdkRectangle const *area,
			       int index,
			       gpointer data)
{
	GOMarker *marker;
	GOMarkerSelectorState *state = data;
	int size = 2 * ((int) (MIN (area->width, area->height) * .30));

	if (size < 1)
		return;

	marker = go_marker_new ();
	go_marker_set_outline_color (marker, state->outline_color);
	go_marker_set_size (marker, size);
	go_marker_set_shape (marker, index);
	go_marker_set_fill_color (marker, (state->auto_fill && !go_marker_is_closed_shape (marker))? 0: state->fill_color);

	cairo_set_line_width (cr, 1);
	cairo_set_source_rgb (cr, 1., 1., 1.);
	cairo_rectangle (cr, area->x + .5 , area->y + .5 , area->width - 1, area->height - 1);
	cairo_fill_preserve (cr);
	cairo_set_source_rgb (cr, .75, .75, .75);
	cairo_stroke (cr);

	go_marker_render (marker, cr, area->x + area->width / 2.0, area->y + area->height / 2.0, 1.0);
	g_object_unref (marker);
}

/**
 * go_marker_selector_new:
 * @initial_shape: marker shape intially selected
 * @default_shape: automatic marker shape
 *
 * Creates a new marker selector.
 *
 * Returns: a new #GtkWidget.
 **/
GtkWidget *
go_marker_selector_new (GOMarkerShape initial_shape,
			GOMarkerShape default_shape)
{
	GtkWidget *palette;
	GtkWidget *selector;
	GOMarkerSelectorState *state;

	state = g_new (GOMarkerSelectorState, 1);
	state->outline_color = GO_COLOR_BLACK;
	state->fill_color = GO_COLOR_WHITE;

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

/**
 * go_marker_selector_set_shape:
 * @selector: a #GOSelector
 * @shape: new marker shape
 *
 * Updates marker shape of @selector.
 **/
void
go_marker_selector_set_shape (GOSelector *selector, GOMarkerShape shape)
{
	g_return_if_fail (GO_IS_SELECTOR (selector));

	go_selector_set_active (GO_SELECTOR (selector),
				CLAMP (shape, 0, GO_MARKER_MAX - 1));
	go_selector_update_swatch (selector);
}


/**
 * go_marker_selector_set_auto_fill:
 * @selector: a #GOSelector
 * @auto_fill: whether to use a transparent color for opened markers such as
 * cross, x, or asterisk.
 *
 **/
void
go_marker_selector_set_auto_fill (GOSelector *selector, gboolean auto_fill)
{
	GOMarkerSelectorState *state;
	g_return_if_fail (GO_IS_SELECTOR (selector));
	state = go_selector_get_user_data (selector);
	g_return_if_fail (state != NULL);
	state->auto_fill = auto_fill;
}
