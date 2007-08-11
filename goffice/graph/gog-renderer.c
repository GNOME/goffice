/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-renderer.c :
 *
 * Copyright (C) 2003-2004 Jody Goldberg (jody@gnome.org)
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
#include <goffice/graph/gog-style.h>
#include <goffice/graph/gog-graph.h>
#include <goffice/graph/gog-graph-impl.h>
#include <goffice/graph/gog-view.h>
#include <goffice/utils/go-units.h>
#include <goffice/utils/go-color.h>
#include <goffice/utils/go-font.h>
#include <goffice/utils/go-math.h>
#include <goffice/utils/go-marker.h>

#include <gsf/gsf.h>
#include <gsf/gsf-impl-utils.h>
#include <gsf/gsf-output-stdio.h>

#include <glib/gi18n-lib.h>

#include <libart_lgpl/art_render_gradient.h>
#include <libart_lgpl/art_render_svp.h>
#include <libart_lgpl/art_render_mask.h>
#include <pango/pangocairo.h>

#include <cairo.h>
#ifdef CAIRO_HAS_PDF_SURFACE
#include <cairo-pdf.h>
#endif
#ifdef CAIRO_HAS_PS_SURFACE
#include <cairo-ps.h>
#endif
#ifdef CAIRO_HAS_SVG_SURFACE
#include <cairo-svg.h>
#endif

#include <math.h>

enum {
	RENDERER_PROP_0,
	RENDERER_PROP_MODEL,
	RENDERER_PROP_VIEW,
	RENDERER_PROP_ZOOM,
	RENDERER_PROP_CAIRO,
	RENDERER_PROP_IS_VECTOR
};
enum {
	RENDERER_SIGNAL_REQUEST_UPDATE,
	RENDERER_SIGNAL_LAST
};
static gulong renderer_signals [RENDERER_SIGNAL_LAST] = { 0, };

static GObjectClass *parent_klass;

typedef struct {
	ArtVpath *path;
	gpointer data;
} GogRendererClip;

struct _GogRenderer {
	GObject	 base;

	GogGraph *model;
	GogView	 *view;
	float	  scale, scale_x, scale_y;
	float	  zoom;

	GogRendererClip const *cur_clip;
	GSList	  *clip_stack;

	GClosure *font_watcher;
	gboolean  needs_update;

	GogStyle const *cur_style;
	GSList   *style_stack;

	ArtVpathDash *line_dash;
	ArtVpathDash *outline_dash;

	GogStyle *grip_style;

	int 		 w, h;

	gboolean	 is_vector;

	cairo_t		*cairo;
	GdkPixbuf 	*pixbuf;

	gboolean	 marker_as_surface;
	cairo_surface_t *marker_surface;
	double		 marker_x_offset;
	double		 marker_y_offset;
};

typedef struct {
	GObjectClass base;

	/* Signals */
	void (*request_update) (GogRenderer *renderer);
} GogRendererClass;

#define GOG_RENDERER_CLASS(k)	 (G_TYPE_CHECK_CLASS_CAST ((k), GOG_RENDERER_TYPE, GogRendererClass))
#define IS_GOG_RENDERER_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), GOG_RENDERER_TYPE))

#define GRC_LINEAR_SCALE(w,scale) (go_sub_epsilon (w) <= 0.0 ? GOG_RENDERER_HAIRLINE_WIDTH_PTS : w) * scale

/* This is a workaround for a cairo bug, due to its internal
 * handling of coordinates (16.16 fixed point). */
#define GO_CAIRO_CLAMP(x) CLAMP((x),-15000,15000)
#define GO_CAIRO_CLAMP_SNAP(x,even) GO_CAIRO_CLAMP(even ? floor (x + .5):floor (x) + .5)

static void
grc_path_raw (cairo_t *cr, ArtVpath *vpath, ArtBpath *bpath)
{
	if (vpath) {
		while (vpath->code != ART_END) {
			switch (vpath->code) {
				case ART_MOVETO_OPEN:
				case ART_MOVETO:
					cairo_move_to (cr, GO_CAIRO_CLAMP (vpath->x),
						       GO_CAIRO_CLAMP (vpath->y));
					break;
				case ART_LINETO:
					cairo_line_to (cr, GO_CAIRO_CLAMP (vpath->x),
						       GO_CAIRO_CLAMP (vpath->y));
					break;
				default:
					break;
			}
			vpath++;
		}
	} else
		while (bpath->code != ART_END) {
			switch (bpath->code) {
				case ART_MOVETO_OPEN:
				case ART_MOVETO:
					cairo_move_to (cr, GO_CAIRO_CLAMP (bpath->x3),
						       GO_CAIRO_CLAMP (bpath->y3));
					break;
				case ART_LINETO:
					cairo_line_to (cr, GO_CAIRO_CLAMP (bpath->x3),
						       GO_CAIRO_CLAMP (bpath->y3));
					break;
				case ART_CURVETO:
					cairo_curve_to (cr, GO_CAIRO_CLAMP (bpath->x1),
							GO_CAIRO_CLAMP (bpath->y1),
							GO_CAIRO_CLAMP (bpath->x2),
							GO_CAIRO_CLAMP (bpath->y2),
							GO_CAIRO_CLAMP (bpath->x3),
							GO_CAIRO_CLAMP (bpath->y3));
					break;
				default:
					break;
			}
			bpath++;
		}
}

static void
grc_path_sharp (cairo_t *cr, ArtVpath *vpath, ArtBpath *bpath, double line_width)
{
	gboolean even = ((int) (go_fake_ceil (line_width)) % 2 == 0) && line_width > 1.0;

	if (vpath)
		while (vpath->code != ART_END) {
			switch (vpath->code) {
				case ART_MOVETO_OPEN:
				case ART_MOVETO:
					cairo_move_to (cr, GO_CAIRO_CLAMP_SNAP (vpath->x, even),
						           GO_CAIRO_CLAMP_SNAP (vpath->y, even));
					break;
				case ART_LINETO:
					cairo_line_to (cr, GO_CAIRO_CLAMP_SNAP (vpath->x, even),
						           GO_CAIRO_CLAMP_SNAP (vpath->y, even));
					break;
				default:
					break;
			}
			vpath++;
		}
}

static void
grc_path (cairo_t *cr, ArtVpath *vpath, ArtBpath *bpath, double line_width, gboolean sharp)
{
	if (sharp)
		grc_path_sharp (cr, vpath, bpath, line_width);
	else
		grc_path_raw (cr, vpath, bpath);
}

static double
_grc_line_size (GogRenderer const *rend, double width, gboolean sharp)
{
	if (rend->is_vector)
		return GRC_LINEAR_SCALE (width, rend->scale);

	if (go_sub_epsilon (width) <= 0.) /* cheesy version of hairline */
		return 1.;

	width *= rend->scale;
	if (!sharp || width <= 1.)
		return width;

	return go_fake_round (width);
}

static double
_line_size (GogRenderer const *rend, double width, gboolean sharp)
{
	double size = _grc_line_size (rend, width, sharp);

	if (!rend->is_vector && size < 1.0)
		return ceil (size);

	return size;
}

static void
_update_dash (GogRenderer *rend)
{
	double size;

	go_line_vpath_dash_free (rend->line_dash);
	rend->line_dash = NULL;
	go_line_vpath_dash_free (rend->outline_dash);
	rend->outline_dash = NULL;

	if (rend->cur_style == NULL)
		return;

	size = _line_size (rend, rend->cur_style->line.width, FALSE);
	rend->line_dash = go_line_get_vpath_dash (rend->cur_style->line.dash_type, size);
	size = _line_size (rend, rend->cur_style->outline.width, FALSE);
	rend->outline_dash = go_line_get_vpath_dash (rend->cur_style->outline.dash_type, size);
}

double
gog_renderer_line_size (GogRenderer const *rend, double width)
{
	return _line_size (rend, width, TRUE);
}

double
gog_renderer_pt2r_x (GogRenderer const *rend, double d)
{
	return d * rend->scale_x;
}

double
gog_renderer_pt2r_y (GogRenderer const *rend, double d)
{
	return d * rend->scale_y;
}

double
gog_renderer_pt2r (GogRenderer const *rend, double d)
{
	return d * rend->scale;
}

/*****************************************************************************/

