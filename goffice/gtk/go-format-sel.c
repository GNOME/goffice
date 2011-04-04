/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-format-sel.c: A widget to select a format
 *
 * Copyright (C) 2006-2007 Morten Welinder (terra@gnome.org)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <goffice/goffice-config.h>
#include <goffice/goffice.h>
#include <goffice/utils/go-marshalers.h>
#include <goffice/gtk/go-gtk-compat.h>

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

#include <string.h>

/* The maximum number of chars in the formatting sample */
#define FORMAT_PREVIEW_MAX 25

#define SETUP_LOCALE_SWITCH char *oldlocale = NULL

#define START_LOCALE_SWITCH							\
	do {									\
		if (gfs->locale) {						\
			_go_currency_date_format_shutdown ();			\
			oldlocale = g_strdup (setlocale (LC_ALL, NULL));	\
			go_setlocale (LC_ALL, gfs->locale);			\
			_go_currency_date_format_init ();			\
		}								\
	} while (0)

#define END_LOCALE_SWITCH							\
	do {									\
		if (oldlocale) {						\
			_go_currency_date_format_shutdown ();			\
			go_setlocale (LC_ALL, oldlocale);			\
			g_free (oldlocale);					\
			_go_currency_date_format_init ();			\
		}								\
	} while (0)

#define FMT_CUSTOM ((GOFormatFamily)(GO_FORMAT_SPECIAL + 1))

/*Format Categories*/
static char const *const format_category_names[] = {
	N_("General"),
	N_("Number"),
	N_("Currency"),
	N_("Accounting"),
	N_("Date"),
	N_("Time"),
	N_("Percentage"),
	N_("Fraction"),
	N_("Scientific"),
	N_("Text"),
	N_("Special"),
	N_("Custom"),
	NULL
};

/* The available format widgets */
typedef enum {
	F_GENERAL_EXPLANATION,
	F_NUMBER_EXPLANATION,
	F_CURRENCY_EXPLANATION,
	F_ACCOUNTING_EXPLANATION,
	F_DATE_EXPLANATION,
	F_TIME_EXPLANATION,
	F_PERCENTAGE_EXPLANATION,
	F_FRACTION_EXPLANATION,
	F_SCIENTIFIC_EXPLANATION,
	F_TEXT_EXPLANATION,
	F_SPECIAL_EXPLANATION,
	F_CUSTOM_EXPLANATION,

	F_SEPARATOR,
	F_SYMBOL_LABEL,		F_SYMBOL,
	F_ENTRY,
	F_LIST_LABEL,		F_LIST_SCROLL,		F_LIST,
	F_DECIMAL_SPIN,		F_ENGINEERING_BUTTON,
	F_SUPERSCRIPT_BUTTON,	F_SUPERSCRIPT_HIDE_1_BUTTON,
	F_NEGATIVE_LABEL,	F_NEGATIVE_SCROLL,	F_NEGATIVE,
	F_DECIMAL_LABEL,	F_CODE_LABEL,
	F_MAX_WIDGET
} FormatWidget;

struct  _GOFormatSel {
	GtkHBox   box;
	GtkBuilder *gui;

	gpointer  value;
	char	 *locale;

	gboolean  enable_edit;
	gboolean  show_format_with_markup;

	GODateConventions const *date_conv;

	struct {
		GtkTextView	*preview;
		GtkWidget	*preview_box;
		GtkTextBuffer	*preview_buffer;

		GtkWidget	*widget[F_MAX_WIDGET];
		GtkWidget	*menu;
		GtkTreeModel	*menu_model;
		GtkSizeGroup    *size_group;

		struct {
			GtkTreeView 		*view;
			GtkListStore		*model;
			GtkTreeSelection 	*selection;
		} negative_types;

		struct {
			GtkTreeView	 *view;
			GtkListStore	 *model;
			GtkTreeSelection *selection;
		} formats;

		gulong          entry_changed_id;
		GOFormat const	*spec;
		gint		current_type;

		GOFormatDetails details;
	} format;
};

typedef struct {
	GtkHBoxClass parent_class;

	gboolean  (*format_changed)   (GOFormatSel *gfs, char const *fmt);
	char     *(*generate_preview) (GOFormatSel *gfs, char *fmt);
} GOFormatSelClass;

/* Signals we emit */
enum {
	FORMAT_CHANGED,
	GENERATE_PREVIEW,
	LAST_SIGNAL
};

static guint go_format_sel_signals [LAST_SIGNAL] = { 0 };

static void format_entry_set_text (GOFormatSel *gfs, const gchar *text);

static char *
generate_preview (GOFormatSel *gfs, GOColor *c)
{
	char *res = NULL;
	g_signal_emit (G_OBJECT (gfs),
		       go_format_sel_signals [GENERATE_PREVIEW], 0,
		       c, &res);
	return res;
}

