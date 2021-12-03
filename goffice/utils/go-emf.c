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
	GocGroup *group;
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
			image->data, image->data_length);
}

static void
go_emf_load_attr (G_GNUC_UNUSED GOImage *image, G_GNUC_UNUSED xmlChar const *attr_name, G_GNUC_UNUSED xmlChar const *attr_value)
{
	/* nothing to do */
}

static void load_wmf_as_pixbuf (GOImage *image, guint8 const *data, size_t length)
{
#ifndef GOFFICE_EMF_SUPPORT
	GdkPixbufLoader *loader = gdk_pixbuf_loader_new_with_type ("wmf", NULL);

	if (loader) {
		gboolean ret = gdk_pixbuf_loader_write (loader, data, length, NULL);
		/* Close in any case. But don't let error during closing
		 * shadow error from loader_write.  */
		gdk_pixbuf_loader_close (loader, NULL);
		if (ret)
			image->pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
		if (image->pixbuf)
			g_object_ref (image->pixbuf);
		g_object_unref (loader);
		image->width = gdk_pixbuf_get_width (image->pixbuf);
		image->height = gdk_pixbuf_get_height (image->pixbuf);
	}
#endif
}

static void
go_emf_load_data (GOImage *image, GsfXMLIn *xin)
{
#ifdef GOFFICE_EMF_SUPPORT
	GOEmf *emf = GO_EMF (image);
	GError *error = NULL;
	GsfInput *input;
#endif
	image->data_length = gsf_base64_decode_simple (xin->content->str, strlen(xin->content->str));
	image->data = go_memdup (xin->content->str, image->data_length);
#ifdef GOFFICE_EMF_SUPPORT
	input = gsf_input_memory_new (image->data, image->data_length, FALSE);
	go_emf_parse (emf, input, &error);
	g_object_unref (input);
	if (error) {
		/* FIXME: emit at least a warning */
		g_error_free (error);
	}
#else
	load_wmf_as_pixbuf (image, image->data, image->data_length);
#endif
}

#ifdef GOFFICE_EMF_SUPPORT
static void
go_emf_draw (GOImage *image, cairo_t *cr)
{
	GOEmf *emf = GO_EMF (image);
	g_return_if_fail (emf && emf->group);
	goc_item_draw_region ((GocItem *) emf->group, cr, 0, 0, image->width, image->height);
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
	goc_item_draw_region ((GocItem *) emf->group, cr, 0, 0, image->width, image->height);
	cairo_destroy (cr);
	res = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, image->width, image->height);
	go_cairo_convert_data_from_pixbuf (gdk_pixbuf_get_pixels (res),
	                                   cairo_image_surface_get_data (surface),
	                                   image->width, image->height,
	                                   cairo_image_surface_get_stride (surface));
	return res;
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
	goc_item_draw_region ((GocItem *) emf->group, cr, 0, 0, image->width, image->height);
	cairo_destroy (cr);
	res = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, width, height);
	go_cairo_convert_data_from_pixbuf (gdk_pixbuf_get_pixels (res),
	                                   cairo_image_surface_get_data (surface),
	                                   width, height,
	                                   cairo_image_surface_get_stride (surface));
	return res;
}
#endif

static gboolean
go_emf_differ (GOImage *first, GOImage *second)
{
	if (first->data_length != second->data_length)
		return TRUE;
	return memcmp (first->data, second->data, first->data_length);
}

static void
go_emf_finalize (GObject *obj)
{
	GOEmf *emf = GO_EMF (obj);
	if (emf->group != NULL)
		g_object_unref (emf->group);
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
#ifdef GOFFICE_EMF_SUPPORT
	image_klass->get_pixbuf = go_emf_get_pixbuf;
	image_klass->get_scaled_pixbuf = go_emf_get_scaled_pixbuf;
	image_klass->draw = go_emf_draw;
#endif
	image_klass->differ = go_emf_differ;
}

static void
go_emf_init (GOEmf *emf)
{
	emf->group = g_object_new (GOC_TYPE_GROUP, NULL);
}

GSF_CLASS (GOEmf, go_emf,
	   go_emf_class_init, go_emf_init,
	   GO_TYPE_IMAGE)

/**
 * go_emf_get_canvas:
 * @emf: #GOEmf
 *
 * Returns: (transfer full): a GocCanvas displaying the EMF image.
 **/
GObject *
go_emf_get_canvas (GOEmf *emf)
{
	GocCanvas *canvas;

	g_return_val_if_fail (GO_IS_EMF (emf), NULL);

	canvas = GOC_CANVAS (g_object_new (GOC_TYPE_CANVAS, NULL));
	goc_item_copy ((GocItem *) goc_canvas_get_root (canvas), (GocItem *) emf->group);
	return (GObject *) canvas;
}

GOImage *
go_emf_new_from_file (char const *filename, GError **error)
{
	GOEmf *emf = NULL;
	GOImage *image;
	GsfInput *input = gsf_input_stdio_new (filename, error);
	guint8 *data;
	size_t size;

	if (input == NULL)
		return NULL;
	size = gsf_input_size (input);
	data = g_try_malloc (size);
	if (!data || !gsf_input_read (input, size, data)) {
		g_free (data);
		if (error)
			*error = g_error_new (go_error_invalid (), 0,
			                      _("Could not load the image data\n"));
		return NULL;
	}
	g_object_unref (input);
	emf = g_object_new (GO_TYPE_EMF, NULL);

	image = GO_IMAGE (emf);
	image->data_length = size;
	image->data = data;
	input = gsf_input_memory_new (data, image->data_length, FALSE);
	if (!go_emf_parse (emf, input, error)) {
		if (image->width < 1.) {
			/* we load the size for an EMF file, so it should be WMF */
			/* try to get a pixbuf */
			load_wmf_as_pixbuf (image, data, size);
		}
	} else if (image->width == 0 || image->height == 0) {
		double x0, y0, x1, y1;
		goc_item_get_bounds ((GocItem *) emf->group, &x0, &y0, &x1, &y1);
		image->width = x1;
		image->height = y1;
	}
	g_object_unref (input);

	return image;
}

GOImage *
go_emf_new_from_data (char const *data, size_t length, GError **error)
{
	GOEmf *emf = NULL;
	GsfInput *input;
	GOImage *image;

	g_return_val_if_fail (data != NULL && length > 0, NULL);
	input = gsf_input_memory_new (data, length, FALSE);
	if (input == NULL) {
		if (error)
			*error = g_error_new (go_error_invalid (), 0,
			                      _("Could not input the image data\n"));
		return NULL;
	}
	emf = g_object_new (GO_TYPE_EMF, NULL);
	image = GO_IMAGE (emf);
	image->data_length = gsf_input_size (input);
	image->data = go_memdup (data, length);
	if (!go_emf_parse (emf, input, error)) {
		if (image->width < 1.) {
			/* we load the size for an EMF file, so it should be WMF */
			/* try to get a pixbuf */
			load_wmf_as_pixbuf (image, data, length);
		}
	} else if (image->width == 0 || image->height == 0) {
		double x0, y0, x1, y1;
		goc_item_get_bounds ((GocItem *) emf->group, &x0, &y0, &x1, &y1);
		image->width = x1;
		image->height = y1;
	}
	g_object_unref (input);
	return image;
}

#ifdef GOFFICE_EMF_SUPPORT

/******************************************************************************
 * DIB support, might go to a separate file
 ******************************************************************************/
typedef struct {
	guint8 type; /* 0: BitmapCoreHeader, 1: BitmapInfoHeader */
	unsigned size, width, height, planes, bit_count, compression,
		 image_size, x_pixels_per_m, y_pixels_per_m,
		 color_used, color_important;
	GOColor *colors;
} GODibHeader;

static void
go_dib_read_header (GODibHeader *header, guint8 const *data)
{
	header->size = GSF_LE_GET_GUINT32 (data);
	header->type = (header->size == 12)? 0: 1;
	if (header->type == 0) {
		header->width = GSF_LE_GET_GUINT16 (data + 4);
		header->height = GSF_LE_GET_GUINT16 (data + 6);
		header->planes = GSF_LE_GET_GUINT16 (data + 8);
		header->bit_count = GSF_LE_GET_GUINT16 (data + 10);
		/* other fields are unused */
		header->compression = 0;
		header->image_size = 0;
		header->x_pixels_per_m = 0;
		header->y_pixels_per_m = 0;
		header->color_used = 0;
		header->color_important = 0;
	} else {
		if (header->size != 40) {
			/* Invalid header */
			header->type = 255;
			return;
		}
		header->width = GSF_LE_GET_GUINT32 (data + 4);
		header->height = GSF_LE_GET_GUINT32 (data + 8);
		header->planes = GSF_LE_GET_GUINT16 (data + 12);
		header->bit_count = GSF_LE_GET_GUINT16 (data + 14);
		header->compression = GSF_LE_GET_GUINT32 (data + 16);
		header->image_size = GSF_LE_GET_GUINT32 (data + 20);
		header->x_pixels_per_m = GSF_LE_GET_GUINT32 (data + 24);
		header->y_pixels_per_m = GSF_LE_GET_GUINT32 (data + 28);
		header->color_used = GSF_LE_GET_GUINT32 (data + 32);
		header->color_important = GSF_LE_GET_GUINT32 (data + 36);
	}
	if (header->bit_count <= 16 && header->color_used == 0)
		header->color_used = 1 << header->bit_count;
}

static GdkPixbuf *
go_dib_create_pixbuf_from_data (GODibHeader const *header, guint8 const *data)
{
	GdkPixbuf *pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, header->width, header->height);
	unsigned i, j;
	unsigned dst_rowstride = gdk_pixbuf_get_rowstride (pixbuf), src_rowstride;
	guint8 *row, *cur;
	guint8 const *src_row;
	/* FIXME, this implementation is very incomplete */
	switch (header->bit_count) {
	case 0:
		break;
	case 1:
		break;
	case 4:
		break;
	case 8:
		switch (header->compression) {
		case 0:
			src_rowstride = (header->width + 3) / 4 * 4;
			if (header->type == 1  && header->image_size > 0 && src_rowstride * header->height != header->image_size)
				g_warning ("Invalid bitmap size");
			row = gdk_pixbuf_get_pixels (pixbuf) + header->height * dst_rowstride;
			src_row = data;
			for (i = 0; i < header->height; i++) {
				row -= dst_rowstride;
				cur = row;
				data = src_row;
				for (j = 0; j < header->width; j++) {
					/* FIXME: this does not work if the color table is not RGB */
					GOColor color = header->colors[*data];
					cur[0] = GO_COLOR_UINT_R (color);
					cur[1] = GO_COLOR_UINT_G (color);
					cur[2] = GO_COLOR_UINT_B (color);
					cur[3] = 0xff;
					data ++;
					cur += 4;
				}
				src_row += src_rowstride;
			}
			break;
		case 1:
			break;
		case 2:
			break;
		case 3:
			break;
		case 4:
			break;
		case 5:
			break;
		case 11:
			break;
		case 12:
			break;
		case 13:
			break;
		}
		break;
	case 16:
		break;
	case 24:
		src_rowstride = (header->width * 3 + 3) / 4 * 4;
		switch (header->compression) {
		case 0:
			if (header->type == 1 && src_rowstride * header->height != header->image_size)
				g_warning ("Invalid bitmap");
			src_row = data;
			row = gdk_pixbuf_get_pixels (pixbuf) + header->height * dst_rowstride;
			for (i = 0; i < header->height; i++) {
				row -= dst_rowstride;
				data = src_row;
				cur = row;
				for (j = 0; j < header->width; j++) {
					cur[0] = data[2];
					cur[1] = data[1];
					cur[2] = data[0];
					cur[3] = 0xff;
					data += 3;
					cur += 4;
				}
				src_row += src_rowstride;
			}
			break;
		default:
			g_warning ("Invalid compression for a DIB file");
			break;
		}
		break;
	case 32:
		switch (header->compression) {
		case 0:
			if (header->type == 1 && header->width * header->height * 4 != header->image_size)
				g_warning ("Invalid bitmap");
			row = gdk_pixbuf_get_pixels (pixbuf) + header->height * dst_rowstride;
			for (i = 0; i < header->height; i++) {
				row -= dst_rowstride;
				cur = row;
				for (j = 0; j < header->width; j++) {
					cur[0] = data[2];
					cur[1] = data[1];
					cur[2] = data[0];
					cur[3] = 0xff;
					data += 4;
					cur += 4;
				}
			}
			break;
		default:
			g_warning ("Invalid compression for a DIB file");
			break;
		}
		break;
	}
	return pixbuf;
}

/******************************************************************************
 * Parsing code
 *
 * Parse WMF file
 *
 *  Copyright (C) 2010 Valek Filippov (frob@gnome.org)
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
 * ****************************************************************************/

#ifdef DEBUG_EMF_SUPPORT
#       define d_(x)     g_print x

char const *go_emf_dib_colors[] = {
	"DIB_RGB_COLORS",
	"DIB_PAL_COLORS",
	"DIB_PAL_INDICES"
};

char const *go_emf_ternary_raster_operation[] = {
	"BLACKNESS",
	"DPSOON",
	"DPSONA",
	"PSON",
	"SDPONA",
	"DPON",
	"PDSXNON",
	"PDSAON",
	"SDPNAA",
	"PDSXON",
	"DPNA",
	"PSDNAON",
	"SPNA",
	"PDSNAON",
	"PDSONON",
	"PN",
	"PDSONA",
	"NOTSRCERASE",
	"SDPXNON",
	"SDPAON",
	"DPSXNON",
	"DPSAON",
	"PSDPSANAXX",
	"SSPXDSXAXN",
	"SPXPDXA",
	"SDPSANAXN",
	"PDSPAOX",
	"SDPSXAXN",
	"PSDPAOX",
	"DSPDXAXN",
	"PDSOX",
	"PDSOAN",
	"DPSNAA",
	"SDPXON",
	"DSNA",
	"SPDNAON",
	"SPXDSXA",
	"PDSPANAXN",
	"SDPSAOX",
	"SDPSXNOX",
	"DPSXA",
	"PSDPSAOXXN",
	"DPSANA",
	"SSPXPDXAXN",
	"SPDSOAX",
	"PSDNOX",
	"PSDPXOX",
	"PSDNOAN",
	"PSNA",
	"SDPNAON",
	"SDPSOOX",
	"NOTSRCCOPY",
	"SPDSAOX",
	"SPDSXNOX",
	"SDPOX",
	"SDPOAN",
	"PSDPOAX",
	"SPDNOX",
	"SPDSXOX",
	"SPDNOAN",
	"PSX",
	"SPDSONOX",
	"SPDSNAOX",
	"PSAN",
	"PSDNAA",
	"DPSXON",
	"SDXPDXA",
	"SPDSANAXN",
	"SRCERASE",
	"DPSNAON",
	"DSPDAOX",
	"PSDPXAXN",
	"SDPXA",
	"PDSPDAOXXN",
	"DPSDOAX",
	"PDSNOX",
	"SDPANA",
	"SSPXDSXOXN",
	"PDSPXOX",
	"PDSNOAN",
	"PDNA",
	"DSPNAON",
	"DPSDAOX",
	"SPDSXAXN",
	"DPSONON",
	"DSTINVERT",
	"DPSOX",
	"DPSOAN",
	"PDSPOAX",
	"DPSNOX",
	"PATINVERT",
	"DPSDONOX",
	"DPSDXOX",
	"DPSNOAN",
	"DPSDNAOX",
	"DPAN",
	"PDSXA",
	"DSPDSAOXXN",
	"DSPDOAX",
	"SDPNOX",
	"SDPSOAX",
	"DSPNOX",
	"SRCINVERT",
	"SDPSONOX",
	"DSPDSONOXXN",
	"PDSXXN",
	"DPSAX",
	"PSDPSOAXXN",
	"SDPAX",
	"PDSPDOAXXN",
	"SDPSNOAX",
	"PDXNAN",
	"PDSANA",
	"SSDXPDXAXN",
	"SDPSXOX",
	"SDPNOAN",
	"DSPDXOX",
	"DSPNOAN",
	"SDPSNAOX",
	"DSAN",
	"PDSAX",
	"DSPDSOAXXN",
	"DPSDNOAX",
	"SDPXNAN",
	"SPDSNOAX",
	"DPSXNAN",
	"SPXDSXO",
	"DPSAAN",
	"DPSAA",
	"SPXDSXON",
	"DPSXNA",
	"SPDSNOAXN",
	"SDPXNA",
	"PDSPNOAXN",
	"DSPDSOAXX",
	"PDSAXN",
	"SRCAND",
	"SDPSNAOXN",
	"DSPNOA",
	"DSPDXOXN",
	"SDPNOA",
	"SDPSXOXN",
	"SSDXPDXAX",
	"PDSANAN",
	"PDSXNA",
	"SDPSNOAXN",
	"DPSDPOAXX",
	"SPDAXN",
	"PSDPSOAXX",
	"DPSAXN",
	"DPSXX",
	"PSDPSONOXX",
	"SDPSONOXN",
	"DSXN",
	"DPSNAX",
	"SDPSOAXN",
	"SPDNAX",
	"DSPDOAXN",
	"DSPDSAOXX",
	"PDSXAN",
	"DPA",
	"PDSPNAOXN",
	"DPSNOA",
	"DPSDXOXN",
	"PDSPONOXN",
	"PDXN",
	"DSPNAX",
	"PDSPOAXN",
	"DPSOA",
	"DPSOXN",
	"D",
	"DPSONO",
	"SPDSXAX",
	"DPSDAOXN",
	"DSPNAO",
	"DPNO",
	"PDSNOA",
	"PDSPXOXN",
	"SSPXDSXOX",
	"SDPANAN",
	"PSDNAX",
	"DPSDOAXN",
	"DPSDPAOXX",
	"SDPXAN",
	"PSDPXAX",
	"DSPDAOXN",
	"DPSNAO",
	"MERGEPAINT",
	"SPDSANAX",
	"SDXPDXAN",
	"DPSXO",
	"DPSANO",
	"MERGECOPY",
	"SPDSNAOXN",
	"SPDSONOXN",
	"PSXN",
	"SPDNOA",
	"SPDSXOXN",
	"SDPNAX",
	"PSDPOAXN",
	"SDPOA",
	"SPDOXN",
	"DPSDXAX",
	"SPDSAOXN",
	"SRCCOPY",
	"SDPONO",
	"SDPNAO",
	"SPNO",
	"PSDNOA",
	"PSDPXOXN",
	"PDSNAX",
	"SPDSOAXN",
	"SSPXPDXAX",
	"DPSANAN",
	"PSDPSAOXX",
	"DPSXAN",
	"PDSPXAX",
	"SDPSAOXN",
	"DPSDANAX",
	"SPXDSXAN",
	"SPDNAO",
	"SDNO",
	"SDPXO",
	"SDPANO",
	"PDSOA",
	"PDSOXN",
	"DSPDXAX",
	"PSDPAOXN",
	"SDPSXAX",
	"PDSPAOXN",
	"SDPSANAX",
	"SPXPDXAN",
	"SSPXDSXAX",
	"DSPDSANAXXN",
	"DPSAO",
	"DPSXNO",
	"SDPAO",
	"SDPXNO",
	"SRCPAINT",
	"SDPNOO",
	"PATCOPY",
	"PDSONO",
	"PDSNAO",
	"PSNO",
	"PSDNAO",
	"PDNO",
	"PDSXO",
	"PDSANO",
	"PDSAO",
	"PDSXNO",
	"DPO",
	"PATPAINT",
	"PSO",
	"PSDNOO",
	"DPSOO",
	"WHITENESS"
};

#else
#       define d_(x)
#endif

/*****************************************************************************
 * First WMF parsing code
 *****************************************************************************/

typedef struct {
	guint8 b;
	guint8 g;
	guint8 r;
	guint8 a;
} GOWmfColor;

typedef struct {
	double VPx;
	double VPy;
	double VPOx;
	double VPOy;
	double Wx;
	double Wy;
	double x;
	double y;
	double curx;
	double cury;
	gint curfg;
	gint curbg;
	gint curfnt;
	gint curpal;
	gint curreg;
	guint txtalign;
	guint bkmode;
	guint pfm;
	GOWmfColor bkclr;
	GOWmfColor txtclr;
} GOWmfDC;

typedef struct {
	guint type; /* 1 pen, 2 brush, 3 font, 4 region, 5 palette */
	gpointer values;
} GOWmfMFobj;

typedef struct {
	guint16 style;
	gint16 width;
	gint16 height;
	GOWmfColor clr;
	guint16 flag;
} GOWmfPen;

typedef enum {
	GO_EMF_OBJ_TYPE_INVALID = 0,
	GO_EMF_OBJ_TYPE_PEN,
	GO_EMF_OBJ_TYPE_BRUSH,
	GO_EMF_OBJ_TYPE_FONT,
	GO_EMF_OBJ_TYPE_MAX
} GOEmfObjType;

typedef struct {
	GOEmfObjType obj_type;
	guint32 style;
	double width;
	GOColor clr;
} GOEmfPen;

typedef struct {
	GOEmfObjType obj_type;
	guint32 style;
	guint32 hatch;
	GOColor clr;
} GOEmfBrush;

typedef struct {
	GOEmfObjType obj_type;
	int size;
	int width;
	int escape;
	int orient;
	int weight;
	gboolean italic;
	gboolean under;
	gboolean strike;
	unsigned charset;
	unsigned outprec;
	unsigned clipprec;
	unsigned quality;
	unsigned pitch_and_family;
	char facename[64];
} GOEmfFont;

