/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-image.h - Image formats
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License, version 2, as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA.
 */
#ifndef GO_IMAGE_H
#define GO_IMAGE_H

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
	GO_IMAGE_FORMAT_SVG,
	GO_IMAGE_FORMAT_PNG,
	GO_IMAGE_FORMAT_JPG,
	GO_IMAGE_FORMAT_PDF,
	GO_IMAGE_FORMAT_PS,
	GO_IMAGE_FORMAT_EMF,
	GO_IMAGE_FORMAT_WMF,
	GO_IMAGE_FORMAT_UNKNOWN
} GOImageFormat;

typedef struct {
	GOImageFormat format;
	char *name;
	char *desc;
	char *ext;
	gboolean has_pixbuf_saver;
	gboolean is_dpi_useful; 
} GOImageFormatInfo;

char 	  *go_mime_to_image_format      (char const *mime_type);
char 	  *go_image_format_to_mime      (char const *format);

GOImageFormatInfo const *go_image_get_format_info       	(GOImageFormat format);
GOImageFormat            go_image_get_format_from_name  	(char const *name);
GSList 			*go_image_get_formats_with_pixbuf_saver (void);

G_END_DECLS

#endif /* GO_IMAGE_H */
