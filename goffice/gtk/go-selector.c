/*
 * go-selector.c :
 *
 * Copyright (C) 2006 Emmanuel Pacaud (emmanuel.pacaud@lapp.in2p3.fr)
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
#include <goffice/gtk/goffice-gtk.h>

#include "go-selector.h"

#include <glib/gi18n-lib.h>

#include <gdk/gdkkeysyms.h>

/**
 * GOSelectorClass:
 * @parent_class: parent class.
 **/

enum {
	GO_SELECTOR_ACTIVATE,
	GO_SELECTOR_LAST_SIGNAL
};

static guint 	go_selector_signals[GO_SELECTOR_LAST_SIGNAL] = {0,};

static void     go_selector_finalize   		(GObject *object);

static void 	go_selector_button_toggled 	(GtkWidget *button,
						 gpointer   data);
static gboolean go_selector_key_press 		(GtkWidget   *widget,
						 GdkEventKey *event,
						 gpointer     data);
static void 	go_selector_popdown 		(GOSelector *selector);
static void 	go_selector_popup		(GOSelector *selector);

static void 	go_selector_set_active_internal (GOSelector *selector,
						 int index,
						 gboolean is_auto);

struct _GOSelectorPrivate {
	GtkWidget *button;
	GtkWidget *box;
	GtkWidget *alignment;
	GtkWidget *swatch;
	GtkWidget *separator;
	GtkWidget *arrow;

	GOPalette *palette;

	gboolean 	 selected_is_auto;
	int	 	 selected_index;

	int		 		dnd_length;
	GtkTargetEntry	 		dnd_type;
	GOSelectorDndDataGet		dnd_data_get;
	GOSelectorDndDataReceived	dnd_data_received;
	GOSelectorDndFillIcon		dnd_fill_icon;
	gboolean			dnd_initialized;
};

G_DEFINE_TYPE (GOSelector, go_selector, GTK_TYPE_BOX)

static void
go_selector_init (GOSelector *selector)
{
	GOSelectorPrivate *priv;

	priv = G_TYPE_INSTANCE_GET_PRIVATE (selector, GO_TYPE_SELECTOR, GOSelectorPrivate);

	selector->priv = priv;

	priv->palette = NULL;
	priv->selected_index = 0;
	priv->selected_is_auto = FALSE;

	priv->button = gtk_toggle_button_new ();
	g_signal_connect (priv->button, "toggled",
			  G_CALLBACK (go_selector_button_toggled), selector);
	g_signal_connect_after (priv->button,
				"key_press_event",
				G_CALLBACK (go_selector_key_press), selector);
	gtk_widget_show (priv->button);

	priv->box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_container_add (GTK_CONTAINER (priv->button),
			   priv->box);
	gtk_widget_show (priv->box);

	priv->alignment = gtk_alignment_new (.5, .5, 1., 1.);
	gtk_alignment_set_padding (GTK_ALIGNMENT (priv->alignment), 0, 0, 0, 4);
	gtk_box_pack_start (GTK_BOX (priv->box),
			    priv->alignment, TRUE, TRUE, 0);
	gtk_widget_show (priv->alignment);

	priv->separator = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
	gtk_box_pack_start (GTK_BOX (priv->box),
			    priv->separator, FALSE, FALSE, 0);
	gtk_widget_show (priv->separator);

	priv->arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
	gtk_box_pack_start (GTK_BOX (priv->box),
			    priv->arrow, FALSE, FALSE, 0);
	gtk_widget_show (priv->arrow);

	gtk_box_pack_start (GTK_BOX (selector), priv->button, TRUE, TRUE, 0);

	priv->dnd_length = 0;
	priv->dnd_type.target = NULL;
	priv->dnd_type.flags = 0;
	priv->dnd_type.info = 0;
	priv->dnd_data_get = NULL;
	priv->dnd_data_received = NULL;
	priv->dnd_initialized = FALSE;
}

