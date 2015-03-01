/*
 * go-gradient-selector.c :
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
#include <goffice/goffice.h>
#include <glib/gi18n-lib.h>

typedef struct {
	GOColor start_color;
	GOColor stop_color;
} GOGradientSelectorState;

static void
go_gradient_swatch_render_func (cairo_t *cr,
				GdkRectangle const *area,
				int index,
				gpointer data)
{
	struct { unsigned x0i, y0i, x1i, y1i; } const grad_i[GO_GRADIENT_MAX] = {
		{0, 0, 0, 1},
		{0, 1, 0, 0},
		{0, 0, 0, 2},
		{0, 2, 0, 1},
		{0, 0, 1, 0},
		{1, 0, 0, 0},
		{0, 0, 2, 0},
		{2, 0, 1, 0},
		{0, 0, 1, 1},
		{1, 1, 0, 0},
		{0, 0, 2, 2},
		{2, 2, 1, 1},
		{1, 0, 0, 1},
		{0, 1, 1, 0},
		{1, 0, 2, 2},
		{2, 2, 0, 1}
	};
	GOGradientSelectorState *state = data;
	cairo_pattern_t *cr_pattern;
	double x[3], y[3];

	cairo_rectangle (cr, area->x + .5 , area->y + .5 , area->width - 1, area->height - 1);
	cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
	cairo_fill_preserve (cr);

	x[0] = area->x;
	y[0] = area->y;
	x[1] = area->x + area->width;
	y[1] = area->y + area->height;
	x[2] = (x[1] - x[0]) / 2.0 + x[0];
	y[2] = (y[1] - y[0]) / 2.0 + y[0];
	cr_pattern = cairo_pattern_create_linear (x[grad_i[index].x0i],
						  y[grad_i[index].y0i],
						  x[grad_i[index].x1i],
						  y[grad_i[index].y1i]);
	cairo_pattern_set_extend (cr_pattern, CAIRO_EXTEND_REFLECT);
	cairo_pattern_add_color_stop_rgba (cr_pattern, 0,
					   GO_COLOR_TO_CAIRO (state->start_color));
	cairo_pattern_add_color_stop_rgba (cr_pattern, 1,
					   GO_COLOR_TO_CAIRO (state->stop_color));
	cairo_set_source (cr, cr_pattern);

	cairo_fill_preserve (cr);
	cairo_set_line_width (cr, 1);
	cairo_set_source_rgb (cr, .75, .75, .75);
	cairo_stroke (cr);

	cairo_pattern_destroy (cr_pattern);
}

static const char *
go_gradient_tooltip_func (int index, gpointer data)
{
	switch ((GOGradientDirection)index) {
	case GO_GRADIENT_N_TO_S:
		return _("Up");
	case GO_GRADIENT_S_TO_N:
		return _("Down");
	case GO_GRADIENT_N_TO_S_MIRRORED:
		return _("Vertical from middle");
	case GO_GRADIENT_S_TO_N_MIRRORED:
		return _("Vertical to middle");
	case GO_GRADIENT_W_TO_E:
		return _("Left");
	case GO_GRADIENT_E_TO_W:
		return _("Right");
	case GO_GRADIENT_W_TO_E_MIRRORED:
		return _("Horizontal from middle");
	case GO_GRADIENT_E_TO_W_MIRRORED:
		return _("Horizontal to middle");
	case GO_GRADIENT_NW_TO_SE:
		return _("Up and left");
	case GO_GRADIENT_SE_TO_NW:
		return _("Down and right");
	case GO_GRADIENT_NW_TO_SE_MIRRORED:
		return _("Diagonal from middle");
	case GO_GRADIENT_SE_TO_NW_MIRRORED:
		return _("Diagonal to middle");
	case GO_GRADIENT_NE_TO_SW:
		return _("Up and right");
	case GO_GRADIENT_SW_TO_NE:
		return _("Down and left");
	case GO_GRADIENT_SW_TO_NE_MIRRORED:
		return _("Reverse diagonal from middle");
	case GO_GRADIENT_NE_TO_SW_MIRRORED:
		return _("Reverse diagonal to middle");
	default:
		return NULL;
	}
}

/**
 * go_selector_new_gradient:
 * @initial_direction: initially selected direction
 * @default_direction: automatic gradient direction
 *
 * Creates a new gradient selector.
 *
 * Since: 0.9.6
 * Returns: (transfer full): a new #GtkWidget.
 **/
GtkWidget *
go_selector_new_gradient (GOGradientDirection initial_direction,
			  GOGradientDirection default_direction)
{
	GtkWidget *palette;
	GtkWidget *selector;
	GOGradientSelectorState *state;

	state = g_new (GOGradientSelectorState, 1);
	state->start_color = GO_COLOR_BLACK;
	state->stop_color = GO_COLOR_WHITE;

	palette = go_palette_new (GO_GRADIENT_MAX, 1.0, 4,
				  go_gradient_swatch_render_func,
				  go_gradient_tooltip_func,
				  state, g_free);
	go_palette_show_automatic (GO_PALETTE (palette),
				   CLAMP (default_direction, 0, GO_GRADIENT_MAX -1),
				   NULL);
	selector = go_selector_new (GO_PALETTE (palette));
	go_selector_set_active (GO_SELECTOR (selector),
				CLAMP (initial_direction, 0, GO_GRADIENT_MAX - 1));
	return selector;
}

/**
 * go_gradient_selector_new:
 * @initial_direction: initially selected direction
 * @default_direction: automatic gradient direction
 *
 * Creates a new gradient selector.
 *
 * Deprecated: 0.9.6, use go_selector_new_gradient().
 * Returns: (transfer full): a new #GtkWidget.
 **/
GtkWidget *
go_gradient_selector_new (GOGradientDirection initial_direction,
			  GOGradientDirection default_direction)
{
	return go_selector_new_gradient (initial_direction, default_direction);
}

void
go_gradient_selector_set_colors (GOSelector *selector,
				 GOColor start_color,
				 GOColor stop_color)
{
	GOGradientSelectorState *state;

	g_return_if_fail (GO_IS_SELECTOR (selector));

	state = go_selector_get_user_data (selector);
	g_return_if_fail (state != NULL);
	state->start_color = start_color;
	state->stop_color = stop_color;
	go_selector_update_swatch (selector);
}
