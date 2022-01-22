/*
 * go-action-combo-pixmaps.c: A custom GtkAction to chose among a set of images
 *
 * Copyright (C) 2004 Jody Goldberg (jody@gnome.org)
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
#include "go-action-combo-pixmaps.h"
#include "go-combo-pixmaps.h"
#include "go-combo-box.h"
#include "goffice-gtk.h"

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

/**
 * GOActionComboPixmapsElement:
 * @untranslated_tooltip: the element tooltip.
 * @stock_id: icon identifier.
 * @id: pixmap identifier.
 **/

typedef struct {
	GtkToolItem	 base;
	GOComboPixmaps	*combo;	/* container has a ref, not us */
} GOToolComboPixmaps;
typedef GtkToolItemClass GOToolComboPixmapsClass;

#define GO_TYPE_TOOL_COMBO_PIXMAPS	(go_tool_combo_pixmaps_get_type ())
#define GO_TOOL_COMBO_PIXMAPS(o)	(G_TYPE_CHECK_INSTANCE_CAST (o, GO_TYPE_TOOL_COMBO_PIXMAPS, GOToolComboPixmaps))
#define GO_IS_TOOL_COMBO_PIXMAPS(o)	(G_TYPE_CHECK_INSTANCE_TYPE (o, GO_TYPE_TOOL_COMBO_PIXMAPS))

static GType go_tool_combo_pixmaps_get_type (void);

static GSF_CLASS (GOToolComboPixmaps, go_tool_combo_pixmaps,
	   NULL, NULL,
	   GTK_TYPE_TOOL_ITEM)

/*****************************************************************************/

struct _GOActionComboPixmaps {
	GtkAction	base;
	GOActionComboPixmapsElement const *elements;
	int ncols, nrows;

	gboolean updating_proxies;
	int selected_id;
};
typedef struct {
	GtkActionClass base;
	void (*combo_activate) (GOActionComboPixmaps *paction);
} GOActionComboPixmapsClass;

enum {
	COMBO_ACTIVATE,
	LAST_SIGNAL
};

static guint go_action_combo_pixmaps_signals [LAST_SIGNAL] = { 0, };

static GdkPixbuf *
make_icon (GtkAction *a, const char *stock_id, GtkWidget *tool)
{
	GtkIconSize size;
	GdkPixbuf *res;

	if (stock_id == NULL)
		return NULL;

	size = GTK_IS_TOOL_ITEM (tool)
		? gtk_tool_item_get_icon_size (GTK_TOOL_ITEM (tool))
		: GTK_ICON_SIZE_MENU;

	/* This should go away when we assume icon names.  */
	res = gtk_widget_render_icon_pixbuf (tool, stock_id, size);
	if (res)
		return res;

	return go_gtk_widget_render_icon_pixbuf (tool, stock_id, size);
}


static GObjectClass *combo_pixmaps_parent;
static void
go_action_combo_pixmaps_connect_proxy (GtkAction *a, GtkWidget *proxy)
{
	GTK_ACTION_CLASS (combo_pixmaps_parent)->connect_proxy (a, proxy);

	if (GTK_IS_IMAGE_MENU_ITEM (proxy)) { /* set the icon */
		GOActionComboPixmaps *paction = (GOActionComboPixmaps *)a;
		const char *stock_id = paction->elements[0].stock_id;
		GdkPixbuf *icon = make_icon (a, stock_id, proxy);
		if (icon) {
			GtkWidget *image = gtk_image_new_from_pixbuf (icon);
			g_object_unref (icon);
			gtk_widget_show (image);
			gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (proxy),
						       image);
		}
	}
}

static void
cb_selection_changed (GOComboPixmaps *combo, int id, GOActionComboPixmaps *paction)
{
	GSList *ptr;
	if (paction->updating_proxies)
		return;
	paction->selected_id = id;

	paction->updating_proxies = TRUE;
	ptr = gtk_action_get_proxies (GTK_ACTION (paction));
	for ( ; ptr != NULL ; ptr = ptr->next)
		if (GO_IS_COMBO_PIXMAPS (ptr->data) &&
		    go_combo_pixmaps_get_selected (ptr->data, NULL) != id)
			go_combo_pixmaps_select_id (ptr->data, id);
	paction->updating_proxies = FALSE;

	g_signal_emit_by_name (G_OBJECT (paction), "combo-activate");
	gtk_action_activate (GTK_ACTION (paction));
}

static void
cb_toolbar_reconfigured (GOToolComboPixmaps *tool, GtkAction *a)
{
	GtkOrientation o = gtk_tool_item_get_orientation (GTK_TOOL_ITEM (tool));
	GtkReliefStyle relief = gtk_tool_item_get_relief_style (GTK_TOOL_ITEM (tool));
	GOActionComboPixmaps *paction = (GOActionComboPixmaps *)a;
	GOActionComboPixmapsElement const *el = paction->elements;

	g_object_set (G_OBJECT (tool->combo),
		      "show-arrow", o == GTK_ORIENTATION_HORIZONTAL,
		      NULL);

	go_combo_pixmaps_clear_elements (tool->combo);
	for ( ; el->stock_id != NULL ; el++) {
		GdkPixbuf *icon = make_icon (a, el->stock_id, GTK_WIDGET (tool));
		/*
		 * FIXME FIXME FIXME: we must find a better solution for
		 * translating strings not in goffice domain
		 */
		go_combo_pixmaps_add_element (tool->combo,
					      icon,
					      el->id,
					      gettext (el->untranslated_tooltip));
	}
	go_combo_pixmaps_select_id (tool->combo, paction->selected_id);

	go_combo_box_set_relief (GO_COMBO_BOX (tool->combo), relief);
}

