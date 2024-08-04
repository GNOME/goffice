/*
 * goffice-gtk.c: Misc gtk utilities
 *
 * Copyright (C) 2004 Jody Goldberg (jody@gnome.org)
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
#include <goffice/goffice.h>
#include <goffice/goffice-priv.h>

#include <gdk/gdkkeysyms.h>

#include <atk/atkrelation.h>
#include <atk/atkrelationset.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <gsf/gsf-input-stdio.h>
#include <gsf/gsf-input-textline.h>
#include <gsf/gsf-input-memory.h>

#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>

#define PREVIEW_HSIZE 150
#define PREVIEW_VSIZE 150

#ifndef GDK_KEY_Escape
#       define GDK_KEY_Escape GDK_Escape
#endif

/* ------------------------------------------------------------------------- */


/**
 * go_gtk_button_build_with_stock:
 * @text: button label
 * @stock_id: icon name (or stock id)
 *
 * FROM : gedit
 * Creates a new GtkButton with custom label and stock image.
 *
 * Since: 0.9.9
 * Returns: (transfer full): newly created button
 **/
GtkWidget*
go_gtk_button_build_with_stock (char const *text, char const* stock_id)
{
	GtkStockItem item;
	GtkWidget *button = gtk_button_new_with_mnemonic (text);
	if (gtk_stock_lookup (stock_id, &item))
		gtk_button_set_image (GTK_BUTTON (button),
			gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_BUTTON));
	else
		gtk_button_set_image (GTK_BUTTON (button),
				      gtk_image_new_from_icon_name (stock_id, GTK_ICON_SIZE_BUTTON));
	return button;
}

/**
 * go_gtk_button_new_with_stock:
 * @text: button label
 * @stock_id: id for stock icon
 *
 * FROM : gedit
 * Creates a new GtkButton with custom label and stock image.
 *
 * Deprecated: 0.9.6., use go_gtk_button_build_with_stock().
 * Returns: (transfer full): newly created button
 **/
GtkWidget*
go_gtk_button_new_with_stock (char const *text, char const* stock_id)
{
	return go_gtk_button_build_with_stock (text, stock_id);
}

/**
 * go_gtk_dialog_add_button:
 * @dialog: dialog you want to add a button
 * @text: button label
 * @stock_id: stock icon id
 * @response_id: respond id when button clicked
 *
 * FROM : gedit
 * Creates and adds a button with stock image to the action area of an existing dialog.
 *
 * Returns: (transfer none): newly created button
 **/
GtkWidget*
go_gtk_dialog_add_button (GtkDialog *dialog, char const* text, char const* stock_id,
			  gint response_id)
{
	GtkWidget *button;

	g_return_val_if_fail (GTK_IS_DIALOG (dialog), NULL);
	g_return_val_if_fail (text != NULL, NULL);
	g_return_val_if_fail (stock_id != NULL, NULL);

	button = go_gtk_button_build_with_stock (text, stock_id);
	g_return_val_if_fail (button != NULL, NULL);

	gtk_widget_set_can_default (button, TRUE);

	gtk_widget_show (button);

	gtk_dialog_add_action_widget (dialog, button, response_id);

	return button;
}


static gboolean
apply_ui_from_file (GtkBuilder *gui, GsfInput *src, const char *uifile,
		    GError **error)
{
	gboolean res;
	GsfInput *orig_src;

	if (!src)
		return FALSE;

	orig_src = g_object_ref (src);
	src = gsf_input_uncompress (src);
	if (uifile && src == orig_src) {
		/*
		 * This is sad, but see bugs 662503 and 662679.  Since
		 * ui files can contain relative filenames we must use
		 * the _file interface to load such files.  Do not
		 * compress such files for the time being.
		 */
		res = gtk_builder_add_from_file (gui, uifile, error);
	} else {
		size_t size = gsf_input_size (src);
		gconstpointer data = gsf_input_read (src, size, NULL);
		res = gtk_builder_add_from_string (gui, data, size, error);
	}
	g_object_unref (src);
	g_object_unref (orig_src);
	return res;
}


/**
 * go_gtk_builder_load:
 * @uifile: the name of the file load
 * @domain: the translation domain
 * @gcc: #GOCmdContext
 *
 * Simple utility to open ui files
 *
 * Since 0.9.6
 * Returns: (transfer full): a new #GtkBuilder or %NULL
 *
 * @uifile should be one of these:
 *
 * res:NAME  -- data from resource manager
 * data:DATA -- data right here
 * filename  -- data from local file
 *
 * Data may be compressed, regardless of source.
**/
GtkBuilder *
go_gtk_builder_load (char const *uifile,
		     char const *domain, GOCmdContext *gcc)
{
	GtkBuilder *gui;
	GError *error = NULL;
	gboolean ok = FALSE;

	g_return_val_if_fail (uifile != NULL, NULL);

	gui = gtk_builder_new ();
	if (domain)
		gtk_builder_set_translation_domain (gui, domain);

	if (strncmp (uifile, "res:", 4) == 0) {
		const char *resname = uifile + 4;
		size_t len;
		gconstpointer data;
		GsfInput *src;
		GBytes *bytes = NULL;

		// First try go resources
		data = go_rsm_lookup (resname, &len);

		// Then try glib
		if (!data) {
			bytes = g_resources_lookup_data (resname, 0, NULL);
			if (bytes) {
				data = g_bytes_get_data (bytes, NULL);
				len = g_bytes_get_size (bytes);
			}
		}

		src = data
			? gsf_input_memory_new (data, len, FALSE)
			: NULL;
		ok = apply_ui_from_file (gui, src, NULL, &error);

		if (bytes)
			g_bytes_unref (bytes);
	} else if (strncmp (uifile, "data:", 5) == 0) {
		const char *data = uifile + 5;
		GsfInput *src = gsf_input_memory_new (data, strlen (data), FALSE);
		ok = apply_ui_from_file (gui, src, NULL, &error);
	} else {
		/* we need to set the current directory so that the builder can find pixbufs */
		GsfInput *src = gsf_input_stdio_new (uifile, &error);
		ok = apply_ui_from_file (gui, src, uifile, &error);
	}

	if (!ok) {
		g_object_unref (gui);
		gui = NULL;
	}

	if (gui == NULL && gcc != NULL) {
		char *msg;
		if (error) {
			msg = g_strdup (error->message);
			g_error_free (error);
		} else
			msg = g_strdup_printf (_("Unable to open file '%s'"), uifile);
		go_cmd_context_error_system (gcc, msg);
		g_free (msg);
	} else if (error)
		g_error_free (error);

	if (gui && go_debug_flag ("leaks")) {
		GSList *l, *objs = gtk_builder_get_objects (gui);
		for (l = objs; l; l = l->next) {
			GObject *obj = l->data;
			/* Darn -- cannot access object name! */
			char *name = g_strdup_printf ("Anonymous from %s", uifile);
			go_debug_check_finalized (obj, name);
			g_free (name);
		}
		g_slist_free (objs);
	}

	return gui;
}


/**
 * go_gtk_builder_new:
 * @uifile: the name of the file load
 * @domain: the translation domain
 * @gcc: #GOCmdContext
 *
 * Simple utility to open ui files
 *
 * Deprecated: 0.9.6, use go_gtk_builder_load().
 * Returns: (transfer full): a new #GtkBuilder or %NULL
 *
 * @uifile should be one of these:
 *
 * res:NAME  -- data from resource manager
 * data:DATA -- data right here
 * filename  -- data from local file
 *
 * Data may be compressed, regardless of source.
**/
GtkBuilder *
go_gtk_builder_new (char const *uifile,
		    char const *domain, GOCmdContext *gcc)
{
	return go_gtk_builder_load (uifile, domain, gcc);
}

