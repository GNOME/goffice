/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-axis-line.c :
 *
 * Copyright (C) 2005 Emmanuel Pacaud (emmanuel.pacaud@univ-poitiers.fr)
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include <goffice/goffice-config.h>

#include <goffice/graph/gog-axis-line-impl.h>
#include <goffice/graph/gog-axis.h>
#include <goffice/graph/gog-chart.h>
#include <goffice/graph/gog-data-allocator.h>
#include <goffice/graph/gog-renderer.h>
#include <goffice/graph/gog-style.h>
#include <goffice/graph/gog-theme.h>

#include <goffice/gtk/goffice-gtk.h>

#include <goffice/utils/go-math.h>

#include <gsf/gsf-impl-utils.h>

#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkcelllayout.h>
#include <gtk/gtkcombobox.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtktogglebutton.h>

#include <glib/gi18n.h>

static GogViewClass *gab_view_parent_klass;
static GObjectClass *gab_parent_klass;

enum {
	AXIS_BASE_PROP_0,
	AXIS_BASE_PROP_POSITION,
	AXIS_BASE_PROP_MAJOR_TICK_LABELED,
	AXIS_BASE_PROP_MAJOR_TICK_IN,
	AXIS_BASE_PROP_MAJOR_TICK_OUT,
	AXIS_BASE_PROP_MAJOR_TICK_SIZE_PTS,
	AXIS_BASE_PROP_MINOR_TICK_IN,
	AXIS_BASE_PROP_MINOR_TICK_OUT,
	AXIS_BASE_PROP_MINOR_TICK_SIZE_PTS,
	AXIS_BASE_PROP_CROSS_AXIS_ID,
	AXIS_BASE_PROP_CROSS_LOCATION
};

static double gog_axis_base_get_cross_location (GogAxisBase *axis_base);

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
			str = g_value_get_string (value);
			if (str == NULL)
				return;
			else if (!g_ascii_strcasecmp (str, "low"))
				position = GOG_AXIS_AT_LOW;
			else if (!g_ascii_strcasecmp (str, "cross"))
				position = GOG_AXIS_CROSS;
			else if (!g_ascii_strcasecmp (str, "high"))
				position = GOG_AXIS_AT_HIGH;
			else
				return;
			resized = (position != axis_base->position);
			axis_base->position = position;
			break;
		case AXIS_BASE_PROP_CROSS_AXIS_ID:
			axis_base->crossed_axis_id = g_value_get_uint (value);
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
			}
			break;
		case AXIS_BASE_PROP_CROSS_AXIS_ID:
			g_value_set_uint (value, axis_base->crossed_axis_id);
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
		if (IS_GOG_AXIS (child))
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

static GogAxisType
gog_axis_base_get_crossed_axis_type (GogAxisBase *axis_base)
{
	GogAxisType axis_type, crossed_type;
	GogAxisSet axis_set;

	axis_type = gog_axis_get_atype (axis_base->axis);
	axis_set = gog_chart_get_axis_set (axis_base->chart);

	crossed_type = GOG_AXIS_UNKNOWN;
	switch (axis_set) {
		case GOG_AXIS_SET_XY:
			if (axis_type == GOG_AXIS_X)
				crossed_type = GOG_AXIS_Y;
			else
				crossed_type = GOG_AXIS_X;
			break;
		default:
			break;
	}
	return crossed_type;
}

static GogAxis *
gog_axis_base_get_crossed_axis (GogAxisBase *axis_base)
{
	GogAxis *crossed_axis = NULL;
	GSList *axes, *ptr;
	gboolean found = FALSE;

	axes = gog_chart_get_axes (axis_base->chart, 
		gog_axis_base_get_crossed_axis_type (axis_base));
	g_return_val_if_fail (axes != NULL, NULL);
	
	for (ptr = axes; ptr != NULL && !found; ptr = ptr->next) {
		crossed_axis = GOG_AXIS (ptr->data);
		if (gog_object_get_id (GOG_OBJECT (crossed_axis)) == axis_base->crossed_axis_id)
			found = TRUE;
	}
	
	if (!found)
		crossed_axis = GOG_AXIS (axes->data);
	
	g_slist_free (axes);
	return crossed_axis;
}	

void
gog_axis_base_set_position_auto (GogAxisBase *axis_base) 
{
	GogAxis *axis;
	GogChart *chart;
	GogAxisPosition position;
	GSList *lines, *axes = NULL, *lptr, *aptr;
	gboolean can_at_low = TRUE, can_at_high = TRUE;

	g_return_if_fail (GOG_AXIS_BASE (axis_base) != NULL);

	if (IS_GOG_AXIS (axis_base))
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
			if (lptr->data == axis_base || !IS_GOG_AXIS_BASE (lptr->data))
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

	gog_axis_base_set_position (axis_base, position);
}	

typedef struct {
	GogAxisBase 	*axis_base;
	GladeXML 	*gui;
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
		(GTK_TOGGLE_BUTTON (glade_xml_get_widget (state->gui, "axis_cross")),
		 TRUE);
}

static void
cb_cross_axis_changed (GtkComboBox *combo, AxisBasePrefs *state)
{
	GtkTreeIter iter;
	GValue value;
	GtkTreeModel *model = gtk_combo_box_get_model (combo);

	gtk_combo_box_get_active_iter (combo, &iter);
	gtk_tree_model_get_value (model, &iter, 1, &value);
	state->axis_base->crossed_axis_id = g_value_get_uint (&value);

	gtk_toggle_button_set_active 
		(GTK_TOGGLE_BUTTON (glade_xml_get_widget (state->gui, "axis_cross")),
		 TRUE);
}

