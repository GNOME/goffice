/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gnm-combo-box.c - a customizable combobox
 * Copyright 2000, 2001, Ximian, Inc.
 *
 * Authors:
 *   Miguel de Icaza (miguel@gnu.org)
 *   Adrian E Feiguin (feiguin@ifir.edu.ar)
 *   Paolo Molnaro (lupus@debian.org).
 *   Jon K Hellan (hellan@acm.org)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License, version 2, as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <gnumeric-config.h>
#include <gdk/gdkkeysyms.h>
#include "gnm-combo-box.h"
#include <gnm-marshalers.h>
#include <gsf/gsf-impl-utils.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtktearoffmenuitem.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkarrow.h>
#include <gtk/gtkeventbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkframe.h>

static GObjectClass *gnm_combo_box_parent_class;

static void gnm_combo_popup_tear_off (GnmComboBox *combo,
				      gboolean set_position);
static void gnm_combo_set_tearoff_state (GnmComboBox *combo,
					 gboolean torn_off);
static void gnm_combo_popup_reparent (GtkWidget *popup, GtkWidget *new_parent,
				      gboolean unrealize);
static gboolean cb_popup_delete (GtkWidget *w, GdkEventAny *event,
			     GnmComboBox *combo);
static void gnm_combo_tearoff_bg_copy (GnmComboBox *combo);

enum {
	POP_DOWN_WIDGET,
	POP_DOWN_DONE,
	PRE_POP_DOWN,
	POST_POP_HIDE,
	LAST_SIGNAL
};

static guint gnm_combo_box_signals [LAST_SIGNAL] = { 0, };

struct _GnmComboBoxPrivate {
	GtkWidget *pop_down_widget;
	GtkWidget *display_widget;

	/*
	 * Internal widgets used to implement the ComboBox
	 */
	GtkWidget *frame;
	GtkWidget *arrow_button;

	GtkWidget *toplevel;	/* Popup's toplevel when not torn off */
	GtkWidget *tearoff_window; /* Popup's toplevel when torn off */
	gboolean torn_off;

	GtkWidget *tearable;	/* The tearoff "button" */
	GtkWidget *popup;	/* Popup */

	gboolean   updating_buttons;
};

static void
gnm_combo_box_finalize (GObject *object)
{
	GnmComboBox *combo_box = GNM_COMBO_BOX (object);

	g_free (combo_box->priv);

	gnm_combo_box_parent_class->finalize (object);
}

static void
gnm_combo_box_destroy (GtkObject *object)
{
	GtkObjectClass *klass = (GtkObjectClass *)gnm_combo_box_parent_class;
	GnmComboBox *combo_box = GNM_COMBO_BOX (object);

	if (combo_box->priv->toplevel) {
		gtk_object_destroy (GTK_OBJECT (combo_box->priv->toplevel));
		combo_box->priv->toplevel = NULL;
	}

	if (combo_box->priv->tearoff_window) {
		gtk_object_destroy (GTK_OBJECT (combo_box->priv->tearoff_window));
		combo_box->priv->tearoff_window = NULL;
	}

	if (klass->destroy)
                klass->destroy (object);
}

static gboolean
gnm_combo_box_mnemonic_activate (GtkWidget *w, gboolean group_cycling)
{
	GnmComboBox *combo_box = GNM_COMBO_BOX (w);
	gtk_toggle_button_set_active (
		GTK_TOGGLE_BUTTON (combo_box->priv->arrow_button), TRUE);
	return TRUE;
}

