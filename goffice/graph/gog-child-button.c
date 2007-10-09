/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-child-button.c :
 *
 * Copyright (C) 2000-2004 Jody Goldberg (jody@gnome.org)
 * Copyright (C) 2007 Emmanuel Pacaud (emmanuel.pacaud@lapp.in2p3.fr)
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

#include "gog-child-button.h"

#include <goffice/graph/gog-chart.h>
#include <goffice/graph/gog-plot-engine.h>
#include <goffice/graph/gog-plot.h>
#include <goffice/graph/gog-trend-line.h>
#include <goffice/gtk/goffice-gtk.h>
#include <goffice/gtk/go-pixbuf.h>
#include <goffice/goffice-config.h>

#include <gtk/gtkmenu.h>
#include <gtk/gtkimagemenuitem.h>
#include <gtk/gtktogglebutton.h>

#include <gdk/gdkpixbuf.h>

#include <glib/gi18n-lib.h>
#include <glib/ghash.h>

#include <string.h>

static void     gog_child_button_finalize   		(GObject *object);
static void	gog_child_button_toggled_cb		(GtkToggleButton *toggle_button,
							 GogChildButton *child_button);
static void 	gog_child_button_popup 			(GogChildButton *child_button);
static void 	gog_child_button_popdown		(GogChildButton *child_button);
static void 	gog_child_button_weak_notify 		(GogChildButton *child_button, GogObject *object);

struct _GogChildButton
{
	GtkHBox parent;

	GtkWidget	*toggle_button;
	GtkMenu 	*menu;

	GogObject 	*object;
	GSList 		*additions;
};

struct _GogChildButtonClass
{
	GtkHBoxClass parent_class;
};

G_DEFINE_TYPE (GogChildButton, gog_child_button, GTK_TYPE_HBOX)

static void
gog_child_button_init (GogChildButton *child_button)
{
	child_button->object = NULL;
	child_button->additions = NULL;
	child_button->menu = NULL;

	gtk_widget_push_composite_child ();

	child_button->toggle_button = gtk_toggle_button_new_with_label ("gtk-add");
	gtk_button_set_use_stock (GTK_BUTTON (child_button->toggle_button), TRUE);
	gtk_widget_show (child_button->toggle_button);

	gtk_widget_pop_composite_child ();

	gtk_box_pack_start (GTK_BOX (child_button), child_button->toggle_button, TRUE, TRUE, 0);

	g_signal_connect (G_OBJECT (child_button->toggle_button), "toggled",
			  G_CALLBACK (gog_child_button_toggled_cb), child_button);

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

	g_slist_free (child_button->additions);
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

/**
 * gog_child_button_set_object:
 * @child_button: a #GogChildButton
 * @object: a #GogObject
 *
 * Sets the current object with which @child_button operates.
 */

void
gog_child_button_set_object (GogChildButton *child_button, GogObject *object)
{
	g_return_if_fail (GOG_IS_CHILD_BUTTON (child_button));
	g_return_if_fail (IS_GOG_OBJECT (object));

	if (object == child_button->object)
		return;

	g_slist_free (child_button->additions);
	child_button->additions = NULL;
	if (child_button->menu != NULL) {
		g_object_unref (child_button->menu);
		child_button->menu =NULL;
	}
	if (child_button->object != NULL) {
		g_object_weak_unref (G_OBJECT (child_button->object),
				     (GWeakNotify) gog_child_button_weak_notify, child_button);
		child_button->object = NULL;
	}

	g_object_weak_ref (G_OBJECT (object), (GWeakNotify) gog_child_button_weak_notify, child_button);
	child_button->object = object;
	child_button->additions = gog_object_possible_additions (object);

	gtk_widget_set_sensitive (GTK_WIDGET (child_button), child_button->additions != NULL);
}

static void
gog_child_button_popup (GogChildButton *child_button)
{
	if (!GTK_WIDGET_REALIZED (child_button))
		return;

	if (GTK_WIDGET_MAPPED (child_button->menu))
		return;

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (child_button->toggle_button), TRUE);
	gtk_menu_popup (GTK_MENU (child_button->menu),
			NULL, NULL,
			go_menu_position_below, child_button,
			0, 0);
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
#define ROLE_KEY		"role"
#define STATE_KEY		"plot_type"

typedef struct {
	GogChildButton	*child_button;
	GtkWidget	*menu;
	gboolean 	 non_blank;
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

	gog_object_add_by_name (GOG_OBJECT (child_button->object),
				"Plot", GOG_OBJECT (plot));
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
	GogChildButton *child_button = closure->child_button;
	GogChart *chart = GOG_CHART (child_button->object);
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
			go_pixbuf_get_from_cache (family->sample_image_file)));
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
				go_pixbuf_get_from_cache (type->sample_image_file)));
		g_object_set_data (G_OBJECT (w), PLOT_TYPE_KEY, type);
		g_signal_connect (G_OBJECT (w), "activate",
				  G_CALLBACK (cb_graph_guru_add_plot),
				  closure->child_button);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), w);
	}
	g_slist_free (types);
}

