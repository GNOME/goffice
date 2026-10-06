/*
 * go-svg.c - SVG image support
 *
 * Copyright (C) 2011 Jean Brefort (jean.brefort@normalesup.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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
#include "go-svg.h"

#include <librsvg/rsvg.h>

#include <gsf/gsf-utils.h>
#include <gsf/gsf-impl-utils.h>
#include <gsf/gsf-input-stdio.h>
#include <string.h>
#include <math.h>

struct _GOSvg {
	GOImage parent;
	RsvgHandle *handle;
};

typedef GOImageClass GOSvgClass;

static GObjectClass *parent_klass;

/*
 * _go_svg_get_size:
 * @handle: an #RsvgHandle
 * @width: (out): location for the width, in pixels
 * @height: (out): location for the height, in pixels
 *
 * Gets the intrinsic size of @handle, rounded up to whole pixels.  If the
 * document has no usable width and height the viewBox is used, and failing
 * that the size is 0 by 0.  The DPI of @handle should be set beforehand.
 */
void
_go_svg_get_size (RsvgHandle *handle, double *width, double *height)
{
	gboolean has_width, has_height, has_viewbox;
	RsvgLength w, h;
	RsvgRectangle viewbox;

	if (rsvg_handle_get_intrinsic_size_in_pixels (handle, width, height)) {
		*width = ceil (*width);
		*height = ceil (*height);
		return;
	}

	/* Width and/or height are missing or relative; use the viewBox. */
	rsvg_handle_get_intrinsic_dimensions (handle,
					      &has_width, &w,
					      &has_height, &h,
					      &has_viewbox, &viewbox);
	if (has_viewbox) {
		*width = ceil (viewbox.width);
		*height = ceil (viewbox.height);
	} else {
		*width = *height = 0.;
	}
}

/* Render @handle at its intrinsic size, @width by @height. */
static void
go_svg_render (RsvgHandle *handle, cairo_t *cr, double width, double height)
{
	RsvgRectangle viewport = { 0., 0., width, height };
	rsvg_handle_render_document (handle, cr, &viewport, NULL);
}

static void
go_svg_save (GOImage *image, GsfXMLOut *output)
{
	GOSvg *svg = GO_SVG (image);
	g_return_if_fail (svg);
	gsf_xml_out_add_base64 (output, NULL,
			image->data, image->data_length);
}

static void
go_svg_load_attr (G_GNUC_UNUSED GOImage *image, G_GNUC_UNUSED xmlChar const *attr_name, G_GNUC_UNUSED xmlChar const *attr_value)
{
	/* nothing to do */
}

static void
go_svg_load_data (GOImage *image, GsfXMLIn *xin)
{
	GOSvg *svg = GO_SVG (image);
	double dpi_x, dpi_y;
	image->data_length = gsf_base64_decode_simple (xin->content->str, strlen(xin->content->str));
	image->data = go_memdup (xin->content->str, image->data_length);
	svg->handle = rsvg_handle_new_from_data (image->data, image->data_length, NULL);
	go_image_get_default_dpi (&dpi_x, &dpi_y);
	rsvg_handle_set_dpi_x_y (svg->handle, dpi_x, dpi_y);
}

static void
go_svg_draw (GOImage *image, cairo_t *cr)
{
	GOSvg *svg = GO_SVG (image);
	go_svg_render (svg->handle, cr, image->width, image->height);
}

static GdkPixbuf *
go_svg_get_pixbuf (GOImage *image)
{
	GOSvg *svg = GO_SVG (image);
	cairo_surface_t *surface;
	cairo_t *cr;
	GdkPixbuf *res = NULL;
	g_return_val_if_fail (svg != NULL, NULL);
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, image->width, image->height);
	cr = cairo_create (surface);
	go_svg_render (svg->handle, cr, image->width, image->height);
	cairo_destroy (cr);
	res = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, image->width, image->height);
	go_cairo_convert_data_to_pixbuf (gdk_pixbuf_get_pixels (res),
	                                   cairo_image_surface_get_data (surface),
	                                   image->width, image->height,
	                                   cairo_image_surface_get_stride (surface));
	cairo_surface_destroy (surface);
	return res;
}