static void
gnm_combo_box_class_init (GObjectClass *object_class)
{
	GtkWidgetClass *widget_class = (GtkWidgetClass *)object_class;
	gnm_combo_box_parent_class = g_type_class_peek_parent (object_class);

	object_class->finalize = gnm_combo_box_finalize;
	widget_class->mnemonic_activate = gnm_combo_box_mnemonic_activate;
	((GtkObjectClass *)object_class)->destroy = gnm_combo_box_destroy;

	gnm_combo_box_signals [POP_DOWN_WIDGET] = g_signal_new (
		"pop_down_widget",
		G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GnmComboBoxClass, pop_down_widget),
		NULL, NULL,
		gnm__POINTER__VOID,
		G_TYPE_POINTER, 0, G_TYPE_NONE);

	gnm_combo_box_signals [POP_DOWN_DONE] = g_signal_new (
		"pop_down_done",
		G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GnmComboBoxClass, pop_down_done),
		NULL, NULL,
		gnm__BOOLEAN__OBJECT,
		G_TYPE_BOOLEAN, 1, G_TYPE_OBJECT);

	gnm_combo_box_signals [PRE_POP_DOWN] = g_signal_new (
		"pre_pop_down",
		G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GnmComboBoxClass, pre_pop_down),
		NULL, NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);

	gnm_combo_box_signals [POST_POP_HIDE] = g_signal_new (
		"post_pop_hide",
		G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GnmComboBoxClass, post_pop_hide),
		NULL, NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
}

gboolean
_gnm_combo_is_updating (GnmComboBox const *combo_box)
{
	return combo_box->priv->updating_buttons;
}

static void
set_arrow_state (GnmComboBox *combo_box, gboolean state)
{
	GnmComboBoxPrivate *priv = combo_box->priv;
	g_return_if_fail (!combo_box->priv->updating_buttons);

	combo_box->priv->updating_buttons = TRUE;
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->arrow_button), state);
	if (GTK_IS_TOGGLE_BUTTON (priv->display_widget))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->display_widget), state);
	combo_box->priv->updating_buttons = FALSE;
}

/* Cut and paste from gtkwindow.c */
static void
do_focus_change (GtkWidget *widget, gboolean in)
{
	GdkEventFocus fevent;

	g_object_ref (widget);

	if (in)
		GTK_WIDGET_SET_FLAGS (widget, GTK_HAS_FOCUS);
	else
		GTK_WIDGET_UNSET_FLAGS (widget, GTK_HAS_FOCUS);

	fevent.type = GDK_FOCUS_CHANGE;
	fevent.window = widget->window;
	fevent.in = in;

	gtk_widget_event (widget, (GdkEvent *)&fevent);

	g_object_notify (G_OBJECT (widget), "has_focus");

	g_object_unref (widget);
}

/**
 * gnm_combo_box_popup_hide_unconditional
 * @combo_box:  Combo box
 *
 * Hide popup, whether or not it is torn off.
 */
static void
gnm_combo_box_popup_hide_unconditional (GnmComboBox *combo_box)
{
	gboolean popup_info_destroyed = FALSE;

	g_return_if_fail (combo_box != NULL);
	g_return_if_fail (IS_GNM_COMBO_BOX (combo_box));

	gtk_widget_hide (combo_box->priv->toplevel);
	gtk_widget_hide (combo_box->priv->popup);
	if (combo_box->priv->torn_off) {
		GTK_TEAROFF_MENU_ITEM (combo_box->priv->tearable)->torn_off
			= FALSE;
		gnm_combo_set_tearoff_state (combo_box, FALSE);
	}

	do_focus_change (combo_box->priv->toplevel, FALSE);
	gtk_grab_remove (combo_box->priv->toplevel);
	gdk_display_pointer_ungrab (gtk_widget_get_display (combo_box->priv->toplevel),
				    GDK_CURRENT_TIME);

	g_object_ref (combo_box->priv->pop_down_widget);
	g_signal_emit (combo_box,
		       gnm_combo_box_signals [POP_DOWN_DONE], 0,
		       combo_box->priv->pop_down_widget, &popup_info_destroyed);

	if (popup_info_destroyed){
		gtk_container_remove (
			GTK_CONTAINER (combo_box->priv->frame),
			combo_box->priv->pop_down_widget);
		combo_box->priv->pop_down_widget = NULL;
	}
	g_object_unref (combo_box->priv->pop_down_widget);
	set_arrow_state (combo_box, FALSE);

	g_signal_emit (combo_box, gnm_combo_box_signals [POST_POP_HIDE], 0);
}

/**
 * gnm_combo_box_popup_hide:
 * @combo_box:  Combo box
 *
 * Hide popup, but not when it is torn off.
 * This is the external interface - for subclasses and apps which expect a
 * regular combo which doesn't do tearoffs.
 */
