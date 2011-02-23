/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-plot.c :
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
#include <goffice/graph/gog-plot-impl.h>
#include <goffice/graph/gog-plot-engine.h>
#include <goffice/graph/gog-series-impl.h>
#include <goffice/graph/gog-chart.h>
#include <goffice/graph/gog-axis.h>
#include <goffice/graph/gog-theme.h>
#include <goffice/graph/gog-graph.h>
#include <goffice/graph/gog-object-xml.h>
#include <goffice/graph/gog-renderer.h>
#include <goffice/data/go-data.h>
#include <goffice/math/go-math.h>
#include <goffice/utils/go-persist.h>
#include <goffice/utils/go-style.h>
#include <goffice/utils/go-styled-object.h>
#include <glib/gi18n-lib.h>

#ifdef GOFFICE_WITH_GTK
#include <gtk/gtk.h>
#endif

#include <gsf/gsf-impl-utils.h>
#include <string.h>

#define GOG_PLOT_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GOG_TYPE_PLOT, GogPlotClass))

/**
 * SECTION: gog-plot
 * @short_description: A plot.
 * @See_also: #GogChart, #GogSeries
 *
 * This is the object that visualizes data.
 * To manipulate the data shown in the plot, use gog_plot_new_series() and
 * gog_plot_clear_series()
 *
 * Plots are implemented as plug-ins, so make sure the plug-in system is
 * initialized before trying to create one. See go_plugins_init()
 *
 * GOffice ships a number of common plot implementations by default.
 */

enum {
	PLOT_PROP_0,
	PLOT_PROP_VARY_STYLE_BY_ELEMENT,
	PLOT_PROP_AXIS_X,
	PLOT_PROP_AXIS_Y,
	PLOT_PROP_GROUP,
	PLOT_PROP_DEFAULT_INTERPOLATION,
	PLOT_PROP_GURU_HINTS
};

static GObjectClass *plot_parent_klass;

static gboolean gog_plot_set_axis_by_id (GogPlot *plot, GogAxisType type, unsigned id);
static unsigned gog_plot_get_axis_id (GogPlot const *plot, GogAxisType type);

static void
gog_plot_finalize (GObject *obj)
{
	GogPlot *plot = GOG_PLOT (obj);

	g_slist_free (plot->series); /* GogObject does the unref */

	gog_plot_axis_clear (plot, GOG_AXIS_SET_ALL); /* just in case */
	g_free (plot->plot_group);
	g_free (plot->guru_hints);

	(*plot_parent_klass->finalize) (obj);
}

static gboolean
role_series_can_add (GogObject const *parent)
{
	GogPlot *plot = GOG_PLOT (parent);
	return g_slist_length (plot->series) < plot->desc.num_series_max;
}

static GogObject *
role_series_allocate (GogObject *plot)
{
	GogPlotClass *klass = GOG_PLOT_GET_CLASS (plot);
	GType type = klass->series_type;

	if (type == 0)
		type = GOG_TYPE_SERIES;
	return g_object_new (type, NULL);
}

static void
role_series_post_add (GogObject *parent, GogObject *child)
{
	GogPlot *plot = GOG_PLOT (parent);
	GogSeries *series = GOG_SERIES (child);
	unsigned num_dim;
	num_dim = plot->desc.series.num_dim;

	/* Alias things so that dim -1 is valid */
	series->values = g_new0 (GogDatasetElement, num_dim+1) + 1;
	series->plot = plot;
	series->interpolation = plot->interpolation;

	/* if there are other series associated with the plot, and there are
	 * shared dimensions, clone them over.  */
	if (series->plot->series != NULL) {
		GogGraph *graph = gog_object_get_graph (GOG_OBJECT (plot));
		GogSeriesDesc const *desc = &plot->desc.series;
		GogSeries const *src = plot->series->data;
		unsigned i;

		for (i = num_dim; i-- > 0 ; ) /* name is never shared */
			if (desc->dim[i].is_shared)
				gog_dataset_set_dim_internal (GOG_DATASET (series),
					i, src->values[i].data, graph);

		gog_series_check_validity (series);
	}

	/* APPEND to keep order, there won't be that many */
	plot->series = g_slist_append (plot->series, series);
	gog_plot_request_cardinality_update (plot);
}

static void
role_series_pre_remove (GogObject *parent, GogObject *series)
{
	GogPlot *plot = GOG_PLOT (parent);
	plot->series = g_slist_remove (plot->series, series);
	gog_plot_request_cardinality_update (plot);
}

#ifdef GOFFICE_WITH_GTK

