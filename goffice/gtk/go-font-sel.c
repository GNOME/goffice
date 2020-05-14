/*
 * A font selector widget.
 *
 * Copyright 2013 by Morten Welinder (terra@gnome.org)
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
#include <goffice/goffice-config.h>
#include <goffice/goffice.h>

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>


struct _GOFontSel {
	GtkBox		base;

	GtkBuilder	*gui;

	GtkWidget       *family_picker;
	PangoFontFamily *current_family;
	GHashTable      *family_by_name;
	GHashTable      *item_by_family;
	GHashTable      *faces_by_family;

	gboolean        show_style;
	GtkWidget       *face_picker;
	PangoFontFace   *current_face;
	GHashTable      *item_by_face;
	GHashTable      *face_renames;

	GtkWidget	*size_entry;
	GtkWidget       *size_picker;
	GSList          *font_sizes;

	gboolean        show_color;
	GtkWidget       *color_picker;
	GOColorGroup    *color_group;
	char            *color_unset_text;
	GOColor          color_default;

	gboolean        show_underline;
	GtkWidget       *underline_picker;

	gboolean        show_strikethrough;
	GtkWidget       *strikethrough_button;

	gboolean        show_script;
	GtkWidget       *script_picker;

	gboolean	show_preview_entry;
	GtkWidget       *preview_label;
	char            *preview_text;

	PangoAttrList	*modifications;

	GtkFontFilterFunc filter_func;
	gpointer          filter_data;
	GDestroyNotify    filter_data_destroy;
};

typedef struct {
	GtkBoxClass parent_class;

	void (* font_changed) (GOFontSel *gfs, PangoAttrList *modfications);
} GOFontSelClass;

enum {
	PROP_0,
	PROP_SHOW_STYLE,
	PROP_SHOW_COLOR,
	PROP_SHOW_UNDERLINE,
	PROP_SHOW_SCRIPT,
	PROP_SHOW_STRIKETHROUGH,
	PROP_COLOR_UNSET_TEXT,
	PROP_COLOR_GROUP,
	PROP_COLOR_DEFAULT,
	PROP_UNDERLINE_PICKER,

	GFS_GTK_FONT_CHOOSER_PROP_FIRST           = 0x4000,
	GFS_GTK_FONT_CHOOSER_PROP_FONT,
	GFS_GTK_FONT_CHOOSER_PROP_FONT_DESC,
	GFS_GTK_FONT_CHOOSER_PROP_PREVIEW_TEXT,
	GFS_GTK_FONT_CHOOSER_PROP_SHOW_PREVIEW_ENTRY,
#if GTK_CHECK_VERSION(3,24,0)
	GFS_GTK_FONT_CHOOSER_PROP_LEVEL,
	GFS_GTK_FONT_CHOOSER_PROP_LANGUAGE,
	GFS_GTK_FONT_CHOOSER_PROP_FONT_FEATURES,
#endif
	GFS_GTK_FONT_CHOOSER_PROP_LAST
};

enum {
	FONT_CHANGED,
	LAST_SIGNAL
};

enum { COL_TEXT, COL_OBJ };

static guint gfs_signals[LAST_SIGNAL] = { 0 };
static GObjectClass *gfs_parent_class;

static void
go_font_sel_add_attr (GOFontSel *gfs, PangoAttribute *attr)
{
	attr->start_index = 0;
	attr->end_index = -1;
	pango_attr_list_change (gfs->modifications, attr);
}

static gboolean
cb_remove_type (PangoAttribute *attr, gpointer user_data)
{
	PangoAttrType atype = GPOINTER_TO_UINT (user_data);
	return (attr->klass->type == atype);
}

static void
go_font_sel_remove_attr (GOFontSel *gfs, PangoAttrType atype)
{
	PangoAttrList *removed =
		pango_attr_list_filter (gfs->modifications,
					cb_remove_type,
					GUINT_TO_POINTER (atype));
	if (removed)
		pango_attr_list_unref (removed);
}

static void
update_preview (GOFontSel *gfs)
{
	PangoAttrList *attrs;

	if (!gfs->preview_label)
		return;

	gtk_widget_set_visible (gfs->preview_label, gfs->show_preview_entry);

	gtk_label_set_text (GTK_LABEL (gfs->preview_label), gfs->preview_text);

	attrs = go_pango_translate_attributes (gfs->modifications);
	if (attrs == gfs->modifications)
		attrs = pango_attr_list_copy (attrs);
	gtk_label_set_attributes (GTK_LABEL (gfs->preview_label), attrs);
	pango_attr_list_unref (attrs);
}

static void
go_font_sel_emit_changed (GOFontSel *gfs)
{
	char *fontname = NULL;

	g_signal_emit (G_OBJECT (gfs),
		       gfs_signals[FONT_CHANGED], 0, gfs->modifications);

	g_object_get (gfs, "font", &fontname, NULL);
	g_signal_emit_by_name (gfs, "font-activated", 0, fontname);
	g_free (fontname);

	update_preview (gfs);
}

static int
by_family_name (gconstpointer a_, gconstpointer b_)
{
	PangoFontFamily *a = *(PangoFontFamily **)a_;
	PangoFontFamily *b = *(PangoFontFamily **)b_;
	return g_utf8_collate (pango_font_family_get_name (a),
			       pango_font_family_get_name (b));
}

static void
dispose_families (GOFontSel *gfs)
{
	g_hash_table_remove_all (gfs->family_by_name);
	g_hash_table_remove_all (gfs->item_by_family);
	g_hash_table_remove_all (gfs->faces_by_family);
	g_hash_table_remove_all (gfs->item_by_face);
}

static void
update_preview_after_face_change (GOFontSel *gfs, gboolean signal_change)
{
	PangoFontDescription *desc =
		pango_font_face_describe (gfs->current_face);
	/*
	 * This isn't 100% right: we are ignoring other attributes.
	 * We could fix that but unsetting extra atttributes might
	 * not be so easy.
	 */
	PangoWeight weight = pango_font_description_get_weight (desc);
	PangoStyle style = pango_font_description_get_style (desc);
	go_font_sel_add_attr (gfs, pango_attr_weight_new (weight));
	go_font_sel_add_attr (gfs, pango_attr_style_new (style));
	pango_font_description_free (desc);
	// reload_size (gfs);
	if (signal_change)
		go_font_sel_emit_changed (gfs);
}

#define SUBST(src_,dst_) do {						\
	const char *where = strstr (name, (src_));			\
	if (where) {							\
		size_t prelen = where - name;				\
		size_t srclen = strlen ((src_));			\
		size_t dstlen = strlen ((dst_));			\
		char *newname = g_malloc (strlen (name) + dstlen - srclen + 1);	\
		memcpy (newname, name, prelen);				\
		memcpy (newname + prelen, (dst_), dstlen);		\
		strcpy (newname + prelen + dstlen, where + srclen);	\
		g_free (name);						\
		name = newname;						\
	}								\
} while (0)

#define ALIAS(src_,dst_) do {			\
	if (strcmp (name, (src_)) == 0) {	\
		g_free (name);			\
		name = g_strdup ((dst_));	\
	}					\
} while (0)


/*
 * In the real world we observe fonts with typos in face names...
 */
