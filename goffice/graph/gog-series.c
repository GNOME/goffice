/*
 * go-graph-data.c :
 *
 * Copyright (C) 2003-2004 Jody Goldberg (jody@gnome.org)
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

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

#include <string.h>

/**
 * SECTION: gog-series
 * @short_description: A single data series.
 *
 * A #GogSeries represents a data series that can be added to a #GogPlot.
 */

/**
 * GogSeriesFillType:
 * @GOG_SERIES_FILL_TYPE_Y_ORIGIN: from origin ox the X-axis.
 * @GOG_SERIES_FILL_TYPE_X_ORIGIN: from the origin of the Y-axis
 * @GOG_SERIES_FILL_TYPE_BOTTOM: from the chart bottom.
 * @GOG_SERIES_FILL_TYPE_LEFT: from the chart left side.
 * @GOG_SERIES_FILL_TYPE_TOP: from the chart top.
 * @GOG_SERIES_FILL_TYPE_RIGHT:, from the chart right side.
 * @GOG_SERIES_FILL_TYPE_ORIGIN: from the origin (for radar and polar plots).
 * @GOG_SERIES_FILL_TYPE_CENTER: from the center (for radar and polar plots).
 * @GOG_SERIES_FILL_TYPE_EDGE: from the edge (for radar and polar plots).
 * @GOG_SERIES_FILL_TYPE_SELF: close the series curve and fills it.
 * @GOG_SERIES_FILL_TYPE_NEXT: from next series.
 * @GOG_SERIES_FILL_TYPE_X_AXIS_MIN: from X-axis minimum.
 * @GOG_SERIES_FILL_TYPE_X_AXIS_MAX: from X-axis maximum.
 * @GOG_SERIES_FILL_TYPE_Y_AXIS_MIN: from Y-axis minimum.
 * @GOG_SERIES_FILL_TYPE_Y_AXIS_MAX: from Y-axis maximum.
 * @GOG_SERIES_FILL_TYPE_INVALID: invalid, should not happen.
 **/

/**
 * GogSeriesClass:
 * @base: base class
 * @has_interpolation: supports interpolation.
 * @has_fill_type: %TRUE if filling is supported.
 * @valid_fill_type_list: list of supported #GogSeriesFillType values.
 * @series_element_type: #GType for the series element if supported.
 * @dim_changed: called when data changed for the series.
 * @get_xy_data: get X and Y data.
 * @get_interpolation_params: get interpolation parameters if any, only applies
 * to constrained cubic spline interpolation.
 **/

/**
 * GogSeriesElementClass:
 * @base: base class
 * @gse_populate_editor: populates editor.
 **/

/**
 * GogSeriesPriority:
 * @GOG_SERIES_REQUIRED: it must be there.
 * @GOG_SERIES_SUGGESTED: allocator will fill it in, but use need not.
 * @GOG_SERIES_OPTIONAL: optional data.
 * @GOG_SERIES_ERRORS: optional data for error bars.
 *
 * Applies to data declarations in #GogSeriesDimDesc.
 **/

/**
 * GogSeriesDesc:
 * @dim: dimensions descriptions.
 * @style_fields: the style fields that are meaningful for series.
 * @num_dim: number of dimensions for series.
 **/

/**
 * GogSeriesDimDesc:
 * @name: name.
 * @priority: priority.
 * @is_shared: whether the dimension is shared among the series.
 * @val_type: data type.
 * @ms_type: data type for foreign formats.
 **/

/* Keep in sync with GogSeriesFillType enum */
static struct {
	GogSeriesFillType  type;
	char const 	  *name;
	char const 	  *label;
} _fill_type_infos[] = {
	{GOG_SERIES_FILL_TYPE_Y_ORIGIN,	"y-origin",	N_("Y origin")},
	{GOG_SERIES_FILL_TYPE_X_ORIGIN,	"x-origin",	N_("X origin")},
	{GOG_SERIES_FILL_TYPE_BOTTOM,	"bottom",	N_("Bottom")},
	{GOG_SERIES_FILL_TYPE_LEFT,	"left",		N_("Left")},
	{GOG_SERIES_FILL_TYPE_TOP,	"top",		N_("Top")},
	{GOG_SERIES_FILL_TYPE_RIGHT,	"right",	N_("Right")},
	{GOG_SERIES_FILL_TYPE_ORIGIN,	"origin",	N_("Origin")},
	{GOG_SERIES_FILL_TYPE_CENTER,	"center",	N_("Center")},
	{GOG_SERIES_FILL_TYPE_EDGE,	"edge",		N_("Edge")},
	{GOG_SERIES_FILL_TYPE_SELF,	"self",		N_("Self")},
	{GOG_SERIES_FILL_TYPE_NEXT,	"next",		N_("Next series")},
	{GOG_SERIES_FILL_TYPE_X_AXIS_MIN,	"x-axis-min",	N_("X axis minimum")},
	{GOG_SERIES_FILL_TYPE_X_AXIS_MAX,	"x-axis-max",	N_("X axis maximum")},
	{GOG_SERIES_FILL_TYPE_Y_AXIS_MIN,	"y-axis-min",	N_("Y axis minimum")},
	{GOG_SERIES_FILL_TYPE_Y_AXIS_MAX,	"y-axis-max",	N_("Y axis maximum")},
	{GOG_SERIES_FILL_TYPE_INVALID,	"invalid",	""}
};

#ifdef GOFFICE_WITH_GTK
#endif

int gog_series_get_valid_element_index (GogSeries const *series, int old_index, int desired_index);

/*****************************************************************************/
static GObjectClass *gse_parent_klass;

enum {
	ELEMENT_PROP_0,
	ELEMENT_INDEX
};

static gint element_compare (GogSeriesElement *gse_a, GogSeriesElement *gse_b)
{
	return gse_a->index - gse_b->index;
}

static void
gog_series_element_set_index (GogSeriesElement *gse, int ind)
{
	gse->index = ind;
	go_styled_object_apply_theme (GO_STYLED_OBJECT (gse), gse->base.style);
	go_styled_object_style_changed (GO_STYLED_OBJECT (gse));
}