/**
 * go_gtk_builder_load_internal:
 * @uifile: the name of the file load
 * @domain: the translation domain
 * @gcc: #GOCmdContext
 *
 * Simple utility to open ui files
 *
 * Since: 0.9.6
 * Returns: (transfer full): a new #GtkBuilder or %NULL
 *
 * Variant of go_gtk_builder_new that searchs goffice directories
 * for files.
 * @uifile should be one of these:
 *
 * res:NAME  -- data from resource manager
 * data:DATA -- data right here
 * filename  -- data from local file
 *
 * Data may be compressed, regardless of source.
**/
GtkBuilder *
go_gtk_builder_load_internal (char const *uifile,
			      char const *domain, GOCmdContext *gcc)
{
	char *f;
	GtkBuilder *res;

	if (g_path_is_absolute (uifile) ||
	    strncmp (uifile, "res:", 4) == 0 ||
	    strncmp (uifile, "data:", 5) == 0)
		return go_gtk_builder_load (uifile, domain, gcc);

	f = g_build_filename (go_sys_data_dir (), "ui", uifile, NULL);
	res = go_gtk_builder_load (f, domain, gcc);
	g_free (f);

	return res;
}

/**
 * go_gtk_builder_signal_connect:
 * @gui: #GtkBuilder
 * @instance_name: widget name
 * @detailed_signal: signal name
 * @c_handler: (scope async): #GCallback
 * @data: arbitrary
 *
 * Convenience wrapper around g_signal_connect for GtkBuilder.
 *
 * Returns: The signal id
 **/
gulong
go_gtk_builder_signal_connect	(GtkBuilder	*gui,
			 gchar const	*instance_name,
			 gchar const	*detailed_signal,
			 GCallback	 c_handler,
			 gpointer	 data)
{
	GObject *obj;
	g_return_val_if_fail (gui != NULL, 0);
	obj = gtk_builder_get_object (gui, instance_name);
	g_return_val_if_fail (obj != NULL, 0);
	return g_signal_connect (obj, detailed_signal, c_handler, data);
}

/**
 * go_gtk_builder_signal_connect_swapped:
 * @gui: #GtkBuilder
 * @instance_name: widget name
 * @detailed_signal: signal name
 * @c_handler: (scope async): #GCallback
 * @data: arbitary
 *
 * Convenience wrapper around g_signal_connect_swapped for GtkBuilder.
 *
 * Returns: The signal id
 **/
gulong
go_gtk_builder_signal_connect_swapped (GtkBuilder	*gui,
				 gchar const	*instance_name,
				 gchar const	*detailed_signal,
				 GCallback	 c_handler,
				 gpointer	 data)
{
	GObject *obj;
	g_return_val_if_fail (gui != NULL, 0);
	obj = gtk_builder_get_object (gui, instance_name);
	g_return_val_if_fail (obj != NULL, 0);
	return g_signal_connect_swapped (obj, detailed_signal, c_handler, data);
}

/**
 * go_gtk_builder_get_widget:
 * @gui: the #GtkBuilder
 * @widget_name: the name of the combo box in the ui file.
 *
 * Simple wrapper to #gtk_builder_get_object which returns the object
 * as a GtkWidget.
 *
 * Returns: (transfer none): a new #GtkWidget or %NULL
 **/
GtkWidget *
go_gtk_builder_get_widget (GtkBuilder *gui, char const *widget_name)
{
	return GTK_WIDGET (gtk_builder_get_object (gui, widget_name));
}

/**
 * go_gtk_builder_combo_box_init_text:
 * @gui: the #GtkBuilder
 * @widget_name: the name of the combo box in the ui file.
 *
 * searches the #GtkComboBox in @gui and ensures it has a model and a
 * renderer appropriate for using with #gtk_combo_box_append_text and friends.
 *
 * Returns: (transfer none): the #GtkComboBox or %NULL
 **/
GtkComboBox *
go_gtk_builder_combo_box_init_text (GtkBuilder *gui, char const *widget_name)
{
	GtkComboBox *box;
	GList *cells;
	g_return_val_if_fail (gui != NULL, NULL);
	box = GTK_COMBO_BOX (gtk_builder_get_object (gui, widget_name));
	/* search for the model and create one if none exists */
	g_return_val_if_fail (box != NULL, NULL);
	if (gtk_combo_box_get_model (box) == NULL) {
		GtkListStore *store = gtk_list_store_new (1, G_TYPE_STRING);
		gtk_combo_box_set_model (box, GTK_TREE_MODEL (store));
		g_object_unref (store);
	}
	cells = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (box));
	if (g_list_length (cells) == 0) {
		/* populate the cell layout */
		GtkCellRenderer *cell = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (box), cell, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (box), cell,
						"text", 0, NULL);
	}
	g_list_free (cells);
	return box;
}

void
go_gtk_combo_box_append_text (GtkComboBox *combo, char const *str)
{
	GtkListStore *model;
	GtkTreeIter iter;

	g_return_if_fail (GTK_IS_COMBO_BOX (combo));
	g_return_if_fail (str != NULL);

	model = GTK_LIST_STORE (gtk_combo_box_get_model (combo));
	gtk_list_store_append (model, &iter);
	gtk_list_store_set (model, &iter, 0, str, -1);
}

void
go_gtk_combo_box_remove_text (GtkComboBox *combo, int position)
{
	GtkTreeModel *model;
	GtkListStore *store;
	GtkTreeIter iter;

	g_return_if_fail (GTK_IS_COMBO_BOX_TEXT (combo));
	g_return_if_fail (position >= 0);

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
	store = GTK_LIST_STORE (model);
	g_return_if_fail (GTK_IS_LIST_STORE (store));

	if (gtk_tree_model_iter_nth_child (model, &iter, NULL, position))
		gtk_list_store_remove (store, &iter);
}

int
go_gtk_builder_group_value (GtkBuilder *gui, char const * const group[])
{
	int i;
	for (i = 0; group[i]; i++) {
		GtkWidget *w = go_gtk_builder_get_widget (gui, group[i]);
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
			return i;
	}
	return -1;
}

/*
 * A variant of gtk_window_activate_default that does not end up reactivating
 * the widget that [Enter] was pressed in.
 */
static void
cb_activate_default (GtkWindow *window)
{
	GtkWidget *w = gtk_window_get_default_widget (window);
	if (w && gtk_widget_is_sensitive (w))
		gtk_widget_activate (w);
}

/**
 * go_gtk_editable_enters:
 * @window: #GtkWindow
 * @w:  #GtkWidget
 *
 * Normally if there's an editable widget (such as #GtkEntry) in your
 * dialog, pressing Enter will activate the editable rather than the
 * default dialog button. However, in most cases, the user expects to
 * type something in and then press enter to close the dialog. This
 * function enables that behavior.
 **/
void
go_gtk_editable_enters (GtkWindow *window, GtkWidget *w)
{
	g_return_if_fail (GTK_IS_WINDOW (window));
	g_signal_connect_swapped (G_OBJECT (w),
		"activate",
		G_CALLBACK (cb_activate_default), window);
}

