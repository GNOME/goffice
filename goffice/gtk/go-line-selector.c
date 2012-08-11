/*
 * go-line-selector.c :
 *
 * Copyright (C) 2006 Emmanuel Pacaud (emmanuel.pacaud@lapp.in2p3.fr)
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

#include "go-line-selector.h"

#include <glib/gi18n-lib.h>

static void
go_line_dash_palette_render_func (cairo_t *cr,
				  GdkRectangle const *area,
				  int index,
				  gpointer data)
{
	GOLineDashSequence *sequence;
	double y;

	if (index < 0 || index >= GO_LINE_MAX)
		return;

	y = .5 + (int) (area->y + area->height / 2);
	cairo_set_line_width (cr, 1);
	cairo_set_source_rgb (cr, 1, 1, 1);
	cairo_rectangle (cr, area->x + .5 , area->y + .5 , area->width - 1, area->height - 1);
	cairo_fill_preserve (cr);
	cairo_set_source_rgb (cr, .75, .75, .75);
	cairo_stroke (cr);

	if (index == GO_LINE_NONE)
		return;

	cairo_set_source_rgb (cr, 0, 0, 0);
	cairo_set_line_width (cr, 1);
	cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);
	sequence = go_line_dash_get_sequence (index, 1);
	if (sequence != NULL) {
		cairo_set_dash (cr, sequence->dash, sequence->n_dash, 0.);
		go_line_dash_sequence_free (sequence);
	}
	cairo_move_to (cr, area->x + 3.5, y);
	cairo_line_to (cr, area->x + area->width - 3.5, y);
	cairo_stroke (cr);
}

static char const *
go_line_dash_palette_get_tooltip_func (int index, void *data)
{
	return go_line_dash_as_label (index);
}

/**
 * go_selector_new_line_dash:
 * @initial_type: line type initially selected
 * @default_type: automatic line type
 *
 * Creates a new line type selector.
 *
 * Since: 0.9.6
 * Returns: (transfer full): a new #GtkWidget.
 **/
GtkWidget *
go_selector_new_line_dash (GOLineDashType initial_type,
			   GOLineDashType default_type)
{
	GtkWidget *palette;
	GtkWidget *selector;

	palette = go_palette_new (GO_LINE_MAX, 3.0, 3,
				  go_line_dash_palette_render_func,
				  go_line_dash_palette_get_tooltip_func,
				  NULL, NULL);
	go_palette_show_automatic (GO_PALETTE (palette),
				   CLAMP (default_type, 0, GO_LINE_MAX -1),
				   NULL);
	selector = go_selector_new (GO_PALETTE (palette));
	go_selector_set_active (GO_SELECTOR (selector),
				CLAMP (initial_type, 0, GO_LINE_MAX - 1));
	return selector;
}

/**
 * go_line_dash_selector_new:
 * @initial_type: line type initially selected
 * @default_type: automatic line type
 *
 * Creates a new line type selector.
 *
 * Deprecated: 0.9.6, use go_selector_new_line_dash().
 * Returns: (transfer full): a new #GtkWidget.
 **/
GtkWidget *
go_line_dash_selector_new (GOLineDashType initial_type,
			   GOLineDashType default_type)
{
	return go_selector_new_line_dash (initial_type, default_type);
}
