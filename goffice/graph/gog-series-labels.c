/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-series-labels.c
 *
 * Copyright (C) 2011 Jean Brefort (jean.brefort@normalesup.org)
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

#include <goffice/goffice-config.h>
#include <goffice/graph/gog-series-labels.h>

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

typedef GogOutlinedObjectClass GogSeriesLabelsClass;

static GObjectClass *series_labels_parent_klass;

enum {
	SERIES_LABELS_PROP_0,
	SERIES_LABELS_PROP_POSITION,
	SERIES_LABELS_PROP_OFFSET,
	SERIES_LABELS_PROP_FORMAT
};

struct {
	char const *label;
	GogSeriesLabelsPos pos;
} positions [] =
{
	{ N_("Centered"), GOG_SERIES_LABELS_CENTERED },
	{ N_("Top"), GOG_SERIES_LABELS_TOP },
	{ N_("Bottom"), GOG_SERIES_LABELS_BOTTOM },
	{ N_("Left"), GOG_SERIES_LABELS_LEFT },
	{ N_("Right"), GOG_SERIES_LABELS_RIGHT },
	{ N_("Outside"), GOG_SERIES_LABELS_OUTSIDE },
	{ N_("Inside"), GOG_SERIES_LABELS_INSIDE },
	{ N_("Near origin"), GOG_SERIES_LABELS_NEAR_ORIGIN }
};

#ifdef GOFFICE_WITH_GTK

struct SeriesLabelsState {
	GtkWidget *offset_btn, *offset_lbl;
	GogSeriesLabels *labels;
	GtkListStore *avail_list, *used_list;
	GtkTreeSelection *avail_sel, *used_sel;
	GtkWidget *raise, *lower, *add, *remove;
};

static void
used_selection_changed_cb (struct SeriesLabelsState *state)
{
	GtkTreeIter iter, last, first;
	GtkTreePath *f, *l;
	GtkTreeModel *model = GTK_TREE_MODEL (state->used_list);
	int count = 0;  /* we count the unselected items to avoid empty labels */
	g_free (state->labels->format);
	state->labels->format = NULL;
	if (gtk_tree_model_get_iter_first (model, &iter)) {
		GtkTreeModel *model = GTK_TREE_MODEL (state->used_list);
		/* if the first row is not selected and a second row exists, set the up button sensitive */
		first = last = iter;
		gtk_widget_set_sensitive (state->raise,
		                          !gtk_tree_selection_iter_is_selected (state->used_sel, &iter)
		                          && gtk_tree_selection_count_selected_rows (state->used_sel)
		                          && gtk_tree_model_iter_next (model, &last));
		/* now find the last iter */
		do {
			int dim;
			char *new_format;
			last = iter;
			if (!gtk_tree_selection_iter_is_selected (state->used_sel, &last))
				count++;
			gtk_tree_model_get (model, &iter, 1, &dim, -1);
			switch (dim) {
			case -1:
				new_format = (state->labels->format)?
						g_strconcat (state->labels->format, " %c", NULL):
						g_strdup ("%c");
				break;
			case -2:
				new_format = (state->labels->format)?
						g_strconcat (state->labels->format, " %l", NULL):
						g_strdup ("%l");
				break;
			default:
				new_format = (state->labels->format)?
						g_strdup_printf ("%s %%%d", state->labels->format, dim):
						g_strdup_printf ("%%%d", dim);
			}
			g_free (state->labels->format);
			state->labels->format = new_format;
		} while (gtk_tree_model_iter_next (model, &iter));
		f = gtk_tree_model_get_path (model, &first);
		l = gtk_tree_model_get_path (model, &last);
		gtk_widget_set_sensitive (state->lower,
		                          !gtk_tree_selection_iter_is_selected (state->used_sel, &last)
		                          && gtk_tree_selection_count_selected_rows (state->used_sel)
		                          && gtk_tree_path_compare (f, l));
		gtk_tree_path_free (f);
		gtk_tree_path_free (l);
	} else {
		gtk_widget_set_sensitive (state->raise, FALSE);
		gtk_widget_set_sensitive (state->lower, FALSE);
	}
	gtk_widget_set_sensitive (state->remove,
	                          count > 0 && gtk_tree_selection_count_selected_rows (state->used_sel));
	gog_object_emit_changed (GOG_OBJECT (state->labels), TRUE);
}

