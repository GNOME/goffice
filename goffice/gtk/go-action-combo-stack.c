/*
 * go-action-combo-stack.c: A custom GtkAction to handle undo/redo menus/toolbars
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
#include "go-action-combo-stack.h"
#include "go-combo-box.h"
#include "goffice-gtk.h"

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>
#include <stdio.h>

/**************************************************************************/

enum {
	LABEL_COL,
	KEY_COL
};

/**************************************************************************/

typedef struct {
	GtkToolItem base;
	GtkWidget *combo; /* container has a ref, not us */
} GOToolComboStack;
typedef GtkToolItemClass GOToolComboStackClass;

#define GO_TYPE_TOOL_COMBO_STACK	(go_tool_combo_stack_get_type ())
#define GO_TOOL_COMBO_STACK(o)		(G_TYPE_CHECK_INSTANCE_CAST (o, GO_TYPE_TOOL_COMBO_STACK, GOToolComboStack))
#define GO_IS_TOOL_COMBO_STACK(o)	(G_TYPE_CHECK_INSTANCE_TYPE (o, GO_TYPE_TOOL_COMBO_STACK))

static GType go_tool_combo_stack_get_type (void);

static GSF_CLASS (GOToolComboStack, go_tool_combo_stack,
	   NULL, NULL,
	   GTK_TYPE_TOOL_ITEM)

/*****************************************************************************/

struct _GOActionComboStack {
	GtkAction	 base;
	GtkTreeModel	*model;
	int              last_selection;
};
typedef GtkActionClass GOActionComboStackClass;

static GObjectClass *combo_stack_parent;

static int
goacs_count (GOActionComboStack *action)
{
	return gtk_tree_model_iter_n_children (action->model, NULL);
}

static void
cb_combo_changed (GtkComboBoxText *combo, GOActionComboStack *action)
{
	/* YUCK
	 * YUCK
	 * YUCK
	 * We really need to return the key in "activate" but cannot for now.
	 * as a result people had better call
	 * 	go_action_combo_stack_selection
	 * from with the handler or they will lose the selection from toolitems.
	 * We cannot tell whether the activation was a menu or accelerator
	 * which just use the top.  */

	action->last_selection = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));
	if (action->last_selection >= 0)
		gtk_action_activate (GTK_ACTION (action));
}

static void
cb_button_clicked (GtkButton *button, GOActionComboStack *action)
{
	int n = goacs_count (action);
	if (n > 0) {
		action->last_selection = 0;
		gtk_action_activate (GTK_ACTION (action));
	} else
		action->last_selection = -1;
}

static void
cb_reconfig (GOToolComboStack *tool, GtkAction *a)
{
	GtkIconSize size = gtk_tool_item_get_icon_size (GTK_TOOL_ITEM (tool));
	GtkWidget *image, *button;
	GtkWidget *child;

	child = gtk_bin_get_child (GTK_BIN (tool->combo));
	if (child)
		gtk_container_remove (GTK_CONTAINER (tool->combo), child);

	image = gtk_action_create_icon (a, size);
	button = g_object_new (GTK_TYPE_BUTTON, "image", image, NULL);
	gtk_widget_show_all (button);
	gtk_container_add (GTK_CONTAINER (tool->combo), button);

	g_signal_connect (button, "clicked", G_CALLBACK (cb_button_clicked), a);
}

static GtkWidget *
go_action_combo_stack_create_tool_item (GtkAction *a)
{
	GOActionComboStack *saction = (GOActionComboStack *)a;
	GOToolComboStack *tool = g_object_new (GO_TYPE_TOOL_COMBO_STACK, NULL);
	gboolean is_sensitive = goacs_count (saction) > 0;
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	GtkStyleContext *context;

	context = gtk_widget_get_style_context (GTK_WIDGET (tool));
        gtk_style_context_add_class (context, "stack");

	tool->combo = gtk_combo_box_new_with_model (saction->model);
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (tool->combo), renderer, FALSE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (tool->combo),
				       renderer, "text", LABEL_COL);

	g_signal_connect (tool, "toolbar-reconfigured",
			  G_CALLBACK (cb_reconfig), a);
	cb_reconfig (tool, a);

	gtk_widget_set_sensitive (GTK_WIDGET (tool), is_sensitive);

	go_gtk_widget_disable_focus (tool->combo);
	gtk_container_add (GTK_CONTAINER (tool), tool->combo);
	gtk_widget_show (tool->combo);
	gtk_widget_show (GTK_WIDGET (tool));

	g_signal_connect_object (tool->combo,
				 "changed",
				 G_CALLBACK (cb_combo_changed), a, 0);

	return GTK_WIDGET (tool);
}

static GtkWidget *
go_action_combo_stack_create_menu_item (GtkAction *a)
{
	GOActionComboStack *saction = (GOActionComboStack *)a;
	GtkWidget *item = gtk_menu_item_new ();
	gboolean is_sensitive = goacs_count (saction) > 0;
	gtk_widget_set_sensitive (GTK_WIDGET (item), is_sensitive);
	return item;
}

