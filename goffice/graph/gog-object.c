/*
 * gog-object.c :
 *
 * Copyright (C) 2003-2007 Jody Goldberg (jody@gnome.org)
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
#include <goffice/goffice-debug.h>

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>
#include <string.h>
#include <stdlib.h>

/**
 * GogObjectNamingConv:
 * @GOG_OBJECT_NAME_BY_ROLE: named built from role.
 * @GOG_OBJECT_NAME_BY_TYPE: named built from type.
 * @GOG_OBJECT_NAME_MANUALLY: custom name.
 **/

/**
 * GogManualSizeMode:
 * @GOG_MANUAL_SIZE_AUTO: auto size, can't be changed.
 * @GOG_MANUAL_SIZE_WIDTH: the width can be changed.
 * @GOG_MANUAL_SIZE_HEIGHT: the height can be changed.
 * @GOG_MANUAL_SIZE_FULL: both height and width can be changed.
 **/

/**
 * GogObjectClass:
 * @base: base class.
 * @roles: roles for the class.
 * @view_type: view type.
 * @use_parent_as_proxy: internal.
 * @roles_allocated: internal.
 * @update: updates the object.
 * @parent_changed: called when parent changed.
 * @type_name: gets the type public name.
 * @populate_editor: populates the editor.
 * @document_changed: the document changed.
 * @get_manual_size_mode: resize mode.
 * @changed: implements the "changed" signal.
 * @name_changed: implements the "name-changed" signal.
 * @possible_additions_changed: implements the "possible-additions-changed" signal.
 * @child_added: implements the "child-added" signal.
 * @child_removed: implements the "child-removed" signal.
 * @child_name_changed: implements the "child-name-changed" signal.
 * @children_reordered: implements the "children-reordered" signal.
 * @update_editor: implements the "update-editor" signal.
 **/

/**
 * GogObjectPosition:
 * @GOG_POSITION_AUTO: automatic.
 * @GOG_POSITION_N: north, might be combined with east or west.
 * @GOG_POSITION_S: south, might be combined with east or west.
 * @GOG_POSITION_E: east.
 * @GOG_POSITION_W: west.
 * @GOG_POSITION_COMPASS: mask of the four previous positions.
 * @GOG_POSITION_ALIGN_FILL: fills.
 * @GOG_POSITION_ALIGN_START: start.
 * @GOG_POSITION_ALIGN_END: end.
 * @GOG_POSITION_ALIGN_CENTER: centered.
 * @GOG_POSITION_ALIGNMENT: mask for start or end.
 * @GOG_POSITION_SPECIAL: special.
 * @GOG_POSITION_MANUAL: manual.
 * @GOG_POSITION_MANUAL_X_ABS: whether the x position is absolute or relative.
 * @GOG_POSITION_MANUAL_Y_ABS: whether the y position is absolute or relative.
 * @GOG_POSITION_MANUAL_X_END: x position relative to start or end.
 * @GOG_POSITION_MANUAL_Y_END: y position relative to start or end.
 * @GOG_POSITION_ANCHOR_NW: anchored north-west.
 * @GOG_POSITION_ANCHOR_N: anchored north.
 * @GOG_POSITION_ANCHOR_NE: anchored north-east.
 * @GOG_POSITION_ANCHOR_E: anchored east.
 * @GOG_POSITION_ANCHOR_SE: anchored south-east.
 * @GOG_POSITION_ANCHOR_S: anchored south.
 * @GOG_POSITION_ANCHOR_SW: anchored south-west.
 * @GOG_POSITION_ANCHOR_W: anchored west.
 * @GOG_POSITION_ANCHOR_CENTER: anchored at center.
 * @GOG_POSITION_ANCHOR: mask for anchors.
 * @GOG_POSITION_ANY_MANUAL: mask for all manual positions
 * @GOG_POSITION_PADDING: padding.
 * @GOG_POSITION_MANUAL_W: relative width.
 * @GOG_POSITION_MANUAL_W_ABS: absolute width.
 * @GOG_POSITION_MANUAL_H: relative height.
 * @GOG_POSITION_MANUAL_H_ABS: absolute height.
 * @GOG_POSITION_ANY_MANUAL_SIZE: mask for manual sizes.
 * @GOG_POSITION_HEXPAND: expands in the horizontal direction.
 * @GOG_POSITION_VEXPAND: expands in the vertical direction.
 * @GOG_POSITION_EXPAND: expands in either direction.
 **/

/**
 * GogObjectRole:
 * @id: id for persistence.
 * @is_a_typename: type name.
 * @priority: priority.
 * @allowable_positions: allowed positions inside parent.
 * @default_position: default position.
 * @naming_conv: naming convention.
 * @can_add: return %TRUE if a new child can be added.
 * @can_remove: return %TRUE if the child can be removed.
 * @allocate: optional allocator, g_object_new() is used if %NULL.
 * @post_add: called after adding the child.
 * @pre_remove: called before removing the child.
 * @post_remove: called after removing the child.
 *
 * Describes allowable children for a #GogObject.
 **/

static GogObjectRole*
gog_object_role_ref (GogObjectRole* role)
{
	return role;
}

static void
gog_object_role_unref (G_GNUC_UNUSED GogObjectRole *role)
{
}

GType
gog_object_role_get_type (void)
{
	static GType t = 0;

	if (t == 0)
		t = g_boxed_type_register_static ("GogObjectRole",
			 (GBoxedCopyFunc) gog_object_role_ref,
			 (GBoxedFreeFunc) gog_object_role_unref);
	return t;
}

/**
 * SECTION: gog-object
 * @short_description: The base class for graph objects.
 * @See_also: #GogGraph
 *
 * Abstract base class that objects in the graph hierarchy are based on.
 * This class handles manipulation of the object hierarchy, and positioning of
 * objects in the graph.
 *
 * Every object has a name that is unique in the graph. It can have a parent
 * and a list of children in specific roles (see #GogObjectRole).
 * There can generally be several children in each role.
 *
 * If built with GTK+ support, each object also knows how to populate a widget
 * that allows one to manipulate the attributes of that object. This can be used
 * by #GOEditor to present a widget that allows manipulation of the whole graph.
 */

typedef struct {
	char const *label;
	char const *value;
	unsigned const flags;
} GogPositionFlagDesc;

static GogPositionFlagDesc const position_compass[] = {
	{N_("Top"), 		"top",		GOG_POSITION_N},
	{N_("Top right"), 	"top-right",	GOG_POSITION_N|GOG_POSITION_E},
	{N_("Right"), 		"right",	GOG_POSITION_E},
	{N_("Bottom right"), 	"bottom-right",	GOG_POSITION_E|GOG_POSITION_S},
	{N_("Bottom"),		"bottom",	GOG_POSITION_S},
	{N_("Bottom left"),	"bottom-left",	GOG_POSITION_S|GOG_POSITION_W},
	{N_("Left"),		"left",		GOG_POSITION_W},
	{N_("Top left"),	"top-left",	GOG_POSITION_W|GOG_POSITION_N}
};

static GogPositionFlagDesc const position_alignment[] = {
	{N_("Fill"), 	"fill",		GOG_POSITION_ALIGN_FILL},
	{N_("Start"), 	"start",	GOG_POSITION_ALIGN_START},
	{N_("End"), 	"end",		GOG_POSITION_ALIGN_END},
	{N_("Center"), 	"center",	GOG_POSITION_ALIGN_CENTER}
};

static GogPositionFlagDesc const position_anchor[] = {
	{N_("Top left"), 	"top-left",	GOG_POSITION_ANCHOR_NW},
	{N_("Top"), 		"top",		GOG_POSITION_ANCHOR_N},
	{N_("Top right"), 	"top-right",	GOG_POSITION_ANCHOR_NE},
	{N_("Left"), 		"left",		GOG_POSITION_ANCHOR_W},
	{N_("Center"), 		"center",	GOG_POSITION_ANCHOR_CENTER},
	{N_("Right"), 		"right",	GOG_POSITION_ANCHOR_E},
	{N_("Bottom left"), 	"bottom-left",	GOG_POSITION_ANCHOR_SW},
	{N_("Bottom"), 		"bottom",	GOG_POSITION_ANCHOR_S},
	{N_("Bottom right"),	"bottom-right",	GOG_POSITION_ANCHOR_SE}
};

static GogPositionFlagDesc const manual_size[] = {
	{N_("None"),			"none",		  GOG_POSITION_AUTO},
	{N_("Width"), 			"width",	  GOG_POSITION_MANUAL_W},
	{N_("Absolute width"), 	"abs-width",  GOG_POSITION_MANUAL_W_ABS},
	{N_("Height"), 			"height",	  GOG_POSITION_MANUAL_H},
	{N_("Absolute height"), "abs-height", GOG_POSITION_MANUAL_H_ABS},
	{N_("Size"), 			"size",		  GOG_POSITION_MANUAL_W | GOG_POSITION_MANUAL_H},
	{N_("Absolute size"),	"abs-size",	  GOG_POSITION_MANUAL_W_ABS | GOG_POSITION_MANUAL_H_ABS}
};

enum {
	OBJECT_PROP_0,
	OBJECT_PROP_ID,
	OBJECT_PROP_POSITION,
	OBJECT_PROP_POSITION_COMPASS,
	OBJECT_PROP_POSITION_ALIGNMENT,
	OBJECT_PROP_POSITION_IS_MANUAL,
	OBJECT_PROP_POSITION_ANCHOR,
	OBJECT_PROP_INVISIBLE,
	OBJECT_PROP_MANUAL_SIZE_MODE
};