void
go_gtk_widget_replace (GtkWidget *victim, GtkWidget *replacement)
{
	GtkContainer *parent = GTK_CONTAINER (gtk_widget_get_parent (victim));

	if (GTK_IS_GRID (parent)) {
		int col, row, width, height;
		gtk_container_child_get (parent,
					 victim,
					 "left-attach", &col,
					 "top-attach", &row,
					 "width", &width,
					 "height", &height,
					 NULL);
		gtk_container_remove (parent, victim);
		gtk_grid_attach (GTK_GRID (parent), replacement,
				 col, row, width, height);
	} else if (GTK_IS_BOX (parent)) {
		GtkBox *box = GTK_BOX (parent);
		gboolean expand, fill;
		guint padding;
		GtkPackType pack_type;
		int pos;

		gtk_box_query_child_packing (box, victim,
					     &expand, &fill,
					     &padding, &pack_type);
		gtk_container_child_get (parent, victim,
					 "position", &pos,
					 NULL);
		gtk_container_remove (parent, victim);
		gtk_container_add (parent, replacement);
		gtk_box_set_child_packing (box, replacement,
					   expand, fill,
					   padding, pack_type);
		gtk_box_reorder_child (box, replacement, pos);
	} else {
		g_error ("Unsupported container: %s",
			 g_type_name_from_instance ((gpointer)parent));
	}
}

struct go_gtk_grid_data {
	GtkWidget *child;
	int top_attach, left_attach, height, width;
};

static GList *
get_grid_data (GtkGrid *grid)
{
	GtkContainer *cont = GTK_CONTAINER (grid);
	GList *children = gtk_container_get_children (cont);
	GList *p;

	for (p = children; p; p = p->next) {
		GtkWidget *child = p->data;
		struct go_gtk_grid_data *data = g_new (struct go_gtk_grid_data, 1);
		data->child = child;
		gtk_container_child_get (cont, child,
					 "top-attach", &data->top_attach,
					 "height", &data->height,
					 "left-attach", &data->left_attach,
					 "width", &data->width,
					 NULL);
		p->data = data;
	}

	return children;
}

static int
by_row (struct go_gtk_grid_data *a, struct go_gtk_grid_data *b)
{
	return a->top_attach - b->top_attach;
}


void
go_gtk_grid_remove_row (GtkGrid *grid, int row)
{
	GtkContainer *cont = GTK_CONTAINER (grid);
	GList *children =
		g_list_sort (get_grid_data (grid), (GCompareFunc)by_row);
	GList *p;

	for (p = children; p; p = p->next) {
		struct go_gtk_grid_data *data = p->data;

		if (data->top_attach <= row &&
		    data->top_attach + data->height > row)
			data->height--;

		if (data->top_attach > row)
			data->top_attach--;

		if (data->height <= 0)
			gtk_container_remove (cont, data->child);
		else
			gtk_container_child_set (cont, data->child,
						 "height", data->height,
						 "top-attach", data->top_attach,
						 NULL);
	}
	g_list_free_full (children, (GDestroyNotify)g_free);
}


/**
 * go_gtk_widget_disable_focus:
 * @w: #GtkWidget
 *
 * Convenience wrapper to disable focus on a container and it's children.
 **/
void
go_gtk_widget_disable_focus (GtkWidget *w)
{
	if (GTK_IS_CONTAINER (w))
		gtk_container_foreach (GTK_CONTAINER (w),
			(GtkCallback) go_gtk_widget_disable_focus, NULL);
	gtk_widget_set_can_focus (w, FALSE);
}

static void
cb_parent_mapped (GtkWidget *parent, GtkWindow *window)
{
	if (gtk_widget_get_mapped (GTK_WIDGET (window))) {
		gtk_window_present (window);
		g_signal_handlers_disconnect_by_func (G_OBJECT (parent),
			G_CALLBACK (cb_parent_mapped), window);
	}
}

/**
 * go_gtk_window_set_transient:
 * @toplevel: The calling window
 * @window: the transient window
 *
 * Make the window a child of the workbook in the command context, if there is
 * one.  The function duplicates the positioning functionality in
 * gnome_dialog_set_parent, but does not require the transient window to be
 * a GnomeDialog.
 **/
void
go_gtk_window_set_transient (GtkWindow *toplevel, GtkWindow *window)
{
#if 0
	GtkWindowPosition position = gnome_preferences_get_dialog_position(); */
	if (position == GTK_WIN_POS_NONE)
		position = GTK_WIN_POS_CENTER_ON_PARENT;
#else
	GtkWindowPosition position = GTK_WIN_POS_CENTER_ON_PARENT;
#endif

	g_return_if_fail (GTK_IS_WINDOW (toplevel));
	g_return_if_fail (GTK_IS_WINDOW (window));

	gtk_window_set_transient_for (window, toplevel);

	gtk_window_set_position (window, position);

	if (!gtk_widget_get_mapped (GTK_WIDGET (toplevel)))
		g_signal_connect_after (G_OBJECT (toplevel),
			"map",
			G_CALLBACK (cb_parent_mapped), window);
}

static gint
cb_non_modal_dialog_keypress (GtkWidget *w, GdkEventKey *e)
{
	if(e->keyval == GDK_KEY_Escape) {
		gtk_widget_destroy (w);
		return TRUE;
	}

	return FALSE;
}

/**
 * go_gtk_nonmodal_dialog:
 * @toplevel: #GtkWindow
 * @dialog: #GtkWindow
 *
 * Utility to set @dialog as a transient of @toplevel
 * and to set up a handler for "Escape"
 **/
void
go_gtk_nonmodal_dialog (GtkWindow *toplevel, GtkWindow *dialog)
{
	go_gtk_window_set_transient (toplevel, dialog);
	g_signal_connect (G_OBJECT (dialog),
		"key-press-event",
		G_CALLBACK (cb_non_modal_dialog_keypress), NULL);
}

static void
fsel_response_cb (GtkFileChooser *dialog,
		  gint response_id,
		  gboolean *result)
{
	if (response_id == GTK_RESPONSE_OK) {
		char *uri = gtk_file_chooser_get_uri (dialog);

		if (uri) {
			g_free (uri);
			*result = TRUE;
		}
	}

	gtk_main_quit ();
}

static gint
gu_delete_handler (GtkDialog *dialog,
		   GdkEventAny *event,
		   gpointer data)
{
	gtk_dialog_response (dialog, GTK_RESPONSE_CANCEL);
	return TRUE; /* Do not destroy */
}

/**
 * go_gtk_file_sel_dialog:
 * @toplevel: #GtkWindow
 * @w: #GtkWidget
 *
 * Runs a modal dialog to select a file.
 *
 * Returns: %TRUE if a file was selected.
 **/
gboolean
go_gtk_file_sel_dialog (GtkWindow *toplevel, GtkWidget *w)
{
	gboolean result = FALSE;
	gulong delete_handler;

	g_return_val_if_fail (GTK_IS_WINDOW (toplevel), FALSE);
	g_return_val_if_fail (GTK_IS_FILE_CHOOSER (w), FALSE);

	gtk_window_set_modal (GTK_WINDOW (w), TRUE);
	go_gtk_window_set_transient (toplevel, GTK_WINDOW (w));
	g_signal_connect (w, "response",
		G_CALLBACK (fsel_response_cb), &result);
	delete_handler = g_signal_connect (w, "delete_event",
		G_CALLBACK (gu_delete_handler), NULL);

	gtk_widget_show (w);
	gtk_grab_add (w);

	gtk_main ();

	g_signal_handler_disconnect (w, delete_handler);

	return result;
}

static gboolean have_pixbufexts = FALSE;
static GSList *pixbufexts = NULL;  /* FIXME: we leak this.  */