static const char *
my_get_face_name (GOFontSel *gfs, PangoFontFace *face)
{
	const char *orig_name = face ?
		pango_font_face_get_face_name (face)
		: NULL;
	char *name;
	const char *res;

	if (!orig_name)
		return NULL;

	/* Get rid of leading space.  */
	while (*orig_name == ' ')
		orig_name++;

	res = g_hash_table_lookup (gfs->face_renames, orig_name);
	if (res)
		return res;

	name = g_strdup (orig_name);

	/* Typos.  */
	SUBST(" Samll ", " Small ");
	SUBST("Reguler", "Regular");

	/* Normalize */
	SUBST("BoldItalic", "Bold Italic");
	SUBST("BoldOblique", "Bold Oblique");
	SUBST("BoldNonextended", "Bold Nonextended");
	SUBST("BoldSlanted", "Bold Slanted");
	SUBST("Cond ", "Condensed ");
	SUBST("ExtraLight", "Extra Light");
	SUBST("LightOblique", "Light Oblique");
	SUBST("SemiBold", "Semi-Bold");
	SUBST("Semibold", "Semi-Bold");
	SUBST("SemiCondensed", "Semi-Condensed");
	SUBST("DemiCondensed", "Demi-Condensed");
	SUBST("Expd ", "Expanded ");
	SUBST("SemiExpanded", "Semi-Expanded");
	SUBST("UprightItalic", "Upright Italic");
	SUBST("RomanSlanted", "Roman Slanted");

	ALIAS ("bold", "Bold");
	ALIAS ("heavy", "Heavy");
	ALIAS ("light", "Light");
	ALIAS ("medium", "Medium");
	ALIAS ("regular", "Regular");
	ALIAS ("thin", "Thin");
	ALIAS ("black", "Black");
	ALIAS ("Oblique Semi-Condensed", "Semi-Condensed Oblique");
	ALIAS ("Bold Semi-Condensed", "Semi-Condensed Bold");
	ALIAS ("Gothic-Regular", "Gothic");
	ALIAS ("Mincho-Regular", "Mincho");
	ALIAS ("SuperBold", "Super-Bold");
	ALIAS ("Condensed Regular", "Condensed");
	ALIAS ("Expanded Regular", "Expanded");
	ALIAS ("Semi-Condensed Regular", "Semi-Condensed");
	ALIAS ("Semi-Expanded Regular", "Semi-Expanded");

	g_hash_table_insert (gfs->face_renames, g_strdup (orig_name), name);

	return name;
}

#undef ALIAS
#undef SUBST

static void
add_item_to_menu (GtkWidget *m, const char *name,
		  const char *what, gpointer data,
		  GHashTable *itemhash)
{
	GtkWidget *w = gtk_menu_item_new_with_label (name);
	gtk_menu_shell_append (GTK_MENU_SHELL (m), w);
	g_object_set_data (G_OBJECT (w), what, data);
	if (itemhash && !g_hash_table_lookup (itemhash, data))
		g_hash_table_insert (itemhash, data, w);
}

static void
add_submenu_to_menu (GtkWidget *m, const char *name, GtkWidget *m2)
{
	GtkWidget *sm = gtk_menu_item_new_with_label (name);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (sm), m2);
	gtk_menu_shell_append (GTK_MENU_SHELL (m), sm);
}

static void
reload_faces (GOFontSel *gfs)
{
	GtkWidget *m;
	GSList *faces;
	PangoWeight current_weight = PANGO_WEIGHT_NORMAL;
	PangoStyle current_style = PANGO_STYLE_NORMAL;

	if (gfs->current_face) {
		PangoFontDescription *desc =
			pango_font_face_describe (gfs->current_face);
		current_weight = pango_font_description_get_weight (desc);
		current_style = pango_font_description_get_style (desc);
		/* This isn't 100% right: we may lose other attributes.  */
	}

	gfs->current_face = NULL;

	g_hash_table_remove_all (gfs->item_by_face);

	m = gtk_menu_new ();
	gtk_menu_set_title (GTK_MENU (m), _("Faces"));
	for (faces = g_hash_table_lookup (gfs->faces_by_family, gfs->current_family);
	     faces;
	     faces = faces->next) {
		PangoFontFace *face = faces->data;
		const char *name = my_get_face_name (gfs, face);
		const char *lname = g_dpgettext2 (NULL, "FontFace", name);
		add_item_to_menu (m, lname, "face", face, gfs->item_by_face);
	}

	gtk_widget_show_all (m);
	go_option_menu_set_menu (GO_OPTION_MENU (gfs->face_picker), m);

	go_font_sel_set_style (gfs, current_weight, current_style);
}

#define ADD_OBSERVED(it) g_hash_table_insert (observed_faces, (gpointer)(it), (gpointer)(it))

