/*
 * gog-child-button.c :
 *
 * Copyright (C) 2000-2004 Jody Goldberg (jody@gnome.org)
 * Copyright (C) 2007 Emmanuel Pacaud (emmanuel.pacaud@lapp.in2p3.fr)
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

#include <gdk/gdkkeysyms.h>

#include <glib/gi18n-lib.h>

#include <string.h>

static void     gog_child_button_finalize   		(GObject *object);
static gboolean	gog_child_button_toggled_cb		(GtkToggleButton *toggle_button,
							 GogChildButton *child_button);
static gboolean	gog_child_button_press_event_cb		(GtkToggleButton *toggle_button,
							 GdkEventButton *event,
							 GogChildButton *child_button);
static void 	gog_child_button_popup 			(GogChildButton *child_button,
							 guint button,
							 guint32 event_time);
static void 	gog_child_button_popdown		(GogChildButton *child_button);
static void 	gog_child_button_weak_notify 		(GogChildButton *child_button, GogObject *object);
static void 	gog_child_button_free_additions 	(GogChildButton *child_button);

struct _GogChildButton
{
	GtkBox parent;

	GtkWidget	*toggle_button;
	GtkMenu 	*menu;

	GogObject 	*object;
	GSList 		*additions;

	gboolean	 button_handling_in_progress;
};

struct _GogChildButtonClass
{
	GtkBoxClass parent_class;
};

G_DEFINE_TYPE (GogChildButton, gog_child_button, GTK_TYPE_BOX)

static void
gog_child_button_init (GogChildButton *child_button)
{
	child_button->object = NULL;
	child_button->additions = NULL;
	child_button->menu = NULL;
	child_button->button_handling_in_progress = FALSE;

	child_button->toggle_button = gtk_toggle_button_new_with_label ("gtk-add");
	gtk_button_set_use_stock (GTK_BUTTON (child_button->toggle_button), TRUE);
	gtk_widget_show (child_button->toggle_button);

	gtk_box_pack_start (GTK_BOX (child_button), child_button->toggle_button, TRUE, TRUE, 0);

	g_signal_connect (G_OBJECT (child_button->toggle_button), "toggled",
			  G_CALLBACK (gog_child_button_toggled_cb), child_button);
	g_signal_connect (G_OBJECT (child_button->toggle_button), "button-press-event",
			  G_CALLBACK (gog_child_button_press_event_cb), child_button);

	gtk_widget_set_sensitive (GTK_WIDGET (child_button), FALSE);
}

static void
gog_child_button_class_init (GogChildButtonClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = gog_child_button_finalize;
}

static void
gog_child_button_finalize (GObject *object)
{
	GogChildButton *child_button = GOG_CHILD_BUTTON (object);

	gog_child_button_free_additions (child_button);
	if (child_button->menu)
		g_object_unref (child_button->menu);
	if (child_button->object != NULL)
		g_object_weak_unref (G_OBJECT (child_button->object),
				     (GWeakNotify) gog_child_button_weak_notify, child_button);

	(* G_OBJECT_CLASS (gog_child_button_parent_class)->finalize) (object);
}

/**
 * gog_child_button_new:
 *
 * Creates an add button that provides a popup menu for child creation.
 *
 * Returns: a new #GtkWidget.
 **/

GtkWidget *
gog_child_button_new (void)
{
	return g_object_new (GOG_TYPE_CHILD_BUTTON, NULL);
}

static void
gog_child_button_weak_notify (GogChildButton *child_button, GogObject *object)
{
	gtk_widget_set_sensitive (GTK_WIDGET (child_button), FALSE);

	g_slist_free (child_button->additions);
	child_button->additions = NULL;
	child_button->object = NULL;
	if (child_button->menu) {
		g_object_unref (child_button->menu);
		child_button->menu = NULL;
	}
}

typedef struct {
	GogObjectRole	*role;
	GogObject	*parent;
} Addition;

typedef struct {
	GHashTable 	*additions;
	GogChildButton	*child_button;
} BuildAdditionData;

