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
#include <goffice/goffice.h>

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

static GObjectClass *data_label_parent_klass;
static GObjectClass *series_labels_parent_klass;

/**
 * GogSeriesLabelElt:
 * @str: the string to display.
 * @legend_pos: the label position.
 * @point: #GogObject
}
**/

/**
 * GogSeriesLabelsPos:
 * @GOG_SERIES_LABELS_DEFAULT_POS: default position.
 * @GOG_SERIES_LABELS_CENTERED: centered.
 * @GOG_SERIES_LABELS_TOP: at top.
 * @GOG_SERIES_LABELS_BOTTOM: at bottom.
 * @GOG_SERIES_LABELS_LEFT: at left.
 * @GOG_SERIES_LABELS_RIGHT: at right.
 * @GOG_SERIES_LABELS_OUTSIDE: outside the element.
 * @GOG_SERIES_LABELS_INSIDE: inside the element.
 * @GOG_SERIES_LABELS_NEAR_ORIGIN: near origin.
 **/

static const struct {
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

static int
gog_series_labels_get_valid_element_index (GogSeriesLabels const *lbls, int old_index, int desired_index)
{
	int index;
	GList *ptr;
	GogSeries *series = GOG_SERIES (gog_object_get_parent (GOG_OBJECT (lbls)));

	g_return_val_if_fail (GOG_IS_SERIES_LABELS (lbls), -1);

	if ((desired_index >= (int) series->num_elements) ||
	    (desired_index < 0))
		return old_index;

	if (desired_index > old_index)
		for (ptr = lbls->overrides; ptr != NULL; ptr = ptr->next) {
			index = GOG_DATA_LABEL (ptr->data)->index;
			if (index > desired_index)
				break;
			if (index == desired_index)
				desired_index++;
		}
	else
		for (ptr = g_list_last (series->overrides); ptr != NULL; ptr = ptr->prev) {
			index = GOG_DATA_LABEL (ptr->data)->index;
			if (index < desired_index)
				break;
			if (index == desired_index)
				desired_index--;
		}

	if ((desired_index >= 0) &&
	    (desired_index < (int) series->num_elements))
		return desired_index;

	return old_index;
}

#ifdef GOFFICE_WITH_GTK

struct SeriesLabelsState {
	GtkWidget *offset_btn, *offset_lbl;
	GogObject *labels;
	GtkListStore *avail_list, *used_list;
	GtkTreeSelection *avail_sel, *used_sel;
	GtkWidget *raise, *lower, *add, *remove;
};

static void
cb_index_changed (GtkSpinButton *spin_button, GogDataLabel *lbl)
{
	int index;
	int value = gtk_spin_button_get_value (spin_button);

	if ((int) lbl->index == value)
		return;

	index = gog_series_labels_get_valid_element_index (
	           GOG_SERIES_LABELS (gog_object_get_parent (GOG_OBJECT (lbl))),
		   lbl->index, value);

	if (index != value)
		gtk_spin_button_set_value (spin_button, index);

	g_object_set (lbl, "index", (int) index, NULL);
}

static void
used_selection_changed_cb (struct SeriesLabelsState *state)
{
	GtkTreeIter iter, last, first;
	GtkTreePath *f, *l;
	GtkTreeModel *model = GTK_TREE_MODEL (state->used_list);
	int count = 0;  /* we count the unselected items to avoid empty labels */
	char **format = GOG_IS_DATA_LABEL (state->labels)?
			&GOG_DATA_LABEL (state->labels)->format:
			&GOG_SERIES_LABELS (state->labels)->format;
	g_free (*format);
	*format = NULL;
	if (gtk_tree_model_get_iter_first (model, &iter)) {
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
			if (*format) /* we need to add a separator */ {
				switch (dim) {
				case -1:
					new_format = g_strconcat (*format, "%s%c", NULL);
					break;
				case -2:
					new_format = g_strconcat (*format, "%s%l", NULL);
					break;
				case -3:
					new_format = g_strconcat (*format, "%s%n", NULL);
					break;
				case -4:
					new_format = g_strconcat (*format, "%s%p", NULL);
					break;
				default:
					new_format = g_strdup_printf ("%s%%s%%%d", *format, dim);
				}
				g_free (*format);
				*format = new_format;
			} else {
				switch (dim) {
				case -1:
					*format = g_strdup ("%c");
					break;
				case -2:
					*format = g_strdup ("%l");
					break;
				case -3:
					*format = g_strdup ("%n");
					break;
				case -4:
					*format = g_strdup ("%p");
					break;
				default:
					*format = g_strdup_printf ("%%%d", dim);
				}
			}
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
	gog_object_request_update (gog_object_get_parent_typed (state->labels, GOG_TYPE_SERIES));
	gog_object_emit_changed (state->labels, TRUE);
}

static void
add_cb (G_GNUC_UNUSED GtkButton *btn, struct SeriesLabelsState *state)
{
	GtkTreeIter iter, add_iter, next;
	char *name;
	int id;
	gboolean next_selected;
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
			next = iter;
			next_selected = (gtk_tree_model_iter_next (GTK_TREE_MODEL (state->avail_list), &next)
			                 && gtk_tree_selection_iter_is_selected (state->avail_sel, &next));
			if (!gtk_list_store_remove (state->avail_list, &iter))
				break;
			if (!next_selected)
				gtk_tree_selection_unselect_iter (state->avail_sel, &iter);
		} else if (!gtk_tree_model_iter_next (GTK_TREE_MODEL (state->avail_list), &iter))
			break;
	}
	used_selection_changed_cb (state);
	gog_object_emit_changed (GOG_OBJECT (state->labels), TRUE);
}