static void
reload_families (GOFontSel *gfs)
{
	PangoContext *context;
	PangoFontFamily **pango_families;
	int i, n_families;
	GtkWidget *m, *mall, *msingle = NULL, *mother = NULL;
	gunichar uc = 0;
	static const char *priority[] = { "Sans", "Serif", "Monospace" };
	gboolean has_priority;
	char *current_family_name;
	GHashTable *observed_faces = NULL;
	gboolean debug_font = go_debug_flag ("font");

	if (debug_font) {
		observed_faces = g_hash_table_new (g_str_hash, g_str_equal);

		/*
		 * List of observed face names.  Translators: these represent
		 * attributes of a font face, i.e., how bold the letters are
		 * and whether it is an italic or regular face.  The four
		 * first here are the important ones.
		 */
		ADD_OBSERVED (NC_("FontFace", "Regular"));
		ADD_OBSERVED (NC_("FontFace", "Italic"));
		ADD_OBSERVED (NC_("FontFace", "Bold"));
		ADD_OBSERVED (NC_("FontFace", "Bold Italic"));

		/* These are fairly rare.  */
		ADD_OBSERVED (NC_("FontFace", "Black"));
		ADD_OBSERVED (NC_("FontFace", "Bold Condensed Italic"));
		ADD_OBSERVED (NC_("FontFace", "Bold Condensed"));
		ADD_OBSERVED (NC_("FontFace", "Bold Italic Small Caps"));
		ADD_OBSERVED (NC_("FontFace", "Bold Nonextended"));
		ADD_OBSERVED (NC_("FontFace", "Bold Oblique"));
		ADD_OBSERVED (NC_("FontFace", "Bold Slanted"));
		ADD_OBSERVED (NC_("FontFace", "Bold Small Caps"));
		ADD_OBSERVED (NC_("FontFace", "Book Oblique"));
		ADD_OBSERVED (NC_("FontFace", "Book"));
		ADD_OBSERVED (NC_("FontFace", "Condensed Bold Italic"));
		ADD_OBSERVED (NC_("FontFace", "Condensed Bold Oblique"));
		ADD_OBSERVED (NC_("FontFace", "Condensed Bold"));
		ADD_OBSERVED (NC_("FontFace", "Condensed Italic"));
		ADD_OBSERVED (NC_("FontFace", "Condensed Oblique"));
		ADD_OBSERVED (NC_("FontFace", "Condensed"));
		ADD_OBSERVED (NC_("FontFace", "Demi Bold Italic"));
		ADD_OBSERVED (NC_("FontFace", "Demi Bold"));
		ADD_OBSERVED (NC_("FontFace", "Demi Oblique"));
		ADD_OBSERVED (NC_("FontFace", "Demi"));
		ADD_OBSERVED (NC_("FontFace", "Demi-Condensed"));
		ADD_OBSERVED (NC_("FontFace", "Embossed"));
		ADD_OBSERVED (NC_("FontFace", "Expanded Bold Italic"));
		ADD_OBSERVED (NC_("FontFace", "Expanded Bold"));
		ADD_OBSERVED (NC_("FontFace", "Expanded Italic"));
		ADD_OBSERVED (NC_("FontFace", "Expanded"));
		ADD_OBSERVED (NC_("FontFace", "Extra Light"));
		ADD_OBSERVED (NC_("FontFace", "Heavy"));
		ADD_OBSERVED (NC_("FontFace", "Italic Small Caps"));
		ADD_OBSERVED (NC_("FontFace", "Italic"));
		ADD_OBSERVED (NC_("FontFace", "Light Italic"));
		ADD_OBSERVED (NC_("FontFace", "Light Oblique"));
		ADD_OBSERVED (NC_("FontFace", "Light"));
		ADD_OBSERVED (NC_("FontFace", "Medium Italic"));
		ADD_OBSERVED (NC_("FontFace", "Medium"));
		ADD_OBSERVED (NC_("FontFace", "Normal"));
		ADD_OBSERVED (NC_("FontFace", "Oblique"));
		ADD_OBSERVED (NC_("FontFace", "Regular Condensed Italic"));
		ADD_OBSERVED (NC_("FontFace", "Regular Condensed"));
		ADD_OBSERVED (NC_("FontFace", "Regular Italic"));
		ADD_OBSERVED (NC_("FontFace", "Regular Oblique"));
		ADD_OBSERVED (NC_("FontFace", "Roman Slanted"));
		ADD_OBSERVED (NC_("FontFace", "Roman"));
		ADD_OBSERVED (NC_("FontFace", "Rounded"));
		ADD_OBSERVED (NC_("FontFace", "Sans Bold Italic"));
		ADD_OBSERVED (NC_("FontFace", "Sans Bold"));
		ADD_OBSERVED (NC_("FontFace", "Sans Italic"));
		ADD_OBSERVED (NC_("FontFace", "Sans"));
		ADD_OBSERVED (NC_("FontFace", "Semi-Bold Italic Small Caps"));
		ADD_OBSERVED (NC_("FontFace", "Semi-Bold Italic"));
		ADD_OBSERVED (NC_("FontFace", "Semi-Bold Oblique"));
		ADD_OBSERVED (NC_("FontFace", "Semi-Bold Slanted"));
		ADD_OBSERVED (NC_("FontFace", "Semi-Bold Small Caps"));
		ADD_OBSERVED (NC_("FontFace", "Semi-Bold"));
		ADD_OBSERVED (NC_("FontFace", "Semi-Condensed Bold Italic"));
		ADD_OBSERVED (NC_("FontFace", "Semi-Condensed Bold"));
		ADD_OBSERVED (NC_("FontFace", "Semi-Condensed Italic"));
		ADD_OBSERVED (NC_("FontFace", "Semi-Condensed Oblique"));
		ADD_OBSERVED (NC_("FontFace", "Semi-Condensed"));
		ADD_OBSERVED (NC_("FontFace", "Semi-Expanded Bold Italic"));
		ADD_OBSERVED (NC_("FontFace", "Semi-Expanded Bold"));
		ADD_OBSERVED (NC_("FontFace", "Semi-Expanded Italic"));
		ADD_OBSERVED (NC_("FontFace", "Semi-Expanded"));
		ADD_OBSERVED (NC_("FontFace", "Slanted"));
		ADD_OBSERVED (NC_("FontFace", "Small Caps"));
		ADD_OBSERVED (NC_("FontFace", "Thin"));
		ADD_OBSERVED (NC_("FontFace", "Upright Italic"));

		/* Observed, but no point in asking to translate. */
		ADD_OBSERVED ("Initials");
		ADD_OBSERVED ("Inline");
		ADD_OBSERVED ("ja");
		ADD_OBSERVED ("ko");
		ADD_OBSERVED ("LR");
		ADD_OBSERVED ("Gothic"); /* Sazanami */
		ADD_OBSERVED ("Mincho"); /* Sazanami */
		ADD_OBSERVED ("Atom Light"); /* Univox */
		ADD_OBSERVED ("Super-Bold"); /* Xolto */
		ADD_OBSERVED ("ReguObli"); /* Nimbus Mono L */

		/* A boat-load of crap from "Latin Modern" fonts. */
		ADD_OBSERVED ("10 Italic");
		ADD_OBSERVED ("10 Regular");
		ADD_OBSERVED ("12 Regular");
		ADD_OBSERVED ("9 Regular");
		ADD_OBSERVED ("8 Regular");
		ADD_OBSERVED ("10 Oblique");
		ADD_OBSERVED ("10 Bold");
		ADD_OBSERVED ("10 Bold Oblique");
		ADD_OBSERVED ("6 Regular");
		ADD_OBSERVED ("5 Regular");
		ADD_OBSERVED ("17 Regular");
		ADD_OBSERVED ("8 Italic");
		ADD_OBSERVED ("10 Bold Italic");
		ADD_OBSERVED ("12 Bold");
		ADD_OBSERVED ("12 Italic");
		ADD_OBSERVED ("7 Italic");
		ADD_OBSERVED ("9 Bold");
		ADD_OBSERVED ("8 Bold");
		ADD_OBSERVED ("7 Bold");
		ADD_OBSERVED ("5 Bold");
		ADD_OBSERVED ("6 Bold");
		ADD_OBSERVED ("9 Italic");
		ADD_OBSERVED ("7 Regular");
		ADD_OBSERVED ("9 Oblique");
		ADD_OBSERVED ("8 Oblique");
		ADD_OBSERVED ("12 Oblique");
		ADD_OBSERVED ("17 Oblique");
		ADD_OBSERVED ("8 Bold Oblique");
	}

	if (debug_font)
		g_printerr ("Reloading fonts\n");

	current_family_name = gfs->current_family
		? g_strdup (pango_font_family_get_name (gfs->current_family))
		: NULL;

	dispose_families (gfs);

	context = gtk_widget_get_pango_context (GTK_WIDGET (gfs));
	pango_context_list_families (context, &pango_families, &n_families);
	qsort (pango_families, n_families,
	       sizeof (pango_families[0]), by_family_name);

	for (i = 0; i < n_families; i++) {
		PangoFontFamily *family = pango_families[i];
		const char *name = pango_font_family_get_name (family);
		g_hash_table_replace (gfs->family_by_name,
				      g_strdup (name),
				      family);
	}

	m = gtk_menu_new ();
	gtk_menu_set_title (GTK_MENU (m), _("Font"));

	has_priority = FALSE;
	for (i = 0; i < (int)G_N_ELEMENTS (priority); i++) {
		const char *name = priority[i];
		PangoFontFamily *family =
			g_hash_table_lookup (gfs->family_by_name, name);
		if (family) {
			has_priority = TRUE;
			add_item_to_menu (m, name,
					  "family", family,
					  gfs->item_by_family);
		}
	}
	if (has_priority)
		gtk_menu_shell_append (GTK_MENU_SHELL (m),
				       gtk_separator_menu_item_new ());

	mall = gtk_menu_new ();
	for (i = 0; i < n_families; i++) {
		PangoFontFamily *family = pango_families[i];
		const char *name = pango_font_family_get_name (family);
		gunichar fc = g_unichar_toupper (g_utf8_get_char (name));
		GSList *ok_faces = NULL;
		PangoFontFace **faces;
		int j, n_faces;

		pango_font_family_list_faces (family, &faces, &n_faces);
		for (j = 0; j < n_faces; j++) {
			PangoFontFace *face = faces[j];
			const char *face_name;

			if (debug_font &&
			    (face_name = my_get_face_name (gfs, face)) &&
			    !g_hash_table_lookup (observed_faces, face_name)) {
				g_printerr ("New observed face: [%s] for [%s]\n",
					    face_name, name);
				ADD_OBSERVED (face_name);
			}

			if (gfs->filter_func &&
			    !gfs->filter_func (family, face, gfs->filter_data))
				continue;

			ok_faces = g_slist_append (ok_faces, face);
		}
		g_free (faces);
		if (!ok_faces)
			continue;

		g_hash_table_insert (gfs->faces_by_family, family, ok_faces);

		if (!g_unichar_isalpha (fc)) {
			if (!mother)
				mother = gtk_menu_new ();
			add_item_to_menu (mother, name,
					  "family", family,
					  gfs->item_by_family);
			continue;
		}

		if (fc != uc || !msingle) {
			char txt[10];

			uc = fc;
			txt[g_unichar_to_utf8 (uc, txt)] = 0;
			msingle = gtk_menu_new ();
			add_submenu_to_menu (mall, txt, msingle);
		}

		add_item_to_menu (msingle, name,
				  "family", family,
				  gfs->item_by_family);
	}
	if (mother)
		add_submenu_to_menu (mall, _("Other"), mother);
	add_submenu_to_menu (m, _("All fonts..."), mall);
	gtk_widget_show_all (m);

	g_free (pango_families);

	go_option_menu_set_menu (GO_OPTION_MENU (gfs->family_picker), m);

	if (current_family_name) {
		gfs->current_family = NULL;
		go_font_sel_set_family (gfs, current_family_name);
		g_free (current_family_name);
	}

	if (observed_faces)
		g_hash_table_destroy (observed_faces);

	reload_faces (gfs);
}
#undef ADD_OBSERVED