static void
emit_stroke (GogRenderer *rend, gboolean preserve, GOPathOptions options)
{
	GogStyle const *style = rend->cur_style;
	cairo_t *cr = rend->cairo;
	double width;

	if (!gog_style_is_line_visible (style))
		return;

	width = _grc_line_size (rend, style->line.width, options & GO_PATH_OPTIONS_SNAP_WIDTH);
	cairo_set_line_width (cr, width);
	if (rend->line_dash != NULL)
		cairo_set_dash (cr,
				rend->line_dash->dash,
				rend->line_dash->n_dash,
				rend->line_dash->offset);
	else
		cairo_set_dash (cr, NULL, 0, 0);

	cairo_set_source_rgba (cr, GO_COLOR_TO_CAIRO (style->line.color));
	cairo_set_line_cap (cr, (width <= 2.0 && !rend->is_vector) ?
			    CAIRO_LINE_CAP_SQUARE : CAIRO_LINE_CAP_ROUND);

	if (preserve)
		cairo_stroke_preserve (cr);
	else
		cairo_stroke (cr);
}

static void
emit_outline (GogRenderer *rend, gboolean preserve, GOPathOptions options)
{
	GogStyle const *style = rend->cur_style;
	cairo_t *cr = rend->cairo;
	double width;

	if (!gog_style_is_outline_visible (style))
		return;

	width = _grc_line_size (rend, style->outline.width, options & GO_PATH_OPTIONS_SNAP_WIDTH);
	cairo_set_line_width (cr, width);
	if (rend->outline_dash != NULL)
		cairo_set_dash (cr,
				rend->outline_dash->dash,
				rend->outline_dash->n_dash,
				rend->outline_dash->offset);
	else
		cairo_set_dash (cr, NULL, 0, 0);

	cairo_set_source_rgba (cr, GO_COLOR_TO_CAIRO (style->outline.color));
	cairo_set_line_cap (cr, (width <= 2.0 && !rend->is_vector) ?
			    CAIRO_LINE_CAP_SQUARE : CAIRO_LINE_CAP_ROUND);

	if (preserve)
		cairo_stroke_preserve (cr);
	else
		cairo_stroke (cr);
}

static void
emit_fill (GogRenderer *rend, gboolean preserve)
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

	GogStyle const *style = rend->cur_style;
	cairo_t *cr = rend->cairo;
	cairo_pattern_t *cr_pattern = NULL;
	cairo_surface_t *cr_surface = NULL;
	cairo_matrix_t cr_matrix;
	GdkPixbuf *pixbuf = NULL;
	GOImage *image = NULL;
	GOColor color;
	double x[3], y[3];
	int i, j, w, h;
	guint8 const *pattern;
	unsigned char *iter;

	switch (style->fill.type) {
		case GOG_FILL_STYLE_NONE:
			return;
			break;
		case GOG_FILL_STYLE_PATTERN:
			if (go_pattern_is_solid (&style->fill.pattern, &color))
				cairo_set_source_rgba (cr, GO_COLOR_TO_CAIRO (color));
			else {
				GOColor fore = style->fill.pattern.fore;
				GOColor back = style->fill.pattern.back;
				int rowstride;

				pattern = go_pattern_get_pattern (&style->fill.pattern);
				pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 8, 8);
				iter = gdk_pixbuf_get_pixels (pixbuf);
				rowstride = gdk_pixbuf_get_rowstride (pixbuf);
				cr_surface = cairo_image_surface_create_for_data ( iter,
					CAIRO_FORMAT_ARGB32, 8, 8, rowstride);
				for (i = 0; i < 8; i++) {
					for (j = 0; j < 8; j++) {
						color = pattern[i] & (1 << j) ? fore : back;
						iter[0] = UINT_RGBA_B (color);
						iter[1] = UINT_RGBA_G (color);
						iter[2] = UINT_RGBA_R (color);
						iter[3] = UINT_RGBA_A (color);
						iter += 4;
					}
					iter += rowstride - 32;
				}
				cr_pattern = cairo_pattern_create_for_surface (cr_surface);
				cairo_pattern_set_extend (cr_pattern, CAIRO_EXTEND_REPEAT);
				cairo_set_source (cr, cr_pattern);
			}
			break;

		case GOG_FILL_STYLE_GRADIENT:
			cairo_fill_extents (cr, &x[0], &y[0], &x[1], &y[1]);
			x[2] = (x[1] - x[0]) / 2.0 + x[1];
			y[2] = (y[1] - y[0]) / 2.0 + y[1];
			cr_pattern = cairo_pattern_create_linear (
				x[grad_i[style->fill.gradient.dir].x0i],
				y[grad_i[style->fill.gradient.dir].y0i],
				x[grad_i[style->fill.gradient.dir].x1i],
				y[grad_i[style->fill.gradient.dir].y1i]);
			cairo_pattern_set_extend (cr_pattern, CAIRO_EXTEND_REFLECT);
			cairo_pattern_add_color_stop_rgba (cr_pattern, 0,
				GO_COLOR_TO_CAIRO (style->fill.pattern.back));
			cairo_pattern_add_color_stop_rgba (cr_pattern, 1,
				GO_COLOR_TO_CAIRO (style->fill.pattern.fore));
			cairo_set_source (cr, cr_pattern);
			break;

		case GOG_FILL_STYLE_IMAGE:
			if (style->fill.image.image == NULL) {
				cairo_set_source_rgba (cr, 1, 1, 1, 1);
				break;
			}
			cairo_fill_extents (cr, &x[0], &y[0], &x[1], &y[1]);
			cr_pattern = go_image_create_cairo_pattern (style->fill.image.image);
			g_object_get (style->fill.image.image, "width", &w, "height", &h, NULL);
			cairo_pattern_set_extend (cr_pattern, CAIRO_EXTEND_REPEAT);
			switch (style->fill.image.type) {
				case GOG_IMAGE_CENTERED:
					cairo_matrix_init_translate (&cr_matrix,
								     -(x[1] - x[0] - w) / 2 - x[0],
								     -(y[1] - y[0] - h) / 2 - y[0]);
					cairo_pattern_set_matrix (cr_pattern, &cr_matrix);
					break;
				case GOG_IMAGE_STRETCHED:
					cairo_matrix_init_scale (&cr_matrix,
								 w / (x[1] - x[0]),
								 h / (y[1] - y[0]));
					cairo_matrix_translate (&cr_matrix, -x[0], -y[0]);
					cairo_pattern_set_matrix (cr_pattern, &cr_matrix);
					break;
				case GOG_IMAGE_WALLPAPER:
					break;
			}
			cairo_set_source (cr, cr_pattern);
			break;
	}

	if (preserve)
		cairo_fill_preserve (cr);
	else
		cairo_fill (cr);

	if (cr_pattern != NULL)
		cairo_pattern_destroy (cr_pattern);
	if (cr_surface != NULL)
		cairo_surface_destroy (cr_surface);
	if (pixbuf)
		g_object_unref (pixbuf);
	if (image)
		g_object_unref (image);
}

static void
path_raw_move_to (void *closure, GOPathPoint const *point)
{
	cairo_move_to ((cairo_t *) closure, point->x, point->y);
}

static void
path_raw_line_to (void *closure, GOPathPoint const *point)
{
	cairo_line_to ((cairo_t *) closure, point->x, point->y);
}

static void
path_snap_even_move_to (void *closure, GOPathPoint const *point)
{
	cairo_move_to ((cairo_t *) closure,
		       floor (point->x + .5),
		       floor (point->y + .5));
}

static void
path_snap_even_line_to (void *closure, GOPathPoint const *point)
{
	cairo_line_to ((cairo_t *) closure,
		       floor (point->x + .5),
		       floor (point->y + .5));
}

static void
path_snap_odd_move_to (void *closure, GOPathPoint const *point)
{
	cairo_move_to ((cairo_t *) closure,
		       floor (point->x) + .5,
		       floor (point->y) + .5);
}

static void
path_snap_odd_line_to (void *closure, GOPathPoint const *point)
{
	cairo_line_to ((cairo_t *) closure,
		       floor (point->x) + .5,
		       floor (point->y) + .5);
}

static void
path_curve_to (void *closure,
	       GOPathPoint const *point0,
	       GOPathPoint const *point1,
	       GOPathPoint const *point2)
{
	cairo_curve_to ((cairo_t *) closure,
			point0->x, point0->y,
			point1->x, point1->y,
			point2->x, point2->y);
}

static void
path_close_path (void *closure)
{
	cairo_close_path ((cairo_t *) closure);
}

