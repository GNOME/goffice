/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-format.c :
 *
 * Copyright (C) 2003-2004 Jody Goldberg (jody@gnome.org)
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include <goffice/goffice-config.h>
#include "go-format.h"
#include "go-math.h"
#include "format.h"
#include "datetime.h"
#include "format-impl.h"
#include <string.h>

GOFormat *
go_format_new_from_XL (char const *descriptor_string, gboolean delocalize)
{
	return style_format_new_XL (descriptor_string, delocalize);
}

char *
go_format_as_XL	(GOFormat const *fmt, gboolean localized)
{
	return style_format_as_XL (fmt, localized);
}

GOFormat *
go_format_ref (GOFormat *fmt)
{
	style_format_ref (fmt);
	return fmt;
}

void
go_format_unref (GOFormat *fmt)
{
	style_format_unref (fmt);
}

static gboolean
go_style_format_condition (StyleFormatEntry const *entry, double val)
{
	if (entry->restriction_type == '*')
		return TRUE;

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
}

void
go_format_value_gstring (GOFormat const *format, GString *res, double val,
			 int col_width, GODateConventions const *date_conv)
{
	StyleFormatEntry const *entry = NULL; /* default to General */
	GSList const *list = NULL;
	gboolean need_abs = FALSE;

	if (format != NULL) {
		for (list = format->entries; list; list = list->next)
			if (go_style_format_condition (list->data, val))
				break;
		if (list == NULL)
			list = format->entries;
	}

	/* If nothing matches treat it as General */
	if (list != NULL) {
		entry = list->data;

		/* Empty formats should be ignored */
		if (entry->format[0] == '\0')
			return;

#if 0
		if (go_color && entry->go_color != 0)
			*go_color = entry->go_color;
#endif

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

	/* More than one format? -- abs the value.  */
	need_abs = entry && format->entries->next;

	if (INT_MAX >= val && val >= INT_MIN && val == floor (val)) {
		int i_val = (int)val;
		if (need_abs)
			i_val = ABS (i_val);

		if (entry == NULL)
			go_fmt_general_int (res, i_val, col_width);
		else
			go_format_number (res, i_val, col_width, entry, date_conv);
	} else {
		if (need_abs)
			val = fabs (val);

		if (entry == NULL)
			go_fmt_general_float (res, val, col_width);
		else
			go_format_number (res, val, col_width, entry, date_conv);
	}
}

char *
go_format_value (GOFormat const *fmt, double val)
{
	GString *res;

	if (!go_finite (val))
		return g_strdup ("#VALUE!");

	res = g_string_sized_new (20);
	go_format_value_gstring (fmt, res, val, -1, NULL);
	return g_string_free (res, FALSE);
}

gboolean
go_format_eq (GOFormat const *a, GOFormat const *b)
{
	if (a == NULL)
		return b == NULL;
	if (b == NULL)
		return FALSE;
	return style_format_equal (a, b);
}

/**
 * go_format_general :
 * 
 * Returns the 'General' #GOFormat but does not add a reference
 **/
GOFormat *
go_format_general (void)
{
	return style_format_general ();
}

/**
 * go_format_default_date :
 * 
 * Returns the default date #GOFormat but does not add a reference
 **/
GOFormat *
go_format_default_date (void)
{
	return style_format_default_date ();
}

/**
 * go_format_default_time :
 * 
 * Returns the default time #GOFormat but does not add a reference
 **/
GOFormat *
go_format_default_time (void)
{
	return style_format_default_time ();
}

/**
 * go_format_default_percentage :
 * 
 * Returns the default percentage #GOFormat but does not add a reference
 **/
GOFormat *
go_format_default_percentage (void)
{
	return style_format_default_percentage ();
}

/**
 * go_format_default_money :
 * 
 * Returns the default money #GOFormat but does not add a reference
 **/
GOFormat *
go_format_default_money	(void)
{
	return style_format_default_money ();
}
