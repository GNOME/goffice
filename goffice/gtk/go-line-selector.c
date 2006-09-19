/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-line-selector.c :
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

#include "go-line-selector.h"

#include <libart_lgpl/libart.h>

static void
go_line_dash_palette_render_func (cairo_t *cr,
				  GdkRectangle const *area,
				  int index, 
				  gpointer data)
{
	ArtVpathDash *dash;
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
	dash = go_line_get_vpath_dash (index, 1);
	if (dash != NULL) {
		cairo_set_dash (cr, dash->dash, dash->n_dash, 0);
		go_line_vpath_dash_free (dash);
	}
	cairo_move_to (cr, area->x + 3, y);
	cairo_line_to (cr, area->x + area->width - 3, y);
	cairo_stroke (cr);
}

/**
 * go_line_dash_selector_new:
 * @initial_type: line type initially selected
 * @default_type: automatic line type
 *
 * Creates a new line type selector.
 *
 * Returns a new #GtkWidget.
 **/
GtkWidget *
go_line_dash_selector_new (GOLineDashType initial_type,
			   GOLineDashType default_type)
{
	GtkWidget *palette;
	GtkWidget *selector;

	palette = go_palette_new (GO_LINE_MAX, 3.0, 2, 
				  go_line_dash_palette_render_func,
				  NULL, NULL);
	go_palette_show_automatic (GO_PALETTE (palette), 
				   CLAMP (default_type, 0, GO_LINE_MAX -1),
				   NULL); 	
	selector = go_selector_new (GO_PALETTE (palette));
	go_selector_set_active (GO_SELECTOR (selector),
				CLAMP (initial_type, 0, GO_LINE_MAX - 1));
	return selector;
}