static void
path_interpret (GogRenderer 	 *rend,
		GOPath const 	 *path,
		double 		  line_width)
{
	if (rend->is_vector
	    || (go_path_get_options (path) & GO_PATH_OPTIONS_SNAP_COORDINATES) == 0) {
		go_path_interpret (path, GO_PATH_DIRECTION_FORWARD,
				   path_raw_move_to,
				   path_raw_line_to,
				   path_curve_to,
				   path_close_path,
				   rend->cairo);
		return;
	}

	if (((int) (go_fake_ceil (line_width)) % 2 == 0) && line_width > 1.0)
		go_path_interpret (path, GO_PATH_DIRECTION_FORWARD,
				   path_snap_even_move_to,
				   path_snap_even_line_to,
				   path_curve_to,
				   path_close_path,
				   rend->cairo);
	else
		go_path_interpret (path, GO_PATH_DIRECTION_FORWARD,
				   path_snap_odd_move_to,
				   path_snap_odd_line_to,
				   path_curve_to,
				   path_close_path,
				   rend->cairo);
}

typedef struct {
	cairo_t	*cairo;
	gboolean first_point;
} FillPathData;

static void
fill_path_line_to (void *closure, GOPathPoint const *point)
{
	FillPathData *data = closure;

	if (data->first_point) {
		cairo_move_to (data->cairo, point->x, point->y);
		data->first_point = FALSE;
	}
	else
		cairo_line_to (data->cairo, point->x, point->y);
}

static void
fill_path_curve_to (void *closure,
		    GOPathPoint const *point0,
		    GOPathPoint const *point1,
		    GOPathPoint const *point2)
{
	FillPathData *data = closure;

	cairo_curve_to (data->cairo,
			point0->x, point0->y,
			point1->x, point1->y,
			point2->x, point2->y);
}

static void
fill_path_close_path (void *closure)
{
}

static void
fill_path_interpret (GogRenderer  *rend,
		     GOPath const *path_0,
		     GOPath const *path_1)
{
	FillPathData data;

	data.first_point = TRUE;
	data.cairo = rend->cairo;

	go_path_interpret (path_0,
			   GO_PATH_DIRECTION_FORWARD,
			   fill_path_line_to,
			   fill_path_line_to,
			   fill_path_curve_to,
			   fill_path_close_path,
			   &data);
	if (path_1 != NULL)
		go_path_interpret (path_1,
				   GO_PATH_DIRECTION_BACKWARD,
				   fill_path_line_to,
				   fill_path_line_to,
				   fill_path_curve_to,
				   fill_path_close_path,
				   &data);
	cairo_close_path (rend->cairo);
}

void
gog_renderer_stroke_serie (GogRenderer *renderer,
			   GOPath const *path)
{
	GogStyle const *style;
	GOPathOptions line_options;
	double width;

	g_return_if_fail (IS_GOG_RENDERER (renderer));
	g_return_if_fail (renderer->cur_style != NULL);
	g_return_if_fail (IS_GO_PATH (path));

        style = renderer->cur_style;
	line_options = go_path_get_options (path);
	width = _grc_line_size (renderer,
			       style->interesting_fields & GOG_STYLE_OUTLINE ?
			       style->outline.width : style->line.width,
			       line_options & GO_PATH_OPTIONS_SNAP_WIDTH);

	if (gog_style_is_line_visible (style)) {
		path_interpret (renderer, path, width);
		emit_stroke (renderer, FALSE, go_path_get_options (path));
	}
}

void
gog_renderer_fill_serie (GogRenderer *renderer,
			 GOPath const *path,
			 GOPath const *close_path)
{
	GogStyle const *style;

	g_return_if_fail (IS_GOG_RENDERER (renderer));
	g_return_if_fail (renderer->cur_style != NULL);
	g_return_if_fail (IS_GO_PATH (path));

	style = renderer->cur_style;

	if (gog_style_is_fill_visible (style)) {
		fill_path_interpret (renderer, path, close_path);
		emit_fill (renderer, FALSE);
	}
}

/*****************************************************************************/

static void
grc_draw_path (GogRenderer *rend, ArtVpath const *vpath, ArtBpath const*bpath, gboolean sharp)
{
	GogStyle const *style = rend->cur_style;

	if (gog_style_is_line_visible (style)) {
		gboolean legend_line;
		double width;

		/* KLUDGE snap coordinate of legend line sample. */
		legend_line =!sharp
			&& !rend->is_vector
			&& vpath != NULL
			&& vpath[0].code == ART_MOVETO
			&& vpath[1].code == ART_LINETO
			&& vpath[2].code == ART_END
			&& (vpath[0].x == vpath[1].x || vpath[0].y == vpath[1].y);

		width = _grc_line_size (rend, style->line.width, sharp);

		grc_path (rend->cairo, (ArtVpath *) vpath, (ArtBpath *) bpath, width,
			  (sharp || legend_line) && !rend->is_vector);
		emit_stroke (rend, FALSE, sharp ? GO_PATH_OPTIONS_SHARP : 0);
	}

}

static void
grc_draw_polygon (GogRenderer *rend, ArtVpath const *vpath, ArtBpath const *bpath,
		  gboolean sharp, gboolean narrow)
{
	GogStyle const *style;
	double width;
	gboolean is_outline_visible, is_fill_visible;

	style = rend->cur_style;

	is_outline_visible = gog_style_is_outline_visible (style) && !narrow;
	is_fill_visible = gog_style_is_fill_visible (style);

	if (!is_outline_visible && !is_fill_visible)
		return;

        width = _grc_line_size (rend, style->outline.width, sharp);

	grc_path (rend->cairo, (ArtVpath *) vpath, (ArtBpath *) bpath, width,
		  sharp && !rend->is_vector);

	if (is_fill_visible)
		emit_fill (rend, is_outline_visible);

	if (is_outline_visible)
		emit_outline (rend, FALSE, sharp ? GO_PATH_OPTIONS_SHARP : 0);
}

/**
 * gog_renderer_draw_sharp_path :
 * @rend : #GogRenderer
 * @path  : #ArtVpath
 *
 * Draws @path using the outline elements of the current style,
 * trying to make line with sharp edge.
 **/

void
gog_renderer_draw_sharp_path (GogRenderer *rend, ArtVpath *path)
{
	g_return_if_fail (IS_GOG_RENDERER (rend));
	g_return_if_fail (rend->cur_style != NULL);

	grc_draw_path (rend, path, NULL, TRUE);
}

/**
 * gog_renderer_draw_path :
 * @rend: #GogRenderer
 * @path: #ArtVpath
 *
 * Draws @path using the outline elements of the current style.
 **/

void
gog_renderer_draw_path (GogRenderer *rend, ArtVpath const *path)
{
	g_return_if_fail (IS_GOG_RENDERER (rend));
	g_return_if_fail (rend->cur_style != NULL);

	grc_draw_path (rend, path, NULL, FALSE);
}

/**
 * gog_renderer_draw_sharp_polygon :
 * @rend : #GogRenderer
 * @path  : #ArtVpath
 * @narrow : if TRUE skip any outline the current style specifies.
 *
 * Draws @path and fills it with the fill elements of the current style,
 * trying to draw line with sharp edge.
 * If @narrow is false it alos outlines it using the outline elements.
 **/

void
gog_renderer_draw_sharp_polygon (GogRenderer *rend, ArtVpath *path, gboolean narrow)
{
	g_return_if_fail (IS_GOG_RENDERER (rend));
	g_return_if_fail (rend->cur_style != NULL);

	grc_draw_polygon (rend, path, NULL, TRUE, narrow);
}

/**
 * gog_renderer_draw_polygon :
 * @rend : #GogRenderer
 * @path  : #ArtVpath
 * @narrow : if TRUE skip any outline the current style specifies.
 *
 * Draws @path and fills it with the fill elements of the current style.
 * If @narrow is false it alos outlines it using the outline elements.
 **/

void
gog_renderer_draw_polygon (GogRenderer *rend, ArtVpath const *path, gboolean narrow)
{
	g_return_if_fail (IS_GOG_RENDERER (rend));
	g_return_if_fail (rend->cur_style != NULL);

	grc_draw_polygon (rend, path, NULL, FALSE, narrow);
}


/**
 * gog_renderer_draw_bezier_path :
 * @rend : #GogRenderer
 * @path  : #ArtBpath
 *
 * Draws @path using the outline elements of the current style.
 **/