static void
build_addition_process_children (GogObject *object, BuildAdditionData *closure)
{
	GSList *role_iter;
	GSList *additions;

	additions = gog_object_possible_additions (object);
	for (role_iter = additions; role_iter != NULL; role_iter = role_iter->next) {
		GogObjectRole *role = role_iter->data;
		Addition *addition =
			g_hash_table_lookup (closure->additions, role->id);
		if ( addition == NULL) {
			addition = g_new (Addition, 1);
			addition->role = role;
			addition->parent = object;
			g_hash_table_insert (closure->additions, (char *) role->id, addition);
		} else if (closure->child_button->object == object) {
			g_hash_table_remove (closure->additions, (char *) addition->role->id);
			g_free (addition);
			addition = g_new (Addition, 1);
			addition->role = role;
			addition->parent = object;
			g_hash_table_insert (closure->additions, (char *) role->id, addition);
		} else if (addition->parent != closure->child_button->object)
			addition->parent = NULL;
	}
	g_slist_free (additions);

	if (!GOG_IS_GRAPH (object)) {
		GSList *child_iter;
		for (child_iter = object->children;
		     child_iter;
		     child_iter = child_iter->next)
			build_addition_process_children (child_iter->data, closure);
	}
}

static void
build_addition_foreach (char *key, Addition *addition, GSList **additions)
{
	if (addition->parent != NULL)
		*additions = g_slist_append (*additions, addition);
	else
		g_free (addition);
}

static int
addition_compare (void const *abstract_addition1, void const *abstract_addition2)
{
	Addition const *addition1 = abstract_addition1;
	Addition const *addition2 = abstract_addition2;

	return strcmp (_(addition1->role->id), _(addition2->role->id));
}

static void
gog_child_button_build_additions (GogChildButton *child_button)
{
	BuildAdditionData closure;
	GogObject *start_object;

	if (child_button->additions != NULL)
		gog_child_button_free_additions (child_button);
	if (child_button->object == NULL)
		return;

	closure.additions = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, NULL);
	closure.child_button = child_button;

	for (start_object = child_button->object;
		  !GOG_IS_CHART (start_object) &&
		  !GOG_IS_GRAPH (start_object) &&
		  start_object != NULL;
		  start_object = start_object->parent);

	if (start_object != NULL) {
		build_addition_process_children (start_object, &closure);

		g_hash_table_foreach (closure.additions,
				      (GHFunc) build_addition_foreach,
				      &child_button->additions);
		child_button->additions = g_slist_sort (child_button->additions, addition_compare);
	}

	g_hash_table_unref (closure.additions);
}

static void
gog_child_button_free_additions (GogChildButton *child_button)
{
	GSList *iter;

	for (iter = child_button->additions; iter != NULL; iter = iter->next)
		g_free (iter->data);

	g_slist_free (child_button->additions);
	child_button->additions = NULL;
}

/**
 * gog_child_button_set_object:
 * @child_button: a #GogChildButton
 * @gog_object: a #GogObject
 *
 * Sets the current object with which @child_button operates.
 */
void
gog_child_button_set_object (GogChildButton *child_button, GogObject *gog_object)
{
	g_return_if_fail (GOG_IS_CHILD_BUTTON (child_button));
	g_return_if_fail (GOG_IS_OBJECT (gog_object));

	if (gog_object == child_button->object)
		return;

	gog_child_button_free_additions (child_button);
	if (child_button->menu != NULL) {
		g_object_unref (child_button->menu);
		child_button->menu =NULL;
	}
	if (child_button->object != NULL) {
		g_object_weak_unref (G_OBJECT (child_button->object),
				     (GWeakNotify) gog_child_button_weak_notify, child_button);
		child_button->object = NULL;
	}

	g_object_weak_ref (G_OBJECT (gog_object), (GWeakNotify) gog_child_button_weak_notify, child_button);
	child_button->object = gog_object;
	gog_child_button_build_additions (child_button);

	gtk_widget_set_sensitive (GTK_WIDGET (child_button), child_button->additions != NULL);
}

static void
gog_child_button_popup (GogChildButton *child_button, guint button, guint32 event_time)
{
	if (!gtk_widget_get_realized (GTK_WIDGET (child_button)))
		return;

	if (gtk_widget_get_mapped (GTK_WIDGET (child_button->menu)))
		return;

	child_button->button_handling_in_progress = TRUE;
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (child_button->toggle_button), TRUE);
	child_button->button_handling_in_progress = FALSE;

	gtk_menu_popup (GTK_MENU (child_button->menu),
			NULL, NULL,
			go_menu_position_below, child_button,
			button, event_time);
}
static void
gog_child_button_popdown (GogChildButton *child_button)
{
	if (child_button->menu != NULL)
		gtk_menu_popdown (GTK_MENU (child_button->menu));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (child_button->toggle_button), FALSE);
}

#define PLOT_TYPE_KEY		"plot_type"
#define TREND_LINE_TYPE_KEY	"trend_line_type"
#define FIRST_MINOR_TYPE	"first_minor_type"
#define ADDITION_KEY		"addition"
#define STATE_KEY		"plot_type"

