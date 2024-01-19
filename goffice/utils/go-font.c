/*
 * go-font.c :
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
#include "go-font.h"
#include "go-glib-extras.h"
#include <pango/pango-layout.h>
#include <string.h>

/**
 * GOFontScript:
 * @GO_FONT_SCRIPT_SUB: subscript.
 * @GO_FONT_SCRIPT_STANDARD: normal.
 * @GO_FONT_SCRIPT_SUPER: superscript.
 **/

static GHashTable	*font_hash;
static GPtrArray	*font_array;
static GSList		*font_watchers;
static GOFont const	*font_default;

#if 0
#define ref_debug(x)	x
#else
#define ref_debug(x)	do { } while (0)
#endif

GType
go_font_get_type (void)
{
	static GType type = 0;

	if (type == 0)
		type = g_boxed_type_register_static
			("GOFont",
			 (GBoxedCopyFunc) go_font_ref,
			 (GBoxedFreeFunc) go_font_unref);

    return type;
}

static void
go_font_free (GOFont *font)
{
	g_return_if_fail (font->ref_count == 1);
	pango_font_description_free (font->desc);
	g_free (font);
}

/**
 * go_font_new_by_desc:
 * @desc: (transfer full): #PangoFontDescription
 *
 * Returns: (transfer full): a font that matches @desc.
 **/
GOFont const *
go_font_new_by_desc (PangoFontDescription *desc)
{
	GOFont *font = g_hash_table_lookup (font_hash, desc);

	if (font == NULL) {
		int i = font_array->len;

		while (i-- > 0 && g_ptr_array_index (font_array, i) != NULL)
			;

		font = g_new0 (GOFont, 1);
		font->desc = desc; /* absorb it */
		font->ref_count = 1; /* one for the hash */
		ref_debug (g_warning ("created %p = 1", font););
		if (i < 0) {
			i = font_array->len;
			g_ptr_array_add (font_array, font);
		} else
			g_ptr_array_index (font_array, i) = font;
		font->font_index = i;
		g_hash_table_insert (font_hash, font->desc, font);
	} else
		pango_font_description_free (desc);	/* free it */

	return go_font_ref (font); /* and another ref for the result */
}

GOFont const *
go_font_new_by_name  (char const *str)
{
	return go_font_new_by_desc (pango_font_description_from_string (str));
}

GOFont const *
go_font_new_by_index (unsigned i)
{
	GOFont const *font;
	g_return_val_if_fail (i < font_array->len, NULL);

	font = g_ptr_array_index (font_array, i);
	return font ? go_font_ref (font): NULL;
}

char *
go_font_as_str (GOFont const *font)
{
	g_return_val_if_fail (font != NULL, g_strdup (""));
	return pango_font_description_to_string (font->desc);
}

GOFont const *
go_font_ref (GOFont const *font)
{
	g_return_val_if_fail (font != NULL, NULL);
	((GOFont *)font)->ref_count++;
	ref_debug (g_warning ("ref added %p = %d", font, font->ref_count););
	return font;
}

void
go_font_unref (GOFont const *font)
{
	g_return_if_fail (font != NULL);

	if (--((GOFont *)font)->ref_count == 1) {
		GValue instance_and_params[2];
		GSList *ptr;

		for (ptr = font_watchers; ptr != NULL ; ptr = ptr->next) {
			GClosure *watcher = ptr->data;
			gpointer data = watcher->is_invalid ?
				NULL : watcher->data;

			instance_and_params[0].g_type = 0;
			g_value_init (&instance_and_params[0], G_TYPE_POINTER);
			g_value_set_pointer (&instance_and_params[0], (gpointer)font);

			instance_and_params[1].g_type = 0;
			g_value_init (&instance_and_params[1], G_TYPE_POINTER);
			g_value_set_pointer (&instance_and_params[1], data);

			g_closure_invoke (watcher, NULL, 2,
				instance_and_params, NULL);
		}
		g_ptr_array_index (font_array, font->font_index) = NULL;
		g_hash_table_remove (font_hash, font->desc);
		ref_debug (g_warning ("unref removed %p = 1 (and deleted)", font););
	} else
		ref_debug (g_warning ("unref removed %p = %d", font, font->ref_count););
}

