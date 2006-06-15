/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-cairo.c
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

/*
 * TODO
 * 	- preserve path in draw_shape when sharp == FALSE.
 * 	- remove libart stuff.
 * 	- use GOImage.
 * 	- take screen resolution into account for image and patterns
 * 	- cache pango_layout
 */

#include <glib.h>

#include <goffice/utils/go-color.h>
#include <goffice/utils/go-math.h>
#include <goffice/utils/go-marker.h>
#include <goffice/utils/go-cairo.h>
#include <goffice/utils/go-path-impl.h>

#define GO_CAIRO_HAIR_LINE_WIDTH	0.24

struct _GOCairo {
	cairo_t *cairo;

	double scale;

	GogStyle const *current_style;
	GSList   *style_stack;

	gboolean is_vector;

	ArtVpathDash *line_dash;
	ArtVpathDash *outline_dash;
};

GOCairo *
go_cairo_new (cairo_t *cairo, double scale)
{
	cairo_surface_t *surface;
	GOCairo *gcairo = g_new (GOCairo, 1);

	gcairo->current_style = NULL;
	gcairo->style_stack = NULL;

	gcairo->cairo = cairo;
	cairo_reference (cairo);

	gcairo->scale = scale;

	gcairo->line_dash =
	gcairo->outline_dash = NULL;
	gcairo->is_vector = FALSE;

	surface = cairo_get_target (cairo);
#if 0
	switch (cairo_surface_get_type (surface)) {
		case CAIRO_SURFACE_TYPE_SVG:
		case CAIRO_SURFACE_TYPE_PDF:
		case CAIRO_SURFACE_TYPE_PS:
			gcairo->is_vector = TRUE;
			break;
		default:
			gcairo->is_vector = FALSE;
	}
#endif
	return gcairo;
}

void
go_cairo_free (GOCairo *gcairo)
{
	g_return_if_fail (IS_GO_CAIRO (gcairo));

	if (gcairo->current_style != NULL) {
		g_warning ("[GOCairo::free] missing calls to go_cairo_pop_style left dangling style references");
		g_slist_foreach (gcairo->style_stack,
			(GFunc)g_object_unref, NULL);
		g_slist_free (gcairo->style_stack);
		gcairo->style_stack = NULL;
		g_object_unref ((gpointer)gcairo->current_style);
		gcairo->current_style = NULL;
	}

	cairo_destroy (gcairo->cairo);

	g_free (gcairo);
}

static void 
go_cairo_emit_ring_wedge (GOCairo *gcairo,
			  double cx, double cy, 
			  double rx_out, double ry_out,
			  double rx_in, double ry_in,
			  double th0, double th1)
{
	cairo_t *cairo = gcairo->cairo;
	double th_arc, th_out, th_in, th_delta, t;
	double x, y;
	int i, n_segs;
	gboolean fill = rx_in >= 0.0 && ry_in >= 0.0;
	gboolean draw_in, ellipse = FALSE;

 	if (rx_out <= 0.0 || ry_out <= 0.0 || rx_out < rx_in || ry_out < ry_in)
		return;

	draw_in = fill && (rx_in > rx_out / 1E6) && (ry_in > ry_out / 1E6);

	if (th1 < th0) { t = th1; th1 = th0; th0 = t; }
	if (go_add_epsilon (th1 - th0) >= 2 * M_PI) {
		ellipse = TRUE;
		th1 = th0 + 2 * M_PI;
	}

	th_arc = th1 - th0;
	n_segs = ceil (fabs (th_arc / (M_PI * 0.5 + 0.001)));

	cairo_move_to (cairo, cx + rx_out * cos (th1), cy + ry_out * sin (th1));

	th_delta = th_arc / n_segs;
	t = - (8.0 / 3.0) * sin (th_delta * 0.25) * sin (th_delta * 0.25) / sin (th_delta * 0.5);
	th_out = th1;
	th_in = th0;
	
	for (i = 1; i <= n_segs; i++) {	
		x = cx + rx_out * cos (th_out - th_delta);
		y = cy + ry_out * sin (th_out - th_delta);
		cairo_curve_to (cairo, 
				cx + rx_out * (cos (th_out) - t * sin (th_out)),
				cy + ry_out * (sin (th_out) + t * cos (th_out)),
				x + rx_out * t * sin (th_out - th_delta),
				y - ry_out * t * cos (th_out - th_delta),
				x, y);
		th_out -= th_delta;
	}
	
	if (fill && !ellipse) {
		cairo_line_to (cairo, cx + rx_in * cos (th0), cy + ry_in * sin (th0));
		if (!draw_in) {
			/* Pie wedge */
			cairo_line_to (cairo, cx, cy);
			cairo_close_path (cairo);
			return;
		}
	}
	else
		/* Arc */
		return;

	/* Ring wedge */
	for (i = 1; i <= n_segs; i++) {	
		x = cx + rx_in * cos (th_in + th_delta);
		y = cy + ry_in * sin (th_in + th_delta);
		cairo_curve_to (cairo,
				cx + rx_in * (cos (th_in) + t * sin (th_in)),
				cy + ry_in * (sin (th_in) - t * cos (th_in)),
				x - rx_in * t * sin (th_in + th_delta),
				y + ry_in * t * cos (th_in + th_delta),
				x, y);
		th_in += th_delta;
	}
	cairo_close_path (cairo);
}