typedef union {
	GOEmfObjType obj_type;
	GOEmfPen pen;
	GOEmfBrush brush;
	GOEmfFont font;
} GOEmfObject;

typedef struct {
	guint width;
	guint height;
	gpointer data;
} GOWmfBrushData;

typedef struct {
	guint16 style;
	GOWmfColor clr;
	guint16 hatch;
	GOWmfBrushData bdata;
} GOWmfBrush;

typedef struct {
	double b;
	double r;
	double t;
	double l;
	double xs;
	double ys;
	double xe;
	double ye;
} GOWmfArc;


typedef struct {
	gpointer name;
	double size;
	gint16 width;
	gint16 escape;
	gint16 orient;
	gint16 weight;
	gint italic;
	gint under;
	gint strike;
	gchar *charset;
} GOWmfFont;

typedef struct {
	int maxobj;
	guint curobj;
	guint curdc;
	GHashTable *mfobjs;
	GArray *dcs;
	double zoom;
	double w;
	double h;
	int type;
} GOWmfPage;

#else
#	define d_(x)
#endif

typedef struct {
	gint32 left, right, top, bottom;
} GOWmfRectL;

static void
go_wmf_read_rectl (GOWmfRectL *rect, guint8 const *data)
{
	rect->left = GSF_LE_GET_GINT32 (data);
	rect->top = GSF_LE_GET_GINT32 (data + 4);
	rect->right = GSF_LE_GET_GINT32 (data + 8);
	rect->bottom = GSF_LE_GET_GINT32 (data + 12);
}

typedef enum {
MM_TEXT = 0x01,
MM_LOMETRIC = 0x02,
MM_HIMETRIC = 0x03,
MM_LOENGLISH = 0x04,
MM_HIENGLISH = 0x05,
MM_TWIPS = 0x06,
MM_ISOTROPIC = 0x07,
MM_ANISOTROPIC = 0x08
} GOEmfMapMode;

typedef enum
{
R2_BLACK = 0x0001,
R2_NOTMERGEPEN = 0x0002,
R2_MASKNOTPEN = 0x0003,
R2_NOTCOPYPEN = 0x0004,
R2_MASKPENNOT = 0x0005,
R2_NOT = 0x0006,
R2_XORPEN = 0x0007,
R2_NOTMASKPEN = 0x0008,
R2_MASKPEN = 0x0009,
R2_NOTXORPEN = 0x000A,
R2_NOP = 0x000B,
R2_MERGENOTPEN = 0x000C,
R2_COPYPEN = 0x000D,
R2_MERGEPENNOT = 0x000E,
R2_MERGEPEN = 0x000F,
R2_WHITE = 0x0010
} GOWmfOperation;

typedef struct {
	GOWmfOperation op;
	gboolean fill_bg;
	GocGroup *group;
	GOStyle *style;
	GOPath *path;
	gboolean closed_path;
	gboolean PolygonFillMode; /* TRUE: winding, FALSE: alternate */
	GOAnchorType text_align;
	GOColor text_color;
	cairo_matrix_t m;
	double xpos, ypos; /* current position used to emit text */
	GOFont const *font;
	double text_rotation;
} GOEmfDC;

typedef struct {
	unsigned version;
	GocGroup *group;
	GError **error;
	guint8 const *data;
	unsigned length;
	GOWmfRectL dubounds; /* bounds in device units */
	GOWmfRectL mmbounds; /* bounds in mm/100 */
	double dx, dy, dw, dh; /* view port in device units */
	double wx, wy, ww, wh; /* window rectangle in logical unit */
	GOEmfMapMode map_mode;
	gboolean is_emf;	/* FIXME: might be EPS or WMF */
	GOEmfDC *curDC;
	GSList *dc_stack;
	GHashTable *mfobjs;
} GOEmfState;

#ifdef GOFFICE_EMF_SUPPORT

#define CMM2PTS(x) (((double) x) * 72. / 254.)

static char *
go_wmf_mtextra_to_utf (char *txt)
{
	const guint16 mte[256] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
	0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x300,
	0x302, 0x303, 0x2d9, 0x27, 0x2322, 0x2323, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0xe13d, 0xe141, 0xe13e, 0x23af, 0x35, 0xe13b, 0xe140, 0xe13c, 0x39, 0x3a, 0x3b,
	0x25c3, 0x3d, 0x25b9, 0x3f, 0x40, 0x41, 0x42, 0x2210, 0x19b, 0x45, 0x46,
	0x47, 0x48, 0x2229, 0x4a, 0x2026, 0x22ef, 0x22ee, 0x22f0, 0x22f1, 0x50,
	0x2235, 0x52, 0x53, 0x54, 0x222a, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c,
	0x5d, 0x5e, 0x5f, 0x2035, 0x21a6, 0x2195, 0x21d5, 0x64, 0x65, 0x227b, 0x67,
	0x210f, 0x69, 0x6a, 0x6b, 0x2113, 0x2213, 0x6e, 0x2218, 0x227a, 0x71, 0x20d7,
	0x20d6, 0x20e1, 0x20d6, 0x20d1, 0x20d0, 0x78, 0x79, 0x7a, 0x23de, 0x7c,
	0x23df, 0x7e,0x7f, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88,
	0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94,
	0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0,
	0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac,
	0xad, 0xae, 0xaf, 0xb0, 0xa1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8,
	0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4,
	0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0,
	0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc,
	0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8,
	0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1, 0xf2, 0xf3, 0xf4,
	0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff};
	guint16  *data = {0};
	char* utxt;
	guint i;

	data = malloc (strlen (txt) * 2);
	for (i = 0; i < strlen (txt); i++)
		data[i] = mte[(guint8) txt[i]];
	utxt =  g_utf16_to_utf8 (data, strlen (txt), NULL, NULL, NULL);
	return utxt;
}

static char *
go_wmf_symbol_to_utf (char* txt)
{
	const guint16 utf[256] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
	0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x2200, 0x23,
	0x2203, 0x25, 0x26, 0x220d, 0x28, 0x29, 0x2217, 0x2b, 0x2c, 0x2212, 0x2e,
	0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a,
	0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x2245, 0x391, 0x392, 0x3a7, 0x394, 0x395,
	0x3a6, 0x393, 0x397, 0x399, 0x3d1, 0x39a, 0x39b, 0x39c, 0x39d, 0x39f,
	0x3a0, 0x398, 0x3a1, 0x3a3, 0x3a4, 0x3a5, 0x3c2, 0x3a9, 0x39e, 0x3a8,
	0x396, 0x5b, 0x2234, 0x5d, 0x22a5, 0x5f, 0xf8e5, 0x3b1, 0x3b2, 0x3c7,
	0x3b4, 0x3b5, 0x3c6, 0x3b3, 0x3b7, 0x3b9, 0x3d5, 0x3ba, 0x3bb, 0x3bc,
	0x3bd, 0x3bf, 0x3c0, 0x3b8, 0x3c1, 0x3c3, 0x3c4, 0x3c5, 0x3d6, 0x3c9,
	0x3be, 0x3c8, 0x3b6, 0x7b, 0x7c, 0x7d, 0x223c, 0x7f, 0x80, 0x81, 0x82,
	0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e,
	0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a,
	0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0x3d2, 0x2032, 0x2264, 0x2044, 0x201e,
	0x192, 0x2663, 0x2666, 0x2665, 0x2660, 0x2194, 0x2190, 0x2191, 0x2192,
	0x2193, 0xb0, 0xb1, 0x2033, 0x2265, 0xd7, 0x221d, 0x2202, 0x2022, 0xf7,
	0x2260, 0x2261, 0x2248, 0x2026, 0xf8e6, 0xf8e7, 0x21b5, 0x2135, 0x2111,
	0x211c, 0x2118, 0x2297, 0x2295, 0x2205, 0x2229, 0x222a, 0x2283, 0x2287,
	0x2284, 0x2282, 0x2286, 0x2208, 0x2209, 0x2220, 0x2207, 0xae, 0xa9, 0x2122,
	0x220f, 0x221a, 0x22c5, 0xac, 0x2227, 0x2228, 0x21d4, 0x21d0, 0x21d1,
	0x21d2, 0x21d3, 0x22c4, 0x2329, 0xf8e8, 0xf8e9, 0xf8ea, 0x2211, 0xf8eb,
	0xf8ec, 0xf8ed, 0xf8ee, 0xf8ef, 0xf8f0, 0xf8f1, 0xf8f2, 0xf8f3, 0xf8f4,
	0xf8ff, 0x232a, 0x222b, 0x2320, 0xf8f5, 0x2321, 0xf8f6, 0xf8f7, 0xf8f8,
	0xf8f9, 0xf8fa, 0xf8fb, 0xf8fc, 0xf8fd, 0xf8fe, 0x2c7};
	guint16  *data = {0};
	char* utxt;
	guint i;

	data = malloc (strlen (txt) * 2);
	for (i = 0; i < strlen (txt); i++)
		data[i] = utf[(guint8) txt[i]];
	utxt =  g_utf16_to_utf8 (data, strlen (txt), NULL, NULL, NULL);
	return utxt;
}

static void
go_wmf_set_font (GOWmfPage *pg, GocItem *item)
{
	GOWmfMFobj *mfo;
	GOWmfFont *font;
	double rot;
	PangoAttrList *pattl = pango_attr_list_new ();
	GOWmfDC *dc;

	dc = &g_array_index (pg->dcs, GOWmfDC, pg->curdc);
	if (-1 == dc->curfnt)
		return;

	mfo = g_hash_table_lookup (pg->mfobjs, GINT_TO_POINTER (dc->curfnt));
	font = mfo->values;
	/* convert font values to PangoAttrList */
	if (font->escape > 0) {
		rot = (double) (font->escape % 3600) * M_PI / 1800.;
		goc_item_set (item, "rotation", M_PI * 2 - rot, NULL);
	}
	if (font->size < 0)
		font->size = -font->size;
	if (font->size == 0)
		font->size = 12;
	pango_attr_list_insert (pattl, pango_attr_size_new (font->size * 1024. * pg->zoom / 1.5));
	pango_attr_list_insert (pattl, pango_attr_foreground_new (dc->txtclr.r * 256, dc->txtclr.g * 256, dc->txtclr.b * 256));

	if (2 == dc->bkmode)
		pango_attr_list_insert (pattl, pango_attr_background_new (dc->bkclr.r * 256, dc->bkclr.g * 256, dc->bkclr.b * 256));
	pango_attr_list_insert (pattl, pango_attr_weight_new (font->weight));
	pango_attr_list_insert (pattl, pango_attr_family_new (font->name));
	if (0 != font->italic)
		pango_attr_list_insert (pattl, pango_attr_style_new (PANGO_STYLE_ITALIC));
	if (0 != font->under)
		pango_attr_list_insert (pattl, pango_attr_underline_new (PANGO_UNDERLINE_SINGLE));
	if (0 != font->strike)
		pango_attr_list_insert (pattl, pango_attr_strikethrough_new (TRUE));
	goc_item_set(item, "attributes", pattl, NULL);
}

static void
go_wmf_init_dc (GOWmfDC *dc)
{
	dc->VPx = 1;
	dc->VPy = 1;
	dc-> VPOx = 0;
	dc->VPOy = 0;
	dc->Wx = 1.;
	dc->Wy = 1.;
	dc->x = 0;
	dc->y = 0;
	dc->curx = 0;
	dc->cury = 0;
	dc->curfg = -1;
	dc->curbg = -1;
	dc->curfnt = -1;
	dc->curpal = -1;
	dc->curreg = -1;
	dc->txtalign = 0;
	dc->bkmode = 0;
	dc->pfm = 1;
	dc->bkclr = (GOWmfColor) {255, 255, 255, 255};
	dc->txtclr = (GOWmfColor) {0, 0, 0, 255};
}

static void
go_wmf_init_page (GOWmfPage* pg)
{
	GOWmfDC mydc;

	go_wmf_init_dc (&mydc);
	pg->maxobj = 0;
	pg->curobj = 0;
	pg->curdc = 0;
	pg->w = 320;
	pg->h = 200;
	pg->zoom = 1.;
	pg->type = 0;
	pg->mfobjs = g_hash_table_new (g_direct_hash, g_direct_equal);
	pg->dcs = g_array_new (FALSE, FALSE, sizeof (GOWmfDC));
	g_array_append_vals (pg->dcs, &mydc, 1);
}

static void
go_wmf_mr_convcoord (double* x, double* y, GOWmfPage* pg)
{
	GOWmfDC *dc;

	dc = &g_array_index (pg->dcs, GOWmfDC, pg->curdc);
	*x = (*x - dc->x) * dc->VPx / dc->Wx + dc->VPOx;
	*y = (*y - dc->y) * dc->VPy / dc->Wy + dc->VPOy;
}

static void
go_wmf_read_points (guint8 const *src, double *x, double *y)
{
	*x = GSF_LE_GET_GINT16 (src);
	*y = GSF_LE_GET_GINT16 (src + 2);
}

static void
go_wmf_read_pointl (guint8 const *src, double *x, double *y)
{
	*x = GSF_LE_GET_GINT32 (src);
	*y = GSF_LE_GET_GINT32 (src + 4);
}

static void
go_wmf_read_point (GsfInput* input, double* y, double* x)
{
	const guint8  *data = {0};

	data = gsf_input_read (input, 2, NULL);
	*y = GSF_LE_GET_GINT16 (data);
	data = gsf_input_read (input, 2, NULL);
	*x = GSF_LE_GET_GINT16 (data);
}

static void
go_wmf_set_anchor (GOWmfPage* pg, GOAnchorType* anchor)
{
	GOWmfDC *dc;

	dc = &g_array_index (pg->dcs, GOWmfDC, pg->curdc);

	switch(dc->txtalign & 6) {
	case 0: /* right */
		switch(dc->txtalign & 24) {
			case 0: /* top */
				*anchor = GO_ANCHOR_SOUTH_WEST;
				break;
			case 8: /* bottom */
				*anchor = GO_ANCHOR_NORTH_WEST;
				break;
			case 24: /* baseline */
				*anchor = GO_ANCHOR_WEST;
				break;
		}
		break;
	case 2: /* left */
		switch(dc->txtalign & 24) {
			case 0: /* top */
				*anchor = GO_ANCHOR_SOUTH_EAST;
				break;
			case 8: /* bottom */
				*anchor = GO_ANCHOR_NORTH_EAST;
				break;
			case 24: /* baseline */
				*anchor = GO_ANCHOR_EAST;
				break;
		}
		break;
	case 6: /* center */
		switch(dc->txtalign & 24) {
			case 0: /* top */
				*anchor = GO_ANCHOR_SOUTH;
				break;
			case 8: /* bottom */
				*anchor = GO_ANCHOR_NORTH;
				break;
			case 24: /* baseline */
				*anchor = GO_ANCHOR_CENTER;
				break;
		}
	}
}

static void
go_wmf_set_align (GsfInput* input, GOWmfPage* pg, double* x, double* y)
{
	GOWmfDC *dc;

	dc = &g_array_index (pg->dcs, GOWmfDC, pg->curdc);
	if (dc->txtalign % 2) {
		/* cpupdate */
		/* FIXME: have to update pg->curx, pg->cury with layout size*/
		*x = dc->curx;
		*y = dc->cury;
		gsf_input_seek (input, 4, G_SEEK_CUR);
	} else {
		go_wmf_read_point (input, y, x);
		go_wmf_mr_convcoord (x, y, pg);
	}
}

static void
go_wmf_set_text (GOWmfPage* pg, GocGroup *group, char* txt, int len, GOAnchorType* anchor, double* x, double* y)
{
	GocItem *gocitem;
	char *utxt;
	GOWmfMFobj *mfo;
	GOWmfFont *font;
	GOWmfDC *dc;

	dc = &g_array_index (pg->dcs, GOWmfDC, pg->curdc);
	txt[len] = 0;
	if (-1 != dc->curfnt) {
		mfo = g_hash_table_lookup (pg->mfobjs, GINT_TO_POINTER (dc->curfnt));
		font = mfo->values;

		if (!g_ascii_strcasecmp ("MT Extra", font->name)) {
			utxt = go_wmf_mtextra_to_utf (txt);
		} else if (!g_ascii_strcasecmp ("SYMBOL", font->name)) {
			utxt = go_wmf_symbol_to_utf (txt);
		} else {
			utxt = g_convert (txt, len, "utf8", font->charset, NULL, NULL, NULL);
		}

	} else {
		utxt = g_convert (txt, len, "utf8", "ASCII", NULL, NULL, NULL);
	}
	gocitem = goc_item_new (group, GOC_TYPE_TEXT,
					"x", *x, "y", *y, "text", utxt, "anchor", *anchor, NULL);
	go_wmf_set_font (pg, gocitem);
}

static void
go_wmf_read_color (GsfInput* input, GOWmfColor* clr)
{
	const guint8  *data = {0};

	data = gsf_input_read (input, 1, NULL);
	clr->r = *data;
	data = gsf_input_read (input, 1, NULL);
	clr->g = *data;
	data = gsf_input_read (input, 1, NULL);
	clr->b = *data;
}

static GOColor
go_wmf_read_gocolor (guint8 const *data)
{
	return GO_COLOR_FROM_RGB (data[0], data[1], data[2]);
}

static int
go_wmf_find_obj (GOWmfPage* pg)
{
	int i = 0;

	while (i <= pg->maxobj) {
		if(NULL == g_hash_table_lookup (pg->mfobjs, GINT_TO_POINTER (i)))
			break;
		else
			i++;
	}
	if (i == pg->maxobj)
		pg->maxobj++;
	return i;
}

/*
 * MF: 0 - solid, 1 - dash, 2 - dot,  3 - dashdot,  4 - dashdotdot
 * GO: 1 - solid, 8 - dash, 6 - dot, 10 - dashdot, 11 - dashdotdot
  */

/*
MF2cairo: capjoin = {0:1,1:2,2:0}
MF: cap = {0:'Flat',1:'Round',2:'Square'}
MF: join = {0:'Miter',1:'Round',2:'Bevel'}
*/

static void
go_wmf_stroke (GOWmfPage *pg, GocItem *item)
{
	GOWmfPen *p;
	GOWmfMFobj *mfo;
	GOStyle *style;
	int dashes[5] = {1, 8, 6, 10, 11};
	int capjoin[3] = {1, 2, 0};
	int pstyle = 0;
	GOWmfDC *dc;

	dc = &g_array_index (pg->dcs, GOWmfDC, pg->curdc);
	if (-1 == dc->curfg)
		return;

	style = go_styled_object_get_style (GO_STYLED_OBJECT (item));

	mfo = g_hash_table_lookup (pg->mfobjs, GINT_TO_POINTER (dc->curfg));
	p = mfo->values;
	pstyle = p->style;
	if (p->style == 5) {
		style->line.dash_type = GO_LINE_NONE;
		style->line.auto_dash = FALSE;
	} else {
		style->line.color = GO_COLOR_FROM_RGB (p->clr.r, p->clr.g, p->clr.b);
		style->line.dash_type = GO_LINE_SOLID;
		if (p->width > 1) {
			style->line.width = (double) p->width * pg->zoom;
		} else {
			style->line.width = 0.;
		}
		if (3 > (pstyle & 0xF00) >> 8) {
			style->line.cap = capjoin[(pstyle & 0xF00) >> 8];
		}
		if (3 > (pstyle & 0xF000) >> 12) {
			style->line.join = capjoin[(pstyle & 0xF000) >> 12];
		}

		if (pstyle > 5)
			pstyle = 0;
		style->line.dash_type = dashes[pstyle];
		if (1 == dc->bkmode) {
			style->line.fore = GO_COLOR_FROM_RGBA (0, 0, 0, 0);
		} else {
			style->line.color = GO_COLOR_FROM_RGB (p->clr.r, p->clr.g, p->clr.b);
		}
	}
}

/*
 * MF: 0 HS_HORIZONTAL, 1 HS_VERTICAL, 2 HS_FDIAGONAL, 3 HS_BDIAGONAL, 4 HS_CROSS, 5 HS_DIAGCROSS
 * GO: 12	       13	       14	       15	       16	       17
 */
static void
go_wmf_fill (GOWmfPage *pg, GocItem *item)
{
	GOWmfBrush *b;
	GOWmfMFobj *mfo;
	GOStyle *style;
	GOWmfDC *dc;

	dc = &g_array_index (pg->dcs, GOWmfDC, pg->curdc);
	if (-1 == dc->curbg)
		return;

	style = go_styled_object_get_style (GO_STYLED_OBJECT (item));
	mfo = g_hash_table_lookup (pg->mfobjs, GINT_TO_POINTER (dc->curbg));
	b = mfo->values;
	switch (b->style) {
	case 1:
		style->fill.type = GO_STYLE_FILL_NONE;
		break;
	case 2:
		style->fill.pattern.pattern = b->hatch + 12;
		if (1 == dc->bkmode) {
			style->fill.pattern.back = GO_COLOR_FROM_RGBA (0, 0, 0, 0);
		} else {
			style->fill.pattern.back = GO_COLOR_FROM_RGB (dc->bkclr.r, dc->bkclr.g, dc->bkclr.b);
		}
		style->fill.type = GO_STYLE_FILL_PATTERN;
		style->fill.pattern.fore = GO_COLOR_FROM_RGB (b->clr.r, b->clr.g, b->clr.b);
		break;
	case 3:
		style->fill.image.image = GO_IMAGE (go_pixbuf_new_from_pixbuf (b->bdata.data));
		style->fill.image.type = GO_IMAGE_WALLPAPER;
		style->fill.type = GO_STYLE_FILL_IMAGE;
		break;
	default:
		style->fill.type = GO_STYLE_FILL_PATTERN;
		style->fill.pattern.back = GO_COLOR_FROM_RGB (b->clr.r, b->clr.g, b->clr.b);
	}
}


