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
	GtkGrid		grid;
	GtkBuilder	*gui;

	GtkWidget	*font_name_entry;
	GtkWidget	*font_style_entry;
	GtkWidget	*font_size_entry;
	GtkTreeView	*font_name_list;
	GtkTreeView	*font_style_list;
	GtkTreeView	*font_size_list;

	GocCanvas	*font_preview_canvas;
	GocItem		*font_preview_text;

	GOFont		*base;
	PangoAttrList	*modifications;

	GPtrArray       *family_faces;
	GHashTable      *family_by_name;
	GHashTable      *family_first_row;

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
go_font_sel_emit_changed (GOFontSel *gfs)
{
	char *fontname = NULL;

	g_signal_emit (G_OBJECT (gfs),
		       gfs_signals[FONT_CHANGED], 0, gfs->modifications);

	g_object_get (gfs, "font", &fontname, NULL);
	g_signal_emit_by_name (gfs, "font-activated", 0, fontname);
	g_free (fontname);

	goc_item_set (gfs->font_preview_text,
		      "attributes", gfs->modifications,
		      NULL);
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

static void
font_selected (GtkTreeSelection *selection, GOFontSel *gfs)
{
	gchar *text;
	GtkTreeModel *model;
	GtkTreeIter   iter;

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter, COL_TEXT, &text, -1);
		gtk_entry_set_text (GTK_ENTRY (gfs->font_name_entry), text);
		go_font_sel_add_attr (gfs, pango_attr_family_new (text));
		go_font_sel_emit_changed (gfs);
		g_free (text);
	}
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

	if (gfs->family_first_row) {
		g_hash_table_destroy (gfs->family_first_row);
		gfs->family_first_row = NULL;
	}

	if (gfs->family_faces) {
		g_ptr_array_foreach (gfs->family_faces,
				     (GFunc)g_object_unref, NULL);
		g_ptr_array_free (gfs->family_faces, TRUE);
		gfs->family_faces = NULL;
	}
}

static void
gfs_fill_font_name_list (GOFontSel *gfs)
{
	GtkListStore *store;
	GtkTreeIter iter;
	PangoContext *context;
	PangoFontFamily **pango_families;
	int n_families;
	size_t ui;
	int row = 0;

	dispose_families (gfs);

	store = GTK_LIST_STORE (gtk_tree_view_get_model (gfs->font_name_list));
	gtk_list_store_clear (store);
	if (!gtk_widget_get_screen (GTK_WIDGET (gfs)))
		return;

	context = gtk_widget_get_pango_context (GTK_WIDGET (gfs));
	pango_context_list_families (context, &pango_families, &n_families);
	qsort (pango_families, n_families,
	       sizeof (pango_families[0]), by_family_name);
	gfs->family_faces = g_ptr_array_new ();
	gfs->family_by_name = g_hash_table_new_full
		(g_str_hash, g_str_equal,
		 (GDestroyNotify)g_free, NULL);
	gfs->family_first_row =
		g_hash_table_new (g_direct_hash, g_direct_equal);
	for (ui = 0; ui < (size_t)n_families; ui++) {
		PangoFontFamily *family = pango_families[ui];
		const char *name = pango_font_family_get_name (family);
		PangoFontFace **faces;
		int j, n_faces;
		gboolean any_face = FALSE;

		pango_font_family_list_faces (family, &faces, &n_faces);
		for (j = 0; j < n_faces; j++) {
			PangoFontFace *face = faces[j];

			if (gfs->filter_func &&
			    !gfs->filter_func (family, face, gfs->filter_data))
				continue;

			if (any_face) {
				if (!gfs->show_style)
					break;
			} else {
				any_face = TRUE;

				g_hash_table_insert (gfs->family_by_name,
						     g_strdup (name),
						     family);
				g_hash_table_insert (gfs->family_first_row,
						     family,
						     GUINT_TO_POINTER (row));
				row++;

				gtk_list_store_append (store, &iter);
				gtk_list_store_set (store, &iter,
						    COL_TEXT, name,
						    COL_OBJ, family,
						    -1);
			}

			g_ptr_array_add (gfs->family_faces,
					 g_object_ref (family));
			g_ptr_array_add (gfs->family_faces,
					 g_object_ref (face));
		}
	}
	g_free (pango_families);
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
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, COL_TEXT, _(styles[i]), -1);
	}
	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (gfs->font_style_list)),
		"changed",
		G_CALLBACK (style_selected), gfs);
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