/* protected */ void
gnm_combo_box_popup_hide (GnmComboBox *combo_box)
{
	if (!combo_box->priv->torn_off)
		gnm_combo_box_popup_hide_unconditional (combo_box);
	else if (GTK_WIDGET_VISIBLE (combo_box->priv->toplevel)) {
		/* Both popup and tearoff window present. Get rid of just
                   the popup shell. */
		gnm_combo_popup_tear_off (combo_box, FALSE);
		set_arrow_state (combo_box, FALSE);
	}
}

/*
 * Find best location for displaying
 */
/* protected */ void
gnm_combo_box_get_pos (GnmComboBox *combo_box, int *x, int *y)
{
	GtkWidget *wcombo = GTK_WIDGET (combo_box);
	GdkScreen *screen = gtk_widget_get_screen (wcombo);
	int ph, pw;

	gdk_window_get_origin (wcombo->window, x, y);
	*y += wcombo->allocation.height + wcombo->allocation.y;
	*x += wcombo->allocation.x;

	ph = combo_box->priv->popup->allocation.height;
	pw = combo_box->priv->popup->allocation.width;

	if ((*y + ph) > gdk_screen_get_height (screen))
		*y = gdk_screen_get_height (screen) - ph;

	if ((*x + pw) > gdk_screen_get_width (screen))
		*x = gdk_screen_get_width (screen) - pw;
}

/* protected */ void
gnm_combo_box_popup_display (GnmComboBox *combo_box)
{
	int x, y;

	g_return_if_fail (combo_box != NULL);
	g_return_if_fail (IS_GNM_COMBO_BOX (combo_box));

	/*
	 * If we have no widget to display on the popdown,
	 * create it
	 */
	if (!combo_box->priv->pop_down_widget){
		GtkWidget *pw = NULL;

		g_signal_emit (combo_box,
			       gnm_combo_box_signals [POP_DOWN_WIDGET], 0, &pw);
		g_assert (pw != NULL);
		combo_box->priv->pop_down_widget = pw;
		gtk_container_add (GTK_CONTAINER (combo_box->priv->frame), pw);
	}

	g_signal_emit (combo_box, gnm_combo_box_signals [PRE_POP_DOWN], 0);

	if (combo_box->priv->torn_off) {
		/* To give the illusion that tearoff still displays the
		 * popup, we copy the image in the popup window to the
		 * background. Thus, it won't be blank after reparenting */
		gnm_combo_tearoff_bg_copy (combo_box);

		/* We force an unrealize here so that we don't trigger
		 * redrawing/ clearing code - we just want to reveal our
		 * backing pixmap.
		 */
		gnm_combo_popup_reparent (combo_box->priv->popup,
					  combo_box->priv->toplevel, TRUE);
	}

	gnm_combo_box_get_pos (combo_box, &x, &y);

	gtk_window_move (GTK_WINDOW (combo_box->priv->toplevel), x, y);
	gtk_widget_realize (combo_box->priv->popup);
	gtk_widget_show (combo_box->priv->popup);
	gtk_widget_realize (combo_box->priv->toplevel);
	gtk_widget_show (combo_box->priv->toplevel);

	gtk_widget_grab_focus (combo_box->priv->toplevel);
	do_focus_change (combo_box->priv->toplevel, TRUE);

	gtk_grab_add (combo_box->priv->toplevel);
	gdk_pointer_grab (combo_box->priv->toplevel->window, TRUE,
			  GDK_BUTTON_PRESS_MASK |
			  GDK_BUTTON_RELEASE_MASK |
			  GDK_POINTER_MOTION_MASK,
			  NULL, NULL, GDK_CURRENT_TIME);
	set_arrow_state (combo_box, TRUE);
}

static gboolean
cb_arrow_pressed (GtkWidget *button,
		  GdkEvent *event, GnmComboBox *combo_box)
{
	if (!combo_box->priv->updating_buttons) {
		if (combo_box->priv->toplevel == NULL ||
		    !GTK_WIDGET_VISIBLE (combo_box->priv->toplevel))
			gnm_combo_box_popup_display (combo_box);
		else
			gnm_combo_box_popup_hide_unconditional (combo_box);
	}

	return TRUE;
}