static void
remove_cb (G_GNUC_UNUSED GtkButton *btn, struct SeriesLabelsState *state)
{
	GtkTreeIter iter, add_iter, next;
	char *name;
	int id;
	gboolean next_selected;
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
			next = iter;
			next_selected = (gtk_tree_model_iter_next (GTK_TREE_MODEL (state->used_list), &next)
			                 && gtk_tree_selection_iter_is_selected (state->used_sel, &next));
			if (!gtk_list_store_remove (state->used_list, &iter))
				break;
			if (!next_selected)
				gtk_tree_selection_unselect_iter (state->used_sel, &iter);
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
		if (GOG_IS_DATA_LABEL (state->labels)) {
			GogDataLabel *lbl = GOG_DATA_LABEL (state->labels);
			gog_data_label_set_position (lbl, pos);
			pos = gog_data_label_get_position (lbl);
		} else {
			GogSeriesLabels *lbls = GOG_SERIES_LABELS (state->labels);
			gog_series_labels_set_position (lbls, pos);
			pos = gog_series_labels_get_position (lbls);
		}
		if (pos ==  GOG_SERIES_LABELS_CENTERED) {
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
offset_changed_cb (GtkSpinButton *btn, GogObject *gobj)
{
	if (GOG_IS_DATA_LABEL (gobj))
		GOG_DATA_LABEL (gobj)->offset = gtk_spin_button_get_value_as_int (btn);
	else
		GOG_SERIES_LABELS (gobj)->offset = gtk_spin_button_get_value_as_int (btn);
	gog_object_emit_changed (gobj, TRUE);
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
	GogPlot *plot = (GogPlot *) gog_object_get_parent_typed (gobj, GOG_TYPE_PLOT);
	int i = 0, id = -1, def = 0;
	unsigned j;
	struct SeriesLabelsState *state = g_new (struct SeriesLabelsState, 1);
	GogSeriesLabelsPos position, allowed, default_pos;
	int offset;
	char *format;
	GogObjectClass *parent_class;
	char const *custom_lbl;
	gboolean supports_percent;

	gui = go_gtk_builder_load_internal ("res:go:graph/gog-series-labels-prefs.ui", GETTEXT_PACKAGE, cc);
	labels_prefs = go_gtk_builder_get_widget (gui, "series-labels-prefs");
	state->labels = gobj;

	if (GOG_IS_DATA_LABEL (gobj)) {
		GogDataLabel *lbl = GOG_DATA_LABEL (gobj);
		GtkWidget *w, *grid;
		position = lbl->position;
		allowed = lbl->allowed_pos;
		default_pos = lbl->default_pos;
		format = lbl->format;
		offset = lbl->offset;
		supports_percent = lbl->supports_percent;
		parent_class = GOG_OBJECT_CLASS (data_label_parent_klass);
		grid = gtk_grid_new ();
		gtk_grid_set_row_spacing (GTK_GRID (grid), 12);
		w = gtk_label_new (_("Index:"));
		gtk_container_add (GTK_CONTAINER (grid), w);
		w = gtk_spin_button_new_with_range (0, G_MAXINT, 1);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), lbl->index);
		g_signal_connect (G_OBJECT (w), "value_changed",
		                  G_CALLBACK (cb_index_changed), gobj);
		gtk_container_add (GTK_CONTAINER (grid), w);
		gtk_container_add (GTK_CONTAINER (labels_prefs), grid);
		custom_lbl = _("Custom label");
	} else {
		GogSeriesLabels *lbls = GOG_SERIES_LABELS (gobj);
		position = lbls->position;
		allowed = lbls->allowed_pos;
		default_pos = lbls->default_pos;
		format = lbls->format;
		offset = lbls->offset;
		supports_percent = lbls->supports_percent;
		parent_class = GOG_OBJECT_CLASS (series_labels_parent_klass);
		custom_lbl = _("Custom labels");
	}

	box = GTK_COMBO_BOX (go_gtk_builder_get_widget (gui, "position-box"));
	list = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_UINT);
	gtk_combo_box_set_model (box, GTK_TREE_MODEL (list));
	cell = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (box), cell, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (box), cell,
					"text", 0, NULL);
	for (j = 0; j < G_N_ELEMENTS (positions); j++)
		if (allowed & positions[j].pos) {
			gtk_list_store_append (list, &iter);
			gtk_list_store_set (list, &iter, 0, _(positions[j].label), 1, positions[j].pos, -1);
			if (position == positions[j].pos)
					id = i;
			if (default_pos == positions[j].pos)
				def = i;
			i++;
		}
	gtk_combo_box_set_active (box, (id >= 0)? id: def);
	g_signal_connect (G_OBJECT (box), "changed", G_CALLBACK (position_changed_cb), state);

	w = go_gtk_builder_get_widget (gui, "offset-btn");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), offset);
	g_signal_connect (G_OBJECT (w), "value-changed", G_CALLBACK (offset_changed_cb), gobj);
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
		cur = format;
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
				gtk_list_store_set (state->used_list, &iter, 0, custom_lbl, 1, -1, -1);
				dims = g_slist_prepend (dims, GINT_TO_POINTER (-1));
				break;
			case 'l':
				gtk_list_store_append (state->used_list, &iter);
				gtk_list_store_set (state->used_list, &iter, 0, _("Legend entry"), 1, -2, -1);
				dims = g_slist_prepend (dims, GINT_TO_POINTER (-2));
				break;
			case 'n':
				gtk_list_store_append (state->used_list, &iter);
				gtk_list_store_set (state->used_list, &iter, 0, _("Series name"), 1, -3, -1);
				dims = g_slist_prepend (dims, GINT_TO_POINTER (-3));
				break;
			case 'p':
				if (supports_percent) {
					gtk_list_store_append (state->used_list, &iter);
					gtk_list_store_set (state->used_list, &iter, 0, _("Values as percent"), 1, -4, -1);
					dims = g_slist_prepend (dims, GINT_TO_POINTER (-4));
				}
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
			gtk_list_store_set (state->avail_list, &iter, 0, custom_lbl, 1, -1, -1);
		}
		if (!g_slist_find (dims, GINT_TO_POINTER (-2))) {
			gtk_list_store_append (state->avail_list, &iter);
			gtk_list_store_set (state->avail_list, &iter, 0, _("Legend entry"), 1, -2, -1);
		}
		if (!g_slist_find (dims, GINT_TO_POINTER (-3))) {
			gtk_list_store_append (state->avail_list, &iter);
			gtk_list_store_set (state->avail_list, &iter, 0, _("Series name"), 1, -3, -1);
		}
		if (supports_percent && !g_slist_find (dims, GINT_TO_POINTER (-4))) {
			gtk_list_store_append (state->avail_list, &iter);
			gtk_list_store_set (state->avail_list, &iter, 0, _("Values as percent"), 1, -4, -1);
		}
		gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (state->avail_list), 1, GTK_SORT_ASCENDING);
	}
	w = GTK_WIDGET (gog_data_allocator_editor (dalloc, GOG_DATASET (gobj), 0,
	                                           GOG_IS_DATA_LABEL (gobj)? GOG_DATA_SCALAR: GOG_DATA_VECTOR));
	gtk_widget_show (w);
	gtk_grid_attach (GTK_GRID (labels_prefs), w, 2, 6, 3, 1);
	w = GTK_WIDGET (gog_data_allocator_editor (dalloc, GOG_DATASET (gobj), 1,
	                                           GOG_DATA_SCALAR));
	gtk_widget_show (w);
	gtk_grid_attach (GTK_GRID (labels_prefs), w, 2, 7, 3, 1);

	g_object_set_data_full (G_OBJECT (labels_prefs), "state", state, g_free);

	go_editor_add_page (editor,labels_prefs, _("Details"));
	gtk_widget_show_all (labels_prefs),
	g_object_unref (gui);
	parent_class->populate_editor (gobj, editor, dalloc, cc);
}
#endif

