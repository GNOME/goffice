/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
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
	GtkEntry    *name_entry;
	GtkIconView *icon_view;
	GtkListStore *model;
	char *uri;
};

static void
cb_file_image_select (GtkWidget *cc, GOImageSelState *state)
{
	g_free (state->uri);

	state->uri = go_gtk_select_image (GTK_WINDOW (gtk_widget_get_toplevel (cc)),
				   NULL);
}

static void
cb_image_add (GtkWidget *cc, GOImageSelState *state)
{
	char const *name;
	char *image_name, *filename;
	GError *error = NULL;
	GOImage *image, *real_image;
	if (!state->uri)
		return;
	name = gtk_entry_get_text (state->name_entry);
	filename = go_filename_from_uri (state->uri);
	g_free (state->uri);
	state->uri = NULL;
	if (!(name && strlen (name))) {
		char *basename = g_path_get_basename (filename);
		char *dot = strrchr (basename, '.');
		image_name = (dot)? g_strndup (basename, dot - basename): g_strdup (basename);
		g_free (basename);
	} else
		image_name = g_strdup (name);
	image = go_image_new_from_file (filename, &error);
	g_free (filename);
	if (error) {
		g_warning ("%s", error->message);
		g_error_free (error);
		error = NULL;
	}
	if (image) {
		real_image = go_doc_add_image (state->doc, image_name, image);
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
		}
		g_object_unref (image);
	}
	g_free (image_name);
	gtk_entry_set_text (state->name_entry, "");
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
	g_free (state->uri);
	g_free (state);
}

static void
cancel_button_clicked_cb (GtkWidget *cc, GOImageSelState *state)
{
	gtk_widget_destroy (state->dialog);
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

/**
 * go_image_sel_new
 * @doc		: The #GODoc owning the image collection
 * @image	:  #GOImage
 *
 * Returns: and shows new image selector.
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
	state->gui = go_gtk_builder_new ("go-image-sel.ui", GETTEXT_PACKAGE, state->cc);
        if (state->gui == NULL) {
		g_free (state);
                return NULL;
	}

	w = go_gtk_builder_get_widget (state->gui, "file-image-select");
	g_signal_connect (G_OBJECT (w),
		"clicked",
		G_CALLBACK (cb_file_image_select), state);

	w = go_gtk_builder_get_widget (state->gui, "add");
	g_signal_connect (G_OBJECT (w),
		"clicked",
		G_CALLBACK (cb_image_add), state);

	state->name_entry = GTK_ENTRY (gtk_builder_get_object (state->gui, "name-entry"));
	state->icon_view = GTK_ICON_VIEW (gtk_builder_get_object (state->gui, "image-iconview"));
	state->model = gtk_list_store_new (2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	gtk_icon_view_set_model (state->icon_view , GTK_TREE_MODEL (state->model));
	g_object_unref (state->model);

	/* Set sort column and function */
	gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (state->model),
					       sort_func,
					       NULL, NULL);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (state->model),
					    GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
					    GTK_SORT_ASCENDING);

	/* populate the list */
	hash = go_doc_get_images (doc);
	if (hash)
		g_hash_table_foreach (hash, (GHFunc) add_image_cb, state);

	/* buttons */
	w = go_gtk_builder_get_widget (state->gui, "ok-button");
	g_signal_connect (w, "clicked", G_CALLBACK (ok_button_clicked_cb), state);
	w = go_gtk_builder_get_widget (state->gui, "cancel-button");
	g_signal_connect (w, "clicked", G_CALLBACK (cancel_button_clicked_cb), state);

	state->dialog = go_gtk_builder_get_widget (state->gui, "go-image-sel");
	g_signal_connect (state->dialog, "delete-event", G_CALLBACK (delete_event_cb), state);
	return state->dialog;
}
