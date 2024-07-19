
/*
 * gog-axis-line.c :
 *
 * Copyright (C) 2005-2007 Emmanuel Pacaud (emmanuel.pacaud@univ-poitiers.fr)
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
#include "gog-axis-line-impl.h"

#include <gsf/gsf-impl-utils.h>

#include <glib/gi18n-lib.h>
#include <string.h>

/**
 * GogAxisTickProperties:
 * @tick_in: whether to have tick inside the plot area.
 * @tick_out: whether to have tick outside the plot area.
 * @size_pts: tick size.
 **/

/**
 * GogAxisPosition:
 * @GOG_AXIS_AT_LOW: crosses the other axis at low values.
 * @GOG_AXIS_CROSS: crosses the other axis at a given value.
 * @GOG_AXIS_AT_HIGH: crosses the other axis at high values.
 * @GOG_AXIS_AUTO: crosses at an automatically determined position.
 **/

/**
 * GogAxisTickTypes:
 * @GOG_AXIS_TICK_NONE: no tick, should not occur.
 * @GOG_AXIS_TICK_MAJOR: major tick.
 * @GOG_AXIS_TICK_MINOR: minor tick.
 **/

static unsigned gog_axis_base_get_ticks (GogAxisBase *axis_base, GogAxisTick **ticks);

static GogViewClass *gab_view_parent_klass;
static GObjectClass *gab_parent_klass;

enum {
	AXIS_BASE_PROP_0,
	AXIS_BASE_PROP_POSITION,
	AXIS_BASE_PROP_POSITION_STR,
	AXIS_BASE_PROP_MAJOR_TICK_LABELED,
	AXIS_BASE_PROP_MAJOR_TICK_IN,
	AXIS_BASE_PROP_MAJOR_TICK_OUT,
	AXIS_BASE_PROP_MAJOR_TICK_SIZE_PTS,
	AXIS_BASE_PROP_MINOR_TICK_IN,
	AXIS_BASE_PROP_MINOR_TICK_OUT,
	AXIS_BASE_PROP_MINOR_TICK_SIZE_PTS,
	AXIS_BASE_PROP_CROSS_AXIS_ID,
	AXIS_BASE_PROP_CROSS_LOCATION,
	AXIS_BASE_PROP_PADDING_PTS
};

static void
gog_axis_base_set_property (GObject *obj, guint param_id,
			    GValue const *value, GParamSpec *pspec)
{
	gboolean resized = FALSE;
	char const *str;
	GogAxisBase *axis_base = GOG_AXIS_BASE (obj);
	int itmp;
	unsigned position;

	switch (param_id) {
		case AXIS_BASE_PROP_POSITION:
			position = g_value_get_uint (value);
			resized = (position != axis_base->position);
			gog_axis_base_set_position (axis_base, position);
			break;
		case AXIS_BASE_PROP_POSITION_STR:
			str = g_value_get_string (value);
			if (str == NULL)
				return;
			else if (!g_ascii_strcasecmp (str, "low"))
				position = GOG_AXIS_AT_LOW;
			else if (!g_ascii_strcasecmp (str, "cross"))
				position = GOG_AXIS_CROSS;
			else if (!g_ascii_strcasecmp (str, "high"))
				position = GOG_AXIS_AT_HIGH;
			else if (!g_ascii_strcasecmp (str, "auto"))
				/* this should not occur, but it is the default */
				position = GOG_AXIS_AUTO;
			else {
				g_warning ("[GogAxisBase::set_property] invalid axis position (%s)", str);
				return;
			}
			resized = (position != axis_base->position);
			gog_axis_base_set_position (axis_base, position);
			break;
		case AXIS_BASE_PROP_CROSS_AXIS_ID:
			axis_base->crossed_axis_id = g_value_get_uint (value);
			break;

		case AXIS_BASE_PROP_PADDING_PTS:
			itmp = g_value_get_int (value);
			if (axis_base->padding != itmp) {
				axis_base->padding = itmp;
				resized = TRUE;
			}
			break;

		case AXIS_BASE_PROP_MAJOR_TICK_LABELED:
			itmp = g_value_get_boolean (value);
			if (axis_base->major_tick_labeled != itmp) {
				axis_base->major_tick_labeled = itmp;
				resized = TRUE;
			}
			break;
		case AXIS_BASE_PROP_MAJOR_TICK_IN :
			axis_base->major.tick_in = g_value_get_boolean (value);
			break;
		case AXIS_BASE_PROP_MAJOR_TICK_OUT :
			itmp = g_value_get_boolean (value);
			if (axis_base->major.tick_out != itmp) {
				axis_base->major.tick_out = itmp;
				resized = axis_base->major.size_pts > 0;
			}
			break;
		case AXIS_BASE_PROP_MAJOR_TICK_SIZE_PTS:
			itmp = g_value_get_int (value);
			if (axis_base->major.size_pts != itmp) {
				axis_base->major.size_pts = itmp;
				resized = axis_base->major.tick_out;
			}
			break;

		case AXIS_BASE_PROP_MINOR_TICK_IN :
			axis_base->minor.tick_in = g_value_get_boolean (value);
			break;
		case AXIS_BASE_PROP_MINOR_TICK_OUT :
			itmp = g_value_get_boolean (value);
			if (axis_base->minor.tick_out != itmp) {
				axis_base->minor.tick_out = itmp;
				resized = axis_base->minor.size_pts > 0;
			}
			break;
		case AXIS_BASE_PROP_MINOR_TICK_SIZE_PTS:
			itmp = g_value_get_int (value);
			if (axis_base->minor.size_pts != itmp) {
				axis_base->minor.size_pts = itmp;
				resized = axis_base->minor.tick_out;
		}
			break;

		default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
			 return; /* NOTE : RETURN */
	}

	gog_object_emit_changed (GOG_OBJECT (obj), resized);
}

static void
gog_axis_base_get_property (GObject *obj, guint param_id,
			    GValue *value, GParamSpec *pspec)
{
	GogAxisBase *axis_base = GOG_AXIS_BASE (obj);

	switch (param_id) {
		case AXIS_BASE_PROP_POSITION:
			g_value_set_uint (value, axis_base->position);
			break;

		case AXIS_BASE_PROP_POSITION_STR:
			switch (axis_base->position) {
				case GOG_AXIS_AT_LOW:
					g_value_set_static_string (value, "low");
					break;
				case GOG_AXIS_CROSS:
					g_value_set_static_string (value, "cross");
					break;
				case GOG_AXIS_AT_HIGH:
					g_value_set_static_string (value, "high");
					break;
				default:
					g_warning ("[GogAxisBase::get_property] invalid axis position (%d)",
						   axis_base->position);
				break;
			}
			break;
		case AXIS_BASE_PROP_CROSS_AXIS_ID:
			g_value_set_uint (value, axis_base->crossed_axis_id);
			break;

		case AXIS_BASE_PROP_PADDING_PTS:
			g_value_set_int	(value, axis_base->padding);
			break;

		case AXIS_BASE_PROP_MAJOR_TICK_LABELED:
			g_value_set_boolean (value, axis_base->major_tick_labeled);
			break;
		case AXIS_BASE_PROP_MAJOR_TICK_IN:
			g_value_set_boolean (value, axis_base->major.tick_in);
			break;
		case AXIS_BASE_PROP_MAJOR_TICK_OUT:
			g_value_set_boolean (value, axis_base->major.tick_out);
			break;
		case AXIS_BASE_PROP_MAJOR_TICK_SIZE_PTS:
			g_value_set_int (value, axis_base->major.size_pts);
			break;

		case AXIS_BASE_PROP_MINOR_TICK_IN:
			g_value_set_boolean (value, axis_base->minor.tick_in);
			break;
		case AXIS_BASE_PROP_MINOR_TICK_OUT:
			g_value_set_boolean (value, axis_base->minor.tick_out);
			break;
		case AXIS_BASE_PROP_MINOR_TICK_SIZE_PTS:
			g_value_set_int (value, axis_base->minor.size_pts);
			break;

		default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
			 break;
	}
}

static void
gog_axis_base_parent_changed (GogObject *child, gboolean was_set)
{
	GogAxisBase *axis_base = GOG_AXIS_BASE (child);

	if (was_set) {
		if (GOG_IS_AXIS (child))
			axis_base->axis = GOG_AXIS (child);
		else
			axis_base->axis = GOG_AXIS (child->parent);
		axis_base->chart = GOG_CHART (GOG_OBJECT (axis_base->axis)->parent);

	} else {
		axis_base->axis = NULL;
		axis_base->chart = NULL;
	}
	(GOG_OBJECT_CLASS (gab_parent_klass)->parent_changed) (child, was_set);
}

static void
gog_axis_base_finalize (GObject *obj)
{
	gog_dataset_finalize (GOG_DATASET (obj));
	(gab_parent_klass->finalize) (obj);
}

GogAxisType
gog_axis_base_get_crossed_axis_type (GogAxisBase *axis_base)
{
	GogAxisType axis_type, crossed_type;
	GogAxisSet axis_set;

	axis_type = gog_axis_get_atype (axis_base->axis);
	axis_set = gog_chart_get_axis_set (axis_base->chart);

	crossed_type = GOG_AXIS_UNKNOWN;
	if (axis_set == GOG_AXIS_SET_UNKNOWN)
		return crossed_type;

	switch (axis_set & GOG_AXIS_SET_FUNDAMENTAL) {
		case GOG_AXIS_SET_XY:
			if (axis_type == GOG_AXIS_X)
				crossed_type = GOG_AXIS_Y;
			else
				crossed_type = GOG_AXIS_X;
			break;
		case GOG_AXIS_SET_RADAR:
			if (axis_type == GOG_AXIS_RADIAL)
				crossed_type = GOG_AXIS_CIRCULAR;
			else
				crossed_type = GOG_AXIS_RADIAL;
			break;
		case GOG_AXIS_SET_X:
		case GOG_AXIS_SET_XYZ:
			break;
		case GOG_AXIS_SET_ALL:
		case GOG_AXIS_SET_NONE:
		default:
			g_message ("[GogAxisBase::get_crossed_axis_type] unimplemented for this axis set (%i)",
				   axis_set);
			break;
	}
	return crossed_type;
}

/**
 * gog_axis_base_get_crossed_axis_for_plot:
 * @axis_base: #GogAxisBase
 * @plot: #GogPlot
 *
 * Returns: (transfer none): returns the crossing axis in a 2D plot if set.
 **/
GogAxis *
gog_axis_base_get_crossed_axis_for_plot (GogAxisBase *axis_base, GogPlot *plot)
{
	GogAxisType cross_axis_type;

	g_return_val_if_fail (GOG_IS_AXIS_BASE (axis_base), NULL);
	g_return_val_if_fail (GOG_IS_PLOT (plot), NULL);

	cross_axis_type = gog_axis_base_get_crossed_axis_type (axis_base);
	return gog_plot_get_axis (plot, cross_axis_type);
}


/**
 * gog_axis_base_get_crossed_axis:
 * @axis_base: #GogAxisBase
 *
 * Returns: (transfer none): returns the crossing axis in a 2D chart if set.
 **/
GogAxis *
gog_axis_base_get_crossed_axis (GogAxisBase *axis_base)
{
	GogAxis *crossed_axis = NULL;
	GSList *axes, *ptr;
	gboolean found = FALSE;
	GogAxisType cross_axis_type = gog_axis_base_get_crossed_axis_type (axis_base);

	if (cross_axis_type == GOG_AXIS_UNKNOWN)
		return NULL;
	axes = gog_chart_get_axes (axis_base->chart, cross_axis_type);
	g_return_val_if_fail (axes != NULL, NULL);

	for (ptr = axes; !found && ptr; ptr = ptr->next) {
		crossed_axis = GOG_AXIS (ptr->data);
		found = (gog_object_get_id (GOG_OBJECT (crossed_axis)) == axis_base->crossed_axis_id);
	}

	if (!found)
		crossed_axis = GOG_AXIS (axes->data);

	g_slist_free (axes);
	return crossed_axis;
}

void
gog_axis_base_set_position (GogAxisBase *axis_base, GogAxisPosition position)
{
	GogAxis *axis;
	GogChart *chart;
	GSList *lines, *axes = NULL, *lptr, *aptr;
	gboolean can_at_low = TRUE, can_at_high = TRUE;

	g_return_if_fail (GOG_AXIS_BASE (axis_base) != NULL);

	if (position == GOG_AXIS_AUTO) {
		if (GOG_IS_AXIS (axis_base))
			axis = GOG_AXIS (axis_base);
		else
			axis = GOG_AXIS (gog_object_get_parent (GOG_OBJECT (axis_base)));

		chart = GOG_CHART (gog_object_get_parent (GOG_OBJECT (axis)));
		if (chart != NULL)
			axes = gog_chart_get_axes (chart, gog_axis_get_atype (axis));
		else
			axes = g_slist_prepend (axes, axis);

		for (aptr = axes; aptr != NULL; aptr = aptr->next) {
			lines = gog_object_get_children (GOG_OBJECT (aptr->data), NULL);
			lines = g_slist_prepend (lines, aptr->data);
			for (lptr = lines; lptr != NULL; lptr = lptr->next) {
				if (lptr->data == axis_base || !GOG_IS_AXIS_BASE (lptr->data))
					continue;
				position = gog_axis_base_get_position (GOG_AXIS_BASE (lptr->data));
				if (position == GOG_AXIS_AT_HIGH )
					can_at_high = FALSE;
				else if (position == GOG_AXIS_AT_LOW)
					can_at_low = FALSE;
			}
			g_slist_free (lines);
		}
		g_slist_free (axes);

		if (can_at_low)
			position = GOG_AXIS_AT_LOW;
		else if (can_at_high)
			position = GOG_AXIS_AT_HIGH;
		else
			position = GOG_AXIS_CROSS;
	}

	axis_base->position = position;
}