static void
gog_series_element_set_property (GObject *obj, guint param_id,
				 GValue const *value, GParamSpec *pspec)
{
	GogSeriesElement *gse = GOG_SERIES_ELEMENT (obj);
	GogObject *gobj = GOG_OBJECT (obj);

	switch (param_id) {
	case ELEMENT_INDEX :
		gog_series_element_set_index (gse, g_value_get_int (value));
		if (gobj->parent != NULL) {
			GogSeries *series = GOG_SERIES (gobj->parent);
			series->overrides = g_list_remove (series->overrides, gse);
			series->overrides = g_list_insert_sorted (series->overrides, gse,
				(GCompareFunc) element_compare);
		}
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}

	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
}

static void
gog_series_element_get_property (GObject *obj, guint param_id,
				 GValue *value, GParamSpec *pspec)
{
	GogSeriesElement *gse = GOG_SERIES_ELEMENT (obj);

	switch (param_id) {
	case ELEMENT_INDEX :
		g_value_set_int (value, gse->index);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

#ifdef GOFFICE_WITH_GTK
static void
cb_index_changed (GtkSpinButton *spin_button, GogSeriesElement *element)
{
	int index;
	int value = gtk_spin_button_get_value (spin_button);

	if ((int) element->index == value)
		return;

	index = gog_series_get_valid_element_index (
		GOG_SERIES (gog_object_get_parent (GOG_OBJECT (element))),
		element->index, value);

	if (index != value)
		gtk_spin_button_set_value (spin_button, index);

	g_object_set (element, "index", (int) index, NULL);
}

static void
gog_series_element_populate_editor (GogObject *gobj,
				    GOEditor *editor,
			   GogDataAllocator *dalloc,
			   GOCmdContext *cc)
{
	static guint series_element_pref_page = 1;
	GtkWidget *w, *gse_grid = NULL, *grid;
	GogSeriesElementClass *klass = GOG_SERIES_ELEMENT_GET_CLASS (gobj);

	if (klass->gse_populate_editor)
		gse_grid = (*klass->gse_populate_editor) (gobj, editor, cc);

	(GOG_OBJECT_CLASS(gse_parent_klass)->populate_editor) (gobj, editor, dalloc, cc);

	if (gse_grid == NULL) {
		grid = gtk_grid_new ();
		gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
		gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
		gtk_container_set_border_width (GTK_CONTAINER (grid), 12);
	} else {
		grid = gse_grid;
		gtk_grid_insert_row (GTK_GRID (grid), 0);
	}
	w = gtk_label_new (_("Index:"));
	g_object_set (G_OBJECT (w), "xalign", 0., NULL);
	gtk_grid_attach (GTK_GRID (grid), w, 0, 0, 1, 1);
	w = gtk_spin_button_new_with_range (0, G_MAXINT, 1);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w),
				   GOG_SERIES_ELEMENT(gobj)->index);
	g_signal_connect (G_OBJECT (w),
			  "value_changed",
			  G_CALLBACK (cb_index_changed), gobj);
	gtk_grid_attach (GTK_GRID (grid), w, 1, 0, 1, 1);
	gtk_widget_show_all (grid);

	if (gse_grid == NULL)
		go_editor_add_page (editor, grid, _("Settings"));

	go_editor_set_store_page (editor, &series_element_pref_page);
}
#endif

static void
gog_series_element_init_style (GogStyledObject *gso, GOStyle *style)
{
	GogSeries const *series = GOG_SERIES (GOG_OBJECT (gso)->parent);
	GOStyle *parent_style;

	g_return_if_fail (series != NULL);

	parent_style = go_styled_object_get_style (GO_STYLED_OBJECT (series));
	style->interesting_fields = parent_style->interesting_fields;
	gog_theme_fillin_style (gog_object_get_theme (GOG_OBJECT (gso)),
		style, GOG_OBJECT (gso), GOG_SERIES_ELEMENT (gso)->index,
	        style->interesting_fields);
}

static void
gog_series_element_class_init (GogSeriesElementClass *klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) klass;
	GogObjectClass *gog_klass = (GogObjectClass *) klass;
	GogStyledObjectClass *style_klass = (GogStyledObjectClass *) klass;
	gse_parent_klass = g_type_class_peek_parent (klass);

	gobject_klass->set_property = gog_series_element_set_property;
	gobject_klass->get_property = gog_series_element_get_property;

#ifdef GOFFICE_WITH_GTK
	gog_klass->populate_editor 	= gog_series_element_populate_editor;
#endif
	style_klass->init_style	    	= gog_series_element_init_style;

	gog_klass->use_parent_as_proxy  = TRUE;

	g_object_class_install_property (gobject_klass, ELEMENT_INDEX,
		g_param_spec_int ("index",
			_("Index"),
			_("Index of the corresponding data element"),
			0, G_MAXINT, 0,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT | GOG_PARAM_FORCE_SAVE));
}

GSF_CLASS_ABSTRACT (GogSeriesElement, gog_series_element,
	   gog_series_element_class_init, NULL /*gog_series_element_init*/,
	   GOG_TYPE_STYLED_OBJECT)

/*****************************************************************************/

static gboolean
regression_curve_can_add (GogObject const *parent)
{
	GogSeries *series = GOG_SERIES (parent);
	return (series->acceptable_children & GOG_SERIES_ACCEPT_TREND_LINE) != 0;
}

static void
regression_curve_post_add (GogObject *parent, GogObject *child)
{
	gog_object_request_update (child);
}

static void
regression_curve_pre_remove (GogObject *parent, GogObject *child)
{
}

/*****************************************************************************/

static GObjectClass *series_parent_klass;

enum {
	SERIES_PROP_0,
	SERIES_PROP_HAS_LEGEND,
	SERIES_PROP_INTERPOLATION,
	SERIES_PROP_INTERPOLATION_SKIP_INVALID,
	SERIES_PROP_FILL_TYPE
};

static gboolean
role_series_element_can_add (GogObject const *parent)
{
	GogSeriesClass *klass = GOG_SERIES_GET_CLASS (parent);

	return ((gog_series_get_valid_element_index(GOG_SERIES (parent), -1, 0) >= 0) &&
		(klass->series_element_type > 0));
}

static GogObject *
role_series_element_allocate (GogObject *series)
{
	GogSeriesClass *klass = GOG_SERIES_GET_CLASS (series);
	GType type = klass->series_element_type;
	GogObject *gse;

	if (type == 0)
		return NULL;

	gse = g_object_new (type, NULL);
	if (gse != NULL)
		gog_series_element_set_index (GOG_SERIES_ELEMENT (gse),
			gog_series_get_valid_element_index (GOG_SERIES (series), -1, 0));
	return gse;
}

