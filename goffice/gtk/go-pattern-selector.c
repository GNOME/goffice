/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-pattern-selector.c :
 *
 * Copyright (C) 2006-2007 Emmanuel Pacaud <emmanuel.pacaud@lapp.in2p3.fr>
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
	GOPattern pattern;
	GOPatternSelectorState *state = data;
	cairo_pattern_t *cr_pattern;

	if (state) {
		pattern.fore = state->foreground;
		pattern.back = state->background;
	} else {
		pattern.fore = GO_COLOR_BLACK;
		pattern.back = GO_COLOR_WHITE;
	}
	pattern.pattern = index;

	cr_pattern = go_pattern_create_cairo_pattern (&pattern, cr);

	cairo_set_source (cr, cr_pattern);
	cairo_paint (cr);
	cairo_rectangle (cr, area->x + .5 , area->y + .5 , area->width - 1, area->height - 1);
	cairo_set_line_width (cr, 1);
	cairo_set_source_rgb (cr, .75, .75, .75);
	cairo_stroke (cr);

	cairo_pattern_destroy (cr_pattern);
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
	state->foreground = GO_COLOR_WHITE;
	state->background = GO_COLOR_BLACK;

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