static gboolean
filter_images (const GtkFileFilterInfo *filter_info, gpointer data)
{
	if (filter_info->mime_type)
		return strncmp (filter_info->mime_type, "image/", 6) == 0;

	if (filter_info->display_name) {
		GSList *l;
		char const *ext = strrchr (filter_info->display_name, '.');
		if (!ext) return FALSE;
		ext++;

		if (!have_pixbufexts) {
			GSList *pixbuf_fmts = gdk_pixbuf_get_formats ();

			for (l = pixbuf_fmts; l != NULL; l = l->next) {
				GdkPixbufFormat *fmt = l->data;
				gchar **support_exts = gdk_pixbuf_format_get_extensions (fmt);
				int i;

				for (i = 0; support_exts[i]; ++i)
					pixbufexts = g_slist_prepend (pixbufexts,
								      support_exts[i]);
				/*
				 * Use g_free here because the strings have been
				 * taken by pixbufexts.
				 */
				g_free (support_exts);
			}

			g_slist_free (pixbuf_fmts);
			have_pixbufexts = TRUE;
		}

		for (l = pixbufexts; l != NULL; l = l->next)
			if (g_ascii_strcasecmp (l->data, ext) == 0)
				return TRUE;
	}

	return FALSE;
}

static void
update_preview_cb (GtkFileChooser *chooser)
{
	gchar *filename = gtk_file_chooser_get_preview_filename (chooser);
	GtkWidget *label = g_object_get_data (G_OBJECT (chooser), "label-widget");
	GtkWidget *image = g_object_get_data (G_OBJECT (chooser), "image-widget");

	if (filename == NULL) {
		gtk_widget_hide (image);
		gtk_widget_hide (label);
	} else if (g_file_test (filename, G_FILE_TEST_IS_DIR)) {
		/* Not quite sure what to do here.  */
		gtk_widget_hide (image);
		gtk_widget_hide (label);
	} else {
		GOImage *buf;
		gboolean dummy;

		buf = go_image_new_from_file (filename, NULL);
		if (buf) {
			dummy = FALSE;
		} else {
			GdkScreen *screen = gtk_widget_get_screen (GTK_WIDGET (chooser));
			buf = go_pixbuf_new_from_pixbuf (gtk_icon_theme_load_icon
						(gtk_icon_theme_get_for_screen (screen),
						 "unknown_image", 100, 0, NULL));
			dummy = buf != NULL;
		}

		if (buf) {
			GdkPixbuf *pixbuf = go_image_get_scaled_pixbuf (buf, PREVIEW_HSIZE, PREVIEW_VSIZE);
			gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
			g_object_unref (pixbuf);
			gtk_widget_show (image);

			if (dummy)
				gtk_label_set_text (GTK_LABEL (label), "");
			else {
				int w = go_image_get_width (buf);
				int h = go_image_get_height (buf);
				char *size = g_strdup_printf (_("%d x %d"), w, h);
				gtk_label_set_text (GTK_LABEL (label), size);
				g_free (size);
			}
			gtk_widget_show (label);

			g_object_unref (buf);
		}

		g_free (filename);
	}
}

static GtkFileChooser *
gui_image_chooser_new (gboolean is_save)
{
	GtkFileChooser *fsel;

	fsel = GTK_FILE_CHOOSER
		(g_object_new (GTK_TYPE_FILE_CHOOSER_DIALOG,
			       "action", is_save ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN,
			       "local-only", FALSE,
			       "use-preview-label", FALSE,
			       NULL));
	gtk_dialog_add_buttons (GTK_DIALOG (fsel),
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				is_save ? GTK_STOCK_SAVE : GTK_STOCK_OPEN,
				GTK_RESPONSE_OK,
				NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (fsel), GTK_RESPONSE_OK);
	/* Filters */
	{
		GtkFileFilter *filter;

		filter = gtk_file_filter_new ();
		gtk_file_filter_set_name (filter, _("All Files"));
		gtk_file_filter_add_pattern (filter, "*");
		gtk_file_chooser_add_filter (fsel, filter);

		filter = gtk_file_filter_new ();
		gtk_file_filter_set_name (filter, _("Images"));
		gtk_file_filter_add_custom (filter, GTK_FILE_FILTER_MIME_TYPE,
					    filter_images, NULL, NULL);
		gtk_file_chooser_add_filter (fsel, filter);
		/* Make this filter the default */
		gtk_file_chooser_set_filter (fsel, filter);
	}

	/* Preview */
	{
		GtkWidget *vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
		GtkWidget *preview_image = gtk_image_new ();
		GtkWidget *preview_label = gtk_label_new ("");

		g_object_set_data (G_OBJECT (fsel), "image-widget", preview_image);
		g_object_set_data (G_OBJECT (fsel), "label-widget", preview_label);

		gtk_widget_set_size_request (vbox, PREVIEW_HSIZE, -1);

		gtk_box_pack_start (GTK_BOX (vbox), preview_image, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), preview_label, FALSE, FALSE, 0);
		gtk_file_chooser_set_preview_widget (fsel, vbox);
		g_signal_connect (fsel, "update-preview",
				  G_CALLBACK (update_preview_cb), NULL);
		update_preview_cb (fsel);
	}
	return fsel;
}

char *
go_gtk_select_image (GtkWindow *toplevel, char const *initial)
{
	return go_gtk_select_image_with_extra_widget (toplevel, initial, NULL);
}

char *
go_gtk_select_image_with_extra_widget (GtkWindow *toplevel, char const *initial, GtkWidget *extra)
{
	char const *key = "go_gtk_select_image";
	char *uri = NULL;
	GtkFileChooser *fsel;

	g_return_val_if_fail (GTK_IS_WINDOW (toplevel), NULL);

	fsel = gui_image_chooser_new (FALSE);
	if (GTK_IS_WIDGET (extra))
		gtk_file_chooser_set_extra_widget (fsel, extra);

	if (!initial)
		initial = g_object_get_data (G_OBJECT (toplevel), key);
	if (initial)
		gtk_file_chooser_set_uri (fsel, initial);
	g_object_set (G_OBJECT (fsel), "title", _("Select an Image"), NULL);

	/* Show file selector */
	if (go_gtk_file_sel_dialog (toplevel, GTK_WIDGET (fsel))) {
		uri = gtk_file_chooser_get_uri (fsel);
		g_object_set_data_full (G_OBJECT (toplevel),
					key, g_strdup (uri),
					g_free);
	}
	gtk_widget_destroy (GTK_WIDGET (fsel));
	return uri;
}

typedef struct {
	char *uri;
	double resolution;
	gboolean is_expanded;
	GOImageFormat format;

	// Not saved, but used by handlers
	gboolean by_ext;
	GSList *supported_formats;
	GtkWidget *resolution_widget;
} SaveInfoState;

static void
save_info_state_free (SaveInfoState *state)
{
	g_free (state->uri);
	g_free (state);
}

static void
cb_format_combo_changed (GtkComboBox *combo, SaveInfoState *state)
{
	gboolean sensitive;
	int i;

	i = gtk_combo_box_get_active (combo);
	if (state->by_ext && i == 0)
		sensitive = TRUE;
	else {
		GOImageFormat fmt = GPOINTER_TO_UINT
			(g_slist_nth_data (state->supported_formats,
					   i - state->by_ext));
		GOImageFormatInfo const *format_info =
			go_image_get_format_info (fmt);
		sensitive = format_info && format_info->is_dpi_useful;
	}
	gtk_widget_set_sensitive (state->resolution_widget, sensitive);
}