static void
go_wmf_mr0 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	d_ (("!EOF\n"));
}

/* -------------- SaveDC ---------------- */
static void
go_wmf_mr1 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	GOWmfDC mydc;

	d_ (("SaveDC\n"));
	mydc = g_array_index (pg->dcs, GOWmfDC, pg->curdc);
	g_array_append_vals (pg->dcs, &mydc, 1);
	pg->curdc = pg->dcs->len - 1;
}

static void
go_wmf_mr2 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	d_ (("RealizePalette: unimplemeted\n"));
}

static void
go_wmf_mr3 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	d_ (("SetPalEntries: unimplemented\n"));
}

/* ------------- CreatePalette ------------ */
static void
go_wmf_mr4 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	/* FIXME: do actual parsing */
	GOWmfMFobj *mf;

	d_ (("CreatePalette\n"));
	mf = malloc (sizeof (GOWmfMFobj));
	mf->type = 5;
	g_hash_table_insert ((*pg).mfobjs, GINT_TO_POINTER (go_wmf_find_obj (pg)), mf);
}

/* ------------- SetBKMode ----------------- */
static void
go_wmf_mr5 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	const guint8  *data = {0};
	GOWmfDC *dc;

	d_ (("SetBkMode\n"));
	dc = &g_array_index (pg->dcs, GOWmfDC, pg->curdc);
	data = gsf_input_read (input, 2, NULL);
	dc->bkmode = GSF_LE_GET_GUINT16 (data);
	gsf_input_seek (input, -2, G_SEEK_CUR);
}

static void
go_wmf_mr6 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	d_ (("SetMapMode: unimplemeted\n"));
}

static void
go_wmf_mr7 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
/*	g_print("SetROP2 "));*/
}

/* ------------- SetReLabs ------------- */
static void
go_wmf_mr8 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	/* FIXME: Exclude, should never be used */
}

/* ----------------- SetPolyfillMode ----------------- */
static void
go_wmf_mr9 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	const guint8  *data = {0};
	GOWmfDC *dc;

	dc = &g_array_index (pg->dcs, GOWmfDC, pg->curdc);
	data = gsf_input_read (input, 2, NULL);
	if (GSF_LE_GET_GUINT16 (data) > 1)
		dc->pfm = 0;
	else
		dc->pfm = 1;
	gsf_input_seek (input, -2, G_SEEK_CUR);
}

static void
go_wmf_mr10 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
/*	g_print("SetStrechBLTMode "));*/
}

static void
go_wmf_mr11 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
/*	g_print("SetTextCharExtra "));*/
}

/* ---------------- RestoreDC ----------------------- */
static void
go_wmf_mr12 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	const guint8  *data = {0};
	gint16 idx;

	data = gsf_input_read (input, 2, NULL);
	idx = GSF_LE_GET_GINT16 (data);
	if (idx > 0)
		pg->curdc = idx;
	else
		pg->curdc -= idx;
	gsf_input_seek (input, -2, G_SEEK_CUR);
}

static void
go_wmf_mr13 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
/*	g_print("InvertRegion "));*/
}

static void
go_wmf_mr14 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
/*	g_print("PaintRegion "));*/
}

static void
go_wmf_mr15 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
/*	g_print("SelectClipRegion "));*/
}

/* -------------------- Select Object ----------------- */
static void
go_wmf_mr16 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	const guint8  *data = {0};
	int idx;
	GOWmfMFobj *mf;
	GOWmfDC *dc;

	dc = &g_array_index (pg->dcs, GOWmfDC, pg->curdc);
	data = gsf_input_read (input, 2, NULL);
	idx = GSF_LE_GET_GUINT16 (data);

	mf = g_hash_table_lookup (pg->mfobjs, GINT_TO_POINTER (idx));
	switch (mf->type) {
	case 1:
		dc->curfg = idx; /* pen */
		break;
	case 2:
		dc->curbg = idx; /* brush */
		break;
	case 3:
		dc->curfnt = idx; /* font */
		break;
	case 4:
		dc->curreg = idx; /* region */
		break;
	case 5:
		/* palette, shouldn't happen "SelectPalette" have to be used to select palette */
		break;
	default:
		break;
	}
	gsf_input_seek (input, -2, G_SEEK_CUR);
}

/* ---------------- SetTextAlign ----------------------- */
static void
go_wmf_mr17 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	const guint8  *data = {0};
	GOWmfDC *dc;

	dc = &g_array_index (pg->dcs, GOWmfDC, pg->curdc);
	data = gsf_input_read (input, 2, NULL);
	dc->txtalign = GSF_LE_GET_GUINT16 (data);
	gsf_input_seek(input, -2, G_SEEK_CUR);
}

static void
go_wmf_mr18 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
/*	g_print("ResizePalette "));*/
}

/* --------------- DIBCreatePatternBrush -------------- */
static void
go_wmf_mr19 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	const guint8  *data = {0};
	GOWmfBrush *brush;
	GOWmfMFobj *mf;
	guint32 w, h, bmpsize, bmpshift = 54;
	guint16 biDepth;
	GdkPixbufLoader *gpbloader = gdk_pixbuf_loader_new ();
	GdkPixbuf *gpb;
	GError **error = NULL;
	guint8 *tmp;
	gint32 rollback = 6 - rsize * 2;

	brush = malloc (sizeof (GOWmfBrush));
	brush->style = 3; /* DibPattern brush */
	gsf_input_seek(input, 8, G_SEEK_CUR);
	data = gsf_input_read (input, 4, NULL);
	w = GSF_LE_GET_GUINT32 (data);
	data = gsf_input_read (input, 4, NULL);
	h = GSF_LE_GET_GUINT32 (data);
	gsf_input_seek (input, 2, G_SEEK_CUR);
	data = gsf_input_read (input, 2, NULL);
	biDepth = GSF_LE_GET_GUINT32 (data);
	gsf_input_seek(input, -16, G_SEEK_CUR);
	data = gsf_input_read (input, rsize * 2 - 6 - 4, NULL);
	bmpsize = rsize * 2 - 6 + 10;

	gsf_input_seek (input, rollback, G_SEEK_CUR);
	switch (biDepth) {
	case 1:
		bmpshift = 62;
		break;
	case 2:
		bmpshift = 70;
		break;
	case 4:
		bmpshift = 118;
		break;
	case 8:
		bmpshift = 1078;
		break;
	}

	tmp = malloc (bmpsize);
	tmp[0] = 0x42;
	tmp[1] = 0x4d;
	memcpy(tmp + 2, &bmpsize, 4);
	tmp[6] = 0;
	tmp[7] = 0;
	tmp[8] = 0;
	tmp[9] = 0;
	memcpy(tmp + 10, &bmpshift, 4);
	memcpy(tmp + 14, data, bmpsize - 14);

	gdk_pixbuf_loader_write (gpbloader, tmp, bmpsize, error);
	gdk_pixbuf_loader_close (gpbloader, error);
	gpb = gdk_pixbuf_loader_get_pixbuf (gpbloader);

	brush->bdata.width = w;
	brush->bdata.height = h;
	brush->bdata.data = gpb;

	mf = malloc (sizeof (GOWmfMFobj));
	mf->type = 2;
	mf->values = brush;
	g_hash_table_insert ((*pg).mfobjs, GINT_TO_POINTER (go_wmf_find_obj (pg)), mf);

}

/* -------------- SetLayout -------------------- */
static void
go_wmf_mr20 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{

}

/* -------------- DeleteObject -------------------- */
static void
go_wmf_mr21 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	const guint8  *data = {0};
	int idx;

	data = gsf_input_read (input, 2, NULL);
	idx = GSF_LE_GET_GUINT16 (data);
	g_hash_table_remove (pg->mfobjs, GINT_TO_POINTER (idx));
	gsf_input_seek (input, -2, G_SEEK_CUR);
}

/*  -------------- CreatePatternBrush -------------------- */
static void
go_wmf_mr22 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	/* FIXME: add pixbufloader */
	GOWmfMFobj *mf;

	mf = malloc (sizeof (GOWmfMFobj));
	mf->type = 2; /* brush */
	g_hash_table_insert ((*pg).mfobjs, GINT_TO_POINTER (go_wmf_find_obj (pg)), mf);
}

/*  -------------- SetBKColor -------------------- */
static void
go_wmf_mr23 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	GOWmfDC *dc;

	dc = &g_array_index (pg->dcs, GOWmfDC, pg->curdc);
	go_wmf_read_color (input, &dc->bkclr);
	gsf_input_seek (input, -3, G_SEEK_CUR);
}

/*  -------------- SetTextColor -------------------- */
static void
go_wmf_mr24 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	GOWmfDC *dc;

	dc = &g_array_index (pg->dcs, GOWmfDC, pg->curdc);
	go_wmf_read_color (input, &dc->txtclr);
	gsf_input_seek (input, -3, G_SEEK_CUR);
}

static void
go_wmf_mr25 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
/*	g_print("SetTextJustification "));*/
}

/* ---------------- SetWindowOrg ----------------- */
static void
go_wmf_mr26 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	GOWmfDC *dc;

	dc = &g_array_index (pg->dcs, GOWmfDC, pg->curdc);
	go_wmf_read_point (input, &dc->y, &dc->x);
	gsf_input_seek (input, -4, G_SEEK_CUR);
}

/*  ---------------- SetWindowExt ------------------- */
static void
go_wmf_mr27 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	GOWmfDC *dc;

	dc = &g_array_index (pg->dcs, GOWmfDC, pg->curdc);
	go_wmf_read_point (input, &dc->Wy, &dc->Wx);
	gsf_input_seek (input, -4, G_SEEK_CUR);
	if (2 == pg->type) {
		if(pg->w*dc->Wy / dc->Wx > pg->h) {
			dc->VPy = pg->h;
			dc->VPx = pg->h * dc->Wx / dc->Wy;
		} else {
			dc->VPx = pg->w;
			dc->VPy = pg->w * dc->Wy / dc->Wx;
		}
		pg->zoom = dc->VPx / dc->Wx;
	}
}

/*  ----------------- SetViewportOrg ------------------- */
static void
go_wmf_mr28 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	GOWmfDC *dc;

	dc = &g_array_index (pg->dcs, GOWmfDC, pg->curdc);
	go_wmf_read_point (input, &dc->VPOy, &dc->VPOx);
	gsf_input_seek (input, -4, G_SEEK_CUR);
}

/*  ----------------- SetViewportExt -------------------- */
static void
go_wmf_mr29 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	GOWmfDC *dc;

	dc = &g_array_index (pg->dcs, GOWmfDC, pg->curdc);
	go_wmf_read_point (input, &dc->VPy, &dc->VPx);
	gsf_input_seek (input, -4, G_SEEK_CUR);
}

/* ------------------- OffsetWindowOrg ------------------ */
static void
go_wmf_mr30 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	GOWmfDC *dc;
	double x,y;

	dc = &g_array_index (pg->dcs, GOWmfDC, pg->curdc);
	go_wmf_read_point (input, &x, &y);
	dc->x += x;
	dc->y += y;
	gsf_input_seek (input, -4, G_SEEK_CUR);
}

/* ------------------- OffsetViewportOrg ---------------- */
static void
go_wmf_mr31 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	GOWmfDC *dc;
	double x,y;

	dc = &g_array_index (pg->dcs, GOWmfDC, pg->curdc);
	go_wmf_read_point (input, &x, &y);
	dc->VPx += x;
	dc->VPy += y;
	gsf_input_seek (input, -4, G_SEEK_CUR);
}

/*  ------------------ LineTo -------------------- */
static void
go_wmf_mr32 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	double x,y;
	GocItem *gocitem;
	GOWmfDC *dc;

	dc = &g_array_index (pg->dcs, GOWmfDC, pg->curdc);
	go_wmf_read_point (input, &y, &x);
	gsf_input_seek (input, -4, G_SEEK_CUR);
	go_wmf_mr_convcoord (&x, &y, pg);
	gocitem = goc_item_new (group, GOC_TYPE_LINE,
		"x0", dc->curx, "y0", dc->cury, "x1", x, "y1", y, NULL);
	dc->curx = x;
	dc->cury = y;
	go_wmf_stroke (pg, gocitem);
}

/*  ------------------ MoveTo -------------------- */
static void
go_wmf_mr33 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	double x,y;
	GOWmfDC *dc;

	dc = &g_array_index (pg->dcs, GOWmfDC, pg->curdc);
	go_wmf_read_point (input, &y, &x);
	gsf_input_seek (input, -4, G_SEEK_CUR);
	go_wmf_mr_convcoord (&x, &y, pg);
	dc->curx = x;
	dc->cury = y;
}

static void
go_wmf_mr34 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
/*	g_print("OffsetClipRgn "));*/
}

static void
go_wmf_mr35 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
/*	g_print("FillRegion "));*/
}

static void
go_wmf_mr36 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
/*	g_print("SetMapperFlags "));*/
}

static void
go_wmf_mr37 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
/*	g_print("SelectPalette "));*/
}

/*  ------------------ CreatePenIndirect ------------------- */
static void
go_wmf_mr38 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	const guint8  *data = {0};
	GOWmfPen *pen;
	GOWmfMFobj *mf;

	pen = malloc (sizeof (GOWmfPen));
	data = gsf_input_read (input, 2, NULL);
	pen->style = GSF_LE_GET_GUINT16 (data);
	data = gsf_input_read (input, 2, NULL);
	pen->width = GSF_LE_GET_GINT16 (data);
	gsf_input_seek (input, 2, G_SEEK_CUR); /* skip "height" */
	go_wmf_read_color (input, &pen->clr);
	gsf_input_seek (input, 1, G_SEEK_CUR);
	data = gsf_input_read (input, 2, NULL);
	pen->flag = GSF_LE_GET_GUINT16 (data);
	gsf_input_seek (input, -12, G_SEEK_CUR);
	mf = malloc (sizeof (GOWmfMFobj));
	mf->type = 1;
	mf->values = pen;
	g_hash_table_insert ((*pg).mfobjs, GINT_TO_POINTER (go_wmf_find_obj (pg)), mf);
}


/* ----------------- CreateFontIndirect ------------- */
static void
go_wmf_mr39 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	const guint8  *data = {0};
	GOWmfFont *font;
	GOWmfMFobj *mf;
	int i = 0;

	font = malloc (sizeof (GOWmfFont));
	data = gsf_input_read (input, 2, NULL);
	font->size = GSF_LE_GET_GINT16 (data);
	gsf_input_seek (input, 2, G_SEEK_CUR); /* skip 'width' */
	data = gsf_input_read (input, 2, NULL);
	font->escape = GSF_LE_GET_GINT16 (data);
	data = gsf_input_read (input, 2, NULL);
	font->orient = GSF_LE_GET_GINT16 (data);
	data = gsf_input_read (input, 2, NULL);
	font->weight = GSF_LE_GET_GINT16 (data);
	data = gsf_input_read (input, 1, NULL);
	font->italic = GSF_LE_GET_GUINT8 (data);
	data = gsf_input_read (input, 1, NULL);
	font->under = GSF_LE_GET_GUINT8 (data);
	data = gsf_input_read (input, 1, NULL);
	font->strike = GSF_LE_GET_GUINT8 (data);
	data = gsf_input_read (input, 1, NULL);
/*
77:'MAC_CHARSET',
*/
	switch (GSF_LE_GET_GUINT8(data)) {
	case 0:
	case 1:
		font->charset = g_strdup ("iso8859-1");
		break;
	case 2:
		font->charset = g_strdup ("Symbol");
		break;
	case 128:
		font->charset = g_strdup ("Shift_JIS");
		break;
	case 129:
		font->charset = g_strdup ("cp949");
		break;
	case 134:
		font->charset = g_strdup ("gb2312");
		break;
	case 136:
		font->charset = g_strdup ("Big5");
		break;
	case 161:
		font->charset = g_strdup ("cp1253");
		break;
	case 162:
		font->charset = g_strdup ("cp1254");
		break;
	case 163:
		font->charset = g_strdup ("cp1258");
		break;
	case 177:
		font->charset = g_strdup ("cp1255");
		break;
	case 178:
		font->charset = g_strdup ("cp1256");
		break;
	case 186:
		font->charset = g_strdup ("cp1257");
		break;
	case 204:
		font->charset = g_strdup ("cp1251");
		break;
	case 222:
		font->charset = g_strdup ("cp874");
		break;
	case 238:
		font->charset = g_strdup ("cp1250");
		break;
	case 255:
		font->charset = g_strdup ("cp437");
		break;
	default:
		font->charset = g_strdup ("cp437");
	}

	gsf_input_seek (input, 4, G_SEEK_CUR);
	data = gsf_input_read (input, 1, NULL);
	while (0 != data[0] && (guint) i < rsize * 2 - 6) {
		i++;
		data = gsf_input_read (input, 1, NULL);
	}
	gsf_input_seek (input, -i - 1, G_SEEK_CUR);
	font->name = malloc (i + 1);
	gsf_input_read (input, i + 1, font->name);
	gsf_input_seek (input, -19 - i, G_SEEK_CUR);
	mf = malloc (sizeof (GOWmfMFobj));
	mf->type = 3;
	mf->values = font;
	g_hash_table_insert ((*pg).mfobjs, GINT_TO_POINTER (go_wmf_find_obj (pg)), mf);
}

/* ---------------- CreateBrushIndirect --------------- */
static void
go_wmf_mr40 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	const guint8 *data = {0};
	GOWmfBrush *brush;
	GOWmfMFobj *mf;

	brush = malloc (sizeof (GOWmfBrush));
	data = gsf_input_read (input, 2, NULL);
	brush->style = GSF_LE_GET_GUINT16 (data);
	go_wmf_read_color (input, &brush->clr);
	gsf_input_seek (input, 1, G_SEEK_CUR); /* skip "clr.a" */
	data = gsf_input_read (input, 2, NULL);
	brush->hatch = GSF_LE_GET_GUINT16 (data);
	gsf_input_seek (input, -8, G_SEEK_CUR);
	mf = malloc (sizeof (GOWmfMFobj));
	mf->type = 2;
	mf->values = brush;
	g_hash_table_insert ((*pg). mfobjs, GINT_TO_POINTER (go_wmf_find_obj (pg)), mf);
}

static void
go_wmf_mr_poly (GsfInput* input, GOWmfPage* pg, GocGroup *group, int type)
{
	const guint8  *data = {0};
	double len;
	guint i;
	GocItem *gocitem;
	GocPoints *points;
	GOWmfDC *dc;

	dc = &g_array_index (pg->dcs, GOWmfDC, pg->curdc);
	data = gsf_input_read (input, 2, NULL);
	len = GSF_LE_GET_GINT16 (data);
	points = goc_points_new (len);
	for (i = 0; i < len; i++) {
		go_wmf_read_point (input, &points->points[i].x, &points->points[i].y);
		go_wmf_mr_convcoord (&points->points[i].x, &points->points[i].y, pg);
	}

	if (0 == type)
		gocitem = goc_item_new (group, GOC_TYPE_POLYGON, "points", points, "fill-rule", dc->pfm, NULL);
	else
		gocitem = goc_item_new (group, GOC_TYPE_POLYLINE, "points", points, NULL);
	go_wmf_fill (pg,gocitem);
	go_wmf_stroke (pg,gocitem);
	gsf_input_seek (input, -len * 4 - 2, G_SEEK_CUR);
}

/*  ---------- Polygon ---------------- */
static void
go_wmf_mr41 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	go_wmf_mr_poly (input, pg, group, 0);
}

/*  ---------- Polyline ---------------- */
static void
go_wmf_mr42 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	go_wmf_mr_poly (input, pg, group, 1);
}

static void
go_wmf_mr43 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
/*	g_print("ScaleWindowExtEx "));*/
}

static void
go_wmf_mr44 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
/*	g_print("ScaleViewportExt "));*/
}

static void
go_wmf_mr45 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
/*	g_print("ExcludeClipRect "));*/
}

static void
go_wmf_mr46 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
/*	g_print("IntersectClipRect "));*/
}

static void
go_wmf_mr_rect (GsfInput* input, GOWmfPage* pg, GocGroup *group, int type)
{
	double x1, x2, y1, y2, tx, ty, rx = 0, ry = 0;
	GocItem *gocitem;

	if (15 == type) {
		go_wmf_read_point (input, &ry, &rx);
		ry = abs (ry / 2);
		rx = abs (rx / 2);
		go_wmf_mr_convcoord (&rx, &ry, pg);
	}
	go_wmf_read_point (input, &y2, &x2);
	go_wmf_read_point (input, &y1, &x1);
	go_wmf_mr_convcoord (&x1, &y1, pg);
	go_wmf_mr_convcoord (&x2, &y2, pg);

	tx = x2 - x1;
	if (x1 > x2) {
		tx = x1;	x1 = x2;	tx -= x1;
	}
	ty = y2 - y1;
	if (y1 > y2 ) {
		ty = y1; y1 = y2; ty -= y1;
	}

	if (1 == type)
		gocitem = goc_item_new (group, GOC_TYPE_ELLIPSE,
			"height", (double) ty, "x", (double) x1, "y", (double) y1, "width", (double) tx, NULL);
	else
		gocitem = goc_item_new (group, GOC_TYPE_RECTANGLE,
			"height", (double) ty, "x", (double) x1, "y", (double) y1, "width",
			(double) tx, "rx", (double) rx, "ry", (double) ry, "type", type, NULL);
	go_wmf_fill (pg, gocitem);
	go_wmf_stroke (pg, gocitem);
	gsf_input_seek (input, -8, G_SEEK_CUR);
}