static GtkWidget *
plot_type_menu_create (GogChildButton *child_button)
{
	TypeMenuCreateData closure;
	closure.child_button = child_button;
	closure.menu = gtk_menu_new ();
	closure.non_blank = FALSE;

	g_hash_table_foreach ((GHashTable *)gog_plot_families (),
		(GHFunc) cb_plot_family_menu_create, &closure);

	if (closure.non_blank)
		return closure.menu;
	gtk_object_destroy (GTK_OBJECT (closure.menu));
	return NULL;
}

static void
cb_graph_guru_add_trend_line (GtkWidget *widget, GogChildButton *child_button)
{
	GogTrendLineType *type = g_object_get_data (G_OBJECT (widget), TREND_LINE_TYPE_KEY);
	GogTrendLine *line = gog_trend_line_new_by_type (type);

	gog_object_add_by_name (GOG_OBJECT (child_button->object),
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
	g_signal_connect (G_OBJECT (menu), "activate",
			  G_CALLBACK (cb_graph_guru_add_trend_line),
			  closure->child_button);
	gtk_menu_shell_append (GTK_MENU_SHELL (closure->menu), menu);
	closure->non_blank = TRUE;
}

static GtkWidget *
trend_line_type_menu_create (GogChildButton *child_button)
{
	TypeMenuCreateData closure;
	closure.child_button = child_button;
	closure.menu = gtk_menu_new ();
	closure.non_blank = FALSE;

	g_hash_table_foreach ((GHashTable *)gog_trend_line_types (),
		(GHFunc) cb_trend_line_type_menu_create, &closure);

	if (closure.non_blank)
		return closure.menu;
	gtk_object_destroy (GTK_OBJECT (closure.menu));
	return NULL;
}

static void
cb_graph_guru_add_item (GtkWidget *widget, GogChildButton *child_button)
{
	gog_object_add_by_role (child_button->object,
				g_object_get_data (G_OBJECT (widget), ROLE_KEY), NULL);

	gog_child_button_popdown (child_button);
}

static void
cb_menu_deactivate (GtkMenu *menu, GogChildButton *child_button)
{
	gog_child_button_popdown (child_button);
}

static void
gog_child_button_toggled_cb (GtkToggleButton *toggle_button, GogChildButton *child_button)
{
	g_return_if_fail (child_button->additions != NULL);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggle_button))) {
		if (child_button->menu == NULL) {
			GtkWidget *widget;
			GogObjectRole const *role;
			GSList *iter;

			child_button->menu = GTK_MENU (gtk_menu_new ());
			g_object_ref_sink (child_button->menu);

			for (iter = child_button->additions ; iter != NULL ; iter = iter->next) {
				role = iter->data;
				if (!strcmp (role->id, "Trend line")) {
					GtkWidget *submenu = trend_line_type_menu_create (child_button);
					if (submenu != NULL) {
						widget = gtk_menu_item_new_with_label (_(role->id));
						gtk_menu_item_set_submenu (GTK_MENU_ITEM (widget), submenu);
					} else
						continue;
				} else if (!strcmp (role->id, "Plot")) {
					GtkWidget *submenu = plot_type_menu_create (child_button);
					if (submenu != NULL) {
						widget = gtk_menu_item_new_with_label (_(role->id));
						gtk_menu_item_set_submenu (GTK_MENU_ITEM (widget), submenu);
					} else
						continue;
				} else if (role->naming_conv == GOG_OBJECT_NAME_BY_ROLE) {
					widget = gtk_menu_item_new_with_label (_(role->id));
					g_object_set_data (G_OBJECT (widget), ROLE_KEY,
							   (gpointer)role);
					g_signal_connect (G_OBJECT (widget), "activate",
							  G_CALLBACK (cb_graph_guru_add_item),
							  child_button);
				} else
					continue;

				gtk_menu_shell_append (GTK_MENU_SHELL (child_button->menu), widget);
				g_signal_connect (child_button->menu, "deactivate",
						  G_CALLBACK(cb_menu_deactivate), child_button);
			}
			gtk_widget_show_all (GTK_WIDGET (child_button->menu));
		}
		gog_child_button_popup (child_button);
	} else
		gog_child_button_popdown (child_button);
}