void
gog_renderer_draw_bezier_path (GogRenderer *rend, ArtBpath const *path)
{
	g_return_if_fail (IS_GOG_RENDERER (rend));
	g_return_if_fail (rend->cur_style != NULL);

	grc_draw_path (rend, NULL, path, FALSE);
}

/**
 * gog_renderer_get_rectangle_vpath :
 * @rect:
 *
 * a utility routine to build a rectangle path.
 **/

ArtVpath *
gog_renderer_get_rectangle_vpath (GogViewAllocation const *rect)
{
	ArtVpath *path;

	path = g_new (ArtVpath, 6);

	path[0].x = path[3].x = path[4].x = rect->x;
	path[1].x = path[2].x = rect->x + rect->w;
	path[0].y = path[1].y = path[4].y = rect->y;
	path[2].y = path[3].y = rect->y + rect->h;
	path[0].code = ART_MOVETO;
	path[1].code = path[2].code = path[3].code = path[4].code = ART_LINETO;
	path[5].code = ART_END;

	return path;
}

/**
 * gog_renderer_draw_rectangle:
 * @rend: a #GogRenderer
 * @rect: position and extent of rectangle
 *
 * A utility routine to build a closed rectangle vpath.
 *
 **/

static void
draw_rectangle (GogRenderer *rend, GogViewAllocation const *rect, gboolean sharp)
{
	gboolean const narrow = (rect->w < 3.) || (rect->h < 3.);
	double o, o_2;
	ArtVpath path[6];

	if (!narrow) {
		o = gog_renderer_line_size (rend, rend->cur_style->outline.width);
		o_2 = o / 2.;
	} else
		o = o_2 = 0.;
	path[0].code = ART_MOVETO;
	path[1].code = ART_LINETO;
	path[2].code = ART_LINETO;
	path[3].code = ART_LINETO;
	path[4].code = ART_LINETO;
	path[5].code = ART_END;
	path[0].x = path[1].x = path[4].x = rect->x + o_2;
	path[2].x = path[3].x = path[0].x + rect->w - o;
	path[0].y = path[3].y = path[4].y = rect->y + o_2;
	path[1].y = path[2].y = path[0].y + rect->h - o;

	if (sharp)
		gog_renderer_draw_sharp_polygon (rend, path, narrow);
	else
		gog_renderer_draw_polygon (rend, path, narrow);
}

void
gog_renderer_draw_sharp_rectangle (GogRenderer *rend, GogViewAllocation const *rect)
{
	draw_rectangle (rend, rect, TRUE);
}

void
gog_renderer_draw_rectangle (GogRenderer *rend, GogViewAllocation const *rect)
{
	draw_rectangle (rend, rect, FALSE);
}

/**
 * gog_renderer_get_ring_wedge_vpath :
 * @x : center x coordinate
 * @y : center y coordinate
 * @width  : x radius
 * @height : y radius
 * @th0 : start arc angle
 * @th1 : stop arc angle
 *
 * a utility routine to build a ring wedge path.
 **/

ArtBpath *
gog_renderer_get_ring_wedge_bpath (double cx, double cy,
				   double rx_out, double ry_out,
				   double rx_in, double ry_in,
				   double th0, double th1)
{
	ArtBpath *path;
	double th_arc, th_out, th_in, th_delta, t, r;
	int i, n_segs;
	gboolean fill;
	gboolean draw_in, ellipse = FALSE;

	if (rx_out < rx_in) {
		r = rx_out;
		rx_out = rx_in;
		rx_in = r;
	}
	if (ry_out < ry_in) {
		r = ry_out;
		ry_out = ry_in;
		ry_in = r;
	}
	/* Here we tolerate slightly negative values for inside radius
	 * when deciding to fill. We use outside radius for comparison. */
	fill = rx_in >= -(rx_out * 1e-6) && ry_in >= -(ry_out * 1e-6);

	if (rx_out <= 0.0 || ry_out <= 0.0 || rx_out < rx_in || ry_out < ry_in)
		return NULL;

	/* Here also we use outside radius for comparison. If inside radius
	 * is high enough, we'll emit an arc, otherwise we just use lines to
	 * wedge center. */
	draw_in = fill && (rx_in > rx_out * 1e-6) && (ry_in > ry_out * 1e-6);

	if (th1 < th0) { t = th1; th1 = th0; th0 = t; }
	if (go_add_epsilon (th1 - th0) >= 2 * M_PI) {
		ellipse = TRUE;
		th1 = th0 + 2 * M_PI;
	}

	th_arc = th1 - th0;
	n_segs = ceil (fabs (th_arc / (M_PI * 0.5 + 0.001)));

	path = g_new (ArtBpath, (1 + n_segs) * (draw_in ? 2 : 1) + (fill ? (draw_in ? 2 : 3) : 1));

	path[0].x3 = cx + rx_out * cos (th1);
	path[0].y3 = cy + ry_out * sin (th1);
	path[0].code = ART_MOVETO;

	if (fill && !ellipse) {
		path[n_segs + 1].x3 = cx + rx_in * cos (th0);
		path[n_segs + 1].y3 = cy + ry_in * sin (th0);
		path[n_segs + 1].code = ART_LINETO;
		if (draw_in) {
			/* Ring wedge */
			path[2 * n_segs + 2].x3 = path[0].x3;
			path[2 * n_segs + 2].y3 = path[0].y3;
			path[2 * n_segs + 2].code = ART_LINETO;
			path[2 * n_segs + 3].code = ART_END;
		} else {
			/* Pie wedge */
			path[n_segs + 1].x3 = cx;
			path[n_segs + 1].y3 = cy;
			path[n_segs + 1].code = ART_LINETO;
			path[n_segs + 2].x3 = path[0].x3;
			path[n_segs + 2].y3 = path[0].y3;
			path[n_segs + 2].code = ART_LINETO;
			path[n_segs + 3].code = ART_END;
		}
	} else if (fill && ellipse) {
		if (draw_in) {
			path[n_segs + 1].x3 = cx + rx_in * cos (th0);
			path[n_segs + 1].y3 = cy + ry_in * sin (th0);
			path[n_segs + 1].code = ART_MOVETO;
			path[2 * n_segs + 2].code = ART_END;
		} else
			path[n_segs + 1].code = ART_END;
	} else
		/* Arc */
		path[n_segs + 1].code = ART_END;

	th_delta = th_arc / n_segs;
	t = - (8.0 / 3.0) * sin (th_delta * 0.25) * sin (th_delta * 0.25) / sin (th_delta * 0.5);
	th_out = th1;
	th_in = th0;
	for (i = 1; i <= n_segs; i++) {
		path[i].x1 = cx + rx_out * (cos (th_out) - t * sin (th_out));
		path[i].y1 = cy + ry_out * (sin (th_out) + t * cos (th_out));
		path[i].x3 = cx + rx_out * cos (th_out - th_delta);
		path[i].y3 = cy + ry_out * sin (th_out - th_delta);
		path[i].x2 = path[i].x3 + rx_out * t * sin (th_out - th_delta);
		path[i].y2 = path[i].y3 - ry_out * t * cos (th_out - th_delta);
		path[i].code = ART_CURVETO;
		th_out -= th_delta;
		if (draw_in) {
			path[i+n_segs+1].x1 = cx + rx_in * (cos (th_in) + t * sin (th_in));
			path[i+n_segs+1].y1 = cy + ry_in * (sin (th_in) - t * cos (th_in));
			path[i+n_segs+1].x3 = cx + rx_in * cos (th_in + th_delta);
			path[i+n_segs+1].y3 = cy + ry_in * sin (th_in + th_delta);
			path[i+n_segs+1].x2 = path[i+n_segs+1].x3 - rx_in * t * sin (th_in + th_delta);
			path[i+n_segs+1].y2 = path[i+n_segs+1].y3 + ry_in * t * cos (th_in + th_delta);
			path[i+n_segs+1].code = ART_CURVETO;
			th_in += th_delta;
		}
	}
	return path;
}

/**
 * gog_renderer_draw_ring_wedge :
 * @rend: #GogRenderer
 * @cx: center x coordinate
 * @cy: center y coordinate
 * @rx_out: x radius
 * @ry_out: y radius
 * @th0: start arc angle
 * @th1: stop arc angle
 *
 * a utility routine to draw an arc.
 **/