static int
by_integer (gconstpointer a_, gconstpointer b_)
{
	int a = *(int *)a_;
	int b = *(int *)b_;
	return (a < b ? -1 : (a == b ? 0 : +1));
}

static void
gfs_screen_changed (GtkWidget *w, GdkScreen *previous_screen)
{
	int width;
	PangoFontDescription *desc;
	GdkScreen *screen;
	GOFontSel *gfs = GO_FONT_SEL (w);
	PangoContext *pctxt = gtk_widget_get_pango_context (w);
	PangoLayout *layout = gtk_widget_create_pango_layout (w, NULL);
	GHashTableIter iter;
	gpointer key;
	GArray *widths;
	GtkWidget *label = go_option_menu_get_label (GO_OPTION_MENU (gfs->family_picker));

	screen = gtk_widget_get_screen (w);
	if (!previous_screen)
		previous_screen = gdk_screen_get_default ();
	if (screen == previous_screen &&
	    g_hash_table_size (gfs->family_by_name) > 0)
		return;

	reload_families (gfs);

	desc = pango_font_description_from_string ("Sans 72");
	g_object_set (w, "font-desc", desc, NULL);
	width = go_pango_measure_string (pctxt, desc, "M");
	pango_font_description_free (desc);
	/* Let's hope pixels are roughly square.  */
	gtk_widget_set_size_request (GTK_WIDGET (gfs->preview_label),
				     5 * width, width * 11 / 10);

	// Compute the 95% percentile of family name widths.  This is in the
	// hope that less than 5% are crazy.
	widths = g_array_new (FALSE, FALSE, sizeof (int));
	g_hash_table_iter_init (&iter, gfs->family_by_name);
	while (g_hash_table_iter_next (&iter, &key, NULL)) {
		const char *name = key;
		pango_layout_set_text (layout, name, -1);
		pango_layout_get_pixel_size (layout, &width, NULL);
		g_array_append_val (widths, width);
	}
	g_array_sort (widths, by_integer);
	width = widths->len > 0
		? g_array_index (widths, int, widths->len * 95 / 100)
		: -1;
	gtk_widget_set_size_request (label, width, -1);
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	g_array_free (widths, TRUE);
	g_object_unref (layout);
}

static void
update_sizes (GOFontSel *gfs)
{
	GtkComboBoxText *cbt = GTK_COMBO_BOX_TEXT (gfs->size_picker);
	GSList       *ptr;

	gtk_combo_box_text_remove_all (cbt);
	for (ptr = gfs->font_sizes; ptr != NULL ; ptr = ptr->next) {
		int size = GPOINTER_TO_INT (ptr->data);
		char *size_text = g_strdup_printf ("%g", size / (double)PANGO_SCALE);
		gtk_combo_box_text_append_text (cbt, size_text);
		g_free (size_text);
	}
}


static void
cb_font_changed (GOOptionMenu *om, GOFontSel *gfs)
{
	GtkWidget *selected = go_option_menu_get_history (om);
	PangoFontFamily *family = selected
		? g_object_get_data (G_OBJECT (selected), "family")
		: NULL;
	if (family && family != gfs->current_family) {
		const char *name = pango_font_family_get_name (family);
		go_font_sel_add_attr (gfs, pango_attr_family_new (name));
		gfs->current_family = family;
		reload_faces (gfs);
		go_font_sel_emit_changed (gfs);
	}
}

static void
cb_face_changed (GOOptionMenu *om, GOFontSel *gfs)
{
	GtkWidget *selected = go_option_menu_get_history (om);
	PangoFontFace *face = selected
		? g_object_get_data (G_OBJECT (selected), "face")
		: NULL;
	if (face && face != gfs->current_face) {
		gfs->current_face = face;
		update_preview_after_face_change (gfs, TRUE);
	}
}

static double
clamp_and_round_size (double size)
{
	size = CLAMP (size, 1.0, 1000.0);
	size = floor ((size * 10) + .5) / 10;
	return size;
}

static void
cb_size_picker_changed (GtkButton *button, GOFontSel *gfs)
{
	GtkEntry *entry = GTK_ENTRY (gfs->size_entry);
	const char *text = gtk_entry_get_text (entry);
	double size;
	char *end;
	int psize;

	size = go_strtod (text, &end);
	if (*text == 0 || end != text + strlen (text) || errno == ERANGE)
		return;

	size = clamp_and_round_size (size);
	psize = pango_units_from_double (size);

	go_font_sel_add_attr (gfs, pango_attr_size_new (psize));
	go_font_sel_emit_changed (gfs);
}

static void
cb_strikethrough_changed (GtkToggleButton *but, GOFontSel *gfs)
{
	gboolean b = gtk_toggle_button_get_active (but);
	go_font_sel_add_attr (gfs, pango_attr_strikethrough_new (b));
	go_font_sel_emit_changed (gfs);
}

static void
cb_color_changed (GOComboColor *color_picker, GOColor color,
		  gboolean is_custom, gboolean by_user, gboolean is_default,
		  GOFontSel *gfs)
{
	if (is_default)
		go_font_sel_remove_attr (gfs, PANGO_ATTR_FOREGROUND);
	else
		go_font_sel_add_attr (gfs, go_color_to_pango (color, TRUE));
	go_font_sel_emit_changed (gfs);
}


static void
cb_underline_changed (GOOptionMenu *om, GOFontSel *gfs)
{
	GtkWidget *selected = go_option_menu_get_history (om);
	PangoUnderline u;

	if (!selected)
		return;

	u = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (selected), "value"));
	go_font_sel_add_attr (gfs, pango_attr_underline_new (u));
	go_font_sel_emit_changed (gfs);
}