static void
go_action_combo_stack_finalize (GObject *obj)
{
	GOActionComboStack *saction = (GOActionComboStack *)obj;
	g_object_unref (saction->model);
	saction->model = NULL;
	combo_stack_parent->finalize (obj);
}

static void
go_action_combo_stack_class_init (GtkActionClass *gtk_act_klass)
{
	GObjectClass *gobject_klass = (GObjectClass *)gtk_act_klass;

	combo_stack_parent = g_type_class_peek_parent (gobject_klass);

	gobject_klass->finalize = go_action_combo_stack_finalize;
	gtk_act_klass->create_tool_item = go_action_combo_stack_create_tool_item;
	gtk_act_klass->create_menu_item = go_action_combo_stack_create_menu_item;
}

static void
go_action_combo_stack_init (GOActionComboStack *saction)
{
	saction->model = (GtkTreeModel *)
		gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
	saction->last_selection = -1;
}

GSF_CLASS (GOActionComboStack, go_action_combo_stack,
	   go_action_combo_stack_class_init, go_action_combo_stack_init,
	   GTK_TYPE_ACTION)

static void
check_sensitivity (GOActionComboStack *saction, unsigned old_count)
{
	unsigned new_count = goacs_count (saction);

	if ((old_count > 0) ^ (new_count > 0)) {
		GSList *ptr = gtk_action_get_proxies (GTK_ACTION (saction));
		gboolean is_sensitive = (new_count > 0);
		for ( ; ptr != NULL ; ptr = ptr->next)
			gtk_widget_set_sensitive (ptr->data, is_sensitive);
	}
}

/**
 * go_action_combo_stack_push:
 * @act: #GOActionComboStack
 * @str: The label to push
 * @key: a key value to id the pushed item
 **/
void
go_action_combo_stack_push (GOActionComboStack *act,
			    char const *label, gpointer key)
{
	GOActionComboStack *saction = GO_ACTION_COMBO_STACK (act);
	GtkTreeIter iter;
	unsigned old_count = goacs_count (saction);

	g_return_if_fail (saction != NULL);

	gtk_list_store_insert (GTK_LIST_STORE (saction->model), &iter, 0);
	gtk_list_store_set (GTK_LIST_STORE (saction->model), &iter,
		LABEL_COL,	label,
		KEY_COL,	key,
		-1);
	check_sensitivity (saction, old_count);
}

/**
 * go_action_combo_stack_pop:
 * @act: #GOActionComboStack
 * @n: count
 *
 * Shorten list @act by removing @n off the top (or fewer if the list is
 * shorter)
 **/
void
go_action_combo_stack_pop (GOActionComboStack *act, unsigned n)
{
	GOActionComboStack *saction = GO_ACTION_COMBO_STACK (act);
	GtkTreeIter iter;
	unsigned old_count = goacs_count (saction);

	g_return_if_fail (saction != NULL);

	if (gtk_tree_model_iter_nth_child (saction->model, &iter, NULL, 0))
		while (n-- > 0 &&
		       gtk_list_store_remove (GTK_LIST_STORE (saction->model), &iter))
			;
	check_sensitivity (saction, old_count);
}

/**
 * go_action_combo_stack_truncate:
 * @act: #GOActionComboStack
 * @n: maximum length
 *
 * Ensure that list @act is no longer than @n, dropping any extra off the
 * bottom.
 **/
void
go_action_combo_stack_truncate (GOActionComboStack *act, unsigned n)
{
	GOActionComboStack *saction = GO_ACTION_COMBO_STACK (act);
	unsigned old_count;

	g_return_if_fail (saction != NULL);

	old_count = goacs_count (saction);
	if (old_count > n) {
		GtkTreeIter iter;
		if (gtk_tree_model_iter_nth_child (saction->model, &iter, NULL, n))
			while (gtk_list_store_remove (GTK_LIST_STORE (saction->model), &iter))
				;
		check_sensitivity (saction, old_count);
	}
}

/**
 * go_action_combo_stack_selection:
 * @a: #GOActionComboStack
 *
 * Returns: (transfer none): the key of the item last selected in one of the proxies.
 * 	Yes this interface is terrible, but we can't return the key in the
 * 	activate signal.
 *
 * NOTE: see writeup in cb_combo_changed.
 **/
gpointer
go_action_combo_stack_selection (GOActionComboStack const *a)
{
	GtkTreeIter iter;
	int i = MAX (0, a->last_selection);

	if (gtk_tree_model_iter_nth_child (a->model, &iter, NULL, i)) {
		gpointer key;
		gtk_tree_model_get (a->model, &iter, KEY_COL, &key, -1);
		return key;
	}

	return NULL;
}