static GtkWidget *
go_action_combo_pixmaps_create_tool_item (GtkAction *a)
{
	GOActionComboPixmaps *paction = (GOActionComboPixmaps *)a;
	GOToolComboPixmaps *tool = g_object_new (GO_TYPE_TOOL_COMBO_PIXMAPS, NULL);
	GtkStyleContext *context;

	context = gtk_widget_get_style_context (GTK_WIDGET (tool));
        gtk_style_context_add_class (context, "pixmaps");

	tool->combo = go_combo_pixmaps_new (paction->ncols);

	g_signal_connect (tool, "toolbar-reconfigured",
			  G_CALLBACK (cb_toolbar_reconfigured),
			  a);
	cb_toolbar_reconfigured (tool, a);

	go_gtk_widget_disable_focus (GTK_WIDGET (tool->combo));
	gtk_container_add (GTK_CONTAINER (tool), GTK_WIDGET (tool->combo));
	gtk_widget_show (GTK_WIDGET (tool->combo));
	gtk_widget_show (GTK_WIDGET (tool));

	g_signal_connect (G_OBJECT (tool->combo),
		"changed",
		G_CALLBACK (cb_selection_changed), a);

	return GTK_WIDGET (tool);
}

static GtkWidget *
go_action_combo_pixmaps_create_menu_item (GtkAction *a)
{
	GOActionComboPixmaps *paction = (GOActionComboPixmaps *)a;
	GOMenuPixmaps *submenu = go_menu_pixmaps_new (paction->ncols);
	GOActionComboPixmapsElement const *el= paction->elements;
	GtkWidget *item = gtk_image_menu_item_new ();

	for ( ; el->stock_id != NULL ; el++)
		go_menu_pixmaps_add_element
			(submenu,
			 make_icon (a, el->stock_id, item),
			 el->id,
			 gettext (el->untranslated_tooltip));

	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), GTK_WIDGET (submenu));
	gtk_widget_show (GTK_WIDGET (submenu));
	g_signal_connect (G_OBJECT (submenu),
		"changed",
		G_CALLBACK (cb_selection_changed), a);
	return item;
}

static void
go_action_combo_pixmaps_finalize (GObject *obj)
{
	combo_pixmaps_parent->finalize (obj);
}

static void
go_action_combo_pixmaps_class_init (GtkActionClass *gtk_act_klass)
{
	GObjectClass *gobject_klass = (GObjectClass *)gtk_act_klass;

	combo_pixmaps_parent = g_type_class_peek_parent (gobject_klass);
	gobject_klass->finalize		= go_action_combo_pixmaps_finalize;

	gtk_act_klass->create_tool_item = go_action_combo_pixmaps_create_tool_item;
	gtk_act_klass->create_menu_item = go_action_combo_pixmaps_create_menu_item;
	gtk_act_klass->connect_proxy	= go_action_combo_pixmaps_connect_proxy;
	go_action_combo_pixmaps_signals [COMBO_ACTIVATE] =
		g_signal_new ("combo-activate",
			      G_OBJECT_CLASS_TYPE (gobject_klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GOActionComboPixmapsClass, combo_activate),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
}

GSF_CLASS (GOActionComboPixmaps, go_action_combo_pixmaps,
	   go_action_combo_pixmaps_class_init, NULL,
	   GTK_TYPE_ACTION)

GOActionComboPixmaps *
go_action_combo_pixmaps_new (char const *name,
			     GOActionComboPixmapsElement const *elements,
			     int ncols, int nrows)
{
	GOActionComboPixmaps *paction;

	g_return_val_if_fail (ncols > 0, NULL);
	g_return_val_if_fail (nrows > 0, NULL);
	g_return_val_if_fail (elements != NULL, NULL);

	paction = g_object_new (go_action_combo_pixmaps_get_type (),
				"name", name,
				NULL);
	paction->elements = elements;
	paction->ncols = ncols;
	paction->nrows = nrows;
	paction->selected_id = elements[0].id;

	return paction;
}

int
go_action_combo_pixmaps_get_selected (GOActionComboPixmaps *paction, int *indx)
{
	g_return_val_if_fail (GO_IS_ACTION_COMBO_PIXMAPS (paction), 0);

	return paction->selected_id;
}

gboolean
go_action_combo_pixmaps_select_id (GOActionComboPixmaps *paction, int id)
{
	gboolean res = TRUE;
	GSList *ptr = gtk_action_get_proxies (GTK_ACTION (paction));

	paction->selected_id = id;
	for ( ; ptr != NULL ; ptr = ptr->next)
		if (GO_IS_TOOL_COMBO_PIXMAPS (ptr->data))
			res |= go_combo_pixmaps_select_id (
				GO_TOOL_COMBO_PIXMAPS (ptr->data)->combo, id);

	return res;
}