static void
cb_axis_changed (GtkComboBox *combo, GogPlot *plot)
{
	GtkTreeIter iter;
	GValue value;
	GtkTreeModel *model = gtk_combo_box_get_model (combo);

	memset (&value, 0, sizeof (GValue));
	gtk_combo_box_get_active_iter (combo, &iter);
	gtk_tree_model_get_value (model, &iter, 1, &value);
	gog_plot_set_axis_by_id (plot,
				 GPOINTER_TO_UINT (g_object_get_data (G_OBJECT(combo), "axis-type")),
				 g_value_get_uint (&value));
}

static void
gog_plot_populate_editor (GogObject *obj,
			  GOEditor *editor,
			  G_GNUC_UNUSED GogDataAllocator *dalloc,
			  GOCmdContext *cc)
{
	static const char *axis_labels[GOG_AXIS_TYPES] = {
		N_("X axis:"),
		N_("Y axis:"),
		N_("Z axis:"),
		N_("Circular axis:"),
		N_("Radial axis:"),
		N_("Pseudo 3D axis:"),
		N_("Color axis:"),
		N_("Bubble axis:")
	};

	GogAxisType type;
	GogPlot *plot = GOG_PLOT (obj);
	unsigned count = 0, axis_count;
	GSList *axes, *ptr;
	GogChart *chart = GOG_CHART (gog_object_get_parent (obj));
	GogAxis *axis;
	GtkListStore *store;
	GtkTreeIter iter;
	GtkCellRenderer *cell;

	g_return_if_fail (chart != NULL);
	
	if (gog_chart_get_axis_set (chart) == GOG_AXIS_SET_XY) {
		GtkWidget *combo;
		GtkWidget *table = gtk_table_new (0, 1, FALSE);

		for (type = 0 ; type < GOG_AXIS_TYPES ; type++) {
			if (plot->axis[type] != NULL) {
				count++;
				gtk_table_resize (GTK_TABLE (table), count, 1);
				gtk_table_attach (GTK_TABLE (table), gtk_label_new (_(axis_labels[type])),
						  0, 1, count - 1, count, 0, 0, 0, 0);

				store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_UINT);
				combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
				g_object_unref (store);

				cell = gtk_cell_renderer_text_new ();
				gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, TRUE);
				gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), cell,
								"text", 0,
								NULL);

				axes = gog_chart_get_axes (chart, type);
				axis_count = 0;
				for (ptr = axes; ptr != NULL; ptr = ptr->next) {
					axis = GOG_AXIS (ptr->data);
					gtk_list_store_prepend (store, &iter);
					gtk_list_store_set (store, &iter,
							    0, gog_object_get_name (GOG_OBJECT (axis)),
							    1, gog_object_get_id (GOG_OBJECT (axis)),
							    -1);
					if (axis == plot->axis[type])
						gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
					axis_count++;
				}
				if (axis_count < 2)
					gtk_widget_set_sensitive (GTK_WIDGET (combo), FALSE);
				g_slist_free (axes);
				gtk_table_attach (GTK_TABLE (table), combo,
						  1, 2, count - 1, count, 0, 0, 0, 0);
				g_object_set_data (G_OBJECT (combo), "axis-type", GUINT_TO_POINTER (type));
				g_signal_connect (G_OBJECT (combo), "changed",
						  G_CALLBACK (cb_axis_changed), plot);
			}
		}

		if (count > 0) {
			gtk_table_set_col_spacings (GTK_TABLE (table), 12);
			gtk_table_set_row_spacings (GTK_TABLE (table), 6);
			gtk_container_set_border_width (GTK_CONTAINER (table), 12);
			gtk_widget_show_all (table);
			go_editor_add_page (editor, table, _("Axes"));
		}
		else
			g_object_unref (G_OBJECT (table));
	}

	(GOG_OBJECT_CLASS(plot_parent_klass)->populate_editor) (obj, editor, dalloc, cc);
}
#endif

