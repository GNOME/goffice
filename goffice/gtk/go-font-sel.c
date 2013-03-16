/*
 * A font selector widget.  This is a simplified version of the
 * GnomePrint font selector widget.
 *
 * Authors:
 *   Jody Goldberg (jody@gnome.org)
 *   Miguel de Icaza (miguel@gnu.org)
 *   Almer S. Tigelaar (almer@gnome.org)
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
	GtkWidget	*font_style_entry;
	GtkWidget	*font_size_entry;
	GtkWidget       *size_picker;
	GtkTreeView	*font_style_list;

	GtkWidget       *preview_label;

	PangoAttrList	*modifications;

	GPtrArray       *family_faces;
	GHashTable      *family_by_name;
	GHashTable      *item_by_family;

	GSList          *font_sizes;

	gboolean        show_style;
	char            *preview_text;

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

static void
cb_list_adjust (GtkTreeView* view)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;

	if (gtk_tree_selection_get_selected (gtk_tree_view_get_selection (view), &model, &iter)) {
		path = gtk_tree_model_get_path (model, &iter);
		gtk_tree_view_set_cursor (view, path, NULL, FALSE);
		gtk_tree_path_free (path);
	}
}

static void
list_init (GtkTreeView* view)
{
	GtkCellRenderer *renderer;
	GtkListStore *store;
	GtkTreeViewColumn *column;

	gtk_tree_view_set_headers_visible (view, FALSE);
	store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_OBJECT);
	gtk_tree_view_set_model (view, GTK_TREE_MODEL (store));
	g_object_unref (store);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (
			NULL, renderer, "text", COL_TEXT, NULL);
	gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_append_column (view, column);
	g_signal_connect (view, "realize", G_CALLBACK (cb_list_adjust), NULL);
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
	if (gfs->family_by_name) {
		g_hash_table_destroy (gfs->family_by_name);
		gfs->family_by_name = NULL;
	}

	if (gfs->item_by_family) {
		g_hash_table_destroy (gfs->item_by_family);
		gfs->item_by_family = NULL;
	}

	if (gfs->family_faces) {
		g_ptr_array_foreach (gfs->family_faces,
				     (GFunc)g_object_unref, NULL);
		g_ptr_array_free (gfs->family_faces, TRUE);
		gfs->family_faces = NULL;
	}
}

static void
add_font_to_menu (GtkWidget *m, const char *name, PangoFontFamily *family,
		  GHashTable *ibf)
{
	GtkWidget *w = gtk_menu_item_new_with_label (name);
	gtk_menu_shell_append (GTK_MENU_SHELL (m), w);
	g_object_set_data (G_OBJECT (w), "family", family);
	if (!g_hash_table_lookup (ibf, family))
		g_hash_table_insert (ibf, family, w);
}

static void
add_submenu_to_menu (GtkWidget *m, const char *name, GtkWidget *m2)
{
	GtkWidget *sm = gtk_menu_item_new_with_label (name);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (sm), m2);
	gtk_menu_shell_append (GTK_MENU_SHELL (m), sm);
}

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

	if (!gtk_widget_get_screen (GTK_WIDGET (gfs)))
		return;

	dispose_families (gfs);

	context = gtk_widget_get_pango_context (GTK_WIDGET (gfs));
	pango_context_list_families (context, &pango_families, &n_families);
	qsort (pango_families, n_families,
	       sizeof (pango_families[0]), by_family_name);

	gfs->family_by_name = g_hash_table_new_full
		(g_str_hash, g_str_equal, (GDestroyNotify)g_free, NULL);
	gfs->item_by_family =
		g_hash_table_new (g_direct_hash, g_direct_equal);
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
			add_font_to_menu (m, name, family, gfs->item_by_family);
		}
	}
	if (has_priority)
		gtk_menu_shell_append (GTK_MENU_SHELL (m),
				       gtk_separator_menu_item_new ());

	mall = gtk_menu_new ();
	for (i = 0; i < n_families; i++) {
		PangoFontFamily *family = pango_families[i];
		const char *name = pango_font_family_get_name (family);
		gunichar fc;

		fc = g_unichar_toupper (g_utf8_get_char (name));

		if (!g_unichar_isalpha (fc)) {
			if (!mother)
				mother = gtk_menu_new ();
			add_font_to_menu (mother, name, family, gfs->item_by_family);
			continue;
		}

		if (fc != uc || !msingle) {
			char txt[10];

			uc = fc;
			txt[g_unichar_to_utf8 (uc, txt)] = 0;
			msingle = gtk_menu_new ();
			add_submenu_to_menu (mall, txt, msingle);
		}

		add_font_to_menu (msingle, name, family, gfs->item_by_family);
	}
	if (mother)
		add_submenu_to_menu (mall, _("Other"), mother);
	add_submenu_to_menu (m, _("All fonts..."), mall);
	gtk_widget_show_all (m);

	g_free (pango_families);

	go_option_menu_set_menu (GO_OPTION_MENU (gfs->family_picker), m);
}

static void
gfs_screen_changed (GOFontSel *gfs)
{
	int width;
	PangoFontDescription *desc;

	if (!gtk_widget_get_screen (GTK_WIDGET (gfs)))
		return;

	reload_families (gfs);

	desc = pango_font_description_from_string ("Sans 72");
	width = go_pango_measure_string
		(gtk_widget_get_pango_context (GTK_WIDGET (gfs)),
		 desc,
		 "M");
	pango_font_description_free (desc);
	/* Let's hope pixels are roughly square.  */
	gtk_widget_set_size_request (GTK_WIDGET (gfs->preview_label),
				     5 * width, width * 11 / 10);
}