enum {
	CHILD_ADDED,
	CHILD_REMOVED,
	CHILD_NAME_CHANGED,
	CHILDREN_REORDERED,
	NAME_CHANGED,
	CHANGED,
	UPDATE_EDITOR,
	LAST_SIGNAL
};
static gulong gog_object_signals [LAST_SIGNAL] = { 0, };

static GObjectClass *parent_klass;

static void gog_object_set_id (GogObject *obj, unsigned id);

static void
gog_object_parent_finalized (GogObject *obj)
{
	obj->parent = NULL;
	g_object_unref (obj);
}

static void
gog_object_finalize (GObject *gobj)
{
	GogObject *obj = GOG_OBJECT (gobj);

	g_free (obj->user_name); obj->user_name = NULL;
	g_free (obj->auto_name); obj->auto_name = NULL;

	g_slist_foreach (obj->children, (GFunc) gog_object_parent_finalized, NULL);
	g_slist_free (obj->children);
	obj->children = NULL;

	(parent_klass->finalize) (gobj);
}

static void
gog_object_parent_changed (GogObject *child, gboolean was_set)
{
	GSList *ptr = child->children;
	for (; ptr != NULL ; ptr = ptr->next) {
		GogObjectClass *klass = GOG_OBJECT_GET_CLASS (ptr->data);
		(*klass->parent_changed) (ptr->data, was_set);
	}

	if (GOG_IS_DATASET (child))
		gog_dataset_parent_changed (GOG_DATASET (child), was_set);
}

static void
gog_object_set_property (GObject *obj, guint param_id,
			 GValue const *value, GParamSpec *pspec)
{
	GogObject *gobj = GOG_OBJECT (obj);
	char const *str;
	char **str_doubles;
	unsigned id;

	switch (param_id) {
	case OBJECT_PROP_ID:
		id = g_value_get_uint (value);
		gog_object_set_id (gobj, id);
		break;
	case OBJECT_PROP_POSITION:
		str = g_value_get_string (value);
		str_doubles = g_strsplit (str, " ", 4);
		if (g_strv_length (str_doubles) != 4) {
			g_strfreev (str_doubles);
			break;
		}
		gobj->manual_position.x = g_ascii_strtod (str_doubles[0], NULL);
		gobj->manual_position.y = g_ascii_strtod (str_doubles[1], NULL);
		gobj->manual_position.w = g_ascii_strtod (str_doubles[2], NULL);
		gobj->manual_position.h = g_ascii_strtod (str_doubles[3], NULL);
		g_strfreev (str_doubles);
		break;
	case OBJECT_PROP_POSITION_COMPASS:
		str = g_value_get_string (value);
		if (str == NULL)
			break;
		for (id = 0; id < G_N_ELEMENTS (position_compass); id++)
			if (strcmp (str, position_compass[id].value) == 0)
				break;
		if (id < G_N_ELEMENTS (position_compass))
			gog_object_set_position_flags (gobj,
						       position_compass[id].flags,
						       GOG_POSITION_COMPASS);
		break;
	case OBJECT_PROP_POSITION_ALIGNMENT:
		str = g_value_get_string (value);
		if (str == NULL)
			break;
		for (id = 0; id < G_N_ELEMENTS (position_alignment); id++)
			if (strcmp (str, position_alignment[id].value) == 0)
				break;
		if (id < G_N_ELEMENTS (position_alignment))
			gog_object_set_position_flags (gobj,
						       position_alignment[id].flags,
						       GOG_POSITION_ALIGNMENT);
		break;
	case OBJECT_PROP_POSITION_IS_MANUAL:
		gog_object_set_position_flags (gobj,
			g_value_get_boolean (value) ? GOG_POSITION_MANUAL : 0,
			GOG_POSITION_MANUAL);
		break;
	case OBJECT_PROP_POSITION_ANCHOR:
		str = g_value_get_string (value);
		if (str == NULL)
			break;
		for (id = 0; id < G_N_ELEMENTS (position_anchor); id++)
			if (strcmp (str, position_anchor[id].value) == 0)
				break;
		if (id < G_N_ELEMENTS (position_anchor))
			gog_object_set_position_flags (gobj,
						       position_anchor[id].flags,
						       GOG_POSITION_ANCHOR);
		break;
	case OBJECT_PROP_INVISIBLE :
		gog_object_set_invisible (gobj, g_value_get_boolean (value));
		break;
	case OBJECT_PROP_MANUAL_SIZE_MODE:
		str = g_value_get_string (value);
		if (str == NULL)
			break;
		for (id = 0; id < G_N_ELEMENTS (manual_size); id++)
			if (strcmp (str, manual_size[id].value) == 0)
				break;
		if (id < G_N_ELEMENTS (manual_size))
			gog_object_set_position_flags (gobj,
						       manual_size[id].flags,
						       GOG_POSITION_ANY_MANUAL_SIZE);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
}

static void
gog_object_get_property (GObject *obj, guint param_id,
		       GValue *value, GParamSpec *pspec)
{
	GogObject *gobj = GOG_OBJECT (obj);
	GogObjectPosition flags;
	GString *string;
	char buffer[G_ASCII_DTOSTR_BUF_SIZE];
	unsigned i;

	switch (param_id) {
	case OBJECT_PROP_ID:
		g_value_set_uint (value, GOG_OBJECT (obj)->id);
		break;
	case OBJECT_PROP_POSITION:
		string = g_string_new ("");
		g_string_append (string, g_ascii_dtostr (buffer, sizeof (buffer), gobj->manual_position.x));
		g_string_append_c (string, ' ');
		g_string_append (string, g_ascii_dtostr (buffer, sizeof (buffer), gobj->manual_position.y));
		g_string_append_c (string, ' ');
		g_string_append (string, g_ascii_dtostr (buffer, sizeof (buffer), gobj->manual_position.w));
		g_string_append_c (string, ' ');
		g_string_append (string, g_ascii_dtostr (buffer, sizeof (buffer), gobj->manual_position.h));
		g_value_set_string (value, string->str);
		g_string_free (string, TRUE);
		break;
	case OBJECT_PROP_POSITION_COMPASS:
		flags = gog_object_get_position_flags (GOG_OBJECT (obj), GOG_POSITION_COMPASS);
		for (i = 0; i < G_N_ELEMENTS (position_compass); i++)
			if (position_compass[i].flags == flags) {
				g_value_set_string (value, position_compass[i].value);
				break;
			}
		break;
	case OBJECT_PROP_POSITION_ALIGNMENT:
		flags = gog_object_get_position_flags (GOG_OBJECT (obj), GOG_POSITION_ALIGNMENT);
		for (i = 0; i < G_N_ELEMENTS (position_alignment); i++)
			if (position_alignment[i].flags == flags) {
				g_value_set_string (value, position_alignment[i].value);
				break;
			}
		break;
	case OBJECT_PROP_POSITION_IS_MANUAL:
		g_value_set_boolean (value, (gobj->position & GOG_POSITION_MANUAL) != 0);
		break;
	case OBJECT_PROP_POSITION_ANCHOR:
		flags = gog_object_get_position_flags (GOG_OBJECT (obj), GOG_POSITION_ANCHOR);
		for (i = 0; i < G_N_ELEMENTS (position_anchor); i++)
			if (position_anchor[i].flags == flags) {
				g_value_set_string (value, position_anchor[i].value);
				break;
			}
		break;
	case OBJECT_PROP_INVISIBLE :
		g_value_set_boolean (value, gobj->invisible != 0);
		break;
	case OBJECT_PROP_MANUAL_SIZE_MODE:
		flags = gog_object_get_position_flags (GOG_OBJECT (obj), GOG_POSITION_ANY_MANUAL_SIZE);
		for (i = 0; i < G_N_ELEMENTS (manual_size); i++)
			if (manual_size[i].flags == flags) {
				g_value_set_string (value, manual_size[i].value);
				break;
			}
		if (i == G_N_ELEMENTS (manual_size))
			g_value_set_string (value, "none");
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

#ifdef GOFFICE_WITH_GTK
typedef struct {
	GtkWidget	*x_spin, *y_spin, *w_spin, *h_spin;
	GtkWidget	*position_select_combo;
	GtkWidget	*position_notebook;
	GogObject	*gobj;
	GtkBuilder	*gui;
	gulong		 update_editor_handler;
	gulong		 h_sig, w_sig;
} ObjectPrefState;

static void
object_pref_state_free (ObjectPrefState *state)
{
	g_signal_handler_disconnect (state->gobj, state->update_editor_handler);
	g_object_unref (state->gobj);
	g_object_unref (state->gui);
	g_free (state);
}

static void
cb_compass_changed (GtkComboBox *combo, ObjectPrefState *state)
{
	GogObjectPosition position = position_compass[gtk_combo_box_get_active (combo)].flags;

	gog_object_set_position_flags (state->gobj, position, GOG_POSITION_COMPASS);
}

static void
cb_alignment_changed (GtkComboBox *combo, ObjectPrefState *state)
{
	GogObjectPosition position = position_alignment[gtk_combo_box_get_active (combo)].flags;

	gog_object_set_position_flags (state->gobj, position, GOG_POSITION_ALIGNMENT);
}

static void
cb_position_changed (GtkWidget *spin, ObjectPrefState *state)
{
	GogViewAllocation pos;
	double value = gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin)) / 100.0;

    gog_object_get_manual_position (state->gobj, &pos);
	if (spin == state->x_spin)
		pos.x = value;
	else if (spin == state->y_spin)
		pos.y = value;
	else if (spin == state->w_spin)
		pos.w = value;
	else if (spin == state->h_spin)
		pos.h = value;
	gog_object_set_manual_position (state->gobj, &pos);
}

static void
cb_size_changed (GtkWidget *spin, ObjectPrefState *state)
{
	GogViewAllocation pos;
	double value = gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin)) / 100.0;

       	gog_object_get_manual_position (state->gobj, &pos);
	if (spin == state->w_spin)
		pos.w = value;
	else if (spin == state->h_spin)
		pos.h = value;
	gog_object_set_manual_position (state->gobj, &pos);
}