static void
cb_position_toggled (GtkWidget *button, GogAxisBase *axis_base)
{
	GogAxisPosition position;
	char const *widget_name = gtk_widget_get_name (button);
	GSList *lines, *axes, *aptr, *lptr;
	
	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
		return;

	if (g_ascii_strcasecmp ("axis_high", widget_name) == 0)
		position = GOG_AXIS_AT_HIGH;
	else if (g_ascii_strcasecmp ("axis_cross", widget_name) == 0) 
		position = GOG_AXIS_CROSS;
	else 
		position = GOG_AXIS_AT_LOW;

	if (position != GOG_AXIS_CROSS) {
		axes = gog_chart_get_axes (axis_base->chart, gog_axis_get_atype (axis_base->axis));
		for (aptr = axes; aptr != NULL; aptr = aptr->next) {
			lines = gog_object_get_children (GOG_OBJECT (aptr->data), NULL);
			lines = g_slist_prepend (lines, aptr->data);
			for (lptr = lines; lptr != NULL; lptr = lptr->next) {
				if (lptr->data == axis_base || !IS_GOG_AXIS_BASE (lptr->data))
					continue;
				if (position == gog_axis_base_get_position (GOG_AXIS_BASE (lptr->data))) { 
					gog_axis_base_set_position (GOG_AXIS_BASE (lptr->data),
								    gog_axis_base_get_position (axis_base));
					break;
				}
			}
			g_slist_free (lines);
		}
		g_slist_free (axes);
	}
	gog_axis_base_set_position (axis_base, position);
	gog_object_emit_changed (GOG_OBJECT (axis_base), TRUE);
}

static void
cb_tick_toggle_changed (GtkToggleButton *toggle_button, GObject *axis_base)
{
	g_object_set (axis_base,
		gtk_widget_get_name (GTK_WIDGET (toggle_button)),
		gtk_toggle_button_get_active (toggle_button),
		NULL);
}

static void
gog_axis_base_populate_editor (GogObject *gobj, 
			       GogEditor *editor, 
			       GogDataAllocator *dalloc, 
			       GOCmdContext *cc)
{
	static char const *toggle_props[] = {
		"major-tick-labeled",
		"major-tick-out",
		"major-tick-in",
		"minor-tick-out",
		"minor-tick-in"
	};
	GogAxis *crossed_axis;
	GogAxisBase *axis_base;
	GladeXML *gui;
	GtkListStore *store;
	GtkTreeIter iter;
	GtkWidget *combo, *data_editor, *container, *w;
	GtkCellRenderer *cell;
	GSList *axes, *ptr;
	AxisBasePrefs *state;
	GogAxisType crossed_axis_type;
	static guint axis_base_pref_page = 0;
	unsigned axis_count;
	unsigned crossed_axis_id;
	unsigned i;
	
	axis_base = GOG_AXIS_BASE (gobj);
	g_return_if_fail (GOG_AXIS_BASE (axis_base) != NULL);

	gui = go_libglade_new ("gog-axis-prefs.glade", "axis_base_pref_box", NULL, cc);
	if (gui == NULL)
		return;
	
	crossed_axis_type = gog_axis_base_get_crossed_axis_type (axis_base);
	if (crossed_axis_type != GOG_AXIS_UNKNOWN) {
		store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_UINT);
		combo = glade_xml_get_widget (gui, "cross_axis_combo");
		gtk_combo_box_set_model (GTK_COMBO_BOX (combo), GTK_TREE_MODEL (store));

		cell = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), cell,
						"text", 0,
						NULL);

		axes = gog_chart_get_axes (axis_base->chart, crossed_axis_type); 
		axis_count = 0;
		for (ptr = axes; ptr != NULL; ptr = ptr->next) {
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

		data_editor = gog_data_allocator_editor (dalloc, GOG_DATASET (axis_base),
			GOG_AXIS_ELEM_CROSS_POINT, GOG_DATA_SCALAR);
		container = glade_xml_get_widget (gui, "cross_location_alignment");
		gtk_container_add (GTK_CONTAINER (container), data_editor);
		gtk_widget_show_all (container);
		
		state = g_new (AxisBasePrefs, 1);
		state->axis_base = axis_base;
		state->gui = gui;
		g_signal_connect (G_OBJECT (combo), "changed",
				  G_CALLBACK (cb_cross_axis_changed), state);
		g_signal_connect (G_OBJECT (data_editor), "changed",
				  G_CALLBACK (cb_cross_location_changed), state);
		g_object_set_data_full (G_OBJECT (combo),
					"state", state, (GDestroyNotify) axis_base_pref_free);

		w = glade_xml_get_widget (gui, "axis_low");
		if (axis_base->position == GOG_AXIS_AT_LOW)
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), TRUE);
		g_signal_connect (G_OBJECT (w), "toggled",
				  G_CALLBACK (cb_position_toggled), axis_base);
		w = glade_xml_get_widget (gui, "axis_cross");
		if (axis_base->position == GOG_AXIS_CROSS)
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), TRUE);
		g_signal_connect (G_OBJECT (w), "toggled",
				  G_CALLBACK (cb_position_toggled), axis_base);
		w = glade_xml_get_widget (gui, "axis_high");
		if (axis_base->position == GOG_AXIS_AT_HIGH)
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), TRUE);
		g_signal_connect (G_OBJECT (w), "toggled",
				  G_CALLBACK (cb_position_toggled), axis_base);
	}
	else {
		w = glade_xml_get_widget (gui, "position_box");
		gtk_widget_hide (w);
	}
	
	for (i = 0; i < G_N_ELEMENTS (toggle_props) ; i++) {
		gboolean cur_val;
		
		w = glade_xml_get_widget (gui, toggle_props[i]);
		g_object_get (G_OBJECT (gobj), toggle_props[i], &cur_val, NULL);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), cur_val);
		g_signal_connect_object (G_OBJECT (w),
					 "toggled",
					 G_CALLBACK (cb_tick_toggle_changed), axis_base, 0);
	}
	if (gog_axis_is_discrete (axis_base->axis)) {
		/* Hide minor tick properties */
		GtkWidget *w = glade_xml_get_widget (gui, "minor_tick_box");
		gtk_widget_hide (w);
	}

	gog_editor_add_page (editor, 
			     glade_xml_get_widget (gui, "axis_base_pref_box"),
			     _("Layout"));

	/* Style page */
	(GOG_OBJECT_CLASS(gab_parent_klass)->populate_editor) (gobj, editor, dalloc, cc);
	
	gog_editor_set_store_page (editor, &axis_base_pref_page);
}