typedef struct {
	GogChildButton	*child_button;
	GtkWidget	*menu;
	gboolean 	 non_blank;
	Addition *addition;
} TypeMenuCreateData;

static gint
cb_cmp_plot_type (GogPlotType const *a, GogPlotType const *b)
{
	if (a->row == b->row)
		return a->col - b->col;
	return a->row - b->row;
}

static void
cb_plot_type_list (char const *id, GogPlotType *type, GSList **list)
{
	*list = g_slist_insert_sorted (*list, type,
		(GCompareFunc) cb_cmp_plot_type);
}

static void
cb_graph_guru_add_plot (GtkWidget *widget, GogChildButton *child_button)
{
	GogPlotType *type = g_object_get_data (G_OBJECT (widget), PLOT_TYPE_KEY);
	GogPlot *plot = gog_plot_new_by_type (type);
	Addition *addition = g_object_get_data (G_OBJECT (widget), ADDITION_KEY);

	gog_object_add_by_name (GOG_OBJECT (addition->parent), "Plot", GOG_OBJECT (plot));
	gog_plot_guru_helper (plot);

	/* as a convenience add a series to the newly created plot */
	gog_object_add_by_name (GOG_OBJECT (plot), "Series", NULL);

	gog_child_button_popdown (child_button);
}

static void
cb_plot_family_menu_create (char const *id,
			    GogPlotFamily *family,
			    TypeMenuCreateData *closure)
{
	GogChart *chart = GOG_CHART (closure->addition->parent);
	GtkWidget *w, *menu;
	GSList *ptr, *types = NULL;
	GogPlotType *type;
	GogAxisSet axis_set;

	if (g_hash_table_size (family->types) <= 0)
		return;

	axis_set = gog_chart_get_axis_set (chart) & GOG_AXIS_SET_FUNDAMENTAL;

	if (axis_set != GOG_AXIS_SET_FUNDAMENTAL &&
	    (family->axis_set & GOG_AXIS_SET_FUNDAMENTAL) != axis_set)
		return;

	menu = gtk_image_menu_item_new_with_label (_(family->name));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu),
		gtk_image_new_from_pixbuf (
			go_gdk_pixbuf_get_from_cache (family->sample_image_file)));
	gtk_menu_shell_append (GTK_MENU_SHELL (closure->menu), menu);
	closure->non_blank = TRUE;

	w = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu), w);
	menu = w;

	g_hash_table_foreach (family->types, (GHFunc) cb_plot_type_list, &types);

	for (ptr = types ; ptr != NULL ; ptr = ptr->next) {
		type = ptr->data;
		w = gtk_image_menu_item_new_with_label (_(type->name));
		gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (w),
			gtk_image_new_from_pixbuf (
				go_gdk_pixbuf_get_from_cache (type->sample_image_file)));
		g_object_set_data (G_OBJECT (w), ADDITION_KEY, closure->addition);
		g_object_set_data (G_OBJECT (w), PLOT_TYPE_KEY, type);
		g_signal_connect (G_OBJECT (w), "activate",
				  G_CALLBACK (cb_graph_guru_add_plot),
				  closure->child_button);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), w);
	}
	g_slist_free (types);
}

static GtkWidget *
plot_type_menu_create (GogChildButton *child_button, Addition *addition)
{
	TypeMenuCreateData closure;
	closure.child_button = child_button;
	closure.menu = gtk_menu_new ();
	closure.non_blank = FALSE;
	closure.addition = addition;

	g_hash_table_foreach ((GHashTable *)gog_plot_families (),
		(GHFunc) cb_plot_family_menu_create, &closure);

	if (closure.non_blank)
		return closure.menu;
	gtk_widget_destroy (closure.menu);
	return NULL;
}

static void
cb_graph_guru_add_trend_line (GtkWidget *widget, GogChildButton *child_button)
{
	GogTrendLineType *type = g_object_get_data (G_OBJECT (widget), TREND_LINE_TYPE_KEY);
	GogTrendLine *line = gog_trend_line_new_by_type (type);
	Addition *addition = g_object_get_data (G_OBJECT (widget), ADDITION_KEY);

	gog_object_add_by_name (GOG_OBJECT (addition->parent),
				"Trend line", GOG_OBJECT (line));

	gog_child_button_popdown (child_button);
}