static  gint
gnm_combo_box_button_press (GtkWidget *widget, GdkEventButton *event, GnmComboBox *combo_box)
{
	GtkWidget *child = gtk_get_event_widget ((GdkEvent *) event);
	if (child != widget){
		while (child){
			if (child == widget)
				return FALSE;
			child = child->parent;
		}
	}

	gnm_combo_box_popup_hide (combo_box);
	return TRUE;
}

/**
 * gnm_combo_box_key_press
 * @widget:     Widget
 * @event:      Event
 * @combo_box:  Combo box
 *
 * Key press handler which dismisses popup on escape.
 * Popup is dismissed whether or not popup is torn off.
 */
static  gint
gnm_combo_box_key_press (GtkWidget *widget, GdkEventKey *event,
			 GnmComboBox *combo_box)
{
	if (event->keyval == GDK_Escape) {
		gnm_combo_box_popup_hide_unconditional (combo_box);
		return TRUE;
	} else
		return FALSE;
}

static void
cb_state_change (GtkWidget *widget, GtkStateType old_state, GnmComboBox *combo_box)
{
	GtkStateType const new_state = GTK_WIDGET_STATE(widget);
	gtk_widget_set_state (combo_box->priv->display_widget, new_state);
}

static void
gnm_combo_box_init (GnmComboBox *combo_box)
{
	GtkWidget *arrow;
	GdkCursor *cursor;

	combo_box->priv = g_new0 (GnmComboBoxPrivate, 1);
	combo_box->priv->updating_buttons = FALSE;

	/*
	 * Create the arrow
	 */
	combo_box->priv->arrow_button = gtk_toggle_button_new ();
	gtk_button_set_relief (GTK_BUTTON (combo_box->priv->arrow_button), GTK_RELIEF_NONE);
	GTK_WIDGET_UNSET_FLAGS (combo_box->priv->arrow_button, GTK_CAN_FOCUS);

	arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (combo_box->priv->arrow_button), arrow);
	gtk_box_pack_end (GTK_BOX (combo_box), combo_box->priv->arrow_button, FALSE, FALSE, 0);
	g_signal_connect (combo_box->priv->arrow_button,
		"button-press-event",
		G_CALLBACK (cb_arrow_pressed), combo_box);
	gtk_widget_show_all (combo_box->priv->arrow_button);

	/*
	 * prelight the display widget when mousing over the arrow.
	 */
	g_signal_connect (combo_box->priv->arrow_button, "state-changed",
			  G_CALLBACK (cb_state_change), combo_box);

	/*
	 * The pop-down container
	 */

	combo_box->priv->toplevel = gtk_window_new (GTK_WINDOW_POPUP);
	gtk_widget_ref (combo_box->priv->toplevel);
	gtk_object_sink (GTK_OBJECT (combo_box->priv->toplevel));
	g_object_set (G_OBJECT (combo_box->priv->toplevel),
		"allow_shrink",	FALSE,
		"allow_grow",	TRUE,
		NULL);

	combo_box->priv->popup = gtk_event_box_new ();
	gtk_container_add (GTK_CONTAINER (combo_box->priv->toplevel),
			   combo_box->priv->popup);
	gtk_widget_show (combo_box->priv->popup);

	gtk_widget_realize (combo_box->priv->popup);
	cursor = gdk_cursor_new_for_display (gtk_widget_get_display (GTK_WIDGET (combo_box)), GDK_TOP_LEFT_ARROW);
	gdk_window_set_cursor (combo_box->priv->popup->window, cursor);
	gdk_cursor_unref (cursor);

	combo_box->priv->torn_off = FALSE;
	combo_box->priv->tearoff_window = NULL;

	combo_box->priv->frame = gtk_frame_new (NULL);
	gtk_container_add (GTK_CONTAINER (combo_box->priv->popup),
			   combo_box->priv->frame);
	gtk_frame_set_shadow_type (GTK_FRAME (combo_box->priv->frame), GTK_SHADOW_OUT);

	g_signal_connect (combo_box->priv->toplevel, "button_press_event",
			  G_CALLBACK (gnm_combo_box_button_press), combo_box);
	g_signal_connect (combo_box->priv->toplevel, "key_press_event",
			  G_CALLBACK (gnm_combo_box_key_press), combo_box);
}