GogAxisPosition
gog_axis_base_get_clamped_position (GogAxisBase *axis_base)
{
	GogAxisPosition axis_pos;

	g_return_val_if_fail (GOG_IS_AXIS_BASE (axis_base), GOG_AXIS_AT_LOW);

	axis_pos = axis_base->position;
	if (axis_pos == GOG_AXIS_CROSS) {
		GogAxis *cross_axis;
		double cross_location;
		double minimum, maximum;

		cross_axis = gog_axis_base_get_crossed_axis (axis_base);
		if (cross_axis == NULL)
			return GOG_AXIS_AUTO;
		cross_location = gog_axis_base_get_cross_location (axis_base);
		if (gog_axis_get_bounds (cross_axis, &minimum, &maximum)) {
			double start, end;
			gog_axis_get_effective_span (cross_axis, &start, &end);
			if (go_sub_epsilon (cross_location - minimum) <= 0.0)
				axis_pos = gog_axis_is_inverted (cross_axis) ? GOG_AXIS_AT_HIGH : GOG_AXIS_AT_LOW;
			else if (go_add_epsilon (cross_location - maximum) >= 0.0)
				axis_pos = gog_axis_is_inverted (cross_axis) ? GOG_AXIS_AT_LOW : GOG_AXIS_AT_HIGH;
			if (axis_pos == GOG_AXIS_AT_LOW && start > 0.)
				return GOG_AXIS_CROSS;
			if (axis_pos == GOG_AXIS_AT_HIGH && end < 1.)
				return GOG_AXIS_CROSS;
		}
	}

	return axis_pos;
}

#ifdef GOFFICE_WITH_GTK
typedef struct {
	GogAxisBase 	*axis_base;
	GtkBuilder 	*gui;
} AxisBasePrefs;

static void
axis_base_pref_free (AxisBasePrefs *state)
{
	g_object_unref (state->gui);
	g_free (state);
}

static void
cb_cross_location_changed (GtkWidget *editor, AxisBasePrefs *state)
{
	gtk_toggle_button_set_active
		(GTK_TOGGLE_BUTTON (gtk_builder_get_object (state->gui, "axis-cross")),
		 TRUE);
}

static void
cb_cross_axis_changed (GtkComboBox *combo, AxisBasePrefs *state)
{
	GtkTreeIter iter;
	GValue value;
	GtkTreeModel *model = gtk_combo_box_get_model (combo);

	memset (&value, 0, sizeof (GValue));
	gtk_combo_box_get_active_iter (combo, &iter);
	gtk_tree_model_get_value (model, &iter, 1, &value);
	state->axis_base->crossed_axis_id = g_value_get_uint (&value);

	gtk_toggle_button_set_active
		(GTK_TOGGLE_BUTTON (gtk_builder_get_object (state->gui, "axis-cross")),
		 TRUE);

	g_value_unset (&value);
}

static void
cb_position_toggled (GtkWidget *button, AxisBasePrefs *state)
{
	GogAxisBase *axis_base = state->axis_base;
	GogAxisPosition position;
	char const *widget_name = gtk_buildable_get_name (GTK_BUILDABLE (button));

	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
		return;

	if (g_ascii_strcasecmp ("axis-high", widget_name) == 0)
		position = GOG_AXIS_AT_HIGH;
	else if (g_ascii_strcasecmp ("axis-cross", widget_name) == 0)
		position = GOG_AXIS_CROSS;
	else
		position = GOG_AXIS_AT_LOW;

	if (position != axis_base->position)
		gtk_widget_set_sensitive (go_gtk_builder_get_widget (state->gui, "padding-spinbutton"),
					  position != GOG_AXIS_CROSS);

	gog_axis_base_set_position (axis_base, position);
	gog_object_emit_changed (GOG_OBJECT (axis_base), TRUE);
}

static void
cb_tick_toggle_changed (GtkToggleButton *toggle_button, GObject *axis_base)
{
	g_object_set (axis_base,
		gtk_buildable_get_name (GTK_BUILDABLE (toggle_button)),
		gtk_toggle_button_get_active (toggle_button),
		NULL);
}

static void
cb_padding_changed (GtkSpinButton *spin, GObject *axis_base)
{
	g_object_set (axis_base, "padding-pts",
		      gtk_spin_button_get_value_as_int (spin), NULL);
}

static void
gog_axis_base_populate_editor (GogObject *gobj,
			       GOEditor *editor,
			       GogDataAllocator *dalloc,
			       GOCmdContext *cc)
{
	static char const * const toggle_props[] = {
		"major-tick-labeled",
		"major-tick-out",
		"major-tick-in",
		"minor-tick-out",
		"minor-tick-in"
	};
	GogAxisBase *axis_base;
	GtkBuilder *gui;
	GtkWidget *w;
	GogAxisType crossed_axis_type, axis_type;
	static guint axis_base_pref_page = 0;
	unsigned i;
	gboolean hide_position_box = TRUE;

	axis_base = GOG_AXIS_BASE (gobj);
	g_return_if_fail (GOG_AXIS_BASE (axis_base) != NULL);
	if (!gog_object_is_visible (axis_base->axis))
		return;

	go_editor_set_store_page (editor, &axis_base_pref_page);

	axis_type = gog_axis_get_atype (axis_base->axis);
	if (axis_type == GOG_AXIS_PSEUDO_3D) {
		(GOG_OBJECT_CLASS(gab_parent_klass)->populate_editor) (gobj, editor, dalloc, cc);
		return;
	}

	gui = go_gtk_builder_load_internal ("res:go:graph/gog-axis-prefs.ui", GETTEXT_PACKAGE, cc);
	if (gui == NULL) {
		(GOG_OBJECT_CLASS(gab_parent_klass)->populate_editor) (gobj, editor, dalloc, cc);
		return;
	}

	crossed_axis_type = gog_axis_base_get_crossed_axis_type (axis_base);
	if (crossed_axis_type != GOG_AXIS_UNKNOWN &&
	    axis_type != GOG_AXIS_CIRCULAR) {
		GtkListStore *store;
		GogDataEditor *deditor;
		GtkWidget *combo, *container, *w;
		GSList *axes, *ptr;
		unsigned axis_count;
		GtkCellRenderer *cell;
		AxisBasePrefs *state;

		store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_UINT);
		combo = go_gtk_builder_get_widget (gui, "cross-axis-combo");
		gtk_combo_box_set_model (GTK_COMBO_BOX (combo), GTK_TREE_MODEL (store));
		g_object_unref (store);

		cell = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), cell,
						"text", 0,
						NULL);

		axes = gog_chart_get_axes (axis_base->chart, crossed_axis_type);
		axis_count = 0;
		for (ptr = axes; ptr != NULL; ptr = ptr->next) {
			GtkTreeIter iter;
			GogAxis *crossed_axis;
			unsigned crossed_axis_id;

			crossed_axis = GOG_AXIS (ptr->data);
			crossed_axis_id = gog_object_get_id (GOG_OBJECT (crossed_axis));
			gtk_list_store_prepend (store, &iter);
			gtk_list_store_set (store, &iter,
					    0, gog_object_get_name (GOG_OBJECT (crossed_axis)),
					    1, crossed_axis_id,
					    -1);
			if (axis_base->crossed_axis_id == crossed_axis_id || axis_count == 0)
				gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
			axis_count++;
		}
		if (axis_count < 2)
			gtk_widget_set_sensitive (GTK_WIDGET (combo), FALSE);
		g_slist_free (axes);

		deditor = gog_data_allocator_editor
			(dalloc, GOG_DATASET (axis_base),
			 GOG_AXIS_ELEM_CROSS_POINT, GOG_DATA_SCALAR);
		container = go_gtk_builder_get_widget (gui, "cross-at-grid");
		gtk_container_add (GTK_CONTAINER (container), GTK_WIDGET (deditor));
		gtk_widget_show_all (container);

		state = g_new (AxisBasePrefs, 1);
		state->axis_base = axis_base;
		state->gui = g_object_ref (gui);
		g_signal_connect (G_OBJECT (combo), "changed",
				  G_CALLBACK (cb_cross_axis_changed), state);
		g_signal_connect (G_OBJECT (deditor), "changed",
				  G_CALLBACK (cb_cross_location_changed), state);
		g_signal_connect_swapped (G_OBJECT (combo), "destroy", G_CALLBACK (axis_base_pref_free), state);

		w = go_gtk_builder_get_widget (gui, "axis-low");
		if (axis_base->position == GOG_AXIS_AT_LOW)
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), TRUE);
		g_signal_connect (G_OBJECT (w), "toggled",
				  G_CALLBACK (cb_position_toggled), state);
		w = go_gtk_builder_get_widget (gui, "axis-cross");
		if (axis_base->position == GOG_AXIS_CROSS)
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), TRUE);
		g_signal_connect (G_OBJECT (w), "toggled",
				  G_CALLBACK (cb_position_toggled), state);
		w = go_gtk_builder_get_widget (gui, "axis-high");
		if (axis_base->position == GOG_AXIS_AT_HIGH)
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), TRUE);
		g_signal_connect (G_OBJECT (w), "toggled",
				  G_CALLBACK (cb_position_toggled), state);

		hide_position_box = FALSE;
	} else {
		w = go_gtk_builder_get_widget (gui, "cross-at-grid");
		gtk_widget_hide (w);
	}

	if (axis_type == GOG_AXIS_X ||
	    axis_type == GOG_AXIS_Y ||
	    axis_type == GOG_AXIS_Z ||
	    axis_type == GOG_AXIS_RADIAL) {
		w = go_gtk_builder_get_widget (gui, "padding-spinbutton");
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), axis_base->padding);
		g_signal_connect (G_CALLBACK (w), "value-changed",
				  G_CALLBACK (cb_padding_changed), axis_base);
		gtk_widget_set_sensitive (w, axis_base->position != GOG_AXIS_CROSS);
		hide_position_box = FALSE;
	} else {
		w = go_gtk_builder_get_widget (gui, "padding-grid");
		gtk_widget_hide (w);
	}

	if (hide_position_box) {
		w = go_gtk_builder_get_widget (gui, "position-grid");
		gtk_widget_hide (w);
	}

	for (i = 0; i < G_N_ELEMENTS (toggle_props) ; i++) {
		gboolean cur_val;

		w = go_gtk_builder_get_widget (gui, toggle_props[i]);
		g_object_get (G_OBJECT (gobj), toggle_props[i], &cur_val, NULL);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), cur_val);
		g_signal_connect_object (G_OBJECT (w),
					 "toggled",
					 G_CALLBACK (cb_tick_toggle_changed), axis_base, 0);
	}

	if (gog_axis_is_discrete (axis_base->axis)) {
		/* Hide minor tick properties */
		GtkWidget *w = go_gtk_builder_get_widget (gui, "minor-tick-grid");
		gtk_widget_hide (w);
	}

	go_editor_add_page (editor,
			     go_gtk_builder_get_widget (gui, "axis-base-pref-grid"),
			     _("Layout"));
	g_object_unref (gui);

	(GOG_OBJECT_CLASS(gab_parent_klass)->populate_editor) (gobj, editor, dalloc, cc);
}
#endif

static void
gog_axis_base_init_style (GogStyledObject *gso, GOStyle *style)
{
	style->interesting_fields = GO_STYLE_LINE | GO_STYLE_FONT | GO_STYLE_TEXT_LAYOUT;
	gog_theme_fillin_style (gog_object_get_theme (GOG_OBJECT (gso)),
		style, GOG_OBJECT (gso), 0, GO_STYLE_LINE | GO_STYLE_FONT | GO_STYLE_TEXT_LAYOUT);
}

static GogGridLine *
gog_axis_base_get_grid_line (GogAxisBase *axis_base, gboolean major)
{
	GogGridLine *grid_line;
	GSList *children;

	children = gog_object_get_children (GOG_OBJECT (axis_base),
		gog_object_find_role_by_name (GOG_OBJECT (axis_base),
			major ? "MajorGrid" : "MinorGrid"));
	if (children != NULL) {
		grid_line = GOG_GRID_LINE (children->data);
		g_slist_free (children);
		return grid_line;
	}
	return NULL;
}

static gboolean
role_grid_line_major_can_add (GogObject const *parent)
{
	GogAxisBase *axis_base = GOG_AXIS_BASE (parent);
	GogAxis *axis = axis_base->axis;
	GogAxisType type = gog_axis_get_atype (axis);

	return ((type == GOG_AXIS_X || type == GOG_AXIS_Y || type == GOG_AXIS_Z
	         || type == GOG_AXIS_RADIAL ||
	         (type == GOG_AXIS_CIRCULAR && !gog_axis_is_discrete (axis))) &&
	        gog_axis_base_get_grid_line (axis_base, TRUE) == NULL);
}

static gboolean
role_grid_line_minor_can_add (GogObject const *parent)
{
	GogAxisBase *axis_base = GOG_AXIS_BASE (parent);
	GogAxis *axis = axis_base->axis;
	GogAxisType type = gog_axis_get_atype (axis);

	return (!gog_axis_is_discrete (axis) &&
		(type == GOG_AXIS_X || type == GOG_AXIS_Y || type == GOG_AXIS_Z ||
		 type == GOG_AXIS_RADIAL || type == GOG_AXIS_CIRCULAR) &&
		 gog_axis_base_get_grid_line (axis_base, FALSE) == NULL);
}

