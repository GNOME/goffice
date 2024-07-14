/*
 * go-image.c: Image formats
 *
 * Copyright (C) 2004, 2005 Jody Goldberg (jody@gnome.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) version 3.
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
#include <string.h>
#include <gsf/gsf-utils.h>
#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

/**
 * GOImageFormat:
 * @GO_IMAGE_FORMAT_SVG: SVG.
 * @GO_IMAGE_FORMAT_PNG: PNG.
 * @GO_IMAGE_FORMAT_JPG: JPEG.
 * @GO_IMAGE_FORMAT_PDF: PDF.
 * @GO_IMAGE_FORMAT_PS: PostScript.
 * @GO_IMAGE_FORMAT_EMF: Enhanced Metafile.
 * @GO_IMAGE_FORMAT_WMF: Windows Metafile.
 * @GO_IMAGE_FORMAT_EPS: Encapsulated Postscript.
 * @GO_IMAGE_FORMAT_UNKNOWN: unknown.
 **/

/**
 * GOImageFormatInfo:
 * @format: #GOImageFormat.
 * @name: image type name.
 * @desc: image type description.
 * @ext: file extension.
 * @has_pixbuf_saver: can be saved as pixbuf.
 * @is_dpi_useful: depends upon device resolution.
 * @alpha_support: supports transparency.
 **/

/**
 * GOImageClass:
 * @parent_klass: parent class.
 * @get_pixbuf: gets a pixbuf with the same size as the image.
 * @get_scaled_pixbuf: gets a scaled pixbuf respecting the original aspect ratio.
 * @save: saves the image.
 * @load_attr: loads the images properties.
 * @load_data: loads the image data.
 * @draw: draws the image.
 * @differ: returns %TRUE if the two images are different.
 **/

static GOImageFormatInfo *pixbuf_image_format_infos = NULL;
static GHashTable *pixbuf_mimes = NULL;
static unsigned pixbuf_format_nbr = 0;
static gboolean pixbuf_format_done = FALSE;

#define PIXBUF_IMAGE_FORMAT_OFFSET (1+GO_IMAGE_FORMAT_UNKNOWN)

static void go_image_build_pixbuf_format_infos (void);

/**
 * go_mime_to_image_format:
 * @mime_type: a mime type string
 *
 * Returns: (transfer full): file extension for the given mime type.
 **/
char *
go_mime_to_image_format (char const *mime_type)
{
 	guint i;
	const char* exceptions[] = {
		"image/svg", "svg",
		"image/svg+xml", "svg",
		"image/svg-xml", "svg",
		"image/vnd.adobe.svg+xml", "svg",
		"text/xml-svg", "svg",
		"image/wmf", "wmf",
		"image/emf", "emf",
		"application/pdf", "pdf",
		"application/postscript", "ps",
		"image/x-eps", "eps",
	};

	for (i = 0; i < G_N_ELEMENTS (exceptions); i += 2)
		if (strcmp (mime_type, exceptions[i]) == 0)
			return g_strdup (exceptions[i + 1]);

	go_image_build_pixbuf_format_infos ();

	return g_strdup (g_hash_table_lookup (pixbuf_mimes, mime_type));
}

/**
 * go_image_format_to_mime:
 * @format: a file extension string
 *
 * Returns: (transfer full) (nullable): corresponding mime type.
 **/