static void
gog_plot_set_property (GObject *obj, guint param_id,
		       GValue const *value, GParamSpec *pspec)
{
	GogPlot *plot = GOG_PLOT (obj);
	gboolean b_tmp;

	switch (param_id) {
	case PLOT_PROP_VARY_STYLE_BY_ELEMENT:
		b_tmp = g_value_get_boolean (value) &&
			gog_plot_supports_vary_style_by_element (plot);
		if (plot->vary_style_by_element ^ b_tmp) {
			plot->vary_style_by_element = b_tmp;
			gog_plot_request_cardinality_update (plot);
		}
		break;
	case PLOT_PROP_AXIS_X:
		gog_plot_set_axis_by_id (plot, GOG_AXIS_X, g_value_get_uint (value));
		break;
	case PLOT_PROP_AXIS_Y:
		gog_plot_set_axis_by_id (plot, GOG_AXIS_Y, g_value_get_uint (value));
		break;
	case PLOT_PROP_GROUP: {
		char const *group = g_value_get_string (value);
		g_free (plot->plot_group);
		plot->plot_group = (group)? g_strdup (g_value_get_string (value)): NULL;
		break;
	}
	case PLOT_PROP_DEFAULT_INTERPOLATION:
		plot->interpolation = go_line_interpolation_from_str (g_value_get_string (value));
		break;
	case PLOT_PROP_GURU_HINTS:
		g_free (plot->guru_hints);
		plot->guru_hints = g_strdup (g_value_get_string (value));
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
}

static void
gog_plot_get_property (GObject *obj, guint param_id,
		       GValue *value, GParamSpec *pspec)
{
	GogPlot *plot = GOG_PLOT (obj);
	switch (param_id) {
	case PLOT_PROP_VARY_STYLE_BY_ELEMENT:
		g_value_set_boolean (value,
			plot->vary_style_by_element &&
			gog_plot_supports_vary_style_by_element (plot));
		break;
	case PLOT_PROP_AXIS_X:
		g_value_set_uint (value, gog_plot_get_axis_id (plot, GOG_AXIS_X));
		break;
	case PLOT_PROP_AXIS_Y:
		g_value_set_uint (value, gog_plot_get_axis_id (plot, GOG_AXIS_Y));
		break;
	case PLOT_PROP_GROUP:
		g_value_set_string (value, plot->plot_group);
		break;
	case PLOT_PROP_DEFAULT_INTERPOLATION:
		g_value_set_string (value, go_line_interpolation_as_str (plot->interpolation));
		break;
	case PLOT_PROP_GURU_HINTS:
		g_value_set_string (value, plot->guru_hints);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
gog_plot_children_reordered (GogObject *obj)
{
	GSList *ptr, *accum = NULL;
	GogPlot *plot = GOG_PLOT (obj);

	for (ptr = obj->children; ptr != NULL ; ptr = ptr->next)
		if (GOG_IS_SERIES (ptr->data))
			accum = g_slist_prepend (accum, ptr->data);
	g_slist_free (plot->series);
	plot->series = g_slist_reverse (accum);

	gog_plot_request_cardinality_update (plot);
}

static void
gog_plot_class_init (GogObjectClass *gog_klass)
{
	static GogObjectRole const roles[] = {
		{ N_("Series"), "GogSeries",	0,
		  GOG_POSITION_SPECIAL, GOG_POSITION_SPECIAL, GOG_OBJECT_NAME_BY_ROLE,
		  role_series_can_add, NULL,
		  role_series_allocate,
		  role_series_post_add, role_series_pre_remove, NULL },
	};
	GObjectClass *gobject_klass = (GObjectClass *) gog_klass;
	GogPlotClass *plot_klass = (GogPlotClass *) gog_klass;

	plot_parent_klass = g_type_class_peek_parent (gog_klass);
	gobject_klass->finalize		= gog_plot_finalize;
	gobject_klass->set_property	= gog_plot_set_property;
	gobject_klass->get_property	= gog_plot_get_property;
#ifdef GOFFICE_WITH_GTK
	gog_klass->populate_editor	= gog_plot_populate_editor;
#endif
	plot_klass->axis_set 		= GOG_AXIS_SET_NONE;
	plot_klass->guru_helper		= NULL;

	g_object_class_install_property (gobject_klass, PLOT_PROP_VARY_STYLE_BY_ELEMENT,
		g_param_spec_boolean ("vary-style-by-element",
			_("Vary style by element"),
			_("Use a different style for each segments"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT | GOG_PARAM_FORCE_SAVE));
	g_object_class_install_property (gobject_klass, PLOT_PROP_AXIS_X,
		g_param_spec_uint ("x-axis",
			_("X axis"),
			_("Reference to X axis"),
			0, G_MAXINT, 0,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, PLOT_PROP_AXIS_Y,
		g_param_spec_uint ("y-axis",
			_("Y axis"),
			_("Reference to Y axis"),
			0, G_MAXINT, 0,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, PLOT_PROP_GROUP,
		g_param_spec_string ("plot-group",
			_("Plot group"),
			_("Name of plot group if any"),
			NULL,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, PLOT_PROP_GURU_HINTS,
		g_param_spec_string ("guru-hints",
			_("Guru hints"),
			_("Semicolon separated list of hints for automatic addition of objects in "
			  "guru dialog"),
			NULL,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
        g_object_class_install_property (gobject_klass, PLOT_PROP_DEFAULT_INTERPOLATION,
		 g_param_spec_string ("interpolation",
			_("Default interpolation"),
			_("Default type of series line interpolation"),
			"linear",
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	gog_klass->children_reordered = gog_plot_children_reordered;
	gog_object_register_roles (gog_klass, roles, G_N_ELEMENTS (roles));
	GOG_PLOT_CLASS (gog_klass)->update_3d = NULL;
}

static void
gog_plot_init (GogPlot *plot, GogPlotClass const *derived_plot_klass)
{
	/* keep a local copy so that we can over-ride things if desired */
	plot->desc = derived_plot_klass->desc;
	/* start as true so that we can queue an update when it changes */
	plot->cardinality_valid = TRUE;
	plot->rendering_order = GOG_PLOT_RENDERING_LAST;
	plot->plot_group = NULL;
	plot->guru_hints = NULL;
	plot->interpolation = GO_LINE_INTERPOLATION_LINEAR;
}

GSF_CLASS_ABSTRACT (GogPlot, gog_plot,
		    gog_plot_class_init, gog_plot_init,
		    GOG_TYPE_OBJECT)

GogPlot *
gog_plot_new_by_type (GogPlotType const *type)
{
	GogPlot *res;

	g_return_val_if_fail (type != NULL, NULL);

	res = gog_plot_new_by_name (type->engine);
	if (res != NULL && type->properties != NULL)
		g_hash_table_foreach (type->properties,
			(GHFunc) gog_object_set_arg, res);
	return res;
}

/****************************************************************
 * convenience routines
 **/

/**
 * gog_plot_new_series :
 * @plot : #GogPlot
 *
 * Returns: a new GogSeries of a type consistent with @plot.
 **/
GogSeries *
gog_plot_new_series (GogPlot *plot)
{
	GogObject *res;

	g_return_val_if_fail (GOG_IS_PLOT (plot), NULL);

	res = gog_object_add_by_name (GOG_OBJECT (plot), "Series", NULL);
	return res ? GOG_SERIES (res) : NULL;
}
GogPlotDesc const *
gog_plot_description (GogPlot const *plot)
{
	g_return_val_if_fail (GOG_IS_PLOT (plot), NULL);
	return &plot->desc;
}

static GogChart *
gog_plot_get_chart (GogPlot const *plot)
{
	return GOG_CHART (GOG_OBJECT (plot)->parent);
}

/* protected */
void
gog_plot_request_cardinality_update (GogPlot *plot)
{
	g_return_if_fail (GOG_IS_PLOT (plot));

	if (plot->cardinality_valid) {
		GogChart *chart = gog_plot_get_chart (plot);
		plot->cardinality_valid = FALSE;
		gog_object_request_update (GOG_OBJECT (plot));
		if (chart != NULL)
			gog_chart_request_cardinality_update (chart);
	}
}

/**
 * gog_plot_update_cardinality :
 * @plot : #GogPlot
 * @index_num: index offset for this plot
 *
 * Update cardinality and cache result. See @gog_chart_get_cardinality.
 **/
void
gog_plot_update_cardinality (GogPlot *plot, int index_num)
{
	GogSeries *series;
	GSList	  *ptr, *children;
	gboolean   is_valid;
	unsigned   size = 0, no_legend = 0, i, j;

	g_return_if_fail (GOG_IS_PLOT (plot));

	plot->cardinality_valid = TRUE;
	plot->index_num = i = j = index_num;

	for (ptr = plot->series; ptr != NULL ; ptr = ptr->next) {
		series = GOG_SERIES (ptr->data);
		is_valid = gog_series_is_valid (GOG_SERIES (series));
		if (plot->vary_style_by_element) {
			if (is_valid && size < series->num_elements)
				size = series->num_elements;
			gog_series_set_index (series, plot->index_num, FALSE);
		} else {
			gog_series_set_index (series, i++, FALSE);
			if (!gog_series_has_legend (series))
				no_legend++;
			j++;
		}
		/* now add the trend lines if any */
		children = GOG_OBJECT (series)->children;
		while (children) {
			if (GOG_IS_TREND_LINE (children->data)) {
				j++;
				if (!gog_trend_line_has_legend (GOG_TREND_LINE (children->data)))
					no_legend++;
			}
			children = children->next;
		}
	}

	plot->full_cardinality =
		plot->vary_style_by_element ? size : (j - plot->index_num);
	plot->visible_cardinality = plot->full_cardinality - no_legend;
}

/**
 * gog_plot_get_cardinality:
 * @plot: #GogPlot
 * @full: placeholder for full cardinality
 * @visible: placeholder for visible cardinality
 *
 * Return the number of logical elements in the plot.
 * See @gog_chart_get_cardinality.
 *
 * @full and @visible may be NULL.
 **/
void
gog_plot_get_cardinality (GogPlot *plot, unsigned *full, unsigned *visible)
{
	g_return_if_fail (GOG_IS_PLOT (plot));

	if (!plot->cardinality_valid)
		g_warning ("[GogPlot::get_cardinality] Invalid cardinality");

	if (full != NULL)
		*full = plot->full_cardinality;
	if (visible != NULL)
		*visible = plot->visible_cardinality;
}

static gboolean
gog_plot_enum_in_reverse (GogPlot const *plot)
{
	GogPlotClass *klass = GOG_PLOT_GET_CLASS (plot);
	return klass != NULL && klass->enum_in_reverse && (klass->enum_in_reverse) (plot);
}

void
gog_plot_foreach_elem (GogPlot *plot, gboolean only_visible,
		       GogEnumFunc func, gpointer data)
{
	GSList *ptr, *children;
	GogSeries const *series;
	GOStyle *style, *tmp_style;
	GOData *labels;
	unsigned i, n, num_labels = 0;
	char *label = NULL;
	GogTheme *theme = gog_object_get_theme (GOG_OBJECT (plot));
	GogPlotClass *klass = GOG_PLOT_GET_CLASS (plot);
	GList *overrides;

	g_return_if_fail (GOG_IS_PLOT (plot));
	if (!plot->cardinality_valid)
		gog_plot_get_cardinality (plot, NULL, NULL);

	if (klass->foreach_elem) {
		klass->foreach_elem (plot, only_visible, func, data);
		return;
	}

	ptr = plot->series;
	if (ptr == NULL)
		return;

	if (!plot->vary_style_by_element) {
		GSList *tmp = NULL;

		unsigned i = plot->index_num;

		if (gog_plot_enum_in_reverse (plot))
			ptr = tmp = g_slist_reverse (g_slist_copy (ptr));

		for (; ptr != NULL ; ptr = ptr->next) {
			if (!only_visible || gog_series_has_legend (ptr->data)) {
				func (i, go_styled_object_get_style (ptr->data),
				      gog_object_get_name (ptr->data), data);
				i++;
			}
			/* now add the trend lines if any */
			children = GOG_OBJECT (ptr->data)->children;
			while (children) {
				if (GOG_IS_TREND_LINE (children->data) &&
				    gog_trend_line_has_legend (GOG_TREND_LINE (children->data))) {
					func (i, go_styled_object_get_style (children->data),
					      gog_object_get_name (children->data), data);
					i++;
				}
				children = children->next;
			}
		}

		g_slist_free (tmp);

		return;
	}

	series = ptr->data; /* start with the first */
	labels = NULL;
	if (series->values[0].data != NULL) {
		labels = series->values[0].data;
		num_labels = go_data_get_vector_size (labels);
	}
	style = go_style_dup (series->base.style);
	n = only_visible ? plot->visible_cardinality : plot->full_cardinality;
	for (overrides = series->overrides, i = 0; i < n ; i++) {
		if (overrides != NULL &&
		    (GOG_SERIES_ELEMENT (overrides->data)->index == i)) {
			tmp_style = GOG_STYLED_OBJECT (overrides->data)->style;
			overrides = overrides->next;
		} else
			tmp_style = style;

		gog_theme_fillin_style (theme, tmp_style, GOG_OBJECT (series),
			plot->index_num + i, tmp_style->interesting_fields);
		if (labels != NULL)
			label = (i < num_labels) ? go_data_get_vector_string (labels, i) : g_strdup ("");
		else
			label = NULL;
		if (label == NULL)
			label = g_strdup_printf ("%d", i);
		(func) (i, tmp_style, label, data);
		g_free (label);
	}
	g_object_unref (style);
}

/**
 * gog_plot_get_series :
 * @plot : #GogPlot
 *
 * Returns: A list of the series in @plot.  Do not modify or free the list.
 **/
GSList const *
gog_plot_get_series (GogPlot const *plot)
{
	g_return_val_if_fail (GOG_IS_PLOT (plot), NULL);
	return plot->series;
}

/**
 * gog_plot_get_axis_bounds :
 * @plot : #GogPlot
 * @axis : #GogAxisType
 * @bounds : #GogPlotBoundInfo
 *
 * Queries @plot for its axis preferences for @axis and stores the results in
 * @bounds.  All elements of @bounds are initialized to sane values before the
 * query _EXCEPT_ ::fmt.  The caller is responsible for initializing it.  This
 * is done so that once on plot has selected a format the others need not do
 * the lookup too if so desired.
 *
 * Caller is responsible for unrefing ::fmt.
 *
 * Returns: The data providing the bound (does not add a ref)
 **/
GOData *
gog_plot_get_axis_bounds (GogPlot *plot, GogAxisType axis,
			  GogPlotBoundInfo *bounds)
{
	GogPlotClass *klass = GOG_PLOT_GET_CLASS (plot);

	g_return_val_if_fail (klass != NULL, NULL);
	g_return_val_if_fail (bounds != NULL, NULL);

	bounds->val.minima =  DBL_MAX;
	bounds->val.maxima = -DBL_MAX;
	bounds->logical.maxima = go_nan;
	bounds->logical.minima = go_nan;
	bounds->is_discrete = FALSE;
	bounds->center_on_ticks = TRUE;
	bounds->date_conv = NULL;

	if (klass->axis_get_bounds == NULL)
		return NULL;
	return (klass->axis_get_bounds) (plot, axis, bounds);
}

gboolean
gog_plot_supports_vary_style_by_element (GogPlot const *plot)
{
	GogPlotClass *klass = GOG_PLOT_GET_CLASS (plot);

	g_return_val_if_fail (klass != NULL, FALSE);

	if (klass->supports_vary_style_by_element)
		return (klass->supports_vary_style_by_element) (plot);
	return TRUE; /* default */
}

GogAxisSet
gog_plot_axis_set_pref (GogPlot const *plot)
{
	GogPlotClass *klass = GOG_PLOT_GET_CLASS (plot);

	g_return_val_if_fail (klass != NULL, GOG_AXIS_SET_UNKNOWN);
	return klass->axis_set;
}

gboolean
gog_plot_axis_set_is_valid (GogPlot const *plot, GogAxisSet axis_set)
{
	GogPlotClass *klass = GOG_PLOT_GET_CLASS (plot);

	g_return_val_if_fail (klass != NULL, FALSE);
	return (axis_set == klass->axis_set);
}

/**
 * gog_plot_set_axis:
 * @plot : #GogPlot
 * @axis : #GogAxis
 *
 * Connect @axis and @plot.
 **/
void
gog_plot_set_axis (GogPlot *plot, GogAxis *axis)
{
	GogAxisType type;
	g_return_if_fail (GOG_IS_PLOT (plot));
	g_return_if_fail (GOG_IS_AXIS (axis));

	type = gog_axis_get_atype (axis);
	g_return_if_fail (type != GOG_AXIS_UNKNOWN);

	if (plot->axis[type] == axis)
		return;

	if (plot->axis[type] != NULL)
		gog_axis_del_contributor (plot->axis[type], GOG_OBJECT (plot));
	plot->axis[type] = axis;
	gog_axis_add_contributor (axis, GOG_OBJECT (plot));
}

static gboolean
gog_plot_set_axis_by_id (GogPlot *plot, GogAxisType type, unsigned id)
{
	GogChart const *chart;
	GogAxis *axis;
	GSList *axes, *ptr;
	gboolean found = FALSE;

	if (id == 0)
		return FALSE;

	g_return_val_if_fail (GOG_IS_PLOT (plot), FALSE);
	g_return_val_if_fail (GOG_OBJECT (plot)->parent != NULL, FALSE);

	chart = gog_plot_get_chart (plot);
	g_return_val_if_fail (GOG_CHART (chart) != NULL, FALSE);

	axes = gog_chart_get_axes (chart, type);
	g_return_val_if_fail (axes != NULL, FALSE);

	for (ptr = axes; ptr != NULL && !found; ptr = ptr->next) {
		axis = GOG_AXIS (ptr->data);
		if (gog_object_get_id (GOG_OBJECT (axis)) == id) {
			gog_plot_set_axis (plot, axis);
			found = TRUE;
		}
	}
	g_slist_free (axes);
	return found;
}

gboolean
gog_plot_axis_set_assign (GogPlot *plot, GogAxisSet axis_set)
{
	GogPlotClass *klass = GOG_PLOT_GET_CLASS (plot);
	GogChart const *chart;
	GogAxisType type;

	g_return_val_if_fail (klass != NULL, FALSE);

	chart = gog_plot_get_chart (plot);
	for (type = 0 ; type < GOG_AXIS_TYPES ; type++) {
		if (plot->axis[type] != NULL) {
			if (!(axis_set & (1 << type))) {
				gog_axis_del_contributor (plot->axis[type], GOG_OBJECT (plot));
				plot->axis[type] = NULL;
			}
		} else if (axis_set & (1 << type)) {
			GSList *axes = gog_chart_get_axes (chart, type);
			if (axes != NULL) {
				gog_axis_add_contributor (axes->data, GOG_OBJECT (plot));
				plot->axis[type] = axes->data;
				g_slist_free (axes);
			}
		}
	}

	return (axis_set == klass->axis_set);
}

/**
 * gog_plot_axis_clear :
 * @plot : #GogPlot
 * @filter : #GogAxisSet
 *
 * A utility method to clear all existing axis associations flagged by @filter
 **/
void
gog_plot_axis_clear (GogPlot *plot, GogAxisSet filter)
{
	GogAxisType type;

	g_return_if_fail (GOG_IS_PLOT (plot));

	for (type = 0 ; type < GOG_AXIS_TYPES ; type++)
		if (plot->axis[type] != NULL && ((1 << type) & filter)) {
			gog_axis_del_contributor (plot->axis[type], GOG_OBJECT (plot));
			plot->axis[type] = NULL;
		}
}

static unsigned
gog_plot_get_axis_id (GogPlot const *plot, GogAxisType type)
{
	GogAxis *axis = gog_plot_get_axis (plot, type);

	return axis != NULL ? gog_object_get_id (GOG_OBJECT (axis)) : 0;
}

GogAxis	*
gog_plot_get_axis (GogPlot const *plot, GogAxisType type)
{
	g_return_val_if_fail (GOG_IS_PLOT (plot), NULL);
	g_return_val_if_fail (type < GOG_AXIS_TYPES, NULL);
	g_return_val_if_fail (GOG_AXIS_UNKNOWN < type, NULL);
	return plot->axis[type];
}

void
gog_plot_update_3d (GogPlot *plot)
{
	GogPlotClass *klass = GOG_PLOT_GET_CLASS (plot);
	g_return_if_fail (GOG_IS_PLOT (plot));

	if (klass->update_3d)
		klass->update_3d (plot);
}

static void
gog_plot_guru_helper_add_grid_line (GogPlot *plot, gboolean major)
{
	GogAxisType type;

	for (type = 0; type < GOG_AXIS_TYPES; type++) {
		if ((((type == GOG_AXIS_X) |
		      (type == GOG_AXIS_Y) |
		      (type == GOG_AXIS_CIRCULAR) |
		      (type == GOG_AXIS_RADIAL))) &&
		    plot->axis[type] != NULL &&
		    gog_axis_get_grid_line (plot->axis[type], major) == NULL)
		{
			gog_object_add_by_name (GOG_OBJECT (plot->axis[type]),
						major ? "MajorGrid": "MinorGrid", NULL);
		}
	}
}

void
gog_plot_guru_helper (GogPlot *plot)
{
	GogPlotClass *klass;
	char **hints;
	char *hint;
	unsigned i;

	g_return_if_fail (GOG_IS_PLOT (plot));
	klass = GOG_PLOT_GET_CLASS (plot);

	if (plot->guru_hints == NULL)
		return;

	hints = g_strsplit (plot->guru_hints, ";", 0);

	for (i = 0; i < g_strv_length (hints); i++) {
		hint = g_strstrip (hints[i]);
		if (strcmp (hints[i], "backplane") == 0) {
			GogChart *chart = GOG_CHART (gog_object_get_parent (GOG_OBJECT (plot)));
			if (chart != NULL && gog_chart_get_grid (chart) == NULL)
				gog_object_add_by_name (GOG_OBJECT (chart), "Backplane", NULL);
		} else if (strcmp (hints[i], "major-grid") == 0) {
			gog_plot_guru_helper_add_grid_line (plot, TRUE);
		} else if (strcmp (hints[i], "minor-grid") == 0) {
			gog_plot_guru_helper_add_grid_line (plot, FALSE);
		} else if (klass->guru_helper)
			klass->guru_helper (plot, hint);
	}

	g_strfreev (hints);
}

void gog_plot_clear_series (GogPlot *plot)
{
	while (plot->series) {
		GogObject *gobj = GOG_OBJECT (plot->series->data);
		gog_object_clear_parent (gobj);
		g_object_unref (gobj);
	}
}

/*****************************************************************************/

#ifdef GOFFICE_WITH_GTK
typedef struct {
	GogViewAllocation	 start_position;
	GogViewAllocation	 chart_allocation;
	GogChart		*chart;
} MovePlotAreaData;

static gboolean
gog_tool_move_plot_area_point (GogView *view, double x, double y, GogObject **gobj)
{
	GogViewAllocation const *plot_area = gog_chart_view_get_plot_area (view->parent);

	return (x >= plot_area->x &&
		x <= (plot_area->x + plot_area->w) &&
		y >= plot_area->y &&
		y <= (plot_area->y + plot_area->h));
}

static void
gog_tool_move_plot_area_render (GogView *view)
{
	GogViewAllocation const *plot_area = gog_chart_view_get_plot_area (view->parent);

	gog_renderer_draw_selection_rectangle (view->renderer, plot_area);
}

static void
gog_tool_move_plot_area_init (GogToolAction *action)
{
	MovePlotAreaData *data = g_new0 (MovePlotAreaData, 1);

	data->chart = GOG_CHART (action->view->parent->model);
	data->chart_allocation = action->view->parent->allocation;
	data->start_position = *gog_chart_view_get_plot_area (action->view->parent);
	data->start_position.x = (data->start_position.x - data->chart_allocation.x) / data->chart_allocation.w;
	data->start_position.w = data->start_position.w / data->chart_allocation.w;
	data->start_position.y = (data->start_position.y - data->chart_allocation.y) / data->chart_allocation.h;
	data->start_position.h = data->start_position.h / data->chart_allocation.h;

	action->data = data;
}

static void
gog_tool_move_plot_area_move (GogToolAction *action, double x, double y)
{
	GogViewAllocation plot_area;
	MovePlotAreaData *data = action->data;

	plot_area.w = data->start_position.w;
	plot_area.h = data->start_position.h;
	plot_area.x = data->start_position.x + (x - action->start_x) / data->chart_allocation.w;
	if (plot_area.x < 0.0)
		plot_area.x = 0.0;
	else if (plot_area.x + plot_area.w > 1.0)
		plot_area.x = 1.0 - plot_area.w;
	plot_area.y = data->start_position.y + (y - action->start_y) / data->chart_allocation.h;
	if (plot_area.y < 0.0)
		plot_area.y = 0.0;
	else if (plot_area.y + plot_area.h > 1.0)
		plot_area.y = 1.0 - plot_area.h;
	gog_chart_set_plot_area (data->chart, &plot_area);
}

static void
gog_tool_move_plot_area_double_click (GogToolAction *action)
{
	MovePlotAreaData *data = action->data;

	gog_chart_set_plot_area (data->chart, NULL);
}

static GogTool gog_tool_move_plot_area = {
	N_("Move plot area"),
	GDK_FLEUR,
	gog_tool_move_plot_area_point,
	gog_tool_move_plot_area_render,
	gog_tool_move_plot_area_init,
	gog_tool_move_plot_area_move,
	gog_tool_move_plot_area_double_click,
	NULL /*destroy*/
};

static gboolean
gog_tool_resize_plot_area_point (GogView *view, double x, double y, GogObject **gobj)
{
	GogViewAllocation const *plot_area = gog_chart_view_get_plot_area (view->parent);

	return gog_renderer_in_grip (x, y,
				     plot_area->x + plot_area->w,
				     plot_area->y + plot_area->h);
}

static void
gog_tool_resize_plot_area_render (GogView *view)
{
	GogViewAllocation const *plot_area = gog_chart_view_get_plot_area (view->parent);

	gog_renderer_draw_grip (view->renderer,
				plot_area->x + plot_area->w,
				plot_area->y + plot_area->h);
}

static void
gog_tool_resize_plot_area_move (GogToolAction *action, double x, double y)
{
	GogViewAllocation plot_area;
	MovePlotAreaData *data = action->data;

	plot_area.x = data->start_position.x;
	plot_area.y = data->start_position.y;
	plot_area.w = data->start_position.w + (x - action->start_x) / data->chart_allocation.w;
	if (plot_area.w + plot_area.x > 1.0)
		plot_area.w = 1.0 - plot_area.x;
	else if (plot_area.w < 0.0)
		plot_area.w = 0.0;
	plot_area.h = data->start_position.h + (y - action->start_y) / data->chart_allocation.h;
	if (plot_area.h + plot_area.y > 1.0)
		plot_area.h = 1.0 - plot_area.y;
	else if (plot_area.h < 0.0)
		plot_area.h = 0.0;
	gog_chart_set_plot_area (data->chart, &plot_area);
}

static GogTool gog_tool_resize_plot_area = {
	N_("Resize plot area"),
	GDK_BOTTOM_RIGHT_CORNER,
	gog_tool_resize_plot_area_point,
	gog_tool_resize_plot_area_render,
	gog_tool_move_plot_area_init,
	gog_tool_resize_plot_area_move,
	NULL /* double-click */,
	NULL /*destroy*/
};
#endif

/*****************************************************************************/

static GogViewClass *pview_parent_klass;

static void
gog_plot_view_build_toolkit (GogView *view)
{
#ifdef GOFFICE_WITH_GTK
	view->toolkit = g_slist_prepend (view->toolkit, &gog_tool_move_plot_area);
	view->toolkit = g_slist_prepend (view->toolkit, &gog_tool_resize_plot_area);
#endif
}

static void
gog_plot_view_class_init (GogPlotViewClass *pview_klass)
{
	GogViewClass *view_klass = (GogViewClass *) pview_klass;

	pview_parent_klass = g_type_class_peek_parent (pview_klass);

	view_klass->build_toolkit 	= gog_plot_view_build_toolkit;
}

GSF_CLASS_ABSTRACT (GogPlotView, gog_plot_view,
		    gog_plot_view_class_init, NULL,
		    GOG_TYPE_VIEW)


/**
 * gog_plot_view_get_data_at_point:
 * @view : #GogPlotView
 * @x : x position
 * @y : y position
 * @series: where to store the series
 *
 * Search a data represented at (@x,@y) in @view and set @series on success.
 *
 * return value: index of the found data in @series or -1.
 **/
int
gog_plot_view_get_data_at_point (GogPlotView *view, double x, double y, GogSeries **series)
{
	GogPlotViewClass *klass = GOG_PLOT_VIEW_GET_CLASS (view);
	g_return_val_if_fail (klass != NULL, -1);
	return (klass->get_data_at_point)? klass->get_data_at_point (view, x, y, series): -1;
}