/**
 * go_gui_get_image_save_info:
 * @toplevel: a #GtkWindow
 * @supported_formats: (element-type void): a #GSList of supported file formats
 * @ret_format: default file format
 * @resolution: export resolution
 *
 * Opens a file chooser and lets user choose file URI and format in a list of
 * supported ones.
 *
 * Returns: file URI string, file #GOImageFormat stored in @ret_format, and
 * 	export resolution in @resolution.
 **/
char *
go_gui_get_image_save_info (GtkWindow *toplevel, GSList *supported_formats,
			    GOImageFormat *ret_format, double *resolution)
{
	GOImageFormat format;
	GOImageFormatInfo const *format_info;
	GtkComboBox *format_combo = NULL;
	GtkFileChooser *fsel = gui_image_chooser_new (TRUE);
	GtkWidget *expander = NULL;
	GtkWidget *resolution_spin = NULL;
	GtkWidget *resolution_grid;
	GtkBuilder *gui;
	SaveInfoState *state;
	char const *key = "go_gui_get_image_save_info";
	char *uri = NULL;

	state = g_object_get_data (G_OBJECT (toplevel), key);
	if (state == NULL) {
		state = g_new (SaveInfoState, 1);
		g_return_val_if_fail (state != NULL, NULL);
		state->uri = NULL;
		state->resolution = 150.0;
		state->is_expanded = FALSE;
		state->format = GO_IMAGE_FORMAT_SVG;
		g_object_set_data_full (G_OBJECT (toplevel), key,
					state, (GDestroyNotify) save_info_state_free);
	}
	state->supported_formats = supported_formats;
	state->by_ext = FALSE;

	g_object_set (G_OBJECT (fsel), "title", _("Save as"), NULL);

	gui = go_gtk_builder_load_internal ("res:go:gtk/go-image-save-dialog-extra.ui",
					   GETTEXT_PACKAGE, NULL);
	if (gui != NULL) {
		GtkWidget *widget;

		state->resolution_widget = resolution_grid = go_gtk_builder_get_widget (gui, "resolution-grid");

		/* Format selection UI */
		if (supported_formats != NULL && ret_format != NULL) {
			int i;
			GSList *l;
			format_combo = go_gtk_builder_combo_box_init_text (gui, "format_combo");

			state->by_ext = TRUE;
			go_gtk_combo_box_append_text (format_combo, _("Auto by extension"));

			for (l = supported_formats, i = 0; l != NULL; l = l->next, i++) {
				format = GPOINTER_TO_UINT (l->data);
				format_info = go_image_get_format_info (format);
				if (format_info == NULL)
					continue;
				go_gtk_combo_box_append_text (format_combo, _(format_info->desc));
				if (format == state->format)
					gtk_combo_box_set_active (format_combo, i);
			}
			if (gtk_combo_box_get_active (format_combo) < 0)
				gtk_combo_box_set_active (format_combo, 0);
		} else {
			widget = go_gtk_builder_get_widget (gui, "file_type_box");
			gtk_widget_hide (widget);
		}

		if (supported_formats != NULL && ret_format != NULL) {
			widget = go_gtk_builder_get_widget (gui, "image_save_dialog_extra");
			gtk_file_chooser_set_extra_widget (fsel, widget);
		}

		/* Export setting expander */
		expander = go_gtk_builder_get_widget (gui, "export_expander");
		if (resolution != NULL) {
			gtk_expander_set_expanded (GTK_EXPANDER (expander), state->is_expanded);
			resolution_spin = go_gtk_builder_get_widget (gui, "resolution_spin");
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (resolution_spin), state->resolution);
		} else
			gtk_widget_hide (expander);

		if (resolution != NULL && supported_formats != NULL && ret_format != NULL) {
			cb_format_combo_changed (format_combo, state);
			g_signal_connect (GTK_WIDGET (format_combo), "changed",
					  G_CALLBACK (cb_format_combo_changed),
					  state);
		}

		g_object_unref (gui);
	}

	if (state->uri != NULL) {
		gtk_file_chooser_set_uri (fsel, state->uri);
		gtk_file_chooser_unselect_all (fsel);
	}

	/* Show file selector */
 loop:
	if (!go_gtk_file_sel_dialog (toplevel, GTK_WIDGET (fsel)))
		goto out;
	uri = gtk_file_chooser_get_uri (fsel);
	if (format_combo) {
		char *new_uri = NULL;
		int index = gtk_combo_box_get_active (format_combo);

		format = GO_IMAGE_FORMAT_UNKNOWN;
		if (index < 0)
			; // That's it.
		else if (state->by_ext && index == 0) {
			GSList *l;

			for (l = supported_formats; l; l = l->next) {
				GOImageFormat f = GPOINTER_TO_UINT (l->data);
				GOImageFormatInfo const *format_info = go_image_get_format_info (f);
				if (go_url_check_extension (uri, format_info->ext, NULL))
					format = f;
			}
			if (format == GO_IMAGE_FORMAT_UNKNOWN)
				goto loop;
		} else {
			format = GPOINTER_TO_UINT (g_slist_nth_data
				(supported_formats, index - state->by_ext));
			format_info = go_image_get_format_info (format);
			if (!go_url_check_extension (uri, format_info->ext, &new_uri) &&
			    !go_gtk_query_yes_no (GTK_WINDOW (fsel), TRUE,
			     _("The given file extension does not match the"
			       " chosen file type. Do you want to use this name"
			       " anyway?"))) {
				g_free (new_uri);
				g_free (uri);
				uri = NULL;
				goto loop;
			}
			g_free (uri);
			uri = new_uri;
		}
		*ret_format = format;
	}
	if (!go_gtk_url_is_writeable (GTK_WINDOW (fsel), uri, TRUE)) {
		g_free (uri);
		uri = NULL;
		goto loop;
	}
 out:
	if (uri != NULL && ret_format != NULL) {
		g_free (state->uri);
		state->uri = g_strdup (uri);
		state->format = *ret_format;
		if (resolution != NULL) {
			state->is_expanded = gtk_expander_get_expanded (GTK_EXPANDER (expander));
			*resolution =  gtk_spin_button_get_value (GTK_SPIN_BUTTON (resolution_spin));
			state->resolution = *resolution;
		}
	}

	gtk_widget_destroy (GTK_WIDGET (fsel));

	return uri;
}

static void
add_atk_relation (GtkWidget *w0, GtkWidget *w1, AtkRelationType type)
{
	AtkObject *atk0 = gtk_widget_get_accessible(w0);
	AtkObject *atk1 = gtk_widget_get_accessible(w1);
	AtkRelationSet *relation_set = atk_object_ref_relation_set (atk0);
	AtkRelation *relation = atk_relation_new (&atk1, 1, type);
	atk_relation_set_add (relation_set, relation);
	g_object_unref (relation_set);
	g_object_unref (relation);
}

/**
 * go_atk_setup_label:
 * @label: #GtkWidget
 * @target: #GtkWidget
 *
 * A convenience routine to setup label-for/labeled-by relationship between a
 * pair of widgets
 **/
void
go_atk_setup_label (GtkWidget *label, GtkWidget *target)
{
	 add_atk_relation (label, target, ATK_RELATION_LABEL_FOR);
	 add_atk_relation (target, label, ATK_RELATION_LABELLED_BY);
}

typedef struct {
	char const *data_dir;
	char const *app;
	char const *link;
} CBHelpPaths;

#ifdef G_OS_WIN32
#include <windows.h>

typedef HWND (* WINAPI HtmlHelpA_t) (HWND hwndCaller,
				     LPCSTR pszFile,
				     UINT uCommand,
				     DWORD_PTR dwData);
