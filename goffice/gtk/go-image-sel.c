/*
 * go-image-sel.c
 *
 * Copyright (C) 2008 Jean Brefort (jean.brefort@normalesup.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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

#include "goffice-config.h"
#include "go-image-sel.h"
#include <goffice/app/go-doc.h>
#include <goffice/gtk/goffice-gtk.h>
#include <goffice/utils/go-file.h>
#include <glib/gi18n-lib.h>
#include <string.h>

typedef struct _GOImageSelState		GOImageSelState;

struct _GOImageSelState {
	GtkWidget *dialog;
	GOCmdContext *cc;
	GODoc *doc;
	GOImage **result;

	/* GUI accessors */
	GtkBuilder    *gui;
	GtkIconView *icon_view;
	GtkListStore *model;
	GtkWidget *ok_button;
	char *uri;
	char *name;
};

static void
cb_entry_focus_out (GtkEntry *entry, G_GNUC_UNUSED GdkEvent *event, GOImageSelState *state)
{
	char const *new_name = gtk_entry_get_text (entry);
	if (new_name && *new_name && !go_doc_get_image (state->doc, new_name)) {
		g_free (state->name);
		state->name = g_strdup (new_name);
	}
}

static void
cb_file_image_select (GtkWidget *cc, GOImageSelState *state)
{
	GtkWidget *box, *w;
	char *new_name, *filename;
	unsigned n = 1;
	GError *error = NULL;
	GOImage *image, *real_image;

	/* FIXME: use a GtkGrid there */
	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
	w = gtk_label_new (_("New image name"));
	gtk_container_add (GTK_CONTAINER (box), w);
	w = gtk_entry_new ();
	while (1) {
		new_name = g_strdup_printf (_("image%u"), n++);
		if (!go_doc_get_image (state->doc, new_name))
			break;
		g_free (new_name);
	}
	gtk_entry_set_text (GTK_ENTRY (w), new_name);
	g_signal_connect (G_OBJECT (w), "focus-out-event", G_CALLBACK (cb_entry_focus_out), state);
	state->name = new_name;
	gtk_container_add (GTK_CONTAINER (box), w);
	gtk_widget_show_all (box);

	g_free (state->uri);

	state->uri = go_gtk_select_image_with_extra_widget (GTK_WINDOW (gtk_widget_get_toplevel (cc)),
				   NULL, box);
	if (state->uri) {
		filename = go_filename_from_uri (state->uri);
		image = go_image_new_from_file (filename, &error);
		g_free (filename);
		if (error) {
			g_warning ("%s", error->message);
			g_error_free (error);
			error = NULL;
		}
		if (image) {
			real_image = go_doc_add_image (state->doc, state->name, image);
			if (real_image == image) {
				/* add the new image to the list */
				GtkTreeIter iter;
				GtkTreePath *path;
				gtk_list_store_append (state->model, &iter);
				gtk_list_store_set (state->model, &iter,
						    0, go_image_get_thumbnail (real_image),
						    1, go_image_get_name (real_image),
						    -1);
				path = gtk_tree_model_get_path (GTK_TREE_MODEL (state->model), &iter);
				gtk_icon_view_select_path (state->icon_view, path);
				gtk_tree_path_free (path);
			} else {
				g_object_unref (image);
				image = g_object_ref (real_image);
			}
			*(state->result) = image;
			gtk_dialog_response (GTK_DIALOG (state->dialog), GTK_RESPONSE_OK);
			gtk_widget_destroy (state->dialog);
			g_free (state->name);
			g_free (state->uri);
			g_free (state);
		}
	}
}
static gint
sort_func (GtkTreeModel *model,
	   GtkTreeIter  *a,
	   GtkTreeIter  *b,
	   gpointer      user_data)
{
	gchar *name_a, *name_b;
	int ret;

	gtk_tree_model_get (model, a, 1, &name_a, -1);
	gtk_tree_model_get (model, b, 1, &name_b, -1);
	ret = g_utf8_collate (name_a, name_b);

	g_free (name_a);
	g_free (name_b);

	return ret;
}

static gboolean
delete_event_cb (GtkWidget *cc, GdkEvent *event, GOImageSelState *state)
{
	gtk_widget_destroy (state->dialog);
	g_free (state->name);
	g_free (state->uri);
	g_free (state);
	return FALSE;
}

