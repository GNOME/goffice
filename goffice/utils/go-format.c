/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-format.c :
 *
 * Copyright (C) 2003-2005 Jody Goldberg (jody@gnome.org)
 * Copyright (C) 2006 Morten Welinder (terra@gnome.org)
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
#include <glib/gi18n-lib.h>
#include "go-format.h"
#include "go-font.h"
#include "go-math.h"
#include "datetime.h"
#include "go-glib-extras.h"
#include "format-impl.h"
#include <string.h>

static gboolean
go_style_format_condition (GOFormatElement const *entry, double val, char type)
{
	if (entry->restriction_type == '*')
		return TRUE;

	switch (type) {
	case 'B': case 'S':
		return entry->restriction_type == '@';

	case 'F':
		switch (entry->restriction_type) {
		case '<': return val < entry->restriction_value;
		case '>': return val > entry->restriction_value;
		case '=': return val == entry->restriction_value;
		case ',': return val <= entry->restriction_value;
		case '.': return val >= entry->restriction_value;
		case '+': return val != entry->restriction_value;
		default:
			return FALSE;
		}

	case 'E':
	default:
		return FALSE;
	}
}

static GOFormatElement const *
go_style_format_find_entry (GOFormat const *format, double val, char type,
			    GOColor *go_color, gboolean *need_abs,
			    gboolean *empty)
{
	GOFormatElement const *entry = NULL;

	if (go_color)
		*go_color = 0;

	if (format) {
		GSList *ptr;
		GOFormatElement const *last_entry = NULL;

		for (ptr = format->entries; ptr; ptr = ptr->next) {
			last_entry = ptr->data;			
			/* 142474 : only set entry if it matches */
			if (go_style_format_condition (ptr->data, val, type)) {
				entry = last_entry;
				break;
			}
		}

		/*
		 * 356140: floating point values need to use the last format
		 * if nothing else matched.
		 */
		if (entry == NULL && type == 'F')
			entry = last_entry;

		if (entry != NULL) {
			/* Empty formats should be ignored */
			if (entry->format[0] == '\0') {
				*empty = TRUE;
				return entry;
			}

			if (go_color && entry->go_color != 0)
				*go_color = entry->go_color;

			if (strcmp (entry->format, "@") == 0) {
				/* FIXME : Formatting a value as a text returns
				 * the entered text.  We need access to the
				 * parse format */
				entry = NULL;

			/* FIXME : Just containing General is enough to be
			 * general for now.  We'll ignore prefixes and suffixes
			 * for the time being */
			} else if (strstr (entry->format, "General") != NULL)
				entry = NULL;
		}
	}

	/* More than one format? -- abs the value.  */
	*need_abs = entry && format->entries->next;
	*empty = FALSE;

	return entry;
}

GOFormatNumberError
go_format_value_gstring (PangoLayout *layout, GString *str,
			 const GOFormatMeasure measure,
			 const GOFontMetrics *metrics,
			 GOFormat const *format,
			 double val, char type, const char *sval,
			 int col_width,
			 GODateConventions const *date_conv,
			 gboolean unicode_minus)
{
	GOFormatElement const *entry;
	gboolean need_abs, empty;

	g_return_val_if_fail (type == 'F' || sval != NULL, (GOFormatNumberError)-1);
	g_return_val_if_fail (str != NULL, (GOFormatNumberError)-1);

	entry = go_style_format_find_entry (format, val, type, NULL, &need_abs, &empty);

	/* Empty formats should be ignored */
	if (empty) {
		if (layout) pango_layout_set_text (layout, "", -1);
		return GO_FORMAT_NUMBER_OK;
	}

	if (type == 'F') {
		if (!go_finite (val)) {
			const char *text = _("#VALUE!");
			if (layout) pango_layout_set_text (layout, text, -1);
			g_string_append (str, text);
			return GO_FORMAT_NUMBER_OK;
		}

		if (entry == NULL) {
			go_render_general (layout, str, measure, metrics,
					   val,
					   col_width, unicode_minus);
			return GO_FORMAT_NUMBER_OK;
		} else {
			GOFormatNumberError err;

			if (need_abs)
				val = fabs (val);

			/* FIXME: -1 kills filling here.  */
			err = go_format_number (str, val, -1, entry,
						date_conv, unicode_minus);
			if (err)
				g_string_truncate (str, 0);
			else if (layout)
				pango_layout_set_text (layout, str->str, -1);
			return err;
		}
	} else {
		if (layout) pango_layout_set_text (layout, sval, -1);
		g_string_append (str, sval);
	}

	return GO_FORMAT_NUMBER_OK;
}

/**
 * go_format_value:
 * @fmt: a #GOFormat
 * @val: value to format
 *
 * Converts @val into a string using format specified by @fmt.
 *
 * returns: a newly allocated string containing formated value.
 **/

char *
go_format_value (GOFormat const *fmt, double val)
{
	GString *res = g_string_sized_new (20);
	GOFormatNumberError err =
		go_format_value_gstring (NULL, res,
					 go_format_measure_strlen,
					 go_font_metrics_unit,
					 fmt,
					 val, 'F', NULL,
					 -1, NULL, FALSE);
	if (err) {
		/* Invalid number for format.  */
		g_string_assign (res, "#####");
	}
	return g_string_free (res, FALSE);
}