char *
go_image_format_to_mime (char const *format)
{
	char *ret = NULL;
 	guint i;
	GSList *ptr, *pixbuf_fmts;
	static const char* const formats[] = {
		"svg", "image/svg,image/svg+xml",
		"wmf", "image/x-wmf",
		"emf", "image/x-emf",
		"pdf", "application/pdf",
		"ps", "application/postscript",
		"eps", "image/x-eps",
	};

	if (format == NULL)
		return NULL;

	for (i = 0; i < G_N_ELEMENTS (formats); i += 2)
		if (strcmp (format, formats[i]) == 0)
			return g_strdup (formats[i + 1]);

	/* Not a format we have special knowledge about - ask gdk-pixbuf */
	pixbuf_fmts = gdk_pixbuf_get_formats ();
	for (ptr = pixbuf_fmts; ptr != NULL; ptr = ptr->next) {
		GdkPixbufFormat *pfmt = ptr->data;
		gchar *name = gdk_pixbuf_format_get_name (pfmt);
		int cmp = strcmp (format, name);
		g_free (name);
		if (cmp == 0) {
			gchar **mimes =
				gdk_pixbuf_format_get_mime_types (pfmt);
			ret = g_strjoinv (",", mimes);
			g_strfreev (mimes);
			break;
		}
	}
	g_slist_free (pixbuf_fmts);

	return ret;
}

static GOImageFormatInfo const image_format_infos[GO_IMAGE_FORMAT_UNKNOWN] = {
	{GO_IMAGE_FORMAT_SVG, (char *) "svg",  (char *) N_("SVG (vector graphics)"),
	 (char *) "svg", FALSE, FALSE, TRUE},
	{GO_IMAGE_FORMAT_PNG, (char *) "png",  (char *) N_("PNG (raster graphics)"),
	 (char *) "png", TRUE,  TRUE, TRUE},
	{GO_IMAGE_FORMAT_JPG, (char *) "jpeg", (char *) N_("JPEG (photograph)"),
	 (char *) "jpg", TRUE,  TRUE, FALSE},
	{GO_IMAGE_FORMAT_PDF, (char *) "pdf",  (char *) N_("PDF (portable document format)"),
	 (char *) "pdf", FALSE, FALSE, TRUE},
	{GO_IMAGE_FORMAT_PS,  (char *) "ps",   (char *) N_("PS (postscript)"),
	 (char *) "ps",  FALSE, TRUE, TRUE},
	{GO_IMAGE_FORMAT_EMF, (char *) "emf",  (char *) N_("EMF (extended metafile)"),
	 (char *) "emf", FALSE, FALSE, TRUE},
	{GO_IMAGE_FORMAT_WMF, (char *) "wmf",  (char *) N_("WMF (windows metafile)"),
	 (char *) "wmf", FALSE, FALSE, TRUE},
	{GO_IMAGE_FORMAT_EPS,  (char *) "eps",   (char *) N_("EPS (encapsulated postscript)"),
	 (char *) "eps",  FALSE, TRUE, TRUE},
};

/* Making GOFormatInfo as a boxed type, as they are static, ref and unref don't
 * need to do anything */
static GOImageFormatInfo *
go_image_format_info_ref (GOImageFormatInfo *info)
{
	return info;
}

static void
go_image_format_info_unref (GOImageFormatInfo const *info)
{
}

GType
go_image_format_info_get_type (void)
{
	static GType t = 0;

	if (t == 0)
		t = g_boxed_type_register_static ("GOImageFormatInfo",
			 (GBoxedCopyFunc)go_image_format_info_ref,
			 (GBoxedFreeFunc)go_image_format_info_unref);
	return t;
}

static void
go_image_build_pixbuf_format_infos (void)
{
	GdkPixbufFormat *fmt;
	GSList *l, *pixbuf_fmts;
	GOImageFormatInfo *format_info;
	gchar **exts, **mimes;
	unsigned i, j;

	if (pixbuf_format_done)
		return;

	pixbuf_fmts = gdk_pixbuf_get_formats ();
	pixbuf_format_nbr = g_slist_length (pixbuf_fmts);

	if (pixbuf_format_nbr > 0) {
		pixbuf_image_format_infos = g_new (GOImageFormatInfo, pixbuf_format_nbr);
		pixbuf_mimes = g_hash_table_new_full
			(g_str_hash, g_str_equal, g_free, g_free);

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
			format_info->alpha_support = FALSE;

			mimes = gdk_pixbuf_format_get_mime_types (fmt);
			for (j = 0; mimes[j]; j++) {
				g_hash_table_insert
					(pixbuf_mimes,
					 g_strdup((gpointer) mimes[j]),
					 g_strdup((gpointer) format_info->name));
			}
			g_strfreev (mimes);
		}
	}

	g_slist_free (pixbuf_fmts);
	pixbuf_format_done = TRUE;
}

