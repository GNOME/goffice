/*
 * go-cairo.c
 *
 * Copyright (C) 2007 Emmanuel Pacaud <emmanuel.pacaud@lapp.in2p3.fr>
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

#include <goffice/utils/go-cairo.h>
#include <goffice/goffice.h>
#include <math.h>

/**
 * go_cairo_emit_svg_path:
 * @cr: a cairo context
 * @path: a SVG path
 *
 * Emits a path described as a SVG path string (d property of path elements) to
 * a cairo context.
 **/

void
go_cairo_emit_svg_path (cairo_t *cr, char const *path)
{
	GOPath *go_path;

	if (path == NULL)
		return;

	go_path = go_path_new_from_svg (path);
	go_path_to_cairo (go_path, GO_PATH_DIRECTION_FORWARD, cr);
	go_path_free (go_path);
}

gboolean
go_cairo_surface_is_vector (cairo_surface_t const *surface)
{
	cairo_surface_type_t type;

	type = cairo_surface_get_type ((cairo_surface_t *) surface);

	return (type == CAIRO_SURFACE_TYPE_SVG ||
		type == CAIRO_SURFACE_TYPE_PDF ||
		type == CAIRO_SURFACE_TYPE_PS);
}

/**
 * go_cairo_convert_data_from_pixbuf:
 * @src: a pointer to pixel data in pixbuf format
 * @dst: a pointer to pixel data in cairo format
 * @width: image width
 * @height: image height
 * @rowstride: data rowstride
 *
 * Converts the pixel data stored in @src in GDK_COLORSPACE_RGB pixbuf
 * format to CAIRO_FORMAT_ARGB32 cairo format and move them
 * to @dst. If @src == @dst, pixel are converted in place.
 **/

void
go_cairo_convert_data_from_pixbuf (unsigned char *dst, unsigned char const *src,
				   int width, int height, int rowstride)
{
	int i,j;
	unsigned int t;

	g_return_if_fail (dst != NULL);

#define MULT(d,c,a,t) G_STMT_START { t = c * a + 0x7f; d = ((t >> 8) + t) >> 8; } G_STMT_END

	if (src == dst || src == NULL) {
		unsigned char a, b, c;

		for (i = 0; i < height; i++) {
			for (j = 0; j < width; j++) {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
				MULT(a, dst[2], dst[3], t);
				MULT(b, dst[1], dst[3], t);
				MULT(c, dst[0], dst[3], t);
				dst[0] = a;
				dst[1] = b;
				dst[2] = c;
#else
				MULT(a, dst[2], dst[3], t);
				MULT(b, dst[1], dst[3], t);
				MULT(c, dst[0], dst[3], t);

				dst[0] = dst[3];
				dst[3] = a;
				dst[2] = b;
				dst[1] = c;
#endif
				dst += 4;
			}
			dst += rowstride - width * 4;
		}
	} else {
		for (i = 0; i < height; i++) {
			for (j = 0; j < width; j++) {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
				MULT(dst[0], src[2], src[3], t);
				MULT(dst[1], src[1], src[3], t);
				MULT(dst[2], src[0], src[3], t);
				dst[3] = src[3];
#else
				MULT(dst[3], src[2], src[3], t);
				MULT(dst[2], src[1], src[3], t);
				MULT(dst[1], src[0], src[3], t);
				dst[0] = src[3];
#endif
				src += 4;
				dst += 4;
			}
			src += rowstride - width * 4;
			dst += rowstride - width * 4;
		}
	}
#undef MULT
}

/**
 * go_cairo_convert_data_to_pixbuf:
 * @src: a pointer to pixel data in cairo format
 * @dst: a pointer to pixel data in pixbuf format
 * @width: image width
 * @height: image height
 * @rowstride: data rowstride
 *
 * Converts the pixel data stored in @src in CAIRO_FORMAT_ARGB32 cairo format
 * to GDK_COLORSPACE_RGB pixbuf format and move them
 * to @dst. If @src == @dst, pixel are converted in place.
 **/

void
go_cairo_convert_data_to_pixbuf (unsigned char *dst, unsigned char const *src,
				 int width, int height, int rowstride)
{
	int i,j;
	unsigned int t;
	unsigned char a, b, c;

	g_return_if_fail (dst != NULL);

#define MULT(d,c,a,t) G_STMT_START { t = (a)? c * 255 / a: 0; d = t;} G_STMT_END

	if (src == dst || src == NULL) {
		for (i = 0; i < height; i++) {
			for (j = 0; j < width; j++) {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
				MULT(a, dst[2], dst[3], t);
				MULT(b, dst[1], dst[3], t);
				MULT(c, dst[0], dst[3], t);
				dst[0] = a;
				dst[1] = b;
				dst[2] = c;
#else
				MULT(a, dst[1], dst[0], t);
				MULT(b, dst[2], dst[0], t);
				MULT(c, dst[3], dst[0], t);
				dst[3] = dst[0];
				dst[0] = a;
				dst[1] = b;
				dst[2] = c;
#endif
				dst += 4;
			}
			dst += rowstride - width * 4;
		}
	} else {
		for (i = 0; i < height; i++) {
			for (j = 0; j < width; j++) {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
				MULT(dst[0], src[2], src[3], t);
				MULT(dst[1], src[1], src[3], t);
				MULT(dst[2], src[0], src[3], t);
				dst[3] = src[3];
#else
				MULT(dst[0], src[1], src[0], t);
				MULT(dst[1], src[2], src[0], t);
				MULT(dst[2], src[3], src[0], t);
				dst[3] = src[0];
#endif
				src += 4;
				dst += 4;
			}
			src += rowstride - width * 4;
			dst += rowstride - width * 4;
		}
	}
#undef MULT
}
