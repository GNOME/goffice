/*
 * go-style.h :
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
#ifndef GO_UTILS_STYLE_H
#define GO_UTILS_STYLE_H

#include <goffice/goffice.h>
#include <goffice/utils/go-editor.h>

G_BEGIN_DECLS

#define GO_TYPE_STYLE	(go_style_get_type ())
#define GO_STYLE(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_STYLE, GOStyle))
#define GO_IS_STYLE(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_STYLE))

GType go_style_get_type (void);

typedef enum {
	GO_STYLE_OUTLINE	= 1 << 0,
	GO_STYLE_FILL		= 1 << 1,
	GO_STYLE_LINE		= 1 << 2,
	GO_STYLE_MARKER	= 1 << 3,
	GO_STYLE_FONT		= 1 << 4,
	GO_STYLE_TEXT_LAYOUT	= 1 << 5,
	GO_STYLE_INTERPOLATION	= 1 << 6,
	GO_STYLE_MARKER_NO_COLOR	= 1 << 7,
	GO_STYLE_ALL		= 0x1F
} GOStyleFlag;

typedef enum {
	GO_STYLE_FILL_NONE	= 0,
	GO_STYLE_FILL_PATTERN	= 1,
	GO_STYLE_FILL_GRADIENT	= 2,
	GO_STYLE_FILL_IMAGE	= 3
} GOStyleFill;

typedef enum {
	GO_IMAGE_STRETCHED,
	GO_IMAGE_WALLPAPER,
	GO_IMAGE_CENTERED,
	GO_IMAGE_CENTERED_WALLPAPER
} GOImageType;

typedef struct {
	/* <0 == no outline,
	 * =0 == hairline : unscaled, minimum useful (can be bigger than visible) size.
	 * >0 in pts */
	double	 	 width;
	GOLineDashType 	 dash_type;
	gboolean	 auto_dash;
	GOColor	 	 color; /* color is used as background for compatibility
						(pattern == 0 means filled with background color) */
	GOColor	 	 fore;
	gboolean 	 auto_color;
	gboolean 	 auto_fore;
	gboolean 	 auto_width;
	GOPatternType	 pattern;
	cairo_line_cap_t cap;
	cairo_line_join_t join;
	double		 miter_limit;
} GOStyleLine;

typedef struct {
	GOMarker *mark;
	gboolean auto_shape;
	gboolean auto_outline_color;
	gboolean auto_fill_color;
} GOStyleMark;

struct _GOStyle {
	GObject	base;

	GOStyleFlag	interesting_fields;
	GOStyleFlag	disable_theming;

	GOStyleLine	line;
	struct {
		GOStyleFill	type;
		gboolean	auto_type;
		gboolean	auto_fore, auto_back;	/* share between pattern and gradient */
		gboolean		auto_pattern;
		gboolean	invert_if_negative;	/* placeholder for XL */

		/* This could all be a union but why bother ? */
		GOPattern pattern;
		struct _GOStyleGradient {
			GOGradientDirection dir;
			double brightness; /* < 0 => 2 color */
			gboolean auto_dir;
			gboolean auto_brightness;
		} gradient;
		struct _GOStyleImage {
			GOImageType	 type;
			GOImage	 *image;
		} image;
	} fill;
	GOStyleMark marker;
	struct {
		GOColor		 color;
		GOFont const 	*font;
		gboolean 	 auto_scale;
		gboolean	 auto_color;
		gboolean	 auto_font;
	} font;
	struct {
		double		 angle;
		gboolean	 auto_angle;
	} text_layout;
};

GOStyle  *go_style_new			(void);
GOStyle  *go_style_dup			(GOStyle const *style);
void	   go_style_assign		(GOStyle *dst, GOStyle const *src);
void	   go_style_apply_theme		(GOStyle *dst, GOStyle const *src,
					 GOStyleFlag fields);

GOMarker const *go_style_get_marker 	(GOStyle const *style);
void            go_style_set_marker 	(GOStyle *style, GOMarker *marker);

void	   go_style_set_font_desc	(GOStyle *style,
					 PangoFontDescription *desc);
void	   go_style_set_font		(GOStyle *style, GOFont const *font);
void	   go_style_set_fill_brightness	(GOStyle *style, double brightness);
void	   go_style_set_text_angle     	(GOStyle *style, double angle);

gboolean   go_style_is_different_size	(GOStyle const *a, GOStyle const *b);
gboolean   go_style_is_marker_visible	(GOStyle const *style);
gboolean   go_style_is_line_visible	(GOStyle const *style);
gboolean   go_style_is_outline_visible	(GOStyle const *style);
gboolean   go_style_is_fill_visible	(GOStyle const *style);
void	   go_style_force_auto		(GOStyle *style);
void	   go_style_clear_auto		(GOStyle *style);
gboolean   go_style_is_auto		(GOStyle *style);

#ifdef GOFFICE_WITH_GTK
void 	   go_style_populate_editor 	(GOStyle *style,
					 GOEditor *editor,
					 GOStyle *default_style,
					 GOCmdContext *cc,
					 GObject *object_with_style,
					 gboolean watch_for_external_change);
gpointer   go_style_get_editor	     	(GOStyle *style,
					 GOStyle *default_style,
					 GOCmdContext *cc,
					 GObject *object_with_style);
#endif

void     go_style_fill			(GOStyle const *style, cairo_t *cr,
			                 gboolean preserve);
gboolean go_style_set_cairo_line (GOStyle const *style, cairo_t *cr);

G_END_DECLS

#endif /* GO_UTILS_STYLE_H */