static void
draw_format_preview (GOFormatSel *gfs, gboolean regen_format)
{
	char *preview;
	GOFormat const *sf = NULL;
	GOColor c = 0;
	GdkColor gdk_color;
	gsize len;

	if (regen_format) {
		GString *fmtstr = g_string_new (NULL);
		char *fmt;

		go_format_generate_str (fmtstr, &gfs->format.details);
		fmt = g_string_free (fmtstr, fmtstr->len == 0);

		if (fmt) {
			char *lfmt = go_format_str_localize (fmt);
			format_entry_set_text (gfs, lfmt);
			g_free (lfmt);
			g_free (fmt);
		}
	}

	if (NULL == (sf = gfs->format.spec))
		return;

	if (NULL == (preview = generate_preview (gfs, &c)))
		return;

	len = g_utf8_strlen (preview, -1);
	if (len > FORMAT_PREVIEW_MAX)
		strcpy (g_utf8_offset_to_pointer (preview, FORMAT_PREVIEW_MAX - 5),
			"...");

	gtk_text_buffer_set_text (gfs->format.preview_buffer, preview, -1);
	if (c != 0)
		go_color_to_gdk (c, &gdk_color);
	else
		gdk_color_parse ("black", &gdk_color);

	gtk_widget_modify_text (GTK_WIDGET (gfs->format.preview),
				GTK_STATE_NORMAL, &gdk_color);
	g_free (preview);
}

static void
fillin_negative_samples (GOFormatSel *gfs)
{
	int i;
	GtkTreeIter iter;
	gboolean more;
	GOFormatDetails details = gfs->format.details;
	double sample_value;
	SETUP_LOCALE_SWITCH;

	START_LOCALE_SWITCH;

	switch (gfs->format.current_type) {
	case GO_FORMAT_DATE:
		g_assert_not_reached ();
		return;
	case GO_FORMAT_PERCENTAGE:
		sample_value = -0.123456;
		break;
	default:
		sample_value = -3210.12345678;
		break;
	}

	more = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (gfs->format.negative_types.model), &iter);
	for (i = 0 ; i < 4; i++) {
		GString *fmtstr = g_string_new (NULL);
		GOFormat *fmt;
		char *buf;

		details.negative_red = (i & 1) != 0;
		details.negative_paren = (i & 2) != 0;

		go_format_generate_str (fmtstr, &details);
		fmt = go_format_new_from_XL (fmtstr->str);
		g_string_free (fmtstr, TRUE);
		buf = go_format_value (fmt, sample_value);
		go_format_unref (fmt);

		if (!more)
			gtk_list_store_append (gfs->format.negative_types.model, &iter);
		gtk_list_store_set (gfs->format.negative_types.model, &iter,
				    0, details.negative_red,
				    1, details.negative_paren,
				    2, buf,
				    3, details.negative_red ? "red" : NULL,
				    -1);
		if (gfs->format.details.negative_red == details.negative_red &&
		    gfs->format.details.negative_paren == details.negative_paren)
			gtk_tree_selection_select_iter
				(gfs->format.negative_types.selection,
				 &iter);

		if (more)
			more = gtk_tree_model_iter_next (GTK_TREE_MODEL (gfs->format.negative_types.model),
							 &iter);

		g_free (buf);
	}

	END_LOCALE_SWITCH;
}

static void
cb_decimals_changed (GtkSpinButton *spin, GOFormatSel *gfs)
{
	gfs->format.details.num_decimals =
		gtk_spin_button_get_value_as_int (spin);

	if (gtk_widget_get_visible (gfs->format.widget[F_NEGATIVE]))
		fillin_negative_samples (gfs);

	draw_format_preview (gfs, TRUE);
}

static void
cb_separator_toggle (GtkWidget *w, GOFormatSel *gfs)
{
	gfs->format.details.thousands_sep =
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));

	if (gtk_widget_get_visible (gfs->format.widget[F_NEGATIVE]))
		fillin_negative_samples (gfs);

	draw_format_preview (gfs, TRUE);
}

static void
cb_engineering_toggle (GtkWidget *w, GOFormatSel *gfs)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
		gfs->format.details.exponent_step = 3;
	else
		gfs->format.details.exponent_step = 1;

	draw_format_preview (gfs, TRUE);
}

static void
cb_superscript_toggle (GtkWidget *w, GOFormatSel *gfs)
{
	gfs->format.details.use_markup =
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));
	gtk_widget_set_sensitive (gfs->format.widget[F_SUPERSCRIPT_HIDE_1_BUTTON],
				  gfs->format.details.use_markup);

	draw_format_preview (gfs, TRUE);
}

static void
cb_superscript_hide_1_toggle (GtkWidget *w, GOFormatSel *gfs)
{
	gfs->format.details.simplify_mantissa =
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));

	draw_format_preview (gfs, TRUE);
}

static void
fmt_dialog_init_fmt_list (GOFormatSel *gfs, char const *const *formats,
			  GtkTreeIter *select)
{
	GOFormatMagic cur_magic = go_format_get_magic (gfs->format.spec);

	for (; *formats; formats++) {
		GtkTreeIter iter;
		char *fmt = go_format_str_localize (*formats);
		GOFormat *f = go_format_new_from_XL (*formats);
		GOFormatMagic magic = go_format_get_magic (f);
		gboolean found;

		gtk_list_store_append (gfs->format.formats.model, &iter);
		gtk_list_store_set (gfs->format.formats.model, &iter,
				    0, fmt, -1);
		g_free (fmt);

		/* Magic formats are fully defined by their magic.  */
		found = cur_magic
			? (cur_magic == magic)
			: go_format_eq (f, gfs->format.spec);

		go_format_unref (f);

		if (found)
			*select = iter;
	}
}

