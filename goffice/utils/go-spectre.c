/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-spectre.h - PostScript image support
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
#include <goffice/goffice.h>
#ifdef GOFFICE_WITH_EPS
#       include <libspectre/spectre-document.h>
#       include <libspectre/spectre-render-context.h>
#endif
#include <gsf/gsf-utils.h>
#include <gsf/gsf-input-stdio.h>
#include <gsf/gsf-impl-utils.h>
#include <string.h>

struct _GOSpectre {
	GOImage parent;
#ifdef GOFFICE_WITH_EPS
	SpectreDocument *doc;
#endif
	gsize data_length;
	cairo_surface_t *surface;
};

typedef GOImageClass GOSpectreClass;

static GObjectClass *parent_klass;

static void
go_spectre_save (GOImage *image, GsfXMLOut *output)
{
	GOSpectre *spectre = GO_SPECTRE
		(image);
	g_return_if_fail (spectre);
	gsf_xml_out_add_base64 (output, NULL,
			image->data, spectre->data_length);
}

static void
go_spectre_load_attr (G_GNUC_UNUSED GOImage *image, G_GNUC_UNUSED xmlChar const *attr_name, G_GNUC_UNUSED xmlChar const *attr_value)
{
	/* nothing to do */
}

static void
go_spectre_load_data (GOImage *image, GsfXMLIn *xin)
{
	GOSpectre *spectre = GO_SPECTRE (image);
	int width, height;
	char *tmpname;
	int f;

	spectre->data_length = gsf_base64_decode_simple (xin->content->str, strlen(xin->content->str));
	image->data = g_malloc (spectre->data_length);
	memcpy (image->data, xin->content->str, spectre->data_length);
#ifdef GOFFICE_WITH_EPS
	spectre->doc = spectre_document_new ();
	if (spectre->doc == NULL)
		return;
	/* libspectre can only load files,
	 see https://bugs.freedesktop.org/show_bug.cgi?id=42424 */
	tmpname = g_strdup ("/tmp/epsXXXXXX.eps");
	f = g_mkstemp (tmpname);
	write (f, image->data, spectre->data_length);
	close (f);
	spectre_document_load (spectre->doc, tmpname);
	if (spectre_document_status (spectre->doc) != SPECTRE_STATUS_SUCCESS)
		return;
	spectre_document_get_page_size (spectre->doc, &width, &height);
	image->width = width;
	image->height = height;
	unlink (tmpname);
	g_free (tmpname);
#endif
}

#ifdef GOFFICE_WITH_EPS
static void
go_spectre_build_surface (GOSpectre *spectre)
{
	guint8 *data = NULL;
	int rowstride;
	static const cairo_user_data_key_t key;

	spectre_document_render  (spectre->doc, &data, &rowstride);
	if (!data)
		return;
	if (spectre_document_status (spectre->doc) != SPECTRE_STATUS_SUCCESS) {
		g_free (data);
		return;
	}

	spectre->surface = cairo_image_surface_create_for_data (data,
						       CAIRO_FORMAT_RGB24,
						       spectre->parent.width, spectre->parent.height,
						       rowstride);
	cairo_surface_set_user_data (spectre->surface, &key,
				     data, (cairo_destroy_func_t) g_free);
}
#endif

static void
go_spectre_draw (GOImage *image, cairo_t *cr)
{
	GOSpectre *spectre = GO_SPECTRE (image);
#ifdef GOFFICE_WITH_EPS
	if (spectre->surface == NULL)
		go_spectre_build_surface (spectre);
	cairo_save (cr);
	cairo_set_source_surface (cr, spectre->surface, 0., 0.);
	cairo_rectangle (cr, 0., 0., image->width, image->height);
	cairo_fill (cr);
	cairo_restore (cr);
#endif
}

static GdkPixbuf *
go_spectre_get_pixbuf (GOImage *image)
{
	GOSpectre *spectre = GO_SPECTRE (image);
	GdkPixbuf *res = NULL;
#ifdef GOFFICE_WITH_EPS
	cairo_surface_t *surface;
	cairo_t *cr;
	g_return_val_if_fail (spectre != NULL, NULL);
	if (spectre->surface == NULL)
		go_spectre_build_surface (spectre);
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, image->width, image->height);
	cr = cairo_create (surface);
	cairo_set_source_surface (cr, spectre->surface, 0., 0.);
	cairo_paint (cr);
	cairo_destroy (cr);
	res = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, image->width, image->height);
	go_cairo_convert_data_to_pixbuf (gdk_pixbuf_get_pixels (res),
	                                   cairo_image_surface_get_data (surface),
	                                   image->width, image->height,
	                                   cairo_image_surface_get_stride (surface));
	cairo_surface_destroy (surface);