static void
add_cb (G_GNUC_UNUSED GtkButton *btn, struct SeriesLabelsState *state)
{
	GtkTreeIter iter, add_iter;
	char *name;
	int id;
	gtk_tree_model_get_iter_first (GTK_TREE_MODEL (state->avail_list), &iter);
	/* we don't need to check if the iter is valid since otherwise,
	 the button would be unsensitive */
	while (1) {
		if (gtk_tree_selection_iter_is_selected (state->avail_sel, &iter)) {
			gtk_tree_model_get (GTK_TREE_MODEL (state->avail_list),
			                    &iter, 0, &name, 1, &id, -1);
			gtk_list_store_append (state->used_list, &add_iter);
			gtk_list_store_set (state->used_list, &add_iter,
			                    0, name, 1, id, -1);
			g_free (name);
			if (!gtk_list_store_remove (state->avail_list, &iter))
				break;
		} else if (!gtk_tree_model_iter_next (GTK_TREE_MODEL (state->avail_list), &iter))
			break;
	}
	used_selection_changed_cb (state);
	gog_object_emit_changed (GOG_OBJECT (state->labels), TRUE);
}

static void
remove_cb (G_GNUC_UNUSED GtkButton *btn, struct SeriesLabelsState *state)
{
	GtkTreeIter iter, add_iter;
	char *name;
	int id;
	gtk_tree_model_get_iter_first (GTK_TREE_MODEL (state->used_list), &iter);
	/* we don't need to check if the iter is valid since otherwise,
	 the button would be unsensitive */
	while (1) {
		if (gtk_tree_selection_iter_is_selected (state->used_sel, &iter)) {
			gtk_tree_model_get (GTK_TREE_MODEL (state->used_list),
			                    &iter, 0, &name, 1, &id, -1);
			gtk_list_store_append (state->avail_list, &add_iter);
			gtk_list_store_set (state->avail_list, &add_iter,
			                    0, name, 1, id, -1);
			g_free (name);
			if (!gtk_list_store_remove (state->used_list, &iter))
				break;
		} else if (!gtk_tree_model_iter_next (GTK_TREE_MODEL (state->used_list), &iter))
			break;
	}
	used_selection_changed_cb (state);
	gog_object_emit_changed (GOG_OBJECT (state->labels), TRUE);
}

static void
raise_cb (G_GNUC_UNUSED GtkButton *btn, struct SeriesLabelsState *state)
{
	GtkTreeModel *model = GTK_TREE_MODEL (state->used_list);
	GtkTreeIter iter, prev;
	gtk_tree_model_get_iter_first (model, &iter);
	while (prev = iter, gtk_tree_model_iter_next (model, &iter))
		if (gtk_tree_selection_iter_is_selected (state->used_sel, &iter))
			gtk_list_store_move_before (state->used_list, &iter, &prev);
	gtk_tree_model_get_iter_first (model, &iter);
	used_selection_changed_cb (state);
	gog_object_emit_changed (GOG_OBJECT (state->labels), FALSE);
}

static void
lower_cb (G_GNUC_UNUSED GtkButton *btn, struct SeriesLabelsState *state)
{
	GtkTreeModel *model = GTK_TREE_MODEL (state->used_list);
	GtkTreeIter iter, prev, last;
	gboolean valid = TRUE;
	gtk_tree_model_get_iter_first (model, &iter);
	while (valid) {
		while (!gtk_tree_selection_iter_is_selected (state->used_sel, &iter)) {
			if (!gtk_tree_model_iter_next (model, &iter)) {
				valid = FALSE;
				break;
			}
		}
		if (!valid)
			break;
		prev = last = iter; /* first selected row in the block */
		while (gtk_tree_selection_iter_is_selected (state->used_sel, &iter)) {
			if (!gtk_tree_model_iter_next (model, &iter)) {
				valid = FALSE;
				break;
			}
			last = iter;
		}
		if (!valid)
			break;
		valid = gtk_tree_model_iter_next (model, &iter);
		/* at this point,last should be the unselectd row after the selected block */
		gtk_list_store_move_before (state->used_list, &last, &prev);
	}
	used_selection_changed_cb (state);
	gog_object_emit_changed (GOG_OBJECT (state->labels), FALSE);
}