static void
go_cairo_set_path (GOCairo *gcairo, GOPath const *path, GOPathSnapType snap_type)
{
	GOPathDataBuffer *buffer;
	GOPathData *data;
	cairo_t *cairo = gcairo->cairo;
	double pre = (snap_type == GO_PATH_SNAP_ROUND) ? 0.5 : 0.0;
	double post = (snap_type == GO_PATH_SNAP_ROUND) ? 0.0 : 0.5;
	int i;
	
	g_return_if_fail (path != NULL);

	buffer = path->data_buffer_head;
	while (buffer != NULL) {
		for (i = 0; i < buffer->num_data; i++) {
			data = &buffer->data[i];
			switch (data->action & GO_PATH_ACTION_MASK) {
				case GO_PATH_ACTION_MOVE_TO:
					if (snap_type == GO_PATH_SNAP_NONE)
						cairo_move_to (cairo, data->point.x, data->point.y);
					else
						cairo_move_to (cairo, 
							       floor (data->point.x + pre) + post, 
							       floor (data->point.y + pre) + post);
					break;
				case GO_PATH_ACTION_LINE_TO:
					if (snap_type == GO_PATH_SNAP_NONE)
						cairo_line_to (cairo, data->point.x, data->point.y);
					else
						cairo_line_to (cairo, 
							       floor (data->point.x + pre) + post, 
							       floor (data->point.y + pre) + post);
					break;
				case GO_PATH_ACTION_CURVE_TO:
					if (snap_type == GO_PATH_SNAP_NONE)
						cairo_curve_to (cairo, 
								data[0].point.x, data[0].point.y,
								data[1].point.x, data[1].point.y,
								data[2].point.x, data[2].point.y);
					else
						cairo_curve_to (cairo, 
								floor (data[0].point.x + pre) + post, 
								floor (data[0].point.y + pre) + post,
								floor (data[1].point.x + pre) + post, 
								floor (data[1].point.y + pre) + post,
								floor (data[2].point.x + pre) + post, 
								floor (data[2].point.y + pre) + post);
					i += 2;
					break;
				case GO_PATH_ACTION_CLOSE_PATH:
					cairo_close_path (cairo);
					break;
				case GO_PATH_ACTION_ARC:
					go_cairo_emit_ring_wedge (gcairo, 
								  data[0].point.x, data[0].point.y,
								  data[1].point.x, data[1].point.y,
								  -1., -1.,
								  data[2].point.x, data[2].point.y);
					i += 2;
					break;
				case GO_PATH_ACTION_PIE_WEDGE:
					go_cairo_emit_ring_wedge (gcairo, 
								  data[0].point.x, data[0].point.y,
								  data[1].point.x, data[1].point.y,
								  0., 0.,
								  data[2].point.x, data[2].point.y);
					i += 2;
					break;
				case GO_PATH_ACTION_RING_WEDGE:
					go_cairo_emit_ring_wedge (gcairo, 
								  data[0].point.x, data[0].point.y,
								  data[1].point.x, data[1].point.y,
								  data[2].point.x, data[2].point.y,
								  data[3].point.x, data[3].point.y);
					i += 3;
					break;
				case GO_PATH_ACTION_RECTANGLE:
					if (snap_type == GO_PATH_SNAP_NONE)
						cairo_rectangle (gcairo->cairo, 
								 data[0].point.x, data[0].point.y,
								 data[1].point.x, data[1].point.y);
					else {
						double x = floor (data[0].point.x + pre) + post;
						double y = floor (data[0].point.y + pre) + post;
						cairo_rectangle (gcairo->cairo, x, y,
								 (floor (data[0].point.x + 
									 data[1].point.x + 
									 pre) + post) - x,
								 (floor (data[0].point.y + 
									 data[1].point.y + 
									 pre) + post) - y);
					}
					i += 1;
					break;
				default:
					g_message ("[GOCairo::set_path] unimplemented feature");
			}
		}
		buffer = buffer->next;
	}
}