/* ----------------- Ellipse --------------- */
static void
go_wmf_mr47 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	go_wmf_mr_rect (input, pg, group, 1);
}

static void
go_wmf_mr48 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
/*	g_print("FloodFill "));*/
}

/*  ---------------- Rectangle -------------- */
static void
go_wmf_mr49 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	go_wmf_mr_rect (input, pg, group, 0);
}

static void
go_wmf_mr50 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
/*	g_print("SetPixel "));*/
}

static void
go_wmf_mr51 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
/*	g_print("FrameRegion "));*/
}

static void
go_wmf_mr52 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
/*	g_print("AnimatePalette "));*/
}

/*---------------- TextOut -------------------- */
static void
go_wmf_mr53 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	const guint8  *data = {0};
	char *txt;
	double x, y;
	int len, shift = 0;
	GOAnchorType anchor = GO_ANCHOR_NORTH_WEST;

	go_wmf_set_anchor (pg, &anchor);
	data = gsf_input_read (input, 2, NULL);
	len = GSF_LE_GET_GINT16 (data);
	txt = malloc (sizeof (char) * len + 1);
	gsf_input_read (input, len, txt);
	if (len % 2) {
		gsf_input_seek (input, 1, G_SEEK_CUR);
		shift = 1;
	}
	go_wmf_set_align (input, pg, &x, &y);
	gsf_input_seek (input, -6 - len - shift, G_SEEK_CUR);
	go_wmf_set_text (pg, group, txt, len, &anchor, &x, &y);
}

/*  ------------ PolyPolygon ------------ */
static void
go_wmf_mr54 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	const guint8  *data = {0};
	double x, y;
	int npoly, len, curpos, sumlen = 0;
	int i, j;
	GocItem *gocitem;
	GocIntArray *array;
	GocPoints *points;
	GOWmfDC *dc;

	dc = &g_array_index (pg->dcs, GOWmfDC, pg->curdc);
	data = gsf_input_read (input, 2, NULL);
	npoly = GSF_LE_GET_GINT16 (data);
	curpos = gsf_input_tell (input);
	array = goc_int_array_new (npoly);

	for (j = 0; j < npoly; j++) {
		gsf_input_seek (input, curpos + j * 2, G_SEEK_SET);
		data = gsf_input_read (input, 2, NULL);
		len = GSF_LE_GET_GINT16 (data);
		array->vals[j] = len;
		sumlen += len;
	}
	points = goc_points_new (sumlen);

	for ( i = 0; i < sumlen; i++) {
			go_wmf_read_point (input, &x, &y);
			go_wmf_mr_convcoord (&x, &y, pg);
			points->points[i].x = x;
			points->points[i].y = y;
	}

	gocitem = goc_item_new (group, GOC_TYPE_POLYGON,
		"points", points, "sizes", array, "fill-rule", dc->pfm, NULL);
	go_wmf_fill (pg, gocitem);
	go_wmf_stroke (pg, gocitem);
	gsf_input_seek (input, curpos - 2, G_SEEK_SET);
}

static void
go_wmf_mr55 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
/*	g_print("ExtFloodFill "));*/
}

/*  ---------------- RoundRect ---------------------- */
static void
go_wmf_mr56 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	go_wmf_mr_rect (input, pg, group, 15);
	gsf_input_seek (input, -4, G_SEEK_CUR);
}

static void
go_wmf_mr57 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
/*	g_print("PatBLT "));*/
}

/* ------------------ Escape ------------------------ */
static void
go_wmf_mr58 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{

}

/* ------------------ CreateRegion ------------------ */
static void
go_wmf_mr59 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	/* FIXME: do actual parsing */
	GOWmfMFobj *mf;

	mf = malloc (sizeof (GOWmfMFobj));
	mf->type = 4;
	g_hash_table_insert ((*pg).mfobjs, GINT_TO_POINTER (go_wmf_find_obj (pg)), mf);
}

static void
go_wmf_mr_arc (GsfInput* input, GOWmfPage* pg, GocGroup *group, int type)
{
	GOWmfArc *arc;
	GocItem *gocitem;
	double a1, a2, xc, yc, rx, ry;

	arc = malloc (sizeof (GOWmfArc));
	go_wmf_read_point (input, &arc->ye, &arc->xe);
	go_wmf_read_point (input, &arc->ys, &arc->xs);
	go_wmf_read_point (input, &arc->b, &arc->r);
	go_wmf_read_point (input, &arc->t, &arc->l);
	gsf_input_seek (input, -16, G_SEEK_CUR);
	go_wmf_mr_convcoord (&arc->xe, &arc->ye, pg);
	go_wmf_mr_convcoord (&arc->xs, &arc->ys, pg);
	go_wmf_mr_convcoord (&arc->r, &arc->b, pg);
	go_wmf_mr_convcoord (&arc->l, &arc->t, pg);

	xc = (arc->l + arc->r) * 0.5;
	yc = (arc->b + arc->t) * 0.5;
	rx = abs ((arc->l - arc->r) * 0.5);
	ry = abs ((arc->b - arc->t) * 0.5);
	a1 = atan2 (yc - arc->ys, arc->xs - xc);
	a2 = atan2 (yc - arc->ye, arc->xe - xc);
	arc->xs = xc;
	arc->ys = yc;
	arc->r = rx;
	arc->t = ry;
	arc->l = a1;
	arc->b = a2;
	gocitem = goc_item_new (group, GOC_TYPE_ARC,
	 "xc", arc->xs, "yc", arc->ys, "xr",arc->r, "yr", arc->t,
	 "ang1",arc->l, "ang2", arc->b,"type", type, NULL);
	go_wmf_stroke (pg, gocitem);
	if (type > 0)
		go_wmf_fill (pg, gocitem);
}

/*  ---------------- Arc ---------------- */
static void
go_wmf_mr60 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	go_wmf_mr_arc (input, pg, group, 0);
}

/*  ----------------- Pie ----------------- */
static void
go_wmf_mr61 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	go_wmf_mr_arc (input, pg, group, 2);
}

/*  ---------------- Chord ------------------ */
static void
go_wmf_mr62 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	go_wmf_mr_arc (input, pg, group, 1);
}

static void
go_wmf_mr63 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
/*	g_print("BitBLT "));*/
}

static void
go_wmf_mr64 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
/*	g_print("DIBBitBLT "));*/
}

/* ----------------- ExtTextOut ---------------- */
static void
go_wmf_mr65 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	const guint8  *data = {0};
	char *txt;
	double x, y;
	int len/*, flag*/;
	GOAnchorType anchor = GO_ANCHOR_SOUTH_WEST;

	go_wmf_set_anchor (pg, &anchor);
	go_wmf_set_align (input, pg, &x, &y);
	data = gsf_input_read (input, 2, NULL);
	len = GSF_LE_GET_GINT16 (data);
	data = gsf_input_read (input, 2, NULL);
	/*flag = GSF_LE_GET_GINT16 (data);*/
	txt = malloc (sizeof (char) * len + 1);
	gsf_input_read (input, len, txt);
	gsf_input_seek (input, -8 - len, G_SEEK_CUR);
	go_wmf_set_text (pg, group, txt, len, &anchor, &x, &y);
}

static void
go_wmf_mr66 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
/*	g_print("StretchBlt "));*/
}

/* ----------------- DIBStretchBlt ----------------------- */
static void
go_wmf_mr67 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
/*	g_print("DIBStretchBlt "));*/
}

static void
go_wmf_mr68 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
/*	g_print("SetDIBtoDEV "));*/
}

/* ---------------- StretchDIB -------------------- */
static void
go_wmf_mr69 (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup *group)
{
	const guint8  *data = {0};
	double x, y, xe, ye, w, h;
	guint32 bmpsize, bmpshift = 54/*, dwROP*/;
	guint16 biDepth;
	GdkPixbufLoader *gpbloader = gdk_pixbuf_loader_new ();
	GdkPixbuf *gpb;
	GError **error = NULL;
	guint8 *tmp;
	gint32 rollback = 6 - rsize * 2;

	data = gsf_input_read (input, 4, NULL);
	/*dwROP = GSF_LE_GET_GUINT32 (data);*/
	gsf_input_seek(input, 10, G_SEEK_CUR); /* src x,y,w,h */
	go_wmf_read_point (input, &h, &w);
	go_wmf_read_point (input, &y, &x);
	gsf_input_seek(input, 14, G_SEEK_CUR);
	data = gsf_input_read (input, 2, NULL);
	biDepth = GSF_LE_GET_GUINT32 (data);
	gsf_input_seek(input, -16, G_SEEK_CUR);
	data = gsf_input_read (input, rsize * 2 - 6 - 22, NULL);
	bmpsize = rsize * 2 - 6 - 12;

	gsf_input_seek (input, rollback, G_SEEK_CUR);

	switch (biDepth) {
	case 1:
		bmpshift = 62;
		break;
	case 2:
		bmpshift = 70;
		break;
	case 4:
		bmpshift = 118;
		break;
	case 8:
		bmpshift = 1078;
		break;
	}

	tmp = malloc (bmpsize);
	tmp[0] = 0x42;
	tmp[1] = 0x4d;
	memcpy (tmp + 2, &bmpsize, 4);
	tmp[6] = 0;
	tmp[7] = 0;
	tmp[8] = 0;
	tmp[9] = 0;
	memcpy (tmp + 10, &bmpshift, 4);
	memcpy (tmp + 14, data, bmpsize - 14);

	gdk_pixbuf_loader_write (gpbloader, tmp, bmpsize, error);
	gdk_pixbuf_loader_close (gpbloader, error);
	gpb = gdk_pixbuf_loader_get_pixbuf (gpbloader);

	xe = w - x;
	ye = h -y;
	go_wmf_mr_convcoord (&x, &y, pg);
	go_wmf_mr_convcoord (&xe, &ye, pg);
	w = xe - x;
	h = ye - y;

	goc_item_new (group, GOC_TYPE_PIXBUF,
	 "height", (double) h, "x", (double) x, "y", (double) y, "width",(double) w, "pixbuf", gpb, NULL);
}

typedef void
(*GOWmfHandler) (GsfInput* input, guint rsize, GOWmfPage* pg, GHashTable* objs, GocGroup* group);

static GOWmfHandler go_wmf_mfrec_dump[70] =
{
	go_wmf_mr0, go_wmf_mr1, go_wmf_mr2, go_wmf_mr3, go_wmf_mr4,
	go_wmf_mr5, go_wmf_mr6, go_wmf_mr7, go_wmf_mr8, go_wmf_mr9,
	go_wmf_mr10, go_wmf_mr11, go_wmf_mr12, go_wmf_mr13, go_wmf_mr14,
	go_wmf_mr15, go_wmf_mr16, go_wmf_mr17, go_wmf_mr18, go_wmf_mr19,
	go_wmf_mr20, go_wmf_mr21, go_wmf_mr22, go_wmf_mr23, go_wmf_mr24,
	go_wmf_mr25, go_wmf_mr26, go_wmf_mr27, go_wmf_mr28, go_wmf_mr29,
	go_wmf_mr30, go_wmf_mr31, go_wmf_mr32, go_wmf_mr33, go_wmf_mr34,
	go_wmf_mr35, go_wmf_mr36, go_wmf_mr37, go_wmf_mr38, go_wmf_mr39,
	go_wmf_mr40, go_wmf_mr41, go_wmf_mr42, go_wmf_mr43, go_wmf_mr44,
	go_wmf_mr45, go_wmf_mr46, go_wmf_mr47, go_wmf_mr48, go_wmf_mr49,
	go_wmf_mr50, go_wmf_mr51, go_wmf_mr52, go_wmf_mr53, go_wmf_mr54,
	go_wmf_mr55, go_wmf_mr56, go_wmf_mr57, go_wmf_mr58, go_wmf_mr59,
	go_wmf_mr60, go_wmf_mr61, go_wmf_mr62, go_wmf_mr63, go_wmf_mr64,
	go_wmf_mr65, go_wmf_mr66, go_wmf_mr67, go_wmf_mr68, go_wmf_mr69
};

static GHashTable *
go_wmf_init_recs (void)
{
	GHashTable	*mrecords;
	gint i =0;
	const gint ridarray[70] =
	{	0, 0x001e, 0x0035, 0x0037, 0x00F7, 0x0102, 0x0103, 0x0104, 0x0105, 0x0106,
		0x0107, 0x0108, 0x0127, 0x012a, 0x012b, 0x012c, 0x012d, 0x012e, 0x0139, 0x0142,
		0x0149, 0x01f0, 0x01f9, 0x0201, 0x0209, 0x020a, 0x020b, 0x020c, 0x020d, 0x020e,
		0x020f, 0x0211, 0x0213, 0x0214, 0x0220, 0x0228, 0x0231, 0x0234, 0x02fa, 0x02fb,
		0x02fc, 0x0324, 0x0325, 0x0410, 0x0412, 0x0415, 0x0416, 0x0418, 0x0419, 0x041b,
		0x041f, 0x0429, 0x0436, 0x0521, 0x0538, 0x0548, 0x061c, 0x061d, 0x0626, 0x06ff,
		0x0817, 0x081a, 0x0830, 0x0922, 0x0940, 0x0a32, 0x0b23, 0x0b41, 0x0d33, 0x0f43};

	mrecords = g_hash_table_new (g_direct_hash, g_direct_equal);
	for (i = 0; i < 70; i++)
		g_hash_table_insert (mrecords, GINT_TO_POINTER (ridarray[i]), GINT_TO_POINTER (i));
	return mrecords;
}

static GHashTable *
go_wmf_init_esc (void)
{
	GHashTable	*escrecords;
	gint i =0;
	const char* escarray[60] = {
		"NEWFRAME", "ABORTDOC", "NEXTBAND", "SETCOLORTABLE", "GETCOLORTABLE",
		"FLUSHOUT", "DRAFTMODE", "QUERYESCSUPPORT", "SETABORTPROC", "STARTDOC",
		"ENDDOC", "GETPHYSPAGESIZE", "GETPRINTINGOFFSET", "GETSCALINGFACTOR",
		"META_ESCAPE_ENHANCED_METAFILE", "SETPENWIDTH", "SETCOPYCOUNT",
		"SETPAPERSOURCE", "PASSTHROUGH", "GETTECHNOLOGY", "SETLINECAP", "SETLINEJOIN",
		"SETMITERLIMIT", "BANDINFO", "DRAWPATTERNRECT","GETVECTORPENSIZE",
		"GETVECTORBRUSHSIZE", "ENABLEDUPLEX", "GETSETPAPERBINS", "GETSETPRINTORIENT",
		"ENUMPAPERBINS", "SETDIBSCALING", "EPSPRINTING", "ENUMPAPERMETRICS",
		"GETSETPAPERMETRICS", "POSTSCRIPT_DATA", "POSTSCRIPT_IGNORE", "GETDEVICEUNITS",
		"GETEXTENDEDTEXTMETRICS", "GETPAIRKERNTABLE", "EXTTEXTOUT", "GETFACENAME",
		"DOWNLOADFACE", "METAFILE_DRIVER", "QUERYDIBSUPPORT", "BEGIN_PATH",
		"CLIP_TO_PATH", "END_PATH", "OPEN_CHANNEL", "DOWNLOADHEADER", "CLOSE_CHANNEL",
		"POSTSCRIPT_PASSTHROUGH", "ENCAPSULATED_POSTSCRIPT", "POSTSCRIPT_IDENTIFY",
		"POSTSCRIPT_INJECTION", "CHECKJPEGFORMAT", "CHECKPNGFORMAT",
		"GET_PS_FEATURESETTING", "MXDC_ESCAPE", "SPCLPASSTHROUGH2"};
	const guint16 escid[60] = {
		0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000A,
		0x000B, 0x000C, 0x000D, 0x000E, 0x000F, 0x0010, 0x0011, 0x0012, 0x0013, 0x0014,
		0x0015, 0x0016, 0x0017, 0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E,
		0x001F, 0x0020, 0x0021, 0x0022, 0x0023, 0x0025, 0x0026, 0x002A, 0x0100, 0x0102,
		0x0200, 0x0201, 0x0202, 0x0801, 0x0C01, 0x1000, 0x1001, 0x1002, 0x100E, 0x100F,
		0x1010, 0x1013, 0x1014, 0x1015, 0x1016, 0x1017, 0x1018, 0x1019, 0x101A, 0x11D8};

	escrecords = g_hash_table_new (g_direct_hash, g_direct_equal);
	for (i = 0; i < 60; i++)
		g_hash_table_insert (escrecords, GINT_TO_POINTER (escid[i]), GINT_TO_POINTER (escarray[i]));
	return escrecords;
}

/*****************************************************************************
 * EMF parsing code
 *****************************************************************************/

static void go_emf_dc_free (GOEmfDC *dc)
{
	if (dc->style)
		g_object_unref (dc->style);
	if (dc->font)
		go_font_unref (dc->font);
	g_free (dc);
}

static void
go_emf_convert_coords (GOEmfState *state, double *x, double *y)
{
	*x = (*x - state->dx) * state->dw / state->ww;
	*y = (*y - state->dy) * state->dh / state->wh;
}

#endif

static gboolean
go_emf_header (GOEmfState *state)
{
	guint32 signature = GSF_LE_GET_GINT32 (state->data + 32);
	cairo_matrix_t m;
	switch (signature) {
	case 0x464D4520:
		state->is_emf = TRUE;
		d_(("Enhanced Metafile\n"));
		break;
	default:
		state->is_emf = FALSE;
		d_(("Unknown type, crossing fingers\n"));
		break;
	}

	d_(("\theader with %u bytes\n", state->length));
	go_wmf_read_rectl (&state->dubounds, state->data);
	go_wmf_read_rectl (&state->mmbounds, state->data + 16);
	d_(("\tbounds are (mm): left=%g top=%g right=%g bottom=%g\n",
	    (double) state->mmbounds.left / 100., (double) state->mmbounds.top / 100.,
	    (double) state->mmbounds.right / 100., (double) state->mmbounds.bottom / 100.));
	cairo_matrix_init_scale (&m,
	                         ((double) (state->mmbounds.right - state->mmbounds.left)) / (state->dubounds.right - state->dubounds.left) / 2540. * 72.,
	                         ((double) (state->mmbounds.bottom - state->mmbounds.top)) / (state->dubounds.bottom - state->dubounds.top) / 2540. * 72.);
	m.x0 = -(double) state->mmbounds.left / 2540. * 72.;
	m.y0 = -(double) state->mmbounds.top / 2540. * 72.;
	goc_item_set_transform (GOC_ITEM (state->group), &m);

	return TRUE;
}

#ifdef GOFFICE_EMF_SUPPORT

static gboolean
go_emf_polybezier (GOEmfState *state)
{
	unsigned count, n, offset;
	double x0, y0, x1, y1, x2, y2;
	GOPath *path;
#ifdef DEBUG_EMF_SUPPORT
	GOWmfRectL rect;
#endif
	d_(("polybezier\n"));
#ifdef DEBUG_EMF_SUPPORT
	go_wmf_read_rectl (&rect, state->data);
#endif
	d_(("\tbounds: left: %u; right: %u; top:%u bottom:%u\n",
	    rect.left,rect.right, rect.top, rect.bottom));
	count = GSF_LE_GET_GUINT32 (state->data + 16);
	d_(("\tfound %u points\n", count));
	if (count % 3 != 1)
		return FALSE;
	path = go_path_new ();
	count /= 3;
	go_wmf_read_pointl (state->data + 20, &x0, &y0);
	go_emf_convert_coords (state, &x0, &y0);
	d_(("\tmove to x0=%g y0=%g\n", x0, y0));
	go_path_move_to (path, x0, y0);
	for (n = 0, offset = 28; n < count; n++, offset += 24) {
		go_wmf_read_pointl (state->data + offset, &x0, &y0);
		go_emf_convert_coords (state, &x0, &y0);
		go_wmf_read_pointl (state->data + offset + 8, &x1, &y1);
		go_emf_convert_coords (state, &x1, &y1);
		go_wmf_read_pointl (state->data + offset + 16, &x2, &y2);
		go_emf_convert_coords (state, &x2, &y2);
		go_path_curve_to (path, x0, y0, x1, y1, x2, y2);
		d_(("\tcurve to x0=%g y0=%g x1=%g y1=%g x2=%g y2=%g\n", x0, y0, x1, y1, x2, y2));
	}
	goc_item_set_transform (goc_item_new (state->curDC->group, GOC_TYPE_PATH,
	                                      "path", path,
	                                      "style", state->curDC->style,
	                                      "closed", FALSE,
	                                      NULL),
	                        &state->curDC->m);
	go_path_free (path);
	return TRUE;
}

static gboolean
go_emf_polygon (GOEmfState *state)
{
	unsigned nb_pts, n, offset;
#ifdef DEBUG_EMF_SUPPORT
	GOWmfRectL rect;
#endif
	GocPoints *points;
	double x, y;
	d_(("polygon\n"));
#ifdef DEBUG_EMF_SUPPORT
	go_wmf_read_rectl (&rect, state->data);
#endif
	d_(("\tbounds: left: %u; right: %u; top:%u bottom:%u\n",
	    rect.left,rect.right, rect.top, rect.bottom));
	nb_pts = GSF_LE_GET_GUINT32 (state->data + 16);
	d_(("\t%u points\n", nb_pts));
	points = goc_points_new (nb_pts);
	for (n = 0, offset = 20; n < nb_pts; n++, offset += 8) {
		go_wmf_read_pointl (state->data + offset, &x, &y);
		go_emf_convert_coords (state, &x, &y);
		points->points[n].x = x;
		points->points[n].y = y;
		d_(("\tpoint #%u at x=%g y=%g\n", n, x, y));
	}
	goc_item_new (state->curDC->group, GOC_TYPE_POLYGON,
	              "points", points,
	              "style", state->curDC->style,
	              NULL);
	return TRUE;
}