#endif
	return res;
}

static GdkPixbuf *
go_spectre_get_scaled_pixbuf (GOImage *image, int width, int height)
{	GOSpectre *spectre = GO_SPECTRE (image);
	GdkPixbuf *res = NULL;
#ifdef GOFFICE_WITH_EPS
	cairo_surface_t *surface;
	cairo_t *cr;
	g_return_val_if_fail (spectre != NULL, NULL);
	if (spectre->surface == NULL)
		go_spectre_build_surface (spectre);
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
	cr = cairo_create (surface);
	cairo_scale (cr, width / image->width, height / image->height);
	cairo_set_source_surface (cr, spectre->surface, 0., 0.);
	cairo_paint (cr);
	res = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, image->width, image->height);
	cairo_destroy (cr);
	go_cairo_convert_data_to_pixbuf (gdk_pixbuf_get_pixels (res),
	                                   cairo_image_surface_get_data (surface),
	                                   width, height,
	                                   cairo_image_surface_get_stride (surface));
	cairo_surface_destroy (surface);
#endif
	return res;
}

static gboolean
go_spectre_differ (GOImage *first, GOImage *second)
{
	GOSpectre *sfirst = GO_SPECTRE (first), *ssecond = GO_SPECTRE (second);
	if (sfirst->data_length != ssecond->data_length)
		return TRUE;
	return memcmp (first->data, second->data, sfirst->data_length);
}

static void
go_spectre_finalize (GObject *obj)
{
	GOSpectre *spectre = GO_SPECTRE (obj);
	if (spectre->surface)
		cairo_surface_destroy (spectre->surface);
#ifdef GOFFICE_WITH_EPS
	if (spectre->doc)
		spectre_document_free (spectre->doc);
#endif
	(parent_klass->finalize) (obj);
}

static void
go_spectre_class_init (GObjectClass *klass)
{
	GOImageClass *image_klass = (GOImageClass *) klass;

	klass->finalize = go_spectre_finalize;
	parent_klass = g_type_class_peek_parent (klass);

	image_klass->save = go_spectre_save;
	image_klass->load_attr = go_spectre_load_attr;
	image_klass->load_data = go_spectre_load_data;
	image_klass->get_pixbuf = go_spectre_get_pixbuf;
	image_klass->get_scaled_pixbuf = go_spectre_get_scaled_pixbuf;
	image_klass->draw = go_spectre_draw;
	image_klass->differ = go_spectre_differ;
}

GSF_CLASS (GOSpectre, go_spectre,
	   go_spectre_class_init, NULL,
	   GO_TYPE_IMAGE)

GOImage *
go_spectre_new_from_file (char const *filename, GError **error)
{
#ifdef GOFFICE_WITH_EPS
	GOSpectre *spectre = g_object_new (GO_TYPE_SPECTRE, NULL);
	guint8 *data;
	GsfInput *input = gsf_input_stdio_new (filename, error);
	int width, height;
	GOImage *image;

	if (!input)
		return NULL;
	spectre->data_length = gsf_input_size (input);
	data = g_malloc (spectre->data_length);
	if (!data || !gsf_input_read (input, spectre->data_length, data)) {
		g_object_unref (spectre);
		g_free (data);
		return NULL;
	}
	image = GO_IMAGE (spectre);
	image->data = data;
	spectre->doc = spectre_document_new ();
	if (spectre->doc == NULL) {
		g_object_unref (spectre);
		return NULL;
	}
	spectre_document_load (spectre->doc, filename);
	if (spectre_document_status (spectre->doc) != SPECTRE_STATUS_SUCCESS) {
		g_object_unref (spectre);
		return NULL;
	}
	spectre_document_get_page_size (spectre->doc, &width, &height);
	image->width = width;
	image->height = height;
	return image;
#else
	return NULL;
#endif
}

GOImage *
go_spectre_new_from_data (char const *data, size_t length, GError **error)
{
#ifdef GOFFICE_WITH_EPS
	GOImage *image;
	char *tmpname;
	int f;

	g_return_val_if_fail (data != NULL && length != 0, NULL);

	/* libspectre can only load files,
	 see https://bugs.freedesktop.org/show_bug.cgi?id=42424 */
	tmpname = g_strdup ("/tmp/epsXXXXXX.eps");
	f = g_mkstemp (tmpname);
	write (f, data, length);
	close (f);
	image = go_spectre_new_from_file (tmpname, error);
	g_free (tmpname);
	return image;
#else
	return NULL;
#endif
}