/**
 * go_image_get_format_info:
 * @format: a #GOImageFormat
 *
 * Retrieves information associated to @format.
 *
 * Returns: (transfer none): a #GOImageFormatInfo struct.
 **/
GOImageFormatInfo const *
go_image_get_format_info (GOImageFormat format)
{
	if (format == GO_IMAGE_FORMAT_UNKNOWN)
		return NULL;
	if (format > GO_IMAGE_FORMAT_UNKNOWN)
		go_image_build_pixbuf_format_infos ();

	g_return_val_if_fail (format >= 0 &&
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

	if (name == NULL || strcmp (name, "unknown") == 0)
		return GO_IMAGE_FORMAT_UNKNOWN;

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
 * Returns: (element-type GOImageFormat) (transfer container): a list of #GOImageFormat that can be created
 * from a pixbuf.
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

/*********************************
 * GOImage object implementation *
 *********************************/

static GObjectClass *parent_klass;

enum {
	IMAGE_PROP_0,
	IMAGE_PROP_WIDTH,
	IMAGE_PROP_HEIGHT
};

static void
go_image_set_property (GObject *obj, guint param_id,
		       GValue const *value, GParamSpec *pspec)
{
	GOImage *image = GO_IMAGE (obj);
	gboolean size_changed = FALSE;
	guint n;

	switch (param_id) {
	case IMAGE_PROP_WIDTH:
		n = g_value_get_uint (value);
		if (n != image->width) {
			image->width = n;
			size_changed = TRUE;
		}
		break;
	case IMAGE_PROP_HEIGHT:
		n = g_value_get_uint (value);
		if (n != image->height) {
			image->height = n;
			size_changed = TRUE;
		}
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		return; /* NOTE : RETURN */
	}

	if (size_changed) {
		g_free (image->data);
		image->data = NULL;
	}
}

static void
go_image_get_property (GObject *obj, guint param_id,
		       GValue *value, GParamSpec *pspec)
{
	GOImage *image = GO_IMAGE (obj);

	switch (param_id) {
	case IMAGE_PROP_WIDTH:
		g_value_set_uint (value, image->width);
		break;
	case IMAGE_PROP_HEIGHT:
		g_value_set_uint (value, image->height);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		return; /* NOTE : RETURN */
	}
}

static void
go_image_finalize (GObject *obj)
{
	GOImage *image = GO_IMAGE (obj);
	g_free (image->data);
	if (image->thumbnail)
		g_object_unref (image->thumbnail);
	if (image->pixbuf)
		g_object_unref (image->pixbuf);
	g_free (image->name);
	(parent_klass->finalize) (obj);
}

/* default implementation for unsupported images */
static void
go_image_draw_fb (GOImage *image, cairo_t *cr)
{
#ifdef GOFFICE_WITH_GTK
	if (image->pixbuf) {
		cairo_rectangle (cr, 0., 0., image->width, image->height);
		gdk_cairo_set_source_pixbuf (cr, image->pixbuf, 0, 0);
		cairo_fill (cr);
	} else {
		GdkPixbuf *placeholder = gtk_icon_theme_load_icon
			(gtk_icon_theme_get_default (),
			 "unknown_image", 100, 0, NULL);
		double dx, dy;
		int n;
		if (placeholder == NULL)
			return;
		n = go_fake_floor (image->width / gdk_pixbuf_get_width (placeholder));
		dx = (image->width - n * gdk_pixbuf_get_width (placeholder)) / 2.;
		n = go_fake_floor (image->height / gdk_pixbuf_get_height (placeholder));
		dy = (image->height - n * gdk_pixbuf_get_height (placeholder)) / 2.;
		cairo_save (cr);
		cairo_rectangle (cr, 0., 0., image->width, image->height);
		cairo_clip (cr);
		cairo_rectangle (cr, -dx, -dy, image->width + dx, image->height + dy);
		gdk_cairo_set_source_pixbuf (cr, placeholder, 0, 0);
		cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_REPEAT);
		cairo_fill (cr);
		cairo_restore (cr);
		g_object_unref (placeholder);
	}
#endif
}

static GdkPixbuf *
go_image_get_pixbuf_fb (GOImage *image)
{
#ifdef GOFFICE_WITH_GTK
	cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
		                                                    image->width,
		                                                    image->height);
	cairo_t *cr = cairo_create (surface);
	GdkPixbuf *ret, *placeholder = gtk_icon_theme_load_icon
		(gtk_icon_theme_get_default (),
		 "unknown_image", 100, 0, NULL);
	double dx, dy;
	int n;
	if (placeholder == NULL)
		return NULL;
	n = go_fake_floor (image->width / gdk_pixbuf_get_width (placeholder));
	dx = (image->width - n * gdk_pixbuf_get_width (placeholder)) / 2.;
	n = go_fake_floor (image->height / gdk_pixbuf_get_height (placeholder));
	dy = (image->height - n * gdk_pixbuf_get_height (placeholder)) / 2.;
	cairo_rectangle (cr, 0., 0., image->width, image->height);
	cairo_clip (cr);
	cairo_rectangle (cr, -dx, -dy, image->width + dx, image->height + dy);
	gdk_cairo_set_source_pixbuf (cr, placeholder, 0, 0);
	cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_REPEAT);
	cairo_fill (cr);
	cairo_destroy (cr);
	ret = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, image->width, image->height);
	if (cairo_image_surface_get_stride (surface) != gdk_pixbuf_get_rowstride (ret)) {
		g_object_unref (ret);
		ret = NULL;
	} else
		go_cairo_convert_data_from_pixbuf (gdk_pixbuf_get_pixels (ret),
			                               cairo_image_surface_get_data (surface),
			                               image->width, image->height,
			                               gdk_pixbuf_get_rowstride (ret));
	cairo_surface_destroy (surface);
	return ret;