static void
update_select_state (ObjectPrefState *state)
{
	if (state->position_select_combo) {
		int index = gog_object_get_position_flags (state->gobj, GOG_POSITION_MANUAL) == 0 ? 0 : 1;

		gtk_combo_box_set_active (GTK_COMBO_BOX (state->position_select_combo), index);
		if (index == 0 && GOG_IS_CHART (state->gobj))
			index = 2;
		gtk_notebook_set_current_page (GTK_NOTEBOOK (state->position_notebook), index);
	}
}

static void
cb_manual_position_changed (GtkComboBox *combo, ObjectPrefState *state)
{
	int index = gtk_combo_box_get_active (combo);

	gog_object_set_position_flags (state->gobj,
		index != 0 ? GOG_POSITION_MANUAL : 0,
		GOG_POSITION_MANUAL);
	if (index == 0 && GOG_IS_CHART (state->gobj))
		index = 2;
	gtk_notebook_set_current_page (GTK_NOTEBOOK (state->position_notebook), index);
}

static void
cb_anchor_changed (GtkComboBox *combo, ObjectPrefState *state)
{
	GogObjectPosition position = position_anchor[gtk_combo_box_get_active (combo)].flags;

	gog_object_set_position_flags (state->gobj, position, GOG_POSITION_ANCHOR);
}

static void
cb_update_editor (GogObject *gobj, ObjectPrefState *state)
{
	GogObjectPosition manual_size = gog_object_get_position_flags (gobj, GOG_POSITION_ANY_MANUAL_SIZE);
	if (state->x_spin != NULL)
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (state->x_spin), gobj->manual_position.x * 100.0);
	if (state->y_spin != NULL)
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (state->y_spin), gobj->manual_position.y * 100.0);
	if (state->w_spin != NULL) {
		gboolean visible = (manual_size & GOG_POSITION_MANUAL_W) != 0;
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (state->w_spin), gobj->manual_position.w * 100.0);
		gtk_widget_set_visible (go_gtk_builder_get_widget (state->gui, "width_label"), visible);
		gtk_widget_set_visible (go_gtk_builder_get_widget (state->gui, "width_spin"), visible);
		gtk_widget_set_visible (go_gtk_builder_get_widget (state->gui, "width-pc-lbl"), visible);
	}
	if (state->h_spin != NULL) {
		gboolean visible = (manual_size & GOG_POSITION_MANUAL_H) != 0;
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (state->h_spin), gobj->manual_position.h * 100.0);
		gtk_widget_set_visible (go_gtk_builder_get_widget (state->gui, "height_label"), visible);
		gtk_widget_set_visible (go_gtk_builder_get_widget (state->gui, "height_spin"), visible);
		gtk_widget_set_visible (go_gtk_builder_get_widget (state->gui, "height-pc-lbl"), visible);
	}

	update_select_state (state);
}

static void
cb_chart_position_changed (GtkWidget *spin, ObjectPrefState *state)
{
	g_object_set (G_OBJECT (state->gobj), gtk_buildable_get_name (GTK_BUILDABLE (spin)),
		      (int) gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin)), NULL);
}

static void
cb_manual_size_changed (GtkComboBox *combo, ObjectPrefState *state)
{
	int index = gtk_combo_box_get_active (combo);
	GogObjectPosition pos = GOG_POSITION_AUTO;
	gboolean visible;
	if (index > 0)
		switch (gog_object_get_manual_size_mode (state->gobj)) {
		case GOG_MANUAL_SIZE_AUTO:
			break;
		case GOG_MANUAL_SIZE_WIDTH:
			pos = GOG_POSITION_MANUAL_W;
			break;
		case GOG_MANUAL_SIZE_HEIGHT:
			pos = GOG_POSITION_MANUAL_H;
			break;
		case GOG_MANUAL_SIZE_FULL:
			pos = GOG_POSITION_MANUAL_W | GOG_POSITION_MANUAL_H;
			break;
		}
	gog_object_set_position_flags (state->gobj, pos, GOG_POSITION_ANY_MANUAL_SIZE);
	visible = (pos & GOG_POSITION_MANUAL_W) != 0;
	gtk_widget_set_visible (go_gtk_builder_get_widget (state->gui, "width_label"), visible);
	gtk_widget_set_visible (go_gtk_builder_get_widget (state->gui, "width_spin"), visible);
	gtk_widget_set_visible (go_gtk_builder_get_widget (state->gui, "width-pc-lbl"), visible);
	visible = (pos & GOG_POSITION_MANUAL_H) != 0;
	gtk_widget_set_visible (go_gtk_builder_get_widget (state->gui, "height_label"), visible);
	gtk_widget_set_visible (go_gtk_builder_get_widget (state->gui, "height_spin"), visible);
	gtk_widget_set_visible (go_gtk_builder_get_widget (state->gui, "height-pc-lbl"), visible);
}

