/*
 * go-distribution-prefs.c
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
#include "go-distribution-prefs.h"
#include <goffice/graph/gog-data-set.h>
#include <goffice/math/go-distribution.h>
#include <goffice/utils/go-persist.h>
#include <glib/gi18n-lib.h>

typedef struct
{
	GObject *client;
	GParamSpec *props[2];
	GtkWidget *labels[2];
	GtkWidget *data[2];
	GtkGrid *grid;
	GogDataAllocator *dalloc;
} DistPrefs;

static void
destroy_cb (DistPrefs *prefs)
{
	g_free (prefs);
}

static void
distribution_changed_cb (GtkComboBox *box, DistPrefs *prefs)
{
	GODistribution *dist = NULL;
	int i, j, n;
	GtkTreeIter iter;
	GODistributionType dist_type;
	GParamSpec **props;
	GtkTreeModel *model = gtk_combo_box_get_model (box);
	gtk_combo_box_get_active_iter (box, &iter);
	gtk_tree_model_get (model, &iter, 1, &dist_type, -1);
	dist = go_distribution_new (dist_type);
	g_object_set (prefs->client, "distribution", dist, NULL);
	props = g_object_class_list_properties (G_OBJECT_GET_CLASS (dist), &n);
	for (j = i = 0; j < n; j++)
		if (props[j]->flags & GO_PARAM_PERSISTENT) {
			char *lbl = g_strconcat (_(g_param_spec_get_nick (props[j])), _(":"), NULL);
			if (!prefs->labels[i]) {
				GtkWidget *w = gtk_label_new (lbl);
				g_free (lbl);
				g_object_set (w, "xalign", 0., NULL);
				gtk_grid_attach (prefs->grid, w, 0, i + 1, 1, 1);
				prefs->labels[i] = w;
			} else
				gtk_label_set_text (GTK_LABEL (prefs->labels[i]), lbl);
			if (!prefs->data[i]) {
				GtkWidget *w = GTK_WIDGET (gog_data_allocator_editor (prefs->dalloc, GOG_DATASET (prefs->client), i, GOG_DATA_SCALAR));
				gtk_grid_attach (prefs->grid, w, 1, i + 1, 1, 1);
				prefs->data[i] = w;
			}
			gtk_widget_show (prefs->labels[i]);
			gtk_widget_show (prefs->data[i]);
			prefs->props[i++] = props[j];
		}
	while (i < 2) {
		if (prefs->labels[i])
			gtk_widget_hide (prefs->labels[i]);
		if (prefs->data[i])
			gtk_widget_hide (prefs->data[i]);
		prefs->props[i++] = NULL;
	}
	g_free (props);
	g_object_unref (dist);
}

GtkWidget *
go_distribution_pref_new (GObject *obj, GogDataAllocator *dalloc, G_GNUC_UNUSED GOCmdContext *cc)
{
	GtkListStore *model;
	GtkCellRenderer *renderer;
	GtkTreeIter iter;
	GParamSpec **props;
	int n, i, j;
	DistPrefs *prefs = g_new0 (DistPrefs, 1);
	GtkWidget *res = gtk_grid_new ();
	GtkWidget *w = gtk_label_new (_("Distribution:"));
	GODistribution *dist = NULL;
	GODistributionType dist_type;

	prefs->dalloc = dalloc;
	prefs->grid = GTK_GRID (res);
	g_object_get (obj, "distribution", &dist, NULL);
	g_return_val_if_fail (GO_IS_DISTRIBUTION (dist), NULL);

	dist_type = go_distribution_get_distribution_type (dist);
	g_object_set (res, "border-width", 12, "row-spacing", 12, "column-spacing", 24, NULL);
	g_object_set (w, "xalign", 0., NULL);
	gtk_grid_attach (prefs->grid, w, 0, 0, 1, 1);
	g_signal_connect_swapped (res, "destroy", G_CALLBACK (destroy_cb), prefs);
	prefs->client = obj;

	/* add the list of known distributions in a combobox */
	model = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);
	w = gtk_combo_box_new_with_model (GTK_TREE_MODEL (model));
	g_object_unref (model);
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (w), renderer, FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (w), renderer,
					"text", 0,
					NULL);
	for (i = 0; i < GO_DISTRIBUTION_MAX; i++) {
		gtk_list_store_append (model, &iter);
		gtk_list_store_set (model, &iter, 0, _(go_distribution_type_to_string (i)), 1, i, -1);
		if (i == dist_type)
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX (w), &iter);
	}
	g_signal_connect (w, "changed", G_CALLBACK (distribution_changed_cb), prefs);
	gtk_grid_attach (prefs->grid, w, 1, 0, 1, 1);

	/* other persistent properties */
	i = 1;
	props = g_object_class_list_properties (G_OBJECT_GET_CLASS (dist), &n);
	for (j = 0; j < n; j++)
		if (props[j]->flags & GO_PARAM_PERSISTENT) {
			char *lbl = g_strconcat (_(g_param_spec_get_nick (props[j])), _(":"), NULL);
			w = gtk_label_new (lbl);
			g_free (lbl);
			g_object_set (w, "xalign", 0., NULL);
			gtk_grid_attach (prefs->grid, w, 0, i, 1, 1);
			prefs->labels[i-1] = w;
			prefs->props[i-1] = props[n];
			w = GTK_WIDGET (gog_data_allocator_editor (dalloc, GOG_DATASET (obj), i - 1, GOG_DATA_SCALAR));
			gtk_grid_attach (prefs->grid, w, 1, i, 1, 1);
			prefs->data[i] = w;
			i++;
		}
	g_free (props);

	gtk_widget_show_all (res);
	return res;
}
