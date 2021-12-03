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
#else
#		include <gsf/gsf-input-textline.h>
#		include <gsf/gsf-input-memory.h>
#endif
#include <gsf/gsf-utils.h>
#include <gsf/gsf-input-stdio.h>
#include <gsf/gsf-impl-utils.h>
#include <string.h>
#include <unistd.h>

struct _GOSpectre {
	GOImage parent;
#ifdef GOFFICE_WITH_EPS
	SpectreDocument *doc;
#endif
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
			image->data, image->data_length);
}

static void
go_spectre_load_attr (G_GNUC_UNUSED GOImage *image, G_GNUC_UNUSED xmlChar const *attr_name, G_GNUC_UNUSED xmlChar const *attr_value)
{
	/* nothing to do */
}

static char *
create_file (const void *data, size_t length)
{
	int f = -1;
	FILE *F = NULL;
	char *tmpname;

	tmpname = g_strdup ("/tmp/epsXXXXXX.eps");
	if (!tmpname) goto error;

	f = g_mkstemp (tmpname);
	if (f == -1) goto error;

	F = fdopen (f, "w");
	if (F == NULL) goto error;

	if (fwrite (data, length, 1, F) != 1)
		goto error;
	if (fclose (F) != 0)
		goto error;

	return tmpname;

error:
	g_free (tmpname);
	if (F)
		fclose (F);
	else if (f != -1)
		close (f);

	return NULL;
}

static void
go_spectre_load_data (GOImage *image, GsfXMLIn *xin)
{
#ifdef GOFFICE_WITH_EPS
	GOSpectre *spectre = GO_SPECTRE (image);
	int width, height;
	char *tmpname;
#endif

	image->data_length = gsf_base64_decode_simple (xin->content->str, strlen(xin->content->str));
	image->data = go_memdup (xin->content->str, image->data_length);
#ifdef GOFFICE_WITH_EPS
	spectre->doc = spectre_document_new ();
	if (spectre->doc == NULL)
		return;
	/* libspectre can only load files,
	 see https://bugs.freedesktop.org/show_bug.cgi?id=42424 */
	tmpname = create_file (image->data, image->data_length);
	if (!tmpname)
		return;
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

static void
go_spectre_draw (GOImage *image, cairo_t *cr)
{
	GOSpectre *spectre = GO_SPECTRE (image);
	if (spectre->surface == NULL)
		go_spectre_build_surface (spectre);
	cairo_save (cr);
	cairo_set_source_surface (cr, spectre->surface, 0., 0.);
	cairo_rectangle (cr, 0., 0., image->width, image->height);
	cairo_fill (cr);
	cairo_restore (cr);
}

static GdkPixbuf *
go_spectre_get_pixbuf (GOImage *image)
{
	GdkPixbuf *res = NULL;
	GOSpectre *spectre = GO_SPECTRE (image);
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
	return res;
}

static GdkPixbuf *
go_spectre_get_scaled_pixbuf (GOImage *image, int width, int height)
{
	GdkPixbuf *res = NULL;
	GOSpectre *spectre = GO_SPECTRE (image);
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
	return res;
}
#endif

static gboolean
go_spectre_differ (GOImage *first, GOImage *second)
{
	if (first->data_length != second->data_length)
		return TRUE;
	return memcmp (first->data, second->data, first->data_length);
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
#ifdef GOFFICE_WITH_EPS
	image_klass->get_pixbuf = go_spectre_get_pixbuf;
	image_klass->get_scaled_pixbuf = go_spectre_get_scaled_pixbuf;
	image_klass->draw = go_spectre_draw;
#endif
	image_klass->differ = go_spectre_differ;
}

GSF_CLASS (GOSpectre, go_spectre,
	   go_spectre_class_init, NULL,
	   GO_TYPE_IMAGE)

GOImage *
go_spectre_new_from_file (char const *filename, GError **error)
{
	GOSpectre *spectre = g_object_new (GO_TYPE_SPECTRE, NULL);
	guint8 *data;
	GsfInput *input = gsf_input_stdio_new (filename, error);
#ifdef GOFFICE_WITH_EPS
	int width, height;
#endif
	GOImage *image;

	if (!input)
		return NULL;
	image = GO_IMAGE (spectre);
	image->data_length = gsf_input_size (input);
	data = g_try_malloc (image->data_length);
	if (!data || !gsf_input_read (input, image->data_length, data)) {
		g_object_unref (spectre);
		g_free (data);
		return NULL;
	}
	image->data = data;
#ifdef GOFFICE_WITH_EPS
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
#else
	{
		GsfInput *input = gsf_input_memory_new (image->data, image->data_length, FALSE);
		GsfInputTextline *text = GSF_INPUT_TEXTLINE (gsf_input_textline_new (input));
		guint8 *line;
		while ((line = gsf_input_textline_ascii_gets (text)))
			if (!strncmp (line, "%%BoundingBox: ", 15)) {
				unsigned x0, x1, y0, y1;
				if (sscanf (line + 15, "%u %u %u %u", &x0, &y0, &x1, &y1) == 4) {
					image->width = x1 - x0;
					image->height = y1 - y0;
				} else {
					image->width = 100;
					image->height = 100;
				}
				break;
			}
	       g_object_unref (text);
	       g_object_unref (input);
	}
#endif
	return image;
}

GOImage *
go_spectre_new_from_data (char const *data, size_t length, GError **error)
{
	GOImage *image;
	char *tmpname;

	g_return_val_if_fail (data != NULL && length != 0, NULL);

	/* libspectre can only load files,
	 see https://bugs.freedesktop.org/show_bug.cgi?id=42424 */
	tmpname = create_file (data, length);
	image = go_spectre_new_from_file (tmpname, error);
	unlink (tmpname);
	g_free (tmpname);
	return image;
}