double
go_cairo_line_size (GOCairo const *gcairo, double width)
{
	if (gcairo->is_vector)
		return go_sub_epsilon (width) <= 0. ?
			GO_CAIRO_HAIR_LINE_WIDTH :
			width * gcairo->scale;

	if (go_sub_epsilon (width) <= 0.) /* cheesy version of hairline */
		return 1.;
	
	width *= gcairo->scale;
	if (width <= 1.)
		return width;

	return floor (width);
}

/* FIXME get rid of libart stuff */
static void
go_cairo_update_dash (GOCairo *gcairo)
{
	double size;
	
	go_line_vpath_dash_free (gcairo->line_dash);
	gcairo->line_dash = NULL;
	go_line_vpath_dash_free (gcairo->outline_dash);
	gcairo->outline_dash = NULL;
	
	if (gcairo->current_style == NULL)
		return;

	size = go_cairo_line_size (gcairo, gcairo->current_style->line.width);
	gcairo->line_dash = go_line_get_vpath_dash (gcairo->current_style->line.dash_type, size);
	size = go_cairo_line_size (gcairo, gcairo->current_style->outline.width);
	gcairo->outline_dash = go_line_get_vpath_dash (gcairo->current_style->outline.dash_type, size);
}

void
go_cairo_push_style (GOCairo *gcairo, GogStyle const *style)
{
	g_return_if_fail (IS_GO_CAIRO (gcairo));
	g_return_if_fail (IS_GOG_STYLE (style));
	
	if (gcairo->current_style != NULL)
		gcairo->style_stack = g_slist_prepend (
			gcairo->style_stack, (gpointer)gcairo->current_style);
	g_object_ref ((gpointer)style);
	gcairo->current_style = style;

	go_cairo_update_dash (gcairo);
}

void
go_cairo_pop_style (GOCairo *gcairo)
{
	g_return_if_fail (IS_GO_CAIRO (gcairo));
	g_return_if_fail (gcairo->current_style != NULL);

	g_object_unref ((gpointer)gcairo->current_style);
	if (gcairo->style_stack != NULL) {
		gcairo->current_style = gcairo->style_stack->data;
		gcairo->style_stack = g_slist_remove (gcairo->style_stack,
			gcairo->current_style);
	} else
		gcairo->current_style = NULL;

	go_cairo_update_dash (gcairo);
}

void
go_cairo_push_clip (GOCairo *gcairo, GOPath const *path)
{
	g_return_if_fail (IS_GO_CAIRO (gcairo));

	cairo_save (gcairo->cairo);
	go_cairo_set_path (gcairo, path, path->sharp ? GO_PATH_SNAP_ROUND : GO_PATH_SNAP_NONE);
	cairo_clip (gcairo->cairo);
}