static void
role_grid_line_major_post_add (GogObject *parent, GogObject *child)
{
	g_object_set (G_OBJECT (child), "is-minor", (gboolean)FALSE, NULL);
}

static void
role_grid_line_minor_post_add (GogObject *parent, GogObject *child)
{
	g_object_set (G_OBJECT (child), "is-minor", (gboolean)TRUE, NULL);
}

static gboolean
role_label_can_add (GogObject const *parent)
{
	GogAxisType type = gog_axis_get_atype (GOG_AXIS_BASE (parent)->axis);

	return (type == GOG_AXIS_X ||
		type == GOG_AXIS_Y ||
		type == GOG_AXIS_Z);
}

static void
gog_axis_base_class_init (GObjectClass *gobject_klass)
{
	static GogObjectRole const roles[] = {
		{ N_("MajorGrid"), "GogGridLine", 0,
		  GOG_POSITION_SPECIAL, GOG_POSITION_SPECIAL, GOG_OBJECT_NAME_BY_ROLE,
		  role_grid_line_major_can_add, NULL, NULL, role_grid_line_major_post_add, NULL, NULL, { -1 } },
		{ N_("MinorGrid"), "GogGridLine", 1,
		  GOG_POSITION_SPECIAL, GOG_POSITION_SPECIAL, GOG_OBJECT_NAME_BY_ROLE,
		  role_grid_line_minor_can_add, NULL, NULL, role_grid_line_minor_post_add, NULL, NULL, { -1 } },
		{ N_("Label"), "GogLabel", 3,
		  GOG_POSITION_SPECIAL|GOG_POSITION_ANY_MANUAL, GOG_POSITION_SPECIAL, GOG_OBJECT_NAME_BY_ROLE,
		  role_label_can_add, NULL, NULL, NULL, NULL, NULL, { -1 } }
	};
	GogObjectClass *gog_klass = (GogObjectClass *) gobject_klass;
	GogStyledObjectClass *gso_klass = (GogStyledObjectClass *) gobject_klass;

	gab_parent_klass = g_type_class_peek_parent (gobject_klass);
	gobject_klass->set_property 	= gog_axis_base_set_property;
	gobject_klass->get_property 	= gog_axis_base_get_property;
	gobject_klass->finalize	    	= gog_axis_base_finalize;
	gog_klass->parent_changed 	= gog_axis_base_parent_changed;

	g_object_class_install_property (gobject_klass, AXIS_BASE_PROP_POSITION,
		g_param_spec_uint ("pos",
			_("Axis position"),
			_("Where to position an axis low, high, or crossing"),
			GOG_AXIS_AT_LOW, GOG_AXIS_AT_HIGH, GOG_AXIS_AT_LOW,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (gobject_klass, AXIS_BASE_PROP_POSITION_STR,
		g_param_spec_string ("pos-str",
			_("Axis position (as a string)"),
			_("Where to position an axis low, high, or crossing"),
			"auto", /*the default will never occur, we need that to avoid axis
		    loose their "low" position on serialization, see #722402. */
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, AXIS_BASE_PROP_MAJOR_TICK_LABELED,
		g_param_spec_boolean ("major-tick-labeled",
			_("Major labels"),
			_("Show labels for major ticks"),
			TRUE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, AXIS_BASE_PROP_MAJOR_TICK_IN,
		g_param_spec_boolean ("major-tick-in",
			_("Inside major ticks"),
			_("Major tick marks inside the chart area"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, AXIS_BASE_PROP_MAJOR_TICK_OUT,
		g_param_spec_boolean ("major-tick-out",
			_("Outside major ticks"),
			_("Major tick marks outside the chart area"),
			TRUE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, AXIS_BASE_PROP_MAJOR_TICK_SIZE_PTS,
		g_param_spec_int ("major-tick-size-pts",
			_("Major tick size"),
			_("Size of the major tick marks, in points"),
			0, 20, 4,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	g_object_class_install_property (gobject_klass, AXIS_BASE_PROP_MINOR_TICK_IN,
		g_param_spec_boolean ("minor-tick-in",
			_("Inside minor ticks"),
			_("Minor tick marks inside the chart area"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, AXIS_BASE_PROP_MINOR_TICK_OUT,
		g_param_spec_boolean ("minor-tick-out",
			_("Outside minor ticks"),
			_("Minor tick marks outside the axis"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, AXIS_BASE_PROP_MINOR_TICK_SIZE_PTS,
		g_param_spec_int ("minor-tick-size-pts",
			_("Minor tick size"),
			_("Size of the minor tick marks, in points"),
			0, 15, 2,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	g_object_class_install_property (gobject_klass, AXIS_BASE_PROP_CROSS_AXIS_ID,
		g_param_spec_uint ("cross-axis-id",
			_("Cross axis ID"),
			_("Which axis to cross"),
			0, G_MAXUINT, 0,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	g_object_class_install_property (gobject_klass, AXIS_BASE_PROP_PADDING_PTS,
		g_param_spec_int ("padding-pts",
			_("Axis padding"),
			_("Distance from axis line to plot area, in points"),
			-G_MAXINT, G_MAXINT, 0,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	gog_object_register_roles (gog_klass, roles, G_N_ELEMENTS (roles));

#ifdef GOFFICE_WITH_GTK
	gog_klass->populate_editor	= gog_axis_base_populate_editor;
#endif
	gog_klass->view_type		= gog_axis_base_view_get_type ();
	gso_klass->init_style 		= gog_axis_base_init_style;
}

static void
gog_axis_base_init (GogAxisBase *gab)
{
	gab->chart = NULL;
	gab->axis = NULL;

	gab->position = GOG_AXIS_AT_LOW;
	gab->crossed_axis_id = 0;

	gab->minor.tick_in = gab->minor.tick_out = gab->major.tick_in = FALSE;
	gab->major.tick_out = TRUE;
	gab->major_tick_labeled = TRUE;
	gab->major.size_pts = 4;
	gab->minor.size_pts = 2;
}

double
gog_axis_base_get_cross_location (GogAxisBase *axis_base)
{
	GOData *data;

	g_return_val_if_fail (GOG_AXIS_BASE (axis_base) != NULL, 0.);

	data = axis_base->cross_location.data;
	if (GO_IS_DATA (data))
		return go_data_get_scalar_value (data);

	return 0.;
}

GogAxisPosition
gog_axis_base_get_position (GogAxisBase *axis_base)
{
	g_return_val_if_fail (GOG_AXIS_BASE (axis_base) != NULL, GOG_AXIS_AT_LOW);

	return axis_base->position;
}

static int
gog_axis_base_get_padding (GogAxisBase *axis_base)
{
	return (axis_base->position == GOG_AXIS_CROSS ?
		0 : axis_base->padding);
}

GSF_CLASS_ABSTRACT (GogAxisBase, gog_axis_base,
		    gog_axis_base_class_init, gog_axis_base_init,
		    GOG_TYPE_STYLED_OBJECT);

/************************************************************************/
#ifdef GOFFICE_WITH_GTK

static gboolean gog_axis_base_view_point (GogView *view, double x, double y);

static gboolean
gog_tool_bound_is_valid_axis (GogView *view)
{
	GogAxisBase *axis_base = GOG_AXIS_BASE (view->model);
	GogAxisType type = gog_axis_get_atype (axis_base->axis);

	return (type == GOG_AXIS_X ||
		type == GOG_AXIS_Y ||
		type == GOG_AXIS_Z);

}

static gboolean
gog_tool_select_axis_point (GogView *view, double x, double y, GogObject **gobj)
{
	return (gog_tool_bound_is_valid_axis (view) &&
		gog_axis_base_view_point (view, x, y));
}

static void
gog_tool_select_axis_render (GogView *view)
{
	if (gog_tool_bound_is_valid_axis (view))
		gog_renderer_draw_selection_rectangle (view->renderer, &view->allocation);
}

static GogTool gog_axis_tool_select_axis = {
	N_("Select axis"),
	GDK_LEFT_PTR,
	gog_tool_select_axis_point,
	gog_tool_select_axis_render,
	NULL /* init */,
        NULL /* move */,
	NULL /* double-click */,
	NULL /* destroy */
};


typedef struct {
	GogAxis *axis;
	GogAxisMap *map;
	double length;
	double start, stop;
} MoveBoundData;

static gboolean
gog_tool_move_start_bound_point (GogView *view, double x, double y, GogObject **gobj)
{
	return gog_tool_bound_is_valid_axis (view) &&
		gog_renderer_in_grip (x, y,
				      GOG_AXIS_BASE_VIEW (view)->x_start,
				      GOG_AXIS_BASE_VIEW (view)->y_start);
}

static void
gog_tool_move_start_bound_render (GogView *view)
{
	if (gog_tool_bound_is_valid_axis (view))
		gog_renderer_draw_grip (view->renderer,
					GOG_AXIS_BASE_VIEW (view)->x_start,
					GOG_AXIS_BASE_VIEW (view)->y_start);
}

static gboolean
gog_tool_move_stop_bound_point (GogView *view, double x, double y, GogObject **gobj)
{
	return (gog_tool_bound_is_valid_axis (view) &&
		gog_renderer_in_grip (x, y,
				     GOG_AXIS_BASE_VIEW (view)->x_stop,
				     GOG_AXIS_BASE_VIEW (view)->y_stop));
}

static void
gog_tool_move_stop_bound_render (GogView *view)
{
	if (gog_tool_bound_is_valid_axis (view))
		gog_renderer_draw_grip (view->renderer,
					GOG_AXIS_BASE_VIEW (view)->x_stop,
					GOG_AXIS_BASE_VIEW (view)->y_stop);
}

static void
gog_tool_move_bound_init (GogToolAction *action)
{
	MoveBoundData *data = g_new0 (MoveBoundData, 1);
	GogAxisBaseView *view = GOG_AXIS_BASE_VIEW (action->view);
	GogAxisBase *axis_base = GOG_AXIS_BASE (action->view->model);

	action->data = data;
	data->map = gog_axis_map_new (axis_base->axis, 0.0, 1.0);
	data->axis = axis_base->axis;
	data->length = hypot (view->x_start - view->x_stop,
			      view->y_start - view->y_stop);
	gog_axis_map_get_real_extents (data->map, &data->start, &data->stop);
}

static void
gog_tool_move_start_bound_move (GogToolAction *action, double x, double y)
{
	GogAxisBaseView *view = GOG_AXIS_BASE_VIEW (action->view);
	MoveBoundData *data = action->data;
	double a = 1.0 - MIN (((x - view->x_start) * (view->x_stop - view->x_start) +
			       (y - view->y_start) * (view->y_stop - view->y_start)) /
			      (data->length * data->length), 0.9);

	gog_axis_set_extents (data->axis,
			      data->stop + ((data->start - data->stop) / a),
			      go_nan);
}

static void
gog_tool_move_stop_bound_move (GogToolAction *action, double x, double y)
{
	GogAxisBaseView *view = GOG_AXIS_BASE_VIEW (action->view);
	MoveBoundData *data = action->data;
	double a = 1.0 - MIN (((x - view->x_stop) * (view->x_start - view->x_stop) +
			       (y - view->y_stop) * (view->y_start - view->y_stop)) /
			      (data->length * data->length), 0.9);

	gog_axis_set_extents (data->axis,
			      go_nan,
			      data->start + ((data->stop - data->start) / a));
}

static void
gog_tool_move_bound_destroy (GogToolAction *action)
{
	MoveBoundData *data = action->data;

	gog_axis_map_free (data->map);
}

static GogTool gog_axis_tool_start_bound = {
	N_("Set start bound"),
	GDK_CROSS,
	gog_tool_move_start_bound_point,
	gog_tool_move_start_bound_render,
	gog_tool_move_bound_init,
	gog_tool_move_start_bound_move,
	NULL /* double-click */,
	gog_tool_move_bound_destroy
};

static GogTool gog_axis_tool_stop_bound = {
	N_("Set stop bound"),
	GDK_CROSS,
	gog_tool_move_stop_bound_point,
	gog_tool_move_stop_bound_render,
	gog_tool_move_bound_init,
	gog_tool_move_stop_bound_move,
	NULL /* double-click */,
	gog_tool_move_bound_destroy
};
#endif

/************************************************************************/

#define POINT_MIN_DISTANCE 	5 	/* distance minimum between point and axis for point = TRUE, in pixels */

typedef enum {
	GOG_AXIS_BASE_RENDER,
	GOG_AXIS_BASE_POINT,
	GOG_AXIS_BASE_PADDING_REQUEST,
	GOG_AXIS_BASE_LABEL_POSITION_REQUEST,
} GogAxisBaseAction;

static gboolean
axis_line_point (GogAxisBase *axis_base, GogRenderer *renderer,
		 double x, double y,
		 double xa, double ya, double wa, double ha,
		 GOGeometrySide side)
{
	double axis_length, axis_angle;
	double padding = gog_axis_base_get_padding (axis_base);
	double cos_alpha, sin_alpha;

	/* we need to take the axis span into account, see #746456 */
	/* using cos_alpha and sin-alpha to store the axis span for now */
	gog_axis_get_effective_span (axis_base->axis, &cos_alpha, &sin_alpha);
	xa += wa * cos_alpha;
	ya += ha * cos_alpha;
	wa *= (sin_alpha - cos_alpha);
	ha *= (sin_alpha - cos_alpha);
	go_geometry_cartesian_to_polar (wa, ha, &axis_length, &axis_angle);
	cos_alpha = side == GO_SIDE_LEFT ? - sin (axis_angle) : + sin (axis_angle);
	sin_alpha = side == GO_SIDE_LEFT ? + cos (axis_angle) : - cos (axis_angle);
	xa -= gog_renderer_pt2r_x (renderer, padding * cos_alpha);
	ya -= gog_renderer_pt2r_y (renderer, padding * sin_alpha);
	return go_geometry_point_to_segment (x, y, xa, ya, wa, ha) <= POINT_MIN_DISTANCE;
}

#define GO_EPSILON 1e-10
static GogViewAllocation
axis_line_get_bbox (GogAxisBase *axis_base, GogRenderer *renderer,
		    double x, double y, double w, double h,
		    GOGeometrySide side, double start_at, gboolean draw_labels)
{
	GogAxisMap *map = NULL;
	GogAxisTick *ticks;
	GOGeometryAABR total_bbox, bbox;
	GOStyle *style = axis_base->base.style;
	GOGeometryAABR txt_aabr;
	GOGeometryOBR txt_obr;
	GOGeometryOBR *obrs = NULL;
	GOGeometrySide label_anchor = GO_SIDE_AUTO;
	double line_width;
	double axis_length, axis_angle, label_padding;
	double cos_alpha, sin_alpha;
	double pos;
	double minor_tick_len, major_tick_len, tick_len;
	double padding = gog_axis_base_get_padding (axis_base);
	double label_size_max = 0;
	double min, max, epsilon;
	unsigned i, tick_nbr;
	gboolean is_line_visible;

	go_geometry_cartesian_to_polar (w, h, &axis_length, &axis_angle);
	cos_alpha = side == GO_SIDE_LEFT ? - sin (axis_angle) : + sin (axis_angle);
	sin_alpha = side == GO_SIDE_LEFT ? + cos (axis_angle) : - cos (axis_angle);

	is_line_visible = go_style_is_line_visible (style);
	line_width = gog_renderer_line_size (renderer, style->line.width) / 2;

	minor_tick_len = gog_renderer_pt2r (renderer, axis_base->minor.size_pts);
	major_tick_len = gog_renderer_pt2r (renderer, axis_base->major.size_pts);
	tick_len = axis_base->major.tick_out ? major_tick_len :
		(axis_base->minor.tick_out ? minor_tick_len : 0.);
	gog_renderer_get_text_OBR (renderer, "0", TRUE, &txt_obr, -1.);
	label_padding = txt_obr.h * .15;

	total_bbox.x = x; total_bbox.y = y;
	total_bbox.w = w; total_bbox.h = h;
	bbox.x = x -= gog_renderer_pt2r_x (renderer, padding * cos_alpha);
	bbox.y = y -= gog_renderer_pt2r_y (renderer, padding * sin_alpha);
	bbox.w = w;
	bbox.h = h;
	go_geometry_AABR_add (&total_bbox, &bbox);

	if (is_line_visible) {
		double out_len, in_len;

		out_len = line_width;
		if (axis_base->major.tick_out)
			out_len += major_tick_len;
		else if (axis_base->minor.tick_out)
			out_len += minor_tick_len;
		in_len  = line_width;
		if (axis_base->major.tick_in)
			in_len += major_tick_len;
		else if (axis_base->minor.tick_in)
			in_len += minor_tick_len;

		bbox.x = x - out_len * cos_alpha;
		bbox.y = y - out_len * sin_alpha;
		bbox.w = (out_len + in_len) * cos_alpha;
		bbox.h = (out_len + in_len) * sin_alpha;
		go_geometry_AABR_add (&total_bbox, &bbox);
		bbox.x += w;
		bbox.y += h;
		go_geometry_AABR_add (&total_bbox, &bbox);
	}

	tick_nbr = gog_axis_base_get_ticks (axis_base, &ticks);

	if (!draw_labels || tick_nbr < 1)
		return total_bbox;

	map = gog_axis_map_new (axis_base->axis, 0., axis_length);
	gog_axis_map_get_real_bounds (map, &min, &max);

	epsilon = (max - min) * GO_EPSILON;
	min -= epsilon;
	max += epsilon;
	obrs = g_new0 (GOGeometryOBR, tick_nbr);
	for (i = 0; i < tick_nbr; i++) {
		if (ticks[i].position < min || ticks[i].position > max)
			continue;
		if (ticks[i].str != NULL) {
			GOGeometryOBR *obr = obrs + i;
			gog_renderer_get_gostring_OBR (renderer, ticks[i].str, obr, -1.);
			if (obr->w > label_size_max
			    || obr->h > label_size_max) {
				label_size_max = MAX (obr->w, obr->h);
				label_anchor = go_geometry_calc_label_anchor (obr, axis_angle);
			}
		}
	}

	for (i = 0; i < tick_nbr; i++) {
		if (ticks[i].position < min || ticks[i].position > max)
			continue;
		if (ticks[i].str != NULL) {
			GOGeometryOBR *obr = obrs + i;
			pos = gog_axis_map_to_view (map, ticks[i].position);
			go_geometry_calc_label_position (obr, axis_angle, tick_len + label_padding,
							 side, label_anchor);
			obr->x += x + pos * cos (axis_angle);
			obr->y += y + pos * sin (axis_angle);
			go_geometry_OBR_to_AABR (obr, &txt_aabr);
			go_geometry_AABR_add (&total_bbox, &txt_aabr);
		}
	}
	g_free (obrs);

	gog_axis_map_free (map);

	return total_bbox;
}

static void
axis_line_render (GogAxisBase *axis_base,
		  GogAxisBaseView *axis_base_view,
		  GogRenderer *renderer,
		  double x, double y, double w, double h,
		  GOGeometrySide side,
		  double start_at,
		  gboolean draw_labels,
		  gboolean sharp,
		  double *ticks_pos)
{
	GogAxisMap *map = NULL;
	GogAxisTick *ticks;
	GOStyle *style = axis_base->base.style;
	GOGeometryOBR zero_obr;
	GOGeometryOBR *obrs = NULL;
	GOGeometrySide label_anchor = GO_SIDE_AUTO;
	GOPath *path = NULL;
	double line_width;
	double axis_length, axis_angle, label_padding;
	double major_tick_len, minor_tick_len, tick_len;
	double major_out_x = 0., major_out_y= 0., major_in_x = 0., major_in_y = 0.;
	double minor_out_x = 0., minor_out_y= 0., minor_in_x = 0., minor_in_y = 0.;
	double cos_alpha, sin_alpha;
	double pos, pos_x, pos_y;
	double padding = gog_axis_base_get_padding (axis_base);
	double label_size_max = 0;
	double s, e;
	double min, max, epsilon;
	unsigned i, tick_nbr, nobr = 0, *indexmap = NULL;
	gboolean draw_major, draw_minor;
	gboolean is_line_visible;

	go_geometry_cartesian_to_polar (w, h, &axis_length, &axis_angle);
	cos_alpha = side == GO_SIDE_LEFT ? - sin (axis_angle) : + sin (axis_angle);
	sin_alpha = side == GO_SIDE_LEFT ? + cos (axis_angle) : - cos (axis_angle);

	x -= gog_renderer_pt2r_x (renderer, padding * cos_alpha);
	y -= gog_renderer_pt2r_y (renderer, padding * sin_alpha);

	gog_axis_get_effective_span (axis_base->axis, &s, &e);
	axis_base_view->x_start = x + w * s;
	axis_base_view->y_start = y + h * s;
	axis_base_view->x_stop = x + w * e;
	axis_base_view->y_stop = y + h * e;

	is_line_visible = go_style_is_line_visible (style);
	line_width = gog_renderer_line_size (renderer, style->line.width) / 2;

	if (is_line_visible) {
		/* draw the line only in the effective range */
		path = go_path_new ();
		go_path_set_options (path, sharp ? GO_PATH_OPTIONS_SHARP : 0);
		go_path_move_to (path, axis_base_view->x_start , axis_base_view->y_start);
		go_path_line_to (path, axis_base_view->x_stop, axis_base_view->y_stop);
	}

	map = gog_axis_map_new (axis_base->axis, 0., axis_length);
	gog_axis_map_get_real_bounds (map, &min, &max);

	draw_major = axis_base->major.tick_in || axis_base->major.tick_out;
	draw_minor = axis_base->minor.tick_in || axis_base->minor.tick_out;

	minor_tick_len = gog_renderer_pt2r (renderer, axis_base->minor.size_pts) + line_width;
	minor_out_x = axis_base->minor.tick_out ? - minor_tick_len * cos_alpha : 0.;
	minor_out_y = axis_base->minor.tick_out ? - minor_tick_len * sin_alpha : 0.;
	minor_in_x = axis_base->minor.tick_in ? minor_tick_len * cos_alpha : 0.;
	minor_in_y = axis_base->minor.tick_in ? minor_tick_len * sin_alpha : 0.;

	major_tick_len = gog_renderer_pt2r (renderer, axis_base->major.size_pts) + line_width;
	major_out_x = axis_base->major.tick_out ? - major_tick_len * cos_alpha : 0.;
	major_out_y = axis_base->major.tick_out ? - major_tick_len * sin_alpha : 0.;
	major_in_x = axis_base->major.tick_in ? major_tick_len * cos_alpha : 0.;
	major_in_y = axis_base->major.tick_in ? major_tick_len * sin_alpha : 0.;

	tick_len = axis_base->major.tick_out ? major_tick_len :
		(axis_base->minor.tick_out ? minor_tick_len : 0.);
	gog_renderer_get_text_OBR (renderer, "0", TRUE, &zero_obr, -1.);
	label_padding = zero_obr.h * .15;

	/* add some epsilon toerance to avoid roundin errors */
	epsilon = (max - min) * GO_EPSILON;
	min -= epsilon;
	max += epsilon;
	tick_nbr = gog_axis_base_get_ticks (axis_base, &ticks);
	if (draw_labels) {
		obrs = g_new0 (GOGeometryOBR, tick_nbr);
		indexmap = g_new0 (unsigned int, tick_nbr);
		for (i = 0; i < tick_nbr; i++) {
			if (ticks[i].position < min || ticks[i].position > max)
				continue;
			if (ticks[i].str != NULL) {
				GOGeometryOBR *obr = obrs + i;
				gog_renderer_get_gostring_OBR (renderer, ticks[i].str, obr, -1.);
				if (obr->w > label_size_max
				    || obr->h > label_size_max) {
					label_size_max = MAX (obr->w, obr->h);
					label_anchor = go_geometry_calc_label_anchor (obr, axis_angle);
				}
			}
		}
	}

	for (i = 0; i < tick_nbr; i++) {
		if (gog_axis_map (map, ticks[i].position) < start_at ||
		    ticks[i].position < min || ticks[i].position > max)
			continue;

		if (ticks_pos) {
			pos_x = ticks_pos[2 * i];
			pos_y = ticks_pos[2 * i + 1];
		} else {
			pos = gog_axis_map_to_view (map, ticks[i].position);
			pos_x = x + pos * cos (axis_angle);
			pos_y = y + pos * sin (axis_angle);
		}

		if (is_line_visible) {
			switch (ticks[i].type) {
				case GOG_AXIS_TICK_MAJOR:
					if (draw_major) {
						go_path_move_to (path, major_out_x + pos_x,
								 major_out_y + pos_y);
						go_path_line_to (path, major_in_x + pos_x,
								 major_in_y + pos_y);
					}
					break;

				case GOG_AXIS_TICK_MINOR:
					if (draw_minor) {
						go_path_move_to (path, minor_out_x + pos_x,
								 minor_out_y + pos_y);
						go_path_line_to (path, minor_in_x + pos_x,
								 minor_in_y + pos_y);
					}
					break;

				default:
					break;
			}
		}

		if (ticks[i].str != NULL && draw_labels) {
			GOGeometryOBR *obr = obrs + i;
			go_geometry_calc_label_position (obr, axis_angle, tick_len + label_padding,
							 side, label_anchor);
			if (ticks_pos) {
				obr->x += ticks_pos[2 * i];
				obr->y += ticks_pos[2 * i + 1];
			} else {
				pos = gog_axis_map_to_view (map, ticks[i].position);
				obr->x += x + pos * cos (axis_angle);
				obr->y += y + pos * sin (axis_angle);
			}

			indexmap[nobr] = i;
			nobr++;
		}
	}

	if (is_line_visible) {
		gog_renderer_stroke_shape (renderer, path);
		go_path_free (path);
	}

	/*
	 * This is far from perfect, but at least things are regular.
	 * The axis really needs to be queried to figure this out,
	 * especially if dates are involved.
	 */
	if (nobr > 0) {
		int skip = 1;

		for (i = skip; i < nobr; i += skip) {
			unsigned j;
			gboolean overlap;

			j = indexmap[i];
			overlap = go_geometry_test_OBR_overlap (obrs + j,
								obrs + indexmap[i - skip]);
			if (overlap) {
				skip++;
				i = 0;
				continue;
			}
		}

		for (i = 0; i < nobr; i += skip) {
			unsigned j = indexmap[i];
			GogViewAllocation label_pos;
			label_pos.x = obrs[j].x;
			label_pos.y = obrs[j].y;
			gog_renderer_draw_gostring (renderer, ticks[j].str,
						    &label_pos, GO_ANCHOR_CENTER,
			                            GO_JUSTIFY_CENTER, -1.);
		}
	}
	g_free (obrs);
	g_free (indexmap);

	gog_axis_map_free (map);
}

static gboolean
axis_circle_point (double x, double y, double center_x, double center_y, double radius, int num_radii)
{

	if (num_radii > 0.0) {
		int i;
		double x0 = center_x;
		double y0 = center_y;
		double x1, y1;
		double angle_rad = 0;

		for (i = 1; i <= num_radii; i++) {
			x1 = x0;
			y1 = y0;
			angle_rad = 2.0 * M_PI * (double) i / (double) num_radii;
			x0 = center_x + radius * sin (angle_rad);
			y0 = center_y - radius * cos (angle_rad);
			if (go_geometry_point_to_segment (x, y, x0, y0, x1 - x0, y1 - y0) < POINT_MIN_DISTANCE)
				return TRUE;
		}
	}

	return (radius - sqrt ((x - center_x) * (x - center_x) + (y - center_y) * (y - center_y))) < POINT_MIN_DISTANCE;
}

static GogViewAllocation
axis_circle_get_bbox (GogAxisBase *axis_base, GogRenderer *renderer,
		      GogChartMap *c_map, gboolean draw_labels)
{
	GogAxisMap *map;
	GogAxisTick *ticks;
	GogViewAllocation total_bbox;
	GogChartMapPolarData *parms = gog_chart_map_get_polar_parms (c_map);
	GOGeometryOBR txt_obr;
	GOGeometryAABR txt_aabr;
	double angle, offset, position, label_padding;
	double major_tick_len, minor_tick_len, tick_len, x, y;
	unsigned i, tick_nbr;
	gboolean draw_ticks;

	total_bbox.x = parms->cx; total_bbox.y = parms->cy; total_bbox.w = 0.; total_bbox.h = 0.;

	minor_tick_len = gog_renderer_pt2r (renderer, axis_base->minor.size_pts);
	major_tick_len = gog_renderer_pt2r (renderer, axis_base->major.size_pts);
	tick_len = axis_base->major.tick_out ? major_tick_len :
		(axis_base->minor.tick_out ? minor_tick_len : 0.);
	gog_renderer_get_text_OBR (renderer, "0", TRUE, &txt_obr, -1.);
	label_padding = txt_obr.h * .15;

	draw_ticks = go_style_is_line_visible (axis_base->base.style) &&
		(axis_base->major.tick_out || axis_base->minor.tick_out);

	map = gog_chart_map_get_axis_map (c_map, 1);
	gog_axis_map_get_extents (map, &offset , &position);
	map = gog_chart_map_get_axis_map (c_map, 0);
	tick_nbr = gog_axis_base_get_ticks (axis_base, &ticks);
	for (i = 0; i < tick_nbr; i++) {
		angle = gog_axis_map_to_view (map, ticks[i].position);
		gog_chart_map_2D_to_view (c_map, ticks[i].position, position, &x, &y);

		if (ticks[i].str != NULL && draw_labels) {
			gog_renderer_get_gostring_OBR (renderer, ticks[i].str, &txt_obr, -1.);
			go_geometry_calc_label_position (&txt_obr, angle + M_PI / 2.0, tick_len + label_padding,
							 GO_SIDE_LEFT, GO_SIDE_AUTO);
			txt_obr.x += x;
			txt_obr.y += y;
			go_geometry_OBR_to_AABR (&txt_obr, &txt_aabr);
			go_geometry_AABR_add (&total_bbox, &txt_aabr);
		} else
			if (draw_ticks) {
				txt_aabr.x = x + cos (angle) * tick_len;
				txt_aabr.y = y + sin (angle) * tick_len;
				txt_aabr.w = txt_aabr.h = 0.;
				go_geometry_AABR_add (&total_bbox, &txt_aabr);
			}
	}

	return total_bbox;
}

static void
axis_circle_render (GogAxisBase *axis_base, GogRenderer *renderer,
		    GogChartMap *c_map, gboolean is_discrete, gboolean draw_labels)
{
	GogAxisMap *map;
	GogAxisTick *ticks;
	GogViewAllocation label_pos;
	GogChartMapPolarData *parms = gog_chart_map_get_polar_parms (c_map);
	GOGeometryOBR txt_obr, txt_obr_old = {0., 0., 0., 0., 0.};
	GOGeometryOBR txt_obr_first;
	GOPath *path;
	double angle, offset, position, label_padding;
	double start, stop;
	double major_tick_len, minor_tick_len, tick_len;
	double x0, y0, x1, y1;
	unsigned i, step_nbr, tick_nbr;
	gboolean draw_major, draw_minor;
	gboolean is_line_visible;
	gboolean first_label_done = FALSE;

	map = gog_chart_map_get_axis_map (c_map, 1);
	gog_axis_map_get_extents (map, &offset , &position);
	map = gog_chart_map_get_axis_map (c_map, 0);

	path = go_path_new ();

	if (is_discrete) {
		gog_axis_map_get_extents (map, &start, &stop);
		step_nbr = go_rint (parms->th1 - parms->th0) + 1;
		for (i = 0; i <= step_nbr; i++) {
			gog_chart_map_2D_to_view (c_map, i + parms->th0, position, &x0, &y0);
			if (i == 0)
				go_path_move_to (path, x0, y0);
			else
				go_path_line_to (path, x0, y0);
		}
	} else
		go_path_arc (path, parms->cx, parms->cy, parms->rx, parms->ry,
			     -parms->th1, -parms->th0);

	is_line_visible = go_style_is_line_visible (axis_base->base.style);
	draw_major = axis_base->major.tick_in || axis_base->major.tick_out;
	draw_minor = axis_base->minor.tick_in || axis_base->minor.tick_out;

	minor_tick_len = gog_renderer_pt2r (renderer, axis_base->minor.size_pts);
	major_tick_len = gog_renderer_pt2r (renderer, axis_base->major.size_pts);
	tick_len = axis_base->major.tick_out ? major_tick_len :
		(axis_base->minor.tick_out ? minor_tick_len : 0.);
	gog_renderer_get_text_OBR (renderer, "0", TRUE, &txt_obr, -1.);
	label_padding = txt_obr.h * .15;

	tick_nbr = gog_axis_base_get_ticks (axis_base, &ticks);
	for (i = 0; i < tick_nbr; i++) {
		angle = gog_axis_map_to_view (map, ticks[i].position);
		if (is_line_visible) {
			switch (ticks[i].type) {
				case GOG_AXIS_TICK_MAJOR:
					if (draw_major) {
						gog_chart_map_2D_to_view (c_map, ticks[i].position, position,
									  &x0, &y0);
						if (axis_base->major.tick_in) {
							x1 = x0 - major_tick_len * cos (angle);
							y1 = y0 - major_tick_len * sin (angle);
						} else {
							x1 = x0;
							y1 = y0;
						}
						if (axis_base->major.tick_out) {
							x0 += major_tick_len * cos (angle);
							y0 += major_tick_len * sin (angle);
						}
						go_path_move_to (path, x0, y0);
						go_path_line_to (path, x1, y1);
					}
					break;

				case GOG_AXIS_TICK_MINOR:
					if (draw_minor) {
						gog_chart_map_2D_to_view (c_map, ticks[i].position, position,
									  &x0, &y0);
						if (axis_base->minor.tick_in) {
							x1 = x0 - minor_tick_len * cos (angle);
							y1 = y0 - minor_tick_len * sin (angle);
						} else {
							x1 = x0;
							y1 = y0;
						}
						if (axis_base->minor.tick_out) {
							x0 += minor_tick_len * cos (angle);
							y0 += minor_tick_len * sin (angle);
						}
						go_path_move_to (path, x0, y0);
						go_path_line_to (path, x1, y1);
					}
					break;

				default:
					break;
			}
		}

		if (ticks[i].str != NULL && draw_labels) {
			gog_chart_map_2D_to_view (c_map, ticks[i].position, position,
						  &label_pos.x, &label_pos.y);
			gog_renderer_get_gostring_OBR (renderer, ticks[i].str, &txt_obr, -1.);
			go_geometry_calc_label_position (&txt_obr, angle + M_PI / 2.0, tick_len + label_padding,
							 GO_SIDE_LEFT, GO_SIDE_AUTO);
			label_pos.x += txt_obr.x;
			label_pos.y += txt_obr.y;
			txt_obr.x = label_pos.x;
			txt_obr.y = label_pos.y;
			if (!first_label_done ||
			    (!go_geometry_test_OBR_overlap (&txt_obr, &txt_obr_old) &&
			     !go_geometry_test_OBR_overlap (&txt_obr, &txt_obr_first))) {
				gog_renderer_draw_gostring
					(renderer, ticks[i].str,
					 &label_pos, GO_ANCHOR_CENTER,
					 GO_JUSTIFY_CENTER, -1.);
				txt_obr_old = txt_obr;
			}
			if (!first_label_done) {
				txt_obr_first = txt_obr;
				first_label_done = TRUE;
			}
		}
	}

	gog_renderer_stroke_shape (renderer, path);
	go_path_free (path);
}

static gboolean
x_process (GogAxisBaseAction action, GogView *view, GogViewPadding *padding,
	   GogViewAllocation const *plot_area, double x, double y)
{
	GogAxisBase *axis_base = GOG_AXIS_BASE (view->model);
	GogAxisBaseView *axis_base_view = GOG_AXIS_BASE_VIEW (view);
	GogAxisType axis_type = gog_axis_get_atype (axis_base->axis);
	GogChartMap *c_map;
	GogAxisMap *a_map;
	GogViewAllocation axis_line_bbox;
	double ax, ay, bx, by;
	double start, stop;

	g_return_val_if_fail (axis_type == GOG_AXIS_X, FALSE);

	c_map = gog_chart_map_new (axis_base->chart, plot_area, axis_base->axis, NULL, NULL, TRUE);
	a_map = gog_chart_map_get_axis_map (c_map, 0);

	gog_axis_map_get_extents (a_map, &start, &stop);
	gog_chart_map_2D_to_view (c_map, start, 0, &ax, &ay);
	gog_chart_map_2D_to_view (c_map, stop, 0, &bx, &by);

	gog_chart_map_free (c_map);

	switch (action) {
		case GOG_AXIS_BASE_RENDER:
			axis_line_render (axis_base, axis_base_view,
					  view->renderer, ax, ay, bx - ax , by - ay,
					  GO_SIDE_RIGHT, -1.,
					  axis_base->major_tick_labeled, TRUE, NULL);
			break;

		case GOG_AXIS_BASE_PADDING_REQUEST:
			axis_line_bbox = axis_line_get_bbox (GOG_AXIS_BASE (view->model),
							     view->renderer, ax, ay, bx - ax, by - ay,
							     GO_SIDE_RIGHT, -1.,
							     axis_base->major_tick_labeled);
			padding->wl = MAX (0., plot_area->x - axis_line_bbox.x);
			padding->ht = MAX (0., plot_area->y - axis_line_bbox.y);
			padding->wr = MAX (0., axis_line_bbox.x + axis_line_bbox.w - plot_area->x - plot_area->w);
			padding->hb = MAX (0., axis_line_bbox.y + axis_line_bbox.h - plot_area->y - plot_area->h);
			break;

		case GOG_AXIS_BASE_POINT:
			return axis_line_point (GOG_AXIS_BASE (view->model), view->renderer,
						x, y, ax, ay, bx - ax, by - ay,
						GO_SIDE_RIGHT);
			break;
		default:
			break;
	}

	return FALSE;
}

static gboolean
xy_process (GogAxisBaseAction action, GogView *view, GogViewPadding *padding,
	    GogViewAllocation const *plot_area, double x, double y)
{
	GogAxisBase *axis_base = GOG_AXIS_BASE (view->model);
	GogAxisBaseView *axis_base_view = GOG_AXIS_BASE_VIEW (view);
	GogAxis *cross_axis;
	GogChartMap *c_map;
	GogAxisMap *a_map;
	GogViewAllocation axis_line_bbox;
	double ax, ay, bx, by;
	GogAxisType axis_type = gog_axis_get_atype (axis_base->axis);
	double position = 0.;
	double minimum, maximum, start, stop;
	GOGeometrySide side;

	g_return_val_if_fail (axis_type == GOG_AXIS_X ||
			      axis_type == GOG_AXIS_Y, FALSE);
	if (!gog_object_is_visible (axis_base->axis))
		return FALSE;

	cross_axis = gog_axis_base_get_crossed_axis (axis_base);
	if (axis_type == GOG_AXIS_X) {
		c_map = gog_chart_map_new (axis_base->chart, plot_area, axis_base->axis, cross_axis, NULL, TRUE);
		a_map = gog_chart_map_get_axis_map (c_map, 1);
	} else {
		c_map = gog_chart_map_new (axis_base->chart, plot_area, cross_axis, axis_base->axis, NULL, TRUE);
		a_map = gog_chart_map_get_axis_map (c_map, 0);
	}

	gog_axis_map_get_extents (a_map, &start, &stop);
	gog_axis_map_get_bounds (a_map, &minimum, &maximum);
	side = GO_SIDE_RIGHT;
	switch (gog_axis_base_get_clamped_position (axis_base)) {
		case GOG_AXIS_CROSS :
			position = gog_axis_base_get_cross_location (axis_base);
			break;
		case GOG_AXIS_AT_LOW :
			position = start;
			break;
		case GOG_AXIS_AT_HIGH :
			position = stop;
			side     = GO_SIDE_LEFT;
			break;
		default:
			g_warning ("[GogAxisLine::xy_process] invalid axis position (%d)",
				   axis_base->position);
			position = start;
			break;
	}

	if (axis_type == GOG_AXIS_X) {
		a_map = gog_chart_map_get_axis_map (c_map, 0);
		gog_axis_map_get_extents (a_map, &start, &stop);
		gog_chart_map_2D_to_view (c_map, start, position, &ax, &ay);
		gog_chart_map_2D_to_view (c_map, stop, position, &bx, &by);
	} else {
		a_map = gog_chart_map_get_axis_map (c_map, 1);
		gog_axis_map_get_extents (a_map, &start, &stop);
		gog_chart_map_2D_to_view (c_map, position, start, &ax, &ay);
		gog_chart_map_2D_to_view (c_map, position, stop, &bx, &by);
		side = (side == GO_SIDE_LEFT) ? GO_SIDE_RIGHT : GO_SIDE_LEFT;
	}
	gog_chart_map_free (c_map);

	switch (action) {
		case GOG_AXIS_BASE_RENDER:
			axis_line_render (axis_base, axis_base_view,
					  view->renderer,
					  ax, ay, bx - ax , by - ay, side, -1.,
					  axis_base->major_tick_labeled, TRUE, NULL);
			break;

		case GOG_AXIS_BASE_PADDING_REQUEST:
			axis_line_bbox = axis_line_get_bbox (GOG_AXIS_BASE (view->model),
							     view->renderer, ax, ay, bx - ax, by - ay, side, -1.,
							     axis_base->major_tick_labeled);
			padding->wl = MAX (0., plot_area->x - axis_line_bbox.x);
			padding->ht = MAX (0., plot_area->y - axis_line_bbox.y);
			padding->wr = MAX (0., axis_line_bbox.x + axis_line_bbox.w - plot_area->x - plot_area->w);
			padding->hb = MAX (0., axis_line_bbox.y + axis_line_bbox.h - plot_area->y - plot_area->h);
			break;

		case GOG_AXIS_BASE_POINT:
			return axis_line_point (GOG_AXIS_BASE (view->model), view->renderer,
						x, y, ax, ay, bx - ax, by - ay,
						side);
			break;
		default:
			break;
	}

	return FALSE;
}

static gboolean
radar_process (GogAxisBaseAction action, GogView *view, GogViewPadding *padding,
	       GogViewAllocation const *area, double x, double y)
{
	GogAxisBase *axis_base = GOG_AXIS_BASE (view->model);
	GogAxisBaseView *axis_base_view = GOG_AXIS_BASE_VIEW (view);
	GogAxis *cross_axis;
	GogChartMap *c_map;
	GogAxisMap *a_map;
	GogAxisType axis_type = gog_axis_get_atype (axis_base->axis);
	GogChartMapPolarData *parms;
	GogViewAllocation bbox;
	GOGeometrySide side;
	double start, stop, minimum, maximum;
	double bx, by, position;
	unsigned i;
	gboolean point = FALSE;

	g_return_val_if_fail (axis_type == GOG_AXIS_CIRCULAR ||
			      axis_type == GOG_AXIS_RADIAL, FALSE);
	if (!gog_object_is_visible (axis_base->axis))
		return FALSE;

	cross_axis = gog_axis_base_get_crossed_axis (axis_base);

	if (axis_type == GOG_AXIS_RADIAL) {
		c_map = gog_chart_map_new (axis_base->chart, area, cross_axis, axis_base->axis, NULL,
					   action == GOG_AXIS_BASE_PADDING_REQUEST);
		parms = gog_chart_map_get_polar_parms (c_map);
		a_map = gog_chart_map_get_axis_map (c_map, 0);
		gog_axis_map_get_bounds (a_map, &minimum, &maximum);
		gog_axis_map_get_extents (a_map, &start, &stop);
		if (axis_base->position == GOG_AXIS_CROSS) {
			position = gog_axis_base_get_cross_location (axis_base);
			if (position < minimum || position > maximum) {
				gog_chart_map_free (c_map);
				return FALSE;
			}
		} else
			position = axis_base->position == GOG_AXIS_AT_LOW ?  start : stop;
		side = axis_base->position == GOG_AXIS_AT_LOW ? GO_SIDE_RIGHT : GO_SIDE_LEFT;

		a_map = gog_chart_map_get_axis_map (c_map, 1);
		gog_axis_map_get_extents (a_map, &start, &stop);

		switch (action) {
			case GOG_AXIS_BASE_RENDER:
				if (gog_axis_is_discrete (cross_axis))
					for (i = parms->th0; i <= parms->th1; i++) {
					       	gog_chart_map_2D_to_view (c_map, i, stop, &bx, &by);
						axis_line_render (axis_base, axis_base_view,
								  view->renderer,
								  parms->cx, parms->cy,
								  bx - parms->cx, by - parms->cy,
								  side, 0.1, i == parms->th0 && axis_base->major_tick_labeled,
								  FALSE, NULL);
					} else {
					       	gog_chart_map_2D_to_view (c_map, position, stop, &bx, &by);
						axis_line_render (axis_base, axis_base_view,
								  view->renderer,
								  parms->cx, parms->cy,
								  bx - parms->cx, by - parms->cy,
								  side, 0., axis_base->major_tick_labeled,
								  FALSE, NULL);
					}
				break;
			case GOG_AXIS_BASE_PADDING_REQUEST:
				if (gog_axis_is_discrete (cross_axis)) break;

				gog_chart_map_2D_to_view (c_map, position, stop, &bx, &by);
				bbox = axis_line_get_bbox (axis_base,
					view->renderer, parms->cx, parms->cy,
					bx - parms->cx, by - parms->cy, side, -1.,
					axis_base->major_tick_labeled);
				padding->wl = MAX (0., area->x - bbox.x);
				padding->ht = MAX (0., area->y - bbox.y);
				padding->wr = MAX (0., bbox.x + bbox.w - area->x - area->w);
				padding->hb = MAX (0., bbox.y + bbox.h - area->y - area->h);
				break;
			case GOG_AXIS_BASE_POINT:
				if (gog_axis_is_discrete (cross_axis))
					for (i = parms->th0; i <= parms->th1; i++) {
						gog_chart_map_2D_to_view (c_map, i, stop, &bx, &by);
						point = axis_line_point (axis_base, view->renderer,
									 x, y, parms->cx, parms->cy,
									 bx - parms->cx, by - parms->cy,
									 side);
						if (point)
							break;
					}
				else {
					gog_chart_map_2D_to_view (c_map, position, stop, &bx, &by);
					point = axis_line_point (axis_base, view->renderer,
								 x, y, parms->cx, parms->cy,
								 bx - parms->cx, by - parms->cy,
								 side);
				}
				break;
			default:
				break;
		}
		gog_chart_map_free (c_map);
	} else {
		c_map = gog_chart_map_new (axis_base->chart, area, axis_base->axis, cross_axis, NULL,
					   action == GOG_AXIS_BASE_PADDING_REQUEST);
		parms = gog_chart_map_get_polar_parms (c_map);

		switch (action) {
			case GOG_AXIS_BASE_RENDER:
				axis_circle_render (GOG_AXIS_BASE (view->model), view->renderer,
						    c_map, gog_axis_is_discrete (axis_base->axis),
						    axis_base->major_tick_labeled);
				break;
			case GOG_AXIS_BASE_PADDING_REQUEST:
				bbox = axis_circle_get_bbox (axis_base, view->renderer, c_map,
							     axis_base->major_tick_labeled);
				padding->wl = MAX (0., area->x - bbox.x);
				padding->ht = MAX (0., area->y - bbox.y);
				padding->wr = MAX (0., bbox.x + bbox.w - area->x - area->w);
				padding->hb = MAX (0., bbox.y + bbox.h - area->y - area->h);
				break;
			case GOG_AXIS_BASE_POINT:
				point = axis_circle_point (x, y, parms->cx, parms->cy, parms->rx, parms->th1);
				break;
			default:
				break;
		}
		gog_chart_map_free (c_map);
	}
	return point;
}

static gboolean
xyz_process (GogAxisBaseAction action, GogView *view, GogViewPadding *padding,
	    GogViewAllocation const *plot_area, double x, double y)
{
	GogAxisBase *axis_base = GOG_AXIS_BASE (view->model);
	GogAxisBaseView *axis_base_view = GOG_AXIS_BASE_VIEW (view);
	GogAxisType axis_type = gog_axis_get_atype (axis_base->axis);
	GogAxis *axis1, *axis2;
	GogChartMap3D *c_map;
	GogAxisMap *a_map;
	GogViewAllocation axis_line_bbox;
	GSList *axes;
	GogAxisType perp_axis;
	GOGeometryOBR obr;
	GogAxisTick *ticks;
	double ax, ay, az, bx, by, bz, ox, oy, dist, tmp;
	double xposition, yposition, zposition;
	double start, stop;
	double *px[] = {&ax, &ax, &bx, &bx, &ax, &ax, &bx, &bx};
	double *py[] = {&ay, &by, &by, &ay, &ay, &by, &by, &ay};
	double *pz[] = {&az, &az, &az, &az, &bz, &bz, &bz, &bz};
	double rx[8], ry[8], rz[8];
	double major_tick_len, minor_tick_len, tick_len;
	double label_w, label_h;
	double *ticks_pos = NULL;

	/* Note: Anti-clockwise order in each face,
	 * important for calculating normals */
	const int faces[] = {
		3, 2, 1, 0, /* Bottom */
		4, 5, 6, 7, /* Top */
		0, 1, 5, 4, /* Left */
		2, 3, 7, 6, /* Right */
		1, 2, 6, 5, /* Front */
		0, 4, 7, 3  /* Back */
	};
	int i, tick_nbr, vertex = 0, base = 0;
	GOGeometrySide side = GO_SIDE_LEFT;

	g_return_val_if_fail (axis_type == GOG_AXIS_X ||
	                      axis_type == GOG_AXIS_Y ||
	                      axis_type == GOG_AXIS_Z, FALSE);

	if (!gog_object_is_visible (axis_base->axis))
		return FALSE;

	if (axis_type == GOG_AXIS_X) {
		axes  = gog_chart_get_axes (axis_base->chart, GOG_AXIS_Y);
		axis1 = GOG_AXIS (axes->data);
		g_slist_free (axes);
		axes  = gog_chart_get_axes (axis_base->chart, GOG_AXIS_Z);
		axis2 = GOG_AXIS (axes->data);
		g_slist_free (axes);
		c_map = gog_chart_map_3d_new (view, plot_area,
			axis_base->axis, axis1, axis2);
	} else if (axis_type == GOG_AXIS_Y) {
		axes  = gog_chart_get_axes (axis_base->chart, GOG_AXIS_Z);
		axis1 = GOG_AXIS (axes->data);
		g_slist_free (axes);
		axes  = gog_chart_get_axes (axis_base->chart, GOG_AXIS_X);
		axis2 = GOG_AXIS (axes->data);
		g_slist_free (axes);
		c_map = gog_chart_map_3d_new (view, plot_area,
			axis2, axis_base->axis, axis1);
	} else {
		axes  = gog_chart_get_axes (axis_base->chart, GOG_AXIS_X);
		axis1 = GOG_AXIS (axes->data);
		g_slist_free (axes);
		axes  = gog_chart_get_axes (axis_base->chart, GOG_AXIS_Y);
		axis2 = GOG_AXIS (axes->data);
		g_slist_free (axes);
		c_map = gog_chart_map_3d_new (view, plot_area,
			axis1, axis2, axis_base->axis);
	}

	a_map = gog_chart_map_3d_get_axis_map (c_map, 0);
	gog_axis_map_get_bounds (a_map, &ax, &bx);
	a_map = gog_chart_map_3d_get_axis_map (c_map, 1);
	gog_axis_map_get_bounds (a_map, &ay, &by);
	a_map = gog_chart_map_3d_get_axis_map (c_map, 2);
	gog_axis_map_get_bounds (a_map, &az, &bz);

	/* Projecting vertices */
	for (i = 0; i < 8; ++i)
		gog_chart_map_3d_to_view (c_map, *px[i], *py[i], *pz[i],
		                          &rx[i], &ry[i], &rz[i]);

	/* Determining base plane */
	dist  = ry[0] + ry[1] + ry[2] + ry[3];
	for (i = 4; i < 24; i += 4) {
		tmp = ry[faces[i]] + ry[faces[i + 1]]
		    + ry[faces[i + 2]] + ry[faces[i + 3]];
		if (tmp > dist) {
			dist = tmp;
			base = i;
		}
	}
	if (base == 0 || base == 4)
		perp_axis = GOG_AXIS_Z;
	else if (base == 8 || base == 12)
		perp_axis = GOG_AXIS_X;
	else
		perp_axis = GOG_AXIS_Y;

	/* Position of the centre of the base plane */
	ox = 0.25 * (rx[faces[base]] + rx[faces[base + 1]]
	   + rx[faces[base + 2]] + rx[faces[base + 3]]);
	oy = 0.25 * dist;

	/* Choosing the most distant vertex with respect to the centre
	 * of the base plane */
	dist = (rx[faces[base]] - ox);
	tmp  = (ry[faces[base]] - oy);
	dist = dist * dist + tmp * tmp;
	for (i = 1; i < 4; ++i) {
		double dx = rx[faces[base + i]] - ox;
		double dy = ry[faces[base + i]] - oy;
		tmp = dx * dx + dy * dy;

		/* Adding 1e-5 to ensure this inequality remains true
		 * after changing the ratio */
		if (tmp > dist + 1e-5) {
			dist = tmp;
			vertex = i;
		}
	}

	if (axis_type != perp_axis) {
		/* Here we're choosing one of the nearest neighbours
		 * of the vertex previously chosen */
		int pvtx = (vertex + 3) % 4; /* Previous vertex */
		int nvtx = (vertex + 1) % 4; /* Next vertex */
		int prev = faces[base + pvtx];
		int next = faces[base + nvtx];
		int curr = faces[base + vertex];
		tmp = (rx[next] - rx[curr]) * (ry[prev] - ry[curr])
		    - (ry[next] - ry[curr]) * (rx[prev] - rx[curr]);
		/* If normal is negative, we're choosing vertex which
		 * is closer to the screen */
		if ((tmp < 0 && rz[prev] < rz[next])
		    || (tmp > 0 && rz[prev] > rz[next]))
			vertex = pvtx;
		else
			vertex = nvtx;
	}

	if (axis_type == GOG_AXIS_Z) {
		xposition = *px[faces[base + vertex]];
		yposition = *py[faces[base + vertex]];

		a_map = gog_chart_map_3d_get_axis_map (c_map, 2);
		gog_axis_map_get_extents (a_map, &start, &stop);
		gog_chart_map_3d_to_view (c_map, xposition, yposition, start,
		                          &ax, &ay, NULL);
		gog_chart_map_3d_to_view (c_map, xposition, yposition, stop,
		                          &bx, &by, NULL);
		if (action == GOG_AXIS_BASE_RENDER) {
			tick_nbr = gog_axis_base_get_ticks (axis_base, &ticks);
			ticks_pos = g_new (double, 2 * tick_nbr);
			for (i = 0; i < tick_nbr; i++)
				gog_chart_map_3d_to_view (c_map, xposition,
							  yposition,
							  ticks[i].position,
							  ticks_pos + 2 * i,
							  ticks_pos + 2 * i + 1,
							  NULL);
		}
	} else if (axis_type == GOG_AXIS_X) {
		yposition = *py[faces[base + vertex]];
		zposition = *pz[faces[base + vertex]];

		a_map = gog_chart_map_3d_get_axis_map (c_map, 0);
		gog_axis_map_get_extents (a_map, &start, &stop);
		gog_chart_map_3d_to_view (c_map, start, yposition, zposition,
		                          &ax, &ay, NULL);
		gog_chart_map_3d_to_view (c_map, stop, yposition, zposition,
		                          &bx, &by, NULL);
		if (action == GOG_AXIS_BASE_RENDER) {
			tick_nbr = gog_axis_base_get_ticks (axis_base, &ticks);
			ticks_pos = g_new (double, 2 * tick_nbr);
			for (i = 0; i < tick_nbr; i++) {
				gog_chart_map_3d_to_view (c_map,
							  ticks[i].position,
							  yposition, zposition,
							  ticks_pos + 2 * i,
							  ticks_pos + 2 * i + 1,
							  NULL);
			}
		}
	} else {
		zposition = *pz[faces[base + vertex]];
		xposition = *px[faces[base + vertex]];

		a_map = gog_chart_map_3d_get_axis_map (c_map, 1);
		gog_axis_map_get_extents (a_map, &start, &stop);
		gog_chart_map_3d_to_view (c_map, xposition, start, zposition,
		                          &ax, &ay, NULL);
		gog_chart_map_3d_to_view (c_map, xposition, stop, zposition,
		                          &bx, &by, NULL);
		if (action == GOG_AXIS_BASE_RENDER) {
			tick_nbr = gog_axis_base_get_ticks (axis_base, &ticks);
			ticks_pos = g_new (double, 2 * tick_nbr);
			for (i = 0; i < tick_nbr; i++)
				gog_chart_map_3d_to_view (c_map, xposition,
							  ticks[i].position,
							  zposition,
							  ticks_pos + 2 * i,
							  ticks_pos + 2 * i + 1,
							  NULL);
		}
	}

	if (axis_type == perp_axis) {
		/* Calculating cross-product of two planar vectors
		 * to determine "chirality" of the projected axis */
		tmp = (ax - 0.5 * plot_area->w) * (by - ay)
		    - (ay - 0.5 * plot_area->h) * (bx - ax);
	} else {
		/* Same here, but relative to the centre of the base,
		 * except for a special case, when its 0 */
		if ((ax == bx && ax == ox) || (ay == by && ay == oy))
			tmp = (ax - 0.5 * plot_area->w) * (by - ay)
			    - (ay - 0.5 * plot_area->h) * (bx - ax);
		else
			tmp = (ax - ox) * (by - ay) - (ay - oy) * (bx - ax);
	}
	side = (tmp > 0)? GO_SIDE_LEFT : GO_SIDE_RIGHT;

	switch (action) {
		case GOG_AXIS_BASE_RENDER:
			axis_line_render (axis_base, axis_base_view,
					  view->renderer,
					  ax, ay, bx - ax , by - ay, side, -1.,
					  axis_base->major_tick_labeled, TRUE,
					  ticks_pos);
			break;
		case GOG_AXIS_BASE_PADDING_REQUEST:
			axis_line_bbox = axis_line_get_bbox (axis_base,
				view->renderer, ax, ay, bx - ax, by - ay,
				side, -1., axis_base->major_tick_labeled);
			padding->wl = MAX (0., plot_area->x - axis_line_bbox.x);
			padding->ht = MAX (0., plot_area->y - axis_line_bbox.y);
			padding->wr = MAX (0., axis_line_bbox.x + axis_line_bbox.w
			                   - plot_area->x - plot_area->w);
			padding->hb = MAX (0., axis_line_bbox.y + axis_line_bbox.h
			                   - plot_area->y - plot_area->h);
			break;
		case GOG_AXIS_BASE_POINT:
			break;
		case GOG_AXIS_BASE_LABEL_POSITION_REQUEST:
			/* Calculating unit vector perpendicular to the
			 * axis projection */
			if (side == GO_SIDE_RIGHT) {
				ox = -(by - ay);
				oy = bx - ax;
			} else {
				ox = by - ay;
				oy = -(bx - ax);
			}
			tmp = sqrt (ox * ox + oy * oy);
			ox *= 1. / tmp;
			oy *= 1. / tmp;

			/* Axis centre; we'll return it along with offset
			 * int the GogViewPadding structure */
			padding->wl = 0.5 * (ax + bx);
			padding->ht = 0.5 * (ay + by);

			/* Calculating axis label offset */
			dist = gog_axis_base_get_padding (axis_base);
			padding->wl += gog_renderer_pt2r_x (view->renderer,
				dist * ox);
			padding->ht += gog_renderer_pt2r_y (view->renderer,
				dist * oy);

			minor_tick_len = gog_renderer_pt2r (view->renderer,
				axis_base->minor.size_pts);
			major_tick_len = gog_renderer_pt2r (view->renderer,
				axis_base->major.size_pts);
			tick_len = axis_base->major.tick_out ? major_tick_len :
				(axis_base->minor.tick_out ? minor_tick_len : 0.);

			tick_nbr = gog_axis_base_get_ticks (axis_base, &ticks);

			gog_renderer_get_text_OBR (view->renderer,
				"0", TRUE, &obr, -1.);
			tick_len += fabs (obr.w * ox);
			if (axis_base->major_tick_labeled) {
				label_w = label_h = 0;
				for (i = 0; i < tick_nbr; i++) {
					if (ticks[i].str == NULL)
						continue;
					gog_renderer_get_text_OBR
						(view->renderer,
						 ticks[i].str->str,
						 FALSE, &obr, -1.);
					if (obr.w > label_w)
						label_w = obr.w;
					if (obr.h > label_h)
						label_h = obr.h;
				}
				tick_len += hypot (label_w, label_h);
			}
			padding->wr = tick_len * ox;
			padding->hb = tick_len * oy;
			break;
		default:
			break;
	}

	g_free (ticks_pos);
	gog_chart_map_3d_free (c_map);

	return FALSE;
}

#ifdef GOFFICE_WITH_GTK

static gboolean
gog_axis_base_view_point (GogView *view, double x, double y)
{
	GogAxisBase *axis_base = GOG_AXIS_BASE (view->model);
	GogAxisSet axis_set = gog_chart_get_axis_set (axis_base->chart);
	gboolean pointed = FALSE;
	GogViewAllocation const *plot_area;

	if (axis_set == GOG_AXIS_SET_UNKNOWN)
		return FALSE;

	/* FIXME: not nice */
	if (GOG_IS_AXIS (view->model))
		plot_area = gog_chart_view_get_plot_area (view->parent);
	else
		plot_area = gog_chart_view_get_plot_area (view->parent->parent);

	switch (axis_set) {
		case GOG_AXIS_SET_X:
			pointed = x_process (GOG_AXIS_BASE_POINT, view, NULL, plot_area, x, y);
			break;
		case GOG_AXIS_SET_XY:
			pointed = xy_process (GOG_AXIS_BASE_POINT, view, NULL, plot_area, x, y);
			break;
		case GOG_AXIS_SET_XY_COLOR:
			if (gog_axis_get_atype (axis_base->axis) != GOG_AXIS_COLOR)
				pointed = xy_process (GOG_AXIS_BASE_POINT, view, NULL, plot_area, x, y);
			break;
		case GOG_AXIS_SET_XY_pseudo_3d:
			if (gog_axis_get_atype (axis_base->axis) != GOG_AXIS_PSEUDO_3D)
				pointed = xy_process (GOG_AXIS_BASE_POINT, view, NULL, plot_area, x, y);
			break;
		case GOG_AXIS_SET_RADAR:
			pointed = radar_process (GOG_AXIS_BASE_POINT, view, NULL, plot_area, x, y);
			break;
		case GOG_AXIS_SET_XYZ:
			xyz_process (GOG_AXIS_BASE_POINT, view, NULL, plot_area, x, y);
			break;
		default:
			g_warning ("[AxisBaseView::point] not implemented for this axis set (%i)",
				   axis_set);
			break;
	}

	return pointed;
}

#endif

static void
gog_axis_base_view_padding_request (GogView *view, GogViewAllocation const *bbox, GogViewPadding *padding)
{
	GogAxisSet axis_set;
	GogAxisBase *axis_base = GOG_AXIS_BASE (view->model);
	GOStyle *style = axis_base->base.style;

	if (gog_axis_get_atype (axis_base->axis) >= GOG_AXIS_VIRTUAL)
		return;
	axis_set = gog_chart_get_axis_set (axis_base->chart);
	if (axis_set == GOG_AXIS_SET_UNKNOWN)
		return;

	gog_renderer_push_style (view->renderer, style);

	switch (axis_set & GOG_AXIS_SET_FUNDAMENTAL) {
		case GOG_AXIS_SET_X:
			x_process (GOG_AXIS_BASE_PADDING_REQUEST, view, padding, bbox, 0., 0.);
			break;
		case GOG_AXIS_SET_XY:
			xy_process (GOG_AXIS_BASE_PADDING_REQUEST, view, padding, bbox, 0., 0.);
			break;
		case GOG_AXIS_SET_RADAR:
			radar_process (GOG_AXIS_BASE_PADDING_REQUEST, view, padding, bbox, 0., 0.);
			break;
		case GOG_AXIS_SET_XYZ:
			xyz_process (GOG_AXIS_BASE_PADDING_REQUEST, view,
			             padding, bbox, 0., 0.);
			break;
		default:
			g_warning ("[AxisBaseView::padding_request] not implemented for this axis set (%i)",
				   axis_set);
			break;
	}

	gog_renderer_pop_style (view->renderer);
}

static void
gog_axis_base_view_render (GogView *view, GogViewAllocation const *bbox)
{
	GogAxisSet axis_set;
	GogAxisBase *axis_base = GOG_AXIS_BASE (view->model);
	GOStyle *style = axis_base->base.style;
	GogViewAllocation const *plot_area;

	if (gog_axis_get_atype (axis_base->axis) >= GOG_AXIS_VIRTUAL)
		return;
	axis_set = gog_chart_get_axis_set (axis_base->chart);
	if (axis_set == GOG_AXIS_SET_UNKNOWN)
		return;

	/* FIXME: not nice */
	if (GOG_IS_AXIS (view->model))
		plot_area = gog_chart_view_get_plot_area (view->parent);
	else
		plot_area = gog_chart_view_get_plot_area (view->parent->parent);

	gog_renderer_push_style (view->renderer, style);

	switch (axis_set & GOG_AXIS_SET_FUNDAMENTAL) {
		case GOG_AXIS_SET_X:
			x_process (GOG_AXIS_BASE_RENDER, view, NULL, plot_area, 0., 0.);
			break;
		case GOG_AXIS_SET_XY:
			xy_process (GOG_AXIS_BASE_RENDER, view, NULL, plot_area, 0., 0.);
			break;
		case GOG_AXIS_SET_RADAR:
			radar_process (GOG_AXIS_BASE_RENDER, view, NULL, plot_area, 0., 0.);
			break;
		case GOG_AXIS_SET_XYZ:
			xyz_process (GOG_AXIS_BASE_RENDER, view, NULL, plot_area, 0., 0.);
			break;
		default:
			g_warning ("[AxisBaseView::render] not implemented for this axis set (%i)",
				   axis_set);
			break;
	}

	gog_renderer_pop_style (view->renderer);
}

static void
gog_axis_base_build_toolkit (GogView *view)
{
#ifdef GOFFICE_WITH_GTK
	view->toolkit = g_slist_prepend (view->toolkit, &gog_axis_tool_select_axis);
	view->toolkit = g_slist_prepend (view->toolkit, &gog_axis_tool_start_bound);
	view->toolkit = g_slist_prepend (view->toolkit, &gog_axis_tool_stop_bound);
#endif
}

void
gog_axis_base_view_label_position_request (GogView *view,
                                           GogViewAllocation const *bbox,
					   GogViewAllocation *pos)
{
	GogAxisSet axis_set;
	GogAxisBase *axis_base = GOG_AXIS_BASE (view->model);
	GOStyle *style = axis_base->base.style;
	GogViewPadding padding = {0};

	if (gog_axis_get_atype (axis_base->axis) >= GOG_AXIS_VIRTUAL)
		return;
	axis_set = gog_chart_get_axis_set (axis_base->chart);
	if (axis_set == GOG_AXIS_SET_UNKNOWN)
		return;

	gog_renderer_push_style (view->renderer, style);

	switch (axis_set & GOG_AXIS_SET_FUNDAMENTAL) {
		case GOG_AXIS_SET_XYZ:
			xyz_process (GOG_AXIS_BASE_LABEL_POSITION_REQUEST, view,
			             &padding, bbox, 0., 0.);
			break;
		default:
			g_warning ("[AxisBaseView::label_position_request] not implemented for this axis set (%i)",
				   axis_set);
			break;
	}

	gog_renderer_pop_style (view->renderer);

	pos->x = padding.wl;
	pos->y = padding.ht;
	pos->w = padding.wr;
	pos->h = padding.hb;
}

static void
gog_axis_base_view_class_init (GogAxisBaseViewClass *gview_klass)
{
	GogViewClass *view_klass = (GogViewClass *) gview_klass;

	gab_view_parent_klass = g_type_class_peek_parent (gview_klass);

	view_klass->padding_request 	= gog_axis_base_view_padding_request;
	view_klass->render 		= gog_axis_base_view_render;
	view_klass->build_toolkit	= gog_axis_base_build_toolkit;
}

GSF_CLASS (GogAxisBaseView, gog_axis_base_view,
	   gog_axis_base_view_class_init, NULL,
	   GOG_TYPE_VIEW)

unsigned
gog_axis_base_get_ticks (GogAxisBase *axis_base, GogAxisTick **ticks)
{
	g_return_val_if_fail (GOG_IS_AXIS_BASE (axis_base), 0);
	g_return_val_if_fail (ticks != NULL, 0);

	if (GOG_IS_AXIS_LINE (axis_base)) {
		unsigned ret = gog_axis_line_get_ticks (GOG_AXIS_LINE (axis_base), ticks);
		if (ret > 0)
			return ret;
	}
	return gog_axis_get_ticks (axis_base->axis, ticks);
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

/*
 * Some fields are common to GogAxis and GogAxisLine, so they should go to
 * the common base class when we have a new development branch
 */
struct _GogAxisLine {
	GogAxisBase       base;
	GogDatasetElement custom_ticks[2];
	GogAxisTick      *ticks;
	unsigned          tick_nbr;
	GOFormat         *format, *assigned_format;
};


enum {
	AXIS_LINE_BASE_PROP_0,
	AXIS_LINE_PROP_ASSIGNED_FORMAT_STR_XL
};

typedef GogAxisBaseClass GogAxisLineClass;

static GObjectClass *gal_parent_klass;

static GOFormat *
gog_axis_line_get_effective_format (GogAxisLine const *line)
{
	if (line->assigned_format &&
	    !go_format_is_general (line->assigned_format))
		return line->assigned_format;
	return line->format;
}

static void
axis_line_format_value (GogAxisLine *line, double val, GOString **str)
{
	GOFormat *fmt = gog_axis_line_get_effective_format (line);
	GODateConventions const *date_conv = gog_axis_get_date_conv (line->base.axis);
	GOFormatNumberError err;
	PangoContext *context = pango_context_new ();
	PangoLayout *layout = pango_layout_new (context);
	g_object_unref (context);

	g_return_if_fail (layout != NULL);

	go_string_unref (*str);

	err = go_format_value_gstring
		(layout, NULL,
		 go_format_measure_strlen,
		 go_font_metrics_unit,
		 fmt,
		 val, 'F', NULL, NULL,
		 -1, date_conv, TRUE);
	if (err)
		*str = go_string_new ("#####");
	else {
		*str = go_string_new_rich
			(pango_layout_get_text (layout), -1,
			 pango_attr_list_ref
			 (pango_layout_get_attributes (layout)),
			 NULL);
		*str = go_string_trim (*str, TRUE);
	}

	g_object_unref (layout);
}

static void
gog_axis_line_discard_ticks (GogAxisLine *line)
{
	if (line->ticks != NULL) {
		unsigned i;
		for (i = 0; i < line->tick_nbr; i++)
			go_string_unref (line->ticks[i].str);

		g_free (line->ticks);
	}
	line->ticks = NULL;
	line->tick_nbr = 0;
}

static void
gog_axis_line_update_ticks (GogAxisLine *line)
{
	GODataVector *pos, *labels;

	gog_axis_line_discard_ticks (line);
	pos = GO_DATA_VECTOR (line->custom_ticks[0].data);
	labels = GO_DATA_VECTOR (line->custom_ticks[1].data);
	if (pos != NULL && go_data_has_value (GO_DATA (pos)) && go_data_is_varying_uniformly (GO_DATA (pos))) {
		unsigned len = go_data_vector_get_len (pos), cur, i, labels_nb;
		double val;
		char *lbl;
		line->ticks = g_new0 (GogAxisTick, len);
		labels_nb = (labels)? go_data_vector_get_len (labels): 0;
		if (labels_nb) {
			// Hack.
			// Gnumeric's gnm_go_data_vector_load_values changes
			// the length when the value is first accessed.
			// That seems like a breach on an (unwritten)
			// contract, but I am worried about the consequences
			// of taking that out.  Querying the first value
			// here is harmless, so for now we go with that.
			// See Gnumeric #774.
			(void)go_data_vector_get_value (labels, 0);
			labels_nb = go_data_vector_get_len (labels);
		}
		for (cur = i = 0; i < len; i++) {
			val = go_data_vector_get_value (pos, i);
			if (!go_finite (val))
				continue;
			line->ticks[cur].position = val;
			if (i < labels_nb) {
				val = go_data_vector_get_value (labels, i);
				if (go_finite (val)) {
					axis_line_format_value (line, val, &line->ticks[cur].str);
					line->ticks[cur].type = GOG_AXIS_TICK_MAJOR;
				} else {
					lbl = go_data_vector_get_str (labels, i);
					if (lbl && *lbl) {
						line->ticks[cur].str = go_string_new (lbl);
						line->ticks[cur].type = GOG_AXIS_TICK_MAJOR;
					} else
						line->ticks[cur].type = GOG_AXIS_TICK_MINOR;
				}
			} else if (labels_nb == 0) {
					line->ticks[cur].str = go_string_new (go_data_vector_get_str (pos, i));
					line->ticks[cur].type = GOG_AXIS_TICK_MAJOR;
			} else
				line->ticks[cur].type = GOG_AXIS_TICK_MINOR;
			cur++;
		}
		line->tick_nbr = cur;
	}
}

#ifdef GOFFICE_WITH_GTK

static void
cb_axis_line_fmt_changed (G_GNUC_UNUSED GtkWidget *widget,
		     char *fmt,
		     GogAxis *axis)
{
	g_object_set (axis, "assigned-format-string-XL", fmt, NULL);
}

static void
gog_axis_line_populate_editor (GogObject *gobj,
			       GOEditor *editor,
			       GogDataAllocator *dalloc,
			       GOCmdContext *cc)
{
	GogAxisLine *line = GOG_AXIS_LINE (gobj);
	GogDataset *set = GOG_DATASET (gobj);
	GogDataEditor *gde;
	unsigned i;
	GtkBuilder *gui;
	GtkGrid *grid;

	(GOG_OBJECT_CLASS(gal_parent_klass)->populate_editor) (gobj, editor, dalloc, cc);

	gui = go_gtk_builder_load_internal ("res:go:graph/gog-axis-prefs.ui", GETTEXT_PACKAGE, cc);
	if (gui == NULL)
		return;

	grid = GTK_GRID (gtk_builder_get_object (gui, "custom-ticks-grid"));
	for (i = 1; i < 3; i++) {
		gde = gog_data_allocator_editor (dalloc, set, GOG_AXIS_ELEM_CROSS_POINT + i, GOG_DATA_VECTOR);
		g_object_set (G_OBJECT (gde), "hexpand", TRUE, NULL);
		gtk_grid_attach (grid, GTK_WIDGET (gde), 1, i, 1, 1);
	}
	gtk_widget_show_all (GTK_WIDGET (grid));
	go_editor_add_page (editor, grid, _("Ticks"));

	/* Format page */
    {
	    GOFormat *fmt = gog_axis_line_get_effective_format (line);
	    GtkWidget *w = go_format_sel_new_full (TRUE);

	    if (fmt)
		    go_format_sel_set_style_format (GO_FORMAT_SEL (w),
						    fmt);
		gtk_widget_show (w);

	    go_editor_add_page (editor, w, _("Format"));

	    g_signal_connect (G_OBJECT (w),
		    "format_changed", G_CALLBACK (cb_axis_line_fmt_changed), line);
    }
}
#endif

static void
gog_axis_line_set_property (GObject *obj, guint param_id,
		       GValue const *value, GParamSpec *pspec)
{
	GogAxisLine *line = GOG_AXIS_LINE (obj);

	switch (param_id) {
	case AXIS_LINE_PROP_ASSIGNED_FORMAT_STR_XL : {
		char const *str = g_value_get_string (value);
		GOFormat *newfmt = str ? go_format_new_from_XL (str) : NULL;
		if (go_format_eq (newfmt, line->assigned_format))
			go_format_unref (newfmt);
		else {
			go_format_unref (line->assigned_format);
			line->assigned_format = newfmt;
		}
		gog_axis_line_update_ticks (line);
		gog_object_emit_changed (GOG_OBJECT (obj), TRUE);
		break;
	}

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		return; /* NOTE : RETURN */
	}
}

static void
gog_axis_line_get_property (GObject *obj, guint param_id,
		       GValue *value, GParamSpec *pspec)
{
	GogAxisLine const *line = GOG_AXIS_LINE (obj);

	switch (param_id) {
	case AXIS_LINE_PROP_ASSIGNED_FORMAT_STR_XL :
		if (line->assigned_format != NULL)
			g_value_set_string (value,
				go_format_as_XL	(line->assigned_format));
		else
			g_value_set_static_string (value, NULL);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
gog_axis_line_finalize (GObject *obj)
{
	GogAxisLine *line = GOG_AXIS_LINE (obj);

	gog_axis_line_discard_ticks (line);
	go_format_unref (line->assigned_format);
	go_format_unref (line->format);

	gog_dataset_finalize (GOG_DATASET (line));
	(gal_parent_klass->finalize) (obj);
}

static void
gog_axis_line_class_init (GObjectClass *gobject_klass)
{
	GogObjectClass *gog_klass = (GogObjectClass *) gobject_klass;

	gal_parent_klass = g_type_class_peek_parent (gobject_klass);
#ifdef GOFFICE_WITH_GTK
	gog_klass->populate_editor	= gog_axis_line_populate_editor;
#endif
	gobject_klass->set_property = gog_axis_line_set_property;
	gobject_klass->get_property = gog_axis_line_get_property;
	gobject_klass->finalize = gog_axis_line_finalize;

	g_object_class_install_property (gobject_klass, AXIS_LINE_PROP_ASSIGNED_FORMAT_STR_XL,
		g_param_spec_string ("assigned-format-string-XL",
			_("Assigned XL format"),
			_("The user assigned format to use for non-discrete axis labels (XL format)"),
			"General",
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
}

static void
gog_axis_line_dataset_dims (GogDataset const *set, int *first, int *last)
{
	*first = GOG_AXIS_ELEM_CROSS_POINT;
	*last  = GOG_AXIS_ELEM_CROSS_POINT + 2;
}

static GogDatasetElement *
gog_axis_line_dataset_get_elem (GogDataset const *set, int dim_i)
{
	GogAxisLine *line = GOG_AXIS_LINE (set);

	g_return_val_if_fail (dim_i >= GOG_AXIS_ELEM_CROSS_POINT && dim_i <= GOG_AXIS_ELEM_CROSS_POINT + 2, NULL);

	return (dim_i == GOG_AXIS_ELEM_CROSS_POINT)?
		&line->base.cross_location:
		line->custom_ticks + (dim_i - 1 - GOG_AXIS_ELEM_CROSS_POINT);
}

static void
gog_axis_line_dim_changed (GogDataset *set, int dim_i)
{
	if (dim_i > GOG_AXIS_ELEM_CROSS_POINT)
		gog_axis_line_update_ticks (GOG_AXIS_LINE (set));
	gog_object_emit_changed (GOG_OBJECT (set), TRUE);
}

static void
gog_axis_line_dataset_init (GogDatasetClass *iface)
{
	iface->dims	   = gog_axis_line_dataset_dims;
	iface->get_elem	   = gog_axis_line_dataset_get_elem;
	iface->dim_changed = gog_axis_line_dim_changed;
}

GSF_CLASS_FULL (GogAxisLine, gog_axis_line,
		NULL, NULL, gog_axis_line_class_init, NULL,
		NULL /*init*/, GOG_TYPE_AXIS_BASE, 0,
		GSF_INTERFACE (gog_axis_line_dataset_init, GOG_TYPE_DATASET))

unsigned
gog_axis_line_get_ticks (GogAxisLine *axis_line, GogAxisTick **ticks)
{
	if (axis_line->custom_ticks[0].data != NULL && go_data_has_value (axis_line->custom_ticks[0].data)) {
		*ticks = axis_line->ticks;
		return axis_line->tick_nbr;
	}
	return 0;
}