static void
gog_axis_base_init_style (GogStyledObject *gso, GogStyle *style)
{
	style->interesting_fields = GOG_STYLE_LINE | GOG_STYLE_FONT;
	gog_theme_fillin_style (gog_object_get_theme (GOG_OBJECT (gso)),
		style, GOG_OBJECT (gso), 0, FALSE);
}

static void
gog_axis_base_class_init (GObjectClass *gobject_klass)
{
	GogObjectClass *gog_klass = (GogObjectClass *) gobject_klass;
	GogStyledObjectClass *gso_klass = (GogStyledObjectClass *) gobject_klass;

	gab_parent_klass = g_type_class_peek_parent (gobject_klass);
	gobject_klass->set_property 	= gog_axis_base_set_property;
	gobject_klass->get_property 	= gog_axis_base_get_property;
	gobject_klass->finalize	    	= gog_axis_base_finalize;
	gog_klass->parent_changed 	= gog_axis_base_parent_changed; 

	g_object_class_install_property (gobject_klass, AXIS_BASE_PROP_POSITION,
		g_param_spec_string ("pos_str", "pos_str",
			"Where to position an axis low, high, or crossing",
			"low", G_PARAM_READWRITE | GOG_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, AXIS_BASE_PROP_MAJOR_TICK_LABELED,
		g_param_spec_boolean ("major-tick-labeled", NULL,
			"Show labels for major ticks",
			TRUE, G_PARAM_READWRITE | GOG_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, AXIS_BASE_PROP_MAJOR_TICK_IN,
		g_param_spec_boolean ("major-tick-in", NULL,
			"Major tick marks inside the axis",
			FALSE, G_PARAM_READWRITE | GOG_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, AXIS_BASE_PROP_MAJOR_TICK_OUT,
		g_param_spec_boolean ("major-tick-out", NULL,
			"Major tick marks outside the axis",
			TRUE, G_PARAM_READWRITE | GOG_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, AXIS_BASE_PROP_MAJOR_TICK_SIZE_PTS,
		g_param_spec_int ("major-tick-size-pts", "major-tick-size-pts",
			"Size of the major tick marks in pts",
			0, 20, 4, G_PARAM_READWRITE | GOG_PARAM_PERSISTENT));

	g_object_class_install_property (gobject_klass, AXIS_BASE_PROP_MINOR_TICK_IN,
		g_param_spec_boolean ("minor-tick-in", NULL,
			"Minor tick marks inside the axis",
			FALSE, G_PARAM_READWRITE | GOG_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, AXIS_BASE_PROP_MINOR_TICK_OUT,
		g_param_spec_boolean ("minor-tick-out", NULL,
			"Minor tick marks outside the axis",
			FALSE, G_PARAM_READWRITE | GOG_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, AXIS_BASE_PROP_MINOR_TICK_SIZE_PTS,
		g_param_spec_int ("minor-tick-size-pts", NULL,
			"Size of the minor tick marks in pts",
			0, 15, 2, G_PARAM_READWRITE | GOG_PARAM_PERSISTENT));

	g_object_class_install_property (gobject_klass, AXIS_BASE_PROP_CROSS_AXIS_ID,
		g_param_spec_uint ("cross_axis_id", NULL,
			"Which axis to cross",
			0, G_MAXUINT, 0, G_PARAM_READWRITE | GOG_PARAM_PERSISTENT));

	gog_klass->populate_editor	= gog_axis_base_populate_editor;
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

static double
gog_axis_base_get_cross_location (GogAxisBase *axis_base)
{
	GOData *data;
	
	g_return_val_if_fail (GOG_AXIS_BASE (axis_base) != NULL, 0.);
	
	data = axis_base->cross_location.data;
	if (data != NULL && IS_GO_DATA_SCALAR (data)) 
		return go_data_scalar_get_value (GO_DATA_SCALAR (data));

	return 0.;
}

GogAxisPosition
gog_axis_base_get_position (GogAxisBase *axis_base)
{
	g_return_val_if_fail (GOG_AXIS_BASE (axis_base) != NULL, GOG_AXIS_AT_LOW);

	return axis_base->position;
}

void
gog_axis_base_set_position (GogAxisBase *axis_base, GogAxisPosition position)
{
	g_return_if_fail (GOG_AXIS_BASE (axis_base) != NULL);

	axis_base->position = position;
}

GSF_CLASS_ABSTRACT (GogAxisBase, gog_axis_base,
		    gog_axis_base_class_init, gog_axis_base_init,
		    GOG_STYLED_OBJECT_TYPE);

/************************************************************************/

static gboolean
overlap (GogViewAllocation const *bbox1, GogViewAllocation const *bbox2) {
	return (!((MAX (bbox2->x, bbox2->x + bbox2->w) < MIN (bbox1->x, bbox1->x + bbox1->w)) ||
		  (MAX (bbox2->y, bbox2->y + bbox2->h) < MIN (bbox1->y, bbox1->y + bbox1->h)) ||
		  (MIN (bbox2->x, bbox2->x + bbox2->w) > MAX (bbox1->x, bbox1->x + bbox1->w)) ||
		  (MIN (bbox2->y, bbox2->y + bbox2->h) > MAX (bbox1->y, bbox1->y + bbox1->h))));
}

static void
compute_angles (double x, double y, double w, double h,
		double side,
		double *axis_length,
		double *axis_angle,
		double *label_angle,
		double *cos_alpha,
		double *sin_alpha)
{
	*axis_length = sqrt (w * w + h * h);
	if (w != 0.) {
		*axis_angle = atan (h / w);
		if (w < 0.) {
			*axis_angle += M_PI;
		}
	} else {
		if (h < 0.) {
			*axis_angle = - M_PI/2.0;
		} else {
			*axis_angle = M_PI/2.0;
		}
	}
	*label_angle = fmod (*axis_angle + 2.0 * M_PI, M_PI);
	if (*label_angle > M_PI / 2.0)
		*label_angle = M_PI - *label_angle;
	*cos_alpha = cos (*axis_angle + side * M_PI / 2.0);
	*sin_alpha = sin (*axis_angle + side * M_PI / 2.0);
}

static void
update_bbox (GogViewAllocation *bbox, GogViewAllocation *area)
{
	double min, max;
	
	if (area->w > 0.) {
		min = MIN (bbox->x, area->x);
		max = MAX (bbox->x + bbox->w, area->x + area->w);
	} else {
		min = MIN (bbox->x, area->x + area->w);
		max = MAX (bbox->x + bbox->w, area->x);
	}
	bbox->x = min;
	bbox->w = max - min;
	
	if (area->h > 0.) {
		min = MIN (bbox->y, area->y);
		max = MAX (bbox->y + bbox->h, area->y + area->h);
	} else {
		min = MIN (bbox->y, area->y + area->h);
		max = MAX (bbox->y + bbox->h, area->y);
	}
	bbox->y = min;
	bbox->h = max - min;
}

static GogViewAllocation
axis_line_get_bbox (GogAxisBase *axis_base, GogRenderer *renderer,
		    double x, double y, double w, double h,
		    double tick_angle, double start_at, gboolean draw_labels)
{
	GogAxisMap *map = NULL;
	GogAxisTick *ticks;
	GogViewRequisition txt_size;
	GogViewAllocation total_bbox, bbox;
	double line_width;
	double axis_length, axis_angle, label_angle;
	double cos_alpha, sin_alpha;
	double pos, offset, label_offset;
	unsigned i, tick_nbr;
	gboolean is_line_visible;
	double minor_tick_len, major_tick_len, tick_len;

	compute_angles (x, y, w, h, tick_angle, &axis_length, &axis_angle, &label_angle, &cos_alpha, &sin_alpha);

	is_line_visible = gog_style_is_line_visible (axis_base->base.style);
	line_width = gog_renderer_line_size (renderer, axis_base->base.style->line.width) / 2;
	
	minor_tick_len = gog_renderer_pt2r_x (renderer, axis_base->minor.size_pts);
	major_tick_len = gog_renderer_pt2r_x (renderer, axis_base->major.size_pts);
	tick_len = axis_base->major.tick_out ? major_tick_len : 
		(axis_base->minor.tick_out ? minor_tick_len : 0.);
	gog_renderer_measure_text (renderer, "0", &txt_size);
	label_offset = txt_size.w / 2.0 * cos_alpha + tick_len;

	total_bbox.x = x; total_bbox.y = y; total_bbox.w = w; total_bbox.h = h;

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
		update_bbox (&total_bbox, &bbox);
		bbox.x += w;
		bbox.y += h;
		update_bbox (&total_bbox, &bbox);
	}

	tick_nbr = gog_axis_get_ticks (axis_base->axis, &ticks);

	if (!draw_labels)
		return total_bbox;
	
	map = gog_axis_map_new (axis_base->axis, 0., axis_length);
	
	for (i = 0; i < tick_nbr; i++) {
		if (ticks[i].label != NULL) { 
			pos = gog_axis_map_to_view (map, ticks[i].position);
			gog_renderer_measure_text (renderer, ticks[i].label, &txt_size);
			offset = (txt_size.h * cos (label_angle) + txt_size.w * sin (label_angle)) / 2.0 + label_offset;
			bbox.x = x + pos * cos (axis_angle) - offset * cos_alpha - txt_size.w / 2.0;
			bbox.y = y + pos * sin (axis_angle) - offset * sin_alpha - txt_size.h / 2.0;
			bbox.w = txt_size.w;
			bbox.h = txt_size.h;
			update_bbox (&total_bbox, &bbox);
		}	
	}
	gog_axis_map_free (map);

	return total_bbox;
}

static void
axis_line_render (GogAxisBase *axis_base, GogRenderer *renderer,
		  double x, double y, double w, double h,
		  double tick_angle,
		  double start_at,
		  gboolean draw_labels,
		  gboolean sharp)
{
	GogAxisMap *map = NULL;
	GogAxisTick *ticks;
	GogViewRequisition txt_size;
	GogViewAllocation label_pos, label_result, label_old = {0., 0., 0., 0.};
	ArtVpath path[3];
	double line_width;
	double axis_length, axis_angle, label_angle;
	double major_tick_len, minor_tick_len, tick_len;
	double major_out_x = 0., major_out_y= 0., major_in_x = 0., major_in_y = 0.;
	double minor_out_x = 0., minor_out_y= 0., minor_in_x = 0., minor_in_y = 0.;
	double cos_alpha, sin_alpha;
	double pos, pos_x, pos_y, offset, label_offset, tick_offset;
	unsigned i, tick_nbr;
	gboolean draw_major, draw_minor;
	gboolean is_line_visible;

	compute_angles (x, y, w, h, tick_angle, &axis_length, &axis_angle, &label_angle, &cos_alpha, &sin_alpha);

	is_line_visible = gog_style_is_line_visible (axis_base->base.style);
	line_width = gog_renderer_line_size (renderer, axis_base->base.style->line.width) / 2;
	if (is_line_visible)
	{
		path[0].code = ART_MOVETO;
		path[1].code = ART_LINETO;
		path[2].code = ART_END;

		path[0].x = x;
		path[0].y = y;
		path[1].x = path[0].x + w;
		path[1].y = path[0].y + h;
		if (sharp)
			gog_renderer_draw_sharp_path (renderer, path, NULL);
		else
			gog_renderer_draw_path (renderer, path, NULL);
	}

	map = gog_axis_map_new (axis_base->axis, 0., axis_length);

	draw_major = axis_base->major.tick_in || axis_base->major.tick_out;
	draw_minor = axis_base->minor.tick_in || axis_base->minor.tick_out;

	minor_tick_len = gog_renderer_pt2r_x (renderer, axis_base->minor.size_pts) + line_width;
	minor_out_x = axis_base->minor.tick_out ? - minor_tick_len * cos_alpha : 0.;
	minor_out_y = axis_base->minor.tick_out ? - minor_tick_len * sin_alpha : 0.;
	minor_in_x = axis_base->minor.tick_in ? minor_tick_len * cos_alpha : 0.;
	minor_in_y = axis_base->minor.tick_in ? minor_tick_len * sin_alpha : 0.;

	major_tick_len = gog_renderer_pt2r_x (renderer, axis_base->major.size_pts) + line_width;
	major_out_x = axis_base->major.tick_out ? - major_tick_len * cos_alpha : 0.;
	major_out_y = axis_base->major.tick_out ? - major_tick_len * sin_alpha : 0.;
	major_in_x = axis_base->major.tick_in ? major_tick_len * cos_alpha : 0.;
	major_in_y = axis_base->major.tick_in ? major_tick_len * sin_alpha : 0.;
	
	tick_len = axis_base->major.tick_out ? major_tick_len : 
		(axis_base->minor.tick_out ? minor_tick_len : 0.);
	gog_renderer_measure_text (renderer, "0", &txt_size);
	label_offset = txt_size.w / 2.0 * cos_alpha + tick_len;

	tick_nbr = gog_axis_get_ticks (axis_base->axis, &ticks);
	tick_offset = gog_axis_is_discrete (axis_base->axis) && 
		!gog_axis_is_center_on_ticks (axis_base->axis) ? 
		-0.5 : 0.0;

	for (i = 0; i < tick_nbr; i++) {
		if (gog_axis_map (map, ticks[i].position) < start_at)
			continue;

		pos = gog_axis_map_to_view (map, ticks[i].position + tick_offset);
		pos_x = x + pos * cos (axis_angle);
		pos_y = y + pos * sin (axis_angle);

		if (is_line_visible) {
			switch (ticks[i].type) {
				case GOG_AXIS_TICK_MAJOR:
					if (draw_major) {
						path[0].x = major_out_x + pos_x;
						path[1].x = major_in_x + pos_x;
						path[0].y = major_out_y + pos_y;
						path[1].y = major_in_y + pos_y;
						if (sharp)
							gog_renderer_draw_sharp_path (renderer, path, NULL);
						else
							gog_renderer_draw_path (renderer, path, NULL);
					}
					break;

				case GOG_AXIS_TICK_MINOR:
					if (draw_minor) {
						path[0].x = minor_out_x + pos_x;
						path[1].x = minor_in_x + pos_x;
						path[0].y = minor_out_y + pos_y;
						path[1].y = minor_in_y + pos_y;
						if (sharp)
							gog_renderer_draw_sharp_path (renderer, path, NULL);
						else
							gog_renderer_draw_path (renderer, path, NULL);
					}
					break;

				default:
					break;
			}
		}

		if (ticks[i].label != NULL && draw_labels) { 
			pos = gog_axis_map_to_view (map, ticks[i].position);
			gog_renderer_measure_text (renderer, ticks[i].label, &txt_size);
			offset = (txt_size.h * cos (label_angle) + txt_size.w * sin (label_angle)) / 2.0 + label_offset;
			label_pos.x = x + pos * cos (axis_angle) - offset * cos_alpha;
			label_pos.y = y + pos * sin (axis_angle) - offset * sin_alpha;
			label_pos.w = txt_size.w;
			label_pos.h = txt_size.h;
			if (!overlap (&label_pos, &label_old)) {
				gog_renderer_draw_text (renderer, ticks[i].label,
							&label_pos, GTK_ANCHOR_CENTER, &label_result);
				label_old = label_pos;
			}
		}	
	}

	gog_axis_map_free (map);
}

static GogViewAllocation
axis_circle_get_bbox (GogAxisBase *axis_base, GogRenderer *renderer, 
		      double center_x, double center_y, double radius_x, double radius_y,
		      gboolean draw_labels)
{
	GogAxisMap *map;
	GogAxisTick *ticks;
	GogViewAllocation total_bbox, bbox;
	GogViewRequisition txt_size;
	double circular_min, circular_max;
	double angle, offset, label_offset, label_angle;
	double major_tick_len, minor_tick_len, tick_len;
	unsigned i, tick_nbr;

	total_bbox.x = center_x; total_bbox.y = center_y; total_bbox.w = 0.; total_bbox.h = 0.;

	minor_tick_len = gog_renderer_pt2r_x (renderer, axis_base->minor.size_pts);
	major_tick_len = gog_renderer_pt2r_x (renderer, axis_base->major.size_pts);
	tick_len = axis_base->major.tick_out ? major_tick_len : 
		(axis_base->minor.tick_out ? minor_tick_len : 0.);
	gog_renderer_measure_text (renderer, "0", &txt_size);
	label_offset = txt_size.w / 2.0;

	gog_axis_get_bounds (axis_base->axis, &circular_min, &circular_max);
	tick_nbr = gog_axis_get_ticks (axis_base->axis, &ticks);
	map = gog_axis_map_new (axis_base->axis, -M_PI / 2.0, 2.0 * M_PI * circular_max / (circular_max + 1.0));
	for (i = 0; i < tick_nbr; i++) {
		if (ticks[i].label != NULL && draw_labels) {
			angle = gog_axis_map_to_view (map, ticks[i].position);
			label_angle = fmod (angle + 2.0 * M_PI, M_PI);
			if (label_angle > M_PI / 2.0)
				label_angle = M_PI - label_angle;
			gog_renderer_measure_text (renderer, ticks[i].label, &txt_size);
			offset = (txt_size.w * cos (label_angle) + txt_size.h * sin (label_angle)) / 2.0 + 
				tick_len + label_offset * cos (label_angle);
			bbox.x = center_x + (radius_x + offset) * cos (angle) - txt_size.w / 2.0;
			bbox.y = center_y + (radius_y + offset) * sin (angle) - txt_size.h / 2.0;
			bbox.w = txt_size.w;
			bbox.h = txt_size.h;
			update_bbox (&total_bbox, &bbox);
		}
	}
	gog_axis_map_free (map);

	return total_bbox;
}

#define CIRCLE_STEP_NBR		360

static void
axis_circle_render (GogAxisBase *axis_base, GogRenderer *renderer, 
		    double center_x, double center_y, double radius, 
		    gboolean as_polygon, gboolean draw_labels)
{
	GogAxisMap *map;
	GogAxisTick *ticks;
	GogViewAllocation label_pos, label_result, label_old = {0., 0., 0., 0.};
	GogViewRequisition txt_size;
	ArtVpath *path;
	double circular_min, circular_max;
	double angle, offset, label_offset, label_angle;
	double major_tick_len, minor_tick_len, tick_len;
	unsigned num_radii, i, step_nbr, tick_nbr;
	
	gog_axis_get_bounds (axis_base->axis, &circular_min, &circular_max);
	num_radii = rint (circular_max + 1);
	if (num_radii < 1)
		return;

	step_nbr = as_polygon ? num_radii : CIRCLE_STEP_NBR;
	path = art_new (ArtVpath, step_nbr + 2);
	for (i = 0; i <= step_nbr; i++) {
		angle = 2.0 * M_PI * i / step_nbr - M_PI / 2.0;
		path[i].x = center_x + radius * cos (angle);
		path[i].y = center_y + radius * sin (angle);
		path[i].code = ART_LINETO;
	}
	path[0].code = ART_MOVETO;
	path[step_nbr + 1].code = ART_END;
	gog_renderer_draw_path (renderer, path, NULL);
	g_free (path);

	minor_tick_len = gog_renderer_pt2r_x (renderer, axis_base->minor.size_pts);
	major_tick_len = gog_renderer_pt2r_x (renderer, axis_base->major.size_pts);
	tick_len = axis_base->major.tick_out ? major_tick_len : 
		(axis_base->minor.tick_out ? minor_tick_len : 0.);
	gog_renderer_measure_text (renderer, "0", &txt_size);
	label_offset = txt_size.w / 2.0;
	
	tick_nbr = gog_axis_get_ticks (axis_base->axis, &ticks);
	map = gog_axis_map_new (axis_base->axis, -M_PI / 2.0, 2.0 * M_PI * circular_max / (circular_max + 1.0));
	for (i = 0; i < tick_nbr; i++) {
		if (ticks[i].label != NULL && draw_labels) {
			angle = gog_axis_map_to_view (map, ticks[i].position);
			label_angle = fmod (angle + 2.0 * M_PI, M_PI);
			if (label_angle > M_PI / 2.0)
				label_angle = M_PI - label_angle;
			gog_renderer_measure_text (renderer, ticks[i].label, &txt_size);
			offset = (txt_size.w * cos (label_angle) + txt_size.h * sin (label_angle)) / 2.0 + 
				tick_len + label_offset * cos (label_angle);
			label_pos.x = center_x + (radius + offset) * cos (angle);
			label_pos.y = center_y + (radius + offset) * sin (angle);
			label_pos.w = txt_size.w;
			label_pos.h = txt_size.h;
			if (!overlap (&label_pos, &label_old)) {
				gog_renderer_draw_text (renderer, ticks[i].label,
							&label_pos, GTK_ANCHOR_CENTER, &label_result);
				label_old = label_pos;
			}
		}
	}
	gog_axis_map_free (map);
}

static void 
xy_process (GogView *view, GogViewPadding *padding, GogViewAllocation const *plot_area)
{
	GogAxisBase *axis_base = GOG_AXIS_BASE (view->model);
	GogChartMap *map;
	GogViewAllocation tmp = *plot_area;
	GogViewAllocation axis_line_bbox;
	double ax, ay, bx, by;
	GogAxisType axis_type = gog_axis_get_atype (axis_base->axis);
	double position = 0.;
	double side = 1.;

	g_return_if_fail (axis_type == GOG_AXIS_X || 
			  axis_type == GOG_AXIS_Y);
	
	map = gog_chart_map_new (axis_base->chart, plot_area);
	
	switch (axis_base->position) {
		case GOG_AXIS_AT_LOW:
			position = 0;
			break;
		case GOG_AXIS_CROSS:
			{
				GogAxis *cross_axis = gog_axis_base_get_crossed_axis (axis_base);
				double location = gog_axis_base_get_cross_location (axis_base);
				GogAxisMap *map = gog_axis_map_new (cross_axis, 0, 1.0);
				position = gog_axis_map (map, location);
				gog_axis_map_free (map);
				break;
			}
		case GOG_AXIS_AT_HIGH:
			position = 1.0;
			break;
	}

	if (axis_type == GOG_AXIS_X) {
		gog_chart_map_2D (map, 0.0, position, &ax, &ay);
		gog_chart_map_2D (map, 1.0, position, &bx, &by);
		side = -1.0;
	} else {
		gog_chart_map_2D (map, position, 0.0, &ax, &ay);
		gog_chart_map_2D (map, position, 1.0, &bx, &by);
		side = 1.0;
	}

	gog_chart_map_free (map);

	/* FIXME: probably rounding issue */
	if (position < 0. || position > 1.0)
		return;

	if (axis_base->position == GOG_AXIS_AT_HIGH)
		side = -side;

	if (padding == NULL) {
		axis_line_render (GOG_AXIS_BASE (view->model), 
			view->renderer, ax, ay, bx - ax , by - ay, side, -1., 
			axis_base->major_tick_labeled, TRUE);
	} else {
		axis_line_bbox = axis_line_get_bbox (GOG_AXIS_BASE (view->model),
			view->renderer, ax, ay, bx - ax, by - ay, side, -1.,
			axis_base->major_tick_labeled);
		padding->wl = MAX (0., tmp.x - axis_line_bbox.x);
		padding->ht = MAX (0., tmp.y - axis_line_bbox.y);
		padding->wr = MAX (0., axis_line_bbox.x + axis_line_bbox.w - tmp.x - tmp.w);
		padding->hb = MAX (0., axis_line_bbox.y + axis_line_bbox.h - tmp.y - tmp.h);
	}
}

static void
calc_polygon_parameters (GogViewAllocation const *area, unsigned edge_nbr,
			 double *x, double *y, double *radius)
{
	double width, height;

	width = 2.0 * sin (2.0 * M_PI * rint (edge_nbr / 4.0) / edge_nbr);
	height = 1.0 - cos (2.0 * M_PI * rint (edge_nbr / 2.0) / edge_nbr);

	*radius = (area->w / width) > (area->h / height) ?
		area->h / height :
		area->w / width;

	*x = area->x + area->w / 2.0;
	*y = area->y + *radius + (area->h - *radius * height) / 2.0;
}

static void 
radar_process (GogView *view, GogViewPadding *padding, GogViewAllocation const *area)
{
	GogAxisBase *axis_base = GOG_AXIS_BASE (view->model);
	GogAxis  *circular_axis;
	GogAxisType axis_type = gog_axis_get_atype (axis_base->axis);
	GogViewAllocation tmp = *area;
	GogViewAllocation axis_circle_bbox;
	GSList   *axis_list;
	double    center_x, center_y, radius, height;
	unsigned  i, num_radii;
	double    circular_min, circular_max;

	g_return_if_fail (axis_type == GOG_AXIS_CIRCULAR || 
			  axis_type == GOG_AXIS_RADIAL);

	if (axis_type == GOG_AXIS_RADIAL) {
		axis_list = gog_chart_get_axes (axis_base->chart, GOG_AXIS_CIRCULAR);
		g_return_if_fail (axis_list != NULL);
		g_return_if_fail (axis_list->data != NULL);
		g_return_if_fail (IS_GOG_AXIS(axis_list->data));
		circular_axis = GOG_AXIS (axis_list->data);
		g_slist_free (axis_list);
	} else
		circular_axis = axis_base->axis;

	gog_axis_get_bounds (circular_axis, &circular_min, &circular_max);
	num_radii = rint (circular_max + 1);
	calc_polygon_parameters (area, num_radii, &center_x, &center_y, &radius);

	switch (axis_type) {
		case GOG_AXIS_RADIAL:
			if (padding == NULL)
				for (i = 0; i < num_radii; i++) {
					double angle_rad = i * (2.0 * M_PI / num_radii);
					axis_line_render (GOG_AXIS_BASE (view->model), view->renderer,
							  center_x, center_y,
							  radius * sin (angle_rad),
							  - radius * cos (angle_rad),
							  1, 0.1, i == 0 && axis_base->major_tick_labeled,
							  FALSE);
				}
			break;
		case GOG_AXIS_CIRCULAR:
			if (padding == NULL)
				axis_circle_render (GOG_AXIS_BASE (view->model), view->renderer,
						    center_x, center_y, radius, TRUE, 
						    axis_base->major_tick_labeled);
			else {
				height = 1.0 - cos (2.0 * M_PI * rint (num_radii / 2.0) / num_radii);
				axis_circle_bbox = axis_circle_get_bbox (GOG_AXIS_BASE (view->model),
					view->renderer, 
					area->x + (area->w / 2.0), area->y + (area->h / height),
					area->w / 2.0, area->h / height, 
					axis_base->major_tick_labeled);
				padding->wl = MAX (0., tmp.x - axis_circle_bbox.x);
				padding->ht = MAX (0., tmp.y - axis_circle_bbox.y);
				padding->wr = MAX (0., axis_circle_bbox.x + axis_circle_bbox.w - tmp.x - tmp.w);
				padding->hb = MAX (0., axis_circle_bbox.y + axis_circle_bbox.h - tmp.y - tmp.h);
			}
			break;
		default:
			break;
	}
}

static void
gog_axis_base_view_padding_request (GogView *view, GogViewAllocation const *bbox, GogViewPadding *padding)
{
	GogAxisSet axis_set;
	GogAxisBase *axis_base = GOG_AXIS_BASE (view->model);
	GogStyle *style = axis_base->base.style;

	axis_set = gog_chart_get_axis_set (axis_base->chart);

	gog_renderer_push_style (view->renderer, style);

	switch (axis_set) {
		case GOG_AXIS_SET_X:
		case GOG_AXIS_SET_XY:
			xy_process (view, padding, bbox);
			break;
		case GOG_AXIS_SET_RADAR:
			radar_process (view, padding, bbox);
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
	GogStyle *style = axis_base->base.style;
	GogViewAllocation const *plot_area;

	axis_set = gog_chart_get_axis_set (axis_base->chart);
	/* FIXME: not nice */
	if (IS_GOG_AXIS (view->model))
		plot_area = gog_chart_view_get_plot_area (view->parent);
	else
		plot_area = gog_chart_view_get_plot_area (view->parent->parent);

	gog_renderer_push_style (view->renderer, style);

	switch (axis_set) {
		case GOG_AXIS_SET_X:
		case GOG_AXIS_SET_XY:
			xy_process (view, NULL, plot_area);
			break;
		case GOG_AXIS_SET_RADAR:
			radar_process (view, NULL, plot_area);
			break;
		default:
			g_warning ("[AxisBaseView::render] not implemented for this axis set (%i)",
				   axis_set);
			break;
	}

	gog_renderer_pop_style (view->renderer);
}

static void
gog_axis_base_view_class_init (GogAxisBaseViewClass *gview_klass)
{
	GogViewClass *view_klass = (GogViewClass *) gview_klass;

	gab_view_parent_klass = g_type_class_peek_parent (gview_klass);
	
	view_klass->padding_request 	= gog_axis_base_view_padding_request;
	view_klass->render 		= gog_axis_base_view_render;
}

GSF_CLASS (GogAxisBaseView, gog_axis_base_view,
	   gog_axis_base_view_class_init, NULL,
	   GOG_VIEW_TYPE)

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

struct _GogAxisLine {
	GogAxisBase	base;
};

typedef GogAxisBaseClass GogAxisLineClass;

static GObjectClass *gal_parent_klass;

static void
gog_axis_line_class_init (GObjectClass *gobject_klass)
{
	gal_parent_klass = g_type_class_peek_parent (gobject_klass);
}

static void
gog_axis_line_dataset_dims (GogDataset const *set, int *first, int *last)
{
	*first = GOG_AXIS_ELEM_CROSS_POINT;
	*last  = GOG_AXIS_ELEM_CROSS_POINT;
}

static GogDatasetElement *
gog_axis_line_dataset_get_elem (GogDataset const *set, int dim_i)
{
	GogAxisBase *axis_base = GOG_AXIS_BASE (set);

	g_return_val_if_fail (dim_i == GOG_AXIS_ELEM_CROSS_POINT, NULL);

	return &axis_base->cross_location;
}

static void
gog_axis_line_dim_changed (GogDataset *set, int dim_i)
{
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
		gog_axis_line_class_init, NULL /*init*/,
		GOG_AXIS_BASE_TYPE, 0,
		GSF_INTERFACE (gog_axis_line_dataset_init, GOG_DATASET_TYPE))