static void
cb_script_changed (GOOptionMenu *om, GOFontSel *gfs)
{
	GtkWidget *selected = go_option_menu_get_history (om);
	GOFontScript script;
	gboolean is_sub, is_super;

	if (!selected)
		return;

	script = GPOINTER_TO_INT
		(g_object_get_data (G_OBJECT (selected), "value"));
	is_super = (script == GO_FONT_SCRIPT_SUPER);
	is_sub = (script == GO_FONT_SCRIPT_SUB);

	go_font_sel_add_attr (gfs, go_pango_attr_subscript_new (is_sub));
	go_font_sel_add_attr (gfs, go_pango_attr_superscript_new (is_super));
	go_font_sel_emit_changed (gfs);
}

static void
gfs_init (GOFontSel *gfs)
{
	gfs->family_by_name = g_hash_table_new_full
		(g_str_hash, g_str_equal, (GDestroyNotify)g_free, NULL);
	gfs->item_by_family =
		g_hash_table_new (g_direct_hash, g_direct_equal);
	gfs->item_by_face =
		g_hash_table_new (g_direct_hash, g_direct_equal);
	gfs->faces_by_family = g_hash_table_new_full
		(g_direct_hash, g_direct_equal,
		 NULL, (GDestroyNotify)g_slist_free);
	gfs->face_renames = g_hash_table_new_full
		(g_str_hash, g_str_equal,
		 (GDestroyNotify)g_free, (GDestroyNotify)free);

	gfs->show_preview_entry = TRUE;
	gfs->preview_text = g_strdup (pango_language_get_sample_string (NULL));
	gfs->font_sizes = go_fonts_list_sizes ();
	gfs->color_default = GO_COLOR_BLACK;
}

static void
remove_row_containing (GtkWidget *w)
{
	int row;
	GtkGrid *grid = GTK_GRID (gtk_widget_get_parent (w));
	gtk_container_child_get (GTK_CONTAINER (grid), w,
				 "top-attach", &row,
				 NULL);
	go_gtk_grid_remove_row (grid, row);
}


static GObject*
gfs_constructor (GType type,
		 guint n_construct_properties,
		 GObjectConstructParam *construct_params)
{
	GtkWidget *fontsel, *placeholder;
	GOFontSel *gfs = (GOFontSel *)
		(gfs_parent_class->constructor (type,
						n_construct_properties,
						construct_params));

	if (!gfs)
		return NULL;

	gfs->gui = go_gtk_builder_load_internal ("res:go:gtk/go-font-sel.ui", GETTEXT_PACKAGE, NULL);
	if (gfs->gui == NULL) {
		g_object_unref (gfs);
                return NULL;
	}

	gfs->modifications = pango_attr_list_new ();

	fontsel = go_gtk_builder_get_widget (gfs->gui, "font-selector");
	gtk_container_add (GTK_CONTAINER (gfs), fontsel);

	gfs->preview_label = go_gtk_builder_get_widget (gfs->gui, "preview-label");
	/* ---------------------------------------- */

	placeholder = go_gtk_builder_get_widget
		(gfs->gui, "family-picker-placeholder");
	gfs->family_picker = go_option_menu_new ();
	gtk_widget_show_all (gfs->family_picker);
	go_gtk_widget_replace (placeholder, gfs->family_picker);
	g_signal_connect (gfs->family_picker,
			  "changed",
			  G_CALLBACK (cb_font_changed),
			  gfs);

	/* ---------------------------------------- */

	placeholder = go_gtk_builder_get_widget
		(gfs->gui, "face-picker-placeholder");
	gfs->face_picker = go_option_menu_new ();
	g_object_ref_sink (gfs->face_picker);
	gtk_widget_show_all (gfs->face_picker);
	go_gtk_widget_replace (placeholder, gfs->face_picker);
	if (gfs->show_style) {
		g_signal_connect (gfs->face_picker,
				  "changed",
				  G_CALLBACK (cb_face_changed),
				  gfs);
	} else
		remove_row_containing (gfs->face_picker);

	/* ---------------------------------------- */

	gfs->size_picker = go_gtk_builder_get_widget (gfs->gui, "size-picker");
	update_sizes (gfs);
	gfs->size_entry = gtk_bin_get_child (GTK_BIN (gfs->size_picker));
	g_signal_connect (gfs->size_picker,
			  "changed",
			  G_CALLBACK (cb_size_picker_changed), gfs);

	/* ---------------------------------------- */

	placeholder = go_gtk_builder_get_widget
		(gfs->gui, "color-picker-placeholder");
	if (!gfs->color_group)
		gfs->color_group = go_color_group_fetch (NULL, gfs);
	gfs->color_picker =
		go_combo_color_new (NULL, gfs->color_unset_text,
				    gfs->color_default,
				    gfs->color_group);
	gtk_widget_set_halign (gfs->color_picker, GTK_ALIGN_START);
	g_object_ref_sink (gfs->color_picker);
	gtk_widget_show_all (gfs->color_picker);
	go_gtk_widget_replace (placeholder, gfs->color_picker);
	if (gfs->show_color) {
		g_signal_connect (gfs->color_picker,
				  "color-changed",
				  G_CALLBACK (cb_color_changed), gfs);
	} else
		remove_row_containing (gfs->color_picker);

	/* ---------------------------------------- */

	placeholder = go_gtk_builder_get_widget
		(gfs->gui, "underline-picker-placeholder");
	if (!gfs->underline_picker) {
		gfs->underline_picker = go_option_menu_build
			(C_("underline", "None"), PANGO_UNDERLINE_NONE,
			 C_("underline", "Single"), PANGO_UNDERLINE_SINGLE,
			 C_("underline", "Double"), PANGO_UNDERLINE_DOUBLE,
			 C_("underline", "Low"), PANGO_UNDERLINE_LOW,
			 C_("underline", "Error"), PANGO_UNDERLINE_ERROR,
			 NULL);
		if (gfs->show_underline)
			g_signal_connect (gfs->underline_picker,
					  "changed",
					  G_CALLBACK (cb_underline_changed),
					  gfs);
	}
	g_object_ref_sink (gfs->underline_picker);
	gtk_widget_show_all (gfs->underline_picker);
	go_gtk_widget_replace (placeholder, gfs->underline_picker);
	if (gfs->show_underline) {
		/* Nothing here */
	} else
		remove_row_containing (gfs->underline_picker);

	/* ---------------------------------------- */

	placeholder = go_gtk_builder_get_widget
		(gfs->gui, "script-picker-placeholder");
	gfs->script_picker =
		go_option_menu_build (_("Normal"), GO_FONT_SCRIPT_STANDARD,
				      _("Subscript"), GO_FONT_SCRIPT_SUB,
				      _("Superscript"), GO_FONT_SCRIPT_SUPER,
				      NULL);

	g_object_ref_sink (gfs->script_picker);
	gtk_widget_show_all (gfs->script_picker);
	go_gtk_widget_replace (placeholder, gfs->script_picker);
	if (gfs->show_script) {
		g_signal_connect (gfs->script_picker,
				  "changed",
				  G_CALLBACK (cb_script_changed),
				  gfs);
	} else
		remove_row_containing (gfs->script_picker);

	/* ---------------------------------------- */

	gfs->strikethrough_button = go_gtk_builder_get_widget
		(gfs->gui, "strikethrough-button");
	g_object_ref_sink (gfs->strikethrough_button);
	gtk_widget_show_all (gfs->strikethrough_button);
	if (gfs->show_strikethrough) {
		g_signal_connect (gfs->strikethrough_button,
				  "toggled",
				  G_CALLBACK (cb_strikethrough_changed), gfs);
	} else
		remove_row_containing (gfs->strikethrough_button);

	/* ---------------------------------------- */

	gtk_widget_show_all (fontsel);

	gfs_screen_changed (GTK_WIDGET (gfs), NULL);

	return (GObject *)gfs;
}