enum {
	DATA_LABEL_PROP_0,
	DATA_LABEL_PROP_POSITION,
	DATA_LABEL_PROP_OFFSET,
	DATA_LABEL_PROP_FORMAT,
	DATA_LABEL_PROP_INDEX
};

typedef GogOutlinedObjectClass GogDataLabelClass;

static void
gog_series_labels_init_style (GogStyledObject *gso, GOStyle *style)
{
	style->interesting_fields = GO_STYLE_OUTLINE | GO_STYLE_FILL |
		GO_STYLE_FONT | GO_STYLE_TEXT_LAYOUT;
	gog_theme_fillin_style (gog_object_get_theme (GOG_OBJECT (gso)),
		style, GOG_OBJECT (gso), 0, GO_STYLE_OUTLINE | GO_STYLE_FILL |
	        GO_STYLE_FONT | GO_STYLE_TEXT_LAYOUT);
}

static gint
element_compare (GogSeriesElement *gse_a, GogSeriesElement *gse_b)
{
	return gse_a->index - gse_b->index;
}

static void
gog_data_label_set_property (GObject *obj, guint param_id,
			 GValue const *value, GParamSpec *pspec)
{
	GogDataLabel *label = GOG_DATA_LABEL (obj);

	switch (param_id) {
	case DATA_LABEL_PROP_POSITION: {
		char const *name = g_value_get_string (value);
		if (!strcmp (name, "centered"))
			gog_data_label_set_position (label, GOG_SERIES_LABELS_CENTERED);
		else if (!strcmp (name, "top"))
			gog_data_label_set_position (label, GOG_SERIES_LABELS_TOP);
		else if (!strcmp (name, "bottom"))
			gog_data_label_set_position (label, GOG_SERIES_LABELS_BOTTOM);
		else if (!strcmp (name, "left"))
			gog_data_label_set_position (label, GOG_SERIES_LABELS_LEFT);
		else if (!strcmp (name, "right"))
			gog_data_label_set_position (label, GOG_SERIES_LABELS_RIGHT);
		else if (!strcmp (name, "outside"))
			gog_data_label_set_position (label, GOG_SERIES_LABELS_OUTSIDE);
		else if (!strcmp (name, "inside"))
			gog_data_label_set_position (label, GOG_SERIES_LABELS_INSIDE);
		else if (!strcmp (name, "near origin"))
			gog_data_label_set_position (label, GOG_SERIES_LABELS_NEAR_ORIGIN);
		return;
	}
	case DATA_LABEL_PROP_OFFSET: {
		unsigned offset = g_value_get_uint (value);
		if (offset == label->offset)
			return;
		label->offset = offset;
		break;
	case DATA_LABEL_PROP_FORMAT:
		g_free (label->format);
		label->format = g_value_dup_string (value);
		break;
	}
	case DATA_LABEL_PROP_INDEX:
		label->index = g_value_get_int (value);
		if (GOG_OBJECT (label)->parent != NULL) {
			GogSeriesLabels *lbls = GOG_SERIES_LABELS (GOG_OBJECT (label)->parent);
			lbls->overrides = g_list_remove (lbls->overrides, label);
			lbls->overrides = g_list_insert_sorted (lbls->overrides, label,
				(GCompareFunc) element_compare);
		}
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}

	gog_object_emit_changed (gog_object_get_parent (GOG_OBJECT (obj)), TRUE);
}

