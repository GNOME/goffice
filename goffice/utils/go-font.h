/*
 * go-font.h :
 *
 * Copyright (C) 2003-2004 Jody Goldberg (jody@gnome.org)
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
#ifndef GO_FONT_H
#define GO_FONT_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

struct _GOFontMetrics {
	int digit_widths[10];
	int min_digit_width;
	int max_digit_width;
	int avg_digit_width;
	int hyphen_width, minus_width, plus_width;
	int E_width;
	int hash_width;
	int space_width;

	/*
	 * A space that is narrower than a regular space, or 0 if no such
	 * character was found.
	 */
	gunichar thin_space;
	int thin_space_width;
};

struct _GOFont {
	int ref_count;
	int font_index; /* each renderer keeps an array for lookup */

	PangoFontDescription *desc;

	/* Attributes.  */
	int underline;
	gboolean strikethrough;
	GOColor color;
};

GType         go_font_get_type (void);
GOFont const *go_font_new_by_desc  (PangoFontDescription *desc);
GOFont const *go_font_new_by_name  (char const *str);
GOFont const *go_font_new_by_index (unsigned i);
char   	     *go_font_as_str       (GOFont const *font);
GOFont const *go_font_ref	   (GOFont const *font);
void	      go_font_unref	   (GOFont const *font);
gboolean      go_font_eq	   (GOFont const *a, GOFont const *b);

GSList       *go_fonts_list_families (PangoContext *context);
GSList       *go_fonts_list_sizes    (void);

GType         go_font_metrics_get_type (void);
GOFontMetrics *go_font_metrics_new (PangoContext *context, GOFont const *font);
GO_VAR_DECL const GOFontMetrics *go_font_metrics_unit;
void go_font_metrics_free (GOFontMetrics *metrics);

/* cache notification */
void go_font_cache_register   (GClosure *callback);
void go_font_cache_unregister (GClosure *callback);

/* private */
void _go_fonts_init     (void);
void _go_fonts_shutdown (void);

G_END_DECLS

#endif /* GO_FONT_H */