static void
role_series_element_post_add (GogObject *parent, GogObject *child)
{
	GogSeries *series = GOG_SERIES (parent);
	go_styled_object_set_style (GO_STYLED_OBJECT (child),
		go_styled_object_get_style (GO_STYLED_OBJECT (parent)));
	series->overrides = g_list_insert_sorted (series->overrides, child,
		(GCompareFunc) element_compare);
}

static void
role_series_element_pre_remove (GogObject *parent, GogObject *child)
{
	GogSeries *series = GOG_SERIES (parent);
	series->overrides = g_list_remove (series->overrides, child);
}

static gboolean
role_series_labels_can_add (GogObject const *parent)
{
	GogSeries *series = GOG_SERIES (parent);

	return (series->allowed_pos != 0);
}

static void
role_series_labels_post_add (GogObject *parent, GogObject *child)
{
	GogSeries *series = GOG_SERIES (parent);
	GogSeriesDesc const *desc;
	unsigned i;
	GogSeriesLabels *labels = GOG_SERIES_LABELS (child);
	gog_series_labels_set_allowed_position (labels, series->allowed_pos);
	gog_series_labels_set_default_position (labels, series->default_pos);
	/* default is to show values */

	/* Are there any shared dimensions */
	desc = &series->plot->desc.series;
	for (i = 0; i < desc->num_dim; i++)
		if (desc->dim[i].ms_type == GOG_MS_DIM_VALUES)
			break;
	labels->format = (i != desc->num_dim)?
		                            g_strdup_printf ("%%%u", i):
		                            g_strdup ("");
	labels->supports_percent = GOG_PLOT_GET_CLASS (series->plot)->get_percent != NULL;
}

static void
gog_series_finalize (GObject *obj)
{
	GogSeries *series = GOG_SERIES (obj);

	if (series->values != NULL) {
		gog_dataset_finalize (GOG_DATASET (obj));
		g_free (series->values - 1); /* it was aliased */
		series->values = NULL;
	}

	g_list_free (series->overrides);

	(*series_parent_klass->finalize) (obj);
}

static void
gog_series_set_property (GObject *obj, guint param_id,
			 GValue const *value, GParamSpec *pspec)
{
	GogSeries *series = GOG_SERIES (obj);
	gboolean b_tmp;
	char const *name;
	unsigned int i;

	switch (param_id) {
	case SERIES_PROP_HAS_LEGEND :
		b_tmp = g_value_get_boolean (value);
		if (series->has_legend ^ b_tmp) {
			series->has_legend = b_tmp;
			if (series->plot != NULL)
				gog_plot_request_cardinality_update (series->plot);
		}
		break;
	case SERIES_PROP_INTERPOLATION:
		series->interpolation = go_line_interpolation_from_str (g_value_get_string (value));
		/* check if the interpolation mode is compatible with the plot type ans
		 * replace if not */
		if (gog_plot_axis_set_pref (gog_series_get_plot (series)) & 1 << GOG_AXIS_RADIAL &&
		    !go_line_interpolation_supports_radial (series->interpolation))
			series->interpolation = GO_LINE_INTERPOLATION_SPLINE;
		else if (g_object_get_data (obj, "no-bezier-interpolation") != NULL &&
		         (series->interpolation == GO_LINE_INTERPOLATION_CLOSED_SPLINE ||
		          series->interpolation == GO_LINE_INTERPOLATION_SPLINE ||
		          series->interpolation == GO_LINE_INTERPOLATION_ODF_SPLINE))
			series->interpolation = GO_LINE_INTERPOLATION_CUBIC_SPLINE;
		break;
	case SERIES_PROP_INTERPOLATION_SKIP_INVALID:
		series->interpolation_skip_invalid = g_value_get_boolean (value);
		break;
	case SERIES_PROP_FILL_TYPE:
		name = g_value_get_string (value);
		for (i = 0; i < G_N_ELEMENTS (_fill_type_infos); i++)
			if (strcmp (_fill_type_infos[i].name, name) == 0)
			       series->fill_type = _fill_type_infos[i].type;
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}

	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
}

static void
gog_series_get_property (GObject *obj, guint param_id,
			 GValue *value, GParamSpec *pspec)
{
	GogSeries *series = GOG_SERIES (obj);

	switch (param_id) {
	case SERIES_PROP_HAS_LEGEND :
		g_value_set_boolean (value, series->has_legend);
		break;
	case SERIES_PROP_INTERPOLATION:
		g_value_set_string (value, go_line_interpolation_as_str (series->interpolation));
		break;
	case SERIES_PROP_INTERPOLATION_SKIP_INVALID:
		g_value_set_boolean (value, series->interpolation_skip_invalid);
		break;
	case SERIES_PROP_FILL_TYPE:
		g_value_set_string (value, _fill_type_infos[series->fill_type].name);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

#ifdef GOFFICE_WITH_GTK
static unsigned
make_dim_editor (GtkGrid *grid, unsigned row, GogDataEditor *deditor,
		 char const *name, GogSeriesPriority priority, gboolean is_shared)
{
	GtkWidget *editor = GTK_WIDGET (deditor);
	char *txt = g_strdup_printf (
		((priority != GOG_SERIES_REQUIRED) ? "(_%s):" : "_%s:"), _(name));
	GtkWidget *label = gtk_label_new_with_mnemonic (txt);
	g_free (txt);

	gtk_grid_attach (grid, label,
		0, row, 1, 1);
	gtk_widget_set_hexpand (editor, TRUE);
	gtk_grid_attach (grid, editor,
		1, row, 1, 1);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), editor);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);

	go_atk_setup_label (label, editor);

	return row + 1;
}

static void
cb_show_in_legend (GtkToggleButton *b, GObject *series)
{
	g_object_set (series,
		"has-legend", gtk_toggle_button_get_active (b),
		NULL);
}

