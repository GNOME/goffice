/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-image.c: Image formats
 *
 * Copyright (C) 2004, 2005 Jody Goldberg (jody@gnome.org)
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
#include <goffice/utils/go-image.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#ifdef GOFFICE_WITH_GTK
#include <gdk/gdkpixbuf.h>
#endif

/**
 * go_mime_to_image_format:
 * @mime_type: a mime type string
 *
 * returns: file extension for the given mime type.
 **/

char *
go_mime_to_image_format (char const *mime_type)
{
 	guint i;
	const char *suffix;
	const char* exceptions[] = {
		"svg+xml", "svg",
		"x-wmf", "wmf",
		"x-emf", "emf",
	};

	if (strncmp (mime_type, "image/", 6) != 0)
		return NULL;
	suffix = mime_type + 6;
	for (i = 0; i < G_N_ELEMENTS (exceptions); i +=2)
		if (strcmp (suffix, exceptions[i]) == 0)
			return g_strdup (exceptions[i+1]);

	return g_strdup (suffix);
}

/**
 * go_image_format_to_mime:
 * @format: a file extension string
 *
 * returns: corresponding mime type.
 **/

char *
go_image_format_to_mime (char const *format)
{
	char *ret = NULL;
 	guint i;
#ifdef GOFFICE_WITH_GTK
	GSList *ptr, *pixbuf_fmts;
	GdkPixbufFormat *pfmt;
	gchar *name;
	int cmp;
	gchar **mimes;
#endif
	const char* formats[] = {
		"svg", "image/svg,image/svg+xml",
		"wmf", "x-wmf",
		"emf", "x-emf",
	};
	
	if (format == NULL)
		return NULL;

	for (i = 0; i < G_N_ELEMENTS (formats); i +=2)
		if (strcmp (format, formats[i]) == 0)
			return g_strdup (formats[i+1]);

#ifdef GOFFICE_WITH_GTK
	/* Not a format we have special knowledge about - ask gdk-pixbuf */
	pixbuf_fmts = gdk_pixbuf_get_formats ();
	for (ptr = pixbuf_fmts; ptr != NULL; ptr = ptr->next) {
		pfmt = (GdkPixbufFormat *)ptr->data;
		name = gdk_pixbuf_format_get_name (pfmt);
		cmp = strcmp (format, name);
		g_free (name);
		if (cmp == 0) {
			mimes = gdk_pixbuf_format_get_mime_types (pfmt);
			ret = g_strjoinv (",", mimes);
			g_strfreev (mimes);
			break;
		}
	}
	g_slist_free (pixbuf_fmts);
#endif

	return ret;
}

static GOImageFormatInfo const image_format_infos[GO_IMAGE_FORMAT_UNKNOWN] = {
	{GO_IMAGE_FORMAT_SVG, (char *) "svg",  (char *) N_("SVG (vector graphics)"), 	 
		(char *) "svg", FALSE, FALSE},
	{GO_IMAGE_FORMAT_PNG, (char *) "png",  (char *) N_("PNG (raster graphics)"), 	 
		(char *) "png", TRUE,  TRUE},
	{GO_IMAGE_FORMAT_JPG, (char *) "jpeg", (char *) N_("JPEG (photograph)"),     	 
		(char *) "jpg", TRUE,  TRUE},
	{GO_IMAGE_FORMAT_PDF, (char *) "pdf",  (char *) N_("PDF (portable document format)"), 
		(char *) "pdf", FALSE, TRUE},
	{GO_IMAGE_FORMAT_PS,  (char *) "ps",   (char *) N_("PS (postscript)"), 		 
		(char *) "ps",  FALSE, TRUE},
	{GO_IMAGE_FORMAT_EMF, (char *) "emf",  (char *) N_("EMF (extended metafile)"),
		(char *) "emf", FALSE, FALSE},
	{GO_IMAGE_FORMAT_WMF, (char *) "wmf",  (char *) N_("WMF (windows metafile)"), 
		(char *) "wmf", FALSE, FALSE}
};

static GOImageFormatInfo *pixbuf_image_format_infos = NULL;
static unsigned pixbuf_format_nbr = 0;
static gboolean pixbuf_format_done = FALSE;

#define PIXBUF_IMAGE_FORMAT_OFFSET (1+GO_IMAGE_FORMAT_UNKNOWN)

