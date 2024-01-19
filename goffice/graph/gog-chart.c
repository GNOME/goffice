/*
 * gog-chart.c :
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
#include <goffice/graph/gog-chart-impl.h>
#include <goffice/graph/gog-chart-map.h>
#include <goffice/graph/gog-chart-map-3d.h>
#include <goffice/graph/gog-3d-box.h>
#include <goffice/graph/gog-plot-impl.h>
#include <goffice/graph/gog-graph-impl.h>
#include <goffice/utils/go-style.h>
#include <goffice/graph/gog-view.h>
#include <goffice/graph/gog-axis.h>
#include <goffice/graph/gog-axis-line-impl.h>
#include <goffice/graph/gog-grid.h>
#include <goffice/graph/gog-grid-line.h>
#include <goffice/graph/gog-renderer.h>
#include <goffice/math/go-math.h>
#include <goffice/math/go-matrix3x3.h>
#include <goffice/utils/go-persist.h>

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>
#include <string.h>
#include <math.h>

#ifdef GOFFICE_WITH_LASEM
#include <goffice/graph/gog-equation.h>
#endif

#ifdef GOFFICE_WITH_GTK
#include <goffice/gtk/goffice-gtk.h>
#endif

/**
 * SECTION: gog-chart
 * @short_description: A chart.
 * @See_also: #GogPlot
 *
 * #GogChart wraps one or more #GogPlot objects, so that you can superimpose
 * them on top of each other. In addition, the chart can have a title and a legend.
*/

static const struct {
	char const *name;
	GogAxisSet const axis_set;
} axis_set_desc[] = {
	{ "none",	GOG_AXIS_SET_NONE},
	{ "x",		GOG_AXIS_SET_X},
	{ "xy", 	GOG_AXIS_SET_XY},
	{ "xyz",	GOG_AXIS_SET_XYZ},
	{ "radar",	GOG_AXIS_SET_RADAR},
	{ "pseudo-3d",	GOG_AXIS_SET_XY_pseudo_3d},
	{ "xy-color",	GOG_AXIS_SET_XY_COLOR}
};

GogAxisSet
gog_axis_set_from_str (char const *str)
{
	GogAxisSet axis_set = GOG_AXIS_SET_NONE;
	unsigned i;
	gboolean found = FALSE;

	if (str == NULL)
		return GOG_AXIS_SET_NONE;

	for (i = 0; i < G_N_ELEMENTS (axis_set_desc); i++)
		if (strcmp (axis_set_desc[i].name, str) == 0) {
			axis_set = axis_set_desc[i].axis_set;
			found = TRUE;
			break;
		}
	if (!found)
		g_warning ("[GogAxisSet::from_str] unknown axis set (%s)", str);
	return axis_set;
}

/*****************************************************************************/
enum {
	CHART_PROP_0,
	CHART_PROP_CARDINALITY_VALID,
	CHART_PROP_PLOT_AREA,
	CHART_PROP_PLOT_AREA_IS_MANUAL,
	CHART_PROP_X_POS,
	CHART_PROP_Y_POS,
	CHART_PROP_ROWS,
	CHART_PROP_COLUMNS
};

static GType gog_chart_view_get_type (void);
static GObjectClass *chart_parent_klass;

static void
gog_chart_update (GogObject *obj)
{
	GogChart *chart = GOG_CHART (obj);
	unsigned full = chart->full_cardinality;
	unsigned visible = chart->visible_cardinality;

	gog_chart_get_cardinality (chart, NULL, NULL);

	if (full != chart->full_cardinality ||
	    visible != chart->visible_cardinality)
		g_object_notify (G_OBJECT (chart), "cardinality-valid");
}

static void
gog_chart_finalize (GObject *obj)
{
	GogChart *chart = GOG_CHART (obj);

	/* on exit the role remove routines are not called */
	g_slist_free (chart->plots);
	g_slist_free (chart->axes);

	(chart_parent_klass->finalize) (obj);
}

static void
gog_chart_set_property (GObject *obj, guint param_id,
			 GValue const *value, GParamSpec *pspec)
{
	GogChart *chart = GOG_CHART (obj);
	char **str_doubles;
	char const *str;
	gboolean changed = FALSE;

	switch (param_id) {
	case CHART_PROP_PLOT_AREA:
		str = g_value_get_string (value);
		str_doubles = g_strsplit (str, " ", 4);
		if (g_strv_length (str_doubles) != 4) {
			g_strfreev (str_doubles);
			break;
		}
		chart->plot_area.x = g_ascii_strtod (str_doubles[0], NULL);
		chart->plot_area.y = g_ascii_strtod (str_doubles[1], NULL);
		chart->plot_area.w = g_ascii_strtod (str_doubles[2], NULL);
		chart->plot_area.h = g_ascii_strtod (str_doubles[3], NULL);
		g_strfreev (str_doubles);
		break;
	case CHART_PROP_PLOT_AREA_IS_MANUAL:
		chart->is_plot_area_manual = g_value_get_boolean (value);
		break;
	case CHART_PROP_X_POS:
		chart->x_pos = g_value_get_int (value);
		changed = TRUE;
		break;
	case CHART_PROP_Y_POS:
		chart->y_pos = g_value_get_int (value);
		changed = TRUE;
		break;
	case CHART_PROP_COLUMNS:
		chart->cols = g_value_get_int (value);
		changed = TRUE;
		break;
	case CHART_PROP_ROWS:
		chart->rows = g_value_get_int (value);
		changed = TRUE;
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}

	if (changed) {
		gog_graph_validate_chart_layout (GOG_GRAPH (GOG_OBJECT (chart)->parent));
		gog_object_emit_changed (GOG_OBJECT (obj), TRUE);
	}
}

static void
gog_chart_get_property (GObject *obj, guint param_id,
			GValue *value, GParamSpec *pspec)
{
	GogChart *chart = GOG_CHART (obj);
	GString *string;
	char buffer[G_ASCII_DTOSTR_BUF_SIZE];

	switch (param_id) {
	case CHART_PROP_CARDINALITY_VALID:
		g_value_set_boolean (value, chart->cardinality_valid);
		break;
	case CHART_PROP_PLOT_AREA:
		string = g_string_new ("");
		g_string_append (string, g_ascii_dtostr (buffer, sizeof (buffer), chart->plot_area.x));
		g_string_append_c (string, ' ');
		g_string_append (string, g_ascii_dtostr (buffer, sizeof (buffer), chart->plot_area.y));
		g_string_append_c (string, ' ');
		g_string_append (string, g_ascii_dtostr (buffer, sizeof (buffer), chart->plot_area.w));
		g_string_append_c (string, ' ');
		g_string_append (string, g_ascii_dtostr (buffer, sizeof (buffer), chart->plot_area.h));
		g_value_set_string (value, string->str);
		g_string_free (string, TRUE);
		break;
	case CHART_PROP_PLOT_AREA_IS_MANUAL:
		g_value_set_boolean (value, chart->is_plot_area_manual);
		break;
	case CHART_PROP_X_POS:
		g_value_set_int (value, chart->x_pos);
		break;
	case CHART_PROP_Y_POS:
		g_value_set_int (value, chart->y_pos);
		break;
	case CHART_PROP_COLUMNS:
		g_value_set_int (value, chart->cols);
		break;
	case CHART_PROP_ROWS:
		g_value_set_int (value, chart->rows);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

#ifdef GOFFICE_WITH_GTK
typedef struct {
	GtkBuilder	*gui;
	GtkWidget	*x_spin, *y_spin, *w_spin, *h_spin;
	gulong		 w_spin_signal, h_spin_signal;
	GtkWidget	*position_select_combo;
	GtkWidget	*manual_setting_grid;
	GogChart	*chart;
} PlotAreaPrefState;

static void
plot_area_pref_state_free (PlotAreaPrefState *state)
{
	g_object_unref (state->chart);
	g_object_unref (state->gui);
}

static void
cb_plot_area_changed (GtkWidget *spin, PlotAreaPrefState *state)
{
	GogViewAllocation pos;
	double value;
	double max;

       	value = gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin)) / 100.0;

       	gog_chart_get_plot_area (state->chart, &pos);
	if (spin == state->x_spin) {
		pos.x = value;
		max = 1.0 - pos.x;
		g_signal_handler_block (state->w_spin, state->w_spin_signal);
		gtk_spin_button_set_range (GTK_SPIN_BUTTON (state->w_spin), 0.0, max * 100.0);
		if (pos.w > max) pos.w = max;
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (state->w_spin), pos.w * 100.0);
		g_signal_handler_unblock (state->w_spin, state->w_spin_signal);
	}
	else if (spin == state->y_spin) {
		pos.y = value;
		max = 1.0 - pos.y;
		g_signal_handler_block (state->h_spin, state->h_spin_signal);
		gtk_spin_button_set_range (GTK_SPIN_BUTTON (state->h_spin), 0.0, max * 100.0);
		if (pos.h > max) pos.h = max;
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (state->h_spin), pos.w * 100.0);
		g_signal_handler_unblock (state->h_spin, state->h_spin_signal);
	}
	else if (spin == state->w_spin) {
		pos.w = value;
	}
	else if (spin == state->h_spin) {
		pos.h = value;
	}
	gog_chart_set_plot_area (state->chart, &pos);
	gtk_combo_box_set_active (GTK_COMBO_BOX (state->position_select_combo), 1);
	gtk_widget_show (state->manual_setting_grid);
}

