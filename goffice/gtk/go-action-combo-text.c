/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-action-combo-text .c: A custom GtkAction to handle lists in menus/toolbars
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
#include "go-action-combo-text.h"
#include "go-combo-box.h"
#include "go-combo-text.h"
#include "goffice-gtk.h"
#include <goffice/utils/go-glib-extras.h>

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

typedef struct {
	GtkToolItem	 base;
	GOComboText	*combo; /* container has a ref, not us */
} GOToolComboText;
typedef GtkToolItemClass GOToolComboTextClass;

#define GO_TYPE_TOOL_COMBO_TEXT		(go_tool_combo_text_get_type ())
#define GO_TOOL_COMBO_TEXT(o)		(G_TYPE_CHECK_INSTANCE_CAST (o, GO_TYPE_TOOL_COMBO_TEXT, GOToolComboText))
#define GO_IS_TOOL_COMBO_TEXT(o)	(G_TYPE_CHECK_INSTANCE_TYPE (o, GO_TYPE_TOOL_COMBO_TEXT))

static GType go_tool_combo_text_get_type (void);
#if 0
static void
go_tool_combo_text_finalize (GObject *obj)
{
	/* Call parent->finalize (obj).  */
}
static gboolean
go_tool_combo_text_create_menu_proxy (GtkToolItem *tool_item)
{
}
#endif

#ifndef HAVE_GTK_TOOL_ITEM_SET_TOOLTIP_TEXT
static gboolean
go_tool_combo_text_set_tooltip (GtkToolItem *tool_item, GtkTooltips *tooltips,
				char const *tip_text,
				char const *tip_private)
{
	GOToolComboText *self = (GOToolComboText *)tool_item;
	go_combo_box_set_tooltip (GO_COMBO_BOX (self->combo), tooltips,
				  tip_text, tip_private);
	return TRUE;
}
#endif

static void
go_tool_combo_text_class_init (GtkToolItemClass *tool_item_klass)
{
#if 0
	gobject_klass->finalize		   = go_tool_combo_text_finalize;
	tool_item_klass->create_menu_proxy = go_tool_combo_text_create_menu_proxy;
#endif
#ifndef HAVE_GTK_TOOL_ITEM_SET_TOOLTIP_TEXT
	tool_item_klass->set_tooltip	   = go_tool_combo_text_set_tooltip;
#endif
}

static GSF_CLASS (GOToolComboText, go_tool_combo_text,
	   go_tool_combo_text_class_init, NULL,
	   GTK_TYPE_TOOL_ITEM)

/*****************************************************************************/

struct _GOActionComboText {
	GtkAction	 base;
	GSList		*elements;
	char const 	*largest_elem;
	char		*entry_val;
	gboolean	 case_sensitive;
};
typedef struct {
	GtkActionClass	base;
} GOActionComboTextClass;

enum {
	PROP_0,
	PROP_CASE_SENSITIVE
};

static GObjectClass *combo_text_parent;

static void
set_entry_val (GOActionComboText *taction, char const *text)
{
	if (taction->entry_val != text) {
		g_free (taction->entry_val);
		taction->entry_val = g_strdup (text);
	}
}

static gint
g_strcase_equal (gconstpointer s1, gconstpointer s2)
{
	return !go_utf8_collate_casefold ((const gchar*) s1, (const gchar*) s2);
}

static gboolean
cb_entry_changed (GOComboText *ct, char const *text, GOActionComboText *taction)
{
	set_entry_val (taction, text);
	gtk_action_activate (GTK_ACTION (taction));
	return TRUE;
}

static GtkWidget *
go_action_combo_text_create_tool_item (GtkAction *act)
{
	GOActionComboText *taction = GO_ACTION_COMBO_TEXT (act);
	GOToolComboText *tool = g_object_new (GO_TYPE_TOOL_COMBO_TEXT, NULL);
	GSList *ptr;
	int w = -1;

	tool->combo = (GOComboText *)go_combo_text_new (taction->case_sensitive ? NULL : g_strcase_equal);
	if (taction->largest_elem != NULL)
		w = g_utf8_strlen (taction->largest_elem, -1);

	for (ptr = taction->elements; ptr != NULL ; ptr = ptr->next) {
		go_combo_text_add_item	(tool->combo, ptr->data);
		if (taction->largest_elem == NULL) {
			int tmp = g_utf8_strlen (ptr->data, -1);
			if (w < tmp)
				w = tmp;
		}
	}

	go_combo_box_set_title (GO_COMBO_BOX (tool->combo),
		_(gtk_action_get_name (act)));
	gtk_entry_set_width_chars (GTK_ENTRY (go_combo_text_get_entry (tool->combo)), w);
	g_object_set (G_OBJECT (tool), "visible-vertical", FALSE, NULL);

	go_combo_box_set_relief (GO_COMBO_BOX (tool->combo), GTK_RELIEF_NONE);
	gtk_container_add (GTK_CONTAINER (tool), GTK_WIDGET (tool->combo));
	gtk_widget_show (GTK_WIDGET (tool->combo));
	gtk_widget_show (GTK_WIDGET (tool));
	g_signal_connect (tool->combo,
		"entry_changed",
		G_CALLBACK (cb_entry_changed), taction);
	return GTK_WIDGET (tool);
}