static void
cb_line_interpolation_changed (GtkComboBox *box, GogSeries *series)
{
	GtkBuilder *gui = g_object_get_data (G_OBJECT (box), "gui");
	GtkWidget *widget = GTK_WIDGET (g_object_get_data (G_OBJECT(box), "skip-button"));
	GtkWidget *grid = go_gtk_builder_get_widget (gui, "clamps-grid");
	GtkTreeIter iter;
	gtk_combo_box_get_active_iter (box, &iter);
	gtk_tree_model_get (gtk_combo_box_get_model (box), &iter, 1, &series->interpolation, -1);
	gtk_widget_set_sensitive (widget, !go_line_interpolation_auto_skip (series->interpolation));
	widget = GTK_WIDGET (g_object_get_data (G_OBJECT(box), "fill-type"));
	if (series->interpolation == GO_LINE_INTERPOLATION_CLAMPED_CUBIC_SPLINE)
		gtk_widget_show (grid);
	else
		gtk_widget_hide (grid);
	if (widget)
		gtk_widget_set_sensitive (widget, !go_line_interpolation_auto_skip (series->interpolation));
	gog_object_emit_changed (GOG_OBJECT (series), FALSE);
}

static void
cb_line_interpolation_skip_changed (GtkToggleButton *btn, GogSeries *series)
{
	series->interpolation_skip_invalid = gtk_toggle_button_get_active (btn);
	gog_object_emit_changed (GOG_OBJECT (series), FALSE);
}

static void
cb_fill_type_changed (GtkComboBox *combo, GObject *obj)
{
	gog_series_set_fill_type (GOG_SERIES (obj),
				  gog_series_get_fill_type_from_combo (GOG_SERIES (obj), combo));
}

static void
gog_series_populate_editor (GogObject *gobj,
			    GOEditor *editor,
		   GogDataAllocator *dalloc,
		   GOCmdContext *cc)
{
	static guint series_pref_page = 1;
	GtkWidget *w;
	GtkGrid  *grid;
	unsigned i, row = 0;
	gboolean has_shared = FALSE;
	GogSeries *series = GOG_SERIES (gobj);
	GogSeriesClass *series_class = GOG_SERIES_GET_CLASS (series);
	GogDataset *set = GOG_DATASET (gobj);
	GogSeriesDesc const *desc;
	GogDataType data_type;
	GtkComboBox *combo = NULL;

	g_return_if_fail (series->plot != NULL);

	/* Are there any shared dimensions */
	desc = &series->plot->desc.series;
	for (i = 0; i < desc->num_dim; i++)
		if (desc->dim[i].is_shared) {
			has_shared = TRUE;
			break;
		}

	w = gtk_grid_new ();
	grid = GTK_GRID (w);
	gtk_grid_set_row_spacing (grid, 6);
	gtk_grid_set_column_spacing (grid, 12);
	gtk_container_set_border_width (GTK_CONTAINER (grid), 12);

	row = make_dim_editor (grid, row,
		gog_data_allocator_editor (dalloc, set, -1, GOG_DATA_SCALAR),
		N_("Name"), TRUE, FALSE);

	/* first the unshared entries */
	for (i = 0; i < desc->num_dim; i++) {
		data_type = (desc->dim[i].val_type == GOG_DIM_MATRIX)?
				GOG_DATA_MATRIX: GOG_DATA_VECTOR;
		if (!desc->dim[i].is_shared && (desc->dim[i].priority != GOG_SERIES_ERRORS))
			row = make_dim_editor (grid, row,
				gog_data_allocator_editor (dalloc, set, i, data_type),
				desc->dim[i].name, desc->dim[i].priority, FALSE);
	}

	if (has_shared) {
		gtk_grid_attach (grid, gtk_separator_new (GTK_ORIENTATION_HORIZONTAL),
			0, row, 2, 1);
		row++;
	}

	/* then the shared entries */
	for (i = 0; i < desc->num_dim; i++) {
		data_type = (desc->dim[i].val_type == GOG_DIM_MATRIX)?
				GOG_DATA_MATRIX: GOG_DATA_VECTOR;
		if (desc->dim[i].is_shared)
			row = make_dim_editor (grid, row,
				gog_data_allocator_editor (dalloc, set, i, data_type),
				desc->dim[i].name, desc->dim[i].priority, TRUE);
	}

	gtk_grid_attach (grid, gtk_separator_new (GTK_ORIENTATION_HORIZONTAL),
		0, row, 2, 1);
	row++;
	w = gtk_check_button_new_with_mnemonic (_("_Show in Legend"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
		gog_series_has_legend (series));
	g_signal_connect (G_OBJECT (w),
		"toggled",
		G_CALLBACK (cb_show_in_legend), series);
	gtk_grid_attach (grid, w,
		0, row, 2, 1);
	gtk_widget_show_all (GTK_WIDGET (grid));

	go_editor_add_page (editor, GTK_WIDGET (grid), _("Data"));

	(GOG_OBJECT_CLASS(series_parent_klass)->populate_editor) (gobj, editor, dalloc, cc);

	grid = GTK_GRID (go_editor_get_registered_widget (editor, "line-grid"));
	if (grid == NULL)
		grid = GTK_GRID (go_editor_get_registered_widget (editor, "outline-grid"));
	if (series_class->has_interpolation && grid != NULL) {
		GtkBuilder *gui;
		GtkWidget *widget;

		gui = go_gtk_builder_load_internal ("res:go:graph/gog-series-prefs.ui", GETTEXT_PACKAGE, cc);
		if (gui != NULL) {
			unsigned i;
			GogAxisSet set = gog_plot_axis_set_pref (gog_series_get_plot (series));
			GogDataset *clamp_set = gog_series_get_interpolation_params (series);
			GtkListStore *model;
			GtkTreeIter iter;
			GtkCellRenderer *renderer;
			/* FIXME: change when we can filter out non supported modes, see #698100, comment #3 */
			gboolean no_bezier = g_object_get_data (G_OBJECT (gobj), "no-bezier-interpolation") != NULL;
			widget = go_gtk_builder_get_widget (gui, "interpolation-prefs");
			gtk_grid_attach (grid, widget, 0, 3, 4, 1);
			/* create an interpolation type combo and populate it */
			combo = GTK_COMBO_BOX (gtk_builder_get_object (gui, "interp-combo"));
			renderer = gtk_cell_renderer_text_new ();
			gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, FALSE);
			gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,
					"text", 0,
					NULL);
			model = GTK_LIST_STORE (gtk_combo_box_get_model (combo));
			for (i = 0; i < GO_LINE_INTERPOLATION_MAX; i++) {
				if ((set & 1 << GOG_AXIS_RADIAL && !go_line_interpolation_supports_radial (i)) ||
				    (no_bezier &&
					    (i == GO_LINE_INTERPOLATION_CLOSED_SPLINE ||
					     i == GO_LINE_INTERPOLATION_SPLINE ||
					     i == GO_LINE_INTERPOLATION_ODF_SPLINE)))
					continue;
				gtk_list_store_append (model, &iter);
				gtk_list_store_set (model, &iter,
				                    0, _(go_line_interpolation_as_label (i)),
				                    1, i, -1);
				if (i == series->interpolation)
					gtk_combo_box_set_active_iter (combo, &iter);
			}
			g_signal_connect (combo, "changed",
					  G_CALLBACK (cb_line_interpolation_changed), series);
			w = go_gtk_builder_get_widget (gui, "interpolation-skip-invalid");
			g_object_set_data (G_OBJECT (combo), "skip-button", w);
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), series->interpolation_skip_invalid);
			if (go_line_interpolation_auto_skip (series->interpolation))
				gtk_widget_set_sensitive (w, FALSE);
			g_signal_connect (w, "toggled",
					  G_CALLBACK (cb_line_interpolation_skip_changed), series);
			gtk_widget_show_all (widget);
			/* FIXME: a very bad hack to hide this check box for line and area plots */
			if (no_bezier)
				gtk_widget_hide (w);
			if (clamp_set) {
				GtkWidget *w;
				widget = go_gtk_builder_get_widget (gui, "clamps-grid");
				w = GTK_WIDGET (gog_data_allocator_editor (dalloc, clamp_set, 0, GOG_DATA_SCALAR));
				gtk_widget_set_tooltip_text (w, _("Derivative at first point of the clamped cubic spline."));
				gtk_widget_show (w);
				gtk_widget_set_hexpand (w, TRUE);
				gtk_grid_attach (GTK_GRID (widget), w, 1, 0, 2, 1);
				w = GTK_WIDGET (gog_data_allocator_editor (dalloc, clamp_set, 1, GOG_DATA_SCALAR));
				gtk_widget_set_tooltip_text (w, _("Derivative at last point of the clamped cubic spline."));
				gtk_widget_show (w);
				gtk_widget_set_hexpand (w, TRUE);
				gtk_grid_attach (GTK_GRID (widget), w, 1, 1, 2, 1);
				if (series->interpolation != GO_LINE_INTERPOLATION_CLAMPED_CUBIC_SPLINE)
					gtk_widget_hide (widget);
			}
			g_object_set_data (G_OBJECT (combo), "gui", gui);
			g_signal_connect_swapped (G_OBJECT (combo), "destroy", G_CALLBACK (g_object_unref), gui);
		}
	}

	grid = GTK_GRID (go_editor_get_registered_widget (editor, "fill-grid"));
	if (series_class->has_fill_type && grid != NULL) {
		GtkBuilder *gui;
		GtkWidget *widget;

		gui = go_gtk_builder_load_internal ("res:go:graph/gog-series-prefs.ui", GETTEXT_PACKAGE, cc);
		if (gui != NULL) {
			widget = go_gtk_builder_get_widget (gui, "fill_type_combo");
				gog_series_populate_fill_type_combo (GOG_SERIES (series), GTK_COMBO_BOX (widget));
			g_signal_connect (G_OBJECT (widget), "changed",
					  G_CALLBACK (cb_fill_type_changed), series);
			if (combo)
				g_object_set_data (G_OBJECT (combo), "fill-type", widget);
			if (series->interpolation == GO_LINE_INTERPOLATION_CLOSED_SPLINE)
				gtk_widget_set_sensitive (widget, FALSE);
			widget = go_gtk_builder_get_widget (gui, "fill_type_prefs");
			gtk_grid_attach (grid, widget, 0, 1, 3, 1);
			g_object_set_data (G_OBJECT (widget), "gui", gui);

			g_signal_connect_swapped (G_OBJECT (widget), "destroy",
						  G_CALLBACK (g_object_unref), gui);
		}
	}

	go_editor_set_store_page (editor, &series_pref_page);
}
#endif