static const char *
find_builtin (const char *fmtstr, int page, gboolean def)
{
	int list_elem = 0;
	char const * const *candidates;

	if (page == GO_FORMAT_UNKNOWN)
		return NULL;

	candidates = _go_format_builtins[page];
	if (!candidates)
		return NULL;

	while (candidates[list_elem]) {
		if (strcmp (candidates[list_elem], fmtstr) == 0)
			break;
		list_elem++;
	}

	if (candidates[list_elem] == NULL &&
	    page == GO_FORMAT_GENERAL &&
	    g_ascii_strcasecmp (candidates[0], fmtstr) == 0)
		list_elem = 0; /* "GENERAL" */

	if (candidates[list_elem] == NULL && def)
		list_elem = 0;

	return candidates[list_elem];
}


static void
fmt_dialog_enable_widgets (GOFormatSel *gfs, int page)
{
	SETUP_LOCALE_SWITCH;
	static FormatWidget const contents[][12] = {
		/* General */
		{
			F_GENERAL_EXPLANATION,
			F_MAX_WIDGET
		},
		/* Number */
		{
			F_NUMBER_EXPLANATION,
			F_DECIMAL_LABEL,
			F_DECIMAL_SPIN,
			F_SEPARATOR,
			F_NEGATIVE_LABEL,
			F_NEGATIVE_SCROLL,
			F_NEGATIVE,
			F_MAX_WIDGET
		},
		/* Currency */
		{
			F_CURRENCY_EXPLANATION,
			F_DECIMAL_LABEL,
			F_DECIMAL_SPIN,
			F_SEPARATOR,
			F_SYMBOL_LABEL,
			F_SYMBOL,
			F_NEGATIVE_LABEL,
			F_NEGATIVE_SCROLL,
			F_NEGATIVE,
			F_MAX_WIDGET
		},
		/* Accounting */
		{
			F_ACCOUNTING_EXPLANATION,
			F_DECIMAL_LABEL,
			F_DECIMAL_SPIN,
			F_SYMBOL_LABEL,
			F_SYMBOL,
			F_MAX_WIDGET
		},
		/* Date */
		{
			F_DATE_EXPLANATION,
			F_LIST_LABEL,
			F_LIST_SCROLL,
			F_LIST,
			F_MAX_WIDGET
		},
		/* Time */
		{
			F_TIME_EXPLANATION,
			F_LIST_LABEL,
			F_LIST_SCROLL,
			F_LIST,
			F_MAX_WIDGET
		},
		/* Percentage */
		{
			F_PERCENTAGE_EXPLANATION,
			F_DECIMAL_LABEL,
			F_DECIMAL_SPIN,
			F_SEPARATOR,
			F_NEGATIVE_LABEL,
			F_NEGATIVE_SCROLL,
			F_NEGATIVE,
			F_MAX_WIDGET
		},
		/* Fraction */
		{
			F_FRACTION_EXPLANATION,
			F_LIST_LABEL,
			F_LIST_SCROLL,
			F_LIST,
			F_MAX_WIDGET
		},
		/* Scientific */
		{
			F_SCIENTIFIC_EXPLANATION,
			F_DECIMAL_LABEL,
			F_DECIMAL_SPIN,
			F_ENGINEERING_BUTTON,
			F_SUPERSCRIPT_BUTTON,
			F_SUPERSCRIPT_HIDE_1_BUTTON,
			F_MAX_WIDGET
		},
		/* Text */
		{
			F_TEXT_EXPLANATION,
			F_MAX_WIDGET
		},
		/* Special */
		{
			F_SPECIAL_EXPLANATION,
			F_MAX_WIDGET
		},
		/* Custom */
		{
			F_CUSTOM_EXPLANATION,
			F_CODE_LABEL,
			F_ENTRY,
			F_LIST_LABEL,
			F_LIST_SCROLL,
			F_LIST,
			F_MAX_WIDGET
		}
	};

	GOFormatFamily const old_page = gfs->format.current_type;
	int i;
	FormatWidget tmp;

	START_LOCALE_SWITCH;

	/* Hide widgets from old page */
	if (old_page >= 0) {
		int i, j;
		FormatWidget wi, wj;
		for (i = 0; (wi = contents[old_page][i]) != F_MAX_WIDGET ; ++i) {
			for (j = 0; (wj = contents[page][j]) != F_MAX_WIDGET ; ++j)
				if (wi == wj)
					goto stays;
			gtk_widget_hide (gfs->format.widget[wi]);
stays:
			; /* No more */
		}
	}

	/* Set the default format if appropriate */
	if (page == GO_FORMAT_GENERAL ||
	    page == GO_FORMAT_ACCOUNTING ||
	    page == GO_FORMAT_FRACTION ||
	    page == GO_FORMAT_TEXT) {
		const char *fmtstr = go_format_as_XL (gfs->format.spec);
		const char *elem = find_builtin (fmtstr, page, TRUE);
		char *lelem;

		lelem = go_format_str_localize (elem);
		format_entry_set_text (gfs, lelem);
		g_free (lelem);
	}

	gfs->format.details.family = gfs->format.current_type = page;
	for (i = 0; (tmp = contents[page][i]) != F_MAX_WIDGET ; ++i) {
		gboolean show_widget = TRUE;
		GtkWidget *w = gfs->format.widget[tmp];

		switch (tmp) {
		case F_LIST: {
			int start = 0, end = -1;
			GtkTreeIter select;

			switch (page) {
			default :
				;
			case GO_FORMAT_DATE:
			case GO_FORMAT_TIME:
			case GO_FORMAT_FRACTION:
				start = end = page;
				break;

			case FMT_CUSTOM:
				start = 0; end = 8;
				break;
			}

			select.stamp = 0;
			gtk_list_store_clear (gfs->format.formats.model);
			for (; start <= end ; ++start)
				fmt_dialog_init_fmt_list (gfs,
					_go_format_builtins[start], &select);

			/* If this is the custom page and the format has
			* not been found append it */
			/* TODO We should add the list of other custom formats created.
			*      It should be easy.  All that is needed is a way to differentiate
			*      the std formats and the custom formats in the GOFormat hash.
			*/
			if  (page == FMT_CUSTOM && select.stamp == 0) {
				const char *fmtstr = go_format_as_XL (gfs->format.spec);
				char *tmp = go_format_str_localize (fmtstr);
				if (tmp) {
					format_entry_set_text (gfs, tmp);
					g_free (tmp);
				} else {
					g_warning ("Localization of %s failed.",
						   fmtstr);
				}
			} else if (select.stamp == 0)
				if (!gtk_tree_model_get_iter_first (
					GTK_TREE_MODEL (gfs->format.formats.model),
					&select))
					break;

			if (select.stamp != 0) {
				GtkTreePath *path = gtk_tree_model_get_path (
					GTK_TREE_MODEL (gfs->format.formats.model),
					&select);
				gtk_tree_selection_select_iter (
					gfs->format.formats.selection,
					&select);
				gtk_tree_view_scroll_to_cell (gfs->format.formats.view,
					path, NULL, FALSE, 0., 0.);
				gtk_tree_path_free (path);
			}
			break;
		}

		case F_NEGATIVE:
			fillin_negative_samples (gfs);
			break;

		case F_DECIMAL_SPIN:
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (gfs->format.widget[F_DECIMAL_SPIN]),
						   gfs->format.details.num_decimals);
			break;

		case F_SUPERSCRIPT_BUTTON:
			if (gfs->show_format_with_markup) {
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
							      gfs->format.details.use_markup);
			} else
				show_widget = FALSE;
			break;

		case F_SUPERSCRIPT_HIDE_1_BUTTON:
			if (gfs->show_format_with_markup) {

				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
							      gfs->format.details.simplify_mantissa);
				gtk_widget_set_sensitive (w, gfs->format.details.use_markup);
			} else
				show_widget = FALSE;
			break;

		case F_ENGINEERING_BUTTON:
			gtk_toggle_button_set_active
				(GTK_TOGGLE_BUTTON (gfs->format.widget[F_ENGINEERING_BUTTON]),
				 gfs->format.details.exponent_step == 3);
			break;

		case F_SEPARATOR:
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gfs->format.widget[F_SEPARATOR]),
						      gfs->format.details.thousands_sep);
			break;

		default:
			; /* Nothing */
		}

		if (show_widget) {
			while (!gtk_widget_get_visible (w)) {
				gtk_widget_show (w);
				w = gtk_widget_get_parent (w);
			}
		}
	}

	draw_format_preview (gfs, TRUE);

	END_LOCALE_SWITCH;
}