static void
cb_menu_activated (GtkMenuItem *item, GOActionComboText *taction)
{
	const char *text = g_object_get_data (G_OBJECT (item), "text");
	set_entry_val (taction, text);
	gtk_action_activate (GTK_ACTION (taction));
}

static GtkWidget *
go_action_combo_text_create_menu_item (GtkAction *act)
{
	GOActionComboText *taction = GO_ACTION_COMBO_TEXT (act);
	GtkWidget *menu = gtk_menu_new ();
	GtkWidget *item = gtk_image_menu_item_new ();
	GSList *ptr;

	for (ptr = taction->elements; ptr != NULL ; ptr = ptr->next) {
		const char *text = ptr->data;
		GtkWidget *item = gtk_menu_item_new_with_label (text);
		g_object_set_data (G_OBJECT (item), "text", (gpointer)text);
		g_signal_connect (item, "activate",
				  G_CALLBACK (cb_menu_activated),
				  act);

		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	}

	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
	gtk_widget_show_all (item);
	return item;
}

static void
go_action_combo_text_finalize (GObject *obj)
{
	GOActionComboText *taction = GO_ACTION_COMBO_TEXT (obj);
	g_free (taction->entry_val);
	go_slist_free_custom (taction->elements, (GFreeFunc)g_free);
	combo_text_parent->finalize (obj);
}

static void
go_action_combo_text_set_property (GObject      *object,
				   guint         prop_id,
				   GValue const *value,
				   GParamSpec   *pspec)
{
	GOActionComboText *taction = GO_ACTION_COMBO_TEXT (object);

	switch (prop_id) {
	case PROP_CASE_SENSITIVE:
		taction->case_sensitive = g_value_get_boolean (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
go_action_combo_text_get_property (GObject      *object,
				   guint         prop_id,
				   GValue       *value,
				   GParamSpec   *pspec)
{
	GOActionComboText *taction = GO_ACTION_COMBO_TEXT (object);

	switch (prop_id) {
	case PROP_CASE_SENSITIVE:
		g_value_set_boolean (value, taction->case_sensitive);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
go_action_combo_text_class_init (GtkActionClass *gtk_act_klass)
{
	GObjectClass *gobject_klass = (GObjectClass *)gtk_act_klass;

	combo_text_parent = g_type_class_peek_parent (gobject_klass);
	gobject_klass->finalize		= go_action_combo_text_finalize;

	gtk_act_klass->create_tool_item = go_action_combo_text_create_tool_item;
	gtk_act_klass->create_menu_item = go_action_combo_text_create_menu_item;

	gobject_klass->set_property	= go_action_combo_text_set_property;
	gobject_klass->get_property	= go_action_combo_text_get_property;

	g_object_class_install_property (gobject_klass,
		PROP_CASE_SENSITIVE,
		g_param_spec_boolean ("case-sensitive", _("Case Sensitive"),
			_("Should the text comparison be case sensitive"),
			TRUE,
			G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
			GSF_PARAM_STATIC));
}

GSF_CLASS (GOActionComboText, go_action_combo_text,
	   go_action_combo_text_class_init, NULL,
	   GTK_TYPE_ACTION)

void
go_action_combo_text_add_item (GOActionComboText *taction, char const *item)
{
	taction->elements = g_slist_append (taction->elements, g_strdup (item));
}

void
go_action_combo_text_set_width (GOActionComboText *taction, char const *largest_elem)
{
	taction->largest_elem = largest_elem;
}

char const *
go_action_combo_text_get_entry (GOActionComboText const *a)
{
	return a->entry_val;
}

/**
 * go_action_combo_text_set_entry :
 * @taction : @GOActionComboText
 * @text : the new text
 * @dir : #GOActionComboTextSearchDir
 *
 * Set the entry of any toolbar proxies of @taction to @text.  Does not
 * generate an 'activate' signal.
 **/
void
go_action_combo_text_set_entry (GOActionComboText *taction, char const *text,
				GOActionComboTextSearchDir dir)
{
	GSList *ptr = gtk_action_get_proxies (GTK_ACTION (taction));

	set_entry_val (taction, text);
	for ( ; ptr != NULL ; ptr = ptr->next)
		if (GO_IS_TOOL_COMBO_TEXT (ptr->data))
			go_combo_text_set_text (GO_TOOL_COMBO_TEXT (ptr->data)->combo, text, dir);
}
