/*
 * go-action-combo-color.c: A custom GtkAction to handle color selection
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
#include "go-action-combo-color.h"

#include <gsf/gsf-impl-utils.h>

#include <glib/gi18n-lib.h>

typedef struct {
	GtkToolItem	 base;
	GOComboColor	*combo;	/* container has a ref, not us */
} GOToolComboColor;
typedef GtkToolItemClass GOToolComboColorClass;

#define GO_TYPE_TOOL_COMBO_COLOR	(go_tool_combo_color_get_type ())
#define GO_TOOL_COMBO_COLOR(o)		(G_TYPE_CHECK_INSTANCE_CAST (o, GO_TYPE_TOOL_COMBO_COLOR, GOToolComboColor))
#define GO_IS_TOOL_COMBO_COLOR(o)	(G_TYPE_CHECK_INSTANCE_TYPE (o, GO_TYPE_TOOL_COMBO_COLOR))

static GType go_tool_combo_color_get_type (void);

static GdkPixbuf *
make_icon (GtkAction *a, GtkWidget *tool)
{
	GtkIconSize size;
	char *stock_id;
	GdkPixbuf *icon;

	g_object_get (a, "stock-id", &stock_id, NULL);
	if (stock_id == NULL)
		return NULL;

	size = GTK_IS_TOOL_ITEM (tool)
		? gtk_tool_item_get_icon_size (GTK_TOOL_ITEM (tool))
		: GTK_ICON_SIZE_MENU;
	icon = go_gtk_widget_render_icon_pixbuf (tool, stock_id, size);
	g_free (stock_id);

	return icon;
}


static GSF_CLASS (GOToolComboColor, go_tool_combo_color,
	   NULL, NULL,
	   GTK_TYPE_TOOL_ITEM)

/*****************************************************************************/

struct _GOActionComboColor {
	GtkAction	 base;
	GOColorGroup 	*color_group;
	char		*default_val_label;
	GOColor		 default_val, current_color;
	gboolean	 allow_alpha;
};
typedef struct {
	GtkActionClass base;
	void (*display_custom_dialog) (GOActionComboColor *caction, GtkWidget *dialog);
	void (*combo_activate) (GOActionComboColor *caction);
} GOActionComboColorClass;

enum {
	DISPLAY_CUSTOM_DIALOG,
	COMBO_ACTIVATE,
	LAST_SIGNAL
};

static guint go_action_combo_color_signals [LAST_SIGNAL] = { 0, };
static GObjectClass *combo_color_parent;

static void
go_action_combo_color_connect_proxy (GtkAction *a, GtkWidget *proxy)
{
	GTK_ACTION_CLASS (combo_color_parent)->connect_proxy (a, proxy);

	if (GTK_IS_IMAGE_MENU_ITEM (proxy)) { /* set the icon */
		GdkPixbuf *icon = make_icon (a, proxy);
		if (icon) {
			GtkWidget *image = gtk_image_new_from_pixbuf (icon);
			g_object_unref (icon);

			gtk_widget_show (image);
			gtk_image_menu_item_set_image (
				GTK_IMAGE_MENU_ITEM (proxy), image);
		}
	}
}

static void
cb_color_changed (GtkWidget *cc, GOColor color,
		  gboolean is_custom, gboolean by_user, gboolean is_default,
		  GOActionComboColor *caction)
{
	if (!by_user)
		return;
	caction->current_color = is_default ? caction->default_val : color;

	g_signal_emit_by_name (G_OBJECT (caction), "combo-activate");
	gtk_action_activate (GTK_ACTION (caction));
}

static char *
get_title (GtkAction *a)
{
	char *res;
	g_object_get (G_OBJECT (a), "label", &res, NULL);
	return res;
}

static void
cb_proxy_custom_dialog (G_GNUC_UNUSED GObject *ignored,
			GtkWidget *dialog, GOActionComboColor *caction)
{
	g_signal_emit (caction,
		       go_action_combo_color_signals [DISPLAY_CUSTOM_DIALOG], 0,
		       dialog);
}

static void
cb_toolbar_reconfigured (GOToolComboColor *tool, GtkAction *a)
{
	GtkOrientation o = gtk_tool_item_get_orientation (GTK_TOOL_ITEM (tool));
	gboolean horiz = (o == GTK_ORIENTATION_HORIZONTAL);
	GtkReliefStyle relief = gtk_tool_item_get_relief_style (GTK_TOOL_ITEM (tool));
	GdkPixbuf *icon;

	g_object_set (G_OBJECT (tool->combo),
		      "show-arrow", horiz,
		      NULL);
	go_combo_color_set_instant_apply (GO_COMBO_COLOR (tool->combo), horiz);

	icon = make_icon (a, GTK_WIDGET (tool));
	go_combo_color_set_icon (GO_COMBO_COLOR (tool->combo), icon);
	if (icon) g_object_unref (icon);

	go_combo_box_set_relief (GO_COMBO_BOX (tool->combo), relief);
}