gboolean
go_font_eq (GOFont const *a, GOFont const *b)
{
	return pango_font_description_equal (a->desc, b->desc);
}

void
go_font_cache_register (GClosure *watcher)
{
	g_return_if_fail (watcher != NULL);

	font_watchers = g_slist_prepend (font_watchers, watcher);
	g_closure_set_marshal (watcher,
		g_cclosure_marshal_VOID__POINTER);
}

void
go_font_cache_unregister (GClosure *watcher)
{
	font_watchers = g_slist_remove (font_watchers, watcher);
}

/**
 * go_fonts_list_families:
 * @context: #PangoContext
 *
 * Returns: (element-type utf8) (transfer full): a sorted list of strings of
 * font family names.
 **/
GSList *
go_fonts_list_families (PangoContext *context)
{
	PangoFontFamily **pango_families;
	int i, n_families;
	GSList *res = NULL;

	pango_context_list_families (context, &pango_families, &n_families);
	for (i = 0 ; i < n_families ; i++) {
		const char *name = pango_font_family_get_name (pango_families[i]);
		if (name)
			res = g_slist_prepend (res, g_strdup (name));
	}
	g_free (pango_families);

	res = g_slist_sort (res, (GCompareFunc)g_utf8_collate);
	return res;
}

/**
 * go_fonts_list_sizes:
 *
 * Returns: (element-type guint) (transfer container):  a sorted list of font
 * sizes in Pango units.
 **/
GSList *
go_fonts_list_sizes (void)
{
	return go_slist_create
		(GUINT_TO_POINTER (4 * PANGO_SCALE),
		 GUINT_TO_POINTER (8 * PANGO_SCALE),
		 GUINT_TO_POINTER (9 * PANGO_SCALE),
		 GUINT_TO_POINTER (10 * PANGO_SCALE),
		 GUINT_TO_POINTER (11 * PANGO_SCALE),
		 GUINT_TO_POINTER (12 * PANGO_SCALE),
		 GUINT_TO_POINTER (14 * PANGO_SCALE),
		 GUINT_TO_POINTER (16 * PANGO_SCALE),
		 GUINT_TO_POINTER (18 * PANGO_SCALE),
		 GUINT_TO_POINTER (20 * PANGO_SCALE),
		 GUINT_TO_POINTER (22 * PANGO_SCALE),
		 GUINT_TO_POINTER (24 * PANGO_SCALE),
		 GUINT_TO_POINTER (26 * PANGO_SCALE),
		 GUINT_TO_POINTER (28 * PANGO_SCALE),
		 GUINT_TO_POINTER (36 * PANGO_SCALE),
		 GUINT_TO_POINTER (48 * PANGO_SCALE),
		 GUINT_TO_POINTER (72 * PANGO_SCALE),
		 NULL);
}