void
gog_renderer_draw_ring_wedge (GogRenderer *rend, double cx, double cy,
			      double rx_out, double ry_out,
			      double rx_in, double ry_in,
			      double th0, double th1,
			      gboolean narrow)
{
	ArtBpath *path;

	g_return_if_fail (IS_GOG_RENDERER (rend));
	g_return_if_fail (rend->cur_style != NULL);

	path = gog_renderer_get_ring_wedge_bpath (cx, cy, rx_out, ry_out, rx_in, ry_in, th0, th1);
	if (path == NULL)
		return;

	if (go_add_epsilon (rx_in) >= 0.0 && go_add_epsilon (ry_in) >= 0.0)
		grc_draw_polygon (rend, NULL, path, FALSE, narrow);
	else
		grc_draw_path (rend, NULL, path, FALSE);

	g_free (path);
}

/**
 * gog_renderer_draw_grip:
 * @renderer : #GogRenderer
 * @x : x position of grip
 * @y : y position of grip
 *
 * Draw a grip, used for moving/resizing of objects.
 **/
void
gog_renderer_draw_grip (GogRenderer *renderer, double x, double y)
{
	GogViewAllocation rectangle;

	rectangle.x = x - GOG_RENDERER_GRIP_SIZE;
	rectangle.y = y - GOG_RENDERER_GRIP_SIZE;
	rectangle.w = rectangle.h = 2 * GOG_RENDERER_GRIP_SIZE;

	gog_renderer_draw_sharp_rectangle (renderer, &rectangle);
}

static void
_draw_marker (cairo_t *cairo, GOMarker *marker, double x, double y, double scale, gboolean is_vector)
{
	ArtVpath *path;
	ArtVpath const *outline_path_raw, *fill_path_raw;
	double scaling[6], translation[6], affine[6];
	double half_size;

	go_marker_get_paths (marker, &outline_path_raw, &fill_path_raw);

	if ((outline_path_raw == NULL) ||
	    (fill_path_raw == NULL))
		return;

	cairo_set_line_cap (cairo, CAIRO_LINE_CAP_SQUARE);
	cairo_set_line_join (cairo, CAIRO_LINE_JOIN_MITER);

	half_size = 0.5 * (is_vector ? scale * go_marker_get_size (marker) :
			   rint (scale * go_marker_get_size (marker)));

	art_affine_scale (scaling, half_size , half_size);
	art_affine_translate (translation, x, y);
	art_affine_multiply (affine, scaling, translation);

	path = art_vpath_affine_transform (fill_path_raw, affine);
	cairo_set_source_rgba (cairo, GO_COLOR_TO_CAIRO (go_marker_get_fill_color (marker)));
	grc_path (cairo, path, NULL, 0.0, FALSE);
	cairo_fill (cairo);
	art_free (path);

	path = art_vpath_affine_transform (outline_path_raw, affine);
	cairo_set_source_rgba (cairo, GO_COLOR_TO_CAIRO (go_marker_get_outline_color (marker)));
	cairo_set_line_width (cairo, scale * go_marker_get_outline_width (marker));
	grc_path (cairo, path, NULL, 0.0, FALSE);
	cairo_stroke (cairo);
	art_free (path);
}

static cairo_surface_t *
_get_marker_surface (GogRenderer *rend)
{
	GOMarker *marker = rend->cur_style->marker.mark;
	cairo_t *cairo;
	cairo_surface_t *surface;
	double half_size, offset;

	if (rend->marker_surface != NULL)
		return rend->marker_surface;

	if (rend->is_vector) {
		half_size = rend->scale * go_marker_get_size (marker) / 2.0;
		offset = half_size + rend->scale * go_marker_get_outline_width (marker);
	} else {
		half_size = rint (rend->scale * go_marker_get_size (marker)) / 2.0;
		offset = ceil (rend->scale * go_marker_get_outline_width (marker) / 2.0) +
			half_size + .5;
	}

	surface = cairo_surface_create_similar (cairo_get_target (rend->cairo),
						CAIRO_CONTENT_COLOR_ALPHA,
						2.0 * offset,
						2.0 * offset);
	cairo = cairo_create (surface);

	_draw_marker (cairo, marker, offset, offset, rend->scale, rend->is_vector);

	cairo_destroy (cairo);

	rend->marker_surface = surface;
	rend->marker_x_offset = rend->marker_y_offset = offset;

	return surface;
}

/**
 * gog_renderer_draw_marker :
 * @rend : #GogRenderer
 * @x: X-coordinate
 * @y: Y-coordinate
 **/
void
gog_renderer_draw_marker (GogRenderer *rend, double x, double y)
{
	cairo_surface_t *surface;

	g_return_if_fail (IS_GOG_RENDERER (rend));
	g_return_if_fail (rend->cur_style != NULL);

	surface = _get_marker_surface (rend);

	if (surface == NULL)
		return;

	if (rend->is_vector) {
		if (rend->marker_as_surface) {
			cairo_set_source_surface (rend->cairo, surface,
						  x - rend->marker_x_offset,
						  y - rend->marker_y_offset);
			cairo_paint (rend->cairo);
		} else
			_draw_marker (rend->cairo, rend->cur_style->marker.mark,
					 x, y, rend->scale, TRUE);

	} else {
		cairo_set_source_surface (rend->cairo, surface,
					  floor (x - rend->marker_x_offset),
					  floor (y - rend->marker_y_offset));
		cairo_paint (rend->cairo);
	}
}

/**
 * gog_renderer_draw_text :
 * @rend   : #GogRenderer
 * @text   : the string to draw
 * @pos    : #GogViewAllocation
 * @anchor : #GtkAnchorType how to draw relative to @pos
 * @use_markup: wether to use pango markup
 *
 * Have @rend draw @text in the at @pos.{x,y} anchored by the @anchor corner.
 * If @pos.w or @pos.h are >= 0 then clip the results to less than that size.
 **/

void
gog_renderer_draw_text (GogRenderer *rend, char const *text,
			GogViewAllocation const *pos, GtkAnchorType anchor,
			gboolean use_markup)
{
	PangoLayout *layout;
	PangoContext *context;
	cairo_t *cairo = rend->cairo;
	GOGeometryOBR obr;
	GOGeometryAABR aabr;
	GogStyle const *style;
	int iw, ih;

	g_return_if_fail (IS_GOG_RENDERER (rend));
	g_return_if_fail (rend->cur_style != NULL);
	g_return_if_fail (text != NULL);

	if (*text == '\0')
		return;

	style = rend->cur_style;

	layout = pango_cairo_create_layout (cairo);
	context = pango_layout_get_context (layout);
	pango_cairo_context_set_resolution (context, 72.0);
	if (use_markup)
		pango_layout_set_markup (layout, text, -1);
	else
		pango_layout_set_text (layout, text, -1);
	pango_layout_set_font_description (layout, style->font.font->desc);
	pango_layout_get_size (layout, &iw, &ih);

	obr.w = rend->scale * (iw / (double)PANGO_SCALE);
	obr.h = rend->scale * (ih / (double)PANGO_SCALE);
	obr.alpha = -style->text_layout.angle * M_PI / 180.0;
	obr.x = pos->x;
	obr.y = pos->y;
	go_geometry_OBR_to_AABR (&obr, &aabr);

	switch (anchor) {
		case GTK_ANCHOR_NW: case GTK_ANCHOR_W: case GTK_ANCHOR_SW:
			obr.x += aabr.w / 2.0;
			break;
		case GTK_ANCHOR_NE : case GTK_ANCHOR_SE : case GTK_ANCHOR_E :
			obr.x -= aabr.w / 2.0;
			break;
		default : break;
	}

	switch (anchor) {
		case GTK_ANCHOR_NW: case GTK_ANCHOR_N: case GTK_ANCHOR_NE:
			obr.y += aabr.h / 2.0;
			break;
		case GTK_ANCHOR_SE : case GTK_ANCHOR_S : case GTK_ANCHOR_SW :
			obr.y -= aabr.h / 2.0;
			break;
		default : break;
	}

	cairo_save (cairo);
	cairo_set_source_rgba (cairo, GO_COLOR_TO_CAIRO (style->font.color));
	cairo_move_to (cairo, obr.x - (obr.w / 2.0) * cos (obr.alpha) + (obr.h / 2.0) * sin (obr.alpha),
		       obr.y - (obr.w / 2.0) * sin (obr.alpha) - (obr.h / 2.0) * cos (obr.alpha));
	cairo_rotate (cairo, obr.alpha);
	cairo_scale (cairo, rend->scale, rend->scale);
	pango_cairo_show_layout (cairo, layout);
	cairo_restore (cairo);
	g_object_unref (layout);
}

