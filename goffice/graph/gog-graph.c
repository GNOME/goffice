/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-graph.c :
 *
 * Copyright (C) 2003-2004 Jody Goldberg (jody@gnome.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
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
#include <goffice/app/go-doc.h>
#include <goffice/graph/gog-graph-impl.h>
#include <goffice/graph/gog-chart-impl.h>
#include <goffice/graph/gog-renderer.h>
#include <goffice/utils/go-style.h>
#include <goffice/graph/gog-theme.h>
#include <goffice/data/go-data.h>
#include <goffice/utils/go-persist.h>
#include <goffice/utils/go-styled-object.h>
#include <goffice/utils/go-units.h>

#include <gsf/gsf-impl-utils.h>

#include <glib/gi18n-lib.h>
#include <string.h>
#include <stdlib.h>

#ifdef GOFFICE_WITH_GTK
#include <gtk/gtk.h>
#endif

#define GOG_GRAPH_DEFAULT_WIDTH 	GO_CM_TO_PT (12.0)
#define GOG_GRAPH_DEFAULT_HEIGHT 	GO_CM_TO_PT (8.0)

/**
 * SECTION: gog-graph
 * @short_description: The graph object.
 *
 * A graph (in the abstract sense) in GOffice is an hierarical object model,
 * with a #GogGraph object as the top-level object.
 * Objects that can be part of a graph is a subclass of #GogObject. Those, and
 * related objects have the prefix "Gog" in the class name.
 * See #GogObject for how to manipulate the object model, and the individual
 * classes for specifics.
 *
 * A #GogGraph can have 1 or more children in the roles "Chart" and "Title".
 */

enum {
	GRAPH_PROP_0,
	GRAPH_PROP_THEME,
	GRAPH_PROP_THEME_NAME,
	GRAPH_PROP_WIDTH,
	GRAPH_PROP_HEIGHT,
	GRAPH_PROP_DOCUMENT
};

enum {
	GRAPH_ADD_DATA,
	GRAPH_REMOVE_DATA,
	GRAPH_LAST_SIGNAL
};

static gulong gog_graph_signals [GRAPH_LAST_SIGNAL] = { 0, };
static GObjectClass *graph_parent_klass;
static GogViewClass *gview_parent_klass;

static void apply_theme (GogObject *object, GogTheme const *theme, gboolean force_auto);

static void
gog_graph_set_property (GObject *obj, guint param_id,
			GValue const *value, GParamSpec *pspec)
{
	GogGraph *graph = GOG_GRAPH (obj);

	switch (param_id) {
	case GRAPH_PROP_THEME :
		gog_graph_set_theme (graph, g_value_get_object (value));
		break;
	case GRAPH_PROP_THEME_NAME :
		gog_graph_set_theme (graph,
			gog_theme_registry_lookup (g_value_get_string (value)));
		break;
	case GRAPH_PROP_WIDTH:
		gog_graph_set_size (graph, g_value_get_double (value),
				    graph->height);
		break;
	case GRAPH_PROP_HEIGHT:
		gog_graph_set_size (graph, graph->width,
				    g_value_get_double (value));
		break;
	case GRAPH_PROP_DOCUMENT: {
		GObject *object = g_value_get_object (value);
		if ((GODoc *) object == graph->doc)
			break;
		if (GO_IS_DOC (object)) {
			graph->doc = (GODoc *) object;
			gog_object_document_changed (GOG_OBJECT (graph), graph->doc);
		}
		break;
	}

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
}

static void
gog_graph_get_property (GObject *obj, guint param_id,
			GValue *value, GParamSpec *pspec)
{
	GogGraph *graph = GOG_GRAPH (obj);

	switch (param_id) {
	case GRAPH_PROP_THEME :
		g_value_set_object (value, graph->theme);
		break;
	case GRAPH_PROP_THEME_NAME :
		g_value_set_string (value, gog_theme_get_name (graph->theme));
		break;
	case GRAPH_PROP_WIDTH:
		g_value_set_double (value, graph->width);
		break;
	case GRAPH_PROP_HEIGHT:
		g_value_set_double (value, graph->height);
		break;
	case GRAPH_PROP_DOCUMENT:
		g_value_set_object (value, graph->doc);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
gog_graph_finalize (GObject *obj)
{
	GogGraph *graph = GOG_GRAPH (obj);
	GSList *tmp;

	tmp = graph->data;
	graph->data = NULL;
	g_slist_foreach (tmp, (GFunc) g_object_unref, NULL);
	g_slist_free (tmp);

	/* on exit the role remove routines are not called */
	g_slist_free (graph->charts);

	if (graph->idle_handler != 0) {
		g_source_remove (graph->idle_handler);
		graph->idle_handler = 0;
	}

	g_hash_table_unref (graph->data_refs);

	(graph_parent_klass->finalize) (obj);
}

static char const *
gog_graph_type_name (GogObject const *gobj)
{
	return N_("Graph");
}

#ifdef GOFFICE_WITH_GTK

static void
cb_theme_changed (GtkComboBox *combo, GogGraph *graph)
{
	GtkTreeIter iter;

	if (gtk_combo_box_get_active_iter (combo, &iter)) {
		GtkTreeModel *model = GTK_TREE_MODEL (gtk_combo_box_get_model (GTK_COMBO_BOX (combo)));
		GogTheme *theme = NULL;
		gtk_tree_model_get (model, &iter, 1, &theme, -1);
		if (theme != NULL) {
			gog_graph_set_theme (graph, theme);
			g_object_unref (theme);
		}
	}
}

static void
cb_force_theme (GtkButton *button, GogGraph *graph)
{
	apply_theme (GOG_OBJECT (graph), graph->theme, TRUE);
}

static void
gog_graph_populate_editor (GogObject *gobj,
			   GOEditor *editor,
			   G_GNUC_UNUSED GogDataAllocator *dalloc,
			   GOCmdContext *cc)
{
	GogGraph *graph = GOG_GRAPH (gobj);
	GtkBuilder *gui;
	GSList *theme_names;
	static guint graph_pref_page = 0;

	gui = go_gtk_builder_new ("gog-graph-prefs.ui", GETTEXT_PACKAGE, cc);
	if (gui == NULL)
		return;

	(GOG_OBJECT_CLASS(graph_parent_klass)->populate_editor) (gobj, editor, dalloc, cc);

	theme_names = gog_theme_registry_get_theme_names ();

	if (theme_names != NULL) {
		GtkWidget *box;
		GtkWidget *combo;
		GSList *ptr;
		GtkListStore *model;
		GtkTreeIter iter;
		GogTheme *theme;
		char const *graph_theme_name;
		int count, index = 0;

		graph_theme_name = gog_theme_get_name (graph->theme);
		combo = go_gtk_builder_get_widget (gui, "theme_combo");
		model = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (combo)));

		count = 0;
		for (ptr = theme_names; ptr != NULL; ptr = ptr->next) {
			theme = gog_theme_registry_lookup (ptr->data);
			gtk_list_store_append (model, &iter);
			gtk_list_store_set (model, &iter,
			                    0, gog_theme_get_local_name (theme),
			                    1, theme,
			                    -1);
			if (strcmp (ptr->data, graph_theme_name) == 0)
				index = count;
			count++;
		}
		gtk_combo_box_set_active (GTK_COMBO_BOX (combo), index);

		g_signal_connect (G_OBJECT (combo), "changed", G_CALLBACK (cb_theme_changed), graph);
		g_signal_connect (gtk_builder_get_object (gui, "force_theme_button"), "clicked",
				  G_CALLBACK (cb_force_theme), graph);

		box = go_gtk_builder_get_widget (gui, "gog_graph_prefs");
		go_editor_add_page (editor, box, _("Theme"));
		g_object_unref (gui);

		g_slist_free (theme_names);
	}

	go_editor_set_store_page (editor, &graph_pref_page);
}
#endif

static void
role_chart_post_add (GogObject *parent, GogObject *chart)
{
	GogGraph *graph = GOG_GRAPH (parent);
	unsigned ypos = 0;
	if (graph->charts != NULL) {
		/* find the first row with an unoccupied first column */
		gboolean *occ = g_alloca (graph->num_rows * sizeof (gboolean));
		GSList *ptr;
		int col, row;
		memset (occ, 0, graph->num_rows * sizeof (gboolean));
		for (ptr = graph->charts; ptr != NULL; ptr = ptr->next)
			if (gog_chart_get_position (GOG_CHART (ptr->data), &col, &row, NULL, NULL) && col == 0 && row < (int) graph->num_rows)
				occ[row] = TRUE;
		while (ypos < graph->num_rows && occ[ypos])
			ypos++;
	}
	graph->charts = g_slist_prepend (graph->charts, chart);
	if (!gog_chart_get_position (GOG_CHART (chart), NULL, NULL, NULL, NULL))
		/* search the first row with column 0 unocupied */
		gog_chart_set_position (GOG_CHART (chart),
			0, ypos, 1, 1);
}

static void
role_chart_pre_remove (GogObject *parent, GogObject *child)
{
	GogGraph *graph = GOG_GRAPH (parent);
	GogChart *chart = GOG_CHART (child);

	graph->charts = g_slist_remove (graph->charts, chart);
	gog_graph_validate_chart_layout (graph);
}

static void
gog_graph_update (GogObject *obj)
{
	GogGraph *graph = GOG_GRAPH (obj);
	if (graph->idle_handler != 0) {
		g_source_remove (graph->idle_handler);
		graph->idle_handler = 0;
	}
}

static void
gog_graph_class_init (GogGraphClass *klass)
{
	GObjectClass *gobject_klass   = (GObjectClass *) klass;
	GogObjectClass *gog_klass = (GogObjectClass *) klass;

	static GogObjectRole const roles[] = {
		{ N_("Chart"), "GogChart",	1,
		  GOG_POSITION_SPECIAL|GOG_POSITION_ANY_MANUAL,
		  GOG_POSITION_SPECIAL,
		  GOG_OBJECT_NAME_BY_ROLE,
		  NULL, NULL, NULL, role_chart_post_add, role_chart_pre_remove, NULL },
		{ N_("Title"), "GogLabel",	0,
		  GOG_POSITION_COMPASS|GOG_POSITION_ANY_MANUAL,
		  GOG_POSITION_N|GOG_POSITION_ALIGN_CENTER,
		  GOG_OBJECT_NAME_BY_ROLE,
		  NULL, NULL, NULL, NULL, NULL, NULL },
	};

	graph_parent_klass = g_type_class_peek_parent (klass);
	gobject_klass->set_property = gog_graph_set_property;
	gobject_klass->get_property = gog_graph_get_property;
	gobject_klass->finalize	    = gog_graph_finalize;

	gog_klass->update	    = gog_graph_update;
	gog_klass->type_name	    = gog_graph_type_name;
	gog_klass->view_type	    = gog_graph_view_get_type ();
#ifdef GOFFICE_WITH_GTK
	gog_klass->populate_editor  = gog_graph_populate_editor;
#endif

	gog_object_register_roles (gog_klass, roles, G_N_ELEMENTS (roles));

	/**
	 * GogGraph::add-data:
	 * @graph: the object on which the signal is emitted
	 * @data: The new #GOData being added to @graph
	 *
	 * The ::add-data signal is emitted BEFORE @data has been added.
	 **/
	gog_graph_signals [GRAPH_ADD_DATA] = g_signal_new ("add-data",
		G_TYPE_FROM_CLASS (klass),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GogGraphClass, add_data),
		NULL, NULL,
		g_cclosure_marshal_VOID__OBJECT,
		G_TYPE_NONE,
		1, G_TYPE_OBJECT);

	/**
	 * GogGraph::remove-data:
	 * @graph: the object on which the signal is emitted
	 * @data: The new #GOData being removed to @graph
	 *
	 * The ::remove-data signal is emitted BEFORE @data has been removed.
	 **/
	gog_graph_signals [GRAPH_REMOVE_DATA] = g_signal_new ("remove-data",
		G_TYPE_FROM_CLASS (klass),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GogGraphClass, remove_data),
		NULL, NULL,
		g_cclosure_marshal_VOID__OBJECT,
		G_TYPE_NONE,
		1, G_TYPE_OBJECT);

	g_object_class_install_property (gobject_klass, GRAPH_PROP_THEME,
		g_param_spec_object ("theme",
			_("Theme"),
			_("The theme for elements of the graph"),
			GOG_TYPE_THEME,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (gobject_klass, GRAPH_PROP_THEME_NAME,
		g_param_spec_string ("theme-name",
			_("Theme name"),
			_("The name of the theme for elements of the graph"),
			"default",
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, GRAPH_PROP_WIDTH,
		g_param_spec_double ("width-pts",
			_("Width"),
			_("Logical graph width, in points"),
			0.0, G_MAXDOUBLE, GOG_GRAPH_DEFAULT_WIDTH,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, GRAPH_PROP_HEIGHT,
		g_param_spec_double ("height-pts",
			_("Height"),
			_("Logical graph height, in points"),
			0.0, G_MAXDOUBLE, GOG_GRAPH_DEFAULT_HEIGHT,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, GRAPH_PROP_DOCUMENT,
		g_param_spec_object ("document",
			_("Document"),
			_("the document for this graph"),
			GO_TYPE_DOC,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
}

static void
gog_graph_init (GogGraph *graph)
{
	GogStyledObject *gso = GOG_STYLED_OBJECT (graph);

	graph->data = NULL;
	graph->num_cols = graph->num_rows = 0;
	graph->width = GOG_GRAPH_DEFAULT_WIDTH;
	graph->height = GOG_GRAPH_DEFAULT_HEIGHT;
	graph->idle_handler = 0;
	graph->theme = gog_theme_registry_lookup (NULL); /* default */

	/* Cheat and assign a name here, graphs will not have parents until we
	 * support graphs in graphs */
	GOG_OBJECT (graph)->user_name = g_strdup (_("Graph"));
	gog_theme_fillin_style (graph->theme,
		gso->style, GOG_OBJECT (graph), 0, GO_STYLE_FILL | GO_STYLE_OUTLINE);
	go_styled_object_apply_theme (GO_STYLED_OBJECT (gso), gso->style);

	graph->data_refs = g_hash_table_new (g_direct_hash, g_direct_equal);
}

GSF_CLASS (GogGraph, gog_graph,
	   gog_graph_class_init, gog_graph_init,
	   GOG_TYPE_OUTLINED_OBJECT)

/**
 * gog_graph_validate_chart_layout :
 * @graph : #GogGraph
 *
 * Check the layout of the chart grid and ensure that there are no empty
 * cols or rows, and resize as necessary
 *
 * Returns: %TRUE if anything changed.
 **/
gboolean
gog_graph_validate_chart_layout (GogGraph *graph)
{
	GSList *ptr;
	GogChart *chart = NULL;
	unsigned i, max_col, max_row;
	gboolean changed = FALSE;

	g_return_val_if_fail (GOG_IS_GRAPH (graph), FALSE);

	/* There won't be many of charts so we do the right thing */

	/* 1) find the max */
	max_col = max_row = 0;
	for (ptr = graph->charts ; ptr != NULL ; ptr = ptr->next) {
		if (gog_object_get_position_flags (ptr->data, GOG_POSITION_MANUAL))
			continue;
		chart = ptr->data;
		chart->x_pos_actual = chart->x_pos;
		chart->y_pos_actual = chart->y_pos;
		if (max_col < (chart->x_pos_actual + chart->cols))
			max_col = (chart->x_pos_actual + chart->cols);
		if (max_row < (chart->y_pos_actual + chart->rows))
			max_row = (chart->y_pos_actual + chart->rows);
	}

	/* 2) see if we need to contract any cols */
	for (i = 0 ; i < max_col ; ) {
		for (ptr = graph->charts ; ptr != NULL ; ptr = ptr->next) {
			if (gog_object_get_position_flags (ptr->data, GOG_POSITION_MANUAL))
				continue;
			chart = ptr->data;
			if (chart->x_pos_actual <= i && i < (chart->x_pos_actual + chart->cols))
				break;
		}
		if (ptr == NULL) {
			changed = TRUE;
			max_col--;
			for (ptr = graph->charts ; ptr != NULL ; ptr = ptr->next) {
				chart = ptr->data;
				if (chart->x_pos_actual > i)
					(chart->x_pos_actual)--;
			}
		} else
			i = chart->x_pos_actual + chart->cols;
	}

	/* 3) see if we need to contract any rows */
	for (i = 0 ; i < max_row ; ) {
		for (ptr = graph->charts ; ptr != NULL ; ptr = ptr->next) {
			if (gog_object_get_position_flags (ptr->data, GOG_POSITION_MANUAL))
				continue;
			chart = ptr->data;
			if (chart->y_pos_actual <= i && i < (chart->y_pos_actual + chart->rows))
				break;
		}
		if (ptr == NULL) {
			changed = TRUE;
			max_row--;
			for (ptr = graph->charts ; ptr != NULL ; ptr = ptr->next) {
				chart = ptr->data;
				if (chart->y_pos_actual > i)
					(chart->y_pos_actual)--;
			}
		} else
			i = chart->y_pos_actual + chart->rows;
	}
	changed |= (graph->num_cols != max_col || graph->num_rows != max_row);

	graph->num_cols = max_col;
	graph->num_rows = max_row;

	if (changed)
		gog_object_emit_changed (GOG_OBJECT (graph), TRUE);
	return changed;
}

unsigned
gog_graph_num_cols (GogGraph const *graph)
{
	g_return_val_if_fail (GOG_IS_GRAPH (graph), 1);
	return graph->num_cols;
}

unsigned
gog_graph_num_rows (GogGraph const *graph)
{
	g_return_val_if_fail (GOG_IS_GRAPH (graph), 1);
	return graph->num_rows;
}

/**
 * gog_graph_dup :
 * @graph : #GogGraph
 *
 * Returns: a deep copy of @graph.
 **/
GogGraph *
gog_graph_dup (GogGraph const *graph)
{
	GogObject *res = gog_object_dup (GOG_OBJECT (graph), NULL, NULL);
	GOG_GRAPH (res)->doc = graph->doc;
	return GOG_GRAPH (res);
}

GogTheme *
gog_graph_get_theme (GogGraph const *graph)
{
	g_return_val_if_fail (GOG_IS_GRAPH (graph), NULL);
	return graph->theme;
}

static void
apply_theme (GogObject *object, GogTheme const *theme, gboolean force_auto)
{
	GSList *ptr;

	for (ptr = object->children; ptr !=  NULL; ptr = ptr->next)
		apply_theme (ptr->data, theme, force_auto);

	if (GO_IS_STYLED_OBJECT (object)) {
		GOStyledObject *styled_object = (GOStyledObject *) object;
		GOStyle *style;

		style = go_styled_object_get_style (styled_object);
		if (force_auto)
			/* FIXME: Some style settings are not themed yet,
			 * such as font or fill type. */
			go_style_force_auto (style);
		go_styled_object_apply_theme (styled_object, style);
		go_styled_object_style_changed (styled_object);
		gog_object_emit_changed (object, TRUE);
	}
}

void
gog_graph_set_theme (GogGraph *graph, GogTheme *theme)
{
	g_return_if_fail (GOG_IS_GRAPH (graph));
	g_return_if_fail (GOG_IS_THEME (theme));

	graph->theme = theme;

	apply_theme (GOG_OBJECT (graph), graph->theme, FALSE);
}

/**
 * gog_graph_get_data :
 * @graph: #GogGraph
 *
 * Returns: a list of the GOData objects that are data to the graph.
 * The caller should _not_ modify or free the list.
 **/
GSList *
gog_graph_get_data (GogGraph const *graph)
{
	g_return_val_if_fail (GOG_IS_GRAPH (graph), NULL);
	return graph->data;
}

/**
 * gog_graph_ref_data :
 * @graph : #GogGraph
 * @dat : #GOData
 *
 * Returns: @dat or something equivalent to it that already exists in the graph.
 * 	Otherwise use @dat.  Adds a gobject ref to the target and increments a
 *	 count of the number of refs made from this #GogGraph.
 **/
GOData *
gog_graph_ref_data (GogGraph *graph, GOData *dat)
{
	gpointer res;
	unsigned count;

	if (dat == NULL)
		return NULL;

	g_return_val_if_fail (GOG_IS_GRAPH (graph), dat);
	g_return_val_if_fail (GO_IS_DATA (dat), dat);

	/* Does it already exist in the graph ? */
	res = g_hash_table_lookup (graph->data_refs, dat);
	if (res == NULL) {

		/* is there something like it already */
		GSList *existing = graph->data;
		for (; existing != NULL ; existing = existing->next)
			if (go_data_eq (dat, existing->data))
				break;

		if (existing == NULL) {
			g_signal_emit (graph, gog_graph_signals [GRAPH_ADD_DATA], 0, dat);
			graph->data = g_slist_prepend (graph->data, dat);
			g_object_ref (dat);
		} else {
			dat = existing->data;
			res = g_hash_table_lookup (graph->data_refs, dat);
		}
	}

	count = GPOINTER_TO_UINT (res) + 1;
	if (res)
		g_hash_table_replace (graph->data_refs, dat, GUINT_TO_POINTER (count));
	else
		g_hash_table_insert (graph->data_refs, dat, GUINT_TO_POINTER (count));
	g_object_ref (dat);
	return dat;
}

/**
 * gog_graph_unref_data :
 * @graph : #GogGraph
 * @dat : #GOData
 *
 **/
void
gog_graph_unref_data (GogGraph *graph, GOData *dat)
{
	gpointer res;
	unsigned count;

	if (dat == NULL)
		return;

	g_return_if_fail (GO_IS_DATA (dat));

	g_object_unref (dat);

	if (graph == NULL)
		return;

	g_return_if_fail (GOG_IS_GRAPH (graph));

	/* once we've been destroyed the list is gone */
	if (graph->data == NULL)
		return;

	res = g_hash_table_lookup (graph->data_refs, dat);

	g_return_if_fail (res != NULL);

	count = GPOINTER_TO_UINT (res);
	if (count-- <= 1) {
		/* signal before removing in case that unrefs */
		g_signal_emit (G_OBJECT (graph),
			gog_graph_signals [GRAPH_REMOVE_DATA], 0, dat);
		graph->data = g_slist_remove (graph->data, dat);
		g_object_unref (dat);
		g_hash_table_remove (graph->data_refs, dat);
	} else
		/* store the decremented count */
		g_hash_table_replace (graph->data_refs, dat, GUINT_TO_POINTER (count));
}

static gboolean
cb_graph_idle (GogGraph *graph)
{
	/* an update may queue an update in a different object,
	 * clear the handler early */
	graph->idle_handler = 0;
	gog_object_update (GOG_OBJECT (graph));
	return FALSE;
}

/**
 * gog_graph_request_update :
 * @graph : #GogGraph
 *
 * queue an update if one had not already be queued.
 *
 * Returns: %TRUE if a handler has been added.
 **/
gboolean
gog_graph_request_update (GogGraph *graph)
{
	/* people may try to queue an update during destruction */
	if (G_OBJECT (graph)->ref_count <= 0)
		return FALSE;

	g_return_val_if_fail (GOG_IS_GRAPH (graph), FALSE);

	if (graph->idle_handler == 0) { /* higher priority than canvas */
		graph->idle_handler = g_idle_add_full (G_PRIORITY_HIGH_IDLE,
			(GSourceFunc) cb_graph_idle, graph, NULL);
		return TRUE;
	}
	return FALSE;
}

/**
 * gog_graph_force_update :
 * @graph : #GogGraph
 *
 * Do an update now if one has been queued.
 **/
void
gog_graph_force_update (GogGraph *graph)
{
	/* people may try to queue an update during destruction */
	while (G_OBJECT (graph)->ref_count > 0 && graph->idle_handler != 0) {
		g_source_remove (graph->idle_handler);
		graph->idle_handler = 0;
		gog_object_update (GOG_OBJECT (graph));
	}
}
/**
 * gog_graph_get_size :
 * @graph: #GogGraph
 * @width: logical width in pts
 * @height: logical height in pts
 *
 * FIXME Returns the logical size of graph, in points.
 **/
void
gog_graph_get_size (GogGraph *graph, double *width, double *height)
{
	g_return_if_fail (GOG_IS_GRAPH (graph));

	if (width != NULL)
		*width = graph->width;
	if (height != NULL)
		*height = graph->height;
}
/**
 * gog_graph_set_size:
 * @graph: #GogGraph
 * @width: logical width in pts
 * @height: logical height in pts
 *
 * Sets the logical size of graph, given in points.
 **/
void
gog_graph_set_size (GogGraph *graph, double width, double height)
{
	g_return_if_fail (GOG_IS_GRAPH (graph));

	if (width != graph->width || height != graph->height) {
		graph->height = height;
		graph->width = width;
		gog_object_emit_changed (GOG_OBJECT (graph), TRUE);
	}
}

/************************************************************************/

enum {
	GRAPH_VIEW_SELECTION_CHANGED,
	GRAPH_VIEW_LAST_SIGNAL
};

static gulong gog_graph_view_signals [GRAPH_VIEW_LAST_SIGNAL] = { 0, };

struct _GogGraphView {
	GogOutlinedView		 base;

	GogToolAction		*action;
	GogObject		*selected_object;
	GogView			*selected_view;
#ifdef GOFFICE_WITH_GTK
	GdkCursorType		 cursor_type;
#endif
};

typedef struct {
	GogOutlinedViewClass	base;

	/* signals */
	void (*selection_changed) (GogGraphView *view, GogObject *gobj);
} GogGraphViewClass;

#define GOG_TYPE_GRAPH_VIEW	(gog_graph_view_get_type ())
#define GOG_GRAPH_VIEW(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_GRAPH_VIEW, GogGraphView))
#define GOG_IS_GRAPH_VIEW(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_GRAPH_VIEW))

enum {
	GRAPH_VIEW_PROP_0,
	GRAPH_VIEW_PROP_RENDERER
};

static void
gog_graph_view_set_property (GObject *gobject, guint param_id,
			     GValue const *value, GParamSpec *pspec)
{
	GogView *view = GOG_VIEW (gobject);

	switch (param_id) {
	case GRAPH_VIEW_PROP_RENDERER:
		g_return_if_fail (view->renderer == NULL);
		view->renderer = GOG_RENDERER (g_value_get_object (value));
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
}

static void
gog_graph_view_size_allocate (GogView *view, GogViewAllocation const *bbox)
{
	GSList *ptr;
	double h, w;
	GogView *child;
	GogGraph *graph = GOG_GRAPH (view->model);
	GogViewAllocation tmp, res = *bbox;

	(gview_parent_klass->size_allocate) (view, &res);

	if (gog_graph_num_cols (graph) <= 0 ||
	    gog_graph_num_rows (graph) <= 0)
		return;

	res = view->residual;
	w = res.w / gog_graph_num_cols (graph);
	h = res.h / gog_graph_num_rows (graph);
	for (ptr = view->children; ptr != NULL ; ptr = ptr->next) {
		child = ptr->data;
		if (GOG_POSITION_IS_SPECIAL (child->model->position)) {
			GogChart *chart = GOG_CHART (child->model);
			tmp.x = chart->x_pos_actual * w + res.x;
			tmp.y = chart->y_pos_actual * h + res.y;
			tmp.w = chart->cols * w;
			tmp.h = chart->rows * h;
			gog_view_size_allocate (child, &tmp);
		}
	}
}

static void
gog_graph_view_render  (GogView *view, GogViewAllocation const *bbox)
{
	GogGraphView *gview = GOG_GRAPH_VIEW (view);
	GSList *ptr;
	GogView *child_view;

	gview_parent_klass->render (view, bbox);

	/* render everything except labels */
	for (ptr = view->children ; ptr != NULL ; ptr = ptr->next) {
		child_view = ptr->data;
		if (GOG_IS_LABEL (child_view->model))
			continue;
		gog_view_render	(ptr->data, bbox);
	}
	/* render labels */
	for (ptr = view->children ; ptr != NULL ; ptr = ptr->next) {
		child_view = ptr->data;
		if (!GOG_IS_LABEL (child_view->model))
			continue;
		gog_view_render	(ptr->data, bbox);
	}

	if (gview->selected_view != NULL)
		gog_view_render_toolkit (gview->selected_view);
}

static void
gog_graph_view_finalize (GObject *obj)
{
	GogGraphView *gview = GOG_GRAPH_VIEW (obj);

	if (gview->action != NULL) {
		gog_tool_action_free (gview->action);
		gview->action = NULL;
	}

	(G_OBJECT_CLASS (gview_parent_klass)->finalize) (obj);
}

static void
gog_graph_view_class_init (GogGraphViewClass *gview_klass)
{
	GogViewClass *view_klass    = (GogViewClass *) gview_klass;
	GObjectClass *gobject_klass = (GObjectClass *) view_klass;
	GogOutlinedViewClass *oview_klass = (GogOutlinedViewClass *) gview_klass;

	gview_parent_klass = g_type_class_peek_parent (gview_klass);
	gobject_klass->set_property = gog_graph_view_set_property;
	gobject_klass->finalize	    = gog_graph_view_finalize;
	view_klass->build_toolkit   = NULL;
	view_klass->render	    = gog_graph_view_render;
	view_klass->size_allocate   = gog_graph_view_size_allocate;
	view_klass->clip	    = TRUE;
	oview_klass->call_parent_render = FALSE;

	g_object_class_install_property (gobject_klass, GRAPH_VIEW_PROP_RENDERER,
		g_param_spec_object ("renderer",
			_("Renderer"),
			_("the renderer for this view"),
			GOG_TYPE_RENDERER,
			GSF_PARAM_STATIC | G_PARAM_WRITABLE));

	gog_graph_view_signals [GRAPH_VIEW_SELECTION_CHANGED] = g_signal_new ("selection-changed",
		G_TYPE_FROM_CLASS (gview_klass),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GogGraphViewClass, selection_changed),
		NULL, NULL,
		g_cclosure_marshal_VOID__OBJECT,
		G_TYPE_NONE,
		1, G_TYPE_OBJECT);
}

static void
gog_graph_view_init (GogGraphView *view)
{
	view->action = NULL;
	view->selected_object = NULL;
	view->selected_view = NULL;
#ifdef GOFFICE_WITH_GTK
	view->cursor_type = GDK_LEFT_PTR;
#endif
}

GSF_CLASS (GogGraphView, gog_graph_view,
	   gog_graph_view_class_init, gog_graph_view_init,
	   GOG_TYPE_OUTLINED_VIEW)

#ifdef GOFFICE_WITH_GTK
static void
update_cursor (GogGraphView *view, GogTool *tool, GdkWindow *window)
{
	GdkCursor *cursor;
	GdkCursorType cursor_type;

	cursor_type = tool != NULL ? tool->cursor_type : GDK_LEFT_PTR;
	if (cursor_type != view->cursor_type) {
		view->cursor_type = cursor_type;
		/* FIXME: gdk_display_get_default is probably not a good idea */
		cursor = gdk_cursor_new_for_display (gdk_display_get_default (), cursor_type);
		gdk_window_set_cursor (window, cursor);
		gdk_cursor_unref (cursor);
	}
}
#endif

static void
update_action (GogGraphView *view, GogTool *tool, double x, double y)
{
	if (view->action != NULL) {
		gog_tool_action_free (view->action);
		view->action = NULL;
	}

	if (tool != NULL)
		view->action = gog_tool_action_new (view->selected_view, tool, x, y);
}

#ifdef GOFFICE_WITH_GTK
#include <gdk/gdk.h>

/**
 * gog_graph_view_handle_event:
 * @gview : #GogGraphView
 * @event : #GdkEvent
 * @x_offset : in pixels
 * @y_offset : in pixels
 *
 * Handle events.
 **/
void
gog_graph_view_handle_event (GogGraphView *view, GdkEvent *event,
			     double x_offset, double y_offset)
{
	GogObject *old_object = view->selected_object;
	GogTool *tool = NULL;
	GdkWindow *window = event->any.window;
	double x, y;
	int x_int, y_int;

	x = event->button.x - x_offset;
	y = event->button.y - y_offset;

	switch (event->type) {
		case GDK_2BUTTON_PRESS:
			if (view->action != NULL)
				gog_tool_action_double_click (view->action);
			break;
		case GDK_BUTTON_PRESS:
			if (view->selected_view != NULL)
				tool = gog_view_get_tool_at_point (view->selected_view, x, y,
								   &view->selected_object);
			if (tool == NULL)
				view->selected_view = gog_view_get_view_at_point (GOG_VIEW (view), x, y,
										  &view->selected_object,
										  &tool);
			if (old_object != view->selected_object) {
				g_signal_emit (G_OBJECT (view),
					       gog_graph_view_signals [GRAPH_VIEW_SELECTION_CHANGED],
					       0, view->selected_object);
				gog_view_queue_redraw (GOG_VIEW (view));
			}
			update_action (view, tool, x, y);
			update_cursor (view, tool, window);
			break;
		case GDK_MOTION_NOTIFY:
			if (event->motion.is_hint) {
				gdk_window_get_pointer (window, &x_int, &y_int, NULL);
				x = x_int - x_offset;
				y = y_int - y_offset;
			}
			if (view->action != NULL) {
				gog_tool_action_move (view->action, x, y);
				gdk_window_process_updates (window, TRUE);
			}
			else if (view->selected_view != NULL) {
				tool = gog_view_get_tool_at_point (view->selected_view, x, y, NULL);
				update_cursor (view, tool, window);
			}
			break;
		case GDK_BUTTON_RELEASE:
			update_action (view, NULL, 0, 0);
			if (view->selected_view != NULL) {
				tool = gog_view_get_tool_at_point (view->selected_view, x, y, NULL);
				update_cursor (view, tool, window);
				gog_object_request_editor_update (view->selected_view->model);
			}
			break;
		default:
			break;
	}
}
#endif

/**
 * gog_graph_view_get_selection :
 * @gview: #GogGraphView
 *
 * Returns: current selected view.
 **/
GogView *
gog_graph_view_get_selection (GogGraphView *gview)
{
	g_return_val_if_fail (GOG_IS_GRAPH_VIEW (gview), NULL);

	return gview->selected_view;
}

/**
 * gog_graph_view_set_selection
 * @gview : #GogGraphView
 * @gobj : new selected object
 *
 * Sets @gobj as current selection. If @gobj is different from previously
 * selected object, a selection-changed signal is emitted.
 **/
void
gog_graph_view_set_selection (GogGraphView *gview, GogObject *gobj)
{
	GogView *view;

	g_return_if_fail (GOG_IS_GRAPH_VIEW (gview));
	g_return_if_fail (GOG_IS_OBJECT (gobj));

	if (gview->selected_object == gobj)
		return;

	gview->selected_object = gobj;
	view = gog_view_find_child_view (GOG_VIEW (gview), gobj);

	if (gview->selected_view != view) {
		gview->selected_view = view;
		update_action (gview, NULL, 0, 0);
	}

	gog_view_queue_redraw (GOG_VIEW (gview));
	g_signal_emit (G_OBJECT (gview),
		       gog_graph_view_signals [GRAPH_VIEW_SELECTION_CHANGED],
		       0, gobj);
}

/**
 * gog_graph_get_supported_image_formats:
 *
 * Builds a list of supported formats for image export.
 *
 * returns: a #GSList of #GOImageFormat.
 **/

GSList *
gog_graph_get_supported_image_formats (void)
{
	static GOImageFormat supported_formats[] = {
#ifdef HAVE_CAIRO_PS_SURFACE_SET_EPS
		GO_IMAGE_FORMAT_EPS,
#endif
#ifdef GOG_RENDERER_CAIRO_WITH_PS
		GO_IMAGE_FORMAT_PS,
#endif
#ifdef GOG_RENDERER_CAIRO_WITH_PDF
		GO_IMAGE_FORMAT_PDF,
#endif
		GO_IMAGE_FORMAT_JPG,
		GO_IMAGE_FORMAT_PNG,
		GO_IMAGE_FORMAT_SVG
	};
	GSList *list = NULL;
	unsigned i;

	for (i = 0; i < G_N_ELEMENTS (supported_formats); i++)
		list = g_slist_prepend (list, GUINT_TO_POINTER (supported_formats[i]));

	return list;
}

/**
 * gog_graph_export_image:
 * @graph: a #GogGraph
 * @format: image format for export
 * @output: a #GsfOutput stream
 * @x_dpi: x resolution of exported graph
 * @y_dpi: y resolution of exported graph
 *
 * Exports an image of @graph in given @format, writing results in a #GsfOutput stream.
 * If export format type is a bitmap one, it computes image size with x_dpi, y_dpi and
 * @graph size (see @gog_graph_get_size()).
 *
 * returns: %TRUE if export succeed.
 **/

gboolean
gog_graph_export_image (GogGraph *graph, GOImageFormat format, GsfOutput *output,
			double x_dpi, double y_dpi)
{
	GogRenderer *renderer;
	gboolean result;

	g_return_val_if_fail (GOG_IS_GRAPH (graph), FALSE);
	g_return_val_if_fail (format != GO_IMAGE_FORMAT_UNKNOWN, FALSE);

	renderer = gog_renderer_new (graph);
	result = gog_renderer_export_image (renderer, format, output, x_dpi, y_dpi);
	g_object_unref (renderer);

	return result;
}

/**
 * gog_graph_render_to_cairo:
 * @graph: a #GogGraph
 * @cairo: a cairo context
 * @w: width
 * @h: height
 *
 * Renders a graph using @cairo. @w and @h are the requested width an height of the rendered graph, in the current @cairo coordinate space. This function is not suited for multiple rendering of the same graph. gog_renderer_render_to cairo or gog_renderer_update/gog_renderer_get_cairo_surface should be used instead.
 **/

void gog_graph_render_to_cairo (GogGraph *graph, cairo_t *cairo, double w, double h)
{
	GogRenderer *renderer;

	g_return_if_fail (GOG_IS_GRAPH (graph));

	renderer = gog_renderer_new (graph);
	gog_renderer_render_to_cairo (renderer, cairo, w, h);
	g_object_unref (renderer);
}