/*
 * Callback routine to manage the relationship between the number
 * formating radio buttons and the widgets required for each mode.
 */

static void
cb_format_class_changed (G_GNUC_UNUSED GtkTreeSelection *ignored,
			 GOFormatSel *gfs)
{
	int selected_item = 0;
	GList *list;
	GtkTreeSelection *selection = gtk_tree_view_get_selection
		(GTK_TREE_VIEW(gfs->format.menu));

	list = gtk_tree_selection_get_selected_rows
		(selection, &gfs->format.menu_model);
	if (list) {
		GtkTreePath *path;
		path = list->data;
		selected_item = *(gtk_tree_path_get_indices (path));

		if (selected_item >= 0) {
			fmt_dialog_enable_widgets (gfs, selected_item);
		}
		g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
		g_list_free (list);
	}
}

static void
cb_format_entry_changed (GtkEditable *w, GOFormatSel *gfs)
{
	char *fmt;
	const char *cur_fmt;
	if (!gfs->enable_edit)
		return;

	fmt = go_format_str_delocalize (gtk_entry_get_text (GTK_ENTRY (w)));
	if (!fmt)
		fmt = g_strdup ("*");
	cur_fmt = go_format_as_XL (gfs->format.spec);

	if (strcmp (cur_fmt, fmt)) {
		go_format_unref (gfs->format.spec);
		gfs->format.spec = go_format_new_from_XL (fmt);
		g_signal_emit (G_OBJECT (gfs),
			       go_format_sel_signals [FORMAT_CHANGED], 0,
			       fmt);
		draw_format_preview (gfs, FALSE);
	}

	g_free (fmt);
}

/*
 * We only want to emit the number format changed signal once for each
 * format change. When not blocking signals when calling
 * gtk_entry_set_text, one would be emitted for deleting the old text
 * and one for inserting the new. That's why we block the signal and
 * invoke cb_format_entry_changed explicitly.
 */