static void
cb_trend_line_type_menu_create (char const *id,
				GogTrendLineType *type,
				TypeMenuCreateData *closure)
{
	GtkWidget *menu;

	menu = gtk_menu_item_new_with_label (_(type->name));
	g_object_set_data (G_OBJECT (menu), TREND_LINE_TYPE_KEY, type);
	g_object_set_data (G_OBJECT (menu), ADDITION_KEY, closure->addition);
	g_signal_connect (G_OBJECT (menu), "activate",
			  G_CALLBACK (cb_graph_guru_add_trend_line),
			  closure->child_button);
	gtk_menu_shell_append (GTK_MENU_SHELL (closure->menu), menu);
	closure->non_blank = TRUE;
}

static GtkWidget *
trend_line_type_menu_create (GogChildButton *child_button, Addition *addition)
{
	TypeMenuCreateData closure;
	closure.child_button = child_button;
	closure.menu = gtk_menu_new ();
	closure.non_blank = FALSE;
	closure.addition = addition;

	g_hash_table_foreach ((GHashTable *)gog_trend_line_types (),
		(GHFunc) cb_trend_line_type_menu_create, &closure);

	if (closure.non_blank)
		return closure.menu;
	gtk_widget_destroy (closure.menu);
	return NULL;
}

static void
cb_graph_guru_add_item (GtkWidget *widget, GogChildButton *child_button)
{
	Addition *addition = g_object_get_data (G_OBJECT (widget), ADDITION_KEY);

	gog_object_add_by_role (addition->parent, addition->role, NULL);

	gog_child_button_popdown (child_button);
}

static void
cb_menu_deactivate (GtkMenu *menu, GogChildButton *child_button)
{
	gog_child_button_popdown (child_button);
}

static void
ensure_menu (GogChildButton *child_button)
{
	if (child_button->menu == NULL) {
		GtkWidget *widget;
		Addition *addition;
		GSList *iter;
		char *label = NULL;

		child_button->menu = GTK_MENU (gtk_menu_new ());
		g_object_ref_sink (child_button->menu);

		for (iter = child_button->additions ; iter != NULL ; iter = iter->next) {
			addition = iter->data;
			g_free (label);
			/* Note for translators: first string represent the new child object and second string is the parent object */
			label = g_strdup_printf(_("%s to %s"), _(addition->role->id), gog_object_get_name (addition->parent));
			if (!strcmp (addition->role->id, "Trend line")) {
				GtkWidget *submenu = trend_line_type_menu_create (child_button, addition);
				if (submenu != NULL) {
					widget = gtk_menu_item_new_with_label (label);
					gtk_menu_item_set_submenu (GTK_MENU_ITEM (widget), submenu);
				} else
					continue;
			} else if (!strcmp (addition->role->id, "Plot")) {
				GtkWidget *submenu = plot_type_menu_create (child_button, addition);
				if (submenu != NULL) {
					widget = gtk_menu_item_new_with_label (label);
					gtk_menu_item_set_submenu (GTK_MENU_ITEM (widget), submenu);
				} else
					continue;
			} else if (addition->role->naming_conv == GOG_OBJECT_NAME_BY_ROLE) {
				widget = gtk_menu_item_new_with_label (label);
				g_object_set_data (G_OBJECT (widget), ADDITION_KEY,
						   (gpointer)addition);
				g_signal_connect (G_OBJECT (widget), "activate",
						  G_CALLBACK (cb_graph_guru_add_item),
						  child_button);
			} else
				continue;

			gtk_menu_shell_append (GTK_MENU_SHELL (child_button->menu), widget);
		}
		g_free (label);
		g_signal_connect (child_button->menu, "deactivate",
				  G_CALLBACK(cb_menu_deactivate), child_button);
		gtk_widget_show_all (GTK_WIDGET (child_button->menu));
		gtk_menu_shell_set_take_focus (GTK_MENU_SHELL (child_button->menu), TRUE);
	}
}

static gboolean
gog_child_button_toggled_cb (GtkToggleButton *toggle_button,
		      GogChildButton *child_button)
{
	g_return_val_if_fail (child_button->additions != NULL, FALSE);

	if (gtk_toggle_button_get_active (toggle_button) &&
	    !child_button->button_handling_in_progress) {
		ensure_menu (child_button);
		gog_child_button_popup (child_button, 0, 0);
	}

	return FALSE;
}

static gboolean
gog_child_button_press_event_cb (GtkToggleButton *toggle_button,
				 GdkEventButton *event,
				 GogChildButton *child_button)
{
	g_return_val_if_fail (child_button->additions != NULL, FALSE);

	ensure_menu (child_button);

	gog_child_button_popup (child_button, event->button, event->time);

	return FALSE;
}
