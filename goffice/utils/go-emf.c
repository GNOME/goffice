/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-emf.c - EMF/WMF image support
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
#include "go-emf.h"
#include <gsf/gsf-utils.h>
#include <gsf/gsf-impl-utils.h>
#include <gsf/gsf-input-memory.h>
#include <gsf/gsf-input-stdio.h>
#include <glib/gi18n-lib.h>
#include <string.h>

struct _GOEmf {
	GOImage parent;
	GocCanvas *canvas;
	gsize data_length;
};

typedef GOImageClass GOEmfClass;

static GObjectClass *parent_klass;

static gboolean go_emf_parse (GOEmf *emf, GsfInput *input, GError **error);

static void
go_emf_save (GOImage *image, GsfXMLOut *output)
{
	GOEmf *emf = GO_EMF (image);
	g_return_if_fail (emf);
	gsf_xml_out_add_base64 (output, NULL,
			image->data, emf->data_length);
}

static void
go_emf_load_attr (G_GNUC_UNUSED GOImage *image, G_GNUC_UNUSED xmlChar const *attr_name, G_GNUC_UNUSED xmlChar const *attr_value)
{
	/* nothing to do */
}

static void
go_emf_load_data (GOImage *image, GsfXMLIn *xin)
{
	GOEmf *emf = GO_EMF (image);
	emf->data_length = gsf_base64_decode_simple (xin->content->str, strlen(xin->content->str));
	image->data = g_malloc (emf->data_length);
	memcpy (image->data, xin->content->str, emf->data_length);
	/* FIXME: build the canvas */
}

static void
go_emf_draw (GOImage *image, cairo_t *cr)
{
	GOEmf *emf = GO_EMF (image);
	g_return_if_fail (emf && emf->canvas);
	goc_canvas_render (emf->canvas, cr, 0, 0, image->width, image->height);
}

static GdkPixbuf *
go_emf_get_pixbuf (GOImage *image)
{
	GOEmf *emf = GO_EMF (image);
	cairo_surface_t *surface;
	cairo_t *cr;
	GdkPixbuf *res = NULL;
	g_return_val_if_fail (emf, NULL);
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, image->width, image->height);
	cr = cairo_create (surface);
	goc_canvas_render (emf->canvas, cr, 0, 0, image->width, image->height);
	cairo_destroy (cr);
	res = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, image->width, image->height);
	go_cairo_convert_data_from_pixbuf (gdk_pixbuf_get_pixels (res),
	                                   cairo_image_surface_get_data (surface),
	                                   image->width, image->height,
	                                   cairo_image_surface_get_stride (surface));
	return res;
	return NULL; /* FIXME */
}

static GdkPixbuf *
go_emf_get_scaled_pixbuf (GOImage *image, int width, int height)
{
	GOEmf *emf = GO_EMF (image);
	cairo_surface_t *surface;
	cairo_t *cr;
	GdkPixbuf *res = NULL;
	g_return_val_if_fail (emf, NULL);
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
	cr = cairo_create (surface);
	cairo_scale (cr, width / image->width, height / image->height);
	goc_canvas_render (emf->canvas, cr, 0, 0, image->width, image->height);
	cairo_destroy (cr);
	res = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, width, height);
	go_cairo_convert_data_from_pixbuf (gdk_pixbuf_get_pixels (res),
	                                   cairo_image_surface_get_data (surface),
	                                   width, height,
	                                   cairo_image_surface_get_stride (surface));
	return res;
}

static gboolean
go_emf_differ (GOImage *first, GOImage *second)
{
	GOEmf *sfirst = GO_EMF (first), *ssecond = GO_EMF (second);
	if (sfirst->data_length != ssecond->data_length)
		return TRUE;
	return memcmp (first->data, second->data, sfirst->data_length);
}

static void
go_emf_finalize (GObject *obj)
{
	GOEmf *emf = GO_EMF (obj);
	if (emf->canvas != NULL)
		g_object_unref (emf->canvas);
	(parent_klass->finalize) (obj);
}

static void
go_emf_class_init (GObjectClass *klass)
{
	GOImageClass *image_klass = (GOImageClass *) klass;

	klass->finalize = go_emf_finalize;
	parent_klass = g_type_class_peek_parent (klass);

	image_klass->save = go_emf_save;
	image_klass->load_attr = go_emf_load_attr;
	image_klass->load_data = go_emf_load_data;
	image_klass->get_pixbuf = go_emf_get_pixbuf;
	image_klass->get_scaled_pixbuf = go_emf_get_scaled_pixbuf;
	image_klass->draw = go_emf_draw;
	image_klass->differ = go_emf_differ;
}

GSF_CLASS (GOEmf, go_emf,
	   go_emf_class_init, NULL,
	   GO_TYPE_IMAGE)

GOEmf *
go_emf_new_from_file (char const *filename, GError **error)
{
#ifdef GOFFICE_EMF_SUPPORT
	GOEmf *emf = NULL;
	GOImage *image;
	GsfInput *input = gsf_input_stdio_new (filename, error);
	guint8 *data;

	if (input == NULL)
		return NULL;
	data = g_malloc (gsf_input_size (input));
	if (!data || !gsf_input_read (input, emf->data_length, data)) {
		g_object_unref (emf);
		g_free (data);
		if (error)
			*error = g_error_new (go_error_invalid (), 0,
			                      _("Could not load the image data"));
		return NULL;
	}
	g_object_unref (input);
	emf = g_object_new (GO_TYPE_EMF, NULL);
	emf->data_length = gsf_input_size (input);
	g_object_unref (input);

	image = GO_IMAGE (emf);
	image->data = data;
	input = gsf_input_memory_new (data, emf->data_length, FALSE);
	if (!go_emf_parse (emf, input, error)) {
		g_object_unref (emf);
		emf = NULL;
	}
	g_object_unref (input);

	return emf;
#else
	return NULL;
#endif
}

GOEmf *
go_emf_new_from_data (char const *data, size_t length, GError **error)
{
#ifdef GOFFICE_EMF_SUPPORT
	GOEmf *emf = NULL;
	GsfInput *input = gsf_input_memory_new (data, length, FALSE);

	if (input == NULL) {
		if (error)
			*error = g_error_new (go_error_invalid (), 0,
			                      _("Could not input the image data"));
		return NULL;
	}
	emf = g_object_new (GO_TYPE_EMF, NULL);
	emf->data_length = gsf_input_size (input);
	if (!go_emf_parse (emf, input, error)) {
		g_object_unref (emf);
		emf = NULL;
	} else {
		GOImage *image = GO_IMAGE (emf);
		image->data = g_malloc (length);
		memcpy (image->data, data, length);
	}
	g_object_unref (input);
	return emf;
#else
	return NULL;
#endif
}

/******************************************************************************
 * Parsing code                                                               *
 * ****************************************************************************/

#ifdef GOFFICE_EMF_SUPPORT
static gboolean go_emf_parse (GOEmf *emf, GsfInput *input, GError **error)
{
	/* FIXME: implement */
	return FALSE;
}
#endif