static void
go_selector_class_init (GOSelectorClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = go_selector_finalize;

	go_selector_signals[GO_SELECTOR_ACTIVATE] =
		g_signal_new ("activate",
			      G_OBJECT_CLASS_TYPE (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GOSelectorClass, activate),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	g_type_class_add_private (object_class, sizeof (GOSelectorPrivate));
}

static void
go_selector_finalize (GObject *object)
{
	GOSelectorPrivate *priv;

	priv = GO_SELECTOR (object)->priv;

	GO_SELECTOR (object)->priv = NULL; /* we need that to avoid side effects when destroying the palette */
	if (priv->palette)
		g_object_unref (priv->palette);
	g_free (priv->dnd_type.target);

	(* G_OBJECT_CLASS (go_selector_parent_class)->finalize) (object);
}

static void
cb_palette_activate (GOPalette *palette, int index, GOSelector *selector)
{
	go_selector_set_active_internal (selector, index, FALSE);
}

static void
cb_palette_automatic_activate (GOPalette *palette, int index, GOSelector *selector)
{
	go_selector_set_active_internal (selector, index, TRUE);
}

static void
cb_palette_deactivate (GOPalette *palette, GOSelector *selector)
{
	go_selector_popdown (selector);
}

/**
 * go_selector_new:
 * @palette: a #GOPalette
 *
 * Creates a new selector, using @palette. Selector button swatch will use
 * swatch render function of @palette.
 *
 * Returns: a new #GtkWidget.
 **/
GtkWidget *
go_selector_new (GOPalette *palette)
{
	GOSelectorPrivate *priv;
	GtkWidget *selector;

	selector = g_object_new (GO_TYPE_SELECTOR, NULL);

	g_return_val_if_fail (GO_IS_PALETTE (palette), selector);

	priv = GO_SELECTOR (selector)->priv;

	g_object_ref_sink (palette);

	priv->palette = palette;
	priv->swatch = go_palette_swatch_new (GO_PALETTE (palette), 0);
	gtk_container_add (GTK_CONTAINER (priv->alignment), priv->swatch);

	g_signal_connect (palette, "activate", G_CALLBACK(cb_palette_activate), selector);
	g_signal_connect (palette, "automatic-activate", G_CALLBACK(cb_palette_automatic_activate), selector);
	g_signal_connect (palette, "deactivate", G_CALLBACK(cb_palette_deactivate), selector);

	return selector;
}

static void
go_selector_popup (GOSelector *selector)
{
	GOSelectorPrivate *priv;

	g_return_if_fail (GO_IS_SELECTOR (selector));

	priv = selector->priv;

	if (!gtk_widget_get_realized (GTK_WIDGET (selector)))
		return;

	if (gtk_widget_get_mapped (GTK_WIDGET (priv->palette)))
		return;

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->button), TRUE);
	gtk_menu_popup (GTK_MENU (priv->palette),
			NULL, NULL,
			go_menu_position_below, selector,
			0, 0);
}

static void
go_selector_popdown (GOSelector *selector)
{
	GOSelectorPrivate *priv;

	g_return_if_fail (GO_IS_SELECTOR (selector));

	priv = selector->priv;

	if (priv) {
		gtk_menu_popdown (GTK_MENU (priv->palette));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->button), FALSE);
	}
}

static void
go_selector_button_toggled (GtkWidget *button,
				 gpointer   data)
{
	GOSelector *selector = GO_SELECTOR (data);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
		go_selector_popup (selector);
	else
		go_selector_popdown (selector);
}