static void
gog_data_label_get_property (GObject *obj, guint param_id,
			 GValue *value, GParamSpec *pspec)
{
	GogDataLabel *label = GOG_DATA_LABEL (obj);

	switch (param_id) {
	case DATA_LABEL_PROP_POSITION: {
		char const *posname;
		switch (label->position) {
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
	case DATA_LABEL_PROP_OFFSET:
		g_value_set_uint (value, label->offset);
		break;
	case DATA_LABEL_PROP_FORMAT:
		g_value_set_string (value, label->format);
		break;
	case DATA_LABEL_PROP_INDEX:
		g_value_set_int (value, label->index);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
gog_data_label_finalize (GObject *obj)
{
	GogDataLabel *label = GOG_DATA_LABEL (obj);
	gog_dataset_finalize (GOG_DATASET (obj));
	g_free (label->format);
	g_free (label->separator);
	go_string_unref (label->element.str);
	data_label_parent_klass->finalize (obj);
}

static void
gog_data_label_changed (GogObject *obj, gboolean size)
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
gog_data_label_update (GogObject *obj)
{
	GogDataLabel *lbl = GOG_DATA_LABEL (obj);
	GogSeries *series = GOG_SERIES (gog_object_get_parent_typed (obj, GOG_TYPE_SERIES));
	GString *str = g_string_new ("");
	unsigned index;
	PangoAttrList *markup = pango_attr_list_new (), *l;
	char const *format = lbl->format;
	char *next;
	lbl->element.legend_pos = -1;
	go_string_unref (lbl->element.str);
	while (*format) {
		if (*format == '%') {
			format++;
			switch (*format) {
			case 0: /* protect from an unexpected string end */
				break;
			case 'c':
				next = GO_IS_DATA (lbl->custom_label[0].data)?
					go_data_get_scalar_string (lbl->custom_label[0].data):
					NULL;
				if (next) {
					index = str->len;
					g_string_append (str, next);
					g_free (next);
					l = go_data_get_scalar_markup (lbl->custom_label[0].data);
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
				lbl->element.legend_pos = str->len;
				g_string_append_c (str, ' '); /* this one will be replaced by the legend entry */
				break;
			}
			case 'n': {
				/* add the series name */
				if (series->values[-1].data) {
					next = go_data_get_scalar_string (series->values[-1].data);
					g_string_append (str, next);
					g_free (next);
				} else
					g_string_append (str, gog_object_get_name (GOG_OBJECT (series)));
				break;
			}
			case 'p': {
				double pc = gog_plot_get_percent_value (series->plot, series->index, lbl->index);
				if (go_finite (pc)) {
					/* Note to translators: a space might be needed before '%%' in some languages */
					/* FIXME: should the number of digits be customizable? */
					next = g_strdup_printf (_("%.1f%%"), pc);
					g_string_append (str, next);
					g_free (next);
				}
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
				next = go_data_get_vector_string (series->values[index].data, lbl->index);
				if (next) {
					g_string_append (str, next);
					g_free (next);
				}
				break;
			case '%':
				g_string_append_c (str, '%');
				break;
			case 's':
				g_string_append (str, lbl->separator);
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
	lbl->element.str = go_string_new_rich_nocopy (g_string_free (str, FALSE), -1, markup, NULL);
}

static void
gog_data_label_class_init (GObjectClass *obj_klass)
{
	GogObjectClass *gog_klass = (GogObjectClass *) obj_klass;
	GogStyledObjectClass *style_klass = (GogStyledObjectClass *) obj_klass;
	data_label_parent_klass = g_type_class_peek_parent (obj_klass);

	obj_klass->set_property	= gog_data_label_set_property;
	obj_klass->get_property	= gog_data_label_get_property;
	obj_klass->finalize	= gog_data_label_finalize;
	/* series labels do not have views, so just forward signals from the plot */
	gog_klass->use_parent_as_proxy  = TRUE;

	g_object_class_install_property (obj_klass, DATA_LABEL_PROP_POSITION,
		 g_param_spec_string ("position",
			_("Position"),
			_("Position of the label relative to the data graphic element"),
			"default",
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
        g_object_class_install_property (obj_klass, DATA_LABEL_PROP_OFFSET,
		 g_param_spec_uint ("offset",
			_("Offset"),
			_("Offset to add to the label position"),
			0, 20, 0,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
        g_object_class_install_property (obj_klass, DATA_LABEL_PROP_FORMAT,
		 g_param_spec_string ("format",
			_("Format"),
			_("Label format"),
			"",
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (obj_klass, DATA_LABEL_PROP_INDEX,
		g_param_spec_int ("index",
			_("Index"),
			_("Index of the corresponding data element"),
			0, G_MAXINT, 0,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT | GOG_PARAM_FORCE_SAVE));

#ifdef GOFFICE_WITH_GTK
	gog_klass->populate_editor = gog_series_labels_populate_editor;
#endif
	gog_klass->changed = gog_data_label_changed;
	gog_klass->update = gog_data_label_update;
	style_klass->init_style = gog_series_labels_init_style;
}

static void
gog_data_label_init (GogDataLabel *lbl)
{
	lbl->separator = g_strdup (" ");
}

static void
gog_data_label_dataset_dims (GogDataset const *set, int *first, int *last)
{
	*first = 0;
	*last = 1;
}

static GogDatasetElement *
gog_data_label_dataset_get_elem (GogDataset const *set, int dim_i)
{
	GogDataLabel const *dl = GOG_DATA_LABEL (set);
	g_return_val_if_fail (0 == dim_i || 1 == dim_i, NULL);
	return (GogDatasetElement *) &dl->custom_label + dim_i;
}

static void
gog_data_label_dataset_dim_changed (GogDataset *set, int dim_i)
{
	if (dim_i == 1) {
		GogDataLabel *dl = GOG_DATA_LABEL (set);
		g_free (dl->separator);
		dl->separator = GO_IS_DATA (dl->custom_label[1].data)?
			go_data_get_scalar_string (dl->custom_label[1].data):
			g_strdup (" ");
	}
	gog_object_emit_changed (gog_object_get_parent (GOG_OBJECT (set)), TRUE);
	gog_object_request_update (gog_object_get_parent (GOG_OBJECT (set)));
}

static void
gog_data_label_dataset_init (GogDatasetClass *iface)
{
	iface->get_elem	   = gog_data_label_dataset_get_elem;
	iface->dims	   = gog_data_label_dataset_dims;
	iface->dim_changed = gog_data_label_dataset_dim_changed;
}

GSF_CLASS_FULL (GogDataLabel, gog_data_label,
		NULL, NULL, gog_data_label_class_init, NULL,
		gog_data_label_init, GOG_TYPE_OUTLINED_OBJECT, 0,
                GSF_INTERFACE (gog_data_label_dataset_init, GOG_TYPE_DATASET))

void gog_data_label_set_allowed_position (GogDataLabel *lbl, unsigned allowed)
{
	g_return_if_fail (GOG_IS_DATA_LABEL (lbl));
	lbl->allowed_pos = allowed;
	if ((lbl->position & allowed) == 0 && lbl->position != GOG_SERIES_LABELS_DEFAULT_POS) {
		lbl->position = GOG_SERIES_LABELS_DEFAULT_POS;
		gog_object_emit_changed (gog_object_get_parent (GOG_OBJECT (lbl)), TRUE);
	}
}

void gog_data_label_set_position (GogDataLabel *lbl, GogSeriesLabelsPos pos)
{
	g_return_if_fail (GOG_IS_DATA_LABEL (lbl));
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
	if ((lbl->allowed_pos & pos) != 0  && lbl->position != pos) {
		lbl->position = (pos == lbl->default_pos)? GOG_SERIES_LABELS_DEFAULT_POS: pos;
		if (gog_data_label_get_position (lbl) == GOG_SERIES_LABELS_CENTERED)
			lbl->offset = 0;
		gog_object_emit_changed (gog_object_get_parent (GOG_OBJECT (lbl)), TRUE);
	}
}

void gog_data_label_set_default_position (GogDataLabel *lbl, GogSeriesLabelsPos pos)
{
	g_return_if_fail (GOG_IS_DATA_LABEL (lbl));
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
	if (lbl->default_pos != pos) {
		lbl->default_pos = pos;
		if ((lbl->allowed_pos & lbl->position) == 0  && lbl->position != GOG_SERIES_LABELS_DEFAULT_POS) {
			lbl->position = GOG_SERIES_LABELS_DEFAULT_POS;
			if (pos == GOG_SERIES_LABELS_CENTERED)
				lbl->offset = 0;
		}
		if (lbl->position == GOG_SERIES_LABELS_DEFAULT_POS)
			gog_object_emit_changed (gog_object_get_parent (GOG_OBJECT (lbl)), TRUE);
	}
}

GogSeriesLabelsPos gog_data_label_get_position (GogDataLabel const *lbl)
{
	return lbl->position;
}

GogSeriesLabelElt const *gog_data_label_get_element (GogDataLabel const *lbl)
{
	return &lbl->element;
}

static void
gog_data_label_set_index (GogDataLabel *lbl, int ind)
{
	lbl->index = ind;
	go_styled_object_apply_theme (GO_STYLED_OBJECT (lbl),
	                              go_styled_object_get_style (GO_STYLED_OBJECT (lbl)));
	go_styled_object_style_changed (GO_STYLED_OBJECT (lbl));
}

typedef GogOutlinedObjectClass GogSeriesLabelsClass;

enum {
	SERIES_LABELS_PROP_0,
	SERIES_LABELS_PROP_POSITION,
	SERIES_LABELS_PROP_OFFSET,
	SERIES_LABELS_PROP_FORMAT
};

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
		labels->format = g_value_dup_string (value);
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

#if 0
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
#endif

static void
gog_series_labels_changed (GogObject *obj, gboolean size)
{
	gog_object_emit_changed (gog_object_get_parent (obj), size);
	gog_object_request_update (gog_object_get_parent (obj));
}

static void
gog_series_labels_update (GogObject *obj)
{
	GogSeriesLabels *labels = GOG_SERIES_LABELS (obj);
	GogObject *parent = gog_object_get_parent (GOG_OBJECT (obj));
	unsigned i, n;
	GList *override;
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
		override = labels->overrides;
		for (i = 0; i < n; i++) {
			if (override && ((GogDataLabel *) override->data)->index == i) {
				gog_object_request_update (override->data);
				gog_object_update (override->data);
				labels->elements[i].str = go_string_ref (((GogDataLabel *) override->data)->element.str);
				labels->elements[i].legend_pos = ((GogDataLabel *) override->data)->element.legend_pos;
				labels->elements[i].point = override->data;
			} else {
				GString *str = g_string_new ("");
				unsigned index;
				PangoAttrList *markup = pango_attr_list_new (), *l;
				char const *format = labels->format;
				char *next;
				labels->elements[i].legend_pos = -1;
				while (*format) {
					if (*format == '%') {
						format++;
						switch (*format) {
						case 0: /* protect from an unexpected string end */
							break;
						case 'c':
							next = (GO_IS_DATA (labels->custom_labels[0].data) &&
							        go_data_get_vector_size (labels->custom_labels[0].data) > i)?
									go_data_get_vector_string (labels->custom_labels[0].data, i):
									NULL;
							if (next) {
								index = str->len;
								g_string_append (str, next);
								g_free (next);
								l = go_data_get_vector_markup (labels->custom_labels[0].data, i);
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
							labels->elements[i].legend_pos = str->len;
							g_string_append_c (str, ' '); /* this one will be replaced by the legend entry */
							break;
						}
						case 'n': {
							/* add the series name */
							if (series->values[-1].data) {
								next = go_data_get_scalar_string (series->values[-1].data);
								g_string_append (str, next);
								g_free (next);
							} else
								g_string_append (str, gog_object_get_name (GOG_OBJECT (series)));
							break;
						}
						case 'p': {
							double pc = gog_plot_get_percent_value (series->plot, series->index, i);
							if (go_finite (pc)) {
								/* Note to translators: a space might be needed before '%%' in someo_is_finie languages */
								/* FIXME: should the number of digits be customizable? */
								next = g_strdup_printf (_("%.1f%%"), pc);
								g_string_append (str, next); /* this one will be replaced by the legend entry */
								g_free (next);
							}
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
						case 's':
							g_string_append (str, labels->separator);
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
				labels->elements[i].str = go_string_new_rich_nocopy (g_string_free (str, FALSE), -1, markup, NULL);
			}
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
	g_free (labels->separator);
	if (labels->elements) {
		unsigned i, n = labels->n_elts;
		for (i = 0; i < n; i++)
			go_string_unref (labels->elements[i].str);
		g_free (labels->elements);
	}
	g_list_free (labels->overrides);
	series_labels_parent_klass->finalize (obj);
}

static gboolean
role_data_label_can_add (GogObject const *parent)
{
	return gog_series_labels_get_valid_element_index(GOG_SERIES_LABELS (parent), -1, 0) >= 0;
}

static GogObject *
role_data_label_allocate (GogObject *lbls)
{
	GogObject *gse = g_object_new (GOG_TYPE_DATA_LABEL, NULL);

	if (gse != NULL)
		gog_data_label_set_index (GOG_DATA_LABEL (gse),
			gog_series_labels_get_valid_element_index (GOG_SERIES_LABELS (lbls), -1, 0));
	return gse;
}

static void
role_data_label_post_add (GogObject *parent, GogObject *child)
{
	GogSeriesLabels *lbls = GOG_SERIES_LABELS (parent);
	GogDataLabel *lbl = GOG_DATA_LABEL (child);
	GogPlot *plot;
	unsigned j;
	go_styled_object_set_style (GO_STYLED_OBJECT (child),
		go_styled_object_get_style (GO_STYLED_OBJECT (parent)));
	lbls->overrides = g_list_insert_sorted (lbls->overrides, child,
		(GCompareFunc) element_compare);

	lbl->format = g_strdup ("");
	lbl->default_pos = lbls->default_pos;
	lbl->allowed_pos = lbls->allowed_pos;
	lbl->position = lbls->position;
	lbl->supports_percent = lbls->supports_percent;

	plot = (GogPlot *) gog_object_get_parent_typed (child, GOG_TYPE_PLOT);
	for (j = 0; j < plot->desc.series.num_dim; j++) {
		/* FIXME, this might depend upon the series type */
		switch (plot->desc.series.dim[j].ms_type) {
		case GOG_MS_DIM_VALUES:
			g_free (lbl->format);
			lbl->format = g_strdup_printf ("%%%u", j);
			j = plot->desc.series.num_dim; /* ensure we exit the loop */
			break;
		default:
			break;
		}
	}
}

static void
role_data_label_pre_remove (GogObject *parent, GogObject *child)
{
	GogSeriesLabels *lbls = GOG_SERIES_LABELS (parent);
	lbls->overrides = g_list_remove (lbls->overrides, child);
}

static void
gog_series_labels_class_init (GObjectClass *obj_klass)
{
	static GogObjectRole const roles[] = {
		{ N_("Point"), "GogDataLabel",	0,
		  GOG_POSITION_SPECIAL, GOG_POSITION_SPECIAL, GOG_OBJECT_NAME_BY_ROLE,
		  role_data_label_can_add,
		  NULL,
		  role_data_label_allocate,
		  role_data_label_post_add,
		  role_data_label_pre_remove,
		  NULL }
	};
	GogObjectClass *gog_klass = (GogObjectClass *) obj_klass;
	GogStyledObjectClass *style_klass = (GogStyledObjectClass *) obj_klass;
	series_labels_parent_klass = g_type_class_peek_parent (obj_klass);

	obj_klass->set_property	= gog_series_labels_set_property;
	obj_klass->get_property	= gog_series_labels_get_property;
	obj_klass->finalize	= gog_series_labels_finalize;
	/* series labels do not have views, so just forward signals from the plot */
	gog_klass->use_parent_as_proxy  = TRUE;

	gog_object_register_roles (gog_klass, roles, G_N_ELEMENTS (roles));

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
			0, 20, 0,
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
/*	gog_klass->parent_changed = gog_series_labels_parent_changed;*/
	gog_klass->changed = gog_series_labels_changed;
	gog_klass->update = gog_series_labels_update;
	style_klass->init_style = gog_series_labels_init_style;
}

static void
gog_series_labels_init (GogSeriesLabels *lbls)
{
	lbls->separator = g_strdup (" ");
}

static void
gog_series_labels_dataset_dims (GogDataset const *set, int *first, int *last)
{
	*first = 0;
	*last = 1;
}

static GogDatasetElement *
gog_series_labels_dataset_get_elem (GogDataset const *set, int dim_i)
{
	GogSeriesLabels const *sl = GOG_SERIES_LABELS (set);
	g_return_val_if_fail (0 == dim_i || 1 == dim_i, NULL);
	return (GogDatasetElement *) &sl->custom_labels + dim_i;
}

static void
gog_series_labels_dataset_dim_changed (GogDataset *set, int dim_i)
{
	if (dim_i == 1) {
		GogSeriesLabels *sl = GOG_SERIES_LABELS (set);
		g_free (sl->separator);
		sl->separator = GO_IS_DATA (sl->custom_labels[1].data)?
			go_data_get_scalar_string (sl->custom_labels[1].data):
			g_strdup (" ");
	}
	gog_object_emit_changed (gog_object_get_parent (GOG_OBJECT (set)), TRUE);
	gog_object_request_update (GOG_OBJECT (set));
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
		gog_series_labels_init, GOG_TYPE_OUTLINED_OBJECT, 0,
                GSF_INTERFACE (gog_series_labels_dataset_init, GOG_TYPE_DATASET))

void
gog_series_labels_set_allowed_position (GogSeriesLabels *lbls, unsigned allowed)
{
	g_return_if_fail (GOG_IS_SERIES_LABELS (lbls));
	lbls->allowed_pos = allowed;
	if ((lbls->position & allowed) == 0 && lbls->position != GOG_SERIES_LABELS_DEFAULT_POS) {
		lbls->position = GOG_SERIES_LABELS_DEFAULT_POS;
		gog_object_emit_changed (gog_object_get_parent (GOG_OBJECT (lbls)), TRUE);
	}
}

void
gog_series_labels_set_position (GogSeriesLabels *lbls, GogSeriesLabelsPos pos)
{
	g_return_if_fail (GOG_IS_SERIES_LABELS (lbls));
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
	g_return_if_fail (GOG_IS_SERIES_LABELS (lbls));
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
	g_return_val_if_fail (GOG_IS_SERIES_LABELS (lbls), GOG_SERIES_LABELS_DEFAULT_POS);
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

/************************************************************
 * The following functions are just there for introspection *
 ************************************************************/

static GogSeriesLabelElt *
gog_series_label_elt_copy (GogSeriesLabelElt *elt)
{
	GogSeriesLabelElt *res = g_new (GogSeriesLabelElt, 1);
	res->str = go_string_ref (elt->str);
	res->legend_pos = elt->legend_pos;
	res->point = (GogObject *) g_object_ref (elt->point);
	return res;
}

static void
gog_series_label_elt_free (GogSeriesLabelElt *elt)
{
	go_string_unref (elt->str);
	g_object_unref (elt->point);
	g_free (elt);
}

GType
gog_series_label_elt_get_type (void)
{
    static GType type = 0;

    if (type == 0)
		type = g_boxed_type_register_static
			("GogSeriesLabelElt",
			 (GBoxedCopyFunc) gog_series_label_elt_copy,
			 (GBoxedFreeFunc) gog_series_label_elt_free);

    return type;
}