GSF_CLASS (GnmComboBox, gnm_combo_box,
	   gnm_combo_box_class_init, gnm_combo_box_init,
	   GTK_TYPE_HBOX)

/**
 * gnm_combo_box_set_display:
 * @combo_box: the Combo Box to modify
 * @display_widget: The widget to be displayed

 * Sets the displayed widget for the @combo_box to be @display_widget
 */
/* protected */ void
gnm_combo_box_set_display (GnmComboBox *combo_box, GtkWidget *display_widget)
{
	g_return_if_fail (IS_GNM_COMBO_BOX (combo_box));
	g_return_if_fail (GTK_IS_WIDGET (display_widget));

	if (combo_box->priv->display_widget != NULL &&
	    combo_box->priv->display_widget != display_widget)
		gtk_container_remove (GTK_CONTAINER (combo_box),
				      combo_box->priv->display_widget);

	combo_box->priv->display_widget = display_widget;

	gtk_box_pack_start (GTK_BOX (combo_box), display_widget, TRUE, TRUE, 0);
}

static gboolean
cb_tearable_enter_leave (GtkWidget *w, GdkEventCrossing *event, gpointer data)
{
	gboolean const flag = GPOINTER_TO_INT(data);
	gtk_widget_set_state (w, flag ? GTK_STATE_PRELIGHT : GTK_STATE_NORMAL);
	return FALSE;
}

/**
 * gnm_combo_popup_tear_off
 * @combo:         Combo box
 * @set_position:  Set to position of popup shell if true
 *
 * Tear off the popup
 *
 * FIXME:
 * Gtk popup menus are toplevel windows, not dialogs. I think this is wrong,
 * and make the popups dialogs. But may be there should be a way to make
 * them toplevel. We can do this after creating:
 * GTK_WINDOW (tearoff)->type = GTK_WINDOW_TOPLEVEL;
 */
static void
gnm_combo_popup_tear_off (GnmComboBox *combo, gboolean set_position)
{
	int x, y;

	if (!combo->priv->tearoff_window) {
		GtkWidget *tearoff;
		gchar *title;

		/* FIXME: made this a toplevel, not a dialog ! */
		tearoff = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		gtk_widget_ref (tearoff);
		gtk_object_sink (GTK_OBJECT (tearoff));
		combo->priv->tearoff_window = tearoff;
		gtk_widget_set_app_paintable (tearoff, TRUE);
		g_signal_connect (tearoff, "key_press_event",
				  G_CALLBACK (gnm_combo_box_key_press),
				  combo);
		gtk_widget_realize (tearoff);
		title = g_object_get_data (G_OBJECT (combo),
					   "gnm-combo-title");
		if (title)
			gdk_window_set_title (tearoff->window, title);
		g_object_set (G_OBJECT (tearoff),
			"allow_shrink",	FALSE,
			"allow_grow",	TRUE,
			NULL);
		gtk_window_set_transient_for
			(GTK_WINDOW (tearoff),
			 GTK_WINDOW (gtk_widget_get_toplevel
				     GTK_WIDGET (combo)));
	}

	if (GTK_WIDGET_VISIBLE (combo->priv->popup)) {
		gtk_widget_hide (combo->priv->toplevel);

		gtk_grab_remove (combo->priv->toplevel);
		gdk_display_pointer_ungrab (gtk_widget_get_display (combo->priv->toplevel),
					    GDK_CURRENT_TIME);
	}

	gnm_combo_popup_reparent (combo->priv->popup,
				  combo->priv->tearoff_window, FALSE);

	/* It may have got confused about size */
	gtk_widget_queue_resize (GTK_WIDGET (combo->priv->popup));

	if (set_position) {
		gnm_combo_box_get_pos (combo, &x, &y);
		gtk_window_move (GTK_WINDOW (combo->priv->tearoff_window), x, y);
	}
	gtk_widget_show (GTK_WIDGET (combo->priv->popup));
	gtk_widget_show (combo->priv->tearoff_window);

}

/**
 * gnm_combo_set_tearoff_state
 * @combo_box:  Combo box
 * @torn_off:   TRUE: Tear off. FALSE: Pop down and reattach
 *
 * Set the tearoff state of the popup
 *
 * Compare with gtk_menu_set_tearoff_state in gtk/gtkmenu.c
 */