static void
cb_manual_position_changed (GtkComboBox *combo, PlotAreaPrefState *state)
{
	if (gtk_combo_box_get_active (combo) == 1) {
		GogViewAllocation plot_area;

		gog_chart_get_plot_area (state->chart, &plot_area);
		gog_chart_set_plot_area (state->chart, &plot_area);
		gtk_widget_show (state->manual_setting_grid);
	} else {
		gog_chart_set_plot_area (state->chart, NULL);
		gtk_widget_hide (state->manual_setting_grid);
	}
}

static void
gog_chart_populate_editor (GogObject *gobj,
			   GOEditor *editor,
			   G_GNUC_UNUSED GogDataAllocator *dalloc,
			   GOCmdContext *cc)
{
	static guint chart_pref_page = 0;

	GtkBuilder *gui;
	PlotAreaPrefState *state;
	gboolean is_plot_area_manual;
	GogViewAllocation plot_area;
	GtkWidget *w;
	GogChart *chart = GOG_CHART (gobj);

	g_return_if_fail (chart != NULL);

	gui = go_gtk_builder_load_internal ("res:go:graph/gog-plot-prefs.ui", GETTEXT_PACKAGE, cc);
	g_return_if_fail (gui != NULL);

	(GOG_OBJECT_CLASS(chart_parent_klass)->populate_editor) (gobj, editor, dalloc, cc);

	state = g_new  (PlotAreaPrefState, 1);
	state->chart = chart;
	state->gui = gui;

	g_object_ref (chart);
	is_plot_area_manual = gog_chart_get_plot_area (chart, &plot_area);

	state->x_spin = go_gtk_builder_get_widget (gui, "x_spin");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (state->x_spin),
				   plot_area.x * 100.0);
	g_signal_connect (G_OBJECT (state->x_spin), "value-changed",
			  G_CALLBACK (cb_plot_area_changed), state);

	state->y_spin = go_gtk_builder_get_widget (gui, "y_spin");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (state->y_spin),
				   plot_area.y * 100.0);
	g_signal_connect (G_OBJECT (state->y_spin), "value-changed",
			  G_CALLBACK (cb_plot_area_changed), state);

	state->w_spin = go_gtk_builder_get_widget (gui, "w_spin");
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (state->w_spin),
				   0.0, (1.0 - plot_area.x) * 100.0);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (state->w_spin),
				   100.0 * plot_area.w);
	state->w_spin_signal = g_signal_connect (G_OBJECT (state->w_spin), "value-changed",
						 G_CALLBACK (cb_plot_area_changed), state);

	state->h_spin = go_gtk_builder_get_widget (gui, "h_spin");
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (state->h_spin),
				   0.0, (1.0 - plot_area.y) * 100.0);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (state->h_spin),
				   100.0 * plot_area.h);
	state->h_spin_signal = g_signal_connect (G_OBJECT (state->h_spin), "value-changed",
						 G_CALLBACK (cb_plot_area_changed), state);

	state->position_select_combo = go_gtk_builder_get_widget (gui, "position_select_combo");
	gtk_combo_box_set_active (GTK_COMBO_BOX (state->position_select_combo),
				  is_plot_area_manual ? 1 : 0);
	state->manual_setting_grid = go_gtk_builder_get_widget (gui, "manual-setting-grid");
	if (!is_plot_area_manual)
		gtk_widget_hide (state->manual_setting_grid);

	g_signal_connect (G_OBJECT (state->position_select_combo),
			  "changed", G_CALLBACK (cb_manual_position_changed), state);

	w = go_gtk_builder_get_widget (gui, "gog-plot-prefs");
	g_signal_connect_swapped (G_OBJECT (w), "destroy", G_CALLBACK (plot_area_pref_state_free), state);
	go_editor_add_page (editor, w, _("Plot area"));

	go_editor_set_store_page (editor, &chart_pref_page);
}
#endif

static void
gog_chart_children_reordered (GogObject *obj)
{
	GSList *ptr, *accum = NULL;
	GogChart *chart = GOG_CHART (obj);

	for (ptr = obj->children; ptr != NULL ; ptr = ptr->next)
		if (GOG_IS_PLOT (ptr->data))
			accum = g_slist_prepend (accum, ptr->data);
	g_slist_free (chart->plots);
	chart->plots = g_slist_reverse (accum);

	gog_chart_request_cardinality_update (chart);
}

static gboolean
role_plot_can_add (GogObject const *parent)
{
	GogChart *chart = GOG_CHART (parent);
	return (chart->axis_set & (1 << GOG_AXIS_Z)) == 0 || g_slist_length (chart->plots) == 0;
}

static void
role_plot_post_add (GogObject *parent, GogObject *plot)
{
	GogChart *chart = GOG_CHART (parent);
	gboolean ok = TRUE;
	GogPlotClass *plot_klass = GOG_PLOT_CLASS (G_OBJECT_GET_CLASS (plot));
	GogAxisSet axis_set = plot_klass->axis_set & ~GOG_AXIS_SET_FUNDAMENTAL;

	if (axis_set) {
		int i = GOG_AXIS_VIRTUAL, j = 1 << GOG_AXIS_VIRTUAL;
		for (; i < GOG_AXIS_TYPES; i++, j <<= 1)
			if ((axis_set & j) != 0 && (chart->axis_set & j) == 0) {
				GogObject *axis = GOG_OBJECT (g_object_new (GOG_TYPE_AXIS, "type", i, NULL));
				chart->axis_set |= j;
				switch (i) {
				case GOG_AXIS_PSEUDO_3D:
					gog_object_add_by_name (GOG_OBJECT (chart), "Pseudo-3D-Axis", axis);
					break;
				case GOG_AXIS_COLOR:
					gog_object_add_by_name (GOG_OBJECT (chart), "Color-Axis", axis);
					break;
				case GOG_AXIS_BUBBLE:
					gog_object_add_by_name (GOG_OBJECT (chart), "Bubble-Axis", axis);
					break;
				default:
					g_warning ("Unknown axis type: %x\n", i);
				}
			}
	}
	/* APPEND to keep order, there won't be that many */
	chart->plots = g_slist_append (chart->plots, plot);
	gog_chart_request_cardinality_update (chart);

	if (chart->plots->next == NULL)
		ok = gog_chart_axis_set_assign (chart,
			gog_plot_axis_set_pref (GOG_PLOT (plot)));
	ok |= gog_plot_axis_set_assign (GOG_PLOT (plot),
		chart->axis_set);

	/* a quick post condition to keep us on our toes */
	g_return_if_fail (ok);
}