void
go_cairo_pop_clip (GOCairo *gcairo)
{
	g_return_if_fail (IS_GO_CAIRO (gcairo));

	cairo_restore (gcairo->cairo);
}

static void
go_cairo_stroke_full (GOCairo *gcairo, GOPath const *path, gboolean is_outline)
{
	GogStyle const *style = gcairo->current_style;
	GOPathSnapType snap_type;
	GOLineDashType dash_type = is_outline ? style->outline.dash_type : style->line.dash_type;
	double line_width;
	
	g_return_if_fail (IS_GO_CAIRO (gcairo));

	if (dash_type == GO_LINE_NONE)
		return;
	
	line_width = go_cairo_line_size (gcairo, is_outline ? style->outline.width : style->line.width);
	
	if (path->sharp)
		snap_type = ((((int) (rint (line_width)) % 2 == 0) && line_width > 1.0) ?
			     GO_PATH_SNAP_ROUND :
			     GO_PATH_SNAP_HALF);
	else
		snap_type = GO_PATH_SNAP_NONE;

	cairo_set_line_width (gcairo->cairo, line_width);
	if (is_outline) {
		if (gcairo->outline_dash != NULL)
			cairo_set_dash (gcairo->cairo, 
					gcairo->outline_dash->dash, 
					gcairo->outline_dash->n_dash, 
					gcairo->outline_dash->offset);
		else
			cairo_set_dash (gcairo->cairo, NULL, 0, 0);
	} else {
		if (gcairo->line_dash != NULL)
			cairo_set_dash (gcairo->cairo, 
					gcairo->line_dash->dash, 
					gcairo->line_dash->n_dash, 
					gcairo->line_dash->offset);
		else
			cairo_set_dash (gcairo->cairo, NULL, 0, 0);
	}
	go_cairo_set_path (gcairo, path, snap_type);
	cairo_set_source_rgba (gcairo->cairo, 
			       GO_COLOR_TO_CAIRO (is_outline ? style->outline.color : style->line.color));
	cairo_stroke (gcairo->cairo);
}

void
go_cairo_stroke (GOCairo *gcairo, GOPath const *path) 
{
	go_cairo_stroke_full (gcairo, path, FALSE);
}

void
go_cairo_fill (GOCairo *gcairo, GOPath const *path) 
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
	
	GogStyle const *style = gcairo->current_style;
	cairo_t *cr = gcairo->cairo;
	cairo_pattern_t *cr_pattern = NULL;
	cairo_surface_t *cr_surface = NULL;
	cairo_matrix_t cr_matrix;
	GOImage *image = NULL;
	GOColor color;
	double width = go_cairo_line_size (gcairo, style->outline.width);
	double x[3], y[3];
	int i, j, w, h;
	guint8 const *pattern;
	unsigned char *iter;

	g_return_if_fail (IS_GO_CAIRO (gcairo));
	g_return_if_fail (IS_GO_PATH (path));

	if (style->fill.type == GOG_FILL_STYLE_NONE)
		return;

	cairo_set_line_width (cr, width);
	go_cairo_set_path (gcairo, path, GO_PATH_SNAP_ROUND);

	switch (style->fill.type) {
		case GOG_FILL_STYLE_PATTERN:
			if (go_pattern_is_solid (&style->fill.pattern, &color))
				cairo_set_source_rgba (cr, GO_COLOR_TO_CAIRO (color)); 
			else {
				GOColor fore = style->fill.pattern.fore;
				GOColor back = style->fill.pattern.back;
				int rowstride;

				pattern = go_pattern_get_pattern (&style->fill.pattern);
				image = g_object_new (GO_IMAGE_TYPE, "width", 8, "height", 8, NULL);
				iter = go_image_get_pixels (image);
				rowstride = go_image_get_rowstride (image);
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

		case GOG_FILL_STYLE_NONE:
			break; /* impossible */
	}
	cairo_fill (cr);
	
	if (cr_pattern != NULL)
		cairo_pattern_destroy (cr_pattern);
	if (cr_surface != NULL)
		cairo_surface_destroy (cr_surface);
	if (image)
		g_object_unref (image);
}

