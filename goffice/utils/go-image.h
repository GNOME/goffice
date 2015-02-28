/*
 * go-image.h - Image formats
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) version 3.
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

#include <goffice/goffice.h>
#include <gsf/gsf-libxml.h>
#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

typedef enum {
	GO_IMAGE_FORMAT_SVG,
	GO_IMAGE_FORMAT_PNG,
	GO_IMAGE_FORMAT_JPG,
	GO_IMAGE_FORMAT_PDF,
	GO_IMAGE_FORMAT_PS,
	GO_IMAGE_FORMAT_EMF,
	GO_IMAGE_FORMAT_WMF,
	GO_IMAGE_FORMAT_EPS,
	GO_IMAGE_FORMAT_UNKNOWN
} GOImageFormat;

typedef struct {
	GOImageFormat format;
	char *name;
	char *desc;
	char *ext;
	gboolean has_pixbuf_saver;
	gboolean is_dpi_useful;
	gboolean alpha_support;
} GOImageFormatInfo;

GType      go_image_format_info_get_type (void);
char 	  *go_mime_to_image_format      (char const *mime_type);
char 	  *go_image_format_to_mime      (char const *format);

GOImageFormatInfo const *go_image_get_format_info       	(GOImageFormat format);
GOImageFormat            go_image_get_format_from_name  	(char const *name);
GSList 			*go_image_get_formats_with_pixbuf_saver (void);

/******************
 * GOImage object *
 ******************/

#define GO_TYPE_IMAGE	(go_image_get_type ())
#define GO_IMAGE(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_IMAGE, GOImage))
#define GO_IS_IMAGE(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_IMAGE))

GType go_image_get_type (void);

struct _GOImage {
	GObject parent;
	guint8 *data;
	double width, height;
	GdkPixbuf *thumbnail;
	GdkPixbuf *pixbuf;
	char *name;
	gsize data_length;
};

typedef struct {
	GObjectClass parent_klass;

	GdkPixbuf *(*get_pixbuf) (GOImage *image);
	GdkPixbuf *(*get_scaled_pixbuf) (GOImage *image, int width, int height);
	void (*save) (GOImage *image, GsfXMLOut *output);
	void (*load_attr) (GOImage *image, xmlChar const *attr_name, xmlChar const *attr_value);
	void (*load_data) (GOImage *image, GsfXMLIn *xin);
	void (*draw) (GOImage *image, cairo_t *cr);
	gboolean (*differ) (GOImage *first, GOImage *second);
} GOImageClass;

GdkPixbuf const *go_image_get_thumbnail		(GOImage *image);
GdkPixbuf       *go_image_get_pixbuf		(GOImage *image);
GdkPixbuf       *go_image_get_scaled_pixbuf	(GOImage *image, int width, int height);
void		 go_image_draw			(GOImage *image, cairo_t *cr);

GOImage 	*go_image_new_from_file 	(char const *filename, GError **error);
GOImage 	*go_image_new_from_data 	(char const *type, guint8 const *data, gsize length, char **format, GError **error);
GOImage 	*go_image_new_for_format   	(char const *format);
GType		 go_image_type_for_format      	(char const *format);

guint8 const	*go_image_get_data 		(GOImage *image, gsize *length);

void		 go_image_set_name		(GOImage *image, char const *name);
char const	*go_image_get_name 		(GOImage const *image);

gboolean	 go_image_differ		(GOImage *first, GOImage *second);

void		 go_image_save			(GOImage *image, GsfXMLOut *output);
void		 go_image_load_attrs		(GOImage *image, GsfXMLIn *xin, xmlChar const **attrs);
void		 go_image_load_data		(GOImage *image, GsfXMLIn *xin);

double		 go_image_get_width		(GOImage const *image);
double		 go_image_get_height		(GOImage const *image);

void		 go_image_set_default_dpi       (double dpi_x, double dpi_y);
void		 go_image_get_default_dpi       (double *dpi_x, double *dpi_y);

GOImageFormatInfo const *go_image_get_info      (GOImage *image);

/* Protected */
void		 _go_image_changed		(GOImage *image, double width, double height);

G_END_DECLS

#endif /* GO_IMAGE_H */