static void
position_changed_cb (GtkComboBox *box, struct SeriesLabelsState *state)
{
	GtkTreeModel *model = gtk_combo_box_get_model (box);
	GtkTreeIter iter;
	GogSeriesLabelsPos pos;

	if (gtk_combo_box_get_active_iter (box, &iter)) {
		gtk_tree_model_get (model, &iter, 1, &pos, -1);
		gog_series_labels_set_position (state->labels, pos);
		if (gog_series_labels_get_position (state->labels) ==  GOG_SERIES_LABELS_CENTERED) {
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (state->offset_btn), 0);
			gtk_widget_set_sensitive (state->offset_btn, FALSE);
			gtk_widget_set_sensitive (state->offset_lbl, FALSE);
		} else {
			gtk_widget_set_sensitive (state->offset_btn, TRUE);
			gtk_widget_set_sensitive (state->offset_lbl, TRUE);
		}
	}
	gog_object_emit_changed (GOG_OBJECT (state->labels), TRUE);
}

static void
offset_changed_cb (GtkSpinButton *btn, GogSeriesLabels *labels)
{
	labels->offset = gtk_spin_button_get_value_as_int (btn);
	gog_object_emit_changed (gog_object_get_parent (GOG_OBJECT (labels)), TRUE);
}

static void
avail_selection_changed_cb (struct SeriesLabelsState *state)
{
	gtk_widget_set_sensitive (state->add,
	                          gtk_tree_selection_count_selected_rows (state->avail_sel));
}