static void
gfs_finalize (GObject *obj)
{
	GOFontSel *gfs = GO_FONT_SEL (obj);

	/* We actually own a ref to these.  */
	g_clear_object (&gfs->face_picker);
	g_clear_object (&gfs->color_picker);
	g_clear_object (&gfs->color_group);
	g_clear_object (&gfs->underline_picker);
	g_clear_object (&gfs->script_picker);
	g_clear_object (&gfs->strikethrough_button);

	g_hash_table_destroy (gfs->family_by_name);
	g_hash_table_destroy (gfs->item_by_family);
	g_hash_table_destroy (gfs->item_by_face);
	g_hash_table_destroy (gfs->faces_by_family);
	g_hash_table_destroy (gfs->face_renames);

	gfs_parent_class->finalize (obj);
}

static void
gfs_dispose (GObject *obj)
{
	GOFontSel *gfs = GO_FONT_SEL (obj);

	if (gfs->gui) {
		g_object_unref (gfs->gui);
		gfs->gui = NULL;
		gfs->family_picker = NULL;
		gfs->size_picker = NULL;
	}
	if (gfs->modifications != NULL) {
		pango_attr_list_unref (gfs->modifications);
		gfs->modifications = NULL;
	}
	dispose_families (gfs);

	g_slist_free (gfs->font_sizes);
	gfs->font_sizes = NULL;

	g_free (gfs->preview_text);
	gfs->preview_text = NULL;

	g_free (gfs->color_unset_text);
	gfs->color_unset_text = NULL;

	gtk_font_chooser_set_filter_func (GTK_FONT_CHOOSER (obj),
					  NULL, NULL, NULL);

	gfs_parent_class->dispose (obj);
}