#define HH_HELP_CONTEXT 0x000F

static HtmlHelpA_t
html_help (void)
{
	HMODULE hhctrl;
	static HtmlHelpA_t result = NULL;
	static gboolean beenhere = FALSE;

	if (!beenhere) {
		hhctrl = LoadLibrary ("hhctrl.ocx");
		if (hhctrl != NULL)
			result = (HtmlHelpA_t) GetProcAddress (hhctrl, "HtmlHelpA");
		beenhere = TRUE;
	}

	return result;
}
#endif

static void
go_help_display (CBHelpPaths const *paths, GdkScreen *screen)
{
#if defined(G_OS_WIN32)
	static GHashTable* context_help_map = NULL;
	guint id;

	if (!context_help_map) {
		GsfInput *input;
		GsfInputTextline *textline;
		GError *err = NULL;
		gchar *mapfile = g_strconcat (paths->app, ".hhmap", NULL);
		gchar *path = g_build_filename (paths->data_dir, "doc", "C", mapfile, NULL);

		g_free (mapfile);

		if (!(input = gsf_input_stdio_new (path, &err)) ||
		    !(textline = (GsfInputTextline *) gsf_input_textline_new (input)))
			go_gtk_notice_dialog (NULL, GTK_MESSAGE_ERROR, "Cannot open '%s': %s",
					      path, err ? err->message :
							  "failed to create gsf-input-textline");
		else {
			gchar *line, *topic;
			gulong id, i = 1;
			context_help_map = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

			while ((line = gsf_input_textline_utf8_gets (textline)) != NULL) {
				if (!(id = strtoul (line, &topic, 10))) {
					go_gtk_notice_dialog (NULL, GTK_MESSAGE_ERROR,
							      "Invalid topic id at line %lu in %s: '%s'",
							      i, path, line);
					continue;
				}
				for (; *topic == ' ' || *topic == '\t'; ++topic);
				g_hash_table_insert (context_help_map, g_strdup (topic),
					(gpointer)id);
			}
			g_object_unref (textline);
		}
		if (input)
			g_object_unref (input);
		g_free (path);
	}

	if (!(id = (guint) g_hash_table_lookup (context_help_map, paths->link)))
		go_gtk_notice_dialog (NULL, GTK_MESSAGE_ERROR, "Topic '%s' not found.",
				      paths->link);
	else {
		gchar *chmfile = g_strconcat (paths->app, ".chm", NULL);
		gchar *path = g_build_filename (paths->data_dir, "doc", "C", chmfile, NULL);

		g_free (chmfile);
		if (html_help () == NULL ||
		    !(html_help ()) (GetDesktopWindow (), path, HH_HELP_CONTEXT, id))
			go_gtk_notice_dialog (NULL, GTK_MESSAGE_ERROR, "Failed to load HtmlHelp");
		g_free (path);
	}
#else
	char *uri = g_strconcat ("ghelp:", paths->app, "#", paths->link, NULL);
	go_gtk_url_show (uri, screen);
	g_free (uri);
	return;
#endif
}

static void
cb_help (GtkWidget *w, CBHelpPaths const *paths)
{
	go_help_display (paths, gtk_widget_get_screen (w));
}

void
go_gtk_help_button_init (GtkWidget *w, char const *data_dir, char const *app, char const *link)
{
	CBHelpPaths *paths = g_new (CBHelpPaths, 1);
	GtkWidget *parent = gtk_widget_get_parent (w);

	if (GTK_IS_BUTTON_BOX (parent))
		gtk_button_box_set_child_secondary (
			GTK_BUTTON_BOX (parent), w, TRUE);

	paths->data_dir = data_dir;
	paths->app	= app;
	paths->link	= link;
	g_signal_connect_data (G_OBJECT (w),
			       "clicked", G_CALLBACK (cb_help),
			       (gpointer)paths, (GClosureNotify)g_free,
			       0);
}

/**
 * go_gtk_url_is_writeable:
 * @parent: #GtkWindow
 * @uri: the uri to test.
 * @overwrite_by_default: gboolean
 *
 * Check if it makes sense to try saving.
 * If it's an existing file and writable for us, ask if we want to overwrite.
 * We check for other problems, but if we miss any, the saver will report.
 * So it doesn't have to be bulletproof.
 *
 * FIXME: The message boxes should really be children of the file selector,
 * not the workbook.
 *
 * Returns: %TRUE if @url is writable
 **/
gboolean
go_gtk_url_is_writeable (GtkWindow *parent, char const *uri,
			 gboolean overwrite_by_default)
{
	gboolean result = TRUE;
	char *filename;

	if (uri == NULL || uri[0] == '\0')
		result = FALSE;

	filename = go_filename_from_uri (uri);
	if (!filename)
		return TRUE;  /* Just assume writable.  */

	if (G_IS_DIR_SEPARATOR (filename [strlen (filename) - 1]) ||
	    g_file_test (filename, G_FILE_TEST_IS_DIR)) {
		go_gtk_notice_dialog (parent, GTK_MESSAGE_ERROR,
				      _("%s\nis a directory name"), uri);
		result = FALSE;
	} else if (go_file_access (uri, GO_W_OK) != 0 && errno != ENOENT) {
		go_gtk_notice_dialog (parent, GTK_MESSAGE_ERROR,
				      _("You do not have permission to save to\n%s"),
				      uri);
		result = FALSE;
	} else if (g_file_test (filename, G_FILE_TEST_EXISTS)) {
		char *dirname = go_dirname_from_uri (uri, TRUE);
		char *basename = go_basename_from_uri (uri);
		GtkWidget *dialog = gtk_message_dialog_new_with_markup (parent,
			 GTK_DIALOG_DESTROY_WITH_PARENT,
			 GTK_MESSAGE_WARNING,
			 GTK_BUTTONS_OK_CANCEL,
			 _("A file called <i>%s</i> already exists in %s.\n\n"
			   "Do you want to save over it?"),
			 basename, dirname);
		gtk_dialog_set_default_response (GTK_DIALOG (dialog),
			overwrite_by_default ? GTK_RESPONSE_OK : GTK_RESPONSE_CANCEL);
		result = GTK_RESPONSE_OK ==
			go_gtk_dialog_run (GTK_DIALOG (dialog), parent);
		g_free (dirname);
		g_free (basename);
	}

	g_free (filename);
	return result;
}

/**
 * go_gtk_dialog_run:
 * @dialog: #GtkDialog
 * @parent: #GtkWindow
 *
 * Pop up a dialog as child of a window.
 *
 * Returns: result ID.
 **/
gint
go_gtk_dialog_run (GtkDialog *dialog, GtkWindow *parent)
{
	gint      result;

	g_return_val_if_fail (GTK_IS_DIALOG (dialog), GTK_RESPONSE_NONE);

	if (parent) {
		g_return_val_if_fail (GTK_IS_WINDOW (parent), GTK_RESPONSE_NONE);

		go_gtk_window_set_transient (parent, GTK_WINDOW (dialog));
	}

	g_object_ref (dialog);

	while ((result = gtk_dialog_run (dialog)) >= 0)
	       ;
	gtk_widget_destroy (GTK_WIDGET (dialog));

	g_object_unref (dialog);

	return result;
}

#define _VPRINTF_MESSAGE(format,args,msg) va_start (args, format); \
					  msg = g_strdup_vprintf (format, args); \
					  va_end (args);
#define VPRINTF_MESSAGE(format,args,msg) _VPRINTF_MESSAGE (format, args, msg); \
					 g_return_if_fail (msg != NULL);