static gboolean
go_selector_key_press (GtkWidget   *widget,
		       GdkEventKey *event,
		       gpointer     data)
{
	GOSelector *selector = GO_SELECTOR (data);
	GOSelectorPrivate *priv;
	guint state = event->state & gtk_accelerator_get_default_mod_mask ();
	int n_swatches, index;
	gboolean found = FALSE;

	priv = selector->priv;

	if ((event->keyval == GDK_KEY_Down || event->keyval == GDK_KEY_KP_Down) &&
	    state == GDK_MOD1_MASK) {
		go_selector_popup (selector);
		return TRUE;
	}

	if (state != 0)
		return FALSE;

	n_swatches = go_palette_get_n_swatches (GO_PALETTE (priv->palette));
	index = 0;

	switch (event->keyval)
	{
		case GDK_KEY_Down:
		case GDK_KEY_KP_Down:
			if (priv->selected_index < n_swatches - 1) {
				index = priv->selected_index + 1;
				found = TRUE;
				break;
			}
			/* else fall through */
		case GDK_KEY_Page_Up:
		case GDK_KEY_KP_Page_Up:
		case GDK_KEY_Home:
		case GDK_KEY_KP_Home:
			if (priv->selected_index != 0) {
				index = 0;
				found = TRUE;
			}
			break;

		case GDK_KEY_Up:
		case GDK_KEY_KP_Up:
			if (priv->selected_index  > 0) {
				index = priv->selected_index - 1;
				found = TRUE;
				break;
			}
			/* else fall through */
		case GDK_KEY_Page_Down:
		case GDK_KEY_KP_Page_Down:
		case GDK_KEY_End:
		case GDK_KEY_KP_End:
			if (priv->selected_index != n_swatches - 1) {
				index = n_swatches - 1;
				found = TRUE;
			}
			break;
		default:
			return FALSE;
	}

	if (found)
		go_selector_set_active_internal (selector, index, FALSE);

	return TRUE;
}

static void
go_selector_set_active_internal (GOSelector *selector, int index, gboolean is_auto)
{
	go_selector_popdown (selector);

	selector->priv->selected_index = index;
	selector->priv->selected_is_auto = is_auto;

	g_object_set_data (G_OBJECT (selector->priv->swatch), "index",
			   GINT_TO_POINTER (index));

	go_selector_update_swatch (selector);

	g_signal_emit (selector, go_selector_signals[GO_SELECTOR_ACTIVATE], 0);
}

/**
 * go_selector_set_active:
 * @selector: a #GOSelector
 * @index: new index
 *
 * Sets current selection index, and emits "activate" signal if
 * selection is actually changed.
 *
 * Returns: %TRUE if selection is actually changed.
 **/
gboolean
go_selector_set_active (GOSelector *selector, int index)
{
	int n_swatches;

	g_return_val_if_fail (GO_IS_SELECTOR (selector), FALSE);

	n_swatches = go_palette_get_n_swatches (GO_PALETTE (selector->priv->palette));

	if (index != selector->priv->selected_index &&
	    index >= 0 &&
	    index < n_swatches) {
		go_selector_set_active_internal (selector, index, FALSE);
		return TRUE;
	}
	return FALSE;
}

/**
 * go_selector_get_active:
 * @selector: a #GOSelector
 * @is_auto: boolean
 *
 * Retrieves current selection index, and set @is_auto to %TRUE if
 * current selection was set by clicking on automatic palette item.
 *
 * Returns: current index.
 **/
int
go_selector_get_active (GOSelector *selector, gboolean *is_auto)
{
	g_return_val_if_fail (GO_IS_SELECTOR (selector), 0);

	if (is_auto != NULL)
		*is_auto = selector->priv->selected_is_auto;
	return selector->priv->selected_index;
}

/**
 * go_selector_update_swatch:
 * @selector: a #GOSelector
 *
 * Requests a swatch update.
 **/
void
go_selector_update_swatch (GOSelector *selector)
{

	g_return_if_fail (GO_IS_SELECTOR (selector));

	gtk_widget_queue_draw (selector->priv->swatch);
}

/**
 * go_selector_activate:
 * @selector: a #GOSelector
 *
 * Updates slector swatch and emits an "activate" signal.
 **/
void
go_selector_activate (GOSelector *selector)
{
	g_return_if_fail (GO_IS_SELECTOR (selector));

	go_selector_update_swatch (selector);
	g_signal_emit (selector, go_selector_signals[GO_SELECTOR_ACTIVATE], 0);
}

/**
 * go_selector_get_user_data:
 * @selector: a #GOSelector
 *
 * A convenience function to access user_data of selector palette.
 * (See @go_palette_get_user_data).
 *
 * Returns: (transfer none): a pointer to palette user_data.
 **/
