/*
 * go-pattern-selector.c :
 *
 * Copyright (C) 2006-2007 Emmanuel Pacaud <emmanuel.pacaud@lapp.in2p3.fr>
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

static const char *
go_pattern_tooltip_func (int index, gpointer data)
{
	switch ((GOPatternType)index) {
	case GO_PATTERN_SOLID: return _("Solid background");
        // xgettext:no-c-format
	case GO_PATTERN_GREY75: return _("75% background");
        // xgettext:no-c-format
	case GO_PATTERN_GREY50: return _("50% background");
        // xgettext:no-c-format
	case GO_PATTERN_GREY25: return _("25% background");
        // xgettext:no-c-format
	case GO_PATTERN_GREY125: return _("12.5% background");
        // xgettext:no-c-format
	case GO_PATTERN_GREY625: return _("6.25% background");
	case GO_PATTERN_HORIZ: return _("Horizontal stripes");
	case GO_PATTERN_VERT: return _("Vertical stripes");
	case GO_PATTERN_REV_DIAG: return _("Reverse diagonal stripes");
	case GO_PATTERN_DIAG: return _("Diagonal stripes");
	case GO_PATTERN_DIAG_CROSS: return _("Diagonal crosshatch");
	case GO_PATTERN_THICK_DIAG_CROSS: return _("Thick diagonal crosshatch");
	case GO_PATTERN_THIN_HORIZ: return _("Thin horizontal stripes");
	case GO_PATTERN_THIN_VERT: return _("Thin vertical stripes");
	case GO_PATTERN_THIN_REV_DIAG: return _("Thin reverse diagonal stripes");
	case GO_PATTERN_THIN_DIAG: return _("Thin diagonal stripes");
	case GO_PATTERN_THIN_HORIZ_CROSS: return _("Thin horizontal crosshatch");
	case GO_PATTERN_THIN_DIAG_CROSS: return _("Thin diagonal crosshatch");
	case GO_PATTERN_FOREGROUND_SOLID: return _("Solid foreground");
	case GO_PATTERN_SMALL_CIRCLES: return _("Small circles");
	case GO_PATTERN_SEMI_CIRCLES: return _("Semi-circles");
	case GO_PATTERN_THATCH: return _("Thatch");
	case GO_PATTERN_LARGE_CIRCLES: return _("Large circles");
	case GO_PATTERN_BRICKS: return _("Bricks");
	default: return NULL;
	}
}

/**
 * go_pattern_selector_new:
 * @initial_type: pattern type initially selected
 * @default_type: automatic pattern type
 *
 * Creates a new pattern selector.
 *
 * Returns: (transfer full): a new #GtkWidget.
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
				  go_pattern_palette_render_func,
				  go_pattern_tooltip_func,
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