/**
 * gog_renderer_get_text_OBR :
 * @rend: #GogRenderer
 * @text: the string to draw
 * @use_markup: wether to use pango markup
 * @obr: #GOGeometryOBR to store the Object Bounding Rectangle of @text.
 **/
void
gog_renderer_get_text_OBR (GogRenderer *rend, char const *text,
			   gboolean use_markup, GOGeometryOBR *obr)
{
	GogStyle const *style;
	PangoLayout *layout;
	PangoContext *context;
	PangoRectangle logical;
	cairo_t *cairo = rend->cairo;

	g_return_if_fail (IS_GOG_RENDERER (rend));
	g_return_if_fail (rend->cur_style != NULL);
	g_return_if_fail (text != NULL);
	g_return_if_fail (obr != NULL);

	obr->x = obr->y = 0;
	if (*text == '\0') {
		/* Make sure invisible things don't skew size */
		obr->w = obr->h = 0;
		obr->alpha = 0;
		return;
	}

	style = rend->cur_style;

	layout = pango_cairo_create_layout (cairo);
	context = pango_layout_get_context (layout);
	pango_cairo_context_set_resolution (context, 72.0);
	if (use_markup)
		pango_layout_set_markup (layout, text, -1);
	else
		pango_layout_set_text (layout, text, -1);
	pango_layout_set_font_description (layout, style->font.font->desc);
	pango_layout_get_extents (layout, NULL, &logical);
	g_object_unref (layout);

	obr->w = rend->scale * ((double) logical.width + (double) PANGO_SCALE / 2.0) / (double) PANGO_SCALE;
	obr->h = rend->scale * ((double) logical.height + (double) PANGO_SCALE / 2.0) /(double) PANGO_SCALE;

	/* Make sure invisible things don't skew size */
	if (obr->w == 0)
		obr->h = 0;
	else if (obr->h == 0)
		obr->w = 0;

	obr->alpha = - style->text_layout.angle * M_PI / 180.0;
}

/**
 * gog_renderer_get_text_AABR :
 * @rend: #GogRenderer
 * @text: the string to draw
 * @use_markup: wether to use pango markup
 * @aabr: #GOGeometryAABR to store the Axis Aligned Bounding Rectangle of @text.
 **/
void
gog_renderer_get_text_AABR (GogRenderer *rend, char const *text,
			    gboolean use_markup, GOGeometryAABR *aabr)
{
	GOGeometryOBR obr;

	gog_renderer_get_text_OBR (rend, text, use_markup, &obr);
	go_geometry_OBR_to_AABR (&obr, aabr);
}

/**
 * gog_renderer_push_clip :
 * @rend      : #GogRenderer
 * @clip_path : #ArtVpath array
 *
 * Defines the current clipping region. 
 **/
void
gog_renderer_push_clip (GogRenderer *rend, ArtVpath *clip_path)
{
	GogRendererClip *clip;
	ArtVpath *path;
	int i;
	gboolean is_rectangle;

	g_return_if_fail (IS_GOG_RENDERER (rend));
	g_return_if_fail (clip_path != NULL);

	clip = g_new (GogRendererClip, 1);
	clip->path = clip_path;

	rend->clip_stack = g_slist_prepend (rend->clip_stack, clip);
	rend->cur_clip = clip;

	path = clip->path;

	if (!rend->is_vector) {
		for (i = 0; i < 6; i++)
			if (path[i].code == ART_END)
				break;

		is_rectangle = i == 5 &&
			path[5].code == ART_END &&
			path[0].x == path[3].x &&
			path[0].x == path[4].x &&
			path[1].x == path[2].x &&
			path[0].y == path[1].y &&
			path[0].y == path[4].y &&
			path[2].y == path[3].y;
	} else
		is_rectangle = FALSE;

	cairo_save (rend->cairo);
	if (is_rectangle) {
		double x = go_fake_floor (path[0].x);
		double y = go_fake_floor (path[0].y);
		cairo_rectangle (rend->cairo, x, y,
				 go_fake_ceil (path[1].x) - x,
				 go_fake_ceil (path[2].y) - y);
	} else {
		grc_path (rend->cairo, path, NULL, 0.0, FALSE);
	}
	cairo_clip (rend->cairo);
}

/**
 * gog_renderer_pop_clip :
 * @rend : #GogRenderer
 *
 * End the current clipping.
 **/
void
gog_renderer_pop_clip (GogRenderer *rend)
{
	GogRendererClip *clip;

	g_return_if_fail (IS_GOG_RENDERER (rend));
	g_return_if_fail (rend->clip_stack != NULL);

	clip = (GogRendererClip *) rend->clip_stack->data;

	cairo_restore (rend->cairo);

	g_free (clip->path);
	g_free (clip);
	rend->clip_stack = g_slist_delete_link (rend->clip_stack, rend->clip_stack);

	if (rend->clip_stack != NULL)
		rend->cur_clip = (GogRendererClip *) rend->clip_stack->data;
	else
		rend->cur_clip = NULL;
}

static void
_free_marker_data (GogRenderer *rend)
{
	if (rend->marker_surface != NULL) {
		cairo_surface_destroy (rend->marker_surface);
		rend->marker_surface = NULL;
	}
}

void
gog_renderer_push_style (GogRenderer *rend, GogStyle const *style)
{
	g_return_if_fail (IS_GOG_RENDERER (rend));
	g_return_if_fail (IS_GOG_STYLE (style));

	if (rend->cur_style != NULL)
		rend->style_stack = g_slist_prepend (
			rend->style_stack, (gpointer)rend->cur_style);
	g_object_ref ((gpointer)style);
	rend->cur_style = style;

	_free_marker_data (rend);

	_update_dash (rend);
}

/**
 * gog_renderer_push_selection_style:
 * @renderer : #GogRenderer
 *
 * Push a style used for selection and grip rendering.
 **/
void
gog_renderer_push_selection_style (GogRenderer *renderer)
{
	if (renderer->grip_style == NULL) {
		renderer->grip_style = gog_style_new ();
		renderer->grip_style->outline.dash_type = GO_LINE_SOLID;
		renderer->grip_style->outline.width = 0.0;
		renderer->grip_style->outline.color =
		renderer->grip_style->fill.pattern.back = 0xff000080;
		renderer->grip_style->line.dash_type = GO_LINE_DOT;
		renderer->grip_style->line.width = 0.0;
		renderer->grip_style->line.color = 0x0000ffB0;
		renderer->grip_style->fill.pattern.pattern = GO_PATTERN_SOLID;
		renderer->grip_style->fill.type = GOG_FILL_STYLE_PATTERN;
	}
	gog_renderer_push_style (renderer, renderer->grip_style);
}

void
gog_renderer_pop_style (GogRenderer *rend)
{
	g_return_if_fail (IS_GOG_RENDERER (rend));
	g_return_if_fail (IS_GOG_STYLE (rend->cur_style));

	g_object_unref ((gpointer)rend->cur_style);
	if (rend->style_stack != NULL) {
		rend->cur_style = rend->style_stack->data;
		rend->style_stack = g_slist_remove (rend->style_stack,
			rend->cur_style);
	} else
		rend->cur_style = NULL;

	_free_marker_data (rend);

	_update_dash (rend);
}

void
gog_renderer_request_update (GogRenderer *renderer)
{
	g_return_if_fail (IS_GOG_RENDERER (renderer));

	if (renderer->needs_update)
		return;
	renderer->needs_update = TRUE;
	g_signal_emit (G_OBJECT (renderer),
		renderer_signals [RENDERER_SIGNAL_REQUEST_UPDATE], 0);
}

/**
 * gog_renderer_update:
 * @renderer: a #GogRenderer
 * @w: requested width
 * @h: requested height
 * @zoom: requested zoom
 *
 * Requests a renderer update, only useful for pixbuf based renderer.
 **/