static gboolean
go_emf_polyline (GOEmfState *state)
{
	unsigned nb_pts, n, offset;
#ifdef DEBUG_EMF_SUPPORT
	GOWmfRectL rect;
#endif
	GocPoints *points;
	double x, y;
	d_(("polyline\n"));
#ifdef DEBUG_EMF_SUPPORT
	go_wmf_read_rectl (&rect, state->data);
#endif
	d_(("\tbounds: left: %u; right: %u; top:%u bottom:%u\n",
	    rect.left,rect.right, rect.top, rect.bottom));
	nb_pts = GSF_LE_GET_GUINT32 (state->data + 16);
	d_(("\t%u points\n", nb_pts));
	points = goc_points_new (nb_pts);
	for (n = 0, offset = 20; n < nb_pts; n++, offset += 8) {
		go_wmf_read_pointl (state->data + offset, &x, &y);
		go_emf_convert_coords (state, &x, &y);
		points->points[n].x = x;
		points->points[n].y = y;
		d_(("\tpoint #%u at x=%g y=%g\n", n, x, y));
	}
	goc_item_new (state->curDC->group, GOC_TYPE_POLYLINE,
	              "points", points,
	              "style", state->curDC->style,
	              NULL);
	return TRUE;
}

static gboolean
go_emf_polybezierto (GOEmfState *state)
{
	unsigned count, n, offset;
	double x0, y0, x1, y1, x2, y2;
#ifdef DEBUG_EMF_SUPPORT
	GOWmfRectL rect;
#endif
	d_(("polybezierto\n"));
#ifdef DEBUG_EMF_SUPPORT
	go_wmf_read_rectl (&rect, state->data);
#endif
	d_(("\tbounds: left: %u; right: %u; top:%u bottom:%u\n",
	    rect.left,rect.right, rect.top, rect.bottom));
	count = GSF_LE_GET_GUINT32 (state->data + 16);
	d_(("\tfound %u points\n", count));
	if (count % 3 != 0)
		return FALSE;
	count /= 3;
	for (n = 0, offset = 20; n < count; n++, offset += 24) {
		go_wmf_read_pointl (state->data + offset, &x0, &y0);
		go_emf_convert_coords (state, &x0, &y0);
		go_wmf_read_pointl (state->data + offset + 8, &x1, &y1);
		go_emf_convert_coords (state, &x1, &y1);
		go_wmf_read_pointl (state->data + offset + 16, &x2, &y2);
		go_emf_convert_coords (state, &x2, &y2);
		go_path_curve_to (state->curDC->path, x0, y0, x1, y1, x2, y2);
		d_(("\tcurve to x0=%g y0=%g x1=%g y1=%g x2=%g y2=%g\n", x0, y0, x1, y1, x2, y2));
	}
	return TRUE;
}

static gboolean
go_emf_polylineto (GOEmfState *state)
{
	unsigned count, n, offset;
	double x, y;
#ifdef DEBUG_EMF_SUPPORT
	GOWmfRectL rect;
#endif
	d_(("polylineto\n"));
#ifdef DEBUG_EMF_SUPPORT
	go_wmf_read_rectl (&rect, state->data);
#endif
	d_(("\tbounds: left: %u; right: %u; top:%u bottom:%u\n",
	    rect.left,rect.right, rect.top, rect.bottom));
	count = GSF_LE_GET_GUINT32 (state->data + 16);
	d_(("\tfound %u points\n", count));
	for (n = 0, offset = 20; n < count; n++, offset += 8) {
		go_wmf_read_pointl (state->data + offset, &x, &y);
		go_emf_convert_coords (state, &x, &y);
		go_path_line_to (state->curDC->path, x, y);
		d_(("\tline to x=%g y0=%g\n", x, y));
	}
	return TRUE;
}

static gboolean
go_emf_polypolyline (GOEmfState *state)
{
	unsigned nb_lines, nb_pts, n, l, i, nm, offset, loffset;
	double x, y;
	GocPoints *points;
#ifdef DEBUG_EMF_SUPPORT
	GOWmfRectL rect;
#endif
	d_(("polypolyline\n"));
#ifdef DEBUG_EMF_SUPPORT
	go_wmf_read_rectl (&rect, state->data);
#endif
	d_(("\tbounds: left: %u; right: %u; top:%u bottom:%u\n",
	    rect.left,rect.right, rect.top, rect.bottom));
	nb_lines = GSF_LE_GET_GUINT32 (state->data + 16);
	nb_pts = GSF_LE_GET_GUINT32 (state->data + 20);
	d_(("\t%u points in %u lines\n", nb_pts, nb_lines));
	points = goc_points_new (nb_pts + nb_lines - 1);
	offset = 24 + 4 * nb_lines;
	loffset = 24;
	i = 0;
	l = 0;
	while (1) {
		nm = GSF_LE_GET_GUINT32 (state->data + loffset);
		loffset += 4;
		for (n = 0; n < nm; n++) {
			go_wmf_read_pointl (state->data + offset, &x, &y);
			go_emf_convert_coords (state, &x, &y);
			points->points[i].x = x;
			points->points[i++].y = y;
			d_(("\tpoint #%u at x=%g y=%g\n", n, x, y));
		}
		l++;
		if (l == nb_lines)
			break;
		points->points[i].x = go_nan;
		points->points[i++].y = go_nan;
	}
	goc_item_new (state->curDC->group, GOC_TYPE_POLYGON,
	              "points", points,
	              "style", state->curDC->style,
	              NULL);
	goc_points_unref (points);
	return TRUE;
}

static gboolean
go_emf_polypolygon (GOEmfState *state)
{
	unsigned nb_polygons, nb_pts, n, offset;
	GocPoints *points;
	GocIntArray *sizes;
	double x, y;
#ifdef DEBUG_EMF_SUPPORT
	GOWmfRectL rect;
#endif
	d_(("polypolygon\n"));
#ifdef DEBUG_EMF_SUPPORT
	go_wmf_read_rectl (&rect, state->data);
#endif
	d_(("\tbounds: left: %u; right: %u; top:%u bottom:%u\n",
	    rect.left,rect.right, rect.top, rect.bottom));
	nb_polygons = GSF_LE_GET_GUINT32 (state->data + 16);
	nb_pts = GSF_LE_GET_GUINT32 (state->data + 20);
	d_(("\t%u points in %u polygons\n", nb_pts, nb_polygons));
	sizes = goc_int_array_new (nb_polygons);
	points = goc_points_new (nb_pts);
	for (n = 0, offset = 24; n < nb_polygons; n++, offset += 4) {
		sizes->vals[n] = GSF_LE_GET_GUINT32 (state->data + offset);
		d_(("\tfound size %u\n", sizes->vals[n]));
	}
	for (n = 0; n < nb_pts; n++, offset += 8) {
		go_wmf_read_pointl (state->data + offset, &x, &y);
		go_emf_convert_coords (state, &x, &y);
		points->points[n].x = x;
		points->points[n].y = y;
		d_(("\tpoint #%u at x=%g y=%g\n", n, x, y));
	}
	goc_item_new (state->curDC->group, GOC_TYPE_POLYGON,
	              "points", points,
	              "sizes", sizes,
	              "fill-rule", state->curDC->PolygonFillMode,
	              "style", state->curDC->style,
	              NULL);
	goc_int_array_unref (sizes);
	goc_points_unref (points);
	return TRUE;
}

static gboolean
go_emf_setwindowextex (GOEmfState *state)
{
	d_(("setwindowextex\n"));
	go_wmf_read_pointl (state->data, &state->ww, &state->wh);
	d_(("\twindow size: %g x %g\n", state->ww, state->wh));
	return TRUE;
}

static gboolean
go_emf_setwindoworgex (GOEmfState *state)
{
	d_(("setwindoworgex\n"));
	go_wmf_read_pointl (state->data, &state->wx, &state->wy);
	d_(("\twindow origin: %g x %g\n", state->wx, state->wy));
	return TRUE;
}

static gboolean
go_emf_setviewportextex (GOEmfState *state)
{
	d_(("setviewportextex\n"));
	go_wmf_read_pointl (state->data, &state->dw, &state->dh);
	d_(("\tview port size: %g x %g\n", state->dw, state->dh));
	return TRUE;
}

static gboolean
go_emf_setviewportorgex (GOEmfState *state)
{
	d_(("setviewportorgex\n"));
	go_wmf_read_pointl (state->data, &state->dx, &state->dy);
	d_(("\tview port origin: %g x %g\n", state->dx, state->dy));
	return TRUE;
}

static gboolean
go_emf_setbrushorgex (GOEmfState *state)
{
	d_(("setbrushorgex\n"));
	return TRUE;
}

static gboolean
go_emf_eof (GOEmfState *state)
{
	d_(("eof\n"));
	return TRUE;
}

static gboolean
go_emf_setpixelv (GOEmfState *state)
{
	d_(("setpixelv\n"));
	return TRUE;
}

static gboolean
go_emf_setmapperflags (GOEmfState *state)
{
	d_(("setmapperflags\n"));
	return TRUE;
}

static gboolean
go_emf_setmapmode (GOEmfState *state)
{
#ifdef DEBUG_EMF_SUPPORT
char const *map_modes[] = {
	"MM_TEXT",
"MM_LOMETRIC",
"MM_HIMETRIC",
"MM_LOENGLISH",
"MM_HIENGLISH",
"MM_TWIPS",
"MM_ISOTROPIC",
"MM_ANISOTROPIC"
};
#endif
	d_(("setmapmode\n"));
	state->map_mode = GSF_LE_GET_GUINT32 (state->data);
	d_(("\tmap mode is %s\n", map_modes[state->map_mode-1]));
	return TRUE;
}

static gboolean
go_emf_setbkmode (GOEmfState *state)
{
	d_(("setbkmode\n"));
	state->curDC->fill_bg = GSF_LE_GET_GINT32 (state->data) - 1;
	d_(("\tbackground is %s\n", state->curDC->fill_bg? "filled": "transparent"));
	return TRUE;
}

static gboolean
go_emf_setpolyfillmode (GOEmfState *state)
{
	d_(("setpolyfillmode\n"));
	state->curDC->PolygonFillMode = GSF_LE_GET_GUINT32 (state->data) == 1;
	d_(("\tpolygon fill mode is %s\n", state->curDC->PolygonFillMode? "alternate": "winding"));
	return TRUE;
}

static gboolean
go_emf_setrop2 (GOEmfState *state)
{
#ifdef DEBUG_EMF_SUPPORT
char const *ops[] = {
NULL,
"R2_BLACK",
"R2_NOTMERGEPEN",
"R2_MASKNOTPEN",
"R2_NOTCOPYPEN",
"R2_MASKPENNOT",
"R2_NOT",
"R2_XORPEN",
"R2_NOTMASKPEN",
"R2_MASKPEN",
"R2_NOTXORPEN",
"R2_NOP",
"R2_MERGENOTPEN",
"R2_COPYPEN",
"R2_MERGEPENNOT",
"R2_MERGEPEN",
"R2_WHITE"
};
#endif
	d_(("setrop2\n"));
	state->curDC->op = GSF_LE_GET_GINT32 (state->data);
	d_(("\toperator is %x i.e. %s\n", state->curDC->op, ops[state->curDC->op]));
	return TRUE;
}

static gboolean
go_emf_setstretchbltmode (GOEmfState *state)
{
	d_(("setstretchbltmode\n"));
	return TRUE;
}

static gboolean
go_emf_settextalign (GOEmfState *state)
{
	int align;
	d_(("settextalign\n"));
	align = GSF_LE_GET_GUINT32 (state->data);
	/* Note: we currently ignore the first bit which tells if current position
	 must be updated after each text output call */
	switch (align & 0x1e) {
	default:
	case 0:
		state->curDC->text_align = GO_ANCHOR_NORTH_WEST;
		break;
	case 2:
		state->curDC->text_align = GO_ANCHOR_NORTH_EAST;
		break;
	case 6:
		state->curDC->text_align = GO_ANCHOR_NORTH;
		break;
	case 8:
		state->curDC->text_align = GO_ANCHOR_SOUTH_WEST;
		break;
	case 0xa:
		state->curDC->text_align = GO_ANCHOR_SOUTH_EAST;
		break;
	case 0xe:
		state->curDC->text_align = GO_ANCHOR_SOUTH;
		break;
	case 18:
		state->curDC->text_align = GO_ANCHOR_BASELINE_WEST;
		break;
	case 0x1a:
		state->curDC->text_align = GO_ANCHOR_BASELINE_EAST;
		break;
	case 0x1e:
		state->curDC->text_align = GO_ANCHOR_BASELINE_CENTER;
		break;
	}
	d_(("\talignment=%04x\n", align));
	return TRUE;
}

static gboolean
go_emf_setcoloradjustment (GOEmfState *state)
{
	d_(("setcoloradjustment\n"));
	return TRUE;
}

static gboolean
go_emf_settextcolor (GOEmfState *state)
{
	d_(("settextcolor\n"));
	state->curDC->text_color = go_wmf_read_gocolor (state->data);
	d_(("\ttext color=%08x\n", state->curDC->text_color));
	return TRUE;
}

static gboolean
go_emf_setbkcolor (GOEmfState *state)
{
	d_(("setbkcolor\n"));
	return TRUE;
}

static gboolean
go_emf_offsetcliprgn (GOEmfState *state)
{
	d_(("offsetcliprgn\n"));
	return TRUE;
}

static gboolean
go_emf_movetoex (GOEmfState *state)
{
	double x, y;
	d_(("movetoex\n"));
	go_wmf_read_pointl (state->data, &x, &y);
	go_emf_convert_coords (state, &x, &y);
	if (state->curDC->path != NULL)
		go_path_move_to (state->curDC->path, x, y);
	else {
		state->curDC->xpos = x;
		state->curDC->ypos = y;
	}
	d_(("\tMove to x=%g y=%g\n", x, y));
	return TRUE;
}

static gboolean
go_emf_setmetargn (GOEmfState *state)
{
	d_(("setmetargn\n"));
	return TRUE;
}

static gboolean
go_emf_excludecliprect (GOEmfState *state)
{
	d_(("excludecliprect\n"));
	return TRUE;
}

static gboolean
go_emf_intersectcliprect (GOEmfState *state)
{
	GOWmfRectL rect;
	GOPath *path;
	d_(("intersectcliprect\n"));
	go_wmf_read_rectl (&rect, state->data);
	d_(("\tclipping rectangle: left=%d top=%d right=%d bottom=%d\n",
	    rect.left, rect.top, rect.right, rect.bottom));
	state->curDC->group = GOC_GROUP (goc_item_new (state->curDC->group, GOC_TYPE_GROUP, NULL));
	path = state->curDC->group->clip_path = go_path_new ();
	/* FIXME adjust the coordinates */
	go_path_rectangle (path, CMM2PTS(rect.left), CMM2PTS(rect.top),
	                   CMM2PTS(rect.right - rect.left),
	                   CMM2PTS(rect.bottom - rect.top));
	return TRUE;
}

static gboolean
go_emf_scaleviewportextex (GOEmfState *state)
{
	d_(("scaleviewportextex\n"));
	return TRUE;
}

static gboolean
go_emf_scalewindowextex (GOEmfState *state)
{
	d_(("scalewindowextex\n"));
	return TRUE;
}

static gboolean
go_emf_savedc (GOEmfState *state)
{
	d_(("savedc\n"));
	state->dc_stack = g_slist_prepend (state->dc_stack, state->curDC);
	state->curDC = g_new (GOEmfDC, 1);
	memcpy (state->curDC, state->dc_stack->data, sizeof (GOEmfDC));
	state->curDC->style = go_style_new ();
	state->curDC->font = NULL;
	return TRUE;
}

static gboolean
go_emf_restoredc (GOEmfState *state)
{
	int n = GSF_LE_GET_GINT32 (state->data);
	d_(("restoredc\n"));
	d_(("\tpoping %d %s\n", -n, n == -1? "context": "contexts"));
	if (n >= 0)
		return FALSE;
	while (n++ < 0) {
		if (state->dc_stack == NULL)
			return FALSE;
		go_emf_dc_free (state->curDC);
		state->curDC = state->dc_stack->data;
		state->dc_stack = g_slist_delete_link (state->dc_stack, state->dc_stack);
	}
	return TRUE;
}

static gboolean
go_emf_setworldtransform (GOEmfState *state)
{
	double m11, m12, m21, m22, dx, dy;
	d_(("setworldtransform\n"));
	m11 = GSF_LE_GET_FLOAT (state->data);
	m12 = GSF_LE_GET_FLOAT (state->data + 4);
	m21 = GSF_LE_GET_FLOAT (state->data + 8);
	m22 = GSF_LE_GET_FLOAT (state->data + 12);
	dx = GSF_LE_GET_FLOAT (state->data + 16);
	dy = GSF_LE_GET_FLOAT (state->data + 20);
	d_(("\tm11 = %g m12 = %g dx=%g\n\tm21 = %g m22=%g dy=%g\n",
	    m11, m12, dx, m21, m22, dy));
	cairo_matrix_init (&state->curDC->m, m11, m12, m21, m22, dx, dy);
	return TRUE;
}

enum {
	GO_MWT_IDENTITY = 0x01,
	GO_MWT_LEFTMULTIPLY = 0x02,
	GO_MWT_RIGHTMULTIPLY = 0x03,
	GO_MWT_SET = 0x04
};

static gboolean
go_emf_modifyworldtransform (GOEmfState *state)
{
#ifdef DEBUG_EMF_SUPPORT
	char const *tmas[5] = {
		"INVALID",
		"MWT_IDENTITY",
		"MWT_LEFTMULTIPLY",
		"MWT_RIGHTMULTIPLY",
		"MWT_SET"
	};
#endif
	double m11, m12, m21, m22, dx, dy;
	unsigned mode;
	cairo_matrix_t m;
	d_(("modifyworldtransform\n"));
	m11 = GSF_LE_GET_FLOAT (state->data);
	m12 = GSF_LE_GET_FLOAT (state->data + 4);
	m21 = GSF_LE_GET_FLOAT (state->data + 8);
	m22 = GSF_LE_GET_FLOAT (state->data + 12);
	dx = GSF_LE_GET_FLOAT (state->data + 16);
	dy = GSF_LE_GET_FLOAT (state->data + 20);
	mode = GSF_LE_GET_GUINT32 (state->data + 24);
	d_(("\tm11 = %g m12 = %g dx=%g\n\tm21 = %g m22=%g dy=%g\n\tmode = %s\n",
	    m11, m12, dx, m21, m22, dy, tmas[(mode < 4)? mode: 0]));
	/* FIXME: do something with it */
	switch (mode) {
	case GO_MWT_IDENTITY:
		cairo_matrix_init_identity (&state->curDC->m);
		break;
	case GO_MWT_LEFTMULTIPLY:
		cairo_matrix_init (&m, m11, m12, m21, m22, dx, dy);
		cairo_matrix_multiply (&state->curDC->m, &m, &state->curDC->m);
		break;
	case GO_MWT_RIGHTMULTIPLY:
		cairo_matrix_init (&m, m11, m12, m21, m22, dx, dy);
		cairo_matrix_multiply (&state->curDC->m, &state->curDC->m, &m);
		break;
	case GO_MWT_SET:
		cairo_matrix_init (&state->curDC->m, m11, m12, m21, m22, dx, dy);
		break;
	}
	return TRUE;
}

enum {
	GO_EMF_WHITE_BRUSH = 0x80000000,
	GO_EMF_LTGRAY_BRUSH = 0x80000001,
	GO_EMF_GRAY_BRUSH = 0x80000002,
	GO_EMF_DKGRAY_BRUSH = 0x80000003,
	GO_EMF_BLACK_BRUSH = 0x80000004,
	GO_EMF_NULL_BRUSH = 0x80000005,
	GO_EMF_WHITE_PEN = 0x80000006,
	GO_EMF_BLACK_PEN = 0x80000007,
	GO_EMF_NULL_PEN = 0x80000008,
	GO_EMF_OEM_FIXED_FONT = 0x8000000A,
	GO_EMF_ANSI_FIXED_FONT = 0x8000000B,
	GO_EMF_ANSI_VAR_FONT = 0x8000000C,
	GO_EMF_SYSTEM_FONT = 0x8000000D,
	GO_EMF_DEVICE_DEFAULT_FONT = 0x8000000E,
	GO_EMF_DEFAULT_PALETTE = 0x8000000F,
	GO_EMF_SYSTEM_FIXED_FONT = 0x80000010,
	GO_EMF_DEFAULT_GUI_FONT = 0x80000011,
	GO_EMF_DC_BRUSH = 0x80000012,
	GO_EMF_DC_PEN = 0x80000013
};