static void
format_entry_set_text (GOFormatSel *gfs, const gchar *text)
{
	GtkEntry *entry = GTK_ENTRY (gfs->format.widget[F_ENTRY]);

	g_signal_handler_block (entry, gfs->format.entry_changed_id);
	gtk_entry_set_text (entry, text);
	g_signal_handler_unblock (entry, gfs->format.entry_changed_id);
	cb_format_entry_changed (GTK_EDITABLE (entry), gfs);
}

static void
cb_format_list_select (GtkTreeSelection *selection, GOFormatSel *gfs)
{
	GtkTreeIter iter;
	gchar *text;

	if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
		return;

	gtk_tree_model_get (GTK_TREE_MODEL (gfs->format.formats.model),
			    &iter, 0, &text, -1);
	format_entry_set_text (gfs, text);
	g_free (text);
}

static gboolean
cb_format_currency_select (G_GNUC_UNUSED GtkWidget *ct,
			   char * new_text, GOFormatSel *gfs)
{
	int i;

	/* ignore the clear while assigning a new value */
	if (!gfs->enable_edit || new_text == NULL || *new_text == '\0')
		return FALSE;

	for (i = 0; _go_format_currencies[i].symbol != NULL ; ++i) {
		GOFormatCurrency const *ci = _go_format_currencies + i;
		if (!strcmp (_(ci->description), new_text)) {
			gfs->format.details.currency = ci;
			break;
		}
	}

	if (gtk_widget_get_visible (gfs->format.widget[F_NEGATIVE]))
		fillin_negative_samples (gfs);

	draw_format_preview (gfs, TRUE);

	return TRUE;
}

static void
cb_format_negative_form_selected (GtkTreeSelection *selection, GOFormatSel *gfs)
{
	GtkTreeIter iter;
	int negative_red, negative_paren;

	if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
		return;

	gtk_tree_model_get (GTK_TREE_MODEL (gfs->format.negative_types.model),
			    &iter,
			    0, &negative_red,
			    1, &negative_paren,
			    -1);
	gfs->format.details.negative_red = negative_red;
	gfs->format.details.negative_paren = negative_paren;
	draw_format_preview (gfs, TRUE);
}

static gint
funny_currency_order (gconstpointer _a, gconstpointer _b)
{
	char const *a = (char const *)_a;
	char const *b = (char const *)_b;

	/* Keep the special 1 char versions, and both euro forms at the top */
	gboolean a1 = a[0] && (*(g_utf8_next_char (a)) == '\0' ||
			       0x20AC == g_utf8_get_char (a)); /* euro */
	gboolean b1 = b[0] && (*(g_utf8_next_char (b)) == '\0' ||
			       0x20AC == g_utf8_get_char (b)); /* euro */

	if (a1) {
		if (b1) {
			return g_utf8_collate (a, b);
		} else {
			return -1;
		}
	} else {
		if (b1) {
			return +1;
		} else {
			return g_utf8_collate (a, b);
		}
	}
}

static void
set_format_category (GOFormatSel *gfs, int row)
{
	GtkTreePath *path;
	GtkTreeSelection *selection = gtk_tree_view_get_selection
		((GTK_TREE_VIEW(gfs->format.menu)));

	path = gtk_tree_path_new_from_indices (row, -1);
	gtk_tree_selection_select_path  (selection, path);
	gtk_tree_path_free (path);
}

static GOFormatFamily
study_format (GOFormat const *fmt, GOFormatDetails *details)
{
	gboolean exact;

	go_format_get_details (fmt, details, &exact);

	if (exact) {
		/* Things we do not have GUI for: */
		if (details->family == GO_FORMAT_ACCOUNTING &&
		    !details->thousands_sep)
			exact = FALSE;

		if (details->min_digits != 1)
			exact = FALSE;
	}

	if (!exact) {
		const char *str = go_format_as_XL (fmt);
		if (!find_builtin (str, details->family, FALSE))
			details->family = FMT_CUSTOM;
	}	
	
	return details->family;
}

static void
set_format_category_menu_from_style (GOFormatSel *gfs)
{
	GOFormatFamily page;

	g_return_if_fail (GO_IS_FORMAT_SEL (gfs));

	/* Attempt to extract general parameters from the current format */
	page = study_format (gfs->format.spec, &gfs->format.details);
	if (page < 0)
		page = FMT_CUSTOM; /* Default to custom */

	set_format_category (gfs, page);
	fmt_dialog_enable_widgets (gfs, page);
}