static void
ok_button_clicked_cb (GtkWidget *cc, GOImageSelState *state)
{
	GList *l = gtk_icon_view_get_selected_items (state->icon_view);
	if (*(state->result))
		g_object_unref (*(state->result));
	if (l) {
		GtkTreePath *path = l->data;
		GtkTreeIter iter;
		char *name;
		if (gtk_tree_model_get_iter (GTK_TREE_MODEL (state->model), &iter, path)) {
			gtk_tree_model_get (GTK_TREE_MODEL (state->model), &iter, 1, &name, -1);
			if (name) {
				*(state->result) = g_object_ref (go_doc_get_image (state->doc, name));
				g_free (name);
			}
		}
		g_list_foreach (l, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (l);
	} else
		*(state->result) = NULL;
	gtk_widget_destroy (state->dialog);
	g_free (state->name);
	g_free (state->uri);
	g_free (state);
}

static void
cancel_button_clicked_cb (GtkWidget *cc, GOImageSelState *state)
{
	gtk_widget_destroy (state->dialog);
	g_free (state->name);
	g_free (state->uri);
	g_free (state);
}

static void
add_image_cb (char const *key, GOImage *image, GOImageSelState *state)
{
	GtkTreeIter iter;
	GtkTreePath *path;
	gtk_list_store_append (state->model, &iter);
	gtk_list_store_set (state->model, &iter,
			    0, go_image_get_thumbnail (image),
			    1, go_image_get_name (image),
			    -1);
	path = gtk_tree_model_get_path (GTK_TREE_MODEL (state->model), &iter);
	if (image == *(state->result))
		gtk_icon_view_select_path (state->icon_view, path);
	gtk_tree_path_free (path);
}

static void
cb_selection_changed (GtkIconView *view, GOImageSelState *state)
{
	GList *l = gtk_icon_view_get_selected_items (view);
	gtk_widget_set_sensitive (state->ok_button, l != NULL);
	g_list_foreach (l, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (l);
}

/**
 * go_image_sel_new:
 * @doc: The #GODoc owning the image collection
 * @cc: A #GOCmdContext for error reporting
 * @image: A #GOImage
 *
 * Returns: (transfer full): and shows new image selector.
 **/
GtkWidget *
go_image_sel_new (GODoc *doc, GOCmdContext *cc, GOImage **image)
{
	GOImageSelState *state;
	GtkWidget *w;
	GHashTable *hash;

	g_return_val_if_fail (doc, NULL);

	if (image == NULL)
		return NULL;

	state = g_new0 (GOImageSelState, 1);
	state->doc = doc;
	state->cc = cc;
	state->result = image;
	state->gui = go_gtk_builder_load_internal ("res:go:gtk/go-image-sel.ui", GETTEXT_PACKAGE, state->cc);
        if (state->gui == NULL) {
		g_free (state);
                return NULL;
	}

	w = go_gtk_builder_get_widget (state->gui, "file-image-select");
	g_signal_connect (G_OBJECT (w),
		"clicked",
		G_CALLBACK (cb_file_image_select), state);

	state->icon_view = GTK_ICON_VIEW (gtk_builder_get_object (state->gui, "image-iconview"));
	state->model = gtk_list_store_new (2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	gtk_icon_view_set_model (state->icon_view , GTK_TREE_MODEL (state->model));
	gtk_icon_view_set_text_column (state->icon_view, 1);
	gtk_icon_view_set_pixbuf_column (state->icon_view, 0);
	g_object_unref (state->model);

	/* populate the list */
	hash = go_doc_get_images (doc);
	if (hash)
		g_hash_table_foreach (hash, (GHFunc) add_image_cb, state);

	/* Set sort column and function */
	gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (state->model),
					       sort_func,
					       NULL, NULL);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (state->model),
					    GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
					    GTK_SORT_ASCENDING);
	g_signal_connect (state->icon_view, "selection-changed", G_CALLBACK (cb_selection_changed), state);

	/* buttons */
	state->ok_button = go_gtk_builder_get_widget (state->gui, "ok-button");
	g_signal_connect (state->ok_button, "clicked", G_CALLBACK (ok_button_clicked_cb), state);
	gtk_widget_set_sensitive (state->ok_button, FALSE);
	w = go_gtk_builder_get_widget (state->gui, "cancel-button");
	g_signal_connect (w, "clicked", G_CALLBACK (cancel_button_clicked_cb), state);

	state->dialog = go_gtk_builder_get_widget (state->gui, "go-image-sel");
	g_signal_connect (state->dialog, "delete-event", G_CALLBACK (delete_event_cb), state);
	return state->dialog;
}