static void
gnm_combo_set_tearoff_state (GnmComboBox *combo,
			     gboolean  torn_off)
{
	g_return_if_fail (combo != NULL);
	g_return_if_fail (IS_GNM_COMBO_BOX (combo));

	if (combo->priv->torn_off != torn_off) {
		combo->priv->torn_off = torn_off;

		if (combo->priv->torn_off) {
			gnm_combo_popup_tear_off (combo, TRUE);
			set_arrow_state (combo, FALSE);
		} else {
			gtk_widget_hide (combo->priv->tearoff_window);
			gnm_combo_popup_reparent (combo->priv->popup,
						  combo->priv->toplevel,
						  FALSE);
		}
	}
}

/**
 * gnm_combo_tearoff_bg_copy
 * @combo_box:  Combo box
 *
 * Copy popup window image to the tearoff window.
 */
static void
gnm_combo_tearoff_bg_copy (GnmComboBox *combo)
{
	GdkPixmap *pixmap;
	GdkGC *gc;
	GdkGCValues gc_values;

	GtkWidget *widget = combo->priv->popup;

	if (combo->priv->torn_off) {
		gc_values.subwindow_mode = GDK_INCLUDE_INFERIORS;
		gc = gdk_gc_new_with_values (widget->window,
					     &gc_values, GDK_GC_SUBWINDOW);

		pixmap = gdk_pixmap_new (widget->window,
					 widget->allocation.width,
					 widget->allocation.height,
					 -1);

		gdk_draw_drawable (pixmap, gc,
				 widget->window,
				 0, 0, 0, 0, -1, -1);
		g_object_unref (gc);

		gtk_widget_set_size_request (combo->priv->tearoff_window,
				      widget->allocation.width,
				      widget->allocation.height);

		gdk_window_set_back_pixmap
			(combo->priv->tearoff_window->window, pixmap, FALSE);
		g_object_unref (pixmap);
	}
}

/**
 * gnm_combo_popup_reparent
 * @popup:       Popup
 * @new_parent:  New parent
 * @unrealize:   Unrealize popup if TRUE.
 *
 * Reparent the popup, taking care of the refcounting
 *
 * Compare with gtk_menu_reparent in gtk/gtkmenu.c
 */
static void
gnm_combo_popup_reparent (GtkWidget *popup,
			  GtkWidget *new_parent,
			  gboolean unrealize)
{
	GtkObject *object = GTK_OBJECT (popup);
	gboolean was_floating = GTK_OBJECT_FLOATING (object);

	g_object_ref (object);
	gtk_object_sink (object);

	if (unrealize) {
		g_object_ref (object);
		gtk_container_remove (GTK_CONTAINER (popup->parent), popup);
		gtk_container_add (GTK_CONTAINER (new_parent), popup);
		g_object_unref (object);
	}
	else
		gtk_widget_reparent (GTK_WIDGET (popup), new_parent);
	gtk_widget_set_size_request (new_parent, -1, -1);

	if (was_floating)
		GTK_OBJECT_SET_FLAGS (object, GTK_FLOATING);
	else
		g_object_unref (object);
}

/**
 * cb_tearable_button_release
 * @w:      Widget
 * @event:  Event
 * @combo:  Combo box
 *
 * Toggle tearoff state.
 */
static gboolean
cb_tearable_button_release (GtkWidget *w, GdkEventButton *event,
			    GnmComboBox *combo)
{
	GtkTearoffMenuItem *tearable;

	g_return_val_if_fail (w != NULL, FALSE);
	g_return_val_if_fail (GTK_IS_TEAROFF_MENU_ITEM (w), FALSE);

	tearable = GTK_TEAROFF_MENU_ITEM (w);
	tearable->torn_off = !tearable->torn_off;

	if (!combo->priv->torn_off) {
		gboolean need_connect;

		need_connect = (!combo->priv->tearoff_window);
		gnm_combo_set_tearoff_state (combo, TRUE);
		if (need_connect)
			g_signal_connect (combo->priv->tearoff_window,
					  "delete_event",
					  G_CALLBACK (cb_popup_delete),
					  combo);
	} else
		gnm_combo_box_popup_hide_unconditional (combo);

	return TRUE;
}