gboolean
gog_renderer_update (GogRenderer *rend, double w, double h, double zoom)
{
	GogGraph *graph;
	GogView *view;
	GogViewAllocation allocation;
	GOImage *image = NULL;
	gboolean redraw = TRUE;
	gboolean size_changed;
	gboolean create_cairo = rend->cairo == NULL;

	g_return_val_if_fail (IS_GOG_RENDERER (rend), FALSE);
	g_return_val_if_fail (IS_GOG_VIEW (rend->view), FALSE);

	size_changed = rend->w != w || rend->h != h;
	if (size_changed && create_cairo) {
		if (rend->pixbuf != NULL)
			g_object_unref (rend->pixbuf);
		rend->pixbuf = NULL;
		if (w == 0 || h == 0)
			return FALSE;
		rend->w = w;
		rend->h = h;
		rend->pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, rend->w, rend->h);
		if (rend->pixbuf == NULL) {
			rend->w = 0;
			rend->h = 0;
			g_warning ("GogRendererCairo::cairo_setup: chart is too large");
			return FALSE;
		}
	}
	if (w == 0 || h == 0)
		return FALSE;

	view = rend->view;
	graph = GOG_GRAPH (view->model);
	gog_graph_force_update (graph);

	allocation.x = allocation.y = 0.;
	allocation.w = w;
	allocation.h = h;

	if (create_cairo) {
		image = go_image_new_from_pixbuf (rend->pixbuf);
		rend->cairo = go_image_get_cairo (image);
		rend->is_vector = FALSE;
	}

	if (size_changed) {
		rend->scale_x = (graph->width >= 1.) ? (w / graph->width) : 1.;
		rend->scale_y = (graph->height >= 1.) ? (h / graph->height) : 1.;
		rend->scale = MIN (rend->scale_x, rend->scale_y);
		rend->zoom  = zoom;

		/* make sure we dont try to queue an update while updating */
		rend->needs_update = TRUE;

		/* scale just changed need to recalculate sizes */
		gog_renderer_request_update (rend);
		gog_view_size_allocate (view, &allocation);
	} else
		if (w != view->allocation.w || h != view->allocation.h)
			gog_view_size_allocate (view, &allocation);
		else
			redraw = gog_view_update_sizes (view);

	redraw |= rend->needs_update;
	rend->needs_update = FALSE;

	if (redraw) {
		if (create_cairo) {
			/* clear if we created it, not otherwise */
			cairo_set_operator (rend->cairo, CAIRO_OPERATOR_CLEAR);
			cairo_paint (rend->cairo);
		}
		cairo_set_operator (rend->cairo, CAIRO_OPERATOR_OVER);

		cairo_set_line_join (rend->cairo, CAIRO_LINE_JOIN_ROUND);
		cairo_set_line_cap (rend->cairo, CAIRO_LINE_CAP_ROUND);

		gog_view_render	(view, NULL);

		if (create_cairo)
			go_image_get_pixbuf (image);
	}

	if (create_cairo) {
		g_object_unref (image);
		cairo_destroy (rend->cairo);
		rend->cairo = NULL;
	}
	return redraw;
}

/** 
 * gog_renderer_get_pixbuf:
 * @renderer : #GogRenderer
 *
 * Returns current pixbuf buffer from a renderer that can render into a pixbuf.
 **/
GdkPixbuf*
gog_renderer_get_pixbuf (GogRenderer *rend)
{
	g_return_val_if_fail (IS_GOG_RENDERER (rend), NULL);

	return rend->pixbuf;
}

static gboolean
_gsf_gdk_pixbuf_save (const gchar *buf,
		      gsize count,
		      GError **error,
		      gpointer data)
{
	GsfOutput *output = GSF_OUTPUT (data);
	gboolean ok = gsf_output_write (output, count, buf);

	if (!ok && error)
		*error = g_error_copy (gsf_output_error (output));

	return ok;
}

static cairo_status_t
_cairo_write_func (void *closure,
		   const unsigned char *data,
		   unsigned int length)
{
	gboolean result;
	GsfOutput *output = GSF_OUTPUT (closure);

	result = gsf_output_write (output, length, data);

	return result ? CAIRO_STATUS_SUCCESS : CAIRO_STATUS_WRITE_ERROR;
}

/**
 * gog_renderer_export_image:
 * @renderer: a #GogRenderer
 * @format: image format for export
 * @output: a #GsfOutput stream
 * @x_dpi: x resolution of exported graph
 * @y_dpi: y resolution of exported graph
 *
 * Exports an image of @graph in given @format, writing results in a #GsfOutput stream.
 * If export format type is a bitmap one, it computes image size with x_dpi, y_dpi and 
 * @graph size (see gog_graph_get_size()).
 *
 * returns: %TRUE if export succeed.
 **/

gboolean
gog_renderer_export_image (GogRenderer *rend, GOImageFormat format,
			   GsfOutput *output, double x_dpi, double y_dpi)
{
	GogViewAllocation allocation;
	GOImageFormatInfo const *format_info;
	cairo_surface_t *surface = NULL;
	cairo_status_t status;
	GdkPixbuf *pixbuf, *output_pixbuf;
	double width_in_pts, height_in_pts;
	gboolean result;

	g_return_val_if_fail (IS_GOG_RENDERER (rend), FALSE);

	if (x_dpi <= 0.)
		x_dpi = 96.;
	if (y_dpi <= 0.)
		y_dpi = 96.;
	gog_graph_force_update (rend->model);

	gog_graph_get_size (rend->model, &width_in_pts, &height_in_pts);

	switch (format) {
		case GO_IMAGE_FORMAT_PDF:
		case GO_IMAGE_FORMAT_PS:
		case GO_IMAGE_FORMAT_SVG:
			rend->scale = 1.0;
			switch (format) {
				case GO_IMAGE_FORMAT_PDF:
					rend->marker_as_surface = FALSE;
#ifdef CAIRO_HAS_PDF_SURFACE
					surface = cairo_pdf_surface_create_for_stream
						(_cairo_write_func,
						 output, width_in_pts, height_in_pts);
#ifdef HAVE_CAIRO_SURFACE_SET_FALLBACK_RESOLUTION
					cairo_surface_set_fallback_resolution (surface, x_dpi, y_dpi);
#endif
					break;
#else
					g_warning ("[GogRendererCairo::export_image] cairo PDF backend missing");
					return FALSE;
#endif
				case GO_IMAGE_FORMAT_PS:
					rend->marker_as_surface = FALSE;
#ifdef CAIRO_HAS_PS_SURFACE
					surface = cairo_ps_surface_create_for_stream
						(_cairo_write_func,
						 output, width_in_pts, height_in_pts);
#ifdef HAVE_CAIRO_SURFACE_SET_FALLBACK_RESOLUTION
					cairo_surface_set_fallback_resolution (surface, x_dpi, y_dpi);
#endif
					break;
#else
					g_warning ("[GogRendererCairo::export_image] cairo PS backend missing");
					return FALSE;
#endif
				case GO_IMAGE_FORMAT_SVG:
					rend->marker_as_surface = TRUE;
#ifdef CAIRO_HAS_SVG_SURFACE
					surface = cairo_svg_surface_create_for_stream
						(_cairo_write_func,
						 output, width_in_pts, height_in_pts);
#ifdef HAVE_CAIRO_SURFACE_SET_FALLBACK_RESOLUTION
					cairo_surface_set_fallback_resolution (surface, x_dpi, y_dpi);
#endif
					break;
#else
					g_warning ("[GogRendererCairo::export_image] cairo SVG backend missing");
					return FALSE;
#endif
				default:
					break;
			}
			rend->cairo = cairo_create (surface);
			cairo_surface_destroy (surface);
			cairo_set_line_join (rend->cairo, CAIRO_LINE_JOIN_ROUND);
			cairo_set_line_cap (rend->cairo, CAIRO_LINE_CAP_ROUND);
			rend->w = width_in_pts;
			rend->h = height_in_pts;
			rend->is_vector = TRUE;

			allocation.x = 0.;
			allocation.y = 0.;
			allocation.w = width_in_pts;
			allocation.h = height_in_pts;
			gog_view_size_allocate (rend->view, &allocation);
			gog_view_render	(rend->view, NULL);

			cairo_show_page (rend->cairo);
			status = cairo_status (rend->cairo);

			cairo_destroy (rend->cairo);
			rend->cairo = NULL;

			return status == CAIRO_STATUS_SUCCESS;
			break;
		default:
			format_info = go_image_get_format_info (format);
			if (!format_info->has_pixbuf_saver) {
				g_warning ("[GogRendererCairo:export_image] unsupported format");
				return FALSE;
			}

			gog_renderer_update (rend,
					     width_in_pts * x_dpi / 72.0,
					     height_in_pts * y_dpi / 72.0, 1.0);
			pixbuf = gog_renderer_get_pixbuf (rend);
			if (pixbuf == NULL)
				return FALSE;
			format_info = go_image_get_format_info (format);
			if (!format_info->alpha_support)
				output_pixbuf = gdk_pixbuf_composite_color_simple (pixbuf,
										   gdk_pixbuf_get_width (pixbuf),
										   gdk_pixbuf_get_height (pixbuf),
										   GDK_INTERP_NEAREST,
										   255, 256, 0xffffffff,
										   0xffffffff);

			else
				output_pixbuf = pixbuf;
			result = gdk_pixbuf_save_to_callback (output_pixbuf,
							      _gsf_gdk_pixbuf_save,
							      output, format_info->name,
							      NULL, NULL);
			if (!format_info->alpha_support)
				g_object_unref (output_pixbuf);
			return result;
			break;
	}

	return FALSE;
}