static void
go_image_build_pixbuf_format_infos (void)
{
#ifdef GOFFICE_WITH_GTK
	GdkPixbufFormat *fmt;
	GSList *l, *pixbuf_fmts;
	GOImageFormatInfo *format_info;
	gchar **exts;
	unsigned i;

	if (pixbuf_format_done)
		return;
	
	pixbuf_fmts = gdk_pixbuf_get_formats ();
	pixbuf_format_nbr = g_slist_length (pixbuf_fmts);
	
	if (pixbuf_format_nbr > 0) {
		pixbuf_image_format_infos = g_new (GOImageFormatInfo, pixbuf_format_nbr);

		for (l = pixbuf_fmts, i = 1, format_info = pixbuf_image_format_infos; 
		     l != NULL; 
		     l = l->next, i++, format_info++) {
			fmt = (GdkPixbufFormat *)l->data;

			format_info->format = GO_IMAGE_FORMAT_UNKNOWN + i;
			format_info->name = gdk_pixbuf_format_get_name (fmt);
			format_info->desc = gdk_pixbuf_format_get_description (fmt);
			exts = gdk_pixbuf_format_get_extensions (fmt);
			format_info->ext = g_strdup (exts[0]);
			if (format_info->ext == NULL)
				format_info->ext = format_info->name;
			g_strfreev (exts);
			format_info->has_pixbuf_saver = gdk_pixbuf_format_is_writable (fmt);
			format_info->is_dpi_useful = FALSE;
		}
	}

	g_slist_free (pixbuf_fmts);
#endif /* GOFFICE_WITH_GTK */
	pixbuf_format_done = TRUE;
}

/**
 * go_image_get_format_info:
 * @format: a #GOImageFormat
 *
 * Retrieves infromation associated to @format.
 * 
 * returns: a #GOImageFormatInfo struct.
 **/

GOImageFormatInfo const *
go_image_get_format_info (GOImageFormat format)
{
	if (format > GO_IMAGE_FORMAT_UNKNOWN)
		go_image_build_pixbuf_format_infos ();
		
	g_return_val_if_fail (format >= 0 && 
			      format != GO_IMAGE_FORMAT_UNKNOWN &&
			      format <= GO_IMAGE_FORMAT_UNKNOWN + pixbuf_format_nbr, NULL);
	if (format < GO_IMAGE_FORMAT_UNKNOWN)	
		return &image_format_infos[format];

	return &pixbuf_image_format_infos[format - PIXBUF_IMAGE_FORMAT_OFFSET];
}

/**
 * go_image_get_format_from_name:
 * @name: a string
 *
 * returns: corresponding #GOImageFormat.
 **/

GOImageFormat	 
go_image_get_format_from_name (char const *name)
{
	unsigned i;

	go_image_build_pixbuf_format_infos ();
	
	for (i = 0; i < GO_IMAGE_FORMAT_UNKNOWN; i++) {
		if (strcmp (name, image_format_infos[i].name) == 0)
			return image_format_infos[i].format;
	}

	for (i = 0; i < pixbuf_format_nbr; i++) {
		if (strcmp (name, pixbuf_image_format_infos[i].name) == 0)
			return pixbuf_image_format_infos[i].format;
	}

	g_warning ("[GOImage::get_format_from_name] Unknown format name (%s)", name);
	return GO_IMAGE_FORMAT_UNKNOWN;
}

/**
 * go_image_get_formats_with_pixbuf_saver:
 *
 * returns: a list of #GOImageFormat that can be created from a pixbuf.
 **/

GSList *
go_image_get_formats_with_pixbuf_saver (void)
{
	GSList *list = NULL;
	unsigned i;

	for (i = 0; i < GO_IMAGE_FORMAT_UNKNOWN; i++) 
		if (image_format_infos[i].has_pixbuf_saver)
			list = g_slist_prepend (list, GUINT_TO_POINTER (i));

	/* TODO: before enabling this code, we must remove duplicate in pixbuf_image_format_infos */
#if 0	
	go_image_build_pixbuf_format_infos ();

	for (i = 0; i < pixbuf_format_nbr; i++) {
		if (pixbuf_image_format_infos[i].has_pixbuf_saver)
			list = g_slist_prepend (list, GUINT_TO_POINTER (i + PIXBUF_IMAGE_FORMAT_OFFSET));
	}
#endif

	return list;
}
