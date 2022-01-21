/*
 * go-combo-pixmaps.c - A pixmap selector combo box
 * Copyright 2000-2004, Ximian, Inc.
 *
 * Authors:
 *   Jody Goldberg <jody@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) version 3.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA.
 */

#include <goffice/goffice-config.h>
#include "go-combo-pixmaps.h"
#include "go-combo-box.h"
#include <goffice/gtk/goffice-gtk.h>

#include <gdk/gdkkeysyms.h>

#include <glib/gi18n-lib.h>

#include <gsf/gsf-impl-utils.h>

#define PIXMAP_PREVIEW_WIDTH 15
#define PIXMAP_PREVIEW_HEIGHT 15

struct _GOComboPixmaps {
	GOComboBox parent;

	int selected_index;
	int cols;
	GArray *elements;

	GtkWidget *grid, *preview_button;
	GtkWidget *preview_image;
};

typedef struct {
	GOComboBoxClass base;
	void (*changed) (GOComboPixmaps *pixmaps, int id);
} GOComboPixmapsClass;

enum {
	CHANGED,
	LAST_SIGNAL
};

typedef struct {
	GdkPixbuf *pixbuf;
	int	   id;
} Element;

static guint go_combo_pixmaps_signals[LAST_SIGNAL] = { 0, };
static GObjectClass *go_combo_pixmaps_parent_class;

static void
go_combo_pixmaps_finalize (GObject *object)
{
	GOComboPixmaps *combo = GO_COMBO_PIXMAPS (object);

	g_clear_object (&combo->grid);

	if (combo->elements) {
		g_array_free (combo->elements, TRUE);
		combo->elements = NULL;
	}

	go_combo_pixmaps_parent_class->finalize (object);
}

static void
go_combo_pixmaps_screen_changed (GtkWidget *widget, G_GNUC_UNUSED GdkScreen *prev)
{
	GOComboPixmaps *combo = GO_COMBO_PIXMAPS (widget);
	GdkScreen *screen = gtk_widget_get_screen (widget);
	GtkWidget *toplevel = gtk_widget_get_toplevel (combo->grid);
	gtk_window_set_screen (GTK_WINDOW (toplevel), screen);
}

static void
emit_change (GOComboPixmaps *combo)
{
	if (_go_combo_is_updating (GO_COMBO_BOX (combo)))
		return;
	g_signal_emit (combo, go_combo_pixmaps_signals[CHANGED], 0,
		g_array_index (combo->elements, Element, combo->selected_index).id);
	go_combo_box_popup_hide (GO_COMBO_BOX (combo));
}

static void
cb_preview_clicked (GOComboPixmaps *combo)
{
	gboolean show_arrow;

	if (_go_combo_is_updating (GO_COMBO_BOX (combo)))
		return;

	g_object_get (G_OBJECT (combo), "show-arrow", &show_arrow, NULL);
	if (show_arrow)
		emit_change (combo);
	else {
		/* Condensed mode. */
		go_combo_box_popup_display (GO_COMBO_BOX (combo));
	}
}

static void
go_combo_pixmaps_init (GOComboPixmaps *combo)
{
	combo->elements = g_array_new (FALSE, FALSE, sizeof (Element));
	combo->grid = g_object_ref (gtk_grid_new ());

	combo->preview_button = gtk_toggle_button_new ();
	gtk_widget_set_name  (combo->preview_button, "preview");
	combo->preview_image = gtk_image_new ();
	gtk_container_add (GTK_CONTAINER (combo->preview_button),
			   GTK_WIDGET (combo->preview_image));

	g_signal_connect_swapped (combo->preview_button,
		"clicked",
		G_CALLBACK (cb_preview_clicked), combo);

	gtk_widget_show_all (combo->preview_button);
	gtk_widget_show_all (combo->grid);
	go_combo_box_construct (GO_COMBO_BOX (combo),
				combo->preview_button, combo->grid, combo->grid);
}