static void
populate_menu (GOFormatSel *gfs)
{
	GtkTreeViewColumn *column;
	GtkTreeSelection  *selection;
	GtkTreeIter iter;
	GtkCellRenderer *renderer;
	char const * const *categories = format_category_names;

	gfs->format.menu_model = GTK_TREE_MODEL (gtk_list_store_new
						 (1, G_TYPE_STRING));
	gtk_tree_view_set_model (GTK_TREE_VIEW (gfs->format.menu),
				 gfs->format.menu_model);
	g_object_unref (gfs->format.menu_model);
	selection = gtk_tree_view_get_selection
		(GTK_TREE_VIEW(gfs->format.menu));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

	while (*categories) {
		gtk_list_store_append
			(GTK_LIST_STORE (gfs->format.menu_model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (gfs->format.menu_model),
				    &iter, 0, _(*categories), -1);
		categories++;
	}

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("", renderer,
							   "text", 0,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(gfs->format.menu), column);

	g_signal_connect (selection,
			  "changed",
			  G_CALLBACK (cb_format_class_changed), gfs);
}
/*
 * static void
 * fmt_dialog_init_format_page (FormatState *state)
 */

static void
nfs_init (GOFormatSel *gfs)
{
	/* The various format widgets */
	static char const *const widget_names[] = {
		"format_general_explanation",
		"format_number_explanation",
		"format_currency_explanation",
		"format_accounting_explanation",
		"format_date_explanation",
		"format_time_explanation",
		"format_percentage_explanation",
		"format_fraction_explanation",
		"format_scientific_explanation",
		"format_text_explanation",
		"format_special_explanation",
		"format_custom_explanation",

		"format_separator",
		"format_symbol_label",
		"format_symbol_select",
		"format_entry",
		"format_list_label",
		"format_list_scroll",
		"format_list",
		"format_number_decimals",
		"format_engineering_button",
		"format_superscript_button",
		"format_superscript_hide_1_button",
		"format_negatives_label",
		"format_negatives_scroll",
		"format_negatives",
		"format_decimal_label",
		"format_code_label",
		NULL
	};

	GtkWidget *tmp;
	GtkTreeViewColumn *column;
	GOComboText *combo;
	char const *name;
	int i;
	GOFormatFamily page;

	GtkWidget *toplevel;

	gfs->enable_edit = FALSE;
	gfs->show_format_with_markup = FALSE;
	gfs->locale = NULL;

	gfs->gui = go_gtk_builder_new ("go-format-sel.ui", GETTEXT_PACKAGE, NULL);
	if (gfs->gui == NULL)
		return;

	toplevel = go_gtk_builder_get_widget (gfs->gui, "number_box");
	gtk_box_pack_start (GTK_BOX (gfs), toplevel, TRUE, TRUE, 0);

	gfs->format.spec = go_format_general ();
	go_format_ref (gfs->format.spec);

	gfs->format.preview = NULL;

	/* The handlers will set the format family later.  -1 flags that
	 * all widgets are already hidden. */
	gfs->format.current_type = -1;

	study_format (gfs->format.spec, &gfs->format.details);

	gfs->format.preview_box = go_gtk_builder_get_widget (gfs->gui, "preview_box");
	gfs->format.preview = GTK_TEXT_VIEW (gtk_builder_get_object (gfs->gui, "preview"));
	{
		PangoFontMetrics *metrics;
		PangoContext *context;
		GtkWidget *w = GTK_WIDGET (gfs->format.preview);
		gint char_width;

		/* request width in number of chars */
		context = gtk_widget_get_pango_context (w);
		metrics = pango_context_get_metrics (context,
						     gtk_widget_get_style(w)->font_desc,
						     pango_context_get_language (context));
		char_width = pango_font_metrics_get_approximate_char_width (metrics);
		gtk_widget_set_size_request (w, PANGO_PIXELS (char_width) * FORMAT_PREVIEW_MAX, -1);
		pango_font_metrics_unref (metrics);
	}
	gfs->format.preview_buffer = gtk_text_view_get_buffer (gfs->format.preview);

	gfs->format.menu = go_gtk_builder_get_widget (gfs->gui, "format_menu");
	populate_menu (gfs);

	/* Collect all the required format widgets and hide them */
	for (i = 0; (name = widget_names[i]) != NULL; ++i) {
		if (i == F_SYMBOL)
			continue;
		tmp = go_gtk_builder_get_widget (gfs->gui, name);

		if (tmp == NULL) {
			g_warning ("nfs_init : failed to load widget %s", name);
		}

		g_return_if_fail (tmp != NULL);

		gtk_widget_hide (tmp);
		gfs->format.widget[i] = tmp;
	}

	gfs->format.widget[F_SYMBOL] = go_combo_text_new (NULL);
	gtk_widget_hide (gfs->format.widget[F_SYMBOL]);

	/* set minimum heights */
	gtk_widget_set_size_request (gfs->format.widget[F_LIST], -1, 100);
	gtk_widget_set_size_request (gfs->format.widget[F_NEGATIVE], -1, 100);

	/* use size group for better widget alignment */
	gfs->format.size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (gfs->format.size_group,
				   gfs->format.widget[F_SYMBOL_LABEL]);
	gtk_size_group_add_widget (gfs->format.size_group,
				   gfs->format.widget[F_DECIMAL_LABEL]);

	/* hide preview by default until a value is set */
	gtk_widget_hide (gfs->format.preview_box);

	/* setup the structure of the negative type list */
	gfs->format.negative_types.model =
		gtk_list_store_new (4,
				    G_TYPE_BOOLEAN, /* red */
				    G_TYPE_BOOLEAN, /* paren */
				    G_TYPE_STRING,  /* format text */
				    G_TYPE_STRING); /* colour */
	gfs->format.negative_types.view = GTK_TREE_VIEW (gfs->format.widget[F_NEGATIVE]);
	gtk_tree_view_set_model (gfs->format.negative_types.view,
				 GTK_TREE_MODEL (gfs->format.negative_types.model));
	g_object_unref (gfs->format.negative_types.model);

	column = gtk_tree_view_column_new_with_attributes
		(_("Negative Number Format"),
		 gtk_cell_renderer_text_new (),
		 "text", 2,
		 "foreground", 3,
		 NULL);
	gtk_tree_view_append_column (gfs->format.negative_types.view, column);
	gfs->format.negative_types.selection =
		gtk_tree_view_get_selection (gfs->format.negative_types.view);
	gtk_tree_selection_set_mode (gfs->format.negative_types.selection,
				     GTK_SELECTION_SINGLE);
	g_signal_connect (G_OBJECT (gfs->format.negative_types.selection), "changed",
		G_CALLBACK (cb_format_negative_form_selected), gfs);
	g_signal_connect (G_OBJECT (gfs->format.widget[F_DECIMAL_SPIN]), "value_changed",
		G_CALLBACK (cb_decimals_changed), gfs);
	g_signal_connect (G_OBJECT (gfs->format.widget[F_SEPARATOR]), "toggled",
		G_CALLBACK (cb_separator_toggle), gfs);
	g_signal_connect (G_OBJECT (gfs->format.widget[F_ENGINEERING_BUTTON]), "toggled",
		G_CALLBACK (cb_engineering_toggle), gfs);
	g_signal_connect (G_OBJECT (gfs->format.widget[F_SUPERSCRIPT_BUTTON]), "toggled",
		G_CALLBACK (cb_superscript_toggle), gfs);
	g_signal_connect (G_OBJECT (gfs->format.widget[F_SUPERSCRIPT_HIDE_1_BUTTON]), "toggled",
		G_CALLBACK (cb_superscript_hide_1_toggle), gfs);

	/* setup custom format list */
	gfs->format.formats.model = gtk_list_store_new (1, G_TYPE_STRING);
	gfs->format.formats.view = GTK_TREE_VIEW (gfs->format.widget[F_LIST]);
	gtk_tree_view_set_model (gfs->format.formats.view,
				 GTK_TREE_MODEL (gfs->format.formats.model));
	g_object_unref (gfs->format.formats.model);
	column = gtk_tree_view_column_new_with_attributes (_("Number Formats"),
		gtk_cell_renderer_text_new (),
		"text",		0,
		NULL);
	gtk_tree_view_append_column (gfs->format.formats.view, column);
	gfs->format.formats.selection =
		gtk_tree_view_get_selection (gfs->format.formats.view);
	gtk_tree_selection_set_mode (gfs->format.formats.selection,
		GTK_SELECTION_BROWSE);
	g_signal_connect (G_OBJECT (gfs->format.formats.selection), "changed",
		G_CALLBACK (cb_format_list_select), gfs);

	/* Setup handler Currency & Accounting currency symbols */
	combo = GO_COMBO_TEXT (gfs->format.widget[F_SYMBOL]);
	if (combo != NULL) {
		GSList *ptr, *l = NULL;
		const char *desc;

		for (i = 0; _go_format_currencies[i].symbol != NULL ; ++i)
			l = g_slist_prepend (l, _((gchar *)_go_format_currencies[i].description));
		l = g_slist_sort (l, funny_currency_order);

		for (ptr = l; ptr != NULL ; ptr = ptr->next)
			go_combo_text_add_item (combo, ptr->data);
		g_slist_free (l);

		desc = gfs->format.details.currency
			? gfs->format.details.currency->description
			: NULL;
		if (!desc)
			desc = N_("None");
		go_combo_text_set_text (combo, _(desc),
					GO_COMBO_TEXT_FROM_TOP);
		g_signal_connect (G_OBJECT (combo), "entry_changed",
			G_CALLBACK (cb_format_currency_select), gfs);
		gtk_label_set_mnemonic_widget (
			GTK_LABEL (gtk_builder_get_object (gfs->gui, "format_symbol_label")),
			GTK_WIDGET (combo));
		/* add the combo to its container */
		gtk_box_pack_start (GTK_BOX (gtk_builder_get_object (gfs->gui, "format_symbol_box")),
				    GTK_WIDGET (combo), TRUE, TRUE, 0);
	}

	/* Setup special handler for Custom */
	gfs->format.entry_changed_id = g_signal_connect (
		G_OBJECT (gfs->format.widget[F_ENTRY]), "changed",
		G_CALLBACK (cb_format_entry_changed), gfs);

	/* Connect signal for format menu */
	set_format_category_menu_from_style (gfs);

	page = go_format_get_family (gfs->format.spec);
	if (page < 0)
		page = FMT_CUSTOM; /* Default to custom */
	fmt_dialog_enable_widgets (gfs, page);

	gfs->enable_edit = TRUE;
}

static void
go_format_sel_finalize (GObject *obj)
{
	GOFormatSel *gfs = GO_FORMAT_SEL (obj);

	g_free (gfs->locale);
	gfs->locale = NULL;

	go_format_unref (gfs->format.spec);
	gfs->format.spec = NULL;

	if (gfs->format.size_group) {
		g_object_unref (gfs->format.size_group);
		gfs->format.size_group = NULL;
	}

	if (gfs->gui) {
		g_object_unref (G_OBJECT (gfs->gui));
		gfs->gui = NULL;
	}

	G_OBJECT_CLASS (g_type_class_peek (GTK_TYPE_HBOX))->finalize (obj);
}

static gboolean
accumulate_first_string (GSignalInvocationHint *ihint,
			 GValue                *accum_result,
			 const GValue          *handler_result,
			 gpointer               data)
{
	gchar const *str = g_value_get_string (handler_result);
	if (NULL != str) {
		g_value_set_string (accum_result, str);
		return FALSE;
	}
	return TRUE;
}

static void
nfs_class_init (GObjectClass *klass)
{
	klass->finalize = go_format_sel_finalize;

	go_format_sel_signals [FORMAT_CHANGED] =
		g_signal_new ("format_changed",
			      G_OBJECT_CLASS_TYPE (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GOFormatSelClass, format_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1, G_TYPE_POINTER);

	go_format_sel_signals [GENERATE_PREVIEW] =
		g_signal_new ("generate-preview",
			      G_OBJECT_CLASS_TYPE (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GOFormatSelClass, generate_preview),
			      accumulate_first_string, NULL,
			      go__STRING__POINTER,
			      G_TYPE_STRING, 1, G_TYPE_POINTER);
}

GSF_CLASS (GOFormatSel, go_format_sel,
	   nfs_class_init, nfs_init, GTK_TYPE_HBOX)

/**
 * go_format_sel_new_full:
 * @use_markup: enable formats using pango markup
 *
 * Creates a format selector widget, with general format selected
 * by default. When @use_markup is set to %TRUE, it shows additional
 * widgets for editing properties of formats using pango markup
 * (e.g. scientific format with superscripted exponent).
 *
 * returns: a format selector widget.
 **/

GtkWidget *
go_format_sel_new_full (gboolean use_markup)
{
	GOFormatSel *gfs;

	gfs = g_object_new (GO_TYPE_FORMAT_SEL, NULL);

	if (gfs != NULL)
		gfs->show_format_with_markup = use_markup;

	return (GtkWidget *) gfs;
}

/**
 * go_format_sel_new:
 *
 * Creates a format selector widget, with general format selected
 * by default, and formats using pango markup disabled. See
 * @go_format_sel_new_full.
 *
 * returns: a format selector widget.
 **/

GtkWidget *
go_format_sel_new (void)
{
	return go_format_sel_new_full (FALSE);
}

void
go_format_sel_set_focus (GOFormatSel *gfs)
{
	g_return_if_fail (GO_IS_FORMAT_SEL (gfs));

	gtk_widget_grab_focus (GTK_WIDGET (gfs->format.menu));
}

void
go_format_sel_set_style_format (GOFormatSel *gfs,
				GOFormat const *style_format)
{
	GOComboText *combo;

	g_return_if_fail (GO_IS_FORMAT_SEL (gfs));
	g_return_if_fail (style_format != NULL);
	g_return_if_fail (!go_format_is_markup (style_format));

	go_format_ref (style_format);
	go_format_unref (gfs->format.spec);
	gfs->format.spec = style_format;

	study_format (gfs->format.spec, &gfs->format.details);

	combo = GO_COMBO_TEXT (gfs->format.widget[F_SYMBOL]);
	if (gfs->format.details.currency) {
		const char *desc = gfs->format.details.currency
			? gfs->format.details.currency->description
			: NULL;
		if (!desc)
			desc = N_("None");
		go_combo_text_set_text (combo, _(desc),
					GO_COMBO_TEXT_FROM_TOP);
	}

	set_format_category_menu_from_style (gfs);
	draw_format_preview (gfs, TRUE);
}

void
go_format_sel_set_dateconv (GOFormatSel *gfs,
			    GODateConventions const *date_conv)
{
	g_return_if_fail (GO_IS_FORMAT_SEL (gfs));
	g_return_if_fail (date_conv != NULL);

	/* FIXME is it safe ? */

	gfs->date_conv = date_conv;

	draw_format_preview (gfs, TRUE);
}

GOFormat const *
go_format_sel_get_fmt (GOFormatSel *gfs)
{
	g_return_val_if_fail (GO_IS_FORMAT_SEL (gfs), NULL);
	return gfs->format.spec;
}

GODateConventions const *
go_format_sel_get_dateconv (GOFormatSel *gfs)
{
	g_return_val_if_fail (GO_IS_FORMAT_SEL (gfs), NULL);
	return gfs->date_conv;
}

void
go_format_sel_show_preview (GOFormatSel *gfs)
{
	g_return_if_fail (GO_IS_FORMAT_SEL (gfs));
	gtk_widget_show (gfs->format.preview_box);
	draw_format_preview (gfs, TRUE);
}

void
go_format_sel_hide_preview (GOFormatSel *gfs)
{
	g_return_if_fail (GO_IS_FORMAT_SEL (gfs));
	gtk_widget_hide (gfs->format.preview_box);
}

void
go_format_sel_editable_enters (GOFormatSel *gfs,
			       GtkWindow *window)
{
	g_return_if_fail (GO_IS_FORMAT_SEL (gfs));
	go_gtk_editable_enters (window, gfs->format.widget[F_DECIMAL_SPIN]);
	go_gtk_editable_enters (window, gfs->format.widget[F_ENTRY]);
}

void
go_format_sel_set_locale (GOFormatSel *gfs,
			  char const *locale)
{
	g_free (gfs->locale);
	gfs->locale = g_strdup (locale);

	cb_format_class_changed (NULL, gfs);
}

/* The following utility function should possibly be in format.h but we */
/* access to the array of category names which are better located here. */
char const *
go_format_sel_format_classification (GOFormat const *style_format)
{
	GOFormatFamily page;
	GOFormatDetails details;

	page = study_format (style_format, &details);

	if (page < 0 || page > FMT_CUSTOM)
		page = FMT_CUSTOM; /* Default to custom */

	return _(format_category_names[page]);
}