static void
gog_object_populate_editor (GogObject *gobj,
			    GOEditor *editor,
			    G_GNUC_UNUSED GogDataAllocator *dalloc,
			    GOCmdContext *cc)
{
	GtkWidget *w;
	GtkSizeGroup *widget_size_group, *label_size_group;
	GtkBuilder *gui;
	GogObjectPosition allowable_positions, flags;
	ObjectPrefState *state;
	unsigned i;

	if (gobj->role == NULL)
		return;

       	allowable_positions = gobj->role->allowable_positions;
	if (!(allowable_positions & (GOG_POSITION_MANUAL | GOG_POSITION_COMPASS)))
		return;

	gui = go_gtk_builder_load_internal ("res:go:graph/gog-object-prefs.ui", GETTEXT_PACKAGE, cc);
	if (gui == NULL)
		return;

	state = g_new (ObjectPrefState, 1);
	state->gobj = gobj;
	state->gui = gui;
	state->position_select_combo = NULL;
	state->x_spin = NULL;
	state->y_spin = NULL;
	state->w_spin = NULL;
	state->h_spin = NULL;
	state->position_notebook = go_gtk_builder_get_widget (gui, "position_notebook");

	g_object_ref (gobj);

	widget_size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	label_size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

	if (allowable_positions & GOG_POSITION_COMPASS) {
		w = GTK_WIDGET (go_gtk_builder_combo_box_init_text (gui, "position_combo"));
		gtk_size_group_add_widget (widget_size_group, w);
		flags = gog_object_get_position_flags (gobj, GOG_POSITION_COMPASS);
		for (i = 0; i < G_N_ELEMENTS (position_compass); i++) {
			go_gtk_combo_box_append_text (GTK_COMBO_BOX (w), _(position_compass[i].label));
			if (position_compass[i].flags == flags)
				gtk_combo_box_set_active (GTK_COMBO_BOX (w), i);
		}
		g_signal_connect (G_OBJECT (w), "changed", G_CALLBACK (cb_compass_changed), state);
		w = go_gtk_builder_get_widget (gui, "position_label");
		gtk_size_group_add_widget (label_size_group, w);
	} else {
		w = go_gtk_builder_get_widget (gui, "compass_position");
		gtk_widget_hide (w);
	}

	if (allowable_positions & GOG_POSITION_COMPASS) {
		w = GTK_WIDGET (go_gtk_builder_combo_box_init_text (gui, "alignment_combo"));
		gtk_size_group_add_widget (widget_size_group, w);
		flags = gog_object_get_position_flags (gobj, GOG_POSITION_ALIGNMENT);
		for (i = 0; i < G_N_ELEMENTS (position_alignment); i++) {
			go_gtk_combo_box_append_text (GTK_COMBO_BOX (w), _(position_alignment[i].label));
			if (position_alignment[i].flags == flags)
				gtk_combo_box_set_active (GTK_COMBO_BOX (w), i);
		}
		g_signal_connect (G_OBJECT (w), "changed", G_CALLBACK (cb_alignment_changed), state);
		w = go_gtk_builder_get_widget (gui, "alignment_label");
		gtk_size_group_add_widget (label_size_group, w);
	} else {
		w = go_gtk_builder_get_widget (gui, "compass_alignment");
		gtk_widget_hide (w);
	}

	if (!(allowable_positions & GOG_POSITION_COMPASS))
		gtk_notebook_set_current_page (GTK_NOTEBOOK (state->position_notebook), 1);

	g_object_unref (widget_size_group);
	g_object_unref (label_size_group);

	widget_size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	label_size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

	if (allowable_positions & GOG_POSITION_MANUAL) {
		w = go_gtk_builder_get_widget (gui, "x_label");
		gtk_size_group_add_widget (label_size_group, w);
		w = go_gtk_builder_get_widget (gui, "x_spin");
		gtk_size_group_add_widget (widget_size_group, w);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), gobj->manual_position.x * 100.0);
		g_signal_connect (G_OBJECT (w), "value-changed", G_CALLBACK (cb_position_changed), state);
		state->x_spin = w;

		w = go_gtk_builder_get_widget (gui, "y_label");
		gtk_size_group_add_widget (label_size_group, w);
		w = go_gtk_builder_get_widget (gui, "y_spin");
		gtk_size_group_add_widget (widget_size_group, w);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), gobj->manual_position.y * 100.0);
		g_signal_connect (G_OBJECT (w), "value-changed", G_CALLBACK (cb_position_changed), state);
		state->y_spin = w;

		w = go_gtk_builder_get_widget (gui, "anchor_label");
		gtk_size_group_add_widget (label_size_group, w);
		w =  GTK_WIDGET (go_gtk_builder_combo_box_init_text (gui, "anchor_combo"));
		flags = gog_object_get_position_flags (gobj, GOG_POSITION_ANCHOR);
		for (i = 0; i < G_N_ELEMENTS (position_anchor); i++) {
			go_gtk_combo_box_append_text (GTK_COMBO_BOX (w), _(position_anchor[i].label));
			if (i == 0 || position_anchor[i].flags == flags)
				gtk_combo_box_set_active (GTK_COMBO_BOX (w), i);
		}
		g_signal_connect (G_OBJECT (w), "changed", G_CALLBACK (cb_anchor_changed), state);
		gtk_combo_box_set_wrap_width (GTK_COMBO_BOX (w), 3);

	}

	if (gog_object_get_manual_size_mode (gobj) == GOG_MANUAL_SIZE_AUTO)  {
		w = go_gtk_builder_get_widget (gui, "manual-sizes");
		gtk_widget_destroy (w);
		w = go_gtk_builder_get_widget (gui, "size-select-box");
		gtk_widget_destroy (w);
	} else {
		gboolean manual_size = (flags = gog_object_get_position_flags (gobj, GOG_POSITION_ANY_MANUAL_SIZE)) != 0;
		w = go_gtk_builder_get_widget (gui, "object-size-combo");
		gtk_combo_box_set_active (GTK_COMBO_BOX (w),
		                          manual_size? 1: 0);
		g_signal_connect (G_OBJECT (w),
				  "changed", G_CALLBACK (cb_manual_size_changed), state);
		w = go_gtk_builder_get_widget (gui, "width_label");
		gtk_size_group_add_widget (label_size_group, w);
		w = go_gtk_builder_get_widget (gui, "width_spin");
		gtk_size_group_add_widget (widget_size_group, w);
		g_signal_connect (G_OBJECT (w), "value-changed",
		                  G_CALLBACK (cb_size_changed), state);
		state->w_spin = w;

		w = go_gtk_builder_get_widget (gui, "height_label");
		gtk_size_group_add_widget (label_size_group, w);
		w = go_gtk_builder_get_widget (gui, "height_spin");
		gtk_size_group_add_widget (widget_size_group, w);
		g_signal_connect (G_OBJECT (w), "value-changed",
		                  G_CALLBACK (cb_size_changed), state);
		state->h_spin = w;
		cb_update_editor (gobj, state);
	}

	if (GOG_IS_CHART (gobj)) {
		/* setting special notebook page */
		int col, row, cols, rows;
		g_object_get (G_OBJECT (gobj), "xpos", &col, "ypos", &row, "columns", &cols, "rows", &rows, NULL);
		w = go_gtk_builder_get_widget (gui, "xpos");
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), col);
		g_signal_connect (G_OBJECT (w), "value-changed",
				  G_CALLBACK (cb_chart_position_changed), state);
		w = go_gtk_builder_get_widget (gui, "columns");
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), cols);
		g_signal_connect (G_OBJECT (w), "value-changed",
				  G_CALLBACK (cb_chart_position_changed), state);
		w = go_gtk_builder_get_widget (gui, "ypos");
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), row);
		g_signal_connect (G_OBJECT (w), "value-changed",
				  G_CALLBACK (cb_chart_position_changed), state);
		w = go_gtk_builder_get_widget (gui, "rows");
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), rows);
		g_signal_connect (G_OBJECT (w), "value-changed",
				  G_CALLBACK (cb_chart_position_changed), state);
	}

	g_object_unref (widget_size_group);
	g_object_unref (label_size_group);

	if ((allowable_positions & GOG_POSITION_MANUAL) &&
	    ((allowable_positions & (GOG_POSITION_COMPASS | GOG_POSITION_ALIGNMENT)) ||
	     (allowable_positions & GOG_POSITION_SPECIAL))) {
		state->position_select_combo = go_gtk_builder_get_widget (gui, "position_select_combo");

		update_select_state (state);

		g_signal_connect (G_OBJECT (state->position_select_combo),
				  "changed", G_CALLBACK (cb_manual_position_changed), state);
	} else {
		w = go_gtk_builder_get_widget (gui, "position_select_box");
		gtk_widget_hide (w);
	}

	state->update_editor_handler = g_signal_connect (G_OBJECT (gobj),
							 "update-editor",
							 G_CALLBACK (cb_update_editor), state);

	w = go_gtk_builder_get_widget (gui, "gog_object_prefs");
	g_signal_connect_swapped (G_OBJECT (w), "destroy", G_CALLBACK (object_pref_state_free), state);
	go_editor_add_page (editor, w, _("Position"));
}
#endif

static void
gog_object_base_init (GogObjectClass *klass)
{
	klass->roles_allocated = FALSE;
	/* klass->roles might be non-NULL; in that case, it points to
	   the roles hash of the superclass. */
}

static void
gog_object_base_finalize (GogObjectClass *klass)
{
	if (klass->roles_allocated)
		g_hash_table_destroy (klass->roles);
}