static void
role_plot_pre_remove (GogObject *parent, GogObject *plot)
{
	GogChart *chart = GOG_CHART (parent);
	gog_plot_axis_clear (GOG_PLOT (plot), GOG_AXIS_SET_ALL);
	chart->plots = g_slist_remove (chart->plots, plot);
	gog_chart_request_cardinality_update (chart);

	if (chart->plots == NULL)
		gog_chart_axis_set_assign (chart, GOG_AXIS_SET_UNKNOWN);

	if (chart->grid != NULL &&
	    chart->axis_set != GOG_AXIS_SET_XY &&
	    chart->axis_set != GOG_AXIS_SET_X &&
	    chart->axis_set != GOG_AXIS_SET_XY_pseudo_3d &&
	    chart->axis_set != GOG_AXIS_SET_XY_COLOR &&
	    chart->axis_set != GOG_AXIS_SET_RADAR) {
		GogObject *grid = chart->grid; /* clear_parent clears ::grid */
		gog_object_clear_parent (GOG_OBJECT (grid));
		g_object_unref (grid);
	}
}

static gboolean
role_grid_can_add (GogObject const *parent)
{
	GogChart const *chart = GOG_CHART (parent);
	return chart->grid == NULL &&
		(chart->axis_set == GOG_AXIS_SET_XY ||
		 chart->axis_set == GOG_AXIS_SET_X ||
		 chart->axis_set == GOG_AXIS_SET_XY_pseudo_3d ||
		 chart->axis_set == GOG_AXIS_SET_XY_COLOR ||
		 chart->axis_set == GOG_AXIS_SET_RADAR);
}

static void
role_grid_post_add (GogObject *parent, GogObject *child)
{
	GogChart *chart = GOG_CHART (parent);
	g_return_if_fail (chart->grid == NULL);
	chart->grid = child;
}

static void
role_grid_pre_remove (GogObject *parent, GogObject *grid)
{
	GogChart *chart = GOG_CHART (parent);
	g_return_if_fail (chart->grid == grid);
	chart->grid = NULL;
}

static gboolean
xy_grid_3d_can_add (GogObject const *parent)
{
	return (GOG_CHART (parent)->axis_set == GOG_AXIS_SET_XYZ &&
		NULL == gog_object_get_child_by_name (parent, "XY-Backplane"));
}

static gboolean
yz_grid_3d_can_add (GogObject const *parent)
{
	return (GOG_CHART (parent)->axis_set == GOG_AXIS_SET_XYZ &&
		NULL == gog_object_get_child_by_name (parent, "YZ-Backplane"));
}

static gboolean
zx_grid_3d_can_add (GogObject const *parent)
{
	return (GOG_CHART (parent)->axis_set == GOG_AXIS_SET_XYZ &&
		NULL == gog_object_get_child_by_name (parent, "ZX-Backplane"));
}

static void
grid_3d_post_add (GogObject *child, GogGridType t)
{
	g_object_set (G_OBJECT (child), "type", (int)t, NULL);
}

static void xy_grid_3d_post_add    (GogObject *parent, GogObject *child)
{ grid_3d_post_add (child, GOG_GRID_XY); }
static void yz_grid_3d_post_add    (GogObject *parent, GogObject *child)
{ grid_3d_post_add (child, GOG_GRID_YZ); }
static void zx_grid_3d_post_add    (GogObject *parent, GogObject *child)
{ grid_3d_post_add (child, GOG_GRID_ZX); }

static gboolean
axis_can_add (GogObject const *parent, GogAxisType t)
{
	GogChart *chart = GOG_CHART (parent);
	if (chart->axis_set == GOG_AXIS_SET_UNKNOWN
	    || chart->axis_set == GOG_AXIS_SET_XYZ)
		return FALSE;
	return (chart->axis_set & (1 << t)) != 0;
}

static gboolean
axis_can_remove (GogObject const *child)
{
	return NULL == gog_axis_contributors (GOG_AXIS (child));
}

static void
axis_post_add (GogObject *axis, GogAxisType t)
{
	GogChart *chart = GOG_CHART (axis->parent);
	g_object_set (G_OBJECT (axis), "type", (int)t, NULL);
	chart->axes = g_slist_prepend (chart->axes, axis);

	gog_axis_base_set_position (GOG_AXIS_BASE (axis), GOG_AXIS_AUTO);
}

static void
axis_pre_remove (GogObject *parent, GogObject *child)
{
	GogChart *chart = GOG_CHART (parent);
	GogAxis *axis = GOG_AXIS (child);
	GogColorScale *scale = gog_axis_get_color_scale (axis);
	if (scale)
		gog_color_scale_set_axis (scale, NULL);
	gog_axis_clear_contributors (GOG_AXIS (axis));
	chart->axes = g_slist_remove (chart->axes, axis);
}

/*  Color scale related code */
static gboolean
color_scale_can_add (GogObject const *parent)
{
	/* %TRUE if there are more color or pseudo-3d axes than there are color scales */
	GogChart *chart = GOG_CHART (parent);
	GSList *ptr;
	GogAxis *axis;
	GogAxisType type;

	if ((chart->axis_set & ((1 << GOG_AXIS_COLOR) | (1 << GOG_AXIS_PSEUDO_3D ))) == 0)
		return FALSE;
	for (ptr = chart->axes; ptr && ptr->data; ptr = ptr->next) {
		axis = GOG_AXIS (ptr->data);
		type = gog_axis_get_atype (axis);
		if ((type == GOG_AXIS_COLOR || type == GOG_AXIS_PSEUDO_3D)
		    && gog_axis_get_color_scale (axis) == NULL)
			return TRUE;
	}
	return FALSE;
}

static void
color_scale_post_add (GogObject *parent, GogObject *child)
{
	/* Link the color scale to an axis without a preexisting color scale */
	GogChart *chart = GOG_CHART (parent);
	GSList *ptr;
	GogAxis *axis;
	GogAxisType type;
	GSList const *l;

	for (ptr = chart->axes; ptr && ptr->data; ptr = ptr->next) {
		axis = GOG_AXIS (ptr->data);
		type = gog_axis_get_atype (axis);
		if ((type == GOG_AXIS_COLOR || type == GOG_AXIS_PSEUDO_3D)
		    && gog_axis_get_color_scale (axis) == NULL) {
				gog_color_scale_set_axis (GOG_COLOR_SCALE (child), axis);
				for (l = gog_axis_contributors (axis); l; l = l->next)
					gog_object_request_update (GOG_OBJECT (l->data));
				break;
			}
	}
	gog_chart_request_cardinality_update (chart);
}

static void
color_scale_pre_remove (GogObject *parent, GogObject *scale)
{
	/* Unlink the color scale */
	GSList const *l = gog_axis_contributors (gog_color_scale_get_axis (GOG_COLOR_SCALE (scale)));
	gog_color_scale_set_axis (GOG_COLOR_SCALE (scale), NULL);
	for (; l; l = l->next)
		gog_object_request_update (GOG_OBJECT (l->data));
	gog_chart_request_cardinality_update (GOG_CHART (parent));
}


static gboolean x_axis_can_add (GogObject const *parent) { return axis_can_add (parent, GOG_AXIS_X); }
static void x_axis_post_add    (GogObject *parent, GogObject *child)  { axis_post_add   (child, GOG_AXIS_X); }
static gboolean y_axis_can_add (GogObject const *parent) { return axis_can_add (parent, GOG_AXIS_Y); }
static void y_axis_post_add    (GogObject *parent, GogObject *child)  { axis_post_add   (child, GOG_AXIS_Y); }
static gboolean z_axis_can_add (GogObject const *parent) { return axis_can_add (parent, GOG_AXIS_Z); }
static void z_axis_post_add    (GogObject *parent, GogObject *child)  { axis_post_add   (child, GOG_AXIS_Z); }
static gboolean circular_axis_can_add (GogObject const *parent) { return axis_can_add (parent, GOG_AXIS_CIRCULAR); }
static void circular_axis_post_add    (GogObject *parent, GogObject *child)  { axis_post_add   (child, GOG_AXIS_CIRCULAR); }
static gboolean radial_axis_can_add (GogObject const *parent) { return axis_can_add (parent, GOG_AXIS_RADIAL); }
static void radial_axis_post_add    (GogObject *parent, GogObject *child)  { axis_post_add   (child, GOG_AXIS_RADIAL); }
static gboolean pseudo_3d_axis_can_add (GogObject const *parent) { return axis_can_add (parent, GOG_AXIS_PSEUDO_3D); }
static void pseudo_3d_axis_post_add    (GogObject *parent, GogObject *child)  { axis_post_add   (child, GOG_AXIS_PSEUDO_3D); }
static gboolean bubble_axis_can_add (GogObject const *parent) { return axis_can_add (parent, GOG_AXIS_BUBBLE); }
static void bubble_axis_post_add    (GogObject *parent, GogObject *child)  { axis_post_add   (child, GOG_AXIS_BUBBLE); }
static gboolean color_axis_can_add (GogObject const *parent) { return axis_can_add (parent, GOG_AXIS_COLOR); }
static void color_axis_post_add    (GogObject *parent, GogObject *child)  { axis_post_add   (child, GOG_AXIS_COLOR); }
static gboolean role_3d_box_can_add	(GogObject const *parent) {return FALSE;}
static gboolean role_3d_box_can_remove	(GogObject const *parent) {return FALSE;}