/*
 * TODO:
 * Get rid of trailing newlines /whitespace.
 */
void
go_gtk_notice_dialog (GtkWindow *parent, GtkMessageType type,
		      char const *format, ...)
{
	va_list args;
	gchar *msg;
	GtkWidget *dialog;

	VPRINTF_MESSAGE (format, args, msg);
	dialog = gtk_message_dialog_new_with_markup (parent,
		GTK_DIALOG_DESTROY_WITH_PARENT, type, GTK_BUTTONS_OK, "%s", msg);
	g_free (msg);
	go_gtk_dialog_run (GTK_DIALOG (dialog), parent);
}

void
go_gtk_notice_nonmodal_dialog (GtkWindow *parent, GtkWidget **ref,
			       GtkMessageType type, char const *format, ...)
{
	va_list args;
	gchar *msg;
	GtkWidget *dialog;

	if (*ref != NULL)
		gtk_widget_destroy (*ref);

	VPRINTF_MESSAGE (format, args, msg);
	*ref = dialog = gtk_message_dialog_new (parent, GTK_DIALOG_DESTROY_WITH_PARENT, type,
					 GTK_BUTTONS_OK, "%s", msg);
	g_free (msg);

	g_signal_connect_object (G_OBJECT (dialog),
		"response",
		G_CALLBACK (gtk_widget_destroy), G_OBJECT (dialog), 0);
	g_signal_connect (G_OBJECT (dialog),
		"destroy",
		G_CALLBACK (gtk_widget_destroyed), ref);

	gtk_widget_show (dialog);
}

gboolean
go_gtk_query_yes_no (GtkWindow *parent, gboolean default_answer,
		     char const *format, ...)
{
	va_list args;
	gchar *msg;
	GtkWidget *dialog;

	_VPRINTF_MESSAGE (format, args, msg);
	g_return_val_if_fail (msg != NULL, default_answer);
	dialog = gtk_message_dialog_new (parent,
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_QUESTION,
		GTK_BUTTONS_YES_NO,
		"%s", msg);
        g_free (msg);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog),
		default_answer ? GTK_RESPONSE_YES : GTK_RESPONSE_NO);
	return GTK_RESPONSE_YES ==
		go_gtk_dialog_run (GTK_DIALOG (dialog), parent);
}

/**
 * go_dialog_guess_alternative_button_order:
 * @dialog: #GtkDialog
 *
 * This function inspects the buttons in the dialog and comes up
 * with a reasonable alternative dialog order.
 **/
void
go_dialog_guess_alternative_button_order (GtkDialog *dialog)
{
	GList *children, *tmp;
	int i, nchildren;
	int *new_order;
	int i_yes = -1, i_no = -1, i_ok = -1, i_cancel = -1, i_apply = -1;
	gboolean again;
	gboolean any = FALSE;
	int loops = 0;

	children = gtk_container_get_children (GTK_CONTAINER (gtk_dialog_get_action_area (dialog)));
	if (!children)
		return;

	nchildren = g_list_length (children);
	new_order = g_new (int, nchildren);

	for (tmp = children, i = 0; tmp; tmp = tmp->next, i++) {
		GtkWidget *child = tmp->data;
		int res = gtk_dialog_get_response_for_widget (dialog, child);
		new_order[i] = res;
		switch (res) {
		case GTK_RESPONSE_YES: i_yes = i; break;
		case GTK_RESPONSE_NO: i_no = i; break;
		case GTK_RESPONSE_OK: i_ok = i; break;
		case GTK_RESPONSE_CANCEL: i_cancel = i; break;
		case GTK_RESPONSE_APPLY: i_apply = i; break;
		}
	}
	g_list_free (children);

#define MAYBE_SWAP(ifirst,ilast)				\
	if (ifirst >= 0 && ilast >= 0 && ifirst > ilast) {	\
		int tmp;					\
								\
		tmp = new_order[ifirst];			\
		new_order[ifirst] = new_order[ilast];		\
		new_order[ilast] = tmp;				\
								\
		tmp = ifirst;					\
		ifirst = ilast;					\
		ilast = tmp;					\
								\
		again = TRUE;					\
		any = TRUE;					\
	}

	do {
		again = FALSE;
		MAYBE_SWAP (i_yes, i_no);
		MAYBE_SWAP (i_ok, i_cancel);
		MAYBE_SWAP (i_cancel, i_apply);
		MAYBE_SWAP (i_no, i_cancel);
	} while (again && ++loops < 2);

#undef MAYBE_SWAP

	if (any)
		gtk_dialog_set_alternative_button_order_from_array (dialog,
								    nchildren,
								    new_order);
	g_free (new_order);
}

/**
 * go_menu_position_below:
 * @menu: a #GtkMenu
 * @x: non-NULL storage for the X coordinate of the menu
 * @y: non-NULL storage for the Y coordinate of the menu
 * @push_in: non-NULL storage for the push-in distance
 * @user_data: arbitrary
 *
 * Implementation of a GtkMenuPositionFunc that positions
 * the child window under the parent one, for use with gtk_menu_popup.
 **/
void
go_menu_position_below (GtkMenu  *menu,
			gint     *x,
			gint     *y,
			gint     *push_in,
			gpointer  user_data)
{
	GtkWidget *widget = GTK_WIDGET (user_data);
	gint sx, sy;
	GtkRequisition req;
	GdkRectangle monitor;
	GdkWindow *window = gtk_widget_get_window (widget);
	GtkAllocation size;

	gtk_widget_get_allocation (widget, &size);

	if (window)
		gdk_window_get_origin (window, &sx, &sy);
	else
		sx = sy = 0;

	if (!gtk_widget_get_has_window (widget))
	{
		sx += size.x;
		sy += size.y;
	}

	gtk_widget_get_preferred_size (GTK_WIDGET (menu), &req, NULL);

	if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_LTR)
		*x = sx;
	else
		*x = sx + size.width - req.width;
	*y = sy;

#ifdef HAVE_GDK_DISPLAY_GET_MONITOR_AT_WINDOW
	gdk_monitor_get_geometry
		(gdk_display_get_monitor_at_window
		 (gtk_widget_get_display (widget), window),
		 &monitor);
#else
	{
		GdkScreen *screen = gtk_widget_get_screen (widget);
		gint monitor_num = gdk_screen_get_monitor_at_window (screen, window);
		gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);
	}
#endif

	if (*x < monitor.x)
		*x = monitor.x;
	else if (*x + req.width > monitor.x + monitor.width)
		*x = monitor.x + monitor.width - req.width;

	if (monitor.y + monitor.height - *y - size.height >= req.height)
		*y +=size.height;
	else if (*y - monitor.y >= req.height)
		*y -= req.height;
	else if (monitor.y + monitor.height - *y - size.height > *y - monitor.y)
		*y += size.height;
	else
		*y -= req.height;

	*push_in = FALSE;
}

/**
 * go_gtk_url_show:
 * @url: the url to show
 * @screen: screen to show the uri on or %NULL for the default screen
 *
 * This function is a simple convenience wrapper for #gtk_show_uri.
 *
 * Returns: %NULL on success, or a newly allocated #GError if something
 * went wrong.
 **/
GError *
go_gtk_url_show (gchar const *url, GdkScreen *screen)
{
	GError *error = NULL;
	gtk_show_uri (screen, url, GDK_CURRENT_TIME, &error);
	return error;
}