static void
gog_series_update (GogObject *obj)
{
	GogSeries *series = GOG_SERIES (obj);
	GogObjectRole const *role = gog_object_find_role_by_name (obj, "Data labels");
	GSList *l = gog_object_get_children (obj, role), *ptr;
	series->needs_recalc = FALSE;
	for (ptr = l; ptr; ptr = ptr->next)
		gog_object_request_update (ptr->data);
	g_slist_free (l);
}

static void
gog_series_child_added (GogObject *parent, GogObject *child)
{
	if (GOG_IS_TREND_LINE (child))
		gog_plot_request_cardinality_update (GOG_SERIES (parent)->plot);
}

static void
gog_series_init_style (GogStyledObject *gso, GOStyle *style)
{
	GogSeries const *series = (GogSeries const *)gso;
	style->interesting_fields = series->plot->desc.series.style_fields;
	gog_theme_fillin_style (gog_object_get_theme (GOG_OBJECT (gso)),
		style, GOG_OBJECT (gso), series->index,
	        style->interesting_fields);
}

static void
gog_series_class_init (GogSeriesClass *klass)
{
	static GogObjectRole const roles[] = {
		{ N_("Point"), "GogSeriesElement",	0,
		  GOG_POSITION_SPECIAL, GOG_POSITION_SPECIAL, GOG_OBJECT_NAME_BY_ROLE,
		  role_series_element_can_add, NULL,
		  role_series_element_allocate,
		  role_series_element_post_add,
		  role_series_element_pre_remove, NULL },
		{ N_("Regression curve"), "GogRegCurve",	1,
		  GOG_POSITION_SPECIAL, GOG_POSITION_SPECIAL, GOG_OBJECT_NAME_BY_TYPE,
		  regression_curve_can_add,
		  NULL,
		  NULL,
		  regression_curve_post_add,
		  regression_curve_pre_remove,
		  NULL },
		{ N_("Trend line"), "GogTrendLine",	2,
		  GOG_POSITION_SPECIAL, GOG_POSITION_SPECIAL, GOG_OBJECT_NAME_BY_TYPE,
		  regression_curve_can_add,
		  NULL,
		  NULL,
		  regression_curve_post_add,
		  regression_curve_pre_remove,
		  NULL },
		{ N_("Data labels"), "GogSeriesLabels",	3,
		  GOG_POSITION_SPECIAL, GOG_POSITION_SPECIAL, GOG_OBJECT_NAME_BY_ROLE,
		  role_series_labels_can_add,
		  NULL,
		  NULL,
		  role_series_labels_post_add,
		  NULL,
		  NULL }
	};
	unsigned int i;

	GObjectClass *gobject_klass = (GObjectClass *) klass;
	GogObjectClass *gog_klass = (GogObjectClass *) klass;
	GogStyledObjectClass *style_klass = (GogStyledObjectClass *) klass;

	series_parent_klass = g_type_class_peek_parent (klass);
	gobject_klass->finalize		= gog_series_finalize;
	gobject_klass->set_property	= gog_series_set_property;
	gobject_klass->get_property	= gog_series_get_property;
	klass->has_interpolation	= FALSE;

#ifdef GOFFICE_WITH_GTK
	gog_klass->populate_editor	= gog_series_populate_editor;
#endif
	gog_klass->update		= gog_series_update;
	gog_klass->child_added		= gog_series_child_added;
	gog_klass->child_removed	= gog_series_child_added;
	style_klass->init_style 	= gog_series_init_style;
	/* series do not have views, so just forward signals from the plot */
	gog_klass->use_parent_as_proxy  = TRUE;

	gog_object_register_roles (gog_klass, roles, G_N_ELEMENTS (roles));

	g_object_class_install_property (gobject_klass, SERIES_PROP_HAS_LEGEND,
		g_param_spec_boolean ("has-legend",
			_("Has-legend"),
			_("Should the series show up in legends"),
			TRUE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
        g_object_class_install_property (gobject_klass, SERIES_PROP_INTERPOLATION,
		 g_param_spec_string ("interpolation",
			_("Interpolation"),
			_("Type of line interpolation"),
			"linear",
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT | GOG_PARAM_FORCE_SAVE));
	g_object_class_install_property (gobject_klass, SERIES_PROP_INTERPOLATION_SKIP_INVALID,
		g_param_spec_boolean ("interpolation-skip-invalid",
			_("Interpolation skip invalid"),
			_("Should the series interpolation ignore the invalid data"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, SERIES_PROP_FILL_TYPE,
		g_param_spec_string ("fill-type",
			_("Fill type"),
			_("How to fill the area"),
			"invalid",
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	klass->valid_fill_type_list = NULL;

	/* Check for consistency between enum and infos */
	for (i = 0; i < G_N_ELEMENTS (_fill_type_infos); i++)
		g_assert (_fill_type_infos[i].type == i);
}

static void
gog_series_init (GogSeries *series)
{
	series->is_valid = FALSE;
	series->has_legend = TRUE;
	series->plot = NULL;
	series->values = NULL;
	series->index = -1;
	series->acceptable_children = 0;
	series->interpolation = GO_LINE_INTERPOLATION_LINEAR;
	series->default_pos = GOG_SERIES_LABELS_CENTERED;
}

static void
gog_series_dataset_dims (GogDataset const *set, int *first, int *last)
{
	GogSeries const *series = GOG_SERIES (set);
	*first = -1;
	if (series->plot == NULL || series->values == NULL)
		*last = -2;
	else
		*last = (series->plot->desc.series.num_dim - 1);
}

static GogDatasetElement *
gog_series_dataset_get_elem (GogDataset const *set, int dim_i)
{
	GogSeries const *series = GOG_SERIES (set);

	g_return_val_if_fail (dim_i >= -1, NULL);

	if (dim_i >= (int)series->plot->desc.series.num_dim)
		return NULL;
	return series->values + dim_i;
}

static void
gog_series_dataset_set_dim (GogDataset *set, int dim_i,
			    GOData *val, GError **err)
{
	GogSeriesDesc const *desc;
	GogSeries *series = GOG_SERIES (set);
	GogGraph *graph = gog_object_get_graph (GOG_OBJECT (series));

	g_return_if_fail (GOG_IS_PLOT (series->plot));
	g_return_if_fail (dim_i >= -1);

	if (dim_i < 0) {
		char *name = NULL;
		if (NULL != series->values[-1].data)
			name = go_data_get_scalar_string (series->values[-1].data);
		gog_object_set_name (GOG_OBJECT (series), name, err);
		return;
	}

	gog_series_check_validity (series);

	/* clone shared dimensions into other series in the plot, and
	 * invalidate if necessary */
	desc = &series->plot->desc.series;
	g_return_if_fail (dim_i < (int) desc->num_dim);

	if (desc->dim[dim_i].is_shared) {
		GSList *ptr = series->plot->series;

		val = series->values[dim_i].data;
		for (; ptr != NULL ; ptr = ptr->next) {
			gog_dataset_set_dim_internal (GOG_DATASET (ptr->data),
				dim_i, val, graph);
			gog_series_check_validity (GOG_SERIES (ptr->data));
		}
	}
}

static void
gog_series_dataset_dim_changed (GogDataset *set, int dim_i)
{
	GogSeries *series = GOG_SERIES (set);

	if (dim_i >= 0) {
		GogSeriesClass	*klass = GOG_SERIES_GET_CLASS (set);
		GogPlot *plot = GOG_PLOT (GOG_OBJECT (set)->parent);
		/* FIXME: we probaly need a signal which will be connected
		 * to axis and legend objects and let them check if resize
		 * is really needed (similar to child-name-changed
		 * connected to legend). For now, let resize for every label
		 * change */
		gboolean resize = plot != NULL ?
			plot->desc.series.dim[dim_i].val_type == GOG_DIM_LABEL :
			FALSE;

		if (!series->needs_recalc) {
			series->needs_recalc = TRUE;
			gog_object_emit_changed (GOG_OBJECT (set), resize);
		}
		if (klass->dim_changed != NULL)
			(klass->dim_changed) (GOG_SERIES (set), dim_i);

		gog_object_request_update (GOG_OBJECT (set));
	} else {
		GOData *name_src = series->values[-1].data;
		char *name = (name_src != NULL)
			? go_data_get_scalar_string (name_src) : NULL;
		gog_object_set_name (GOG_OBJECT (set), name, NULL);
	}
	gog_series_check_validity (series);
}

static void
gog_series_dataset_init (GogDatasetClass *iface)
{
	iface->get_elem	   = gog_series_dataset_get_elem;
	iface->set_dim	   = gog_series_dataset_set_dim;
	iface->dims	   = gog_series_dataset_dims;
	iface->dim_changed = gog_series_dataset_dim_changed;
}

GSF_CLASS_FULL (GogSeries, gog_series,
		NULL, NULL, gog_series_class_init, NULL,
		gog_series_init, GOG_TYPE_STYLED_OBJECT, G_TYPE_FLAG_ABSTRACT,
		GSF_INTERFACE (gog_series_dataset_init, GOG_TYPE_DATASET))

/**
 * gog_series_get_plot:
 * @series: #GogSeries
 *
 * Returns: (transfer none): the possibly NULL plot that contains this series.
 **/
GogPlot *
gog_series_get_plot (GogSeries const *series)
{
	g_return_val_if_fail (GOG_IS_SERIES (series), NULL);
	return series->plot;
}

/**
 * gog_series_is_valid:
 * @series: #GogSeries
 *
 * Returns: the current cached validity.  Does not recheck
 **/
gboolean
gog_series_is_valid (GogSeries const *series)
{
	g_return_val_if_fail (GOG_IS_SERIES (series), FALSE);
	return series->is_valid;
}

/**
 * gog_series_check_validity:
 * @series: #GogSeries
 *
 * Updates the is_valid flag for a series.
 * This is an internal utility that should not really be necessary for general
 * usage.
 **/
void
gog_series_check_validity (GogSeries *series)
{
	unsigned i;
	GogSeriesDesc const *desc;

	g_return_if_fail (GOG_IS_SERIES (series));
	g_return_if_fail (GOG_IS_PLOT (series->plot));

	desc = &series->plot->desc.series;
	for (i = series->plot->desc.series.num_dim; i-- > 0; )
		if (desc->dim[i].priority == GOG_SERIES_REQUIRED && (
		      series->values[i].data == NULL ||
		      !go_data_has_value (series->values[i].data))) {
			series->is_valid = FALSE;
			return;
		}
	series->is_valid = TRUE;
}

/**
 * gog_series_has_legend:
 * @series: #GogSeries
 *
 * Returns: %TRUE if the series has a visible legend entry
 **/
gboolean
gog_series_has_legend (GogSeries const *series)
{
	g_return_val_if_fail (GOG_IS_SERIES (series), FALSE);
	return series->has_legend;
}

/**
 * gog_series_set_index:
 * @series: #GogSeries
 * @ind: >= 0 assigns a new index, < 0 resets to auto
 * @is_manual: gboolean
 *
 * If @ind >= 0 attempt to assign the new index.  Auto
 * indicies (@is_manual == FALSE) will not override the current
 * index if it is manual.  An @index < 0, will reset the index to
 * automatic and potentially queue a revaluation of the parent
 * chart's cardinality.
 **/
void
gog_series_set_index (GogSeries *series, int ind, gboolean is_manual)
{
	g_return_if_fail (GOG_IS_SERIES (series));

	if (ind < 0) {
		if (series->manual_index && series->plot != NULL)
			gog_plot_request_cardinality_update (series->plot);
		series->manual_index = FALSE;
		return;
	}

	if (is_manual)
		series->manual_index = TRUE;
	else if (series->manual_index)
		return;

	series->index = ind;
	go_styled_object_apply_theme (GO_STYLED_OBJECT (series), series->base.style);
	go_styled_object_style_changed (GO_STYLED_OBJECT (series));
}

/**
 * gog_series_get_name:
 * @series: a #GogSeries
 *
 * Gets the _source_ of the name associated with the series.
 * NOTE : this is _NOT_ the actual name.
 *
 * return value: (transfer none): a #GODataScalar, without added reference.
 **/
GOData *
gog_series_get_name (GogSeries const *series)
{
	g_return_val_if_fail (GOG_IS_SERIES (series), NULL);

	return series->values[-1].data;
}

/**
 * gog_series_set_name:
 * @series: a #GogSeries
 * @name_src: (transfer full): a #GODataScalar
 * @err: a #GError
 *
 * Absorbs a ref to @name_src.
 *
 **/
void
gog_series_set_name (GogSeries *series, GODataScalar *name_src, GError **err)
{
	gog_dataset_set_dim (GOG_DATASET (series), -1, GO_DATA (name_src), err);
}

/**
 * gog_series_set_dim:
 * @series: #GogSeries
 * @dim_i: Which dimension
 * @val: (transfer full): #GOData
 * @err: (allow-none): optional #GError pointer
 *
 * Absorbs a ref to @val
 **/
void
gog_series_set_dim (GogSeries *series, int dim_i, GOData *val, GError **err)
{
	gog_dataset_set_dim (GOG_DATASET (series), dim_i, val, err);
}

int
gog_series_map_XL_dim (GogSeries const *series, GogMSDimType ms_type)
{
	GogSeriesDesc const *desc = &series->plot->desc.series;
	unsigned i = desc->num_dim;

	if (ms_type == GOG_MS_DIM_LABELS)
		return -1;
	while (i-- > 0)
		if (desc->dim[i].ms_type == ms_type)
			return i;
	return -2;
}

void
gog_series_set_XL_dim (GogSeries *series, GogMSDimType ms_type, GOData *val, GError **err)
{
	int dim;
	g_return_if_fail (series !=NULL);
	dim = gog_series_map_XL_dim (series, ms_type);
	if (dim >= -1) {
		gog_series_set_dim (series, dim, val, err);
		return;
	}
	g_object_unref (val);
}

/**
 * gog_series_num_elements:
 * @series: #GogSeries
 *
 * Returns: the number of elements in the series
 **/
unsigned
gog_series_num_elements (GogSeries const *series)
{
	return series->num_elements;
}

/**
 * gog_series_get_overrides:
 * @series: #GogSeries
 *
 * Overrides are data owning their own style, overriding the series style.
 * Returns: (element-type GogSeriesElement) (transfer none): the series
 * overrides.
 **/
GList const *
gog_series_get_overrides (GogSeries const *series)
{
	return series->overrides;
}

int
gog_series_get_valid_element_index (GogSeries const *series, int old_index, int desired_index)
{
	int index;
	GList *ptr;

	g_return_val_if_fail (GOG_IS_SERIES (series), -1);

	if ((desired_index >= (int) series->num_elements) ||
	    (desired_index < 0))
		return old_index;

	if (desired_index > old_index)
		for (ptr = series->overrides; ptr != NULL; ptr = ptr->next) {
			index = GOG_SERIES_ELEMENT (ptr->data)->index;
			if (index > desired_index)
				break;
			if (index == desired_index)
				desired_index++;
		}
	else
		for (ptr = g_list_last (series->overrides); ptr != NULL; ptr = ptr->prev) {
			index = GOG_SERIES_ELEMENT (ptr->data)->index;
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

/**
 * gog_series_get_element:
 * @series: #GogSeries
 * @index: the element index
 *
 * Returns: (transfer none): the #GogSeriesElement corresponding to @index if
 * any
 **/
GogSeriesElement *
gog_series_get_element (GogSeries const *series, int index)
{
	GList *ptr;
	GogSeriesElement *element;

	g_return_val_if_fail (GOG_IS_SERIES (series), NULL);

	for (ptr = series->overrides; ptr != NULL; ptr = ptr->next) {
		element = GOG_SERIES_ELEMENT (ptr->data);
		if ((int) element->index == index)
			return element;
	}

	return NULL;
}

static unsigned int
gog_series_get_data (GogSeries const *series, int *indices, double **data, int n_vectors)
{
	GOData *vector;
	int i, n_points = 0, vector_n_points;
	int first, last;
	int index;
	gboolean is_set = FALSE;

	if (!gog_series_is_valid (series))
		return 0;

	gog_dataset_dims (GOG_DATASET (series), &first, &last);

	for (i = 0; i < n_vectors; i++) {
		index = indices != NULL ? indices[i] : i;
		if (index >= first && index <= last &&
		    (vector = series->values[index].data) != NULL) {
			data[i] = (go_data_has_value (vector))? go_data_get_values (vector): NULL;
			vector_n_points = go_data_get_vector_size (vector);
			if (!is_set) {
				is_set = TRUE;
				n_points = vector_n_points;
			} else
				n_points = MIN (vector_n_points, n_points);
		} else
			data[i] = NULL;
	}

	return n_points;
}

unsigned
gog_series_get_xy_data (GogSeries const  *series,
			double const 	**x,
			double const 	**y)
{
	GogSeriesClass	*klass = GOG_SERIES_GET_CLASS (series);
	double *data[2];
	unsigned int n_points;
	int first, last;

	g_return_val_if_fail (klass != NULL, 0);

	if (klass->get_xy_data != NULL) {
		if (!gog_series_is_valid (GOG_SERIES (series)))
			return 0;

		return (klass->get_xy_data) (series, x, y);
	}

	gog_dataset_dims (GOG_DATASET (series), &first, &last);

	g_return_val_if_fail (first <= 0, 0);
	g_return_val_if_fail (last >= 1, 0);

	n_points = gog_series_get_data (series, NULL, data, 2);

	*x = data[0];
	*y = data[1];

	return n_points;
}

unsigned
gog_series_get_xyz_data (GogSeries const  *series,
			 double const 	 **x,
			 double const 	 **y,
			 double const 	 **z)
{
	double *data[3];
	unsigned int n_points;

	n_points = gog_series_get_data (series, NULL, data, 3);

	*x = data[0];
	*y = data[1];
	*z = data[2];

	return n_points;
}

GogSeriesFillType
gog_series_get_fill_type (GogSeries const *series)
{
	g_return_val_if_fail (GOG_IS_SERIES (series), GOG_SERIES_FILL_TYPE_INVALID);

	return series->fill_type;
}

void
gog_series_set_fill_type (GogSeries *series, GogSeriesFillType fill_type)
{
	GogSeriesClass *series_klass;

	g_return_if_fail (GOG_IS_SERIES (series));
	if (series->fill_type == fill_type)
		return;
	g_return_if_fail (fill_type >= 0 && fill_type < GOG_SERIES_FILL_TYPE_INVALID);

	series_klass = GOG_SERIES_GET_CLASS (series);
	g_return_if_fail (series_klass->valid_fill_type_list != NULL);

	series->fill_type = fill_type;
	gog_object_request_update (GOG_OBJECT (series));
}

#ifdef GOFFICE_WITH_GTK
void
gog_series_populate_fill_type_combo (GogSeries const *series, GtkComboBox *combo)
{
	GogSeriesClass *series_klass;
	GogSeriesFillType fill_type;
	unsigned int i;

	g_return_if_fail (GOG_IS_SERIES (series));
	series_klass = GOG_SERIES_GET_CLASS (series);
	g_return_if_fail (series_klass->valid_fill_type_list != NULL);

	gtk_list_store_clear (GTK_LIST_STORE (gtk_combo_box_get_model (combo)));
	for (i = 0; series_klass->valid_fill_type_list[i] != GOG_SERIES_FILL_TYPE_INVALID; i++) {
		fill_type = series_klass->valid_fill_type_list[i];
		if (fill_type < GOG_SERIES_FILL_TYPE_INVALID) {
			go_gtk_combo_box_append_text (combo, _(_fill_type_infos[fill_type].label));
			if (fill_type == series->fill_type)
				gtk_combo_box_set_active (combo, i);
		}
	}
}

GogSeriesFillType
gog_series_get_fill_type_from_combo (GogSeries const *series, GtkComboBox *combo)
{
	GogSeriesClass *series_klass;

	g_return_val_if_fail (GOG_IS_SERIES (series), GOG_SERIES_FILL_TYPE_INVALID);
	series_klass = GOG_SERIES_GET_CLASS (series);
	g_return_val_if_fail (series_klass->valid_fill_type_list != NULL, GOG_SERIES_FILL_TYPE_INVALID);

	return series_klass->valid_fill_type_list[gtk_combo_box_get_active (combo)];
}
#endif

/**
 * gog_series_get_interpolation_params:
 * @series: #GogSeries
 *
 * Only constrained cubic spline interpolation use parameters (for the
 * slopes at each end).
 * Returns: (transfer none): the interpolation parameters.
 **/
GogDataset *
gog_series_get_interpolation_params (GogSeries const *series)
{
	GogSeriesClass *series_klass;

	g_return_val_if_fail (GOG_IS_SERIES (series), NULL);
	series_klass = GOG_SERIES_GET_CLASS (series);
	return (series_klass->get_interpolation_params)?
			series_klass->get_interpolation_params (series):
			NULL;
}
