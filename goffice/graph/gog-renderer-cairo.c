/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-renderer-cairo.c :
 *
 * Copyright (C) 2005 Emmanuel Pacaud (emmanuel.pacaud@univ-poitiers.fr)
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

/* TODO:
 *
 * 	- take screen resolution into account for image and patterns
 * 	- cache pango_layout
 */

#include <goffice/goffice-config.h>
#include <goffice/graph/gog-graph-impl.h>
#include <goffice/graph/gog-renderer-cairo.h>
#include <goffice/graph/gog-renderer-impl.h>
#include <goffice/graph/gog-style.h>
#include <goffice/graph/gog-view.h>
#include <goffice/utils/go-color.h>
#include <goffice/utils/go-font.h>
#include <goffice/utils/go-marker.h>
#include <goffice/utils/go-units.h>
#include <goffice/utils/go-math.h>

#include <libart_lgpl/art_render_gradient.h>
#include <libart_lgpl/art_render_svp.h>
#include <libart_lgpl/art_render_mask.h>
#include <pango/pangoft2.h>

#include <gsf/gsf-impl-utils.h>
#include <gsf/gsf-output-stdio.h>

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
	RENDERER_CAIRO_PROP_0,
	RENDERER_CAIRO_PROP_CAIRO,
	RENDERER_CAIRO_PROP_IS_VECTOR,
};

static void grc_free_marker_data (GogRendererCairo *crend);

struct _GogRendererCairo {
	GogRenderer base;

	int 		 w, h;

	gboolean	 is_vector;

	cairo_t		*cairo;
	GdkPixbuf 	*pixbuf;

	cairo_surface_t *marker_surface;
	double		 marker_x_offset;
	double		 marker_y_offset;
};

typedef GogRendererClass GogRendererCairoClass;

static GObjectClass *parent_klass;

static void
gog_renderer_cairo_finalize (GObject *obj)
{
	GogRendererCairo *crend = GOG_RENDERER_CAIRO (obj);

	if (crend->cairo != NULL){
		cairo_destroy (crend->cairo);
		crend->cairo = NULL;
	}
	if (crend->pixbuf != NULL) {
		g_object_unref (crend->pixbuf);
		crend->pixbuf = NULL;
	}
	grc_free_marker_data (crend);
	
	(*parent_klass->finalize) (obj);
}