static GogObjectRole const roles[] = {
	{ N_("Backplane"), "GogGrid",	1,
	  GOG_POSITION_SPECIAL, GOG_POSITION_SPECIAL, GOG_OBJECT_NAME_BY_ROLE,
	  role_grid_can_add, NULL, NULL, role_grid_post_add, role_grid_pre_remove, NULL, { -1 } },
	{ N_("XY-Backplane"), "GogGrid",	1,
	  GOG_POSITION_SPECIAL, GOG_POSITION_SPECIAL, GOG_OBJECT_NAME_BY_ROLE,
	  xy_grid_3d_can_add, NULL, NULL, xy_grid_3d_post_add, NULL, NULL,
	  { GOG_GRID_XY } },
	{ N_("YZ-Backplane"), "GogGrid",	1,
	  GOG_POSITION_SPECIAL, GOG_POSITION_SPECIAL, GOG_OBJECT_NAME_BY_ROLE,
	  yz_grid_3d_can_add, NULL, NULL, yz_grid_3d_post_add, NULL, NULL,
	  { GOG_GRID_YZ } },
	{ N_("ZX-Backplane"), "GogGrid",	1,
	  GOG_POSITION_SPECIAL, GOG_POSITION_SPECIAL, GOG_OBJECT_NAME_BY_ROLE,
	  zx_grid_3d_can_add, NULL, NULL, zx_grid_3d_post_add, NULL, NULL,
	  { GOG_GRID_ZX } },
	{ N_("X-Axis"), "GogAxis",	2,
	  GOG_POSITION_PADDING, GOG_POSITION_PADDING, GOG_OBJECT_NAME_BY_ROLE,
	  x_axis_can_add, axis_can_remove, NULL, x_axis_post_add, axis_pre_remove, NULL,
	  { GOG_AXIS_X } },
	{ N_("Y-Axis"), "GogAxis",	3,
	  GOG_POSITION_PADDING, GOG_POSITION_PADDING, GOG_OBJECT_NAME_BY_ROLE,
	  y_axis_can_add, axis_can_remove, NULL, y_axis_post_add, axis_pre_remove, NULL,
	  { GOG_AXIS_Y } },
	{ N_("Z-Axis"), "GogAxis",	4,
	  GOG_POSITION_PADDING, GOG_POSITION_PADDING, GOG_OBJECT_NAME_BY_ROLE,
	  z_axis_can_add, axis_can_remove, NULL, z_axis_post_add, axis_pre_remove, NULL,
	  { GOG_AXIS_Z } },
	{ N_("Circular-Axis"), "GogAxis", 2,
	  GOG_POSITION_PADDING, GOG_POSITION_PADDING, GOG_OBJECT_NAME_BY_ROLE,
	  circular_axis_can_add, axis_can_remove, NULL, circular_axis_post_add, axis_pre_remove, NULL,
	  { GOG_AXIS_CIRCULAR } },
	{ N_("Radial-Axis"), "GogAxis",	3,
	  GOG_POSITION_PADDING, GOG_POSITION_PADDING, GOG_OBJECT_NAME_BY_ROLE,
	  radial_axis_can_add, axis_can_remove, NULL, radial_axis_post_add, axis_pre_remove, NULL,
	  { GOG_AXIS_RADIAL } },
	{ N_("Pseudo-3D-Axis"), "GogAxis", 4,
	  GOG_POSITION_SPECIAL, GOG_POSITION_SPECIAL, GOG_OBJECT_NAME_BY_ROLE,
	  pseudo_3d_axis_can_add, axis_can_remove, NULL, pseudo_3d_axis_post_add, axis_pre_remove, NULL,
	  { GOG_AXIS_PSEUDO_3D } },
	{ N_("Bubble-Axis"), "GogAxis", 4,
	  GOG_POSITION_SPECIAL, GOG_POSITION_SPECIAL, GOG_OBJECT_NAME_BY_ROLE,
	  bubble_axis_can_add, axis_can_remove, NULL, bubble_axis_post_add, axis_pre_remove, NULL,
	  { GOG_AXIS_BUBBLE } },
	{ N_("Color-Axis"), "GogAxis", 4,
	  GOG_POSITION_SPECIAL, GOG_POSITION_SPECIAL, GOG_OBJECT_NAME_BY_ROLE,
	  color_axis_can_add, axis_can_remove, NULL, color_axis_post_add, axis_pre_remove, NULL,
	  { GOG_AXIS_COLOR } },
	{ N_("Plot"), "GogPlot",	6,	/* keep the axis before the plots */
	  GOG_POSITION_SPECIAL, GOG_POSITION_SPECIAL, GOG_OBJECT_NAME_BY_TYPE,
	  role_plot_can_add, NULL, NULL, role_plot_post_add, role_plot_pre_remove, NULL, { -1 } },
	{ N_("Title"), "GogLabel",	0,
	  GOG_POSITION_COMPASS|GOG_POSITION_ANY_MANUAL,
	  GOG_POSITION_N|GOG_POSITION_ALIGN_CENTER,
	  GOG_OBJECT_NAME_BY_ROLE,
	  NULL, NULL, NULL, NULL, NULL, NULL, { -1 } },
	{ N_("Legend"), "GogLegend",	11,
	  GOG_POSITION_COMPASS|GOG_POSITION_ANY_MANUAL,
	  GOG_POSITION_E|GOG_POSITION_ALIGN_CENTER,
	  GOG_OBJECT_NAME_BY_ROLE,
	  NULL, NULL, NULL, NULL, NULL, NULL, { -1 } },
#ifdef GOFFICE_WITH_LASEM
	{ N_("Equation"), "GogEquation",	11,
	  GOG_POSITION_COMPASS|GOG_POSITION_ANY_MANUAL,
	  GOG_POSITION_S|GOG_POSITION_ALIGN_CENTER,
	  GOG_OBJECT_NAME_BY_ROLE,
	  NULL, NULL, NULL, NULL, NULL, NULL, { -1 } },
#endif
	{ N_("3D-Box"), "Gog3DBox",	1,
	  GOG_POSITION_SPECIAL, GOG_POSITION_SPECIAL, GOG_OBJECT_NAME_BY_ROLE,
	  role_3d_box_can_add, role_3d_box_can_remove, NULL, NULL, NULL, NULL, { -1 } },
	{ N_("Color-Scale"), "GogColorScale", 7,
	  GOG_POSITION_COMPASS|GOG_POSITION_ANY_MANUAL|GOG_POSITION_ANY_MANUAL_SIZE,
	  GOG_POSITION_E|GOG_POSITION_ALIGN_CENTER,
	  GOG_OBJECT_NAME_BY_ROLE,
	  color_scale_can_add, NULL, NULL,
	  color_scale_post_add, color_scale_pre_remove, NULL, { -1 } }
};

static GogManualSizeMode
gog_chart_get_manual_size_mode (GogObject *gobj)
{
	return GOG_MANUAL_SIZE_FULL;
}