static void
gog_series_labels_populate_editor (GogObject *gobj,
				   GOEditor *editor,
				   GogDataAllocator *dalloc,
				   GOCmdContext *cc)
{
	GtkBuilder *gui;
	GtkWidget *labels_prefs, *w;
	GtkComboBox *box;
	GtkListStore *list;
	GtkCellRenderer *cell;
	GtkTreeIter iter;
	GogSeriesLabels *labels = (GogSeriesLabels *) gobj;
	GogPlot *plot = (GogPlot *) gog_object_get_parent_typed (gobj, GOG_TYPE_PLOT);
	int i = 0, id = -1, def = 0;
	unsigned j;
	struct SeriesLabelsState *state = g_new (struct SeriesLabelsState, 1);

	gui = go_gtk_builder_new ("gog-series-labels-prefs.ui", GETTEXT_PACKAGE, cc);
	labels_prefs = go_gtk_builder_get_widget (gui, "series-labels-prefs");
	state->labels = labels;

	box = GTK_COMBO_BOX (go_gtk_builder_get_widget (gui, "position-box"));
	list = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_UINT);
	gtk_combo_box_set_model (box, GTK_TREE_MODEL (list));
	cell = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (box), cell, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (box), cell,
					"text", 0, NULL);
	for (j = 0; j < G_N_ELEMENTS (positions); j++)
		if (labels->allowed_pos & positions[j].pos) {
			gtk_list_store_append (list, &iter);
			gtk_list_store_set (list, &iter, 0, _(positions[j].label), 1, positions[j].pos, -1);
			if (labels->position == positions[j].pos)
					id = i;
			if (labels->default_pos == positions[j].pos)
				def = i;
			i++;
		}
	gtk_combo_box_set_active (box, (id >= 0)? id: def);
	g_signal_connect (G_OBJECT (box), "changed", G_CALLBACK (position_changed_cb), state);

	w = go_gtk_builder_get_widget (gui, "offset-btn");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), labels->offset);
	g_signal_connect (G_OBJECT (w), "value-changed", G_CALLBACK (offset_changed_cb), labels);
	state->offset_btn = w;
	state->offset_lbl = go_gtk_builder_get_widget (gui, "offset-label");

	w = go_gtk_builder_get_widget (gui, "available-tree");
	cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (w), 0, _("Available data"),
	                                             cell, "text", 0, NULL);
	state->avail_sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (w));
	gtk_tree_selection_set_mode (state->avail_sel, GTK_SELECTION_MULTIPLE);
	g_signal_connect_swapped (G_OBJECT (state->avail_sel), "changed",
	                  G_CALLBACK (avail_selection_changed_cb), state);
	w = go_gtk_builder_get_widget (gui, "used-tree");
	cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (w), 0, _("Used data"),
	                                             cell, "text", 0, NULL);
	state->used_sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (w));
	gtk_tree_selection_set_mode (state->used_sel, GTK_SELECTION_MULTIPLE);
	g_signal_connect_swapped (G_OBJECT (state->used_sel), "changed",
	                  G_CALLBACK (used_selection_changed_cb), state);

	state->add = go_gtk_builder_get_widget (gui, "add");
	gtk_widget_set_sensitive (state->add, FALSE);
	g_signal_connect (G_OBJECT (state->add), "clicked", G_CALLBACK (add_cb), state);
	state->remove = go_gtk_builder_get_widget (gui, "remove");
	gtk_widget_set_sensitive (state->remove, FALSE);
	g_signal_connect (G_OBJECT (state->remove), "clicked", G_CALLBACK (remove_cb), state);
	state->raise = go_gtk_builder_get_widget (gui, "raise");
	gtk_widget_set_sensitive (state->raise, FALSE);
	g_signal_connect (G_OBJECT (state->raise), "clicked", G_CALLBACK (raise_cb), state);
	state->lower = go_gtk_builder_get_widget (gui, "lower");
	gtk_widget_set_sensitive (state->lower, FALSE);
	g_signal_connect (G_OBJECT (state->lower), "clicked", G_CALLBACK (lower_cb), state);

	if (plot != NULL) {
		/* what should be done if there is no plot, btw is it possible that there is no plot? */
		GSList *dims = NULL;
		char *cur;
		state->avail_list = GTK_LIST_STORE (gtk_builder_get_object (gui, "available-list"));
		gtk_list_store_clear (state->avail_list); /* GtkBuilder seems to add an empty line */
		state->used_list = GTK_LIST_STORE (gtk_builder_get_object (gui, "used-list"));
		gtk_list_store_clear (state->used_list);
		/* populate used list */
		cur = labels->format;
		while (*cur) {
			/* go to next % */
			while (*cur && *cur != '%')
				cur = g_utf8_next_char (cur);
			cur++; /* skip the % */
			switch (*cur) {
			case 0:
				break; /* protect against a % terminated string */
			case 'c':
				gtk_list_store_append (state->used_list, &iter);
				gtk_list_store_set (state->used_list, &iter, 0, _("Custom labels"), 1, -1, -1);
				dims = g_slist_prepend (dims, GINT_TO_POINTER (-1));
				break;
			case 'l':
				gtk_list_store_append (state->used_list, &iter);
				gtk_list_store_set (state->used_list, &iter, 0, _("Legend entry"), 1, -2, -1);
				dims = g_slist_prepend (dims, GINT_TO_POINTER (-2));
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9': {
				unsigned dim = strtoul (cur, &cur, 10);
				if (dim < plot->desc.series.num_dim)
				switch (plot->desc.series.dim[j].ms_type) {
				case GOG_MS_DIM_ERR_plus1:
				case GOG_MS_DIM_ERR_minus1:
				case GOG_MS_DIM_ERR_plus2:
				case GOG_MS_DIM_ERR_minus2:
					/* we need to eliminate these */
					break;
				default:
					gtk_list_store_append (state->used_list, &iter);
					gtk_list_store_set (state->used_list, &iter, 0, _(plot->desc.series.dim[dim].name), 1, dim, -1);
					dims = g_slist_prepend (dims, GINT_TO_POINTER (dim));
					break;
				}
				break;
			}
			default:
				cur = g_utf8_next_char (cur); /* skip unknown character */
			}
		}
		for (j = 0; j < plot->desc.series.num_dim; j++) {
			switch (plot->desc.series.dim[j].ms_type) {
			case GOG_MS_DIM_ERR_plus1:
			case GOG_MS_DIM_ERR_minus1:
			case GOG_MS_DIM_ERR_plus2:
			case GOG_MS_DIM_ERR_minus2:
				/* we need to eliminate these */
				break;
			default:
				if (!g_slist_find (dims, GUINT_TO_POINTER (j))) {
					gtk_list_store_append (state->avail_list, &iter);
					gtk_list_store_set (state->avail_list, &iter, 0, _(plot->desc.series.dim[j].name), 1, j, -1);
				}
				break;
			}
		}
		if (!g_slist_find (dims, GINT_TO_POINTER (-1))) {
			gtk_list_store_append (state->avail_list, &iter);
			gtk_list_store_set (state->avail_list, &iter, 0, _("Custom labels"), 1, -1, -1);
		}
		if (!g_slist_find (dims, GINT_TO_POINTER (-2))) {
			gtk_list_store_append (state->avail_list, &iter);
			gtk_list_store_set (state->avail_list, &iter, 0, _("Legend entry"), 1, -2, -1);
		}
		gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (state->avail_list), 1, GTK_SORT_ASCENDING);
	}
	w = GTK_WIDGET (gog_data_allocator_editor (dalloc, GOG_DATASET (gobj), 0, GOG_DATA_VECTOR));
	gtk_widget_show (w);
	gtk_grid_attach (GTK_GRID (labels_prefs), w, 2, 6, 3, 1);

	g_object_set_data_full (G_OBJECT (labels_prefs), "state", state, g_free);

	go_editor_add_page (editor,labels_prefs, _("Details"));
	gtk_widget_show_all (labels_prefs),
	g_object_unref (gui);
	(GOG_OBJECT_CLASS(series_labels_parent_klass)->populate_editor) (gobj, editor, dalloc, cc);
}
#endif