static char const *styles[] = {
	 N_("Normal"),
	 N_("Bold"),
	 N_("Bold italic"),
	 N_("Italic"),
	 NULL
};

static void
style_selected (GtkTreeSelection *selection,
		GOFontSel *gfs)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;
	int row;

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		path = gtk_tree_model_get_path (model, &iter);
		row = *gtk_tree_path_get_indices (path);
		gtk_tree_path_free (path);
		gtk_entry_set_text (GTK_ENTRY (gfs->font_style_entry), _(styles[row]));
		go_font_sel_add_attr (gfs,
			pango_attr_weight_new ((row == 1 || row == 2)
					       ?  PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL));
		go_font_sel_add_attr (gfs,
			pango_attr_style_new ((row == 2 || row == 3)
				? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL));
		go_font_sel_emit_changed (gfs);
	}
}

static void
gfs_fill_font_style_list (GOFontSel *gfs)
{
	int i;
	GtkListStore *store;
	GtkTreeIter iter;

	store = GTK_LIST_STORE (gtk_tree_view_get_model (gfs->font_style_list));
	for (i = 0; styles[i] != NULL; i++) {
		PangoFontFace *face = NULL;
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    COL_TEXT, _(styles[i]),
				    COL_OBJ, face,
				    -1);
	}
	g_signal_connect (gtk_tree_view_get_selection (gfs->font_style_list),
			  "changed",
			  G_CALLBACK (style_selected), gfs);
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
select_row (GtkTreeView *list, int row)
{
	if (row < 0)
		gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (list));
	else {
		GtkTreePath *path = gtk_tree_path_new_from_indices (row, -1);

		gtk_tree_selection_select_path (gtk_tree_view_get_selection (list), path);
		gtk_tree_path_free (path);

		if (gtk_widget_get_realized (GTK_WIDGET (list)))
			cb_list_adjust (list);
	}
}