static void
go_combo_pixmaps_class_init (GObjectClass *gobject_class)
{
	GtkWidgetClass *wclass = (GtkWidgetClass *)gobject_class;

	go_combo_pixmaps_parent_class = g_type_class_ref (GO_TYPE_COMBO_BOX);
	gobject_class->finalize = go_combo_pixmaps_finalize;

	wclass->screen_changed = go_combo_pixmaps_screen_changed;

	go_combo_pixmaps_signals[CHANGED] =
		g_signal_new ("changed",
			      G_OBJECT_CLASS_TYPE (gobject_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GOComboPixmapsClass, changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE, 1, G_TYPE_INT);
}

GSF_CLASS (GOComboPixmaps, go_combo_pixmaps,
	   go_combo_pixmaps_class_init, go_combo_pixmaps_init,
	   GO_TYPE_COMBO_BOX)

GOComboPixmaps *
go_combo_pixmaps_new (int ncols)
{
	GOComboPixmaps *combo;

	g_return_val_if_fail (ncols > 0, NULL);

	combo = g_object_new (GO_TYPE_COMBO_PIXMAPS, NULL);
	combo->cols = ncols;
	return combo;
}

static gboolean
swatch_activated (GOComboPixmaps *combo, GtkWidget *button)
{
	go_combo_pixmaps_select_index (combo,
		GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "ItemIndex")));
	emit_change (combo);
	return TRUE;
}

static gboolean
cb_swatch_release_event (GtkWidget *button, GdkEventButton *event, GOComboPixmaps *combo)
{
/* FIXME FIXME FIXME TODO do I want to check for which button ? */
	return swatch_activated (combo, button);
}
static gboolean
cb_swatch_key_press (GtkWidget *button, GdkEventKey *event, GOComboPixmaps *combo)
{
	switch (event->keyval) {
	case GDK_KEY_Return:
	case GDK_KEY_KP_Enter:
	case GDK_KEY_space:
		return swatch_activated (combo, button);
	default:
		return FALSE;
	}
}

/**
 * go_combo_pixmaps_clear_elements:
 * @combo: #GOComboPixmaps
 *
 * Remove all elements.
 */
void
go_combo_pixmaps_clear_elements (GOComboPixmaps *combo)
{
	GList *children, *l;

	children = gtk_container_get_children (GTK_CONTAINER (combo->grid));
	for (l = children; l; l = l->next) {
		GtkWidget *child = l->data;
		gtk_container_remove (GTK_CONTAINER (combo->grid), child);
	}
	g_list_free (children);

	g_array_set_size (combo->elements, 0);
}


/**
 * go_combo_pixmaps_add_element:
 * @combo: #GOComboPixmaps
 * @pixbuf: (transfer full): #GdkPixbuf
 * @id: an identifier for the callbacks
 * @tooltip: optional
 *
 **/
void
go_combo_pixmaps_add_element (GOComboPixmaps *combo,
			      GdkPixbuf *pixbuf, int id, char const *tooltip)
{
	GtkWidget *button, *box;
	Element tmp;
	int item_index, col, row;

	g_return_if_fail (GO_IS_COMBO_PIXMAPS (combo));

	item_index = combo->elements->len;
	row = item_index / combo->cols;
	col = item_index % combo->cols;

	/* Wrap inside a vbox with a border so that we can see the focus indicator */
	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_start (GTK_BOX (box),
			    gtk_image_new_from_pixbuf (pixbuf),
			    TRUE, TRUE, 0);
	g_object_unref (pixbuf);

	tmp.pixbuf = pixbuf;
	tmp.id = id;
	g_array_append_val (combo->elements, tmp);

	button = gtk_button_new ();
	gtk_container_set_border_width (GTK_CONTAINER (box), 2);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	gtk_container_add (GTK_CONTAINER (button), box);
	g_object_set_data (G_OBJECT (button), "ItemIndex", GINT_TO_POINTER (item_index));
	if (tooltip != NULL)
		gtk_widget_set_tooltip_text (button, tooltip);
	gtk_widget_show_all (button);
	g_object_connect (button,
		"signal::button_release_event", G_CALLBACK (cb_swatch_release_event), combo,
		"signal::key_press_event", G_CALLBACK (cb_swatch_key_press), combo,
		NULL);
	gtk_grid_attach (GTK_GRID (combo->grid), button,
			 col, row + 1, 1, 1);
}