static void
gog_series_labels_init_style (GogStyledObject *gso, GOStyle *style)
{
	style->interesting_fields = GO_STYLE_OUTLINE | GO_STYLE_FILL |
		GO_STYLE_FONT | GO_STYLE_TEXT_LAYOUT;
	gog_theme_fillin_style (gog_object_get_theme (GOG_OBJECT (gso)),
		style, GOG_OBJECT (gso), 0, GO_STYLE_OUTLINE | GO_STYLE_FILL |
	        GO_STYLE_FONT | GO_STYLE_TEXT_LAYOUT);
}

static void
gog_series_labels_set_property (GObject *obj, guint param_id,
			 GValue const *value, GParamSpec *pspec)
{
	GogSeriesLabels *labels = GOG_SERIES_LABELS (obj);

	switch (param_id) {
	case SERIES_LABELS_PROP_POSITION: {
		char const *name = g_value_get_string (value);
		if (!strcmp (name, "centered"))
			gog_series_labels_set_position (labels, GOG_SERIES_LABELS_CENTERED);
		else if (!strcmp (name, "top"))
			gog_series_labels_set_position (labels, GOG_SERIES_LABELS_TOP);
		else if (!strcmp (name, "bottom"))
			gog_series_labels_set_position (labels, GOG_SERIES_LABELS_BOTTOM);
		else if (!strcmp (name, "left"))
			gog_series_labels_set_position (labels, GOG_SERIES_LABELS_LEFT);
		else if (!strcmp (name, "right"))
			gog_series_labels_set_position (labels, GOG_SERIES_LABELS_RIGHT);
		else if (!strcmp (name, "outside"))
			gog_series_labels_set_position (labels, GOG_SERIES_LABELS_OUTSIDE);
		else if (!strcmp (name, "inside"))
			gog_series_labels_set_position (labels, GOG_SERIES_LABELS_INSIDE);
		else if (!strcmp (name, "near origin"))
			gog_series_labels_set_position (labels, GOG_SERIES_LABELS_NEAR_ORIGIN);
		return;
	}
	case SERIES_LABELS_PROP_OFFSET: {
		unsigned offset = g_value_get_uint (value);
		if (offset == labels->offset)
			return;
		labels->offset = offset;
		break;
	case SERIES_LABELS_PROP_FORMAT:
		g_free (labels->format);
		labels->format = g_strdup (g_value_get_string (value));
		break;
	}

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}

	gog_object_emit_changed (gog_object_get_parent (GOG_OBJECT (obj)), TRUE);
}