static void
gfs_get_property (GObject         *object,
		  guint            prop_id,
		  GValue          *value,
		  GParamSpec      *pspec)
{
	GOFontSel *gfs = GO_FONT_SEL (object);

	switch (prop_id) {
	case PROP_SHOW_STYLE:
		g_value_set_boolean (value, gfs->show_style);
		break;

	case PROP_SHOW_COLOR:
		g_value_set_boolean (value, gfs->show_color);
		break;

	case PROP_SHOW_UNDERLINE:
		g_value_set_boolean (value, gfs->show_underline);
		break;

	case PROP_SHOW_SCRIPT:
		g_value_set_boolean (value, gfs->show_script);
		break;

	case PROP_SHOW_STRIKETHROUGH:
		g_value_set_boolean (value, gfs->show_strikethrough);
		break;

	case PROP_COLOR_UNSET_TEXT:
		g_value_set_string (value, gfs->color_unset_text);
		break;

	case PROP_COLOR_GROUP:
		g_value_set_object (value, gfs->color_group);
		break;

	case PROP_COLOR_DEFAULT:
		g_value_set_uint (value, gfs->color_default);
		break;

	case PROP_UNDERLINE_PICKER:
		g_value_set_object (value, gfs->underline_picker);
		break;

	case GFS_GTK_FONT_CHOOSER_PROP_FONT: {
		PangoFontDescription *desc = go_font_sel_get_font_desc (gfs);
		g_value_take_string (value, pango_font_description_to_string (desc));
		pango_font_description_free (desc);
		break;
	}

	case GFS_GTK_FONT_CHOOSER_PROP_FONT_DESC:
		g_value_take_boxed (value, go_font_sel_get_font_desc (gfs));
		break;

	case GFS_GTK_FONT_CHOOSER_PROP_PREVIEW_TEXT:
		g_value_set_string (value, gfs->preview_text);
		break;

	case GFS_GTK_FONT_CHOOSER_PROP_SHOW_PREVIEW_ENTRY:
		g_value_set_boolean (value, gfs->show_preview_entry);
		break;

#if GTK_CHECK_VERSION(3,24,0)
	case GFS_GTK_FONT_CHOOSER_PROP_LEVEL:
		g_value_set_int (value, GTK_FONT_CHOOSER_LEVEL_FAMILY |
				              GTK_FONT_CHOOSER_LEVEL_STYLE |
				              GTK_FONT_CHOOSER_LEVEL_SIZE);
		break;

	case GFS_GTK_FONT_CHOOSER_PROP_LANGUAGE:
		g_value_set_string (value, "");
		break;

	case GFS_GTK_FONT_CHOOSER_PROP_FONT_FEATURES:
		g_value_set_string (value, "");
		break;
#endif

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gfs_set_property (GObject         *object,
		  guint            prop_id,
		  const GValue    *value,
		  GParamSpec      *pspec)
{
	GOFontSel *gfs = GO_FONT_SEL (object);

	switch (prop_id) {
	case PROP_SHOW_STYLE:
		gfs->show_style = g_value_get_boolean (value);
		break;

	case PROP_SHOW_COLOR:
		gfs->show_color = g_value_get_boolean (value);
		break;

	case PROP_SHOW_UNDERLINE:
		gfs->show_underline = g_value_get_boolean (value);
		break;

	case PROP_SHOW_SCRIPT:
		gfs->show_script = g_value_get_boolean (value);
		break;

	case PROP_SHOW_STRIKETHROUGH:
		gfs->show_strikethrough = g_value_get_boolean (value);
		break;

	case PROP_COLOR_UNSET_TEXT:
		g_free (gfs->color_unset_text);
		gfs->color_unset_text = g_value_dup_string (value);
		break;

	case PROP_COLOR_GROUP:
		g_clear_object (&gfs->color_group);
		gfs->color_group = g_value_dup_object (value);
		break;

	case PROP_COLOR_DEFAULT:
		gfs->color_default = g_value_get_uint (value);
		break;

	case PROP_UNDERLINE_PICKER:
		g_clear_object (&gfs->underline_picker);
		gfs->underline_picker = g_value_dup_object (value);
		break;

	case GFS_GTK_FONT_CHOOSER_PROP_FONT: {
		PangoFontDescription *desc = pango_font_description_from_string
			(g_value_get_string (value));
		go_font_sel_set_font_desc (gfs, desc);
		pango_font_description_free (desc);
		break;
	}

	case GFS_GTK_FONT_CHOOSER_PROP_FONT_DESC:
		go_font_sel_set_font_desc (gfs, g_value_get_boxed (value));
		break;

	case GFS_GTK_FONT_CHOOSER_PROP_PREVIEW_TEXT:
		go_font_sel_set_sample_text (gfs, g_value_get_string (value));
		break;

	case GFS_GTK_FONT_CHOOSER_PROP_SHOW_PREVIEW_ENTRY:
		gfs->show_preview_entry = g_value_get_boolean (value);
		update_preview (gfs);
		break;

#if GTK_CHECK_VERSION(3,24,0)
	case GFS_GTK_FONT_CHOOSER_PROP_LEVEL:
		/* not supported, just to avoid criticals */

		break;
	case GFS_GTK_FONT_CHOOSER_PROP_LANGUAGE:
		/* not supported, just to avoid criticals */

		break;
	case GFS_GTK_FONT_CHOOSER_PROP_FONT_FEATURES:
		/* not supported, just to avoid criticals */
		break;
#endif

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gfs_class_init (GObjectClass *klass)
{
	GtkWidgetClass *widget_klass = GTK_WIDGET_CLASS (klass);

	klass->constructor = gfs_constructor;
	klass->finalize = gfs_finalize;
	klass->dispose = gfs_dispose;
	klass->get_property = gfs_get_property;
	klass->set_property = gfs_set_property;

	widget_klass->screen_changed = gfs_screen_changed;

	gfs_parent_class = g_type_class_peek_parent (klass);

	g_object_class_install_property
		(klass, PROP_SHOW_STYLE,
		 g_param_spec_boolean ("show-style",
				       _("Show Style"),
				       _("Whether style is part of the font being selected"),
				       FALSE,
				       G_PARAM_READWRITE |
				       G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property
		(klass, PROP_SHOW_COLOR,
		 g_param_spec_boolean ("show-color",
				       _("Show Color"),
				       _("Whether color is part of the font being selected"),
				       FALSE,
				       G_PARAM_READWRITE |
				       G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property
		(klass, PROP_SHOW_UNDERLINE,
		 g_param_spec_boolean ("show-underline",
				       _("Show Underline"),
				       _("Whether underlining is part of the font being selected"),
				       FALSE,
				       G_PARAM_READWRITE |
				       G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property
		(klass, PROP_SHOW_SCRIPT,
		 g_param_spec_boolean ("show-script",
				       _("Show Script"),
				       _("Whether subscript/superscript is part of the font being selected"),
				       FALSE,
				       G_PARAM_READWRITE |
				       G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property
		(klass, PROP_SHOW_STRIKETHROUGH,
		 g_param_spec_boolean ("show-strikethrough",
				       _("Show Strikethrough"),
				       _("Whether strikethrough is part of the font being selected"),
				       FALSE,
				       G_PARAM_READWRITE |
				       G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property
		(klass, PROP_COLOR_UNSET_TEXT,
		 g_param_spec_string ("color-unset-text",
				      _("Color unset text"),
				      _("The text to show for selecting no color"),
				      NULL,
				      G_PARAM_READWRITE |
				      G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property
		(klass, PROP_COLOR_GROUP,
		 g_param_spec_object ("color-group",
				      _("Color Group"),
				      _("The color group to use for the color picker"),
				      GO_TYPE_COLOR_GROUP,
				      G_PARAM_READWRITE |
				      G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property
		(klass, PROP_COLOR_DEFAULT,
		 g_param_spec_uint ("color-default",
				    _("Color Default"),
				    _("The color to show for an unset color"),
				    GO_COLOR_FROM_RGBA (0,0,0,0),
				    GO_COLOR_FROM_RGBA (0xff,0xff,0xff,0xff),
				    GO_COLOR_BLACK,
				    G_PARAM_READWRITE |
				    G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property
		(klass, PROP_UNDERLINE_PICKER,
		 g_param_spec_object ("underline-picker",
				      _("Underline Picker"),
				      _("The widget to use for picking underline type"),
				      GTK_TYPE_WIDGET,
				      G_PARAM_READWRITE |
				      G_PARAM_CONSTRUCT_ONLY));

	g_object_class_override_property (klass,
					  GFS_GTK_FONT_CHOOSER_PROP_FONT,
					  "font");
	g_object_class_override_property (klass,
					  GFS_GTK_FONT_CHOOSER_PROP_FONT_DESC,
					  "font-desc");
	g_object_class_override_property (klass,
					  GFS_GTK_FONT_CHOOSER_PROP_PREVIEW_TEXT,
					  "preview-text");
	g_object_class_override_property (klass,
					  GFS_GTK_FONT_CHOOSER_PROP_SHOW_PREVIEW_ENTRY,
					  "show-preview-entry");
#if GTK_CHECK_VERSION(3,24,0)
	g_object_class_override_property (klass,
					  GFS_GTK_FONT_CHOOSER_PROP_LEVEL,
					  "level");
	g_object_class_override_property (klass,
					  GFS_GTK_FONT_CHOOSER_PROP_LANGUAGE,
					  "language");
	g_object_class_override_property (klass,
					  GFS_GTK_FONT_CHOOSER_PROP_FONT_FEATURES,
					  "font-features");
#endif

	gfs_signals[FONT_CHANGED] =
		g_signal_new (
			"font_changed",
			G_OBJECT_CLASS_TYPE (klass),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (GOFontSelClass, font_changed),
			NULL, NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE, 1, G_TYPE_POINTER);

#ifdef HAVE_GTK_WIDGET_CLASS_SET_CSS_NAME
	gtk_widget_class_set_css_name (widget_klass, "fontselector");
#endif
}

static void
gfs_font_chooser_set_filter_func (GtkFontChooser    *chooser,
				  GtkFontFilterFunc  filter_func,
				  gpointer           filter_data,
				  GDestroyNotify     data_destroy)
{
	GOFontSel *gfs = GO_FONT_SEL (chooser);

	if (gfs->filter_data_destroy)
		gfs->filter_data_destroy (gfs->filter_data);

	gfs->filter_func = filter_func;
	gfs->filter_data = filter_data;
	gfs->filter_data_destroy = data_destroy;

	if (gfs->family_picker)
		reload_families (gfs);
}

static PangoFontFamily *
gfs_font_chooser_get_font_family (GtkFontChooser *chooser)
{
	GOFontSel *gfs = GO_FONT_SEL (chooser);
	return gfs->current_family;
}

static int
gfs_font_chooser_get_font_size (GtkFontChooser *chooser)
{
	GOFontSel *gfs = GO_FONT_SEL (chooser);
	PangoFontDescription *desc = go_font_sel_get_font_desc (gfs);
	int size = pango_font_description_get_size (desc);
	pango_font_description_free (desc);
	return size;
}

static PangoFontFace *
gfs_font_chooser_get_font_face (GtkFontChooser *chooser)
{
	GOFontSel *gfs = GO_FONT_SEL (chooser);
	return gfs->current_face;
}

static void
gfs_font_chooser_iface_init (GtkFontChooserIface *iface)
{
	iface->get_font_family = gfs_font_chooser_get_font_family;
	iface->get_font_face = gfs_font_chooser_get_font_face;
	iface->get_font_size = gfs_font_chooser_get_font_size;
	iface->set_filter_func = gfs_font_chooser_set_filter_func;
}

GSF_CLASS_FULL (GOFontSel, go_font_sel,
		NULL, NULL, gfs_class_init, NULL,
		gfs_init, GTK_TYPE_BOX, 0,
		GSF_INTERFACE (gfs_font_chooser_iface_init, GTK_TYPE_FONT_CHOOSER);
	)
#if 0
;
#endif


GtkWidget *
go_font_sel_new (void)
{
	return g_object_new (GO_TYPE_FONT_SEL, NULL);
}

void
go_font_sel_editable_enters (GOFontSel *gfs, GtkWindow *dialog)
{
	go_gtk_editable_enters (dialog,
				GTK_WIDGET (gfs->size_entry));
}

void
go_font_sel_set_sample_text (GOFontSel *gfs, char const *text)
{
	g_return_if_fail (GO_IS_FONT_SEL (gfs));

	if (!text) text = "";
	if (g_strcmp0 (text, gfs->preview_text)) {
		g_free (gfs->preview_text);
		gfs->preview_text = g_strdup (text);
		g_object_notify (G_OBJECT (gfs), "preview-text");
		update_preview (gfs);
	}
}

PangoAttrList *
go_font_sel_get_sample_attributes (GOFontSel *fs)
{
	return fs->modifications;
}

void
go_font_sel_set_sample_attributes (GOFontSel *fs, PangoAttrList *attrs)
{
	PangoAttrList *acopy;
#if 0
	PangoAttrIterator *aiter = pango_attr_list_get_iterator (attrs);
	const PangoAttribute *attr;
	PangoWeight weight;
	PangoStyle style;
#endif

	acopy = pango_attr_list_copy (attrs);
	pango_attr_list_unref (fs->modifications);
	fs->modifications = acopy;

#if 0
	attr = pango_attr_iterator_get (aiter, PANGO_ATTR_WEIGHT);
	weight = attr ? ((PangoAttrInt*)attr)->value : PANGO_WEIGHT_NORMAL;
	attr = pango_attr_iterator_get (aiter, PANGO_ATTR_STYLE);
	style = attr ? ((PangoAttrInt*)attr)->value : PANGO_STYLE_NORMAL;
	go_font_sel_set_style (fs, weight, style);

	attr = pango_attr_iterator_get (aiter, PANGO_ATTR_STRIKETHROUGH);
	go_font_sel_set_strikethrough (fs, attr && ((PangoAttrInt*)attr)->value);

	pango_attr_iterator_destroy (aiter);
#endif

	update_preview (fs);
}



GOFont const *
go_font_sel_get_font (GOFontSel const *gfs)
{
	g_return_val_if_fail (GO_IS_FONT_SEL (gfs), NULL);

	/* This hasn't worked in a while! */

	return NULL;
}

void
go_font_sel_set_family (GOFontSel *fs, char const *font_name)
{
	PangoFontFamily *family =
		g_hash_table_lookup (fs->family_by_name, font_name);
	GtkMenuItem *item = g_hash_table_lookup (fs->item_by_family, family);
	if (item && family != fs->current_family) {
		go_option_menu_select_item (GO_OPTION_MENU (fs->family_picker),
					    item);
		go_font_sel_add_attr (fs, pango_attr_family_new (font_name));
		fs->current_family = family;
		update_preview (fs);
		reload_faces (fs);
	}
}

void
go_font_sel_set_style (GOFontSel *fs, PangoWeight weight, PangoStyle style)
{
	PangoFontFamily *family;
	int best_badness = G_MAXINT;
	PangoFontFace *best = NULL;
	GSList *faces;

	g_return_if_fail (GO_IS_FONT_SEL (fs));

	family = fs->current_family;
	faces = g_hash_table_lookup (fs->faces_by_family, family);

	for (; faces; faces = faces->next) {
		PangoFontFace *face = faces->data;
		PangoFontDescription *desc = pango_font_face_describe (face);
		PangoWeight fweight = pango_font_description_get_weight (desc);
		PangoStyle fstyle = pango_font_description_get_style (desc);
		int badness =
			(500 * ABS ((int)style - (int)fstyle) +
			 ABS ((int)weight - (int)fweight));
		pango_font_description_free (desc);

		if (badness < best_badness) {
			best_badness = badness;
			best = face;
		}
	}

	if (best && best != fs->current_face) {
		GtkMenuItem *item;
		fs->current_face = best;
		item = g_hash_table_lookup (fs->item_by_face, best);
		go_option_menu_select_item (GO_OPTION_MENU (fs->face_picker),
					    item);
		update_preview_after_face_change (fs, FALSE);
	}
}

static void
go_font_sel_set_points (GOFontSel *gfs,	double size)
{
	char *new_text;

	size = clamp_and_round_size (size);
	new_text = g_strdup_printf ("%g", size);
	/* This is a null op if the text does not change.  */
	gtk_entry_set_text (GTK_ENTRY (gfs->size_entry), new_text);
	g_free (new_text);
}

void
go_font_sel_set_size (GOFontSel *fs, int size)
{
	go_font_sel_set_points (fs, size / (double)PANGO_SCALE);
}


void
go_font_sel_set_strikethrough (GOFontSel *fs, gboolean strikethrough)
{
	GtkToggleButton *but = GTK_TOGGLE_BUTTON (fs->strikethrough_button);
	gboolean b = gtk_toggle_button_get_active (but);

	strikethrough = !!strikethrough;
	if (b == strikethrough)
		return;

	gtk_toggle_button_set_active (but, strikethrough);
	go_font_sel_add_attr (fs, pango_attr_strikethrough_new (strikethrough));
	update_preview (fs);
}

void
go_font_sel_set_script (GOFontSel *fs, GOFontScript script)
{
	GOOptionMenu *om = GO_OPTION_MENU (fs->script_picker);
	GtkMenuShell *ms = GTK_MENU_SHELL (go_option_menu_get_menu (om));
	GList *children = gtk_container_get_children (GTK_CONTAINER (ms));
	GList *l;

	for (l = children; l; l = l->next) {
		GtkMenuItem *item = GTK_MENU_ITEM (l->data);
		GOFontScript s = GPOINTER_TO_INT
			(g_object_get_data (G_OBJECT (item), "value"));
		if (s == script)
			go_option_menu_select_item (om, item);
	}

	g_list_free (children);
}

static void
go_font_sel_set_uline (GOFontSel *gfs, int uline)
{
}

void
go_font_sel_set_color (GOFontSel *gfs, GOColor c, gboolean is_default)
{
	GOComboColor *cc = GO_COMBO_COLOR (gfs->color_picker);
	gboolean old_is_default;
	GOColor old_color = go_combo_color_get_color (cc, &old_is_default);

	if (old_is_default == is_default && (is_default || old_color == c))
		return;

	if (is_default)
		go_combo_color_set_color_to_default (cc);
	else
		go_combo_color_set_color (cc, c);
}

GOColor
go_font_sel_get_color (GOFontSel *gfs) {
	g_return_val_if_fail (GO_IS_FONT_SEL (gfs), 0);
	return go_combo_color_get_color (GO_COMBO_COLOR (gfs->color_picker), NULL);
}

void
go_font_sel_set_font_desc (GOFontSel *fs, PangoFontDescription *desc)
{
	PangoFontMask fields;
	g_return_if_fail (GO_IS_FONT_SEL (fs));

	fields = pango_font_description_get_set_fields (desc);

	if (fields & PANGO_FONT_MASK_FAMILY)
		go_font_sel_set_family (fs,
					pango_font_description_get_family (desc));
	if (fields & (PANGO_FONT_MASK_WEIGHT | PANGO_FONT_MASK_STYLE))
		go_font_sel_set_style (fs,
				       pango_font_description_get_weight (desc),
				       pango_font_description_get_style (desc));

	if (fields & PANGO_FONT_MASK_SIZE)
		go_font_sel_set_size (fs,
				      pango_font_description_get_size (desc));
}


void
go_font_sel_set_font (GOFontSel *fs, GOFont const *font)
{
	g_return_if_fail (GO_IS_FONT_SEL (fs));

	go_font_sel_set_font_desc (fs, font->desc);
	go_font_sel_set_strikethrough (fs, font->strikethrough);
	go_font_sel_set_uline (fs, font->underline);
	go_font_sel_set_color (fs, font->color, FALSE);
}


/**
 * go_font_sel_get_font_desc:
 * @fs: the font selector
 *
 * Returns: (transfer full): a description of the font set.
 */
PangoFontDescription *
go_font_sel_get_font_desc (GOFontSel *fs)
{
	PangoAttrIterator *aiter;
	PangoFontDescription *desc;

	g_return_val_if_fail (GO_IS_FONT_SEL (fs), NULL);

	aiter = pango_attr_list_get_iterator (fs->modifications);
	desc = pango_font_description_new ();
	pango_attr_iterator_get_font (aiter, desc, NULL, NULL);
	pango_attr_iterator_destroy (aiter);

	/* Ensure family stays valid.  */
	pango_font_description_set_family
		(desc, pango_font_description_get_family (desc));

	return desc;
}