gboolean
go_combo_pixmaps_select_index (GOComboPixmaps *combo, int i)
{
	g_return_val_if_fail (GO_IS_COMBO_PIXMAPS (combo), FALSE);
	g_return_val_if_fail (0 <= i, FALSE);
	g_return_val_if_fail (i < (int)combo->elements->len, FALSE);

	combo->selected_index = i;
	gtk_image_set_from_pixbuf (GTK_IMAGE (combo->preview_image),
		g_array_index (combo->elements, Element, i).pixbuf);

	return TRUE;
}

gboolean
go_combo_pixmaps_select_id (GOComboPixmaps *combo, int id)
{
	unsigned i;

	g_return_val_if_fail (GO_IS_COMBO_PIXMAPS (combo), FALSE);

	for (i = 0 ; i < combo->elements->len ; i++)
		if (g_array_index (combo->elements, Element, i).id == id)
			break;

	g_return_val_if_fail (i <combo->elements->len, FALSE);

	combo->selected_index = i;
	gtk_image_set_from_pixbuf (GTK_IMAGE (combo->preview_image),
		g_array_index (combo->elements, Element, i).pixbuf);

	return TRUE;
}

int
go_combo_pixmaps_get_selected (GOComboPixmaps const *combo, int *index)
{
	Element *el;

	g_return_val_if_fail (GO_IS_COMBO_PIXMAPS (combo), 0);
	el = &g_array_index (combo->elements, Element, combo->selected_index);

	if (index != NULL)
		*index = combo->selected_index;
	return el->id;
}

/**
 * go_combo_pixmaps_get_preview:
 * @combo: #GOComboPixmaps
 *
 * Returns: (transfer none): the preview button.
 **/
GtkWidget *
go_combo_pixmaps_get_preview (GOComboPixmaps const *combo)
{
	g_return_val_if_fail (GO_IS_COMBO_PIXMAPS (combo), NULL);
	return combo->preview_button;
}

/************************************************************************/

struct _GOMenuPixmaps {
	GtkMenu base;
	unsigned cols, n;
};
typedef struct {
	GtkMenuClass base;
	void (*changed) (GOMenuPixmaps *pixmaps, int id);
} GOMenuPixmapsClass;

static guint go_menu_pixmaps_signals[LAST_SIGNAL] = { 0, };
static void
go_menu_pixmaps_class_init (GObjectClass *gobject_class)
{
	go_menu_pixmaps_signals[CHANGED] =
		g_signal_new ("changed",
			      G_OBJECT_CLASS_TYPE (gobject_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GOMenuPixmapsClass, changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE, 1, G_TYPE_INT);
}

GSF_CLASS (GOMenuPixmaps, go_menu_pixmaps,
		  go_menu_pixmaps_class_init, NULL,
		  GTK_TYPE_MENU)

GOMenuPixmaps *
go_menu_pixmaps_new (int ncols)
{
        GOMenuPixmaps *submenu = g_object_new (go_menu_pixmaps_get_type (), NULL);
	submenu->cols = ncols;
	submenu->n = 0;
	gtk_widget_show (GTK_WIDGET (submenu));
	return submenu;
}

static void
cb_menu_item_activate (GtkWidget *button, GtkWidget *menu)
{
	int id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "ItemID"));
	g_signal_emit (menu, go_menu_pixmaps_signals[CHANGED], 0, id);
}

void
go_menu_pixmaps_add_element (GOMenuPixmaps *menu,
			     GdkPixbuf *pixbuf, int id,
			     const char *tooltip)
{
        GtkWidget *button;
	int col, row;

	col = menu->n++;
	row = col / menu->cols;
	col = col % menu->cols;

	button = gtk_menu_item_new ();
	gtk_container_add (GTK_CONTAINER (button),
		gtk_image_new_from_pixbuf (pixbuf));
	g_object_unref ((GdkPixbuf *)pixbuf);
	g_object_set_data (G_OBJECT (button),
		"ItemID", GINT_TO_POINTER (id));
	gtk_widget_show_all (button);
	gtk_menu_attach (GTK_MENU (menu), button,
		col, col + 1, row + 1, row + 2);
	g_signal_connect (G_OBJECT (button),
		"activate",
		G_CALLBACK (cb_menu_item_activate), menu);

	if (tooltip != NULL)
		gtk_widget_set_tooltip_text (button, tooltip);
}