static void
gog_series_labels_get_property (GObject *obj, guint param_id,
			 GValue *value, GParamSpec *pspec)
{
	GogSeriesLabels *labels = GOG_SERIES_LABELS (obj);

	switch (param_id) {
	case SERIES_LABELS_PROP_POSITION: {
		char const *posname;
		switch (labels->position) {
		default:
		case GOG_SERIES_LABELS_DEFAULT_POS:
			posname = "default";
			break;
		case GOG_SERIES_LABELS_CENTERED:
			posname = "centered";
			break;
		case GOG_SERIES_LABELS_TOP:
			posname = "top";
			break;
		case GOG_SERIES_LABELS_BOTTOM:
			posname = "bottom";
			break;
		case GOG_SERIES_LABELS_LEFT:
			posname = "left";
			break;
		case GOG_SERIES_LABELS_RIGHT:
			posname = "right";
			break;
		case GOG_SERIES_LABELS_OUTSIDE:
			posname = "outside";
			break;
		case GOG_SERIES_LABELS_INSIDE:
			posname = "inside";
			break;
		case GOG_SERIES_LABELS_NEAR_ORIGIN:
			posname = "near origin";
			break;
		}
		g_value_set_string (value, posname);
		break;
	}
	case SERIES_LABELS_PROP_OFFSET:
		g_value_set_uint (value, labels->offset);
		break;
	case SERIES_LABELS_PROP_FORMAT:
		g_value_set_string (value, labels->format);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
gog_series_labels_parent_changed (GogObject *obj, gboolean was_set)
{
	GogSeriesLabels *labels = GOG_SERIES_LABELS (obj);
	GogPlot *plot;
	unsigned j;

	if (!was_set)
		return;
	plot = (GogPlot *) gog_object_get_parent_typed (obj, GOG_TYPE_PLOT);
	g_free (labels->format);
	labels->format = NULL;
	for (j = 0; j < plot->desc.series.num_dim; j++) {
		/* FIXME, this might depend upon the series type */
		switch (plot->desc.series.dim[j].ms_type) {
		case GOG_MS_DIM_VALUES:
			labels->format = g_strdup_printf ("%%%u", j);
			j = plot->desc.series.num_dim; /* ensure we exit the loop */
			break;
		default:
			break;
		}
	}

}

static void
gog_series_labels_changed (GogObject *obj, gboolean size)
{
	gog_object_emit_changed (gog_object_get_parent (obj), size);
	gog_object_request_update (gog_object_get_parent (obj));
}

struct attr_closure {
	PangoAttrList *l;
	unsigned offset;
};

static gboolean
attr_position (PangoAttribute *attr, gpointer data)
{
	struct attr_closure *c = (struct attr_closure *) data;
	PangoAttribute *new_attr = pango_attribute_copy (attr);
	new_attr->start_index += c->offset;
	new_attr->end_index += c->offset;
	pango_attr_list_change (c->l, new_attr);
	return FALSE;
}

static void
gog_series_labels_update (GogObject *obj)
{
	GogSeriesLabels *labels = GOG_SERIES_LABELS (obj);
	GogObject *parent = gog_object_get_parent (GOG_OBJECT (obj));
	GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (obj));
	int height = pango_font_description_get_size (style->font.font->desc);
	unsigned i, n;
	if (labels->elements) {
		n = labels->n_elts;
		for (i = 0; i < n; i++)
			go_string_unref (labels->elements[i].str);
		g_free (labels->elements);
	}
	if (GOG_IS_SERIES (parent)) {
		GogSeries *series = GOG_SERIES (parent);
		labels->n_elts = n = gog_series_num_elements (series);
		labels->elements = g_new0 (GogSeriesLabelElt, n);
		for (i = 0; i < n; i++) {
			GString *str = g_string_new ("");
			unsigned index;
			PangoAttrList *markup = pango_attr_list_new (), *l;
			char const *format = labels->format;
			char *next;
			while (*format) {
				if (*format == '%') {
					format++;
					switch (*format) {
					case 0: /* protect from an unexpected string end */
						break;
					case 'c':
						next = go_data_get_vector_string (labels->custom_labels.data, i);
						if (next) {
							index = str->len;
							g_string_append (str, next);
							g_free (next);
							l = go_data_get_vector_markup (labels->custom_labels.data, i);
							if (l) {
								struct attr_closure c;
								c.l = markup;
								c.offset = index;
								pango_attr_list_filter (l, attr_position, &c);
								pango_attr_list_unref (l);
							}
						}
						break;
					case 'l': {
						/* add room for the legend entry */
						PangoRectangle rect;
						PangoAttribute *attr;
						rect.x = rect.y = 0;
						rect.height = 1; /* only the width is important */
						rect.width = 2 * height; /* FIXME, should not always be 2 */
						attr = pango_attr_shape_new (&rect, &rect);
						attr->start_index = str->len;
						g_string_append_c (str, 'o');
						attr->end_index = str->len;
						pango_attr_list_insert (markup, attr);
						break;
					}
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						index = *format - '0';
						next = go_data_get_vector_string (series->values[index].data, i);
						if (next) {
							g_string_append (str, next);
							g_free (next);
						}
						break;
					case '%':
						g_string_append_c (str, '%');
						break;
					default:
						continue;
					}
					format++;
				} else {
					next = g_utf8_next_char (format);
					g_string_append_len (str, format, next - format);
					format = next;
				}
			}
			labels->elements[i].str = go_string_new_rich (g_string_free (str, FALSE), -1, FALSE, markup, NULL);
		}
	} else
		labels->elements = NULL; /* FIXME: might be a SeriesElement */
}

static void
gog_series_labels_finalize (GObject *obj)
{
	GogSeriesLabels *labels = GOG_SERIES_LABELS (obj);
	gog_dataset_finalize (GOG_DATASET (obj));
	g_free (labels->format);
	if (labels->elements) {
		unsigned i, n = labels->n_elts;
		for (i = 0; i < n; i++)
			go_string_unref (labels->elements[i].str);
		g_free (labels->elements);
	}
	series_labels_parent_klass->finalize (obj);
}

static void
gog_series_labels_class_init (GObjectClass *obj_klass)
{
	GogObjectClass *gog_klass = (GogObjectClass *) obj_klass;
	GogStyledObjectClass *style_klass = (GogStyledObjectClass *) obj_klass;
	series_labels_parent_klass = g_type_class_peek_parent (obj_klass);

	obj_klass->set_property	= gog_series_labels_set_property;
	obj_klass->get_property	= gog_series_labels_get_property;
	obj_klass->finalize	= gog_series_labels_finalize;
        g_object_class_install_property (obj_klass, SERIES_LABELS_PROP_POSITION,
		 g_param_spec_string ("position",
			_("Position"),
			_("Position of the label relative to the data graphic element"),
			"default",
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
        g_object_class_install_property (obj_klass, SERIES_LABELS_PROP_OFFSET,
		 g_param_spec_uint ("offset",
			_("Offset"),
			_("Offset to add to the label position"),
			0, 10, 0,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
        g_object_class_install_property (obj_klass, SERIES_LABELS_PROP_FORMAT,
		 g_param_spec_string ("format",
			_("Format"),
			_("Label format"),
			"",
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

#ifdef GOFFICE_WITH_GTK
	gog_klass->populate_editor = gog_series_labels_populate_editor;
#endif
	gog_klass->parent_changed = gog_series_labels_parent_changed;
	gog_klass->changed = gog_series_labels_changed;
	gog_klass->update = gog_series_labels_update;
	style_klass->init_style = gog_series_labels_init_style;
}

static void
gog_series_labels_dataset_dims (GogDataset const *set, int *first, int *last)
{
	*first = 0;
	*last = 0;
}

static GogDatasetElement *
gog_series_labels_dataset_get_elem (GogDataset const *set, int dim_i)
{
	GogSeriesLabels const *sl = GOG_SERIES_LABELS (set);
	g_return_val_if_fail (0 == dim_i, NULL);
	return (GogDatasetElement *) &sl->custom_labels;
}

static void
gog_series_labels_dataset_dim_changed (GogDataset *set, int dim_i)
{
	gog_object_request_update (gog_object_get_parent (GOG_OBJECT (set)));
}

static void
gog_series_labels_dataset_init (GogDatasetClass *iface)
{
	iface->get_elem	   = gog_series_labels_dataset_get_elem;
	iface->dims	   = gog_series_labels_dataset_dims;
	iface->dim_changed = gog_series_labels_dataset_dim_changed;
}

GSF_CLASS_FULL (GogSeriesLabels, gog_series_labels,
		NULL, NULL, gog_series_labels_class_init, NULL,
		NULL, GOG_TYPE_OUTLINED_OBJECT, 0,
                GSF_INTERFACE (gog_series_labels_dataset_init, GOG_TYPE_DATASET))

void
gog_series_labels_set_allowed_position (GogSeriesLabels *lbls, unsigned allowed)
{
	lbls->allowed_pos = allowed;
	if ((lbls->position & allowed) == 0 && lbls->position != GOG_SERIES_LABELS_DEFAULT_POS) {
		lbls->position = GOG_SERIES_LABELS_DEFAULT_POS;
		gog_object_emit_changed (gog_object_get_parent (GOG_OBJECT (lbls)), TRUE);
	}
}

void
gog_series_labels_set_position (GogSeriesLabels *lbls, GogSeriesLabelsPos pos)
{
	switch (pos) {
	case GOG_SERIES_LABELS_DEFAULT_POS:
	case GOG_SERIES_LABELS_CENTERED:
	case GOG_SERIES_LABELS_TOP:
	case GOG_SERIES_LABELS_BOTTOM:
	case GOG_SERIES_LABELS_LEFT:
	case GOG_SERIES_LABELS_RIGHT:
	case GOG_SERIES_LABELS_OUTSIDE:
	case GOG_SERIES_LABELS_INSIDE:
	case GOG_SERIES_LABELS_NEAR_ORIGIN:
		break;
	default:
		return;
	}
	if ((lbls->allowed_pos & pos) != 0  && lbls->position != pos) {
		lbls->position = (pos == lbls->default_pos)? GOG_SERIES_LABELS_DEFAULT_POS: pos;
		if (gog_series_labels_get_position (lbls) == GOG_SERIES_LABELS_CENTERED)
			lbls->offset = 0;
		gog_object_emit_changed (gog_object_get_parent (GOG_OBJECT (lbls)), TRUE);
	}
}

void
gog_series_labels_set_default_position (GogSeriesLabels *lbls, GogSeriesLabelsPos pos)
{
	switch (pos) {
	case GOG_SERIES_LABELS_CENTERED:
	case GOG_SERIES_LABELS_TOP:
	case GOG_SERIES_LABELS_BOTTOM:
	case GOG_SERIES_LABELS_LEFT:
	case GOG_SERIES_LABELS_RIGHT:
	case GOG_SERIES_LABELS_OUTSIDE:
	case GOG_SERIES_LABELS_INSIDE:
	case GOG_SERIES_LABELS_NEAR_ORIGIN:
		break;
	case GOG_SERIES_LABELS_DEFAULT_POS:
	default:
		return;
	}
	if (lbls->default_pos != pos) {
		lbls->default_pos = pos;
		if ((lbls->allowed_pos & lbls->position) == 0  && lbls->position != GOG_SERIES_LABELS_DEFAULT_POS) {
			lbls->position = GOG_SERIES_LABELS_DEFAULT_POS;
			if (pos == GOG_SERIES_LABELS_CENTERED)
				lbls->offset = 0;
		}
		if (lbls->position == GOG_SERIES_LABELS_DEFAULT_POS)
			gog_object_emit_changed (gog_object_get_parent (GOG_OBJECT (lbls)), TRUE);
	}
}

GogSeriesLabelsPos
gog_series_labels_get_position (GogSeriesLabels const *lbls)
{
	return (lbls->position == GOG_SERIES_LABELS_DEFAULT_POS)?
		lbls->default_pos: lbls->position;
}

GogSeriesLabelElt const *gog_series_labels_scalar_get_element (GogSeriesLabels const *lbls)
{
	g_return_val_if_fail (GOG_IS_SERIES_LABELS (lbls), NULL);
	return lbls->elements;
}

GogSeriesLabelElt const *gog_series_labels_vector_get_element (GogSeriesLabels const *lbls, unsigned n)
{
	GogObject *obj;
	g_return_val_if_fail (GOG_IS_SERIES_LABELS (lbls) && lbls->elements != NULL, NULL);
	obj= gog_object_get_parent (GOG_OBJECT (lbls));
	if (GOG_IS_SERIES (obj)) {
		GogSeries *series = GOG_SERIES (obj);
		g_return_val_if_fail (n < gog_series_num_elements (series), NULL);
		return lbls->elements + n;
	}
	return NULL;
}
