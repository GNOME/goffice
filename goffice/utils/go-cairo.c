/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-cairo.c
 *
 * Copyright (C) 2007 Emmanuel Pacaud <emmanuel.pacaud@lapp.in2p3.fr>
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

#include <goffice/utils/go-cairo.h>
#include <math.h>

static void
skip_spaces (char **path)
{
	while (**path == ' ')
		(*path)++;
}

static void
skip_comma_and_spaces (char **path)
{
	while (**path == ' ' || **path == ',')
		(*path)++;
}

static gboolean
parse_value (char **path, double *x)
{
	char *end, *c;
	gboolean integer_part = FALSE;
	gboolean fractional_part = FALSE;
	gboolean exponent_part = FALSE;
	double mantissa = 0.0;
	double exponent =0.0;
	double divisor;
	gboolean mantissa_sign = 1.0;
	gboolean exponent_sign = 1.0;

	c = *path;

	if (*c == '-') {
		mantissa_sign = -1.0;
		c++;
	} else if (*c == '+')
		c++;

	if (*c >= '0' && *c <= '9') {
		integer_part = TRUE;
		mantissa = *c - '0';
		c++;

		while (*c >= '0' && *c <= '9') {
			mantissa = mantissa * 10.0 + *c - '0';
			c++;
		}
	}


	if (*c == '.')
		c++;
	else if (!integer_part)
		return FALSE;

	if (*c >= '0' && *c <= '9') {
		fractional_part = TRUE;
		mantissa += (*c - '0') * 0.1;
		divisor = 0.01;
		c++;

		while (*c >= '0' && *c <= '9') {
			mantissa += (*c - '0') * divisor;
			divisor *= 0.1;
			c++;
		}
	}

	if (!fractional_part && !integer_part)
		return FALSE;

	end = c;

	if (*c == 'E' || *c == 'e') {
		c++;

		if (*c == '-') {
			exponent_sign = -1.0;
			c++;
		} else if (*c == '+')
			c++;

		if (*c >= '0' && *c <= '9') {
			exponent_part = TRUE;
			exponent = *c - '0';
			c++;

			while (*c >= '0' && *c <= '9') {
				exponent = exponent * 10.0 + *c - '0';
				c++;
			}
		}

	}

	if (exponent_part) {
		end = c;
		*x = mantissa_sign * mantissa * pow (10.0, exponent_sign * exponent);
	} else
		*x = mantissa_sign * mantissa;

	*path = end;

	return TRUE;
}

static gboolean
parse_values (char **path, unsigned int n_values, double *values)
{
	char *ptr = *path;
	unsigned int i;

	skip_comma_and_spaces (path);

	for (i = 0; i < n_values; i++) {
		if (!parse_value (path, &values[i])) {
			*path = ptr;
			return FALSE;
		}
		skip_comma_and_spaces (path);
	}

	return TRUE;
}

static void
emit_function_2 (char **path, cairo_t *cr,
		 void (*cairo_func) (cairo_t *, double, double))
{
	double values[2];

	skip_spaces (path);

	while (parse_values (path, 2, values))
		cairo_func (cr, values[0], values[1]);
}

static void
emit_function_6 (char **path, cairo_t *cr,
		 void (*cairo_func) (cairo_t *, double, double, double ,double, double, double))
{
	double values[6];

	skip_spaces (path);

	while (parse_values (path, 6, values))
		cairo_func (cr, values[0], values[1], values[2], values[3], values[4], values[5]);
}

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
	char *ptr;

	if (path == NULL)
		return;

	ptr = (char *) path;

	skip_spaces (&ptr);

	while (*ptr != '\0') {
		if (*ptr == 'M') {
			ptr++;
			emit_function_2 (&ptr, cr, cairo_move_to);
		} else if (*ptr == 'm') {
			ptr++;
			emit_function_2 (&ptr, cr, cairo_rel_move_to);
		} else if (*ptr == 'L') {
			ptr++;
			emit_function_2 (&ptr, cr, cairo_line_to);
		} else if (*ptr == 'l') {
			ptr++;
			emit_function_2 (&ptr, cr, cairo_rel_line_to);
		} else if (*ptr == 'C') {
			ptr++;
			emit_function_6 (&ptr, cr, cairo_curve_to);
		} else if (*ptr == 'c') {
			ptr++;
			emit_function_6 (&ptr, cr, cairo_rel_curve_to);
		} else if (*ptr == 'Z' || *ptr == 'z') {
			ptr++;
			cairo_close_path (cr);
		} else
			ptr++;
	}
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

void
go_cairo_convert_data_from_pixbuf (unsigned char *data, int width, int height, int rowstride)
{
	int i,j;
	unsigned int t;
	unsigned char a, b, c;

	g_return_if_fail (data != NULL);

#define MULT(d,c,a,t) G_STMT_START { t = c * a + 0x7f; d = ((t >> 8) + t) >> 8; } G_STMT_END

	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
			MULT(a, data[2], data[3], t);
			MULT(b, data[1], data[3], t);
			MULT(c, data[0], data[3], t);
			data[0] = a;
			data[1] = b;
			data[2] = c;
#else
			MULT(a, data[2], data[3], t);
			MULT(b, data[1], data[3], t);
			MULT(c, data[0], data[3], t);

			data[0] = data[3];
			data[3] = a;
			data[2] = b;
			data[1] = c;
#endif
			data += 4;
		}
		data += rowstride - width * 4;
	}
#undef MULT
}

void
go_cairo_convert_data_to_pixbuf (unsigned char *data, int width, int height, int rowstride)
{
	int i,j;
	unsigned int t;
	unsigned char a, b, c;

	g_return_if_fail (data != NULL);

#define MULT(d,c,a,t) G_STMT_START { t = (a)? c * 255 / a: 0; d = t;} G_STMT_END

	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
			MULT(a, data[2], data[3], t);
			MULT(b, data[1], data[3], t);
			MULT(c, data[0], data[3], t);
			data[0] = a;
			data[1] = b;
			data[2] = c;
#else
			MULT(a, data[1], data[0], t);
			MULT(b, data[2], data[0], t);
			MULT(c, data[3], data[0], t);
			data[3] = data[0];
			data[0] = a;
			data[1] = b;
			data[2] = c;
#endif
			data += 4;
		}
		data += rowstride - width * 4;
	}
#undef MULT
}