static gboolean
go_emf_selectobject (GOEmfState *state)
{
	GOEmfObject *tool;
	unsigned index;
	d_(("selectobject\n"));
	index = GSF_LE_GET_GUINT32 (state->data);
	if (index < 0x80000000) {
		tool = g_hash_table_lookup (state->mfobjs, GUINT_TO_POINTER (index));
		if (tool == NULL) {
			d_(("\tinvalid object of index %u\n", index));
			return FALSE;
		}
		switch (tool->obj_type) {
		case GO_EMF_OBJ_TYPE_PEN: {
			GOEmfPen *pen = (GOEmfPen *) tool;
			state->curDC->style->interesting_fields |= GO_STYLE_LINE;
			switch (pen->style & 0xff) {
			case 0:
				state->curDC->style->line.dash_type = GO_LINE_SOLID;
				break;
			case 1:
				state->curDC->style->line.dash_type = GO_LINE_DASH;
				break;
			case 2:
				state->curDC->style->line.dash_type = GO_LINE_DOT;
				break;
			case 3:
				state->curDC->style->line.dash_type = GO_LINE_DASH_DOT;
				break;
			case 4:
				state->curDC->style->line.dash_type = GO_LINE_DASH_DOT_DOT;
				break;
			case 5:
				state->curDC->style->line.dash_type = GO_LINE_NONE;
				break;
			case 6: /* FIXME */
			case 7: /* FIXME */
			case 8: /* FIXME */
			default:
				state->curDC->style->line.dash_type = GO_LINE_NONE;
				g_warning ("Invalid pen style");
				break;
			}
			state->curDC->style->line.auto_dash = FALSE;
			state->curDC->style->line.width = pen->width;
			state->curDC->style->line.color = pen->clr;
			state->curDC->style->line.auto_color = FALSE;
			switch ((pen->style & 0xf00) >> 16) {
			case 0:
				state->curDC->style->line.cap = CAIRO_LINE_CAP_ROUND;
				break;
			case 1:
				state->curDC->style->line.cap = CAIRO_LINE_CAP_SQUARE;
				break;
			case 2:
				state->curDC->style->line.cap = CAIRO_LINE_CAP_BUTT;
				break;
			}
			switch ((pen->style & 0xf000) >> 24) {
			case 0:
				state->curDC->style->line.join = CAIRO_LINE_JOIN_ROUND;
				break;
			case 1:
				state->curDC->style->line.join = CAIRO_LINE_JOIN_BEVEL;
				break;
			case 2:
				state->curDC->style->line.join = CAIRO_LINE_JOIN_MITER;
				break;
			}
			d_(("\tselected pen #%u\n", index));
			break;
		}
		case GO_EMF_OBJ_TYPE_BRUSH: {
			GOEmfBrush *brush = (GOEmfBrush *) tool;
			state->curDC->style->interesting_fields |= GO_STYLE_FILL;
			state->curDC->style->fill.pattern.pattern = GO_PATTERN_FOREGROUND_SOLID;
			state->curDC->style->fill.pattern.back = 0;
			state->curDC->style->fill.pattern.fore = brush->clr;
			state->curDC->style->fill.auto_back = FALSE;
			state->curDC->style->fill.auto_fore = FALSE;
			d_(("\tselected brush #%u\n", index));
			break;
		}
		case GO_EMF_OBJ_TYPE_FONT: {
			double rotation;
			GOEmfFont *font = (GOEmfFont *) tool;
			PangoFontDescription *desc = pango_font_description_new ();
			pango_font_description_set_family (desc, font->facename);
			/* FIXME take size sign into account */
			if (font->size != 0) /* what happens if size is 0? */
				pango_font_description_set_size (desc, abs (font->size * PANGO_SCALE));
			if (font->weight != 0)
				pango_font_description_set_weight (desc, font->weight);
			if (font->italic)
				pango_font_description_set_style (desc, PANGO_STYLE_ITALIC);
			if (state->curDC->font != NULL)
				go_font_unref (state->curDC->font);
			state->curDC->font = go_font_new_by_desc (desc);
			rotation = (double) font->escape / 10.;
			if (rotation < 0.)
				rotation += 360.;
			state->curDC->text_rotation = (360. - rotation) / 180. * M_PI;
			/* we actually igonore the glyphs orientation which we suppose to
			 be the same than the text rotation, if not we should display the
			 text glyph per glyph, not using a global layout */
			d_(("\tselected font #%u\n", index));
			break;
		}
		default:
			d_(("\tinvalid object type %u\n", tool->obj_type));
			return FALSE;
		}
	} else switch (index) {
	case GO_EMF_WHITE_BRUSH:
		state->curDC->style->interesting_fields |= GO_STYLE_FILL;
		state->curDC->style->fill.pattern.pattern = GO_PATTERN_FOREGROUND_SOLID;
		state->curDC->style->fill.pattern.back = 0;
		state->curDC->style->fill.pattern.fore = GO_COLOR_WHITE;
		state->curDC->style->fill.auto_back = FALSE;
		state->curDC->style->fill.auto_fore = FALSE;
		d_(("\tWhite brush\n"));
		break;
	case GO_EMF_LTGRAY_BRUSH:
		state->curDC->style->interesting_fields |= GO_STYLE_FILL;
		state->curDC->style->fill.pattern.pattern = GO_PATTERN_FOREGROUND_SOLID;
		state->curDC->style->fill.pattern.back = 0;
		state->curDC->style->fill.pattern.fore = GO_COLOR_GREY (0xc0);
		state->curDC->style->fill.auto_back = FALSE;
		state->curDC->style->fill.auto_fore = FALSE;
		d_(("\tLight gray brush\n"));
		break;
	case GO_EMF_GRAY_BRUSH:
		state->curDC->style->interesting_fields |= GO_STYLE_FILL;
		state->curDC->style->fill.pattern.pattern = GO_PATTERN_FOREGROUND_SOLID;
		state->curDC->style->fill.pattern.back = 0;
		state->curDC->style->fill.pattern.fore = GO_COLOR_GREY (0x80);
		state->curDC->style->fill.auto_back = FALSE;
		state->curDC->style->fill.auto_fore = FALSE;
		d_(("\tGray brush\n"));
		break;
	case GO_EMF_DKGRAY_BRUSH:
		state->curDC->style->interesting_fields |= GO_STYLE_FILL;
		state->curDC->style->fill.pattern.pattern = GO_PATTERN_FOREGROUND_SOLID;
		state->curDC->style->fill.pattern.back = 0;
		state->curDC->style->fill.pattern.fore = GO_COLOR_GREY (0x40);
		state->curDC->style->fill.auto_back = FALSE;
		state->curDC->style->fill.auto_fore = FALSE;
		d_(("\tDark gray brush\n"));
		break;
	case GO_EMF_BLACK_BRUSH:
		state->curDC->style->interesting_fields |= GO_STYLE_FILL;
		state->curDC->style->fill.pattern.pattern = GO_PATTERN_FOREGROUND_SOLID;
		state->curDC->style->fill.pattern.back = 0;
		state->curDC->style->fill.pattern.fore = GO_COLOR_BLACK;
		state->curDC->style->fill.auto_back = FALSE;
		state->curDC->style->fill.auto_fore = FALSE;
		d_(("\tBlack brush\n"));
		break;
	case GO_EMF_NULL_BRUSH:
		state->curDC->style->interesting_fields |= GO_STYLE_FILL;
		state->curDC->style->fill.pattern.pattern = GO_PATTERN_FOREGROUND_SOLID;
		state->curDC->style->fill.pattern.back = 0;
		state->curDC->style->fill.pattern.fore = 0;
		state->curDC->style->fill.auto_back = FALSE;
		state->curDC->style->fill.auto_fore = FALSE;
		d_(("\tNull brush\n"));
		break;
	case GO_EMF_WHITE_PEN:
		state->curDC->style->line.dash_type = GO_LINE_SOLID;
		state->curDC->style->line.color = GO_COLOR_WHITE;
		state->curDC->style->line.width = 0.;
		state->curDC->style->line.auto_dash = FALSE;
		state->curDC->style->line.auto_color = FALSE;
		d_(("\tWhite pen\n"));
		break;
	case GO_EMF_BLACK_PEN:
		state->curDC->style->line.dash_type = GO_LINE_SOLID;
		state->curDC->style->line.color = GO_COLOR_BLACK;
		state->curDC->style->line.width = 0.;
		state->curDC->style->line.auto_dash = FALSE;
		state->curDC->style->line.auto_color = FALSE;
		d_(("\tBlack pen\n"));
		break;
	case GO_EMF_NULL_PEN:
		state->curDC->style->line.dash_type = GO_LINE_NONE;
		state->curDC->style->line.auto_dash = FALSE;
		d_(("\tNull pen\n"));
		break;
	case GO_EMF_OEM_FIXED_FONT:
		d_(("\tEOM fixed font\n"));
		if (state->curDC->font != NULL) {
			go_font_unref (state->curDC->font);
			state->curDC->font = NULL;
		}
		/* FIXME, select a font? */
		break;
	case GO_EMF_ANSI_FIXED_FONT:
		d_(("\tANSI fixed font\n"));
		if (state->curDC->font != NULL) {
			go_font_unref (state->curDC->font);
			state->curDC->font = NULL;
		}
		/* FIXME, select a font? */
		break;
	case GO_EMF_ANSI_VAR_FONT:
		d_(("\tANSI var font\n"));
		if (state->curDC->font != NULL) {
			go_font_unref (state->curDC->font);
			state->curDC->font = NULL;
		}
		/* FIXME, select a font? */
		break;
	case GO_EMF_SYSTEM_FONT:
		d_(("\tSystem font\n"));
		if (state->curDC->font != NULL) {
			go_font_unref (state->curDC->font);
			state->curDC->font = NULL;
		}
		/* FIXME, select a font? */
		break;
	case GO_EMF_DEVICE_DEFAULT_FONT:
		d_(("\tDevice default font\n"));
		if (state->curDC->font != NULL) {
			go_font_unref (state->curDC->font);
			state->curDC->font = NULL;
		}
		/* FIXME, select a font? */
		break;
	case GO_EMF_DEFAULT_PALETTE:
		d_(("\tDefault palette\n"));
		break;
	case GO_EMF_SYSTEM_FIXED_FONT:
		d_(("\tSystem fixed font\n"));
		if (state->curDC->font != NULL) {
			go_font_unref (state->curDC->font);
			state->curDC->font = NULL;
		}
		/* FIXME, select a font? */
		break;
	case GO_EMF_DEFAULT_GUI_FONT:
		d_(("\tDefault GUI font\n"));
		if (state->curDC->font != NULL) {
			go_font_unref (state->curDC->font);
			state->curDC->font = NULL;
		}
		/* FIXME, select a font? */
		break;
	case GO_EMF_DC_BRUSH:
		d_(("\tDC brush\n"));
		break;
	case GO_EMF_DC_PEN:
		d_(("\tDC pen\n"));
		break;
	default:
		break;
	}
	return TRUE;
}

#ifdef DEBUG_EMF_SUPPORT
char const *dashes[] = {
"Solid",
"Dash",
"Dot",
"Dash-dot",
"Dash-dot-dot",
"None",
"Inside frame",
"User style",
"Alternate"
};
char const *join[]= {
"round",
"bevel",
"mitter"
};
char const *cap[]= {
"round",
"square",
"flat"
};
#endif

static gboolean
go_emf_createpen (GOEmfState *state)
{
	unsigned index;
	GOEmfPen *pen = g_new0 (GOEmfPen, 1);
	pen->obj_type = GO_EMF_OBJ_TYPE_PEN;
	d_(("createpen\n"));
	index = GSF_LE_GET_GUINT32 (state->data);
	pen->style = GSF_LE_GET_GUINT32 (state->data + 4);
	pen->width = GSF_LE_GET_GUINT32 (state->data + 8);
	pen->clr = go_wmf_read_gocolor (state->data + 16);
	d_(("\tpen index=%u style=%s width=%g color=%08x join=%s cap=%s%s\n",index,
	    dashes[pen->style&0xf], pen->width, pen->clr,
	    join[(pen->style&0xf000)>>24], cap[(pen->style&0xf00)>>16],
	    pen->style&0x00010000? "Geometric": ""));
	g_hash_table_replace (state->mfobjs, GUINT_TO_POINTER (index), pen);
	return TRUE;
}

enum {
	GO_EMF_HS_HORIZONTAL = 0x0000,
	GO_EMF_HS_VERTICAL = 0x0001,
	GO_EMF_HS_FDIAGONAL = 0x0002,
	GO_EMF_HS_BDIAGONAL = 0x0003,
	GO_EMF_HS_CROSS = 0x0004,
	GO_EMF_HS_DIAGCROSS = 0x0005,
	GO_EMF_HS_SOLIDCLR = 0x0006,
	GO_EMF_HS_DITHEREDCLR = 0x0007,
	GO_EMF_HS_SOLIDTEXTCLR = 0x0008,
	GO_EMF_HS_DITHEREDTEXTCLR = 0x0009,
	GO_EMF_HS_SOLIDBKCLR = 0x000A,
	GO_EMF_HS_DITHEREDBKCLR = 0x000B
};

enum {
	GO_EMF_BS_SOLID = 0x0000,
	GO_EMF_BS_NULL = 0x0001,
	GO_EMF_BS_HATCHED = 0x0002,
	GO_EMF_BS_PATTERN = 0x0003,
	GO_EMF_BS_INDEXED = 0x0004,
	GO_EMF_BS_DIBPATTERN = 0x0005,
	GO_EMF_BS_DIBPATTERNPT = 0x0006,
	GO_EMF_BS_PATTERN8X8 = 0x0007,
	GO_EMF_BS_DIBPATTERN8X8 = 0x0008,
	GO_EMF_BS_MONOPATTERN = 0x0009
};

static gboolean
go_emf_createbrushindirect (GOEmfState *state)
{
#ifdef DEBUG_EMF_SUPPORT
	char const *brushes[] = {
"BS_SOLID",
"BS_NULL",
"BS_HATCHED",
"BS_PATTERN",
"BS_INDEXED",
"BS_DIBPATTERN",
"BS_DIBPATTERNPT",
"BS_PATTERN8X8",
"BS_DIBPATTERN8X8",
"BS_MONOPATTERN"
};
char const *hatches[] = {
"HORIZONTAL",
"HS_VERTICAL",
"HS_FDIAGONAL",
"HS_BDIAGONAL",
"HS_CROSS",
"HS_DIAGCROSS",
"HS_SOLIDCLR",
"HS_DITHEREDCLR",
"HS_SOLIDTEXTCLR",
"HS_DITHEREDTEXTCLR",
"HS_SOLIDBKCLR",
"HS_DITHEREDBKCLR"
};
#endif
	unsigned index;
	GOEmfBrush *brush = g_new0 (GOEmfBrush, 1);
	brush->obj_type = GO_EMF_OBJ_TYPE_BRUSH;
	d_(("createbrushindirect\n"));
	index = GSF_LE_GET_GUINT32 (state->data);
	brush->style = GSF_LE_GET_GUINT32 (state->data + 4);
	brush->clr = go_wmf_read_gocolor (state->data + 8);
	brush->hatch = GSF_LE_GET_GUINT32 (state->data + 12);
	g_hash_table_replace (state->mfobjs, GUINT_TO_POINTER (index), brush);
	d_(("\tbrush index=%u style=%s color=%08x hatch=%s\n",index,
	    brushes[brush->style],brush->clr,hatches[brush->hatch]));
	return TRUE;
}

static gboolean
go_emf_deleteobject (GOEmfState *state)
{
	d_(("deleteobject\n"));
	return TRUE;
}

static gboolean
go_emf_anglearc (GOEmfState *state)
{
	d_(("anglearc\n"));
	return TRUE;
}

static gboolean
go_emf_ellipse (GOEmfState *state)
{
	GOWmfRectL rect;
	double x, y, h, w;
	d_(("ellipse\n"));
	go_wmf_read_rectl (&rect, state->data);
	d_(("\tleft: %u; right: %u; top:%u bottom:%u\n",
	    rect.left, rect.right, rect.top, rect.bottom));
	x = rect.left;
	y = rect.top;
	go_emf_convert_coords (state, &x, &y);
	w = rect.right;
	h = rect.bottom;
	go_emf_convert_coords (state, &w, &h);
	if (x > w) {
		double t = x;
		x = w;
		w = t;
	}
	if (y > h) {
		double t = y;
		y = h;
		h = t;
	}
	goc_item_set_transform (goc_item_new (state->curDC->group, GOC_TYPE_ELLIPSE,
	                                      "x", x, "y", y, "width", w - x,
	                                      "height", h - y,
	                                      "style", state->curDC->style, NULL),
	                        &state->curDC->m);
	return TRUE;
}

static gboolean
go_emf_rectangle (GOEmfState *state)
{
	GOWmfRectL rect;
	double x, y, h, w;
	d_(("rectangle\n"));
	go_wmf_read_rectl (&rect, state->data);
	d_(("\tleft: %u; right: %u; top:%u bottom:%u\n",
	    rect.left, rect.right, rect.top, rect.bottom));
	x = rect.left;
	y = rect.top;
	go_emf_convert_coords (state, &x, &y);
	w = rect.right;
	h = rect.bottom;
	go_emf_convert_coords (state, &w, &h);
	if (x > w) {
		double t = x;
		x = w;
		w = t;
	}
	if (y > h) {
		double t = y;
		y = h;
		h = t;
	}
	goc_item_set_transform (goc_item_new (state->curDC->group, GOC_TYPE_RECTANGLE,
	                                      "x", x, "y", y, "width", w - x,
	                                      "height", h - y,
	                                      "style", state->curDC->style, NULL),
	                        &state->curDC->m);
	return TRUE;
}

static gboolean
go_emf_roundrect (GOEmfState *state)
{
	GOWmfRectL rect;
	double x, y, h, w, rx, ry;
	d_(("roundrect\n"));
	go_wmf_read_rectl (&rect, state->data);
	d_(("\tleft: %u; right: %u; top:%u bottom:%u\n",
	    rect.left, rect.right, rect.top, rect.bottom));
	x = rect.left;
	y = rect.top;
	go_emf_convert_coords (state, &x, &y);
	w = rect.right;
	h = rect.bottom;
	go_emf_convert_coords (state, &w, &h);
	go_wmf_read_pointl (state->data + 16, &rx, &ry);
	go_emf_convert_coords (state, &rx, &ry);
	d_(("\trx: %g; ry: %g\n", rx, ry));
	goc_item_set_transform (goc_item_new (state->curDC->group, GOC_TYPE_RECTANGLE,
	                                      "x", x, "y", y, "width", w - x,
	                                      "height", h - y, "rx", rx, "ry", ry,
	                                      "style", state->curDC->style,
	                                      "type", 15, NULL),
	                        &state->curDC->m);
	return TRUE;
}

static void
_go_emf_arc (GOEmfState *state, unsigned type)
{
	GOWmfRectL rect;
	double x0, y0, x1, y1, xc, yc;
	go_wmf_read_rectl (&rect, state->data);
	d_(("\tleft: %u; right: %u; top:%u bottom:%u\n",
	    rect.left, rect.right, rect.top, rect.bottom));
	go_wmf_read_pointl (state->data + 16, &x0, &y0);
	go_emf_convert_coords (state, &x0, &y0);
	d_(("\tstart at x=%g y=%g\n", x0, y0));
	go_wmf_read_pointl (state->data + 24, &x1, &y1);
	go_emf_convert_coords (state, &x1, &y1);
	d_(("\tend at x=%g y=%g\n", x1, y1));
	xc = (rect.left + rect.right) / 2.;
	yc = (rect.top + rect.bottom) / 2.;
	goc_item_set_transform (goc_item_new (state->curDC->group, GOC_TYPE_ARC,
	                                      "xc", xc, "yc", yc,
	                                      "xr", rect.right - xc,
	                                      "yr", rect.bottom - yc,
	                                      "ang1", -atan2 (y0 - yc, x0 - xc),
	                                      "ang2", -atan2 (y1 - yc, x1 - xc),
	                                      "type", type,
	                                      "style", state->curDC->style,
	                                      NULL),
	                        &state->curDC->m);
}

static gboolean
go_emf_arc (GOEmfState *state)
{
	d_(("arc\n"));
	_go_emf_arc (state, 0);
	return TRUE;
}

static gboolean
go_emf_chord (GOEmfState *state)
{
	d_(("chord\n"));
	_go_emf_arc (state, 1);
	return TRUE;
}

static gboolean
go_emf_pie (GOEmfState *state)
{
	d_(("pie\n"));
	_go_emf_arc (state, 2);
	return TRUE;
}

static gboolean
go_emf_selectpalette (GOEmfState *state)
{
	d_(("selectpalette\n"));
	return TRUE;
}

static gboolean
go_emf_createpalette (GOEmfState *state)
{
	d_(("createpalette\n"));
	return TRUE;
}

static gboolean
go_emf_setpaletteentries (GOEmfState *state)
{
	d_(("setpaletteentries\n"));
	return TRUE;
}

static gboolean
go_emf_resizepalette (GOEmfState *state)
{
	d_(("resizepalette\n"));
	return TRUE;
}

static gboolean
go_emf_realizepalette (GOEmfState *state)
{
	d_(("realizepalette\n"));
	return TRUE;
}

static gboolean
go_emf_extfloodfill (GOEmfState *state)
{
	d_(("extfloodfill\n"));
	return TRUE;
}

static gboolean
go_emf_lineto (GOEmfState *state)
{
	double x, y;
	d_(("lineto\n"));
	if (state->curDC->path == NULL) {
		state->curDC->path = go_path_new ();
		go_path_move_to (state->curDC->path,
		                 state->curDC->xpos,
		                 state->curDC->ypos);
	}
	go_wmf_read_pointl (state->data, &x, &y);
	go_emf_convert_coords (state, &x, &y);
	go_path_line_to (state->curDC->path, x, y);
	d_(("\tLine to x=%g y=%g\n", x, y));
	return TRUE;
}