static GtkWidget *
go_action_combo_color_create_tool_item (GtkAction *a)
{
	GOActionComboColor *caction = (GOActionComboColor *)a;
	GOToolComboColor *tool = g_object_new (GO_TYPE_TOOL_COMBO_COLOR, NULL);
	char *title;
	GdkPixbuf *icon = make_icon (a, GTK_WIDGET (tool));

	tool->combo = (GOComboColor *)go_combo_color_new (icon,
		caction->default_val_label, caction->default_val,
		caction->color_group);
	if (icon) g_object_unref (icon);

	go_combo_color_set_allow_alpha (GO_COMBO_COLOR (tool->combo), caction->allow_alpha);
	title = get_title (a);
	go_combo_box_set_title (GO_COMBO_BOX (tool->combo), title);
	g_free (title);

	g_signal_connect (tool, "toolbar-reconfigured",
			  G_CALLBACK (cb_toolbar_reconfigured),
			  a);
	cb_toolbar_reconfigured (tool, a);

	go_gtk_widget_disable_focus (GTK_WIDGET (tool->combo));
	gtk_container_add (GTK_CONTAINER (tool), GTK_WIDGET (tool->combo));
	gtk_widget_show (GTK_WIDGET (tool->combo));
	gtk_widget_show (GTK_WIDGET (tool));

	g_object_connect (G_OBJECT (tool->combo),
		"signal::color_changed", G_CALLBACK (cb_color_changed), a,
		"signal::display-custom-dialog", G_CALLBACK (cb_proxy_custom_dialog), a,
		NULL);
	return GTK_WIDGET (tool);
}

static GtkWidget *
go_action_combo_color_create_menu_item (GtkAction *a)
{
	GOActionComboColor *caction = (GOActionComboColor *)a;
	char * title = get_title (a);
	GtkWidget *submenu = go_color_palette_make_menu (
		caction->default_val_label,
		caction->default_val,
		caction->color_group, title, caction->current_color);
	GtkWidget *item = gtk_image_menu_item_new ();

	g_free (title);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);
	gtk_widget_show (submenu);

	g_object_connect (G_OBJECT (submenu),
		"signal::color_changed", G_CALLBACK (cb_color_changed), a,
		"signal::display-custom-dialog", G_CALLBACK (cb_proxy_custom_dialog), a,
		NULL);
	return item;
}

static void
go_action_combo_color_finalize (GObject *obj)
{
	GOActionComboColor *color = (GOActionComboColor *)obj;
	if (color->color_group != NULL)
		g_object_unref (color->color_group);
	g_free (color->default_val_label);

	combo_color_parent->finalize (obj);
}

static void
go_action_combo_color_class_init (GtkActionClass *gtk_act_class)
{
	GObjectClass *gobject_class = (GObjectClass *)gtk_act_class;

	combo_color_parent = g_type_class_peek_parent (gobject_class);
	gobject_class->finalize		= go_action_combo_color_finalize;

	gtk_act_class->create_tool_item = go_action_combo_color_create_tool_item;
	gtk_act_class->create_menu_item = go_action_combo_color_create_menu_item;
	gtk_act_class->connect_proxy	= go_action_combo_color_connect_proxy;

	go_action_combo_color_signals [DISPLAY_CUSTOM_DIALOG] =
		g_signal_new ("display-custom-dialog",
			      G_OBJECT_CLASS_TYPE (gobject_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GOActionComboColorClass, display_custom_dialog),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1, G_TYPE_OBJECT);
	go_action_combo_color_signals [COMBO_ACTIVATE] =
		g_signal_new ("combo-activate",
			      G_OBJECT_CLASS_TYPE (gobject_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GOActionComboColorClass, combo_activate),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
}

GSF_CLASS (GOActionComboColor, go_action_combo_color,
	   go_action_combo_color_class_init, NULL,
	   GTK_TYPE_ACTION)

GOActionComboColor *
go_action_combo_color_new (char const  *action_name,
			   char const  *stock_id,
			   char const  *default_color_label,
			   GOColor	default_color,
			   gpointer	group_key)
{
	GOActionComboColor *res =
		g_object_new (go_action_combo_color_get_type (),
			      "name", action_name,
			      "stock-id", stock_id,
			      NULL);
	res->color_group = go_color_group_fetch (action_name, group_key);
	res->default_val_label = g_strdup (default_color_label);
	res->current_color = res->default_val = default_color;

	return res;
}

void
go_action_combo_color_set_group (GOActionComboColor *action, gpointer group_key)
{
/* FIXME FIXME FIXME TODO */
}

GOColor
go_action_combo_color_get_color (GOActionComboColor *a, gboolean *is_default)
{
	if (is_default != NULL)
		*is_default = (a->current_color == a->default_val);
	return a->current_color;
}

void
go_action_combo_color_set_color (GOActionComboColor *a, GOColor color)
{
	GSList *ptr = gtk_action_get_proxies (GTK_ACTION (a));

	a->current_color = color;
	for ( ; ptr != NULL ; ptr = ptr->next)
		if (GO_IS_TOOL_COMBO_COLOR (ptr->data))
			go_combo_color_set_color (GO_TOOL_COMBO_COLOR (ptr->data)->combo, color);
}

void
go_action_combo_color_set_allow_alpha (GOActionComboColor *a, gboolean allow_alpha)
{
	GSList *ptr = gtk_action_get_proxies (GTK_ACTION (a));

	a->allow_alpha = allow_alpha;
	for ( ; ptr != NULL ; ptr = ptr->next)
		if (GO_IS_TOOL_COMBO_COLOR (ptr->data))
			go_combo_color_set_allow_alpha (GO_TOOL_COMBO_COLOR (ptr->data)->combo, allow_alpha);
}