gpointer
go_selector_get_user_data (GOSelector *selector)
{
	g_return_val_if_fail (GO_IS_SELECTOR (selector), NULL);

	return go_palette_get_user_data (GO_PALETTE (selector->priv->palette));
}

static void
go_selector_drag_data_received (GtkWidget        *button,
				GdkDragContext   *context,
				gint              x,
				gint              y,
				GtkSelectionData *selection_data,
				guint             info,
				guint32           time,
				GOSelector       *selector)
{
	GOSelectorPrivate *priv = selector->priv;

	if (gtk_selection_data_get_length (selection_data) != priv->dnd_length ||
	    priv->dnd_data_received == NULL)
		return;

	(priv->dnd_data_received) (selector, gtk_selection_data_get_data (selection_data));
}

static void
go_selector_drag_begin (GtkWidget      *button,
			GdkDragContext *context,
			GOSelector     *selector)
{
	GOSelectorPrivate *priv = selector->priv;
	GdkPixbuf *pixbuf;

	pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE,
				 8, 48, 32);

	if (priv->dnd_fill_icon != NULL)
		(priv->dnd_fill_icon) (selector, pixbuf);
	else
		gdk_pixbuf_fill (pixbuf, 0);

	gtk_drag_set_icon_pixbuf (context, pixbuf, -2, -2);
	g_object_unref (pixbuf);
}

static void
go_selector_drag_data_get (GtkWidget        *button,
			   GdkDragContext   *context,
			   GtkSelectionData *selection_data,
			   guint             info,
			   guint             time,
			   GOSelector       *selector)
{
	GOSelectorPrivate *priv = selector->priv;
	gpointer data;

	if (priv->dnd_data_get == NULL)
		return;

	data = (priv->dnd_data_get) (selector);

	if (data != NULL) {
		gtk_selection_data_set (selection_data,
		                        gtk_selection_data_get_target (selection_data),
					8, data, priv->dnd_length);
		g_free (data);
	}
}

/**
 * go_selector_setup_dnd:
 * @selector: a #GOSelector
 * @dnd_target: drag and drop target type
 * @dnd_length: length of data transfered on drop
 * @data_get: (scope notified): a user provided data_get method
 * @data_received: (scope notified): a user provided data_received method
 * @fill_icon: (scope notified): a user function for dnd icon creation
 *
 * Setups drag and drop for @selector.
 **/
void
go_selector_setup_dnd (GOSelector *selector,
		       char const *dnd_target,
		       int dnd_length,
		       GOSelectorDndDataGet data_get,
		       GOSelectorDndDataReceived data_received,
		       GOSelectorDndFillIcon fill_icon)
{
	GOSelectorPrivate *priv;

	g_return_if_fail (GO_IS_SELECTOR (selector));

	priv = selector->priv;
	g_return_if_fail (!priv->dnd_initialized);
	g_return_if_fail (dnd_length > 0);
	g_return_if_fail (dnd_target != NULL);

	priv->dnd_length = dnd_length;
	priv->dnd_data_get = data_get;
	priv->dnd_data_received = data_received;
	priv->dnd_fill_icon = fill_icon;
	priv->dnd_type.target = g_strdup (dnd_target);

	gtk_drag_dest_set (priv->button,
			   GTK_DEST_DEFAULT_MOTION |
			   GTK_DEST_DEFAULT_HIGHLIGHT |
			   GTK_DEST_DEFAULT_DROP,
			   &priv->dnd_type, 1, GDK_ACTION_COPY);
	gtk_drag_source_set (priv->button,
			     GDK_BUTTON1_MASK|GDK_BUTTON3_MASK,
			     &priv->dnd_type, 1, GDK_ACTION_COPY);

	g_signal_connect (priv->button, "drag_begin",
			  G_CALLBACK (go_selector_drag_begin), selector);
	g_signal_connect (priv->button, "drag_data_received",
			  G_CALLBACK (go_selector_drag_data_received), selector);
	g_signal_connect (priv->button, "drag_data_get",
			  G_CALLBACK (go_selector_drag_data_get), selector);

	priv->dnd_initialized = TRUE;
}