#else
	return NULL;
#endif
}

static GdkPixbuf *
go_image_get_scaled_pixbuf_fb (GOImage *image, int width, int height)
{
	if (image->pixbuf == NULL)
		image->pixbuf = go_image_get_pixbuf_fb (image);
	return gdk_pixbuf_scale_simple (image->pixbuf, width, height, GDK_INTERP_BILINEAR);
}

static void
go_image_class_init (GOImageClass *klass)
{
	GObjectClass *obj_klass = (GObjectClass *) klass;
	obj_klass->finalize = go_image_finalize;
	obj_klass->set_property = go_image_set_property;
	obj_klass->get_property = go_image_get_property;
	parent_klass = g_type_class_peek_parent (klass);
	g_object_class_install_property (obj_klass, IMAGE_PROP_WIDTH,
					 g_param_spec_uint ("width", _("Width"),
							    _("Image width in pixels"),
							    0, G_MAXUINT16, 0, G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, IMAGE_PROP_HEIGHT,
					 g_param_spec_uint ("height", _("Height"),
							    _("Image height in pixels"),
							    0, G_MAXUINT16, 0, G_PARAM_READWRITE));

	klass->draw = go_image_draw_fb;
	klass->get_pixbuf = go_image_get_pixbuf_fb;
	klass->get_scaled_pixbuf = go_image_get_scaled_pixbuf_fb;
}

GSF_CLASS_ABSTRACT (GOImage, go_image,
		go_image_class_init, NULL,
		G_TYPE_OBJECT)

void
go_image_draw (GOImage *image, cairo_t *cr)
{
	g_return_if_fail (GO_IS_IMAGE (image));
	((GOImageClass *) G_OBJECT_GET_CLASS (image))->draw (image, cr);
}

#define GO_THUMBNAIL_SIZE 64

/**
 * go_image_get_thumbnail:
 * @image: #GOImage
 *
 * Generates a thumbnail for @image if not already done and returns it, adding
 a reference. The pixbuf is scaled so that its width and height are not larger
 * than 64 pixels, and preserving the aspect ratio.
 * Returns: (transfer full): the thumbnail for @image
 **/
GdkPixbuf const *
go_image_get_thumbnail (GOImage *image)
{
	g_return_val_if_fail (GO_IS_IMAGE (image), NULL);
	if (image->thumbnail == NULL)
		image->thumbnail = go_image_get_scaled_pixbuf (image, GO_THUMBNAIL_SIZE, GO_THUMBNAIL_SIZE);
	return image->thumbnail;
}

/**
 * go_image_get_pixbuf:
 * @image: #GOImage
 *
 * Builds a pixbuf from the image if not already done and returns it, adding
 * a reference.
 * Returns: (transfer full): the pixbuf for @image
 **/
GdkPixbuf *
go_image_get_pixbuf (GOImage *image)
{
	g_return_val_if_fail (GO_IS_IMAGE (image), NULL);
	if (image->pixbuf == NULL)
		image->pixbuf = ((GOImageClass *) G_OBJECT_GET_CLASS (image))->get_pixbuf (image);
	return g_object_ref (image->pixbuf);
}

/**
 * go_image_get_scaled_pixbuf:
 * @image: #GOImage
 * @width: the new pixbuf width
 * @height: the new pixbuf height
 *
 * Builds a scaled pixbuf from the image and returns it. The caller needs to
 * unref it. The pixbuf is scaled so that its width and height are not larger
 * than, respectively, @width and @height, and preserving the aspect ratio.
 * Returns: (transfer full): the scaled pixbuf for @image
 **/
GdkPixbuf *
go_image_get_scaled_pixbuf (GOImage *image, int width, int height)
{
	g_return_val_if_fail (GO_IS_IMAGE (image), NULL);
	if (image->width > width || image->height > height) {
		if (image->width * height > image->height * width) {
			height = width * image->height / image->width;
		} else {
			width = height * image->width / image->height;
		}
	} else
		return ((GOImageClass *) G_OBJECT_GET_CLASS (image))->get_pixbuf (image);
	return ((GOImageClass *) G_OBJECT_GET_CLASS (image))->get_scaled_pixbuf (image, width, height);
}

GOImage *
go_image_new_from_file (char const *filename, GError **error)
{
	char *mime, *name;
	GOImageFormat format;

	if (!filename) {
		return NULL;
	}
	mime = go_get_mime_type (filename);
	if (!mime) {
		return NULL;
	}
	name = go_mime_to_image_format (mime);
	g_free (mime);
	if (!name) {
		return NULL;
	}
	format = go_image_get_format_from_name (name);
	g_free (name);
	switch (format) {
	case GO_IMAGE_FORMAT_SVG:
#ifdef GOFFICE_WITH_LIBRSVG
		return go_svg_new_from_file (filename, error);
#endif /* GOFFICE_WITH_LIBRSVG */
		break;
	case GO_IMAGE_FORMAT_EMF:
	case GO_IMAGE_FORMAT_WMF:
		return go_emf_new_from_file (filename, error);
	case GO_IMAGE_FORMAT_EPS:
		return go_spectre_new_from_file (filename, error);
	case GO_IMAGE_FORMAT_PS: /* we might support ps with libspectre */
	case GO_IMAGE_FORMAT_PDF:
	case GO_IMAGE_FORMAT_UNKNOWN:
		break;
	default: {
		GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (filename, error);
		if (pixbuf) {
			GOImage *image = go_pixbuf_new_from_pixbuf (pixbuf);
			g_object_unref (pixbuf);
			return image;
		}
	}
	}
	return NULL;
}

GOImage	*
go_image_new_from_data (char const *type, guint8 const *data, gsize length, char **format, GError **error)
{
	char *real_type = NULL;
	GOImage *image = NULL;
	if (type == NULL || *type == 0) {
		char *mime_type = go_get_mime_type_for_data (data, length);
		real_type = go_mime_to_image_format (mime_type);
		g_free (mime_type);
		type = real_type;
	}
	if (type == NULL) {
		g_warning ("unrecognized image format");
		return NULL;
	}
	if (data == NULL || length == 0) {
		image = NULL;
		type = "unknown";
#ifdef GOFFICE_WITH_LIBRSVG
	} else if (!strcmp (type, "svg")) {
		image = go_svg_new_from_data (data, length, error);
#endif /* GOFFICE_WITH_LIBRSVG */
	} else if (!strcmp (type, "emf") || !strcmp (type, "wmf")) {
		image = go_emf_new_from_data (data, length, error);
	} else if (!strcmp (type, "eps")) {
		image = go_spectre_new_from_data (data, length, error);
	} else {
		image = go_pixbuf_new_from_data (type, data, length, error);
	}
	if (image == NULL) {
#ifdef GOFFICE_WITH_GTK
		GdkScreen *screen = gdk_screen_get_default ();
		GtkIconTheme *theme = screen
			? gtk_icon_theme_get_default()
			: gtk_icon_theme_new();
		GdkPixbuf *placeholder;
		const char *icon_name;

		if (theme) {
			if (gtk_icon_theme_has_icon (theme, "unknown_image"))
				icon_name = "unknown_image";
			else if (gtk_icon_theme_has_icon (theme, "unknown"))
				icon_name = "unknown";
			else
				icon_name = gtk_icon_theme_get_example_icon_name (theme);

			placeholder = gtk_icon_theme_load_icon (theme, icon_name, 100, 0, NULL);
			image = go_pixbuf_new_from_pixbuf (placeholder);
			g_object_unref (placeholder);

			if (!screen)
				g_object_unref (theme);
		}
#endif
	}
	if (format)
		*format = g_strdup (type);
	g_free (real_type);
	return image;
}

GOImage *
go_image_new_for_format (char const *format)
{
	GType gtype = go_image_type_for_format (format);
	return (gtype > 0)? g_object_new (gtype, NULL): NULL;
}

GType
go_image_type_for_format (char const *format)
{
	g_return_val_if_fail (format && *format, 0);
#ifdef GOFFICE_WITH_LIBRSVG
	if (!strcmp (format, "svg"))
		return GO_TYPE_SVG;
#endif /* GOFFICE_WITH_LIBRSVG */
	if (!strcmp (format, "emf") || !strcmp (format, "wmf"))
		return GO_TYPE_EMF;
	if (!strcmp (format, "eps"))
		return GO_TYPE_SPECTRE;
	if (go_image_get_format_from_name (format) != GO_IMAGE_FORMAT_UNKNOWN)
		return GO_TYPE_PIXBUF;
	return 0;
}

guint8 const *
go_image_get_data (GOImage *image, gsize *length)
{
	g_return_val_if_fail (image, NULL);
	if (length)
		*length = image->data_length;
	return image->data;
}

void
go_image_set_name (GOImage *image, char const *name)
{
	g_return_if_fail (GO_IS_IMAGE (image));
	g_free (image->name);
	image->name = (name)? g_strdup (name): NULL;
}

char const *
go_image_get_name (GOImage const *image)
{
	g_return_val_if_fail (GO_IS_IMAGE (image), NULL);
	return image->name;
}

gboolean
go_image_differ (GOImage *first, GOImage *second)
{
	g_return_val_if_fail (GO_IS_IMAGE (first), FALSE);
	g_return_val_if_fail (GO_IS_IMAGE (second), FALSE);
	if (G_OBJECT_TYPE (first) != G_OBJECT_TYPE (second))
		return TRUE;
	if (first->width != second->width || first->height != second->height)
		return TRUE;
	return ((GOImageClass *) G_OBJECT_GET_CLASS (first))->differ (first, second);
}

void
go_image_save (GOImage *image, GsfXMLOut *output)
{
	g_return_if_fail (GO_IS_IMAGE (image) && image->name);
	gsf_xml_out_start_element (output, "GOImage");
	gsf_xml_out_add_cstr (output, "name", image->name);
	gsf_xml_out_add_cstr (output, "type", G_OBJECT_TYPE_NAME (image));
	gsf_xml_out_add_int (output, "width", image->width);
	gsf_xml_out_add_int (output, "height", image->height);
	((GOImageClass *) G_OBJECT_GET_CLASS (image))->save (image, output);
	gsf_xml_out_end_element (output);
}

void
go_image_load_attrs (GOImage *image, GsfXMLIn *xin, xmlChar const **attrs)
{
	xmlChar const **attr;
	g_return_if_fail (image != NULL);
	for (attr = attrs; attr != NULL && attr[0] && attr[1] ; attr += 2)
		if (0 == strcmp (attr[0], "width"))
			image->width = strtol (attr[1], NULL, 10);
		else if (0 == strcmp (attr[0], "height"))
			image->height= strtol (attr[1], NULL, 10);
		else
			((GOImageClass *) G_OBJECT_GET_CLASS (image))->load_attr (image, attr[0], attr[1]);
}

void
go_image_load_data (GOImage *image, GsfXMLIn *xin)
{
	((GOImageClass *) G_OBJECT_GET_CLASS (image))->load_data (image, xin);
}

void
_go_image_changed (GOImage *image, double width, double height)
{
	image->width = width;
	image->height = height;
	if (image->thumbnail) {
		g_object_unref (image->thumbnail);
		image->thumbnail = NULL;
	}
}

double
go_image_get_width (GOImage const *image)
{
	g_return_val_if_fail (GO_IS_IMAGE (image), 0.);
	return image->width;
}

double
go_image_get_height (GOImage const *image)
{
	g_return_val_if_fail (GO_IS_IMAGE (image), 0.);
	return image->height;
}

static double _go_image_dpi_x = 96.,  _go_image_dpi_y = 96.;

void
go_image_set_default_dpi (double dpi_x, double dpi_y)
{
	_go_image_dpi_x = dpi_x;
	_go_image_dpi_y = dpi_y;
}

void
go_image_get_default_dpi (double *dpi_x, double *dpi_y)
{
	*dpi_x = _go_image_dpi_x;
	*dpi_y = _go_image_dpi_y;
}

GOImageFormatInfo const *
go_image_get_info (GOImage *image)
{
	if (GO_IS_PIXBUF (image)) {
		GOImageFormat f;
		char *typ;
		g_object_get (image, "image-type", &typ, NULL);
		f = go_image_get_format_from_name (typ);
		g_free (typ);
		return go_image_get_format_info (f);
	}

	/* Dubious */
	if (GO_IS_EMF (image))
		return go_image_get_format_info (GO_IMAGE_FORMAT_EMF);
#ifdef GOFFICE_WITH_LIBRSVG
	if (GO_IS_SVG (image))
		return go_image_get_format_info (GO_IMAGE_FORMAT_SVG);
#endif /* GOFFICE_WITH_LIBRSVG */
	if (GO_IS_SPECTRE (image))
		return go_image_get_format_info (GO_IMAGE_FORMAT_EPS);
	return NULL;
}