static double
size_set_text (GOFontSel *gfs, char const *size_text)
{
	char *end;
	double size;
	size = go_strtod (size_text, &end);
	size = CLAMP (size, 0.0, 1000.0);
	size = floor ((size * 20.) + .5) / 20.;	/* round .05 */

	if (size_text != end && errno != ERANGE && 1. <= size && size <= 400.) {
		gtk_entry_set_text (GTK_ENTRY (gfs->font_size_entry), size_text);
		go_font_sel_add_attr (gfs,
				      pango_attr_size_new (size * PANGO_SCALE));
		go_font_sel_emit_changed (gfs);
		return size;
	}
	return -1;
}

static void
size_selected (GtkTreeSelection *selection,
	       GOFontSel *gfs)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	char *size_text;

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter, COL_TEXT, &size_text, -1);
		size_set_text (gfs, size_text);
		g_free (size_text);
	}
}

static void
size_changed (GtkEntry *entry, GOFontSel *gfs)
{
	double size = size_set_text (gfs, gtk_entry_get_text (entry));

	if (size > 0) {
		int psize = (int)(size * PANGO_SCALE + 0.5);
		int i = 0;
		GSList *l;

		for (l = gfs->font_sizes; l; i++, l = l->next) {
			int this_psize = GPOINTER_TO_INT (l->data);
			if (this_psize == psize)
				break;
		}
		g_signal_handlers_block_by_func (
			gtk_tree_view_get_selection (gfs->font_size_list),
			size_selected, gfs);
		select_row (gfs->font_size_list, (l ? i : -1));
		g_signal_handlers_unblock_by_func (
			gtk_tree_view_get_selection (gfs->font_size_list),
			size_selected, gfs);
	}
}

static void
gfs_fill_font_size_list (GOFontSel *gfs)
{
	GtkListStore *store;
	GtkTreeIter   iter;
	GSList       *ptr;

	gfs->font_sizes = go_fonts_list_sizes ();

	store = GTK_LIST_STORE (gtk_tree_view_get_model (gfs->font_size_list));
	for (ptr = gfs->font_sizes; ptr != NULL ; ptr = ptr->next) {
		int psize = GPOINTER_TO_INT (ptr->data);
		char *size_text = g_strdup_printf ("%g", psize / (double)PANGO_SCALE);
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, COL_TEXT, size_text, -1);
		g_free (size_text);
	}
	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (gfs->font_size_list)),
		"changed",
		G_CALLBACK (size_selected), gfs);
	g_signal_connect (G_OBJECT (gfs->font_size_entry),
		"changed",
		G_CALLBACK (size_changed), gfs);
}

static void
canvas_size_changed (G_GNUC_UNUSED GtkWidget *widget,
		     GtkAllocation *allocation, GOFontSel *gfs)
{
	int width  = allocation->width - 1;
	int height = allocation->height - 1;

	goc_item_set (gfs->font_preview_text,
		"x", (double)width/2.,
		"y", (double)height/2.,
		NULL);
}

static void
gfs_init (GOFontSel *gfs)
{
}

static void
gfs_screen_changed (GOFontSel *gfs)
{
	gfs_fill_font_name_list (gfs);
}

