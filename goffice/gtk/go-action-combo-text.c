/*
 * go-action-combo-text .c: A custom GtkAction to handle lists in menus/toolbars
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
#include "go-action-combo-text.h"
#include "goffice-gtk.h"
#include <goffice/utils/go-glib-extras.h>

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

/**
 * GOActionComboTextSearchDir:
 * @GO_ACTION_COMBO_SEARCH_FROM_TOP: search from the top of the list.
 * @GO_ACTION_COMBO_SEARCH_CURRENT: search from the current selection.
 * @GO_ACTION_COMBO_SEARCH_NEXT: search from the next element after current.
 **/

/*****************************************************************************/

struct _GOActionComboText {
	GtkAction	 base;
	GSList		*elements;
	char	 	*largest_elem;
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

static gboolean
cb_entry_acticated (GtkEntry *entry, GOActionComboText *taction)
{
	const char *text = gtk_entry_get_text (entry);
	set_entry_val (taction, text);
	gtk_action_activate (GTK_ACTION (taction));
	return TRUE;
}

static void
cb_combo_changed (GtkComboBoxText *combo, GOActionComboText *taction)
{
	GtkWidget *entry = gtk_bin_get_child (GTK_BIN (combo));
	/*
	 * We get the changed signal also when someone is typing in the
	 * entry.  We don't want that, so if the entry has focus, ignore
	 * the signal.  The entry will send the activate signal itself
	 * when Enter is pressed.
	 */
	if (entry && !gtk_widget_has_focus (entry))
		cb_entry_acticated (GTK_ENTRY (entry), taction);
}

static GtkWidget *
go_action_combo_text_create_tool_item (GtkAction *act)
{
	GOActionComboText *taction = GO_ACTION_COMBO_TEXT (act);
	GtkToolItem *tool = g_object_new
		(GTK_TYPE_TOOL_ITEM,
		 "visible-vertical", FALSE,
		 NULL);
	GtkComboBoxText *combo = (GtkComboBoxText *)
		gtk_combo_box_text_new_with_entry ();
	GtkEntry *entry;
	GSList *ptr;
	int w = -1;
	GtkStyleContext *context;

	context = gtk_widget_get_style_context (GTK_WIDGET (tool));
        gtk_style_context_add_class (context, "text");

	if (taction->largest_elem != NULL)
		w = g_utf8_strlen (taction->largest_elem, -1);

	for (ptr = taction->elements; ptr != NULL ; ptr = ptr->next) {
		const char *s = ptr->data;
		gtk_combo_box_text_append_text (combo, s);
		if (taction->largest_elem == NULL) {
			int tmp = g_utf8_strlen (s, -1);
			if (w < tmp)
				w = tmp;
		}
	}

	gtk_combo_box_set_title (GTK_COMBO_BOX (combo),
				 _(gtk_action_get_name (act)));
	entry = GTK_ENTRY (gtk_bin_get_child (GTK_BIN (combo)));
	gtk_entry_set_width_chars (entry, w);
	gtk_container_add (GTK_CONTAINER (tool), GTK_WIDGET (combo));
	gtk_widget_show_all (GTK_WIDGET (tool));
	g_signal_connect_object (entry,
				 "activate",
				 G_CALLBACK (cb_entry_acticated), taction, 0);

	g_signal_connect_object (combo,
				 "changed",
				 G_CALLBACK (cb_combo_changed), taction, 0);

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
	GtkWidget *item = gtk_menu_item_new ();
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
	g_free (taction->largest_elem);
	g_free (taction->entry_val);
	g_slist_free_full (taction->elements, (GDestroyNotify)g_free);
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
	g_free (taction->largest_elem);
	taction->largest_elem = g_strdup (largest_elem);
}

char const *
go_action_combo_text_get_entry (GOActionComboText const *a)
{
	return a->entry_val;
}

/**
 * go_action_combo_text_set_entry:
 * @taction: @GOActionComboText
 * @text: the new text
 * @dir: #GOActionComboTextSearchDir
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
	for ( ; ptr != NULL ; ptr = ptr->next) {
		GObject *proxy = ptr->data;
		GtkWidget *combo = GTK_IS_TOOL_ITEM (proxy)
			? gtk_bin_get_child (GTK_BIN (proxy))
			: NULL;
		GtkWidget *entry = combo && GTK_IS_COMBO_BOX_TEXT (combo)
			? gtk_bin_get_child (GTK_BIN (combo))
			: NULL;
		if (entry && GTK_IS_ENTRY (entry)) {
			gtk_action_block_activate (GTK_ACTION (taction));
			gtk_entry_set_text (GTK_ENTRY (entry), text);
			gtk_action_unblock_activate (GTK_ACTION (taction));
		}
	}
}
