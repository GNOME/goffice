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

	GtkWidget	*font_size_entry;
	GtkWidget       *size_picker;
	GSList          *font_sizes;

	gboolean        show_color;
	GtkWidget       *color_picker;

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
	GtkGridClass parent_class;

	void (* font_changed) (GOFontSel *gfs, PangoAttrList *modfications);
} GOFontSelClass;

enum {
	PROP_0,
	PROP_SHOW_STYLE,
	PROP_SHOW_COLOR,
	PROP_SHOW_SCRIPT,
	PROP_SHOW_STRIKETHROUGH,

	GFS_GTK_FONT_CHOOSER_PROP_FIRST           = 0x4000,
	GFS_GTK_FONT_CHOOSER_PROP_FONT,
	GFS_GTK_FONT_CHOOSER_PROP_FONT_DESC,
	GFS_GTK_FONT_CHOOSER_PROP_PREVIEW_TEXT,
	GFS_GTK_FONT_CHOOSER_PROP_SHOW_PREVIEW_ENTRY,
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
	PangoWeight style = pango_font_description_get_style (desc);
	go_font_sel_add_attr (gfs, pango_attr_weight_new (weight));
	go_font_sel_add_attr (gfs, pango_attr_style_new (style));
	pango_font_description_free (desc);
	// reload_size (gfs);
	if (signal_change)
		go_font_sel_emit_changed (gfs);
}

/*
 * In the real world we observe fonts with typos in face names...
 */
static const char *
my_get_face_name (PangoFontFace *face)
{
	const char *name = face ? pango_font_face_get_face_name (face) : NULL;

	if (!name)
		return NULL;
	if (strcmp (name, "BoldItalic") == 0)
		return "Bold Italic";
	if (strcmp (name, "BoldOblique") == 0)
		return "Bold Oblique";
	if (strcmp (name, "LightOblique") == 0)
		return "Light Oblique";
	if (strcmp (name, "ExtraLight") == 0)
		return "Extra Light";

	return name;
}