static void
cb_font_changed (GOOptionMenu *om, GOFontSel *gfs)
{
	GtkWidget *selected = go_option_menu_get_history (om);
	PangoFontFamily *family = selected
		? g_object_get_data (G_OBJECT (selected), "family")
		: NULL;
	if (family) {
		const char *name = pango_font_family_get_name (family);
		go_font_sel_add_attr (gfs, pango_attr_family_new (name));
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
	go_font_sel_set_sample_text (gfs, _("AaBbCcDdEe12345"));
	gfs->font_sizes = go_fonts_list_sizes ();
}

static GObject*
gfs_constructor (GType type,
		 guint n_construct_properties,
		 GObjectConstructParam *construct_params)
{
	GtkWidget *fontsel;
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
	gfs->font_style_entry = go_gtk_builder_get_widget (gfs->gui, "font-style-entry");
	gfs->font_style_list = GTK_TREE_VIEW (gtk_builder_get_object (gfs->gui, "font-style-list"));

	if (!gfs->show_style) {
		gtk_widget_destroy (gfs->font_style_entry);
		gfs->font_style_entry = NULL;
		gtk_widget_destroy (go_gtk_builder_get_widget (gfs->gui, "font-style-window"));
		gfs->font_style_list = NULL;
		gtk_widget_destroy (go_gtk_builder_get_widget (gfs->gui, "font-style-label"));
	}

	gfs->preview_label = go_gtk_builder_get_widget (gfs->gui, "preview-label");
	/* ---------------------------------------- */

	gfs->family_picker = go_option_menu_new ();
	gtk_widget_show_all (gfs->family_picker);
	go_gtk_widget_replace (go_gtk_builder_get_widget (gfs->gui, "family-picker-placeholder"),
			       gfs->family_picker);
	g_signal_connect (gfs->family_picker,
			  "changed",
			  G_CALLBACK (cb_font_changed),
			  gfs);

	/* ---------------------------------------- */

	if (gfs->show_style) {
		list_init (gfs->font_style_list);
		gfs_fill_font_style_list (gfs);
	}

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

	g_signal_connect (gfs,
			  "screen-changed",
			  G_CALLBACK (gfs_screen_changed),
			  NULL);

	gtk_widget_show_all (fontsel);

	return (GObject *)gfs;
}


static void
gfs_dispose (GObject *obj)
{
	GOFontSel *gfs = GO_FONT_SEL (obj);

	if (gfs->gui) {
		g_object_unref (gfs->gui);
		gfs->gui = NULL;
		gfs->family_picker = NULL;
		gfs->font_style_list = NULL;
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
		/* Not implemented */
		g_value_set_boolean (value, TRUE);
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
		g_free (gfs->preview_text);
		gfs->preview_text = g_value_dup_string (value);
		update_preview (gfs);
		break;

	case GFS_GTK_FONT_CHOOSER_PROP_SHOW_PREVIEW_ENTRY:
		/* Not implemented */
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gfs_class_init (GObjectClass *klass)
{
	klass->constructor = gfs_constructor;
	klass->dispose = gfs_dispose;
	klass->get_property = gfs_get_property;
	klass->set_property = gfs_set_property;

	gfs_parent_class = g_type_class_peek_parent (klass);

	g_object_class_install_property
		(klass, PROP_SHOW_STYLE,
		 g_param_spec_boolean ("show-style",
				       _("Show Style"),
				       _("Whether style is part of the font being selected"),
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

	if (gfs->family_picker) {
		PangoFontDescription *desc = go_font_sel_get_font_desc (gfs);
		reload_families (gfs);
		go_font_sel_set_font_desc (gfs, desc);
		pango_font_description_free (desc);
	}
}

static PangoFontFamily *
gfs_font_chooser_get_font_family (GtkFontChooser *chooser)
{
	GOFontSel *gfs = GO_FONT_SEL (chooser);
	PangoFontDescription *desc = go_font_sel_get_font_desc (gfs);
	const char *name = pango_font_description_get_family (desc);
	PangoFontFamily *family = g_hash_table_lookup (gfs->family_by_name,
						       name);
	pango_font_description_free (desc);
	return family;
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
	(void)gfs;
	return NULL;
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
	if (gfs->font_style_entry)
		go_gtk_editable_enters (dialog,
					GTK_WIDGET (gfs->font_style_entry));
	go_gtk_editable_enters (dialog,
				GTK_WIDGET (gfs->font_size_entry));
}

void
go_font_sel_set_sample_text (GOFontSel *gfs, char const *text)
{
	g_return_if_fail (GO_IS_FONT_SEL (gfs));

	g_object_set (gfs, "preview-text", text, NULL);
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
	if (item) {
		go_option_menu_select_item (GO_OPTION_MENU (fs->family_picker),
					    item);
		go_font_sel_add_attr (fs, pango_attr_family_new (font_name));
		update_preview (fs);
	}
}

void
go_font_sel_set_style (GOFontSel *fs, PangoWeight weight, PangoStyle style)
{
	gboolean is_bold = (weight >= PANGO_WEIGHT_BOLD);
	gboolean is_italic = (style != PANGO_STYLE_NORMAL);
	int n;

	if (is_bold) {
		if (is_italic)
			n = 2;
		else
			n = 1;
	} else {
		if (is_italic)
			n = 3;
		else
			n = 0;
	}

	if (fs->font_style_list)
		select_row (fs->font_style_list, n);

	go_font_sel_add_attr (fs, pango_attr_weight_new (weight));
	go_font_sel_add_attr (fs, pango_attr_style_new (style));

	update_preview (fs);
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