static void
gog_chart_class_init (GogObjectClass *gog_klass)
{
	GObjectClass *gobject_klass = (GObjectClass *)gog_klass;

	chart_parent_klass = g_type_class_peek_parent (gog_klass);
	gobject_klass->finalize = gog_chart_finalize;
	gobject_klass->set_property = gog_chart_set_property;
	gobject_klass->get_property = gog_chart_get_property;

#ifdef GOFFICE_WITH_GTK
	gog_klass->populate_editor = gog_chart_populate_editor;
#else
	gog_klass->populate_editor = NULL;
#endif

	gog_klass->get_manual_size_mode = gog_chart_get_manual_size_mode;

	g_object_class_install_property (gobject_klass, CHART_PROP_CARDINALITY_VALID,
		g_param_spec_boolean ("cardinality-valid",
				      _("Valid cardinality"),
				      _("Is the charts cardinality currently valid"),
				      FALSE,
				      GSF_PARAM_STATIC | G_PARAM_READABLE));
	g_object_class_install_property (gobject_klass, CHART_PROP_PLOT_AREA,
		g_param_spec_string ("plot-area",
				     _("Plot area"),
				     _("Position and size of plot area, in percentage of chart size"),
				     "0 0 1 1",
				     GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, CHART_PROP_PLOT_AREA_IS_MANUAL,
		g_param_spec_boolean ("is-plot-area-manual",
				      _("Manual plot area"),
				      _("Is plot area manual"),
				      FALSE,
				      GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, CHART_PROP_X_POS,
		g_param_spec_int ("xpos", _("xpos"),
			_("Horizontal chart position in graph grid"),
			0, G_MAXINT, 0, G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	/* we need to force saving of ypos since the default is not constant */
	g_object_class_install_property (gobject_klass, CHART_PROP_Y_POS,
		g_param_spec_int ("ypos", _("ypos"),
			_("Vertical chart position in graph grid"),
			0, G_MAXINT, 0, G_PARAM_READWRITE | GO_PARAM_PERSISTENT | GOG_PARAM_FORCE_SAVE));
	g_object_class_install_property (gobject_klass, CHART_PROP_COLUMNS,
		g_param_spec_int ("columns", _("columns"),
			_("Number of columns in graph grid"),
			1, G_MAXINT, 1, G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, CHART_PROP_ROWS,
		g_param_spec_int ("rows", _("rows"),
			_("Number of rows in graph grid"),
			1, G_MAXINT, 1, G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	gog_klass->view_type = gog_chart_view_get_type ();
	gog_klass->update    = gog_chart_update;
	gog_klass->children_reordered = gog_chart_children_reordered;
	gog_object_register_roles (gog_klass, roles, G_N_ELEMENTS (roles));
}

static void
gog_chart_init (GogChart *chart)
{
	chart->x_pos =
	chart->y_pos =
	chart->cols  =
	chart->rows  =
	chart->x_pos_actual =
	chart->y_pos_actual = 0;

	/* start as true so that we can queue an update when it changes */
	chart->cardinality_valid = TRUE;
	chart->axis_set = GOG_AXIS_SET_UNKNOWN;

	chart->is_plot_area_manual = FALSE;
	chart->plot_area.x =
	chart->plot_area.y = 0.0;
	chart->plot_area.w =
	chart->plot_area.h = 1.0;
}

GSF_CLASS (GogChart, gog_chart,
	   gog_chart_class_init, gog_chart_init,
	   GOG_TYPE_OUTLINED_OBJECT)

/**
 * gog_chart_get_position:
 * @chart: const #GogChart
 * @x:
 * @y:
 * @cols:
 * @rows:
 *
 * Returns: %TRUE if the chart has been positioned.
 **/
gboolean
gog_chart_get_position (GogChart const *chart,
			unsigned *x, unsigned *y, unsigned *cols, unsigned *rows)
{
	g_return_val_if_fail (GOG_CHART (chart), FALSE);

	if (chart->cols <= 0 || chart->rows <= 0)
		return FALSE;

	if (x != NULL)	  *x	= chart->x_pos;
	if (y != NULL)	  *y	= chart->y_pos;
	if (cols != NULL) *cols	= chart->cols;
	if (rows != NULL) *rows	= chart->rows;

	return TRUE;
}

/**
 * gog_chart_set_position:
 * @chart: #GogChart
 * @x:
 * @y:
 * @cols:
 * @rows:
 *
 **/
void
gog_chart_set_position (GogChart *chart,
			unsigned int x, unsigned int y, unsigned int cols, unsigned int rows)
{
	g_return_if_fail (GOG_IS_CHART (chart));

	if (chart->x_pos == x && chart->y_pos == y &&
	    chart->cols == cols && chart->rows == rows)
		return;

	chart->x_pos = x;
	chart->y_pos = y;
	chart->cols = cols;
	chart->rows = rows;

	gog_graph_validate_chart_layout (GOG_GRAPH (GOG_OBJECT (chart)->parent));
	gog_object_emit_changed (GOG_OBJECT (chart), TRUE);
}

/**
 * gog_chart_get_plot_area:
 * @chart: #GogChart
 * @plot_area: #GogViewAllocation
 *
 * Stores plot area in plot_area, in fraction of chart size.
 *
 * Returns: %TRUE if plot area position is manual.
 **/
gboolean
gog_chart_get_plot_area (GogChart *chart, GogViewAllocation *plot_area)
{
	if (plot_area != NULL)
		*plot_area = chart->plot_area;

	return chart->is_plot_area_manual;
}

/**
 * gog_chart_set_plot_area:
 * @chart: #GogChart
 * @plot_area: #GogViewAllocation
 *
 * If plot_area != NULL, sets plot area size and location, in fraction
 * of chart size, and sets GogChart::is_plot_area_manual flag to TRUE.
 * If plot_area == NULL, sets GogChart::is_plot_area_manual to FALSE.
 **/
void
gog_chart_set_plot_area (GogChart *chart, GogViewAllocation const *plot_area)
{
	if (plot_area == NULL) {
		chart->is_plot_area_manual = FALSE;
	} else {
		chart->plot_area = *plot_area;
		chart->is_plot_area_manual = TRUE;
	}
	gog_object_emit_changed (GOG_OBJECT (chart), TRUE);
}

/**
 * gog_chart_get_cardinality:
 * @chart: a #GogChart
 * @full: placeholder for full cardinality
 * @visible: placeholder for visible cardinality
 *
 * Update and cache cardinality values if required, and returns
 * full and visible cardinality. Full cardinality is the number of
 * chart elements that require a different style. Visible cardinality is
 * the number of chart elements shown in chart legend.
 *
 * @full and @visible may be %NULL.
 **/
void
gog_chart_get_cardinality (GogChart *chart, unsigned *full, unsigned *visible)
{
	GSList *ptr;
	unsigned tmp_full, tmp_visible;

	g_return_if_fail (GOG_IS_CHART (chart));

	if (!chart->cardinality_valid) {
		chart->cardinality_valid = TRUE;
		chart->full_cardinality = chart->visible_cardinality = 0;
		for (ptr = chart->plots ; ptr != NULL ; ptr = ptr->next) {
			gog_plot_update_cardinality (ptr->data, chart->full_cardinality);
			gog_plot_get_cardinality (ptr->data, &tmp_full, &tmp_visible);
			chart->full_cardinality += tmp_full;
			chart->visible_cardinality += tmp_visible;
		}
	}

	if (full != NULL)
		*full = chart->full_cardinality;
	if (visible != NULL)
		*visible = chart->visible_cardinality;
}

void
gog_chart_request_cardinality_update (GogChart *chart)
{
	g_return_if_fail (GOG_IS_CHART (chart));

	if (chart->cardinality_valid) {
		chart->cardinality_valid = FALSE;
		gog_object_request_update (GOG_OBJECT (chart));
	}
}

/**
 * gog_chart_foreach_elem:
 * @chart: #GogChart
 * @only_visible: whether to only apply to visible children
 * @handler: (scope call): callback
 * @data: user data
 *
 * Applies @handler to children
 **/
void
gog_chart_foreach_elem (GogChart *chart, gboolean only_visible,
			GogEnumFunc handler, gpointer data)
{
	GSList *ptr;

	g_return_if_fail (GOG_IS_CHART (chart));
	g_return_if_fail (chart->cardinality_valid);

	for (ptr = chart->plots ; ptr != NULL ; ptr = ptr->next)
		gog_plot_foreach_elem (ptr->data, only_visible, handler, data);
}


/**
 * gog_chart_get_plots:
 * @chart: #GogChart
 *
 * Returns: (element-type GogPlot) (transfer none): the list of the plots
 * in @chart.
 **/
GSList *
gog_chart_get_plots (GogChart const *chart)
{
	g_return_val_if_fail (GOG_IS_CHART (chart), NULL);
	return chart->plots;
}

GogAxisSet
gog_chart_get_axis_set (GogChart const *chart)
{
	g_return_val_if_fail (GOG_IS_CHART (chart), GOG_AXIS_SET_UNKNOWN);
	return chart->axis_set;
}

gboolean
gog_chart_axis_set_is_valid (GogChart const *chart, GogAxisSet type)
{
	GSList *ptr;

	g_return_val_if_fail (GOG_IS_CHART (chart), FALSE);

	for (ptr = chart->plots ; ptr != NULL ; ptr = ptr->next)
		if (!gog_plot_axis_set_is_valid (ptr->data, type))
			return FALSE;
	return TRUE;
}

static void
gog_chart_add_axis (GogChart *chart, GogAxisType type)
{
	unsigned i = G_N_ELEMENTS (roles);
	while (i-- > 0)
		if (roles[i].user.i == (int)type) {
			gog_object_add_by_role (GOG_OBJECT (chart), roles + i, NULL);
			return;
		}
	g_warning ("unknown axis type %d", type);
}

gboolean
gog_chart_axis_set_assign (GogChart *chart, GogAxisSet axis_set)
{
	GogAxis *axis;
	GSList  *ptr;
	GogAxisType type;

	g_return_val_if_fail (GOG_IS_CHART (chart), FALSE);

	if (chart->axis_set == axis_set)
		return TRUE;
	chart->axis_set = axis_set;

	if (axis_set == GOG_AXIS_SET_UNKNOWN)
		return TRUE;

	/* Add at least 1 instance of any required axis */
	for (type = 0 ; type < GOG_AXIS_TYPES ; type++)
		if ((axis_set & (1 << type))) {
			GSList *tmp = gog_chart_get_axes (chart, type);
			if (tmp == NULL)
				gog_chart_add_axis (chart, type);
			else
				g_slist_free (tmp);
		}

	/* link the plots */
	for (ptr = chart->plots ; ptr != NULL ; ptr = ptr->next)
		if (!gog_plot_axis_set_assign (ptr->data, axis_set))
			return FALSE;

	/* add any existing axis that do not fit this scheme to the set */
	for (ptr = GOG_OBJECT (chart)->children ; ptr != NULL ; ) {
		axis = ptr->data;
		ptr = ptr->next; /* list may change under us */
		if (GOG_IS_AXIS (axis)) {
			type = -1;
			g_object_get (G_OBJECT (axis), "type", &type, NULL);
			if (type < 0 || type >= GOG_AXIS_TYPES) {
				g_warning ("Invalid axis");
				continue;
			}

			if (0 == (axis_set & (1 << type))) {
				/* We used to delete those axes but if the first plot use a
				 restricted set, all other axes are lost, see #708292 */
				chart->axis_set |= 1 << type;
			}
		}
	}

	return TRUE;
}

/**
 * gog_chart_get_axes:
 * @chart: #GogChart
 * @target: #GogAxisType
 *
 * Returns: (element-type GogAxis) (transfer container): a list which the
 * caller must free of all axis of type @target
 * associated with @chart.
 **/
GSList *
gog_chart_get_axes (GogChart const *chart, GogAxisType target)
{
	GSList *ptr, *res = NULL;
	GogAxis *axis;
	int type;

	g_return_val_if_fail (GOG_IS_CHART (chart), NULL);

	for (ptr = GOG_OBJECT (chart)->children ; ptr != NULL ; ptr = ptr->next) {
		axis = ptr->data;
		if (GOG_IS_AXIS (axis)) {
			type = -1;
			g_object_get (G_OBJECT (axis), "type", &type, NULL);
			if (type < 0 || type >= GOG_AXIS_TYPES) {
				g_warning ("Invalid axis");
				continue;
			}
			if (type == target)
				res = g_slist_prepend (res, axis);
		}
	}

	return res;
}

/**
 * gog_chart_get_grid:
 * @chart: #GogChart
 *
 * Returns: (transfer none): the grid associated with @chart if one exists
 * otherwise NULL.
 **/
GogGrid  *
gog_chart_get_grid (GogChart const *chart)
{
	g_return_val_if_fail (GOG_IS_CHART (chart), NULL);
	return GOG_GRID (chart->grid);
}

gboolean
gog_chart_is_3d (GogChart const *chart)
{
	return chart->axis_set == GOG_AXIS_SET_XYZ;
}

/*********************************************************************/

static void
gog_chart_view_3d_process (GogView *view, GogViewAllocation *bbox)
{
	/* A XYZ axis set in supposed. If new sets (cylindrical, spherical or
	other are added, we'll need to change this code */
	GogViewAllocation tmp = *bbox;
	GogAxis *axisX, *axisY, *axisZ, *ref = NULL;
	GSList *axes;
	double xmin, xmax, ymin, ymax, zmin, zmax;
	double o[3], x[3], y[3], z[3], tg, d;
	Gog3DBox *box;
	Gog3DBoxView *box_view;
	GogChart *chart = GOG_CHART (gog_view_get_model (view));
	GogObject *obj = gog_object_get_child_by_name (GOG_OBJECT (chart), "3D-Box");
	GSList *ptr;
	GogView *child;
	GogViewPadding padding;
	GogAxisMetrics xm, ym, zm;

	if (!obj) {
		obj = g_object_new (GOG_3D_BOX_TYPE, NULL);
		gog_object_add_by_name (GOG_OBJECT (chart), "3D-Box", obj);
	}
	box = GOG_3D_BOX (obj);
	box_view = GOG_3D_BOX_VIEW (gog_view_find_child_view (view, obj));

	/* Only use the first of the axes. */
	axes = gog_chart_get_axes (chart, GOG_AXIS_X);
	axisX = GOG_AXIS (axes->data);
	xm = gog_axis_get_metrics (axisX);
	if (xm != GOG_AXIS_METRICS_DEFAULT)
		ref = gog_axis_get_ref_axis (axisX);
	g_slist_free (axes);
	gog_axis_get_bounds (axisX, &xmin, &xmax);
	axes = gog_chart_get_axes (chart, GOG_AXIS_Y);
	axisY = GOG_AXIS (axes->data);
	ym = gog_axis_get_metrics (axisY);
	if (ym != GOG_AXIS_METRICS_DEFAULT && ref == NULL)
		ref = gog_axis_get_ref_axis (axisY);
	g_slist_free (axes);
	gog_axis_get_bounds (axisY, &ymin, &ymax);
	axes = gog_chart_get_axes (chart, GOG_AXIS_Z);
	axisZ = GOG_AXIS (axes->data);
	zm = gog_axis_get_metrics (axisZ);
	if (zm != GOG_AXIS_METRICS_DEFAULT && ref == NULL)
		ref = gog_axis_get_ref_axis (axisZ);
	g_slist_free (axes);
	gog_axis_get_bounds (axisZ, &zmin, &zmax);
	/* define the 3d box */
	if (ref == NULL) {
		box_view->dz = tmp.h;
		if (ymax - ymin > xmax - xmin) {
			box_view->dy = tmp.w;
			box_view->dx = (xmax - xmin) / (ymax - ymin) * tmp.w;
		} else {
			box_view->dx = tmp.w;
			box_view->dy = (ymax - ymin) / (xmax - xmin) * tmp.w;
		}
	} else {
		double ref_length, ref_tick_dist, xspan, yspan, zspan;
		gog_axis_get_bounds (ref, &ref_length, &xspan);
		ref_length -= xspan;
		ref_tick_dist = gog_axis_get_major_ticks_distance (ref);
		xspan = xmax - xmin;
		if (xm == GOG_AXIS_METRICS_RELATIVE_TICKS) {
			double ratio, tick_dist = gog_axis_get_major_ticks_distance (axisX);
			g_object_get (axisX, "metrics-ratio", &ratio, NULL);
			xspan = (xmax - xmin) / tick_dist * ref_tick_dist * ratio;
		}
		yspan = ymax - ymin;
		if (ym == GOG_AXIS_METRICS_RELATIVE_TICKS) {
			double ratio, tick_dist = gog_axis_get_major_ticks_distance (axisY);
			g_object_get (axisY, "metrics-ratio", &ratio, NULL);
			yspan = (ymax - ymin) / tick_dist * ref_tick_dist * ratio;
		}
		zspan = zmax - zmin;
		if (zm == GOG_AXIS_METRICS_RELATIVE_TICKS) {
			double ratio, tick_dist = gog_axis_get_major_ticks_distance (axisZ);
			g_object_get (axisZ, "metrics-ratio", &ratio, NULL);
			zspan = (zmax - zmin) / tick_dist * ref_tick_dist * ratio;
		}
		if (ref == axisZ) {
			gboolean xrel = FALSE;
			box_view->dz = tmp.h;
			switch (xm) {
			case GOG_AXIS_METRICS_RELATIVE:
			case GOG_AXIS_METRICS_RELATIVE_TICKS:
				box_view->dx = xspan / zspan * tmp.h;
				if (box_view->dx > tmp.w) {
					box_view->dz *= tmp.w / box_view->dx;
					box_view->dx = tmp.w;
				}
					xrel = TRUE;
				break;
			default:
				box_view->dx = tmp.w;
				break;
			}
			switch (ym) {
			case GOG_AXIS_METRICS_RELATIVE:
			case GOG_AXIS_METRICS_RELATIVE_TICKS:
				box_view->dy = yspan / zspan * box_view->dz;
				if (box_view->dy > tmp.w) {
					box_view->dz *= tmp.w / box_view->dy;
					if (xrel)
						box_view->dx *= tmp.w / box_view->dy;
					box_view->dy = tmp.w;
				}
				break;
			default:
				box_view->dy = tmp.w;
				break;
			}
		} else {
			if (yspan > xspan) {
				box_view->dy = tmp.w;
				box_view->dx = xspan / yspan * tmp.w;
			} else {
				box_view->dx = tmp.w;
				box_view->dy = yspan / xspan * tmp.w;
			}
			if (zm == GOG_AXIS_METRICS_DEFAULT)
				box_view->dz = tmp.h;
			else
				box_view->dz = (ref == axisX)?
								zspan / xspan * box_view->dx:
								zspan / yspan * box_view->dy;
		}
	}

	/* now compute the position of each vertex, ignoring the fov */
	go_matrix3x3_transform (&box->mat, -box_view->dx, -box_view->dy, -box_view->dz, o, o + 1, o + 2);
	go_matrix3x3_transform (&box->mat, box_view->dx, -box_view->dy, -box_view->dz, x, x + 1, x + 2);
	go_matrix3x3_transform (&box->mat, -box_view->dx, box_view->dy, -box_view->dz, y, y + 1, y + 2);
	go_matrix3x3_transform (&box->mat, -box_view->dx, -box_view->dy, box_view->dz, z, z + 1, z + 2);
	/* for each diagonal, we need to take the vertex closer to the view point */
	if (o[1] > 0) {
		o[0] = -o[0];
		o[1] = -o[1];
		o[2] = -o[2];
	}
	if (x[1] > 0) {
		x[0] = -x[0];
		x[1] = -x[1];
		x[2] = -x[2];
	}
	if (y[1] > 0) {
		y[0] = -y[0];
		y[1] = -y[1];
		y[2] = -y[2];
	}
	if (z[1] > 0) {
		z[0] = -z[0];
		z[1] = -z[1];
		z[2] = -z[2];
	}
	/* if the fov is positive, calculate the position of the viewpoint */
	if (box->fov > 0.) {
		tg = tan (box->fov / 2.);
		box_view->r = -sqrt (o[0] * o[0] + o[2] * o[2]) / tg + o[1];
		d = -sqrt (x[0] * x[0] + x[2] * x[2]) / tg + x[1];
		if (d < box_view->r)
			box_view->r = d;
		d = -sqrt (y[0] * y[0] + y[2] * y[2]) / tg + y[1];
		if (d < box_view->r)
			box_view->r = d;
		d = -sqrt (z[0] *z[0] + z[2] * z[2]) / tg + z[1];
		if (d < box_view->r)
			box_view->r = d;
		/* also calculate the reduction factor we need to make things fit in the bbox */
		xmax = fabs (o[0]) / (1. - o[1] / box_view->r);
		zmax = fabs (o[2]) / (1. - o[1] / box_view->r);
		d = fabs (x[0]) / (1. - x[1] / box_view->r);
		if (d > xmax)
			xmax = d;
		d = fabs (x[2]) / (1. - x[1] / box_view->r);
		if (d > zmax)
			zmax = d;
		d = fabs (y[0]) / (1. - y[1] / box_view->r);
		if (d > xmax)
			xmax = d;
		d = fabs (y[2]) / (1. - y[1] / box_view->r);
		if (d > zmax)
			zmax = d;
		d = fabs (z[0]) / (1. - z[1] / box_view->r);
		if (d > xmax)
			xmax = d;
		d = fabs (z[2]) / (1. - z[1] / box_view->r);
		if (d > zmax)
			zmax = d;
	} else {
	    /* calculate the reduction factor we need to make things fit in the bbox */
		xmax = fabs (o[0]);
		zmax = fabs (o[2]);
		d = fabs (x[0]);
		if (d > xmax)
			xmax = d;
		d = fabs (x[2]);
		if (d > zmax)
			zmax = d;
		d = fabs (y[0]);
		if (d > xmax)
			xmax = d;
		d = fabs (y[2]);
		if (d > zmax)
			zmax = d;
		d = fabs (z[0]);
		if (d > xmax)
			xmax = d;
		d = fabs (z[2]);
		if (d > zmax)
			zmax = d;
	}
	/* use d and tg as x and z ratios, respectively */
	d = xmax / tmp.w;
	tg = zmax / tmp.h;
	box_view->ratio = (d > tg)? d: tg;

	gog_view_padding_request (view, bbox, &padding);

	if (!chart->is_plot_area_manual) {
		bbox->x += padding.wl;
		bbox->w -= padding.wl + padding.wr;
		bbox->y += padding.ht;
		bbox->h -= padding.ht + padding.hb;
	} else {
		tmp.x -= padding.wl;
		tmp.w += padding.wl + padding.wr;
		tmp.y -= padding.ht;
		tmp.h += padding.ht + padding.hb;
	}

	/* Recalculating ratio */
	d = xmax / bbox->w;
	tg = zmax / bbox->h;
	box_view->ratio = (d > tg)? d: tg;

	for (ptr = view->children; ptr != NULL ; ptr = ptr->next) {
		child = ptr->data;
		if (GOG_POSITION_IS_PADDING (child->model->position)) {
			gog_view_size_allocate (child, &tmp);
		}
	}

	/* by default, overlay all GOG_POSITION_SPECIAL children in residual */
	for (ptr = view->children; ptr != NULL ; ptr = ptr->next) {
		child = ptr->data;
		if (GOG_POSITION_IS_SPECIAL (child->model->position))
			gog_view_size_allocate (child, bbox);
	}
}

typedef struct {
	GogOutlinedView base;

	GogViewAllocation	plot_area;
} GogChartView;
typedef GogOutlinedViewClass	GogChartViewClass;

#define GOG_TYPE_CHART_VIEW	(gog_chart_view_get_type ())
#define GOG_CHART_VIEW(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_CHART_VIEW, GogChartView))
#define GOG_IS_CHART_VIEW(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_CHART_VIEW))

static GogViewClass *cview_parent_klass;

static void
gog_chart_view_size_allocate (GogView *view, GogViewAllocation const *bbox)
{
	GSList *ptr;
	GogView *child;
	GogChartView *chart_view = GOG_CHART_VIEW (view);
	GogViewAllocation tmp, *plot_area = &chart_view->plot_area;
	GogViewPadding padding;
	GogChart *chart = GOG_CHART (gog_view_get_model (view));

	(cview_parent_klass->size_allocate) (view, bbox);

	if (chart->is_plot_area_manual) {
		plot_area->x = bbox->x + chart->plot_area.x * bbox->w;
		plot_area->y = bbox->y + chart->plot_area.y * bbox->h;
		plot_area->w = chart->plot_area.w * bbox->w;
		plot_area->h = chart->plot_area.h * bbox->h;
	} else
		*plot_area = view->residual;

	/* special treatment for 3d charts */
	if (gog_chart_is_3d (chart)) {
		gog_chart_view_3d_process (view, plot_area);
		return;
	}

	tmp = *plot_area;
	gog_view_padding_request (view, plot_area, &padding);

	if (!chart->is_plot_area_manual) {
		plot_area->x += padding.wl;
		plot_area->w -= padding.wl + padding.wr;
		plot_area->y += padding.ht;
		plot_area->h -= padding.ht + padding.hb;
	} else {
		tmp.x -= padding.wl;
		tmp.w += padding.wl + padding.wr;
		tmp.y -= padding.ht;
		tmp.h += padding.ht + padding.hb;
	}

	for (ptr = view->children; ptr != NULL ; ptr = ptr->next) {
		child = ptr->data;
		if (GOG_POSITION_IS_PADDING (child->model->position)) {
			gog_view_size_allocate (child, &tmp);
		}
	}

	/* by default, overlay all GOG_POSITION_SPECIAL children in residual */
	for (ptr = view->children; ptr != NULL ; ptr = ptr->next) {
		child = ptr->data;
		if (GOG_POSITION_IS_SPECIAL (child->model->position))
			gog_view_size_allocate (child, plot_area);
	}
}

static void
gog_chart_view_init (GogChartView *cview)
{
}

static void
grid_line_render (GSList *start_ptr, GogViewAllocation const *bbox)
{
	GSList *ptr, *child_ptr;
	GSList *minor_grid_lines = NULL;
	GSList *major_grid_lines = NULL;
	GogView *child_view, *axis_child_view;

	for (ptr = start_ptr; ptr != NULL; ptr = ptr->next) {
		child_view = ptr->data;
		if (GOG_IS_AXIS (child_view->model)) {
			for (child_ptr = child_view->children; child_ptr != NULL; child_ptr = child_ptr->next) {
				axis_child_view = child_ptr->data;
				if (GOG_IS_GRID_LINE (axis_child_view->model)) {
					if (gog_grid_line_is_minor (GOG_GRID_LINE (axis_child_view->model)))
						minor_grid_lines = g_slist_prepend (minor_grid_lines,
										    axis_child_view);
					else
						major_grid_lines = g_slist_prepend (major_grid_lines,
										    axis_child_view);
				}
				else if (GOG_IS_AXIS_LINE (axis_child_view->model)) {
					GogView *grid_child_view;
					GSList *lptr;
					for (lptr = axis_child_view->children; lptr != NULL; lptr = lptr->next) {
						grid_child_view = lptr->data;
					if (GOG_IS_GRID_LINE (grid_child_view->model)) {
						if (gog_grid_line_is_minor (GOG_GRID_LINE (grid_child_view->model)))
							minor_grid_lines = g_slist_prepend (minor_grid_lines,
												grid_child_view);
						else
							major_grid_lines = g_slist_prepend (major_grid_lines,
												grid_child_view);
				}

					}
				}
			}
		}
	}

	/* Render stripes, minor first then major */
	for (ptr = minor_grid_lines; ptr != NULL; ptr = ptr->next) {
		gog_grid_line_view_render_stripes (ptr->data);
	}
	for (ptr = major_grid_lines; ptr != NULL; ptr = ptr->next) {
		gog_grid_line_view_render_stripes (ptr->data);
	}

	/* Render lines, minor first then major */
	for (ptr = minor_grid_lines; ptr != NULL; ptr = ptr->next) {
		gog_grid_line_view_render_lines (ptr->data);
	}
	for (ptr = major_grid_lines; ptr != NULL; ptr = ptr->next) {
		gog_grid_line_view_render_lines (ptr->data);
	}

	g_slist_free (minor_grid_lines);
	g_slist_free (major_grid_lines);
}

static void
plot_render (GogView *view, GogViewAllocation const *bbox, GogPlotRenderingOrder order)
{
	GSList *ptr;
	GogView *child_view;

	/* Render some plots before axes */
	for (ptr = view->children ; ptr != NULL ; ptr = ptr->next) {
		child_view = ptr->data;
		if (GOG_IS_PLOT (child_view->model) &&
		    GOG_PLOT (child_view->model)->rendering_order == order)
			gog_view_render	(ptr->data, bbox);
	}
}

static void
gog_chart_view_render (GogView *view, GogViewAllocation const *bbox)
{
	GSList *ptr;
	GogView *child_view;
	gboolean grid_line_rendered = FALSE;
	GogChart *chart = GOG_CHART (gog_view_get_model (view));

	cview_parent_klass->render (view, bbox);

	if (gog_chart_is_3d (chart)) {
		for (ptr = view->children ; ptr != NULL ; ptr = ptr->next) {
			child_view = ptr->data;
			if (!GOG_IS_AXIS (child_view->model) && !GOG_IS_PLOT (child_view->model) && !GOG_IS_LABEL (child_view->model))
				gog_view_render	(ptr->data, bbox);
		}
		/* now render plot and axes */
		for (ptr = view->children ; ptr != NULL ; ptr = ptr->next) {
			child_view = ptr->data;
			if (!GOG_IS_AXIS (child_view->model))
				continue;
			gog_view_render (ptr->data, bbox);
			grid_line_render (ptr, bbox);
		}
		for (ptr = view->children ; ptr != NULL ; ptr = ptr->next) {
			child_view = ptr->data;
			if (!GOG_IS_PLOT (child_view->model))
				continue;
			gog_view_render	(ptr->data, bbox);
		}
	} else {
		/* KLUDGE: render grid lines before axis */
		for (ptr = view->children ; ptr != NULL ; ptr = ptr->next) {
			child_view = ptr->data;
			if (!grid_line_rendered && GOG_IS_AXIS (child_view->model)) {
				plot_render (view, bbox, GOG_PLOT_RENDERING_BEFORE_GRID);
				grid_line_render (ptr, bbox);
				plot_render (view, bbox, GOG_PLOT_RENDERING_BEFORE_AXIS);
				grid_line_rendered = TRUE;
			}
			if (GOG_IS_PLOT (child_view->model)) {
			    if (!GOG_PLOT (child_view->model)->rendering_order)
				gog_view_render	(ptr->data, bbox);
			} else if (!GOG_IS_LABEL (child_view->model))
				gog_view_render	(ptr->data, bbox);
		}
	}
	for (ptr = view->children ; ptr != NULL ; ptr = ptr->next) {
		child_view = ptr->data;
		if (!GOG_IS_LABEL (child_view->model))
			continue;
		gog_view_render	(ptr->data, bbox);
	}
}

static void
gog_chart_view_class_init (GogChartViewClass *gview_klass)
{
	GogViewClass *view_klass = (GogViewClass *) gview_klass;
	GogOutlinedViewClass *oview_klass = (GogOutlinedViewClass *) gview_klass;

	cview_parent_klass = g_type_class_peek_parent (gview_klass);

	view_klass->size_allocate   	= gog_chart_view_size_allocate;
	view_klass->render 		= gog_chart_view_render;
	view_klass->clip 		= FALSE;
	oview_klass->call_parent_render = FALSE;
}

static GSF_CLASS (GogChartView, gog_chart_view,
		  gog_chart_view_class_init, gog_chart_view_init,
		  GOG_TYPE_OUTLINED_VIEW)

GogViewAllocation const *
gog_chart_view_get_plot_area (GogView const *view)
{
	g_return_val_if_fail (GOG_IS_CHART_VIEW (view), NULL);

	return &(GOG_CHART_VIEW (view)->plot_area);
}