static GObject*
gfs_constructor (GType type,
		 guint n_construct_properties,
		 GObjectConstructParam *construct_params)
{
	GtkWidget *w, *fontsel;
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
	gfs->font_name_entry  = go_gtk_builder_get_widget (gfs->gui, "font-name-entry");
	gfs->font_style_entry = go_gtk_builder_get_widget (gfs->gui, "font-style-entry");
	gfs->font_size_entry  = go_gtk_builder_get_widget (gfs->gui, "font-size-entry");
	gfs->font_name_list  = GTK_TREE_VIEW (gtk_builder_get_object (gfs->gui, "font-name-list"));
	gfs->font_style_list = GTK_TREE_VIEW (gtk_builder_get_object (gfs->gui, "font-style-list"));
	gfs->font_size_list  = GTK_TREE_VIEW (gtk_builder_get_object (gfs->gui, "font-size-list"));

	if (!gfs->show_style) {
		gtk_widget_destroy (gfs->font_style_entry);
		gfs->font_style_entry = NULL;
		gtk_widget_destroy (go_gtk_builder_get_widget (gfs->gui, "font-style-window"));
		gfs->font_style_list = NULL;
		gtk_widget_destroy (go_gtk_builder_get_widget (gfs->gui, "font-style-label"));
	}

	w = GTK_WIDGET (g_object_new (GOC_TYPE_CANVAS, NULL));
	gfs->font_preview_canvas = GOC_CANVAS (w);
	gtk_widget_set_hexpand (w, TRUE);
	gtk_widget_set_size_request (w, -1, 96);
	go_gtk_widget_replace
		(go_gtk_builder_get_widget (gfs->gui, "preview-placeholder"),
		 w);
	gtk_widget_show_all (fontsel);
	
	gfs->font_preview_text = goc_item_new (
		goc_canvas_get_root (gfs->font_preview_canvas),
		GOC_TYPE_TEXT,
		NULL);
	go_font_sel_set_sample_text (gfs, NULL); /* init to default */

	g_signal_connect (G_OBJECT (gfs->font_preview_canvas),
		"size-allocate",
		G_CALLBACK (canvas_size_changed), gfs);

	list_init (gfs->font_name_list);
	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (gfs->font_name_list)),
			  "changed",
			  G_CALLBACK (font_selected), gfs);

	g_signal_connect (gfs,
			  "screen-changed",
			  G_CALLBACK (gfs_screen_changed),
			  NULL);

	if (gfs->show_style) {
		list_init (gfs->font_style_list);
		gfs_fill_font_style_list (gfs);
	}

	list_init (gfs->font_size_list);
	gfs_fill_font_size_list (gfs);

	return (GObject *)gfs;
}


static void
gfs_dispose (GObject *obj)
{
	GOFontSel *gfs = GO_FONT_SEL (obj);

	if (gfs->gui) {
		g_object_unref (gfs->gui);
		gfs->gui = NULL;
		gfs->font_name_list = NULL;
		gfs->font_style_list = NULL;
		gfs->font_size_list = NULL;
	}
	if (gfs->base != NULL) {
		go_font_unref (gfs->base);
		gfs->base = NULL;
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
		go_font_sel_set_sample_text (gfs, gfs->preview_text);
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

	if (gfs->font_name_list) {
		PangoFontDescription *desc = go_font_sel_get_font_desc (gfs);
		gfs_fill_font_name_list (gfs);
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
		gfs_init, GTK_TYPE_GRID, 0,
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
				GTK_WIDGET (gfs->font_name_entry));
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
	goc_item_set (gfs->font_preview_text,
		/* xgettext: This text is used as a sample when selecting a font
		 * please choose a translation that would produce common
		 * characters specific to the target alphabet. */
		"text",  ((text == NULL) ? _("AaBbCcDdEe12345") : text),
		NULL);
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

	acopy = go_pango_translate_attributes (acopy);

	goc_item_set (fs->font_preview_text,
		      "attributes", acopy,
		      NULL);

	if (acopy != fs->modifications)
		pango_attr_list_unref (acopy);
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
	int row = family
		? GPOINTER_TO_INT (g_hash_table_lookup (fs->family_first_row, family))
		: -1;
	select_row (fs->font_name_list, row);
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

	go_font_sel_emit_changed (fs);
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

	return desc;
}