static void
gog_object_class_init (GObjectClass *klass)
{
	GogObjectClass *gog_klass = (GogObjectClass *)klass;
	parent_klass = g_type_class_peek_parent (klass);

	klass->finalize = gog_object_finalize;
	klass->set_property	= gog_object_set_property;
	klass->get_property	= gog_object_get_property;

	gog_klass->parent_changed  = gog_object_parent_changed;
#ifdef GOFFICE_WITH_GTK
	gog_klass->populate_editor = gog_object_populate_editor;
#endif

	gog_klass->use_parent_as_proxy = FALSE;

	g_object_class_install_property (klass, OBJECT_PROP_ID,
		g_param_spec_uint ("id",
			_("Object ID"),
			_("Object numerical ID"),
			0, G_MAXINT, 0,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (klass, OBJECT_PROP_POSITION,
		g_param_spec_string ("position",
			_("Position"),
			_("Position and size of object, in percentage of parent size"),
			"0 0 1 1",
			GSF_PARAM_STATIC | G_PARAM_READWRITE|GO_PARAM_PERSISTENT));
	g_object_class_install_property (klass, OBJECT_PROP_POSITION_COMPASS,
		g_param_spec_string ("compass",
			_("Compass"),
			_("Compass auto position flags"),
			"top",
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT | GOG_PARAM_POSITION));
	g_object_class_install_property (klass, OBJECT_PROP_POSITION_ALIGNMENT,
		g_param_spec_string ("alignment",
			_("Alignment"),
			_("Alignment flag"),
			"fill",
		       	GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT | GOG_PARAM_POSITION));
	g_object_class_install_property (klass, OBJECT_PROP_POSITION_IS_MANUAL,
		g_param_spec_boolean ("is-position-manual",
			_("Is position manual"),
			_("Is position manual"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (klass, OBJECT_PROP_POSITION_ANCHOR,
		g_param_spec_string ("anchor",
			_("Anchor"),
			_("Anchor for manual position"),
			"top-left",
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT | GOG_PARAM_POSITION));
	g_object_class_install_property (klass, OBJECT_PROP_INVISIBLE,
		g_param_spec_boolean ("invisible",
			_("Should the object be hidden"),
			_("Should the object be hidden"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (klass, OBJECT_PROP_MANUAL_SIZE_MODE,
		g_param_spec_string ("manual-size",
			_("Manual size"),
			_("Whether the height or width are manually set"),
			"none",
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	/**
	 * GogObject::child-added:
	 * @object: the object on which the signal is emitted
	 * @child: The new #GogObject whose parent is @object
	 *
	 * The ::child-added signal is emitted AFTER the child has been added
	 * and AFTER the parent-changed signal has been called for it.
	 **/
	gog_object_signals [CHILD_ADDED] = g_signal_new ("child-added",
		G_TYPE_FROM_CLASS (klass),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GogObjectClass, child_added),
		NULL, NULL,
		g_cclosure_marshal_VOID__OBJECT,
		G_TYPE_NONE,	1, G_TYPE_OBJECT);
	/**
	 * GogObject::child-removed:
	 * @object: the object on which the signal is emitted
	 * @child: The new #GogObject whose parent is @object
	 *
	 * The ::child-removed signal is emitted BEFORE the child has been
	 * added and BEFORE the parent-changed signal has been called for it.
	 **/
	gog_object_signals [CHILD_REMOVED] = g_signal_new ("child-removed",
		G_TYPE_FROM_CLASS (klass),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GogObjectClass, child_removed),
		NULL, NULL,
		g_cclosure_marshal_VOID__OBJECT,
		G_TYPE_NONE,	1, G_TYPE_OBJECT);
	gog_object_signals [CHILD_NAME_CHANGED] = g_signal_new ("child-name-changed",
		G_TYPE_FROM_CLASS (gog_klass),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GogObjectClass, child_name_changed),
		NULL, NULL,
		g_cclosure_marshal_VOID__OBJECT,
		G_TYPE_NONE,	1, G_TYPE_OBJECT);
	gog_object_signals [CHILDREN_REORDERED] = g_signal_new ("children-reordered",
		G_TYPE_FROM_CLASS (klass),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GogObjectClass, children_reordered),
		NULL, NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
	gog_object_signals [NAME_CHANGED] = g_signal_new ("name-changed",
		G_TYPE_FROM_CLASS (klass),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GogObjectClass, name_changed),
		NULL, NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
	gog_object_signals [CHANGED] = g_signal_new ("changed",
		G_TYPE_FROM_CLASS (klass),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GogObjectClass, changed),
		NULL, NULL,
		g_cclosure_marshal_VOID__BOOLEAN,
		G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
	gog_object_signals [UPDATE_EDITOR] = g_signal_new ("update-editor",
		G_TYPE_FROM_CLASS (klass),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GogObjectClass, update_editor),
		NULL, NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
}

static void
gog_object_init (GogObject *obj)
{
	obj->children = NULL;
	obj->user_name = NULL;
	obj->auto_name = NULL;
	obj->id = 0;
	obj->needs_update = FALSE;
	obj->being_updated = FALSE;
	obj->explicitly_typed_role = FALSE;
	obj->invisible = FALSE;
	obj->manual_position.x =
	obj->manual_position.y = 0.0;
	obj->manual_position.w =
	obj->manual_position.h = 1.0;
}

GSF_CLASS_FULL (GogObject, gog_object,
		gog_object_base_init, gog_object_base_finalize,
		gog_object_class_init, NULL, gog_object_init,
		G_TYPE_OBJECT, G_TYPE_FLAG_ABSTRACT, {})

static gboolean
gog_object_is_same_type (GogObject *obj_a, GogObject *obj_b)
{
	g_return_val_if_fail (obj_a->role != NULL, FALSE);
	g_return_val_if_fail (obj_b->role != NULL, FALSE);

	if (obj_a->role->naming_conv != obj_b->role->naming_conv)
		return FALSE;

	if (obj_a->role->naming_conv == GOG_OBJECT_NAME_BY_ROLE)
		return (obj_a->role == obj_b->role);

	return (G_OBJECT_TYPE (obj_a) == G_OBJECT_TYPE (obj_b));
}

static void
gog_object_generate_name (GogObject *obj)
{
	GogObjectClass *klass;

	char const *type_name;

	g_return_if_fail (GOG_IS_OBJECT (obj));

	klass = GOG_OBJECT_GET_CLASS (obj);
	g_return_if_fail (obj->role != NULL);

	switch (obj->role->naming_conv) {
	default :
	case GOG_OBJECT_NAME_MANUALLY :
		g_warning ("Role %s should not be autogenerating names",
			   obj->role->id);

	case GOG_OBJECT_NAME_BY_ROLE :
		g_return_if_fail (obj->role != NULL);
		type_name = _(obj->role->id);
		break;

	case GOG_OBJECT_NAME_BY_TYPE :
		g_return_if_fail (klass->type_name != NULL);
		type_name = _((*klass->type_name) (obj));
		break;
	}

	if (type_name == NULL)
		type_name =  "BROKEN";

	g_free (obj->auto_name);
	obj->auto_name =  g_strdup_printf ("%s%d", type_name, obj->id);
}

unsigned
gog_object_get_id (GogObject const *obj)
{
	g_return_val_if_fail (GOG_IS_OBJECT (obj), 0);
	g_return_val_if_fail (obj != NULL, 0);

	return obj->id;
}

static void
gog_object_generate_id (GogObject *obj)
{
	GSList *ptr;
	unsigned id_max = 0;
	GogObject *child;

	obj->id = 0;

	if (obj->parent == NULL)
		return;

	for (ptr = obj->parent->children; ptr != NULL ; ptr = ptr->next) {
		child = GOG_OBJECT (ptr->data);
		if (gog_object_is_same_type (obj, child))
		    id_max = MAX (child->id, id_max);
	}
	obj->id = id_max + 1;

	gog_object_generate_name (obj);
}

static void
gog_object_set_id (GogObject *obj, unsigned id)
{
	gboolean found = FALSE;
	GSList *ptr;
	GogObject *child;

	g_return_if_fail (GOG_IS_OBJECT (obj));

	if (id == 0) {
		gog_object_generate_id (obj);
		return;
	}

	g_return_if_fail (GOG_OBJECT (obj)->parent != NULL);

	for (ptr = obj->parent->children; ptr != NULL && !found; ptr = ptr->next) {
		child = GOG_OBJECT (ptr->data);
		found = child->id == id &&
			gog_object_is_same_type (obj, child) &&
			ptr->data != obj;
		}

	if (found) {
		g_warning ("id %u already exists", id);
		gog_object_generate_id (obj);
		return;
	}

	if (id == obj->id)
		return;

	obj->id = id;
	gog_object_generate_name (obj);
}

static void
dataset_dup (GogDataset const *src, GogDataset *dst)
{
	gint	     n, last;
	gog_dataset_dims (src, &n, &last);
	for ( ; n <= last ; n++)
		gog_dataset_set_dim (dst, n,
			go_data_dup (gog_dataset_get_dim (src, n)),
			NULL);
}

/**
 * gog_object_dup:
 * @src: #GogObject
 * @new_parent: #GogObject the parent tree for the object (can be %NULL)
 * @datadup: (scope call): a function to duplicate the data (a default one is used if %NULL)
 *
 * Create a deep copy of @obj using @new_parent as its parent.
 *
 * Returns: (transfer full): the duplicated object
 **/
GogObject *
gog_object_dup (GogObject const *src, GogObject *new_parent, GogDataDuplicator datadup)
{
	guint	     n;
	GParamSpec **props;
	GogObject   *dst = NULL;
	GSList      *ptr;
	GValue	     val = { 0 };

	if (src == NULL)
		return NULL;

	g_return_val_if_fail (GOG_OBJECT (src) != NULL, NULL);

	if (src->role == NULL || src->explicitly_typed_role)
		dst = g_object_new (G_OBJECT_TYPE (src), NULL);
	if (new_parent)
		dst = gog_object_add_by_role (new_parent, src->role, dst);

	g_return_val_if_fail (GOG_OBJECT (dst) != NULL, NULL);

	dst->position = src->position;
	/* properties */
	props = g_object_class_list_properties (G_OBJECT_GET_CLASS (src), &n);
	while (n-- > 0)
		if (props[n]->flags & GO_PARAM_PERSISTENT) {
			g_value_init (&val, props[n]->value_type);
			g_object_get_property (G_OBJECT (src), props[n]->name, &val);
			g_object_set_property (G_OBJECT (dst), props[n]->name, &val);
			g_value_unset (&val);
		}
	g_free (props);

	if (GOG_IS_DATASET (src)) {	/* convenience to save data */
		if (datadup)
			datadup (GOG_DATASET (src), GOG_DATASET (dst));
		else
			dataset_dup (GOG_DATASET (src), GOG_DATASET (dst));
	}
	if (GOG_IS_GRAPH (src))
		GOG_GRAPH (dst)->doc = GOG_GRAPH (src)->doc;
	else if (GOG_IS_CHART (src))
		GOG_CHART (dst)->axis_set = GOG_CHART (src)->axis_set;

	for (ptr = src->children; ptr != NULL ; ptr = ptr->next)
		/* children added directly to new parent, no need to use the
		 * function result */
		gog_object_dup (ptr->data, dst, datadup);

	return dst;
}

/**
 * gog_object_get_parent:
 * @obj: a #GogObject
 *
 * Returns: (transfer none): @obj's parent, potentially %NULL if it has not been added to a
 * 	heirarchy yet.  does not change ref-count in any way.
 **/
GogObject *
gog_object_get_parent (GogObject const *obj)
{
	g_return_val_if_fail (GOG_IS_OBJECT (obj), NULL);
	return obj->parent;
}

/**
 * gog_object_get_parent_typed:
 * @obj: a #GogObject
 * @t: a #GType
 *
 * Returns: (transfer none): @obj's parent of type @type, potentially %NULL if it has not been
 * added to a hierarchy yet or none of the parents are of type @type.
 **/
GogObject *
gog_object_get_parent_typed (GogObject const *obj, GType t)
{
	g_return_val_if_fail (GOG_IS_OBJECT (obj), NULL);

	for (; obj != NULL ; obj = obj->parent)
		if (G_TYPE_CHECK_INSTANCE_TYPE (obj, t))
			return GOG_OBJECT (obj); /* const cast */
	return NULL;
}

/**
 * gog_object_get_graph:
 * @obj: const * #GogObject
 *
 * Returns: (transfer none): the parent graph.
 **/
GogGraph *
gog_object_get_graph (GogObject const *obj)
{
	g_return_val_if_fail (GOG_IS_OBJECT (obj), NULL);

	for (; obj != NULL ; obj = obj->parent)
		if (GOG_IS_GRAPH (obj))
			return GOG_GRAPH (obj);
	return NULL;
}

/**
 * gog_object_get_theme:
 * @obj: const * #GogObject
 *
 * Returns: (transfer none): the parent graph theme.
 **/
GogTheme *
gog_object_get_theme (GogObject const *obj)
{
	GogGraph *graph = gog_object_get_graph (obj);

	return (graph != NULL) ? gog_graph_get_theme (graph) : NULL;
}

/**
 * gog_object_get_name:
 * @obj: a #GogObject
 *
 * No need to free the result
 *
 * Returns: a name.
 **/
char const *
gog_object_get_name (GogObject const *obj)
{
	g_return_val_if_fail (GOG_IS_OBJECT (obj), NULL);
	return (obj->user_name != NULL && *obj->user_name != '\0') ? obj->user_name : obj->auto_name;
}

/**
 * gog_object_set_name:
 * @obj: #GogObject
 * @name: (transfer full): The new name for @obj
 * @err: #GError
 *
 * Assign the new name and signals that it has changed.
 * NOTE : it _absorbs_ @name rather than copying it, and generates a new name
 * if @name == %NULL
 **/
void
gog_object_set_name (GogObject *obj, char *name, GError **err)
{
	GogObject *tmp;

	g_return_if_fail (GOG_IS_OBJECT (obj));

	if (obj->user_name == name)
		return;
	g_free (obj->user_name);
	obj->user_name = name;

	g_signal_emit (G_OBJECT (obj),
		gog_object_signals [NAME_CHANGED], 0);

	for (tmp = obj; tmp != NULL ; tmp = tmp->parent)
		g_signal_emit (G_OBJECT (tmp),
			gog_object_signals [CHILD_NAME_CHANGED], 0, obj);
}

/**
 * gog_object_get_children:
 * @obj: a #GogObject
 * @filter: an optional #GogObjectRole to use as a filter
 *
 * Returns: (element-type GogObject) (transfer container): list of @obj's
 * Children.  Caller must free the list, but not the children.
 **/
GSList *
gog_object_get_children (GogObject const *obj, GogObjectRole const *filter)
{
	GSList *ptr, *res = NULL;

	g_return_val_if_fail (GOG_IS_OBJECT (obj), NULL);

	if (filter == NULL)
		return g_slist_copy (obj->children);

	for (ptr = obj->children ; ptr != NULL ; ptr = ptr->next)
		if (GOG_OBJECT (ptr->data)->role == filter)
			res = g_slist_prepend (res, ptr->data);
	return g_slist_reverse (res);
}

/**
 * gog_object_get_child_by_role:
 * @obj: a #GogObject
 * @role: a #GogObjectRole to use as a filter
 *
 * A convenience routine to find a unique child with @role.
 *
 * Returns: (transfer none): %NULL and spews an error if there is more than one.
 **/
GogObject *
gog_object_get_child_by_role (GogObject const *obj, GogObjectRole const *role)
{
	GogObject *res = NULL;
	GSList *children;

	g_return_val_if_fail (GOG_IS_OBJECT (obj), NULL);

	children = gog_object_get_children (obj, role);
	if (children != NULL && children->next == NULL)
		res = children->data;
	g_slist_free (children);
	return res;
}

/**
 * gog_object_get_child_by_name:
 * @obj: a #GogObject
 * @name: a #char to use as a role name filter
 *
 * A convenience routine to find a unique child with role == @name
 *
 * Returns: (transfer none): %NULL and spews an error if there is more than one.
 **/
GogObject *
gog_object_get_child_by_name (GogObject const *obj, char const *name)
{
	g_return_val_if_fail (GOG_IS_OBJECT (obj), NULL);
	return gog_object_get_child_by_role (obj,
		gog_object_find_role_by_name (obj, name));
}

/**
 * gog_object_is_deletable:
 * @obj: a #GogObject
 *
 * Returns: %TRUE if @obj can be deleted.
 **/
gboolean
gog_object_is_deletable (GogObject const *obj)
{
	g_return_val_if_fail (GOG_IS_OBJECT (obj), FALSE);

	if (GOG_IS_GRAPH (obj))
		return FALSE;

	return obj->role == NULL || obj->role->can_remove == NULL ||
		(obj->role->can_remove) (obj);
}

struct possible_add_closure {
	GSList *res;
	GogObject const *parent;
};

static void
cb_collect_possible_additions (char const *name, GogObjectRole const *role,
			       struct possible_add_closure *data)
{
	if (role->can_add == NULL || (role->can_add) (data->parent))
		data->res = g_slist_prepend (data->res, (gpointer)role);
}

static int
gog_object_position_cmp (GogObjectPosition pos)
{
	if (pos & GOG_POSITION_COMPASS)
		return 0;
	if (GOG_POSITION_IS_SPECIAL (pos) ||
	    GOG_POSITION_IS_PADDING (pos))
		return 2;
	return 1; /* GOG_POSITION_MANUAL */
}

static int
gog_role_cmp (GogObjectRole const *a, GogObjectRole const *b)
{
	int index_a = gog_object_position_cmp (a->allowable_positions);
	int index_b = gog_object_position_cmp (b->allowable_positions);

	if (b->priority != a->priority)
		return b->priority - a->priority;

	/* intentionally reverse to put SPECIAL at the top */
	if (index_a < index_b)
		return 1;
	else if (index_a > index_b)
		return -1;
	return 0;
}

static int
gog_role_cmp_full (GogObjectRole const *a, GogObjectRole const *b)
{
	int res = gog_role_cmp (a, b);
	if (res != 0)
		return res;
	return g_utf8_collate (a->id, b->id);
}

/**
 * gog_object_possible_additions:
 * @parent: a #GogObject
 *
 * Returns: (element-type GogObjectRole) (transfer container): a list
 * of GogObjectRoles that could be added. The resulting list needs to be freed
 **/
GSList *
gog_object_possible_additions (GogObject const *parent)
{
	GogObjectClass *klass;

	g_return_val_if_fail (GOG_IS_OBJECT (parent), NULL);

	klass = GOG_OBJECT_GET_CLASS (parent);

	if (klass->roles != NULL) {
		struct possible_add_closure data;
		data.res = NULL;
		data.parent = parent;

		g_hash_table_foreach (klass->roles,
			(GHFunc) cb_collect_possible_additions, &data);

		return g_slist_sort (data.res, (GCompareFunc) gog_role_cmp_full);
	}

	return NULL;
}

/**
 * gog_object_can_reorder:
 * @obj: #GogObject
 * @inc_ok: optionally %NULL pointer for result.
 * @dec_ok: optionally %NULL pointer for result.
 *
 * If @obj can move forward or backward in its parents child list
 **/
void
gog_object_can_reorder (GogObject const *obj, gboolean *inc_ok, gboolean *dec_ok)
{
	GogObject const *parent;
	GSList *ptr;

	g_return_if_fail (GOG_IS_OBJECT (obj));

	if (inc_ok != NULL)
		*inc_ok = FALSE;
	if (dec_ok != NULL)
		*dec_ok = FALSE;

	if (obj->parent == NULL || gog_object_get_graph (obj) == NULL)
		return;
	parent = obj->parent;
	ptr = parent->children;

	g_return_if_fail (ptr != NULL);

	/* find a pointer to the previous sibling */
	if (ptr->data != obj) {
		while (ptr->next != NULL && ptr->next->data != obj)
			ptr = ptr->next;

		g_return_if_fail (ptr->next != NULL);

		if (inc_ok != NULL &&
		    !gog_role_cmp (((GogObject *)ptr->data)->role, obj->role))
			*inc_ok = TRUE;

		ptr = ptr->next;
	}

	/* ptr now points at @obj */
	if (dec_ok != NULL && ptr->next != NULL &&
	    !gog_role_cmp (obj->role, ((GogObject *)ptr->next->data)->role))
		*dec_ok = TRUE;
}

/**
 * gog_object_reorder:
 * @obj: #GogObject
 * @inc:
 * @goto_max:
 *
 * Returns: (transfer none): the object just before @obj in the new ordering.
 **/
GogObject *
gog_object_reorder (GogObject const *obj, gboolean inc, gboolean goto_max)
{
	GogObject *parent, *obj_follows;
	GSList **ptr, *tmp;

	g_return_val_if_fail (GOG_IS_OBJECT (obj), NULL);

	if (obj->parent == NULL || gog_object_get_graph (obj) == NULL)
		return NULL;
	parent = obj->parent;

	if (inc)
		parent->children = g_slist_reverse (parent->children);

	for (ptr = &parent->children; *ptr != NULL && (*ptr)->data != obj ;)
		ptr = &(*ptr)->next;

	g_return_val_if_fail (*ptr != NULL, NULL);
	g_return_val_if_fail ((*ptr)->next != NULL, NULL);

	tmp = *ptr;
	*ptr = tmp->next;
	ptr = &(*ptr)->next;

	while (goto_max && *ptr != NULL &&
	       !gog_role_cmp (obj->role, ((GogObject *)((*ptr)->data))->role))
		ptr = &(*ptr)->next;

	tmp->next = *ptr;
	*ptr = tmp;

	if (inc)
		parent->children = g_slist_reverse (parent->children);

	if (parent->children->data != obj) {
		for (tmp = parent->children ; tmp->next->data != obj ; )
			tmp = tmp->next;
		obj_follows = tmp->data;
	} else
		obj_follows = NULL;

	/* Pass the sibling that precedes obj, or %NULL if is the head */
	g_signal_emit (G_OBJECT (parent),
		gog_object_signals [CHILDREN_REORDERED], 0);
	gog_object_emit_changed (parent, FALSE);

	return obj_follows;
}

/**
 * gog_object_get_editor:
 * @obj: a #GogObject
 * @dalloc: a #GogDataAllocator
 * @cc: a #GOCmdContext
 *
 * Builds an object property editor, by calling GogObject::populate_editor
 * virtual functions.
 *
 * Returns: (transfer full): a #GtkNotebook widget
 **/
gpointer
gog_object_get_editor (GogObject *obj, GogDataAllocator *dalloc,
		       GOCmdContext *cc)
{
#ifdef GOFFICE_WITH_GTK
	GtkWidget *notebook;
	GOEditor *editor;
	GogObjectClass *klass;

	g_return_val_if_fail (GOG_IS_OBJECT (obj), NULL);

	klass = GOG_OBJECT_GET_CLASS (obj);

	editor = go_editor_new ();
	go_editor_set_use_scrolled_window (editor, TRUE);
	if (klass->populate_editor) {
		/* If there are pending updates do them before creating the editor
		 * to avoid expensive widget changes later */
		gog_graph_force_update (gog_object_get_graph (obj));
		(*klass->populate_editor) (obj, editor, dalloc, cc);
	}

	notebook = go_editor_get_notebook (editor);

	go_editor_free (editor);

	return notebook;
#else
	return NULL;
#endif
}

/**
 * gog_object_new_view:
 * @obj: a #GogObject
 * @parent: parent view
 *
 * Creates a new #GogView associated to @obj, and sets its parent to @parent.
 *
 * Returns: (transfer full): a new #GogView
 **/
GogView *
gog_object_new_view (GogObject const *obj, GogView *parent)
{
	GogObjectClass *klass;

	g_return_val_if_fail (GOG_IS_OBJECT (obj), NULL);

	klass = GOG_OBJECT_GET_CLASS (obj);

	if (klass->view_type != 0)
		/* set model before parent */
		return g_object_new (klass->view_type,
			"model", obj,
			"parent", parent,
			NULL);

	return NULL;
}

void
gog_object_update (GogObject *obj)
{
	GogObjectClass *klass;
	GSList *ptr;

	g_return_if_fail (GOG_IS_OBJECT (obj));

	klass = GOG_OBJECT_GET_CLASS (obj);

	ptr = obj->children; /* depth first */
	for (; ptr != NULL ; ptr = ptr->next)
		gog_object_update (ptr->data);

	if (obj->needs_update) {
		obj->needs_update = FALSE;
		obj->being_updated = TRUE;
		gog_debug (0, g_warning ("updating %s (%p)", G_OBJECT_TYPE_NAME (obj), obj););
		if (klass->update != NULL)
			(*klass->update) (obj);
		obj->being_updated = FALSE;
	}
}

gboolean
gog_object_request_update (GogObject *obj)
{
	GogGraph *graph;
	g_return_val_if_fail (GOG_OBJECT (obj), FALSE);
	g_return_val_if_fail (!obj->being_updated, FALSE);

	if (obj->needs_update)
		return FALSE;

	graph = gog_object_get_graph (obj);
	if (graph == NULL) /* we are not linked into a graph yet */
		return FALSE;

	gog_graph_request_update (graph);
	obj->needs_update = TRUE;

	return TRUE;
}

void
gog_object_emit_changed (GogObject *obj, gboolean resize)
{
	GogObjectClass *gog_klass;

	g_return_if_fail (GOG_OBJECT (obj));

	gog_klass = GOG_OBJECT_GET_CLASS (obj);

	if (gog_klass->use_parent_as_proxy) {
		obj = obj->parent;
		if (obj != NULL) {
			g_return_if_fail (GOG_IS_OBJECT (obj));
			gog_object_emit_changed (obj, resize);
		}
		return;
	}
	g_signal_emit (G_OBJECT (obj),
		gog_object_signals [CHANGED], 0, resize);
}

/**
 * gog_object_request_editor_update:
 * @obj: #GogObject
 *
 * Emits a update-editor signal. This signal should be used by object editors
 * in order to refresh their states.
 **/
void
gog_object_request_editor_update (GogObject *obj)
{
	g_signal_emit (G_OBJECT (obj),
		gog_object_signals [UPDATE_EDITOR], 0);
}

/******************************************************************************/

/**
 * gog_object_clear_parent:
 * @obj: #GogObject
 *
 * Does _not_ unref the child, which in effect adds a ref by freeing up the ref
 * previously associated with the parent.
 *
 * Returns: %TRUE on success.
 **/
gboolean
gog_object_clear_parent (GogObject *obj)
{
	GogObjectClass *klass;
	GogObject *parent;

	g_return_val_if_fail (GOG_OBJECT (obj), FALSE);
	g_return_val_if_fail (obj->parent != NULL, FALSE);
	g_return_val_if_fail (gog_object_is_deletable (obj), FALSE);

	klass = GOG_OBJECT_GET_CLASS (obj);
	parent = obj->parent;
	(*klass->parent_changed) (obj, FALSE);

	if (obj->role != NULL && obj->role->pre_remove != NULL)
		(obj->role->pre_remove) (parent, obj);

	parent->children = g_slist_remove (parent->children, obj);
	obj->parent = NULL;

	if (obj->role != NULL && obj->role->post_remove != NULL)
		(obj->role->post_remove) (parent, obj);

	obj->role = NULL;

	g_signal_emit (G_OBJECT (parent),
		gog_object_signals [CHILD_REMOVED], 0, obj);

	return TRUE;
}

/**
 * gog_object_set_parent:
 * @child: (transfer full): #GogObject.
 * @parent: #GogObject.
 * @id: optionally %NULL.
 * @role: a static string that can be sent to @parent::add
 *
 * Absorbs a ref to @child
 *
 * Returns: %TRUE on success
 **/
gboolean
gog_object_set_parent (GogObject *child, GogObject *parent,
		       GogObjectRole const *role, unsigned int id)
{
	GogObjectClass *klass;
	GSList **step;

	g_return_val_if_fail (GOG_OBJECT (child), FALSE);
	g_return_val_if_fail (child->parent == NULL, FALSE);
	g_return_val_if_fail (role != NULL, FALSE);

	klass = GOG_OBJECT_GET_CLASS (child);
	child->parent	= parent;
	child->role	= role;
	child->position = role->default_position;

	/* Insert sorted based on hokey little ordering */
	step = &parent->children;
	while (*step != NULL &&
	       gog_role_cmp_full (GOG_OBJECT ((*step)->data)->role, role) >= 0)
		step = &((*step)->next);
	*step = g_slist_prepend (*step, child);

	if (id != 0)
		gog_object_set_id (child, id);
	else
		gog_object_generate_id (child);

	if (role->post_add != NULL)
		(role->post_add) (parent, child);
	(*klass->parent_changed) (child, TRUE);

	g_signal_emit (G_OBJECT (parent),
		gog_object_signals [CHILD_ADDED], 0, child);

	return TRUE;
}

/**
 * gog_object_add_by_role:
 * @parent: #GogObject
 * @role: #GogObjectRole
 * @child: (transfer full) (allow-none): #GogObject
 *
 * Absorb a ref to @child if it is non-NULL.
 * Returns: (transfer none): @child or a newly created object with @role.
 **/
GogObject *
gog_object_add_by_role (GogObject *parent, GogObjectRole const *role, GogObject *child)
{
	GType is_a;
	gboolean explicitly_typed_role;

	g_return_val_if_fail (role != NULL, NULL);
	g_return_val_if_fail (GOG_OBJECT (parent) != NULL, NULL);

	is_a = g_type_from_name (role->is_a_typename);

	g_return_val_if_fail (is_a != 0, NULL);

	/*
	 * It's unclear why we need this flag; just set is to indicate a non-default
	 * type.  We used to set for any pre-allocated child.
	 */
	explicitly_typed_role = (child && G_OBJECT_TYPE (child) != is_a);

	if (child == NULL) {
		child = (role->allocate)
			? (role->allocate) (parent)
			: (G_TYPE_IS_ABSTRACT (is_a)? NULL: g_object_new (is_a, NULL));

		/* g_object_new or the allocator have already generated an
		 * error message, just exit */
		if (child == NULL)
			return NULL;
	}

	g_return_val_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (child, is_a), NULL);

	child->explicitly_typed_role = explicitly_typed_role;
	if (gog_object_set_parent (child, parent, role, 0))
		return child;
	g_object_unref (child);
	return NULL;
}

/**
 * gog_object_add_by_name:
 * @parent: #GogObject
 * @role:
 * @child: (transfer full) (allow-none): optionally null #GogObject
 *
 * Returns: (transfer none):  a newly created child of @parent in @role.  If @child is provided,
 * it is assumed to be an unaffiliated object that will be assigned in @role.
 * On failure return NULL.
 **/
GogObject *
gog_object_add_by_name (GogObject *parent,
			char const *role, GogObject *child)
{
	g_return_val_if_fail (GOG_IS_OBJECT (parent), NULL);
	return gog_object_add_by_role (parent,
		gog_object_find_role_by_name (parent, role), child);
}

/**
 * gog_object_set_invisible:
 * @obj: #GogObject
 * @invisible:
 **/
void
gog_object_set_invisible (GogObject *obj, gboolean invisible)
{
	unsigned int new_val = invisible ? 1 : 0;
	if ((!obj->invisible) != !(new_val)) {
		obj->invisible = new_val;
		gog_object_emit_changed (obj, TRUE);
	}
}

/**
 * gog_object_get_position_flags:
 * @obj: #GogObject
 * @mask: #GogObjectPosition
 *
 * Returns: @obj's position flags, masked by @mask.
 **/
GogObjectPosition
gog_object_get_position_flags (GogObject const *obj, GogObjectPosition mask)
{
	g_return_val_if_fail (GOG_IS_OBJECT (obj), GOG_POSITION_SPECIAL & mask);
	return obj->position & mask;
}

/**
 * gog_object_set_position_flags:
 * @obj: #GogObject
 * @flags: #GogObjectPosition
 * @mask: #GogObjectPosition
 *
 * Attempts to set the position flags of @obj to @flags.
 *
 * Returns: %TRUE if the new flags are permitted.
 **/
gboolean
gog_object_set_position_flags (GogObject *obj, GogObjectPosition flags, GogObjectPosition mask)
{
	g_return_val_if_fail (GOG_IS_OBJECT (obj), FALSE);

	if (obj->role == NULL)
		return FALSE;

	if ((obj->position & mask) == flags)
		return TRUE;

	if ((flags & obj->role->allowable_positions) !=
	    (flags & (GOG_POSITION_COMPASS | GOG_POSITION_ANY_MANUAL |
	              GOG_POSITION_ANY_MANUAL_SIZE))) {
		g_warning ("[GogObject::set_position_flags] Invalid flags (%s) flags=0x%x  allowable=0x%x",
			   gog_object_get_name (obj),
			   flags, obj->role->allowable_positions);
		return FALSE;
	}
	obj->position = (obj->position & ~mask) | (flags & mask);
	if (GOG_IS_CHART (obj))
		gog_graph_validate_chart_layout (GOG_GRAPH (obj->parent));
	gog_object_emit_changed (obj, TRUE);
	return TRUE;
}

/**
 * gog_object_get_manual_position:
 * @obj: #GogObject
 * @pos: #GogViewAllocation
 *
 * FIXME
 **/
void
gog_object_get_manual_position (GogObject *gobj, GogViewAllocation *pos)
{
	g_return_if_fail (GOG_OBJECT (gobj) != NULL);

	if (pos != NULL)
		*pos = gobj->manual_position;
}

/**
 * gog_object_set_manual_position:
 * @obj: #GogObject
 * @pos: #GogViewAllocation
 *
 * set manual position of given object, in points.
 **/
void
gog_object_set_manual_position (GogObject *gobj, GogViewAllocation const *pos)
{
	g_return_if_fail (GOG_OBJECT (gobj) != NULL);

	if (gobj->manual_position.x == pos->x &&
	    gobj->manual_position.y == pos->y &&
	    gobj->manual_position.w == pos->w &&
	    gobj->manual_position.h == pos->h)
		return;

	gobj->manual_position = *pos;
	gog_object_emit_changed (gobj, TRUE);
}

/**
 * gog_object_get_manual_allocation:
 * @gobj: #GogObject
 * @parent_allocation: #GogViewAllocation
 * @requisition: #GogViewRequisition
 *
 * Returns: manual allocation of a GogObject given its parent allocation and
 * its size request.
 **/
GogViewAllocation
gog_object_get_manual_allocation (GogObject *gobj,
				  GogViewAllocation const *parent_allocation,
				  GogViewRequisition const *requisition)
{
	GogViewAllocation pos;
	unsigned anchor, size, expand;
	GogManualSizeMode size_mode = gog_object_get_manual_size_mode (gobj);

	pos.x = parent_allocation->x + gobj->manual_position.x * parent_allocation->w;
	pos.y = parent_allocation->y + gobj->manual_position.y * parent_allocation->h;

	size = gog_object_get_position_flags (gobj, GOG_POSITION_ANY_MANUAL_SIZE);
	anchor = gog_object_get_position_flags (gobj, GOG_POSITION_ANCHOR);
	expand = gobj->role->allowable_positions & GOG_POSITION_EXPAND;

	if ((size_mode & GOG_MANUAL_SIZE_WIDTH) &&
	         (size & (GOG_POSITION_MANUAL_W | GOG_POSITION_MANUAL_W_ABS)))
		pos.w = gobj->manual_position.w * parent_allocation->w;
	else  if (expand & GOG_POSITION_HEXPAND) {
		/* use available width */
		switch (anchor) {
			case GOG_POSITION_ANCHOR_N:
			case GOG_POSITION_ANCHOR_CENTER:
			case GOG_POSITION_ANCHOR_S:
				pos.w = MIN (gobj->manual_position.x, 1 - gobj->manual_position.x)
						 * 2. * parent_allocation->w;
				break;
			case GOG_POSITION_ANCHOR_SE:
			case GOG_POSITION_ANCHOR_E:
			case GOG_POSITION_ANCHOR_NE:
				pos.w = gobj->manual_position.x * parent_allocation->w;
				break;
			default:
				pos.w = (1 - gobj->manual_position.x) * parent_allocation->w;
				break;
		}
		if (pos.w < requisition->w)
			pos.w = requisition->w;
	} else
		pos.w = requisition->w;

	switch (anchor) {
		case GOG_POSITION_ANCHOR_N:
		case GOG_POSITION_ANCHOR_CENTER:
		case GOG_POSITION_ANCHOR_S:
			pos.x -= pos.w / 2.0;
			break;
		case GOG_POSITION_ANCHOR_SE:
		case GOG_POSITION_ANCHOR_E:
		case GOG_POSITION_ANCHOR_NE:
			pos.x -= pos.w;
			break;
		default:
			break;
	}
	if ((size_mode & GOG_MANUAL_SIZE_HEIGHT) &&
	         (size & (GOG_POSITION_MANUAL_H | GOG_POSITION_MANUAL_H_ABS)))
		pos.h = gobj->manual_position.h * parent_allocation->h;
	else  if (expand & GOG_POSITION_VEXPAND) {
		/* use available width */
		switch (anchor) {
			case GOG_POSITION_ANCHOR_E:
			case GOG_POSITION_ANCHOR_CENTER:
			case GOG_POSITION_ANCHOR_W:
				pos.h = MIN (gobj->manual_position.y, 1 - gobj->manual_position.y)
						 * 2. * parent_allocation->h;
				break;
			case GOG_POSITION_ANCHOR_SE:
			case GOG_POSITION_ANCHOR_S:
			case GOG_POSITION_ANCHOR_SW:
				pos.h = gobj->manual_position.y * parent_allocation->h;
				break;
			default:
				pos.h = (1 - gobj->manual_position.y) * parent_allocation->h;
				break;
		}
		if (pos.h < requisition->h)
			pos.h = requisition->h;
	} else
		pos.h = requisition->h;
	switch (anchor) {
		case GOG_POSITION_ANCHOR_E:
		case GOG_POSITION_ANCHOR_CENTER:
		case GOG_POSITION_ANCHOR_W:
			pos.y -= pos.h / 2.0;
			break;
		case GOG_POSITION_ANCHOR_SE:
		case GOG_POSITION_ANCHOR_S:
		case GOG_POSITION_ANCHOR_SW:
			pos.y -= pos.h;
			break;
		default:
			break;
	}

	return pos;
}

/*
 * gog_object_is_default_position_flags:
 * @obj: a #GogObject
 * @name: name of the position property
 *
 * returns: true if the current position flags corresponding to the property @name
 * are the defaults stored in @obj role.
 **/
gboolean
gog_object_is_default_position_flags (GogObject const *obj, char const *name)
{
	int mask;

	g_return_val_if_fail (name != NULL, FALSE);

	if (obj->role == NULL)
		return FALSE;

	if (strcmp (name, "compass") == 0)
		mask = GOG_POSITION_COMPASS;
	else if (strcmp (name, "alignment") == 0)
		mask = GOG_POSITION_ALIGNMENT;
	else if (strcmp (name, "anchor") == 0)
		mask = GOG_POSITION_ANCHOR;
	else
		return FALSE;

	return (obj->position & mask) == (obj->role->default_position & mask);
}

GogObjectRole const *
gog_object_find_role_by_name (GogObject const *obj, char const *role)
{
	GogObjectClass *klass;

	g_return_val_if_fail (GOG_IS_OBJECT (obj), NULL);

	klass = GOG_OBJECT_GET_CLASS (obj);

	return g_hash_table_lookup (klass->roles, role);
}

static void
cb_copy_hash_table (gpointer key, gpointer value, GHashTable *hash_table)
{
	g_hash_table_insert (hash_table, key, value);
}

static void
gog_object_allocate_roles (GogObjectClass *klass)
{
	GHashTable *roles = g_hash_table_new (g_str_hash, g_str_equal);

	if (klass->roles != NULL)
		g_hash_table_foreach (klass->roles,
			(GHFunc) cb_copy_hash_table, roles);
	klass->roles = roles;
	klass->roles_allocated = TRUE;
}

/**
 * gog_object_register_roles:
 * @klass: #GogObjectClass
 * @roles: #GogObjectRole
 * @n_roles: number of roles
 *
 **/

void
gog_object_register_roles (GogObjectClass *klass,
			   GogObjectRole const *roles, unsigned int n_roles)
{
	unsigned i;

	if (!klass->roles_allocated)
		gog_object_allocate_roles (klass);

	for (i = 0 ; i < n_roles ; i++) {
		g_return_if_fail (g_hash_table_lookup (klass->roles,
			(gpointer )roles[i].id) == NULL);
		g_hash_table_replace (klass->roles,
			(gpointer )roles[i].id, (gpointer) (roles + i));
	}
}

void
gog_object_document_changed (GogObject *obj, GODoc *doc)
{
	GSList *ptr;
	g_return_if_fail (GOG_IS_OBJECT (obj) && GO_IS_DOC (doc));
	if (GOG_OBJECT_GET_CLASS (obj)->document_changed != NULL)
		GOG_OBJECT_GET_CLASS (obj)->document_changed (obj, doc);
	for (ptr = obj->children; ptr != NULL; ptr = ptr->next)
		gog_object_document_changed (GOG_OBJECT (ptr->data), doc);
}

GogManualSizeMode
gog_object_get_manual_size_mode (GogObject *obj)
{
	if (GOG_OBJECT_GET_CLASS (obj)->get_manual_size_mode != NULL)
		return GOG_OBJECT_GET_CLASS (obj)->get_manual_size_mode (obj);
	return GOG_MANUAL_SIZE_AUTO;
}