static GdkPixbuf *
go_svg_get_scaled_pixbuf (GOImage *image, int width, int height)
{
	GOSvg *svg = GO_SVG (image);
	cairo_surface_t *surface;
	cairo_t *cr;
	GdkPixbuf *res = NULL;
	g_return_val_if_fail (svg != NULL, NULL);
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
	cr = cairo_create (surface);
	cairo_scale (cr, width / image->width, height / image->height);
	go_svg_render (svg->handle, cr, image->width, image->height);
	cairo_destroy (cr);
	res = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, width, height);
	go_cairo_convert_data_to_pixbuf (gdk_pixbuf_get_pixels (res),
	                                   cairo_image_surface_get_data (surface),
	                                   width, height,
	                                   cairo_image_surface_get_stride (surface));
	cairo_surface_destroy (surface);
	return res;
}

static gboolean
go_svg_differ (GOImage *first, GOImage *second)
{
	if (first->data_length != second->data_length)
		return TRUE;
	return memcmp (first->data, second->data, first->data_length);
}

static void
go_svg_finalize (GObject *obj)
{
	GOSvg *svg = GO_SVG (obj);
	if (svg->handle)
		g_object_unref (svg->handle);
	(parent_klass->finalize) (obj);
}

static void
go_svg_class_init (GObjectClass *klass)
{
	GOImageClass *image_klass = (GOImageClass *) klass;

	klass->finalize = go_svg_finalize;
	parent_klass = g_type_class_peek_parent (klass);

	image_klass->save = go_svg_save;
	image_klass->load_attr = go_svg_load_attr;
	image_klass->load_data = go_svg_load_data;
	image_klass->get_pixbuf = go_svg_get_pixbuf;
	image_klass->get_scaled_pixbuf = go_svg_get_scaled_pixbuf;
	image_klass->draw = go_svg_draw;
	image_klass->differ = go_svg_differ;
}

GSF_CLASS (GOSvg, go_svg,
	   go_svg_class_init, NULL,
	   GO_TYPE_IMAGE)


/**
 * go_svg_new_from_file:
 * @filename: path to an SVG file
 * @error: (out) (optional): location to store error
 *
 * Creates a new SVG image from a file.
 *
 * Returns: (transfer full) (nullable): a new #GOImage or %NULL on error.
 **/
GOImage *
go_svg_new_from_file (char const *filename, GError **error)
{
	GOSvg *svg;
	guint8 *data;
	GsfInput *input = gsf_input_stdio_new (filename, error);
	GOImage *image;
	double dpi_x, dpi_y;

	if (!input)
		return NULL;
	svg = g_object_new (GO_TYPE_SVG, NULL);
	image = GO_IMAGE (svg);
	image->data_length = gsf_input_size (input);
	data = g_try_malloc (image->data_length);
	if (!data || !gsf_input_read (input, image->data_length, data)) {
		g_object_unref (svg);
		g_free (data);
		return NULL;
	}
	image->data = data;
	svg->handle = rsvg_handle_new_from_data (data, image->data_length, error);
	if (svg->handle == NULL) {
		g_object_unref (svg);
		return NULL;
	}
	go_image_get_default_dpi (&dpi_x, &dpi_y);
	rsvg_handle_set_dpi_x_y (svg->handle, dpi_x, dpi_y);
	_go_svg_get_size (svg->handle, &image->width, &image->height);
	return image;
}

/**
 * go_svg_new_from_data:
 * @data: SVG data
 * @length: length of @data in bytes
 * @error: (out) (optional): location to store error
 *
 * Creates a new SVG image from data in memory.
 *
 * Returns: (transfer full) (nullable): a new #GOImage or %NULL on error.
 **/
GOImage *
go_svg_new_from_data (char const *data, size_t length, GError **error)
{
	GOSvg *svg;
	GOImage *image;
	double dpi_x, dpi_y;

	g_return_val_if_fail (data != NULL && length != 0, NULL);
	svg = g_object_new (GO_TYPE_SVG, NULL);
	image = GO_IMAGE (svg);
	image->data_length = length;
	image->data = g_try_malloc (length);
	if (image->data == NULL) {
		g_object_unref (svg);
		return NULL;
	}
	memcpy (image->data, data, length);
	svg->handle = rsvg_handle_new_from_data (image->data, image->data_length, error);
	if (svg->handle == NULL) {
		g_object_unref (svg);
		return NULL;
	}
	go_image_get_default_dpi (&dpi_x, &dpi_y);
	rsvg_handle_set_dpi_x_y (svg->handle, dpi_x, dpi_y);
	_go_svg_get_size (svg->handle, &image->width, &image->height);
	return image;
}