static gboolean
go_emf_arcto (GOEmfState *state)
{
	d_(("arcto\n"));
	return TRUE;
}

static gboolean
go_emf_polydraw (GOEmfState *state)
{
	d_(("polydraw\n"));
	return TRUE;
}

static gboolean
go_emf_setarcdirection (GOEmfState *state)
{
	d_(("setarcdirection\n"));
	return TRUE;
}

static gboolean
go_emf_setmiterlimit (GOEmfState *state)
{
	d_(("setmiterlimit\n"));
	return TRUE;
}

static gboolean
go_emf_beginpath (GOEmfState *state)
{
	d_(("beginpath\n"));
	if (state->curDC->path != NULL)
		return FALSE;
	state->curDC->path = go_path_new ();
	state->curDC->closed_path = FALSE;
	return TRUE;
}

static gboolean
go_emf_endpath (GOEmfState *state)
{
	d_(("endpath\n"));
	return state->curDC->path != NULL;
}

static gboolean
go_emf_closefigure (GOEmfState *state)
{
	d_(("closefigure\n"));
	if (state->curDC->path == NULL)
		return FALSE;
	go_path_close (state->curDC->path);
	state->curDC->closed_path = TRUE;
	return TRUE;
}

static gboolean
go_emf_fillpath (GOEmfState *state)
{
	GOStyle *style;
	d_(("fillpath\n"));
	if (state->curDC->path == NULL)
		return FALSE;
	style = go_style_dup (state->curDC->style);
	style->interesting_fields = GO_STYLE_FILL;
	goc_item_new (state->curDC->group, GOC_TYPE_PATH,
	              "path", state->curDC->path,
	              "style", style,
	              "closed", state->curDC->closed_path,
	              "fill-rule", state->curDC->PolygonFillMode,
	              NULL);
	g_object_unref (style);
	go_path_free (state->curDC->path);
	state->curDC->path = NULL;
	return TRUE;
}

static gboolean
go_emf_strokeandfillpath (GOEmfState *state)
{
	d_(("strokeandfillpath\n"));
	if (state->curDC->path == NULL)
		return FALSE;
	goc_item_new (state->curDC->group, GOC_TYPE_PATH,
	              "path", state->curDC->path,
	              "style", state->curDC->style,
	              "closed", state->curDC->closed_path,
	              "fill-rule", state->curDC->PolygonFillMode,
	              NULL);
	go_path_free (state->curDC->path);
	state->curDC->path = NULL;
	return TRUE;
}

static gboolean
go_emf_strokepath (GOEmfState *state)
{
	GOStyle *style;
	d_(("strokepath\n"));
	if (state->curDC->path == NULL)
		return FALSE;
	style = go_style_dup (state->curDC->style);
	style->interesting_fields = GO_STYLE_LINE;
	goc_item_new (state->curDC->group, GOC_TYPE_PATH,
	              "path", state->curDC->path,
	              "style", style,
	              NULL);
	g_object_unref (style);
	go_path_free (state->curDC->path);
	state->curDC->path = NULL;
	return TRUE;
}

static gboolean
go_emf_flattenpath (GOEmfState *state)
{
	d_(("flattenpath\n"));
	return TRUE;
}

static gboolean
go_emf_widenpath (GOEmfState *state)
{
	d_(("widenpath\n"));
	return TRUE;
}

static gboolean
go_emf_selectclippath (GOEmfState *state)
{
	d_(("selectclippath\n"));
	return TRUE;
}

static gboolean
go_emf_abortpath (GOEmfState *state)
{
	d_(("abortpath\n"));
	return TRUE;
}

static gboolean
go_emf_comment (GOEmfState *state)
{
	d_(("comment\n"));
	return TRUE;
}

static gboolean
go_emf_fillrgn (GOEmfState *state)
{
	d_(("fillrgn\n"));
	return TRUE;
}

static gboolean
go_emf_framergn (GOEmfState *state)
{
	d_(("framergn\n"));
	return TRUE;
}

static gboolean
go_emf_invertrgn (GOEmfState *state)
{
	d_(("invertrgn\n"));
	return TRUE;
}

static gboolean
go_emf_paintrgn (GOEmfState *state)
{
	d_(("paintrgn\n"));
	return TRUE;
}

static gboolean
go_emf_extselectcliprgn (GOEmfState *state)
{
	d_(("extselectcliprgn\n"));
	return TRUE;
}

static gboolean
go_emf_bitblt (GOEmfState *state)
{
	GOWmfRectL rect;
	gint32 xDest, yDest, cxDest, cyDest, xSrc, ySrc;
	guint32 Oper;
	cairo_matrix_t m;
	d_(("bitblt\n"));
	go_wmf_read_rectl (&rect, state->data); /* do we need it? */
	xDest = GSF_LE_GET_GINT32 (state->data + 16);
	yDest = GSF_LE_GET_GINT32 (state->data + 20);
	cxDest = GSF_LE_GET_GINT32 (state->data + 24);
	cyDest = GSF_LE_GET_GINT32 (state->data + 28);
	Oper = GSF_LE_GET_GINT32 (state->data + 32);
	xSrc = GSF_LE_GET_GINT32 (state->data + 36);
	ySrc = GSF_LE_GET_GINT32 (state->data + 40);
	d_(("\tDestination: x=%d y=%d cx=%d cy=%d Operation: %X\n", xDest, yDest, cxDest, cyDest, Oper));
	d_(("\tSource: x=%d y=%d\n", xSrc, ySrc));
	m.xx = GSF_LE_GET_FLOAT (state->data + 44);
	m.yx = GSF_LE_GET_FLOAT (state->data + 48);
	m.xy = GSF_LE_GET_FLOAT (state->data + 52);
	m.yy = GSF_LE_GET_FLOAT (state->data + 56);
	m.x0 = GSF_LE_GET_FLOAT (state->data + 60);
	m.y0 = GSF_LE_GET_FLOAT (state->data + 64);
	d_(("\tXForm: m11=%g m12=%g dx=%g\n\t       m21=%g m22=%g dy=%g\n",
	    m.xx, m.yx, m.xy, m.yy, m.x0, m.y0));
	if (cxDest > 0 && cyDest > 0) {
		switch (Oper) {
		case 0x62: // black rectangle
		case 0xFF0042: //white rectangle
		case 0xF00021: { // fill with brush
			GOStyle *style = go_style_dup (state->curDC->style);
			style->interesting_fields = GO_STYLE_FILL;
			if (Oper != 0xF00021) {
				style->fill.type = GO_STYLE_FILL_PATTERN;
				style->fill.pattern.pattern = GO_PATTERN_SOLID;
				style->fill.pattern.back = (Oper == 0x62)? GO_COLOR_BLACK: GO_COLOR_WHITE;
			}
			goc_item_set_transform (goc_item_new (state->curDC->group, GOC_TYPE_RECTANGLE,
					                              "x", (double) xDest,
			                                      "y", (double) yDest,
			                                      "width", (double) cxDest,
					                              "height", (double) cyDest,
					                              "style", style, NULL),
					                &m);
			g_object_unref (style);
			break;
		}
		default:
			// TODO: implement, see https://wiki.winehq.org/Ternary_Raster_Ops
			break;
		}
	}
	return TRUE;
}

static gboolean
go_emf_stretchblt (GOEmfState *state)
{
	d_(("stretchblt\n"));
	return TRUE;
}

static gboolean
go_emf_maskblt (GOEmfState *state)
{
	d_(("maskblt\n"));
	return TRUE;
}

static gboolean
go_emf_plgblt (GOEmfState *state)
{
	d_(("plgblt\n"));
	return TRUE;
}

static gboolean
go_emf_setdibitstodevice (GOEmfState *state)
{
	d_(("setdibitstodevice\n"));
	return TRUE;
}

static gboolean
go_emf_stretchdibits (GOEmfState *state)
{
	GOWmfRectL rect;
	gint32 dst_x, dst_y, dst_cx, dst_cy;
	gint32 src_x, src_y, src_cx, src_cy;
	guint32 usage_src, op;
	guint32 header_pos, header_size, buffer_pos, buffer_size;
	unsigned offset;
	GODibHeader header;
	GdkPixbuf *pixbuf;
	unsigned i;

	d_(("stretchdibits\n"));
	go_wmf_read_rectl (&rect, state->data);
	dst_x = GSF_LE_GET_GINT32 (state->data + 16);
	dst_y = GSF_LE_GET_GINT32 (state->data + 20);
	src_x = GSF_LE_GET_GINT32 (state->data + 24);
	src_y = GSF_LE_GET_GINT32 (state->data + 28);
	src_cx = GSF_LE_GET_GINT32 (state->data + 32);
	src_cy = GSF_LE_GET_GINT32 (state->data + 36);
	header_pos = GSF_LE_GET_GUINT32 (state->data + 40) - 8; /* because data does not contain the id and size */
	header_size = GSF_LE_GET_GUINT32 (state->data + 44);
	buffer_pos = GSF_LE_GET_GUINT32 (state->data + 48) - 8;
	buffer_size = GSF_LE_GET_GUINT32 (state->data + 52);
	d_ (("\tbits header at %u (%u bytes), pixels at %u (%u bytes)\n",
	     header_pos, header_size, buffer_pos, buffer_size));
	usage_src = GSF_LE_GET_GUINT32 (state->data + 56);
	op = GSF_LE_GET_GUINT32 (state->data + 60);
	dst_cx = GSF_LE_GET_GINT32 (state->data + 64);
	dst_cy = GSF_LE_GET_GINT32 (state->data + 68);
	d_ (("\tsource format: %s, operation: %s\n", go_emf_dib_colors[usage_src], go_emf_ternary_raster_operation[op >> 16]));
	d_ (("\tsource: x=%d y=%d cx=%d cy=%d\n",src_x, src_y, src_cx, src_cy));
	d_ (("\tdestination: x=%d y=%d cx=%d cy=%d\n",dst_x, dst_y, dst_cx, dst_cy));
	/* try building an in memory BMP file from the data */
	go_dib_read_header (&header, state->data + header_pos);
	if (header.type == 0xff)
		return FALSE;
	/* load the colors if needed */
	if (header.color_used != 0) {
		/* check that the header has the right size */
		if (header_size - header.size != 4 * header.color_used) {
			g_warning ("Invalid color table");
			return FALSE;
		}
		offset = header_pos + header.size;
		header.colors = g_new (GOColor, 256);
		for (i = 0; i < header.color_used; i++) {
			header.colors[i] = GO_COLOR_FROM_RGB (state->data[offset + 2],
			                                      state->data[offset + 1],
			                                      state->data[offset]);
			offset += 4;
		}
	} else
		header.colors = NULL;
	pixbuf = go_dib_create_pixbuf_from_data (&header, state->data + buffer_pos);
	g_free (header.colors);
	if (src_x != 0 || src_y != 0 || src_cx != dst_cx || src_cy != dst_cy) {
		/* FIXME: take src coordinates into account */
	}
	goc_item_new (state->group,
	              GOC_TYPE_PIXBUF, "pixbuf", pixbuf,
	              "x", (double) dst_x, "y", (double) dst_y,
	              "width", (double) dst_cx, "height", (double) dst_cy,
	              NULL);
	return TRUE;
}

static gboolean
go_emf_extcreatefontindirectw (GOEmfState *state)
{
	unsigned index;
	char *buf;
	GOEmfFont *font = g_new0 (GOEmfFont, 1);
	font->obj_type = GO_EMF_OBJ_TYPE_FONT;
	d_(("extcreatefontindirectw\n"));
	index = GSF_LE_GET_GUINT32 (state->data);
	font->size = GSF_LE_GET_GINT32 (state->data + 4) * state->dh / state->wh;
	font->width = GSF_LE_GET_GINT32 (state->data + 8);
	font->escape = GSF_LE_GET_GINT32 (state->data + 12);
	font->orient = GSF_LE_GET_GINT32 (state->data + 16);
	font->weight = GSF_LE_GET_GINT32 (state->data + 20);
	font->italic = GSF_LE_GET_GUINT8 (state->data + 24);
	font->under = GSF_LE_GET_GUINT8 (state->data + 25);
	font->strike = GSF_LE_GET_GUINT8 (state->data + 26);
	font->charset = GSF_LE_GET_GUINT8 (state->data + 27);
	font->outprec = GSF_LE_GET_GUINT8 (state->data + 28);
	font->clipprec = GSF_LE_GET_GUINT8 (state->data + 29);
	font->quality = GSF_LE_GET_GUINT8 (state->data + 30);
	font->pitch_and_family = GSF_LE_GET_GUINT8 (state->data + 31);
	buf = g_utf16_to_utf8 ((gunichar2 const *) (state->data + 32), 32, NULL, NULL, NULL);
	strncpy (font->facename, buf, 64);
	g_free (buf);
#if 0
	/* We do not support DesignVector objects for now and we don't need full font names. */
	if (state->length > 332) {
	}
#endif
	g_hash_table_replace (state->mfobjs, GUINT_TO_POINTER (index), font);
	d_(("\tfont index=%u face=%s size=%d\n",index,font->facename, font->size));
	return TRUE;
}

static gboolean
go_emf_exttextouta (GOEmfState *state)
{
	unsigned mode, nb, offset;
	float xscale = 1., yscale = 1.;
	double x, y;
	char *buf;
	GOStyle *style = go_style_new ();
	d_(("exttextouta\n"));
	mode = GSF_LE_GET_GUINT32 (state->data + 16);
	d_(("\t graphic mode is %s\n", mode == 1? "compatible": "advanced"));
	if (mode == 1) {
		xscale = GSF_LE_GET_FLOAT (state->data + 20); /* don't use for now */
		yscale = GSF_LE_GET_FLOAT (state->data + 24);
		d_(("\t exScale = %g \t eyScale = %g\n", xscale, yscale));
	}
	go_wmf_read_pointl (state->data + 28, &x, &y);
	nb = GSF_LE_GET_GUINT32 (state->data + 36);
	offset = GSF_LE_GET_GUINT32 (state->data + 40) - 8; /* 8 for code and size */
	buf = g_strndup (state->data + offset, nb);
	d_(("\t text is %s\n", buf));
	style->font.color = state->curDC->text_color;
	if (style->font.font)
		go_font_unref (style->font.font);
	style->font.font = go_font_ref (state->curDC->font);
	goc_item_set_transform (goc_item_new (state->curDC->group, GOC_TYPE_TEXT,
	                                      "text", buf,
	                                      "x", x,
	                                      "y", y,
	                                      "anchor",  state->curDC->text_align,
	                                      "rotation", state->curDC->text_rotation,
	                                      "style", style,
	                                      NULL),
	                        &state->curDC->m);
	g_object_unref (style);
	g_free (buf);
	return TRUE;
}

static gboolean
go_emf_exttextoutw (GOEmfState *state)
{
	unsigned mode, nb, offset;
	float xscale = 1., yscale = 1.;
	double x, y;
	char *buf;
	GOStyle *style = go_style_new ();
	d_(("exttextoutw\n"));
	mode = GSF_LE_GET_GUINT32 (state->data + 16);
	d_(("\t graphic mode is %s\n", mode == 1? "compatible": "advanced"));
	if (mode == 1) {
		xscale = GSF_LE_GET_FLOAT (state->data + 20); /* don't use for now */
		yscale = GSF_LE_GET_FLOAT (state->data + 24);
		d_(("\t exScale = %g \t eyScale = %g\n", xscale, yscale));
	}
	go_wmf_read_pointl (state->data + 28, &x, &y);
	nb = GSF_LE_GET_GUINT32 (state->data + 36);
	offset = GSF_LE_GET_GUINT32 (state->data + 40) - 8; /* 8 for code and size */
	buf = g_utf16_to_utf8 ((gunichar2 const *) (state->data + offset), nb, NULL, NULL, NULL);
	d_(("\t text is %s\n", buf));
	style->font.color = state->curDC->text_color;
	if (style->font.font)
		go_font_unref (style->font.font);
	style->font.font = go_font_ref (state->curDC->font);
	goc_item_set_transform (goc_item_new (state->curDC->group, GOC_TYPE_TEXT,
	                                      "text", buf,
	                                      "x", x,
	                                      "y", y,
	                                      "anchor",  state->curDC->text_align,
	                                      "rotation", state->curDC->text_rotation,
	                                      "style", style,
	                                      NULL),
	                        &state->curDC->m);
	g_object_unref (style);
	g_free (buf);
	return TRUE;
}

static gboolean
go_emf_polybezier16 (GOEmfState *state)
{
	unsigned count, n, offset;
	double x0, y0, x1, y1, x2, y2;
	GOPath *path;
#ifdef DEBUG_EMF_SUPPORT
	GOWmfRectL rect;
#endif
	d_(("polybezier16\n"));
#ifdef DEBUG_EMF_SUPPORT
	go_wmf_read_rectl (&rect, state->data);
#endif
	d_(("\tbounds: left: %u; right: %u; top:%u bottom:%u\n",
	    rect.left,rect.right, rect.top, rect.bottom));
	count = GSF_LE_GET_GUINT32 (state->data + 16);
	d_(("\tfound %u points\n", count));
	if (count % 3 != 1)
		return FALSE;
	path = go_path_new ();
	count /= 3;
	go_wmf_read_points (state->data + 20, &x0, &y0);
	go_emf_convert_coords (state, &x0, &y0);
	d_(("\tmove to x0=%g y0=%g\n", x0, y0));
	go_path_move_to (path, x0, y0);
	for (n = 0, offset = 24; n < count; n++, offset += 12) {
		go_wmf_read_points (state->data + offset, &x0, &y0);
		go_emf_convert_coords (state, &x0, &y0);
		go_wmf_read_points (state->data + offset + 4, &x1, &y1);
		go_emf_convert_coords (state, &x1, &y1);
		go_wmf_read_points (state->data + offset + 8, &x2, &y2);
		go_emf_convert_coords (state, &x2, &y2);
		go_path_curve_to (path, x0, y0, x1, y1, x2, y2);
		d_(("\tcurve to x0=%g y0=%g x1=%g y1=%g x2=%g y2=%g\n", x0, y0, x1, y1, x2, y2));
	}
	goc_item_set_transform (goc_item_new (state->curDC->group, GOC_TYPE_PATH,
	                                      "path", path,
	                                      "style", state->curDC->style,
	                                      "closed", FALSE,
	                                      NULL),
	                        &state->curDC->m);
	go_path_free (path);
	return TRUE;
}

static gboolean
go_emf_polygon16 (GOEmfState *state)
{
	unsigned nb_pts, n, offset;
#ifdef DEBUG_EMF_SUPPORT
	GOWmfRectL rect;
#endif
	GocPoints *points;
	double x, y;
	d_(("polygon16\n"));
#ifdef DEBUG_EMF_SUPPORT
	go_wmf_read_rectl (&rect, state->data);
#endif
	d_(("\tbounds: left: %u; right: %u; top:%u bottom:%u\n",
	    rect.left,rect.right, rect.top, rect.bottom));
	nb_pts = GSF_LE_GET_GUINT32 (state->data + 16);
	d_(("\t%u points\n", nb_pts));
	points = goc_points_new (nb_pts);
	for (n = 0, offset = 20; n < nb_pts; n++, offset += 4) {
		go_wmf_read_points (state->data + offset, &x, &y);
		go_emf_convert_coords (state, &x, &y);
		points->points[n].x = x;
		points->points[n].y = y;
		d_(("\tpoint #%u at x=%g y=%g\n", n, x, y));
	}
	goc_item_set_transform (goc_item_new (state->curDC->group, GOC_TYPE_POLYGON,
	                                      "points", points,
	                                      "style", state->curDC->style,
	                                      NULL),
	                        &state->curDC->m);
	return TRUE;
}

static gboolean
go_emf_polyline16 (GOEmfState *state)
{
	unsigned nb_pts, n, offset;
#ifdef DEBUG_EMF_SUPPORT
	GOWmfRectL rect;
#endif
	GocPoints *points;
	double x, y;
	d_(("polyline16\n"));
#ifdef DEBUG_EMF_SUPPORT
	go_wmf_read_rectl (&rect, state->data);
#endif
	d_(("\tbounds: left: %u; right: %u; top:%u bottom:%u\n",
	    rect.left,rect.right, rect.top, rect.bottom));
	nb_pts = GSF_LE_GET_GUINT32 (state->data + 16);
	d_(("\t%u points\n", nb_pts));
	points = goc_points_new (nb_pts);
	for (n = 0, offset = 20; n < nb_pts; n++, offset += 4) {
		go_wmf_read_points (state->data + offset, &x, &y);
		go_emf_convert_coords (state, &x, &y);
		points->points[n].x = x;
		points->points[n].y = y;
		d_(("\tpoint #%u at x=%g y=%g\n", n, x, y));
	}
	goc_item_new (state->curDC->group, GOC_TYPE_POLYLINE,
	              "points", points,
	              "style", state->curDC->style,
	              NULL);
	return TRUE;
}