/**
 * go_gtk_widget_render_icon_pixbuf:
 * @widget: a mapped widget determining the screen targeted
 * @icon_name: the name of the icon to render
 * @size: the symbolic size desired.
 *
 * This function works as gtk_widget_render_icon_pixbuf except that it takes
 * an icon name, not a stock id.
 *
 * Returns: (transfer full): A #GdkPixbuf.
 **/
GdkPixbuf *
go_gtk_widget_render_icon_pixbuf (GtkWidget   *widget,
				  const gchar *icon_name,
				  GtkIconSize  size)
{
	GdkScreen *screen;
	GtkIconTheme *theme;
	int pixels;
	GdkPixbuf *res;

	/* The widget really ought to be mapped.  */
	screen = gtk_widget_has_screen (widget)
		? gtk_widget_get_screen (widget)
		: gdk_screen_get_default ();
	theme = gtk_icon_theme_get_for_screen (screen);

	switch (size) {
	default:
	case GTK_ICON_SIZE_MENU:
	case GTK_ICON_SIZE_SMALL_TOOLBAR:
	case GTK_ICON_SIZE_BUTTON:
		pixels = 16;
		break;
	case GTK_ICON_SIZE_LARGE_TOOLBAR:
	case GTK_ICON_SIZE_DND:
		pixels = 24;
		break;
	case GTK_ICON_SIZE_DIALOG:
		pixels = 48;
		break;
	}

	res = gtk_icon_theme_load_icon (theme, icon_name, pixels, 0, NULL);

	if (res && go_debug_flag ("leaks"))
		go_debug_check_finalized (res, icon_name);

	return res;
}

static GtkCssProvider *css_provider;
static GSList *css_provider_screens;

static void
cb_screen_changed (GtkWidget *widget)
{
	GdkScreen *screen = gtk_widget_get_screen (widget);

	if (!screen || g_slist_find (css_provider_screens, screen))
		return;

	css_provider_screens = g_slist_prepend (css_provider_screens, screen);
	gtk_style_context_add_provider_for_screen
		(screen,
		 GTK_STYLE_PROVIDER (css_provider),
		 GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}


void
_go_gtk_widget_add_css_provider (GtkWidget *w)
{
	g_return_if_fail (GTK_IS_WIDGET (w));

	if (!css_provider) {
		const char *data = _go_gtk_new_theming ()
			? go_rsm_lookup ("go:gtk/goffice.css", NULL)
			: go_rsm_lookup ("go:gtk/goffice-old.css", NULL);
		css_provider = gtk_css_provider_new ();
		gtk_css_provider_load_from_data (css_provider, data, -1, NULL);
	}

	g_signal_connect (w, "screen-changed",
			  G_CALLBACK (cb_screen_changed), NULL);
	cb_screen_changed (w);
}

gboolean
_go_gtk_new_theming (void)
{
#if GTK_CHECK_VERSION(3,20,0)
	return TRUE;
#else
	static int new_theming = -1;
	if (new_theming == -1)
		new_theming = (gtk_get_major_version() > 3 ||
			       gtk_get_minor_version() >= 20);

	return new_theming;
#endif
}

// ---------------------------------------------------------------------------
// Foreign drawing style code copied from foreigndrawing.c

#if GTK_CHECK_VERSION(3,20,0)
static void
append_element (GtkWidgetPath *path,
                const char    *selector)
{
	static const struct {
		const char    *name;
		GtkStateFlags  state_flag;
	} pseudo_classes[] = {
		{ "active",        GTK_STATE_FLAG_ACTIVE },
		{ "hover",         GTK_STATE_FLAG_PRELIGHT },
		{ "selected",      GTK_STATE_FLAG_SELECTED },
		{ "disabled",      GTK_STATE_FLAG_INSENSITIVE },
		{ "indeterminate", GTK_STATE_FLAG_INCONSISTENT },
		{ "focus",         GTK_STATE_FLAG_FOCUSED },
		{ "backdrop",      GTK_STATE_FLAG_BACKDROP },
		{ "dir(ltr)",      GTK_STATE_FLAG_DIR_LTR },
		{ "dir(rtl)",      GTK_STATE_FLAG_DIR_RTL },
		{ "link",          GTK_STATE_FLAG_LINK },
		{ "visited",       GTK_STATE_FLAG_VISITED },
		{ "checked",       GTK_STATE_FLAG_CHECKED },
		{ "drop(active)",  GTK_STATE_FLAG_DROP_ACTIVE }
	};
	const char *next;
	char *name;
	char type;
	guint i;

	next = strpbrk (selector, "#.:");
	if (next == NULL)
		next = selector + strlen (selector);

	name = g_strndup (selector, next - selector);
	if (g_ascii_isupper (selector[0]))
	{
		GType gtype;
		gtype = g_type_from_name (name);
		if (gtype == G_TYPE_INVALID)
		{
			g_critical ("Unknown type name `%s'", name);
			g_free (name);
			return;
		}
		gtk_widget_path_append_type (path, gtype);
	}
	else
	{
		/* Omit type, we're using name */
		gtk_widget_path_append_type (path, G_TYPE_NONE);
		gtk_widget_path_iter_set_object_name (path, -1, name);
	}
	g_free (name);

	while (*next != '\0')
	{
		type = *next;
		selector = next + 1;
		next = strpbrk (selector, "#.:");
		if (next == NULL)
			next = selector + strlen (selector);
		name = g_strndup (selector, next - selector);

		switch (type)
		{
		case '#':
			gtk_widget_path_iter_set_name (path, -1, name);
			break;

		case '.':
			gtk_widget_path_iter_add_class (path, -1, name);
			break;

		case ':':
			for (i = 0; i < G_N_ELEMENTS (pseudo_classes); i++)
			{
				if (g_str_equal (pseudo_classes[i].name, name))
				{
					gtk_widget_path_iter_set_state (path,
									-1,
									gtk_widget_path_iter_get_state (path, -1)
									| pseudo_classes[i].state_flag);
					break;
				}
			}
			if (i == G_N_ELEMENTS (pseudo_classes))
				g_critical ("Unknown pseudo-class :%s", name);
			break;

		default:
			g_assert_not_reached ();
			break;
		}

		g_free (name);
	}
}

static GtkStyleContext *
create_context_for_path (GtkWidgetPath   *path,
                         GtkStyleContext *parent)
{
	GtkStyleContext *context;

	context = gtk_style_context_new ();
	gtk_style_context_set_path (context, path);
	gtk_style_context_set_parent (context, parent);
	/* Unfortunately, we have to explicitly set the state again here
	 * for it to take effect
	 */
	gtk_style_context_set_state (context, gtk_widget_path_iter_get_state (path, -1));
	gtk_widget_path_unref (path);

	return context;
}
#endif

/**
 * go_style_context_from_selector:
 * @parent: (allow-none): style context for container
 * @selector: a css selector
 *
 * Returns: (transfer full): a new style context.
 */
GtkStyleContext *
go_style_context_from_selector (GtkStyleContext *parent,
				const char      *selector)
{
#if GTK_CHECK_VERSION(3,20,0)
	GtkWidgetPath *path;

	g_return_val_if_fail (selector != NULL, NULL);

	path = parent
		? gtk_widget_path_copy (gtk_style_context_get_path (parent))
		: gtk_widget_path_new ();

	append_element (path, selector);

	return create_context_for_path (path, parent);
#else
	(void)parent;
	g_return_val_if_fail (selector != NULL, NULL);
	g_assert_not_reached ();
	return NULL;
#endif
}

// ---------------------------------------------------------------------------

void
_go_gtk_shutdown (void)
{
	g_clear_object (&css_provider);
	g_slist_free (css_provider_screens);
	css_provider_screens = NULL;
}