static gboolean
cb_popup_delete (GtkWidget *w, GdkEventAny *event, GnmComboBox *combo)
{
	gnm_combo_box_popup_hide_unconditional (combo);
	return TRUE;
}

void
gnm_combo_box_construct (GnmComboBox *combo_box, GtkWidget *display_widget, GtkWidget *pop_down_widget)
{
	GtkWidget *tearable;
	GtkWidget *vbox;

	g_return_if_fail (combo_box != NULL);
	g_return_if_fail (IS_GNM_COMBO_BOX (combo_box));
	g_return_if_fail (display_widget  != NULL);
	g_return_if_fail (GTK_IS_WIDGET (display_widget));

	GTK_BOX (combo_box)->spacing = 0;
	GTK_BOX (combo_box)->homogeneous = FALSE;

	combo_box->priv->pop_down_widget = pop_down_widget;
	combo_box->priv->display_widget = NULL;

	vbox = gtk_vbox_new (FALSE, 5);
	tearable = gtk_tearoff_menu_item_new ();
	g_signal_connect (tearable, "enter-notify-event",
			  G_CALLBACK (cb_tearable_enter_leave),
			  GINT_TO_POINTER (TRUE));
	g_signal_connect (tearable, "leave-notify-event",
			  G_CALLBACK (cb_tearable_enter_leave),
			  GINT_TO_POINTER (FALSE));
	g_signal_connect (tearable, "button-release-event",
			  G_CALLBACK (cb_tearable_button_release),
			  (gpointer) combo_box);
	gtk_box_pack_start (GTK_BOX (vbox), tearable, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), pop_down_widget, TRUE, TRUE, 0);
	combo_box->priv->tearable = tearable;

	/*
	 * Finish setup
	 */
	gnm_combo_box_set_display (combo_box, display_widget);

	gtk_container_add (GTK_CONTAINER (combo_box->priv->frame), vbox);
	gtk_widget_show_all (combo_box->priv->frame);
}

GtkWidget *
gnm_combo_box_new (GtkWidget *display_widget, GtkWidget *optional_popdown)
{
	GnmComboBox *combo_box;

	g_return_val_if_fail (display_widget  != NULL, NULL);
	g_return_val_if_fail (GTK_IS_WIDGET (display_widget), NULL);

	combo_box = g_object_new (GNM_COMBO_BOX_TYPE, NULL);
	gnm_combo_box_construct (combo_box, display_widget, optional_popdown);
	return GTK_WIDGET (combo_box);
}

GtkWidget *
gnm_combo_box_get_arrow (GnmComboBox *cc)
{
	g_return_val_if_fail (IS_GNM_COMBO_BOX (cc), NULL);

	return cc->priv->arrow_button;
}

/**
 * gnm_combo_box_set_title
 * @combo: Combo box
 * @title: Title
 *
 * Set a title to display over the tearoff window.
 *
 * FIXME:
 *
 * This should really change the title even when the popup is already torn off.
 * I guess the tearoff window could attach a listener to title change or
 * something. But I don't think we need the functionality, so I didn't bother
 * to investigate.
 */
void
gnm_combo_box_set_title (GnmComboBox *combo,
			 const gchar *title)
{
	g_return_if_fail (combo != NULL);
	g_return_if_fail (IS_GNM_COMBO_BOX (combo));

	g_object_set_data_full (G_OBJECT (combo), "gnm-combo-title",
				g_strdup (title), (GDestroyNotify) g_free);
}

/**
 * gnm_combo_box_set_tearable:
 * @combo: Combo box
 * @tearable: whether to allow the @combo to be tearable
 *
 * controls whether the combo box's pop up widget can be torn off.
 */
void
gnm_combo_box_set_tearable (GnmComboBox *combo, gboolean tearable)
{
	g_return_if_fail (combo != NULL);
	g_return_if_fail (IS_GNM_COMBO_BOX (combo));

	if (tearable){
		gtk_widget_show (combo->priv->tearable);
	} else {
		gnm_combo_set_tearoff_state (combo, FALSE);
		gtk_widget_hide (combo->priv->tearable);
	}
}