void 
go_cairo_draw_shape (GOCairo *gcairo, GOPath const *path)
{
	go_cairo_fill (gcairo, path);
	go_cairo_stroke_full (gcairo, path, TRUE);
}

/* FIXME get rid of libart stuff */
static void
cairo_emit_libart_path (cairo_t *cr, ArtVpath *vpath, ArtBpath *bpath)
{
	if (vpath) 
		while (vpath->code != ART_END) {
			switch (vpath->code) {
				case ART_MOVETO_OPEN:
				case ART_MOVETO:
					cairo_move_to (cr, vpath->x, vpath->y);
					break;
				case ART_LINETO:
					cairo_line_to (cr, vpath->x, vpath->y);
					break;
				default:
					break;
			}
			vpath++;
		}
	else
		while (bpath->code != ART_END) {
			switch (bpath->code) {
				case ART_MOVETO_OPEN:
				case ART_MOVETO:
					cairo_move_to (cr, bpath->x3, bpath->y3);
					break;
				case ART_LINETO:
					cairo_line_to (cr, bpath->x3, bpath->y3);
					break;
				case ART_CURVETO:
					cairo_curve_to (cr, 
						       bpath->x1, bpath->y1,
						       bpath->x2, bpath->y2,
						       bpath->x3, bpath->y3);
					break;
				default:
					break;
			}
			bpath++;
		}
}

static cairo_surface_t *
go_cairo_get_marker_surface (GOCairo *gcairo, double *x_offset, double *y_offset)
{
	GOMarker *marker = gcairo->current_style->marker.mark;
	cairo_t *cairo;
	cairo_surface_t *surface;
	ArtVpath *path;
	ArtVpath const *outline_path_raw, *fill_path_raw;
	double scaling[6], translation[6], affine[6];
	double half_size, offset;

	go_marker_get_paths (marker, &outline_path_raw, &fill_path_raw);

	if ((outline_path_raw == NULL) ||
	    (fill_path_raw == NULL))
		return NULL;

	if (gcairo->is_vector) {
		half_size = gcairo->scale * go_marker_get_size (marker) / 2.0;
		offset = half_size + gcairo->scale * go_marker_get_outline_width (marker);
	} else {
		half_size = rint (gcairo->scale * go_marker_get_size (marker)) / 2.0;
		offset = ceil (gcairo->scale * go_marker_get_outline_width (marker) / 2.0) + 
			half_size + .5;
	}
	surface = cairo_surface_create_similar (cairo_get_target (gcairo->cairo),
						CAIRO_CONTENT_COLOR_ALPHA,
						2.0 * offset, 
						2.0 * offset);
	cairo = cairo_create (surface);

	cairo_set_line_cap (cairo, CAIRO_LINE_CAP_SQUARE);
	cairo_set_line_join (cairo, CAIRO_LINE_JOIN_MITER);
						       
	art_affine_scale (scaling, half_size, half_size);
	art_affine_translate (translation, offset, offset);
	art_affine_multiply (affine, scaling, translation);

	path = art_vpath_affine_transform (fill_path_raw, affine);
	cairo_set_source_rgba (cairo, GO_COLOR_TO_CAIRO (go_marker_get_fill_color (marker)));
	cairo_emit_libart_path (cairo, path, NULL);
	cairo_fill (cairo);
	art_free (path);

	path = art_vpath_affine_transform (outline_path_raw, affine);
	cairo_set_source_rgba (cairo, GO_COLOR_TO_CAIRO (go_marker_get_outline_color (marker)));
	cairo_set_line_width (cairo, gcairo->scale * go_marker_get_outline_width (marker));
	cairo_emit_libart_path (cairo, path, NULL);
	cairo_stroke (cairo);
	art_free (path);

	cairo_destroy (cairo);

	*x_offset = *y_offset = offset;

	return surface;
}