static GtkMenuItem *
add_font_to_menu (GtkWidget *m, const char *name,
		  const char *what, gpointer obj,
		  GHashTable *itemhash)
{
	GtkWidget *w = gtk_menu_item_new_with_label (name);
	gtk_menu_shell_append (GTK_MENU_SHELL (m), w);
	g_object_set_data (G_OBJECT (w), what, obj);
	if (!g_hash_table_lookup (itemhash, obj))
		g_hash_table_insert (itemhash, obj, w);
	return GTK_MENU_ITEM (w);
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
	char *current_face_name;
	GtkMenuItem *selected_item = NULL, *first_item = NULL;

	current_face_name = g_strdup (my_get_face_name (gfs->current_face));
	gfs->current_face = NULL;

	g_hash_table_remove_all (gfs->item_by_face);

	m = gtk_menu_new ();
	gtk_menu_set_title (GTK_MENU (m), _("Faces"));
	for (faces = g_hash_table_lookup (gfs->faces_by_family, gfs->current_family);
	     faces;
	     faces = faces->next) {
		PangoFontFace *face = faces->data;
		const char *name = my_get_face_name (face);
		GtkMenuItem *w = add_font_to_menu
			(m, g_dpgettext2 (NULL, "FontFace", name),
			 "face", face, gfs->item_by_face);

		if (!first_item)
			first_item = w;

		if (g_strcmp0 (current_face_name, name) == 0)
			selected_item = w;
	}
	if (!selected_item)
		selected_item = first_item;

	gtk_widget_show_all (m);
	go_option_menu_set_menu (GO_OPTION_MENU (gfs->face_picker), m);
	if (selected_item)
		go_option_menu_select_item (GO_OPTION_MENU (gfs->face_picker),
					    selected_item);

	if (selected_item) {
		const char *new_face_name;
		gboolean changed;

		gfs->current_face =
			g_object_get_data (G_OBJECT (selected_item), "face");

		new_face_name = my_get_face_name (gfs->current_face);
		changed = g_strcmp0 (current_face_name, new_face_name) != 0;
		update_preview_after_face_change (gfs, changed);
	}
	g_free (current_face_name);
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
		 * and whether it is an italic or regular face.
		 */
		ADD_OBSERVED (NC_("FontFace", "Bold"));
		ADD_OBSERVED (NC_("FontFace", "Bold Condensed"));
		ADD_OBSERVED (NC_("FontFace", "Bold Condensed Italic"));
		ADD_OBSERVED (NC_("FontFace", "Bold Italic"));
		ADD_OBSERVED (NC_("FontFace", "Bold Oblique"));
		ADD_OBSERVED (NC_("FontFace", "Book"));
		ADD_OBSERVED (NC_("FontFace", "Book Oblique"));
		ADD_OBSERVED (NC_("FontFace", "Condensed"));
		ADD_OBSERVED (NC_("FontFace", "Condensed Bold"));
		ADD_OBSERVED (NC_("FontFace", "Condensed Bold Italic"));
		ADD_OBSERVED (NC_("FontFace", "Condensed Bold Oblique"));
		ADD_OBSERVED (NC_("FontFace", "Condensed Italic"));
		ADD_OBSERVED (NC_("FontFace", "Condensed Oblique"));
		ADD_OBSERVED (NC_("FontFace", "Demi"));
		ADD_OBSERVED (NC_("FontFace", "Demi Bold"));
		ADD_OBSERVED (NC_("FontFace", "Demi Bold Italic"));
		ADD_OBSERVED (NC_("FontFace", "Demi Oblique"));
		ADD_OBSERVED (NC_("FontFace", "Extra Light"));
		ADD_OBSERVED (NC_("FontFace", "Italic"));
		ADD_OBSERVED (NC_("FontFace", "Light"));
		ADD_OBSERVED (NC_("FontFace", "Light Italic"));
		ADD_OBSERVED (NC_("FontFace", "Light Oblique"));
		ADD_OBSERVED (NC_("FontFace", "Medium"));
		ADD_OBSERVED (NC_("FontFace", "Medium Italic"));
		ADD_OBSERVED (NC_("FontFace", "Normal"));
		ADD_OBSERVED (NC_("FontFace", "Oblique"));
		ADD_OBSERVED (NC_("FontFace", "Regular"));
		ADD_OBSERVED (NC_("FontFace", "Regular Condensed"));
		ADD_OBSERVED (NC_("FontFace", "Regular Condensed Italic"));
		ADD_OBSERVED (NC_("FontFace", "Regular Italic"));
		ADD_OBSERVED (NC_("FontFace", "Regular Oblique"));
		ADD_OBSERVED (NC_("FontFace", "Roman"));
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
			add_font_to_menu (m, name,
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
			const char *name;

			if (debug_font &&
			    (name = my_get_face_name (face)) &&
			    !g_hash_table_lookup (observed_faces, name)) {
				g_printerr ("New observed face: %s\n",
					    name);
				ADD_OBSERVED (name);
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
			add_font_to_menu (mother, name,
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

		add_font_to_menu (msingle, name,
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

static void
gfs_screen_changed (GtkWidget *w, GdkScreen *previous_screen)
{
	int width;
	PangoFontDescription *desc;
	GdkScreen *screen;
	GOFontSel *gfs = GO_FONT_SEL (w);

	screen = gtk_widget_get_screen (w);
	if (!previous_screen)
		previous_screen = gdk_screen_get_default ();
	if (screen == previous_screen &&
	    g_hash_table_size (gfs->family_by_name) > 0)
		return;

	reload_families (gfs);

	desc = pango_font_description_from_string ("Sans 72");
	width = go_pango_measure_string
		(gtk_widget_get_pango_context (w),
		 desc,
		 "M");
	pango_font_description_free (desc);
	/* Let's hope pixels are roughly square.  */
	gtk_widget_set_size_request (GTK_WIDGET (gfs->preview_label),
				     5 * width, width * 11 / 10);
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

static gboolean
cb_size_picker_acticated (GtkEntry *entry, GOFontSel *gfs)
{
	const char *text = gtk_entry_get_text (entry);
	double size;
	char *end;

	size = go_strtod (text, &end);
	if (text != end && errno != ERANGE) {
		int psize;

		size = CLAMP (size, 0.0, 1000.0);
		size = floor ((size * 10.) + .5) / 10.;	/* round .1 */
		psize = pango_units_from_double (size);

		g_signal_handlers_block_by_func
			(entry, cb_size_picker_acticated, gfs);
		go_font_sel_set_size (gfs, psize);
		g_signal_handlers_unblock_by_func
			(entry, cb_size_picker_acticated, gfs);

		go_font_sel_add_attr (gfs, pango_attr_size_new (psize));
		go_font_sel_emit_changed (gfs);
	}

	return TRUE;
}

static void
cb_size_picker_changed (GtkButton *button, GOFontSel *gfs)
{
	/*
	 * We get the changed signal also when someone is typing in the
	 * entry.  We don't want that, so if the entry has focus, ignore
	 * the signal.  The entry will send the activate signal itself
	 * when Enter is pressed.
	 */
	if (!gtk_widget_has_focus (gfs->font_size_entry))
		cb_size_picker_acticated (GTK_ENTRY (gfs->font_size_entry),
					  gfs);
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

	gfs->show_preview_entry = TRUE;
	gfs->preview_text = g_strdup (pango_language_get_sample_string (NULL));
	gfs->font_sizes = go_fonts_list_sizes ();
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
	gfs->font_size_entry = gtk_bin_get_child (GTK_BIN (gfs->size_picker));
	g_signal_connect (gfs->size_picker,
			  "changed",
			  G_CALLBACK (cb_size_picker_changed), gfs);
	g_signal_connect (gfs->font_size_entry,
			  "activate",
			  G_CALLBACK (cb_size_picker_acticated), gfs);
	
	/* ---------------------------------------- */

	placeholder = go_gtk_builder_get_widget
		(gfs->gui, "color-picker-placeholder");
	gfs->color_picker = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	g_object_ref_sink (gfs->color_picker);
	gtk_widget_show_all (gfs->color_picker);
	go_gtk_widget_replace (placeholder, gfs->color_picker);
	if (gfs->show_color) {
	} else
		remove_row_containing (gfs->color_picker);

	/* ---------------------------------------- */

	placeholder = go_gtk_builder_get_widget
		(gfs->gui, "script-picker-placeholder");
	gfs->script_picker = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	g_object_ref_sink (gfs->script_picker);
	gtk_widget_show_all (gfs->script_picker);
	go_gtk_widget_replace (placeholder, gfs->script_picker);
	if (gfs->show_script) {
	} else
		remove_row_containing (gfs->script_picker);

	/* ---------------------------------------- */

	gfs->strikethrough_button = go_gtk_builder_get_widget
		(gfs->gui, "strikethrough-button");
	g_object_ref_sink (gfs->strikethrough_button);
	gtk_widget_show_all (gfs->strikethrough_button);
	if (gfs->show_strikethrough) {
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
	g_hash_table_destroy (gfs->family_by_name);
	g_hash_table_destroy (gfs->item_by_family);
	g_hash_table_destroy (gfs->item_by_face);
	g_hash_table_destroy (gfs->faces_by_family);
	gfs_parent_class->finalize (obj);
}

static void
gfs_dispose (GObject *obj)
{
	GOFontSel *gfs = GO_FONT_SEL (obj);

	/* We actually own a ref to these.  */
	g_clear_object (&gfs->face_picker);
	g_clear_object (&gfs->color_picker);
	g_clear_object (&gfs->script_picker);
	g_clear_object (&gfs->strikethrough_button);

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

	case PROP_SHOW_SCRIPT:
		g_value_set_boolean (value, gfs->show_script);
		break;

	case PROP_SHOW_STRIKETHROUGH:
		g_value_set_boolean (value, gfs->show_strikethrough);
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

	case PROP_SHOW_SCRIPT:
		gfs->show_script = g_value_get_boolean (value);
		break;

	case PROP_SHOW_STRIKETHROUGH:
		gfs->show_strikethrough = g_value_get_boolean (value);
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

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gfs_class_init (GObjectClass *klass)
{
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	klass->constructor = gfs_constructor;
	klass->finalize = gfs_finalize;
	klass->dispose = gfs_dispose;
	klass->get_property = gfs_get_property;
	klass->set_property = gfs_set_property;

	widget_class->screen_changed = gfs_screen_changed;

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

	gfs_signals[FONT_CHANGED] =
		g_signal_new (
			"font_changed",
			G_OBJECT_CLASS_TYPE (klass),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (GOFontSelClass, font_changed),
			NULL, NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE, 1, G_TYPE_POINTER);
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
				GTK_WIDGET (gfs->font_size_entry));
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
	PangoAttrList *acopy = pango_attr_list_copy (attrs);
	pango_attr_list_unref (fs->modifications);
	fs->modifications = acopy;

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
		PangoWeight fstyle = pango_font_description_get_style (desc);
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
go_font_sel_set_points (GOFontSel *gfs,
			double point_size)
{
	const char *old_text = gtk_entry_get_text (GTK_ENTRY (gfs->font_size_entry));
	char *buffer = g_strdup_printf ("%g", point_size);
	if (strcmp (old_text, buffer) != 0)
		gtk_entry_set_text (GTK_ENTRY (gfs->font_size_entry), buffer);
	g_free (buffer);
}

void
go_font_sel_set_size (GOFontSel *fs, int size)
{
	go_font_sel_set_points (fs, size / (double)PANGO_SCALE);
}


static void
go_font_sel_set_strike (GOFontSel *gfs, gboolean strike)
{
}

static void
go_font_sel_set_uline (GOFontSel *gfs, int uline)
{
}

static void
go_font_sel_set_color (GOFontSel *gfs, GOColor c)
{
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
	go_font_sel_set_strike (fs, font->strikethrough);
	go_font_sel_set_uline (fs, font->underline);
	go_font_sel_set_color (fs, font->color);
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