/**
 * gog_renderer_get_hairline_width_pts:
 * @rend: a #GogRenderer
 *
 * returns the hairline width in pts.
 **/
double
gog_renderer_get_hairline_width_pts (GogRenderer const *rend)
{
	g_return_val_if_fail (IS_GOG_RENDERER (rend), GOG_RENDERER_HAIRLINE_WIDTH_PTS);

	if (rend->is_vector
	    || go_sub_epsilon (rend->scale) <= 0.0)
	       return GOG_RENDERER_HAIRLINE_WIDTH_PTS;

	return 1.0 / rend->scale;
}

/**
 * gog_renderer_new_for_pixbuf:
 * @graph : graph model
 *
 * Creates a new renderer which can render into a pixbuf, and sets
 * @graph as its model.
 **/
GogRenderer*
gog_renderer_new_for_pixbuf (GogGraph *graph)
{
	return g_object_new (GOG_RENDERER_TYPE, "model", graph, NULL);
}

/**
 * gog_renderer_new_for_format:
 * @graph: a #GogGraph
 * @format: image format
 * 
 * Creates a new #GogRenderer which is capable of export to @format.
 *
 * returns: a new #GogRenderer.
 **/

GogRenderer *
gog_renderer_new_for_format (GogGraph *graph, GOImageFormat format)
{
	return g_object_new (GOG_RENDERER_TYPE, "model", graph, NULL);
}

static void
_cb_font_removed (GogRenderer *rend, GOFont const *font)
{
	g_return_if_fail (IS_GOG_RENDERER (rend));

	gog_debug (0, g_warning ("notify a '%s' that %p is invalid",
				 G_OBJECT_TYPE_NAME (rend), font););
}

static void
gog_renderer_init (GogRenderer *rend)
{
	rend->cairo = NULL;
	rend->pixbuf = NULL;
	rend->marker_surface = NULL;
	rend->w = rend->h = 0;
	rend->is_vector = FALSE;

	rend->cur_clip = NULL;
	rend->clip_stack = NULL;

	rend->line_dash = NULL;
	rend->outline_dash = NULL;

	rend->needs_update = FALSE;
	rend->cur_style    = NULL;
	rend->style_stack  = NULL;
	rend->zoom = rend->scale = rend->scale_x = rend->scale_y = 1.;
	rend->font_watcher = g_cclosure_new_swap (G_CALLBACK (_cb_font_removed),
		rend, NULL);
	go_font_cache_register (rend->font_watcher);

	rend->grip_style = NULL;
}

static void
gog_renderer_finalize (GObject *obj)
{
	GogRenderer *rend = GOG_RENDERER (obj);

	if (rend->cairo != NULL){
		cairo_destroy (rend->cairo);
		rend->cairo = NULL;
	}
	if (rend->pixbuf != NULL) {
		g_object_unref (rend->pixbuf);
		rend->pixbuf = NULL;
	}
	_free_marker_data (rend);

	go_line_vpath_dash_free (rend->line_dash);
	rend->line_dash = NULL;
	go_line_vpath_dash_free (rend->outline_dash);
	rend->outline_dash = NULL;

	if (rend->grip_style != NULL) {
		g_object_unref (rend->grip_style);
		rend->grip_style = NULL;
	}

	if (rend->clip_stack != NULL)
		g_warning ("Missing calls to gog_renderer_pop_clip");

	if (rend->cur_style != NULL) {
		g_warning ("Missing calls to gog_renderer_style_pop left dangling style references");
		g_slist_foreach (rend->style_stack,
			(GFunc)g_object_unref, NULL);
		g_slist_free (rend->style_stack);
		rend->style_stack = NULL;
		g_object_unref ((gpointer)rend->cur_style);
		rend->cur_style = NULL;
	}

	if (rend->view != NULL) {
		g_object_unref (rend->view);
		rend->view = NULL;
	}

	if (rend->font_watcher != NULL) {
		go_font_cache_unregister (rend->font_watcher);
		g_closure_unref (rend->font_watcher);
		rend->font_watcher = NULL;
	}

	(*parent_klass->finalize) (obj);
}

static void
gog_renderer_set_property (GObject *obj, guint param_id,
			   GValue const *value, GParamSpec *pspec)
{
	GogRenderer *rend = GOG_RENDERER (obj);

	switch (param_id) {
	case RENDERER_PROP_MODEL:
		rend->model = GOG_GRAPH (g_value_get_object (value));
		if (rend->view != NULL)
			g_object_unref (rend->view);
		rend->view = g_object_new (gog_graph_view_get_type (),
					   "renderer",	rend,
					   "model",	rend->model,
					   NULL);
		gog_renderer_request_update (rend);
		break;

	case RENDERER_PROP_ZOOM:
		rend->zoom = g_value_get_double (value);
		break;

	case RENDERER_PROP_CAIRO:
		if (rend->cairo)
			cairo_destroy (rend->cairo);
		rend->cairo = g_value_get_pointer (value);
		if (rend->cairo)
			cairo_reference (rend->cairo);
		break;

	case RENDERER_PROP_IS_VECTOR:
		rend->is_vector = g_value_get_boolean (value);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
}

static void
gog_renderer_get_property (GObject *obj, guint param_id,
			   GValue *value, GParamSpec *pspec)
{
	GogRenderer const *rend = GOG_RENDERER (obj);

	switch (param_id) {
	case RENDERER_PROP_MODEL:
		g_value_set_object (value, rend->model);
		break;
	case RENDERER_PROP_VIEW:
		g_value_set_object (value, rend->view);
		break;
	case RENDERER_PROP_ZOOM:
		g_value_set_double (value, rend->zoom);
		break;
	case RENDERER_PROP_CAIRO:
		g_value_set_pointer (value, rend->cairo);
		break;
	case RENDERER_PROP_IS_VECTOR:
		g_value_set_boolean (value, rend->is_vector);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
gog_renderer_class_init (GogRendererClass *renderer_klass)
{
	GObjectClass *gobject_klass = (GObjectClass *)renderer_klass;

	parent_klass = g_type_class_peek_parent (gobject_klass);
	gobject_klass->finalize	    = gog_renderer_finalize;
	gobject_klass->set_property = gog_renderer_set_property;
	gobject_klass->get_property = gog_renderer_get_property;

	g_object_class_install_property (gobject_klass, RENDERER_PROP_MODEL,
		g_param_spec_object ("model",
			_("Model"),
			_("The GogGraph this renderer displays"),
			GOG_GRAPH_TYPE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (gobject_klass, RENDERER_PROP_VIEW,
		g_param_spec_object ("view",
			_("View"),
			_("the GogView this renderer is displaying"),
			GOG_VIEW_TYPE,
			GSF_PARAM_STATIC | G_PARAM_READABLE));
	g_object_class_install_property (gobject_klass, RENDERER_PROP_ZOOM,
		g_param_spec_double ("zoom",
			_("Zoom Height Pts"),
			_("Global scale factor"),
			1., G_MAXDOUBLE, 1.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (gobject_klass, RENDERER_PROP_CAIRO,
		g_param_spec_pointer ("cairo",
			_("Cairo"),
			_("The cairo_t* this renderer uses"),
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (gobject_klass, RENDERER_PROP_IS_VECTOR,
		g_param_spec_boolean ("is-vector",
			_("Is vector"),
			_("Tells if the cairo surface is vectorial or raster"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));

	renderer_signals [RENDERER_SIGNAL_REQUEST_UPDATE] = g_signal_new ("request-update",
		G_TYPE_FROM_CLASS (renderer_klass),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GogRendererClass, request_update),
		NULL, NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
}

GSF_CLASS (GogRenderer, gog_renderer,
	   gog_renderer_class_init, gog_renderer_init,
	   G_TYPE_OBJECT)