static void
gog_renderer_cairo_set_property (GObject *obj, guint param_id,
			   GValue const *value, GParamSpec *pspec)
{
	GogRendererCairo *crend = GOG_RENDERER_CAIRO (obj);

	switch (param_id) {
	case RENDERER_CAIRO_PROP_CAIRO:
		if (crend->cairo)
			cairo_destroy (crend->cairo);
		crend->cairo = g_value_get_pointer (value);
		if (crend->cairo)
			cairo_reference (crend->cairo);
		break;

	case RENDERER_CAIRO_PROP_IS_VECTOR:
		crend->is_vector = g_value_get_boolean (value);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
}

static void
gog_renderer_cairo_get_property (GObject *obj, guint param_id,
			   GValue *value, GParamSpec *pspec)
{
	GogRendererCairo *crend = GOG_RENDERER_CAIRO (obj);

	switch (param_id) {
	case RENDERER_CAIRO_PROP_CAIRO:
		g_value_set_pointer (value, crend->cairo);
		break;
	case RENDERER_CAIRO_PROP_IS_VECTOR:
		g_value_set_boolean (value, crend->is_vector);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
grc_free_marker_data (GogRendererCairo *crend)
{
	if (crend->marker_surface != NULL) {
		cairo_surface_destroy (crend->marker_surface);
		crend->marker_surface = NULL;
	}
}

static double
grc_line_size (GogRenderer const *rend, double width)
{
	GogRendererCairo *crend = GOG_RENDERER_CAIRO (rend);

	if (crend->is_vector)
		return go_sub_epsilon (width) <= 0. ?
			GOG_RENDERER_HAIR_LINE_WIDTH :
			width * rend->scale;

	if (go_sub_epsilon (width) <= 0.) /* cheesy version of hairline */
		return 1.;
	
	width *= rend->scale;
	if (width <= 1.)
		return width;

	return floor (width);
}

static void
grc_path (cairo_t *cr, ArtVpath *vpath, ArtBpath *bpath)
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

static void
grc_draw_path (GogRenderer *rend, ArtVpath const *vpath, ArtBpath const*bpath)
{
	GogRendererCairo *crend = GOG_RENDERER_CAIRO (rend);
	GogStyle const *style = rend->cur_style;
	cairo_t *cr = crend->cairo;
	double width = grc_line_size (rend, style->line.width);

	g_return_if_fail (bpath != NULL || vpath != NULL);

	if (style->line.dash_type == GO_LINE_NONE)
		return;

	cairo_set_line_width (cr, width);
	if (rend->line_dash != NULL)
		cairo_set_dash (cr, 
				rend->line_dash->dash, 
				rend->line_dash->n_dash, 
				rend->line_dash->offset);
	else
		cairo_set_dash (cr, NULL, 0, 0);
	grc_path (cr, (ArtVpath *) vpath, (ArtBpath *) bpath);
	cairo_set_source_rgba (cr, GO_COLOR_TO_CAIRO (style->line.color));
	cairo_stroke (cr);
}

static void
gog_renderer_cairo_push_clip (GogRenderer *rend, GogRendererClip *clip)
{
	GogRendererCairo *crend = GOG_RENDERER_CAIRO (rend);
	ArtVpath *path = clip->path;
	int i;
	gboolean is_rectangle;

	if (!crend->is_vector) {
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

	cairo_save (crend->cairo);
	if (is_rectangle) {
		double x = floor (path[0].x + 0.5);
		double y = floor (path[0].y + 0.5);
		cairo_rectangle (crend->cairo, x, y,
				 floor (path[1].x + 0.5) - x,
				 floor (path[2].y + 0.5) - y);
	} else {
		grc_path (crend->cairo, path, NULL);
	}
	cairo_clip (crend->cairo);
}

static void
gog_renderer_cairo_pop_clip (GogRenderer *rend, GogRendererClip *clip)
{
	GogRendererCairo *crend = GOG_RENDERER_CAIRO (rend);

	cairo_restore (crend->cairo);
}

static double
gog_renderer_cairo_line_size (GogRenderer const *rend, double width)
{
	GogRendererCairo *crend = GOG_RENDERER_CAIRO (rend);
	double size = grc_line_size (rend, width);

	if (!crend->is_vector && size < 1.0)
		return ceil (size);

	return size;
}

static void
gog_renderer_cairo_sharp_path (GogRenderer *rend, ArtVpath *path, double line_width) 
{
	GogRendererCairo *crend = GOG_RENDERER_CAIRO (rend);
	ArtVpath *iter = path;

	if (crend->is_vector)
		return;

	if (((int) (rint (line_width)) % 2 == 0) && line_width > 1.0) 
		while (iter->code != ART_END) {
			iter->x = floor (iter->x + .5);
			iter->y = floor (iter->y + .5);
			iter++;
		}
	else
		while (iter->code != ART_END) {
			iter->x = floor (iter->x) + .5;
			iter->y = floor (iter->y) + .5;
			iter++;
		}
}

static void
gog_renderer_cairo_draw_path (GogRenderer *rend, ArtVpath const *path)
{
	grc_draw_path (rend, path, NULL);
}
  
static void
gog_renderer_cairo_draw_bezier_path (GogRenderer *rend, ArtBpath const *path)
{
	grc_draw_path (rend, NULL, path);
}

static void
grc_draw_polygon (GogRenderer *rend, ArtVpath const *vpath, 
		  ArtBpath const *bpath, gboolean narrow)
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
	
	GogRendererCairo *crend = GOG_RENDERER_CAIRO (rend);
	GogStyle const *style = rend->cur_style;
	cairo_t *cr = crend->cairo;
	cairo_pattern_t *cr_pattern = NULL;
	cairo_surface_t *cr_surface = NULL;
	cairo_matrix_t cr_matrix;
	GdkPixbuf *pixbuf = NULL;
	GOImage *image = NULL;
	GOColor color;
	double width = grc_line_size (rend, style->outline.width);
	double x[3], y[3];
	int i, j, w, h;
	guint8 const *pattern;
	unsigned char *iter;

	g_return_if_fail (bpath != NULL || vpath != NULL);

	narrow = narrow || (style->outline.dash_type == GO_LINE_NONE);

	if (narrow && style->fill.type == GOG_FILL_STYLE_NONE)
		return;

	cairo_set_line_width (cr, width);
	grc_path (cr, (ArtVpath *) vpath, (ArtBpath *) bpath);

	switch (style->fill.type) {
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
			pixbuf = gdk_pixbuf_add_alpha (style->fill.image.image, FALSE, 0, 0, 0);
			image = go_image_new_from_pixbuf (pixbuf);
			cr_pattern = go_image_create_cairo_pattern (image);
			h = gdk_pixbuf_get_height (pixbuf);
			w = gdk_pixbuf_get_width (pixbuf);
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

	if (style->fill.type != GOG_FILL_STYLE_NONE) {
		if (!narrow) 
			cairo_fill_preserve (cr);
		else 
			cairo_fill (cr);
	} 

	if (!narrow) {
		cairo_set_source_rgba (cr, GO_COLOR_TO_CAIRO (style->outline.color));
		if (rend->outline_dash != NULL)
			cairo_set_dash (cr, 
					rend->outline_dash->dash, 
					rend->outline_dash->n_dash, 
					rend->outline_dash->offset);
		else
			cairo_set_dash (cr, NULL, 0, 0);
		cairo_stroke (cr);
	}

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
gog_renderer_cairo_draw_polygon (GogRenderer *rend, ArtVpath const *path, 
				 gboolean narrow)
{
	grc_draw_polygon (rend, path, NULL, narrow);
}

static void
gog_renderer_cairo_draw_bezier_polygon (GogRenderer *rend, ArtBpath const *path,
					 gboolean narrow)
{
	grc_draw_polygon (rend, NULL, path, narrow);
}

static void
gog_renderer_cairo_draw_text (GogRenderer *rend, char const *text,
			      GogViewAllocation const *pos, GtkAnchorType anchor,
			      GogViewAllocation *result)
{
	GogRendererCairo *crend = GOG_RENDERER_CAIRO (rend);
	GogStyle const *style = rend->cur_style;
	PangoLayout *layout;
	PangoContext *context;
	cairo_t *cairo = crend->cairo;
	GOGeometryOBR obr;
	GOGeometryAABR aabr;
	int iw, ih;

	layout = pango_cairo_create_layout (cairo);
	context = pango_layout_get_context (layout);
	pango_cairo_context_set_resolution (context, 72 * rend->scale);
	pango_layout_set_markup (layout, text, -1);
	pango_layout_set_font_description (layout, style->font.font->desc);
	pango_layout_get_size (layout, &iw, &ih);

	obr.w = iw / (double)PANGO_SCALE;
	obr.h = ih / (double)PANGO_SCALE;
	obr.alpha = style->text_layout.angle * M_PI / 180.0;
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
	cairo_move_to (cairo, obr.x - (obr.w / 2.0) * cos (obr.alpha) - (obr.h / 2.0) * sin (obr.alpha),
		       obr.y + (obr.w / 2.0) * sin (obr.alpha) - (obr.h / 2.0) * cos (obr.alpha));
	cairo_rotate (cairo, -obr.alpha);
	pango_cairo_show_layout (cairo, layout);
	cairo_restore (cairo);
	g_object_unref (layout);

	if (result != NULL) {
		result->x = aabr.x;
		result->y = aabr.y;
		result->w = aabr.w;
		result->h = aabr.h;
	}
}

static void
gog_renderer_cairo_get_text_OBR (GogRenderer *rend,
				 char const *text, GOGeometryOBR *obr)
{
	GogRendererCairo *crend = GOG_RENDERER_CAIRO (rend);
	GogStyle const *style = rend->cur_style;
	PangoLayout *layout;
	PangoContext *context;
	PangoRectangle logical;
	cairo_t *cairo = crend->cairo;

	layout = pango_cairo_create_layout (cairo);
	context = pango_layout_get_context (layout);
	pango_cairo_context_set_resolution (context, 72 * rend->scale);
	pango_layout_set_markup (layout, text, -1);
	pango_layout_set_font_description (layout, style->font.font->desc);
	pango_layout_get_extents (layout, NULL, &logical);
	g_object_unref (layout);
	
	obr->w = ((double) logical.width + (double) PANGO_SCALE / 2.0) / (double) PANGO_SCALE;
	obr->h = ((double) logical.height + (double) PANGO_SCALE / 2.0) /(double) PANGO_SCALE;
}

static cairo_surface_t *
grc_get_marker_surface (GogRenderer *rend)
{
	GogRendererCairo *crend = GOG_RENDERER_CAIRO (rend);
	GOMarker *marker = rend->cur_style->marker.mark;
	cairo_t *cairo;
	cairo_surface_t *surface;
	ArtVpath *path;
	ArtVpath const *outline_path_raw, *fill_path_raw;
	double scaling[6], translation[6], affine[6];
	double half_size, offset;

	if (crend->marker_surface != NULL)
		return crend->marker_surface;

	go_marker_get_paths (marker, &outline_path_raw, &fill_path_raw);

	if ((outline_path_raw == NULL) ||
	    (fill_path_raw == NULL))
		return NULL;

	if (crend->is_vector) {
		half_size = rend->scale * go_marker_get_size (marker) / 2.0;
		offset = half_size + rend->scale * go_marker_get_outline_width (marker);
	} else {
		half_size = rint (rend->scale * go_marker_get_size (marker)) / 2.0;
		offset = ceil (rend->scale * go_marker_get_outline_width (marker) / 2.0) + 
			half_size + .5;
	}
	surface = cairo_surface_create_similar (cairo_get_target (crend->cairo),
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
	grc_path (cairo, path, NULL);
	cairo_fill (cairo);
	art_free (path);

	path = art_vpath_affine_transform (outline_path_raw, affine);
	cairo_set_source_rgba (cairo, GO_COLOR_TO_CAIRO (go_marker_get_outline_color (marker)));
	cairo_set_line_width (cairo, rend->scale * go_marker_get_outline_width (marker));
	grc_path (cairo, path, NULL);
	cairo_stroke (cairo);
	art_free (path);

	cairo_destroy (cairo);

	crend->marker_surface = surface;
	crend->marker_x_offset = crend->marker_y_offset = offset;

	return surface;
}

static void
gog_renderer_cairo_draw_marker (GogRenderer *rend, double x, double y)
{
	GogRendererCairo *crend = GOG_RENDERER_CAIRO (rend);
	cairo_surface_t *surface;

	surface = grc_get_marker_surface (rend);

	if (surface == NULL)
		return;

	if (crend->is_vector)
		cairo_set_source_surface (crend->cairo, surface, 
					  x - crend->marker_x_offset, 
					  y - crend->marker_y_offset);
	else 
		cairo_set_source_surface (crend->cairo, surface, 
					  floor (x - crend->marker_x_offset), 
					  floor (y - crend->marker_y_offset));
	cairo_paint (crend->cairo);
}

static void
gog_renderer_cairo_push_style (GogRenderer *rend, GogStyle const *style)
{
	GogRendererCairo *crend = GOG_RENDERER_CAIRO (rend);
	
	grc_free_marker_data (crend);
}

static void
gog_renderer_cairo_pop_style (GogRenderer *rend)
{
	GogRendererCairo *crend = GOG_RENDERER_CAIRO (rend);
	
	grc_free_marker_data (crend);
}

static gboolean
grc_gsf_gdk_pixbuf_save (const gchar *buf,
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
grc_cairo_write_func (void *closure,
		  const unsigned char *data,
		  unsigned int length)
{
	gboolean result;
	GsfOutput *output = GSF_OUTPUT (closure);

	result = gsf_output_write (output, length, data);

	return result ? CAIRO_STATUS_SUCCESS : CAIRO_STATUS_WRITE_ERROR;
}

static gboolean
gog_renderer_cairo_export_image (GogRenderer *renderer, GOImageFormat format,
				 GsfOutput *output, double x_dpi, double y_dpi)
{
	GogRendererCairo *crend = GOG_RENDERER_CAIRO (renderer);
	GogViewAllocation allocation;
	GOImageFormatInfo const *format_info;
	cairo_surface_t *surface = NULL;
	cairo_status_t status;
	GdkPixbuf *pixbuf, *output_pixbuf;
	double width_in_pts, height_in_pts;
	gboolean result;

	gog_graph_get_size (renderer->model, &width_in_pts, &height_in_pts);

	switch (format) {
		case GO_IMAGE_FORMAT_PDF:
		case GO_IMAGE_FORMAT_PS:
		case GO_IMAGE_FORMAT_SVG:
			crend->base.scale = 1.0;
			switch (format) {
				case GO_IMAGE_FORMAT_PDF:
#ifdef CAIRO_HAS_PDF_SURFACE
					surface = cairo_pdf_surface_create_for_stream 
						(grc_cairo_write_func, 
						 output, width_in_pts, height_in_pts);
					break;
#else
					g_warning ("[GogRendererCairo::export_image] cairo PDF backend missing");
					return FALSE;
#endif
				case GO_IMAGE_FORMAT_PS:
#ifdef CAIRO_HAS_PS_SURFACE
					surface = cairo_ps_surface_create_for_stream 
						(grc_cairo_write_func, 
						 output, width_in_pts, height_in_pts);
					break;
#else
					g_warning ("[GogRendererCairo::export_image] cairo PS backend missing");
					return FALSE;
#endif
				case GO_IMAGE_FORMAT_SVG:
#ifdef CAIRO_HAS_SVG_SURFACE
					surface = cairo_svg_surface_create_for_stream 
						(grc_cairo_write_func, 
						 output, width_in_pts, height_in_pts);
					break;
#else
					g_warning ("[GogRendererCairo::export_image] cairo SVG backend missing");
					return FALSE;
#endif
				default:
					break;
			}
			crend->cairo = cairo_create (surface);
			cairo_surface_destroy (surface);
			cairo_set_line_join (crend->cairo, CAIRO_LINE_JOIN_ROUND);
			cairo_set_line_cap (crend->cairo, CAIRO_LINE_CAP_ROUND);
			crend->w = width_in_pts;
			crend->h = height_in_pts;
			crend->is_vector = TRUE;

			allocation.x = 0.;
			allocation.y = 0.;
			allocation.w = width_in_pts;
			allocation.h = height_in_pts;
			gog_view_size_allocate (crend->base.view, &allocation);
			gog_view_render	(crend->base.view, NULL);

			cairo_show_page (crend->cairo);
			status = cairo_status (crend->cairo);

			cairo_destroy (crend->cairo);
			crend->cairo = NULL;

			return status == CAIRO_STATUS_SUCCESS;
			break;
		default:
			format_info = go_image_get_format_info (format);
			if (!format_info->has_pixbuf_saver) {
				g_warning ("[GogRendererCairo:export_image] unsupported format");
				return FALSE;
			}

			gog_renderer_cairo_update (crend, 
						   width_in_pts * x_dpi / 72.0, 
						   height_in_pts * y_dpi / 72.0, 1.0);
			pixbuf = gog_renderer_cairo_get_pixbuf (crend);
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
							      grc_gsf_gdk_pixbuf_save,
							      output, format_info->name,
							      NULL, NULL);
			if (!format_info->alpha_support)
				g_object_unref (output_pixbuf);
			return result;
			break;
	}
	return FALSE;
}

static void
gog_renderer_cairo_class_init (GogRendererClass *rend_klass)
{
	GObjectClass *gobject_klass   = (GObjectClass *) rend_klass;

	parent_klass = g_type_class_peek_parent (rend_klass);
	gobject_klass->finalize		= gog_renderer_cairo_finalize;
	gobject_klass->get_property = gog_renderer_cairo_get_property;
	gobject_klass->set_property = gog_renderer_cairo_set_property;
	rend_klass->push_style		= gog_renderer_cairo_push_style;
	rend_klass->pop_style		= gog_renderer_cairo_pop_style;
	rend_klass->push_clip  		= gog_renderer_cairo_push_clip;
	rend_klass->pop_clip     	= gog_renderer_cairo_pop_clip;
	rend_klass->sharp_path		= gog_renderer_cairo_sharp_path;
	rend_klass->draw_path	  	= gog_renderer_cairo_draw_path;
	rend_klass->draw_polygon  	= gog_renderer_cairo_draw_polygon;
	rend_klass->draw_bezier_path 	= gog_renderer_cairo_draw_bezier_path;
	rend_klass->draw_bezier_polygon = gog_renderer_cairo_draw_bezier_polygon;
	rend_klass->draw_text	  	= gog_renderer_cairo_draw_text;
	rend_klass->draw_marker	  	= gog_renderer_cairo_draw_marker;
	rend_klass->get_text_OBR	= gog_renderer_cairo_get_text_OBR;
	rend_klass->line_size		= gog_renderer_cairo_line_size;
	rend_klass->export_image	= gog_renderer_cairo_export_image;

	g_object_class_install_property (gobject_klass, RENDERER_CAIRO_PROP_CAIRO,
		g_param_spec_pointer ("cairo", "cairo",
			"the cairo_t* this renderer uses",
			G_PARAM_READWRITE));
	g_object_class_install_property (gobject_klass, RENDERER_CAIRO_PROP_IS_VECTOR,
		g_param_spec_boolean ("is-vector", "is-vector",
			"tells if the cairo surface is vectorial or raster", FALSE,
			G_PARAM_READWRITE));
}

static void
gog_renderer_cairo_init (GogRendererCairo *crend)
{
	crend->cairo = NULL;
	crend->pixbuf = NULL;
	crend->marker_surface = NULL;
	crend->w = crend->h = 0;
	crend->is_vector = FALSE;
}

GSF_CLASS (GogRendererCairo, gog_renderer_cairo,
	   gog_renderer_cairo_class_init, gog_renderer_cairo_init,
	   GOG_RENDERER_TYPE)

/**
 * gog_renderer_cairo_update:
 * @crend: a #GogRendererCairo
 * @w:
 * @h:
 * @zoom:
 *
 * Returns TRUE if the size actually changed.
 **/
gboolean
gog_renderer_cairo_update (GogRendererCairo *crend, int w, int h, double zoom)
{
	GogGraph *graph;
	GogView *view;
	GogViewAllocation allocation;
	GOImage *image = NULL;
	gboolean redraw = TRUE;
	gboolean size_changed;
	gboolean create_cairo = crend->cairo == NULL;

	g_return_val_if_fail (IS_GOG_RENDERER_CAIRO (crend), FALSE);
	g_return_val_if_fail (IS_GOG_VIEW (crend->base.view), FALSE);

	size_changed = crend->w != w || crend->h != h;
	if (size_changed && create_cairo) {
		if (crend->pixbuf != NULL)
			g_object_unref (crend->pixbuf);
		crend->pixbuf = NULL;
		if (w == 0 || h == 0)
			return FALSE;
		crend->w = w;
		crend->h = h;
		crend->pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, crend->w, crend->h);
		if (crend->pixbuf == NULL) {
			crend->w = 0;
			crend->h = 0;
			g_warning ("GogRendererCairo::cairo_setup: chart is too large");
			return FALSE;
		}
	}
	if (w == 0 || h == 0)
		return FALSE;

	view = crend->base.view;
	graph = GOG_GRAPH (view->model);
	gog_graph_force_update (graph);
	
	allocation.x = allocation.y = 0.;
	allocation.w = w;
	allocation.h = h;

	if (create_cairo) {
		image = go_image_new_from_pixbuf (crend->pixbuf);
		crend->cairo = go_image_get_cairo (image);;
		crend->is_vector = FALSE;
	}

	if (size_changed) {
		crend->base.scale_x = w / graph->width;
		crend->base.scale_y = h / graph->height;
		crend->base.scale = MIN (crend->base.scale_x, crend->base.scale_y);
		crend->base.zoom  = zoom;

		/* make sure we dont try to queue an update while updating */
		crend->base.needs_update = TRUE;

		/* scale just changed need to recalculate sizes */
		gog_renderer_invalidate_size_requests (&crend->base);
		gog_view_size_allocate (view, &allocation);
	} else 
		if (w != view->allocation.w || h != view->allocation.h)
			gog_view_size_allocate (view, &allocation);
		else
			redraw = gog_view_update_sizes (view);

	redraw |= crend->base.needs_update;
	crend->base.needs_update = FALSE;

	if (redraw) {
		if (create_cairo) {
			/* clear if we created it, not otherwise */
			cairo_set_operator (crend->cairo, CAIRO_OPERATOR_CLEAR);
			cairo_paint (crend->cairo);
		}
		cairo_set_operator (crend->cairo, CAIRO_OPERATOR_OVER);

		cairo_set_line_join (crend->cairo, CAIRO_LINE_JOIN_ROUND);
		cairo_set_line_cap (crend->cairo, CAIRO_LINE_CAP_ROUND);

		gog_view_render	(view, NULL);

		if (create_cairo) {
			go_image_get_pixbuf (image);
			g_object_unref (image);
		}
	}

	if (create_cairo) {
		cairo_destroy (crend->cairo);
		crend->cairo = NULL;
	}
	return redraw;
}

GdkPixbuf *
gog_renderer_cairo_get_pixbuf (GogRendererCairo *crend)
{
	g_return_val_if_fail (crend != NULL, NULL);

	return crend->pixbuf;
}