static gboolean
go_emf_polybezierto16 (GOEmfState *state)
{
	unsigned count, n, offset;
	double x0, y0, x1, y1, x2, y2;
#ifdef DEBUG_EMF_SUPPORT
	GOWmfRectL rect;
#endif
	d_(("polybezierto16\n"));
#ifdef DEBUG_EMF_SUPPORT
	go_wmf_read_rectl (&rect, state->data);
#endif
	d_(("\tbounds: left: %u; right: %u; top:%u bottom:%u\n",
	    rect.left,rect.right, rect.top, rect.bottom));
	count = GSF_LE_GET_GUINT32 (state->data + 16);
	d_(("\tfound %u points\n", count));
	if (count % 3 != 0)
		return FALSE;
	count /= 3;
	for (n = 0, offset = 20; n < count; n++, offset += 12) {
		go_wmf_read_points (state->data + offset, &x0, &y0);
		go_emf_convert_coords (state, &x0, &y0);
		go_wmf_read_points (state->data + offset + 4, &x1, &y1);
		go_emf_convert_coords (state, &x1, &y1);
		go_wmf_read_points (state->data + offset + 8, &x2, &y2);
		go_emf_convert_coords (state, &x2, &y2);
		go_path_curve_to (state->curDC->path, x0, y0, x1, y1, x2, y2);
		d_(("\tcurve to x0=%g y0=%g x1=%g y1=%g x2=%g y2=%g\n", x0, y0, x1, y1, x2, y2));
	}
	return TRUE;
}

static gboolean
go_emf_polylineto16 (GOEmfState *state)
{
	unsigned count, n, offset;
	double x, y;
#ifdef DEBUG_EMF_SUPPORT
	GOWmfRectL rect;
#endif
	d_(("polylineto16\n"));
#ifdef DEBUG_EMF_SUPPORT
	go_wmf_read_rectl (&rect, state->data);
#endif
	d_(("\tbounds: left: %u; right: %u; top:%u bottom:%u\n",
	    rect.left,rect.right, rect.top, rect.bottom));
	count = GSF_LE_GET_GUINT32 (state->data + 16);
	d_(("\tfound %u points\n", count));
	for (n = 0, offset = 20; n < count; n++, offset += 4) {
		go_wmf_read_points (state->data + offset, &x, &y);
		go_emf_convert_coords (state, &x, &y);
		go_path_line_to (state->curDC->path, x, y);
		d_(("\tline to x=%g y0=%g\n", x, y));
	}
	return TRUE;
}

static gboolean
go_emf_polypolyline16 (GOEmfState *state)
{
	unsigned nb_lines, nb_pts, n, l, i, nm, offset, loffset;
	double x, y;
	GocPoints *points;
#ifdef DEBUG_EMF_SUPPORT
	GOWmfRectL rect;
#endif
	d_(("polypolyline16\n"));
#ifdef DEBUG_EMF_SUPPORT
	go_wmf_read_rectl (&rect, state->data);
#endif
	d_(("\tbounds: left: %u; right: %u; top:%u bottom:%u\n",
	    rect.left,rect.right, rect.top, rect.bottom));
	nb_lines = GSF_LE_GET_GUINT32 (state->data + 16);
	nb_pts = GSF_LE_GET_GUINT32 (state->data + 20);
	d_(("\t%u points in %u lines\n", nb_pts, nb_lines));
	points = goc_points_new (nb_pts + nb_lines - 1);
	offset = 24 + 4 * nb_lines;
	loffset = 24;
	i = 0;
	l = 0;
	while (1) {
		nm = GSF_LE_GET_GUINT32 (state->data + loffset);
		loffset += 4;
		for (n = 0; n < nm; n++) {
			go_wmf_read_points (state->data + offset, &x, &y);
			go_emf_convert_coords (state, &x, &y);
			points->points[i].x = x;
			points->points[i++].y = y;
			d_(("\tpoint #%u at x=%g y=%g\n", n, x, y));
		}
		l++;
		if (l == nb_lines)
			break;
		points->points[i].x = go_nan;
		points->points[i++].y = go_nan;
	}
	goc_item_new (state->curDC->group, GOC_TYPE_POLYGON,
	              "points", points,
	              "style", state->curDC->style,
	              NULL);
	goc_points_unref (points);
	return TRUE;
}

static gboolean
go_emf_polypolygon16 (GOEmfState *state)
{
	unsigned nb_polygons, nb_pts, n, offset;
	GocPoints *points;
	GocIntArray *sizes;
	double x, y;
#ifdef DEBUG_EMF_SUPPORT
	GOWmfRectL rect;
#endif
	d_(("polypolygon16\n"));
#ifdef DEBUG_EMF_SUPPORT
	go_wmf_read_rectl (&rect, state->data);
#endif
	d_(("\tbounds: left: %u; right: %u; top:%u bottom:%u\n",
	    rect.left,rect.right, rect.top, rect.bottom));
	nb_polygons = GSF_LE_GET_GUINT32 (state->data + 16);
	nb_pts = GSF_LE_GET_GUINT32 (state->data + 20);
	d_(("\t%u points in %u polygons\n", nb_pts, nb_polygons));
	sizes = goc_int_array_new (nb_polygons);
	points = goc_points_new (nb_pts);
	for (n = 0, offset = 24; n < nb_polygons; n++, offset += 4) {
		sizes->vals[n] = GSF_LE_GET_GUINT32 (state->data + offset);
		d_(("\tfound size %u\n", sizes->vals[n]));
	}
	for (n = 0; n < nb_pts; n++, offset += 8) {
		go_wmf_read_points (state->data + offset, &x, &y);
		go_emf_convert_coords (state, &x, &y);
		points->points[n].x = x;
		points->points[n].y = y;
		d_(("\tpoint #%u at x=%g y=%g\n", n, x, y));
	}
	goc_item_new (state->curDC->group, GOC_TYPE_POLYGON,
	              "points", points,
	              "sizes", sizes,
	              "fill-rule", state->curDC->PolygonFillMode,
	              "style", state->curDC->style,
	              NULL);
	goc_int_array_unref (sizes);
	goc_points_unref (points);
	return TRUE;
}

static gboolean
go_emf_polydraw16 (GOEmfState *state)
{
	d_(("polydraw16 \n"));
	return TRUE;
}

static gboolean
go_emf_createmonobrush (GOEmfState *state)
{
	d_(("createmonobrush\n"));
	return TRUE;
}

static gboolean
go_emf_createdibpatternbrushpt (GOEmfState *state)
{
	d_(("createdibpatternbrushpt\n"));
	return TRUE;
}

static gboolean
go_emf_extcreatepen (GOEmfState *state)
{
	unsigned index/*, offBmi, cbBmi, offBits, cbBits*/;
	GOEmfPen *pen = g_new0 (GOEmfPen, 1);
	pen->obj_type = GO_EMF_OBJ_TYPE_PEN;
	d_(("extcreatepen\n"));
	index = GSF_LE_GET_GUINT32 (state->data);
/*	offBmi = GSF_LE_GET_GUINT32 (state->data + 4);
	cbBmi = GSF_LE_GET_GUINT32 (state->data + 8);
	offBits = GSF_LE_GET_GUINT32 (state->data + 12);
	cbBits = GSF_LE_GET_GUINT32 (state->data + 16);
	if (cbBmi != 0 || cbBits != NULL) {
		FIXME: load DIB data if any
	}*/
	pen->style = GSF_LE_GET_GUINT32 (state->data + 20);
	pen->width = (pen->style)? GSF_LE_GET_GUINT32 (state->data + 24): 0;
	pen->clr = go_wmf_read_gocolor (state->data + 32);
	d_(("\tpen index=%u style=%s width=%g color=%08x join=%s cap=%s %s\n",index,
	    dashes[pen->style&0xf], pen->width, pen->clr,
	    join[(pen->style&0xf000)>>24], cap[(pen->style&0xf00)>>16],
	    pen->style&0x00010000? "Geometric": ""));
	g_hash_table_replace (state->mfobjs, GUINT_TO_POINTER (index), pen);
	return TRUE;
}

static gboolean
go_emf_polytextouta (GOEmfState *state)
{
	d_(("polytextouta\n"));
	return TRUE;
}

static gboolean
go_emf_polytextoutw (GOEmfState *state)
{
	d_(("polytextoutw\n"));
	return TRUE;
}

static gboolean
go_emf_seticmmode (GOEmfState *state)
{
	d_(("seticmmode\n"));
	return TRUE;
}

static gboolean
go_emf_createcolorspace (GOEmfState *state)
{
	d_(("createcolorspace\n"));
	return TRUE;
}

static gboolean
go_emf_setcolorspace (GOEmfState *state)
{
	d_(("setcolorspace\n"));
	return TRUE;
}

static gboolean
go_emf_deletecolorspace (GOEmfState *state)
{
	d_(("deletecolorspace\n"));
	return TRUE;
}

typedef gboolean (*GOEmfHandler) (GOEmfState* state);

static  GOEmfHandler go_emf_handlers[] = {
	NULL,
	go_emf_header,			/* 0x0001 ok */
	go_emf_polybezier,		/* 0x0002 untested */
	go_emf_polygon,			/* 0x0003 ok */
	go_emf_polyline,		/* 0x0004 ok */
	go_emf_polybezierto,		/* 0x0005 ok */
	go_emf_polylineto,		/* 0x0006 ok */
	go_emf_polypolyline,		/* 0x0007 untested */
	go_emf_polypolygon,		/* 0x0008 ok */
	go_emf_setwindowextex,		/* 0x0009 ok */
	go_emf_setwindoworgex,		/* 0x000A ok */
	go_emf_setviewportextex,	/* 0x000B ok */
	go_emf_setviewportorgex,	/* 0x000C ok */
	go_emf_setbrushorgex,		/* 0x000D todo */
	go_emf_eof,			/* 0x000E ok */
	go_emf_setpixelv,		/* 0x000F todo */
	go_emf_setmapperflags,		/* 0x0010 todo */
	go_emf_setmapmode,		/* 0x0011 todo */
	go_emf_setbkmode,		/* 0x0012 todo */
	go_emf_setpolyfillmode,		/* 0x0013 ok */
	go_emf_setrop2,			/* 0x0014 todo */
	go_emf_setstretchbltmode,       /* 0x0015 todo */
	go_emf_settextalign,		/* 0x0016 ok */
	go_emf_setcoloradjustment,	/* 0x0017 todo */
	go_emf_settextcolor,		/* 0x0018 ok */
	go_emf_setbkcolor,		/* 0x0019 todo */
	go_emf_offsetcliprgn,		/* 0x001A todo */
	go_emf_movetoex,		/* 0x001B ok */
	go_emf_setmetargn,		/* 0x001C todo */
	go_emf_excludecliprect,		/* 0x001D todo */
	go_emf_intersectcliprect,	/* 0x001E todo */
	go_emf_scaleviewportextex,	/* 0x001F todo */
	go_emf_scalewindowextex,	/* 0x0020 todo */
	go_emf_savedc,			/* 0x0021 ok */
	go_emf_restoredc,		/* 0x0022 ok */
	go_emf_setworldtransform,	/* 0x0023 ok */
	go_emf_modifyworldtransform,	/* 0x0024 ok */
	go_emf_selectobject,		/* 0x0025 partial */
	go_emf_createpen,		/* 0x0026 ok */
	go_emf_createbrushindirect,	/* 0x0027 ok */
	go_emf_deleteobject,		/* 0x0028 todo (if needed?) */
	go_emf_anglearc,		/* 0x0029 todo */
	go_emf_ellipse,			/* 0x002A ok */
	go_emf_rectangle,		/* 0x002B ok */
	go_emf_roundrect,		/* 0x002C ok */
	go_emf_arc,			/* 0x002D ok */
	go_emf_chord,			/* 0x002E ok */
	go_emf_pie,			/* 0x002F ok */
	go_emf_selectpalette,		/* 0x0030 todo */
	go_emf_createpalette,		/* 0x0031 todo */
	go_emf_setpaletteentries,	/* 0x0032 todo */
	go_emf_resizepalette,		/* 0x0033 todo */
	go_emf_realizepalette,		/* 0x0034 todo */
	go_emf_extfloodfill,		/* 0x0035 todo */
	go_emf_lineto,			/* 0x0036 ok */
	go_emf_arcto,			/* 0x0037 todo */
	go_emf_polydraw,		/* 0x0038 todo */
	go_emf_setarcdirection,		/* 0x0039 todo */
	go_emf_setmiterlimit,		/* 0x003A todo */
	go_emf_beginpath,		/* 0x003B ok */
	go_emf_endpath,			/* 0x003C ok */
	go_emf_closefigure,		/* 0x003D ok */
	go_emf_fillpath,		/* 0x003E ok */
	go_emf_strokeandfillpath,	/* 0x003F ok */
	go_emf_strokepath,		/* 0x0040 ok */
	go_emf_flattenpath,		/* 0x0041 todo */
	go_emf_widenpath,		/* 0x0042 todo */
	go_emf_selectclippath,		/* 0x0043 todo */
	go_emf_abortpath,		/* 0x0044 todo */
	NULL,
	go_emf_comment,			/* 0x0046 todo (if needed?) */
	go_emf_fillrgn,			/* 0x0047 todo */
	go_emf_framergn,		/* 0x0048 todo */
	go_emf_invertrgn,		/* 0x0049 todo */
	go_emf_paintrgn,		/* 0x004A todo */
	go_emf_extselectcliprgn,	/* 0x004B todo */
	go_emf_bitblt,			/* 0x004C todo */
	go_emf_stretchblt,		/* 0x004D todo */
	go_emf_maskblt,			/* 0x004E todo */
	go_emf_plgblt,			/* 0x004F todo */
	go_emf_setdibitstodevice,	/* 0x0050 todo */
	go_emf_stretchdibits,		/* 0x0051 partial */
	go_emf_extcreatefontindirectw,	/* 0x0052 partial */
	go_emf_exttextouta,		/* 0x0053 partial and untested */
	go_emf_exttextoutw,		/* 0x0054 partial */
	go_emf_polybezier16,		/* 0x0055 ok */
	go_emf_polygon16,		/* 0x0056 ok */
	go_emf_polyline16,		/* 0x0057 ok */
	go_emf_polybezierto16,		/* 0x0058 untested */
	go_emf_polylineto16,		/* 0x0059 untested */
	go_emf_polypolyline16,		/* 0x005A untested */
	go_emf_polypolygon16,		/* 0x005B untested */
	go_emf_polydraw16,		/* 0x005C todo */
	go_emf_createmonobrush,		/* 0x005D todo */
	go_emf_createdibpatternbrushpt,	/* 0x005E todo */
	go_emf_extcreatepen,		/* 0x005F partial */
	go_emf_polytextouta,		/* 0x0060 todo */
	go_emf_polytextoutw,		/* 0x0051 todo */
	go_emf_seticmmode,		/* 0x0062 todo */
	go_emf_createcolorspace,	/* 0x0063 todo */
	go_emf_setcolorspace,		/* 0x0064 todo */
	go_emf_deletecolorspace		/* 0x0065 todo */
};
#endif

/*****************************************************************************
 * EMF parsing code
 *****************************************************************************/

static gboolean
go_emf_parse (GOEmf *emf, GsfInput *input, GError **error)
{
	const guint8  *data = {0};
	guint32 offset = 0;
	guint64 fsize = 0;
	guint32 rsize = 3;
#ifdef GOFFICE_EMF_SUPPORT
	GHashTable	*mrecords, *escrecords;
	GHashTable	*objs;
	GOWmfPage *mypg;
	int rid = 0, escfunc;
	gint mr = 0;
	guint32 type = 0; /* wmf, apm or emf */
	double x1, y1, x2, y2, w, h;
	GOWmfDC *dc;

#endif
	fsize = gsf_input_size (input);
	if (fsize < 4)
		return FALSE;
	data = gsf_input_read (input, 4, NULL);
	if (!data)
		return FALSE;
	switch (GSF_LE_GET_GUINT32 (data)) {
	case 0x9ac6cdd7:
		d_ (("Aldus Placeable Metafile\n"));
#ifdef GOFFICE_EMF_SUPPORT
		type = 1;
		offset = 40;
			break;
#else
		return FALSE;
#endif
	case 0x090001:
		d_ (("Standard metafile\n"));
#ifdef GOFFICE_EMF_SUPPORT
		type = 2 ;
		offset = 18;
		break;
#else
		return FALSE;
#endif
	case 1:
#ifdef GOFFICE_EMF_SUPPORT
		type = 3;
		rid = 1;
		break;
#else
		{
			GOEmfState state;
			GOImage *image = GO_IMAGE (emf);
			data = gsf_input_read (input, 4, NULL);
			if (!data)
				break;
			rsize = GSF_LE_GET_GUINT32 (data) - 8;
			if ((offset += rsize) > fsize)
				break;
			state.length = rsize;
			state.data = gsf_input_read (input, rsize, NULL);
			state.group = emf->group;
			go_emf_header (&state);
			image->width = (state.mmbounds.right - state.mmbounds.left) / 2540. * 72.;
			image->height = (state.mmbounds.bottom - state.mmbounds.top) / 2540. * 72.;
		}

		return FALSE;
#endif
	default:
		d_ (("Unknown type\n"));
	}

#ifdef GOFFICE_EMF_SUPPORT
	if (1 == type || 2 == type) {
		mypg = malloc (sizeof (GOWmfPage));
		mrecords = go_wmf_init_recs ();
		escrecords = go_wmf_init_esc ();
		go_wmf_init_page (mypg);
		objs = g_hash_table_new (g_direct_hash, g_direct_equal);
		dc = &g_array_index (mypg->dcs, GOWmfDC, mypg->curdc);
		if (1 == type) {
			gsf_input_seek (input, 6, G_SEEK_SET);
			go_wmf_read_point (input, &x1, &y1);
			go_wmf_read_point (input, &x2, &y2);
			w = abs (x2 - x1);
			h = abs (y2 - y1);
			dc->VPx = mypg->w;
			dc->VPy = mypg->w * h / w;
			if (mypg->w * h / w > mypg->h) {
				dc->VPy = mypg->h;
				dc->VPx = mypg->h * w / h;
			}
			mypg->zoom = dc->VPx / w;
			mypg->type = 1;
			dc->Wx = w;
			dc->Wy = h;
		} else {
			mypg->type = 2;
		}

		gsf_input_seek (input, offset, G_SEEK_SET);
		while (offset < fsize - 6) {  /* check if it's end of file already */
			data = gsf_input_read (input, 4, NULL);
			rsize = GSF_LE_GET_GUINT32 (data);
			data = gsf_input_read (input, 2, NULL);
			rid = GSF_LE_GET_GINT16 (data);
			if (0 == rid || offset + rsize * 2 > fsize)
				break;
			if (0x626 != rid) {
				mr = GPOINTER_TO_INT (g_hash_table_lookup (mrecords, GINT_TO_POINTER (rid)));
				d_ (("Offset: %d, Rid: %x, MR: %d, RSize: %d\n", offset, rid, mr, rsize));
				go_wmf_mfrec_dump[mr] (input, rsize, mypg, objs, emf->group);
			} else {
				data = gsf_input_read (input, 2, NULL);
				escfunc = GSF_LE_GET_GINT16 (data);
				d_ (("ESCAPE! Function is %04x -- %s. Size: %d bytes.\n", escfunc, (char*) g_hash_table_lookup (escrecords, GINT_TO_POINTER (escfunc)), rsize * 2 - 6));
				if (0xf == escfunc)
					go_wmf_mr58 (input, rsize, mypg, objs, emf->group);
				gsf_input_seek (input, -2, G_SEEK_CUR);
			}
			gsf_input_seek (input, rsize * 2 - 6, G_SEEK_CUR);
			offset += rsize * 2;
		}
		g_free (mypg);
		g_hash_table_destroy (mrecords);
		g_hash_table_destroy (escrecords);
		return TRUE;
	} else if (type == 3) {
		GOEmfState state;
		state.version = 3;
		state.group = emf->group;
		state.error = error;
		state.map_mode = 0;
		state.dc_stack = NULL;
		state.curDC = g_new0 (GOEmfDC, 1);
		state.curDC->style = go_style_new ();
		state.curDC->group = state.group;
		goc_group_freeze (state.group, TRUE);
		state.curDC->text_color = GO_COLOR_BLACK;
		state.dx = state.dy = state.wx = state.wy = 0.;
		state.dw = state.dh = state.ww = state.wh = 1.;
		state.mfobjs = g_hash_table_new_full (g_direct_hash, g_direct_equal,
		                                     NULL, g_free);
		cairo_matrix_init_identity (&state.curDC->m);
		offset = 4;
		while ((offset += 4) < fsize && rid != 0xe) { /* EOF */
			data = gsf_input_read (input, 4, NULL);
			if (!data)
				break;
			rsize = GSF_LE_GET_GUINT32 (data) - 8;
			if ((offset += rsize) > fsize)
				break;
			state.length = rsize;
			state.data = gsf_input_read (input, rsize, NULL);
			if (rsize > 0 && !state.data)
					break;
			if (!go_emf_handlers[rid] (&state))
				break;
			if (offset + 4 >= fsize)
				break;
			data = gsf_input_read (input, 4, NULL);
			if (!data)
				break;
			rid = GSF_LE_GET_GUINT32 (data);
		}
		goc_group_freeze (state.group, FALSE);
		go_emf_dc_free (state.curDC);
		g_hash_table_destroy (state.mfobjs);
		if (state.dc_stack != NULL) {
			g_slist_free_full (state.dc_stack, (GDestroyNotify) go_emf_dc_free);
			if (error)
				if (*error == NULL)
					*error = g_error_new (go_error_invalid (), 0,
					                      _("Invalid image data\n"));
		}
		if (rid != 0xe) {
			if (error) {
				if (*error == NULL)
					*error = g_error_new (go_error_invalid (), 0,
					                      _("Invalid image data\n"));
			}
			return FALSE;
		} else {
			go_emf_handlers[rid] (&state); /* just for debugging */
			emf->parent.width = (state.mmbounds.right - state.mmbounds.left) / 2540. * 72.;
			emf->parent.height = (state.mmbounds.bottom - state.mmbounds.top) / 2540. * 72.;
			return TRUE;
		}
	}
#endif
	return FALSE;
}