GOFontMetrics *
go_font_metrics_new (PangoContext *context, GOFont const *font)
{
	static gunichar thin_spaces[] = {
		0x200a, /* Hair space */
		0x2009, /* Thin space */
		0x2008, /* Puctuation space */
		0x2006, /* Six-per-em space */
		0x2007, /* Figure space */
		0x2005, /* Four-per-em space */
		0x2004, /* Three-per-em space */
		0x2003, /* Em space */
		0x2002, /* En space */
		0x202f  /* Narrow no-break space */
	};
	PangoLayout *layout = pango_layout_new (context);
	GOFontMetrics *res = g_new0 (GOFontMetrics, 1);
	int i, sumw = 0;
	int space_height;

	pango_layout_set_font_description (layout, font->desc);
	res->min_digit_width = INT_MAX;
	for (i = 0; i <= 9; i++) {
		char c = '0' + i;
		int w;

		pango_layout_set_text (layout, &c, 1);
		pango_layout_get_size (layout, &w, NULL);

		res->digit_widths[i] = w;

		w = MAX (w, PANGO_SCALE);  /* At least one pixel.  */
		res->min_digit_width = MIN (w, res->min_digit_width);
		res->max_digit_width = MAX (w, res->max_digit_width);
		sumw += w;
	}
	res->avg_digit_width = (sumw + 5) / 10;

	pango_layout_set_text (layout, "-", -1);
	pango_layout_get_size (layout, &res->hyphen_width, NULL);

	pango_layout_set_text (layout, "\xe2\x88\x92", -1);
	pango_layout_get_size (layout, &res->minus_width, NULL);

	pango_layout_set_text (layout, "+", -1);
	pango_layout_get_size (layout, &res->plus_width, NULL);

	pango_layout_set_text (layout, "E", -1);
	pango_layout_get_size (layout, &res->E_width, NULL);

	pango_layout_set_text (layout, "#", -1);
	pango_layout_get_size (layout, &res->hash_width, NULL);

	pango_layout_set_text (layout, " ", -1);
	pango_layout_get_size (layout, &res->space_width, &space_height);

	res->thin_space = 0;
	res->thin_space_width = 0;
	for (i = 0; i < (int)G_N_ELEMENTS (thin_spaces); i++) {
		gunichar uc = thin_spaces[i];
		char ucs[8];
		int w, h;

		ucs[g_unichar_to_utf8 (uc, ucs)] = 0;

		pango_layout_set_text (layout, ucs, -1);
		pango_layout_get_size (layout, &w, &h);

		if (w > 0 && w < res->space_width && h <= space_height &&
		    (!res->thin_space || w < res->thin_space_width)) {
			res->thin_space = uc;
			res->thin_space_width = w;
		}
	}

	g_object_unref (layout);

	return res;
}


void
go_font_metrics_free (GOFontMetrics *metrics)
{
	g_free (metrics);
}

static GOFontMetrics *
go_font_metrics_copy (GOFontMetrics *file_permissions)
{
	GOFontMetrics *res = g_new (GOFontMetrics, 1);
	memcpy (res, file_permissions, sizeof(GOFontMetrics));
	return res;
}

GType
go_font_metrics_get_type (void)
{
	static GType t = 0;

	if (t == 0) {
		t = g_boxed_type_register_static ("GOFontMetrics",
			 (GBoxedCopyFunc)go_font_metrics_copy,
			 (GBoxedFreeFunc)go_font_metrics_free);
	}
	return t;
}

static GOFontMetrics go_font_metrics_unit_var;

/* All widths == 1.  */
const GOFontMetrics *go_font_metrics_unit = &go_font_metrics_unit_var;

/**
 * _go_fonts_init: (skip)
 */
void
_go_fonts_init (void)
{
	int i;

	go_font_metrics_unit_var.min_digit_width = 1;
	go_font_metrics_unit_var.max_digit_width = 1;
	go_font_metrics_unit_var.avg_digit_width = 1;
	go_font_metrics_unit_var.hyphen_width = 1;
	go_font_metrics_unit_var.minus_width = 1;
	go_font_metrics_unit_var.plus_width = 1;
	go_font_metrics_unit_var.E_width = 1;
	go_font_metrics_unit_var.hash_width = 1;
	for (i = 0; i <= 9; i++)
		go_font_metrics_unit_var.digit_widths[i] = 1;
	go_font_metrics_unit_var.space_width = 1;
	go_font_metrics_unit_var.thin_space = 0;
	go_font_metrics_unit_var.thin_space_width = 1;

	font_array = g_ptr_array_new ();
	font_hash = g_hash_table_new_full (
		(GHashFunc)pango_font_description_hash,
		(GEqualFunc)pango_font_description_equal,
		NULL, (GDestroyNotify) go_font_free);

	/* This will become font #0 available through go_font_new_by_index.  */
	font_default = go_font_new_by_desc (
		pango_font_description_from_string ("Sans 8"));
}

/**
 * _go_fonts_shutdown: (skip)
 */
void
_go_fonts_shutdown (void)
{
	go_font_unref (font_default);
	font_default = NULL;
	g_ptr_array_free (font_array, TRUE);
	font_array = NULL;
	g_hash_table_destroy (font_hash);
	font_hash = NULL;

	if (font_watchers != NULL) {
		g_warning ("Missing calls to go_font_cache_unregister");
		/* be careful and _leak_ the closured in case they are already freed */
		g_slist_free (font_watchers);
		font_watchers = NULL;
	}
}