void
go_cairo_draw_markers (GOCairo *gcairo, GOPath const *path)
{
	cairo_surface_t *surface;
	GOPathDataBuffer *buffer;
	GOPathData *data;
	double x_offset, y_offset;
	int i;

	surface = go_cairo_get_marker_surface (gcairo, &x_offset, &y_offset);
	if (surface == NULL)
		return;

	buffer = path->data_buffer_head;
	while (buffer != NULL) {
		for (i = 0; i < buffer->num_data; i++) {
			data = &buffer->data[i];
			if ((data->action & GO_PATH_FLAG_MARKER) != 0) {
				if (gcairo->is_vector)
					cairo_set_source_surface (gcairo->cairo, surface, 
								  data->point.x - x_offset, 
								  data->point.y - y_offset);
				else 
					cairo_set_source_surface (gcairo->cairo, surface, 
								  floor (data->point.x - x_offset), 
								  floor (data->point.y - y_offset));
				cairo_paint (gcairo->cairo);
			}
		}
		buffer = buffer->next;
	}
	cairo_surface_destroy (surface);
}

void
go_cairo_draw_text (GOCairo *gcairo, double x, double y, char const *text, GtkAnchorType anchor)
{
	GogStyle const *style = gcairo->current_style;
	PangoLayout *layout;
	PangoContext *context;
	cairo_t *cairo = gcairo->cairo;
	GOGeometryOBR obr;
	GOGeometryAABR aabr;
	int iw, ih;

	layout = pango_cairo_create_layout (cairo);
	context = pango_layout_get_context (layout);
	pango_cairo_context_set_resolution (context, 72 * gcairo->scale);
	pango_layout_set_markup (layout, text, -1);
	pango_layout_set_font_description (layout, style->font.font->desc);
	pango_layout_get_size (layout, &iw, &ih);

	obr.w = iw / (double)PANGO_SCALE;
	obr.h = ih / (double)PANGO_SCALE;
	obr.alpha = style->text_layout.angle * M_PI / 180.0;
	obr.x = x;
	obr.y = y;
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
	cairo_move_to (cairo, obr.x - (obr.w / 2.0) * cos (obr.alpha) - (obr.h / 2.0) * sin (obr.alpha),
		       obr.y + (obr.w / 2.0) * sin (obr.alpha) - (obr.h / 2.0) * cos (obr.alpha));
	cairo_rotate (cairo, -obr.alpha);
	pango_cairo_show_layout (cairo, layout);
	cairo_restore (cairo);
	g_object_unref (layout);
}

void
go_cairo_get_text_OBR (GOCairo *gcairo, char const *text, GOGeometryOBR *obr)
{
	GogStyle const *style = gcairo->current_style;
	PangoLayout *layout;
	PangoContext *context;
	PangoRectangle logical;
	cairo_t *cairo = gcairo->cairo;

	g_return_if_fail (IS_GO_CAIRO (gcairo));

	layout = pango_cairo_create_layout (cairo);
	context = pango_layout_get_context (layout);
	pango_cairo_context_set_resolution (context, 72 * gcairo->scale);
	pango_layout_set_markup (layout, text, -1);
	pango_layout_set_font_description (layout, style->font.font->desc);
	pango_layout_get_extents (layout, NULL, &logical);
	g_object_unref (layout);
	
	obr->w = ((double) logical.width + (double) PANGO_SCALE / 2.0) / (double) PANGO_SCALE;
	obr->h = ((double) logical.height + (double) PANGO_SCALE / 2.0) /(double) PANGO_SCALE;
}

void
go_cairo_get_text_AABR (GOCairo *gcairo, char const *text, GOGeometryAABR *aabr)
{
	GOGeometryOBR obr;

	go_cairo_get_text_OBR (gcairo, text, &obr);
	go_geometry_OBR_to_AABR (&obr, aabr);
}
