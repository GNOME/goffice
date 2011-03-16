/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-gtk-compat.h :
 *
 * Copyright (C) 2008-2009 Jean Brefort (jean.brefort@normalesup.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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

#ifndef HAVE_GTK_ADJUSTMENT_GET_PAGE_SIZE
#       define gtk_adjustment_get_page_size(a) (a)->page_size
#endif

#ifndef HAVE_GTK_ADJUSTMENT_GET_UPPER
#       define gtk_adjustment_get_upper(a) (a)->upper
#endif

#ifndef HAVE_GTK_COLOR_SELECTION_DIALOG_GET_COLOR_SELECTION
#       define gtk_color_selection_dialog_get_color_selection(w) (w)->colorsel
#endif

#ifndef HAVE_GTK_DIALOG_GET_ACTION_AREA
#       define gtk_dialog_get_action_area(x) (x)->action_area
#endif

#ifndef HAVE_GTK_LAYOUT_GET_BIN_WINDOW
#       define gtk_layout_get_bin_window(x) (x)->bin_window
#endif

#ifndef HAVE_GTK_MENU_SHELL_GET_ACTIVE
#       define gtk_menu_shell_get_active(w) (w)->active
#endif

#ifndef HAVE_GTK_SELECTION_DATA_GET_DATA
#       define gtk_selection_data_get_data(d) (d)->data
#endif

#ifndef HAVE_GTK_SELECTION_DATA_GET_LENGTH
#       define gtk_selection_data_get_length(d) (d)->length
#endif

#ifndef HAVE_GTK_SELECTION_DATA_GET_LENGTH
#       define gtk_selection_data_get_target(d) (d)->target
#endif

#ifndef HAVE_GTK_TEAROFF_MENU_ITEM_GET_TORN_OFF
#       define gtk_tearoff_menu_item_get_torn_off(w) (w)->torn_off
#endif

#ifndef HAVE_GTK_TEAROFF_MENU_ITEM_SET_TORN_OFF
#       define gtk_tearoff_menu_item_set_torn_off(w,b) (w)->torn_off = b
#endif

#ifndef HAVE_GTK_WIDGET_GET_ALLOCATION
#	define gtk_widget_get_allocation(w,a) *(a) = (w)->allocation
#endif

#ifndef HAVE_GTK_WIDGET_GET_HAS_WINDOW
#	define gtk_widget_get_has_window(w) !GTK_WIDGET_NO_WINDOW (w)
#endif

#ifndef HAVE_GTK_WIDGET_GET_STATE
#       define gtk_widget_get_state(w) GTK_WIDGET_STATE (w)
#endif

#ifndef HAVE_GTK_WIDGET_GET_STYLE
#       define gtk_widget_get_style(w) (w)->style
#endif

#ifndef HAVE_GTK_WIDGET_GET_VISIBLE
#       define gtk_widget_get_visible(w) GTK_WIDGET_VISIBLE (w)
#endif

#ifndef HAVE_GTK_WIDGET_GET_WINDOW
#       define gtk_widget_get_window(w) (w)->window
#endif

#ifndef HAVE_GTK_WIDGET_HAS_FOCUS
#       define gtk_widget_has_focus(w) GTK_WIDGET_HAS_FOCUS (w)
#endif

#ifndef HAVE_GTK_WIDGET_GET_MAPPED
#       define gtk_widget_get_mapped(w) GTK_WIDGET_MAPPED (w)
#endif

#ifndef HAVE_GTK_WIDGET_GET_REALIZED
#       define gtk_widget_get_realized(w) GTK_WIDGET_REALIZED (w)
#endif

#ifndef HAVE_GTK_WIDGET_GET_VISIBLE
#       define gtk_widget_get_visible(w) GTK_WIDGET_VISIBLE (w)
#endif

#ifndef HAVE_GTK_WIDGET_IS_SENSITIVE
#       define gtk_widget_is_sensitive(w) GTK_WIDGET_IS_SENSITIVE (w)
#endif

#ifndef HAVE_GTK_WIDGET_SEND_FOCUS_CHANGE
#define gtk_widget_send_focus_change(w,ev)			\
	do {							\
	g_object_ref (w);					\
	if (((GdkEventFocus*)(ev))->in) GTK_WIDGET_SET_FLAGS ((w), GTK_HAS_FOCUS);       \
	else GTK_WIDGET_UNSET_FLAGS ((w), GTK_HAS_FOCUS);       \
	gtk_widget_event (w, (GdkEvent*)(ev));			\
	g_object_notify (G_OBJECT (w), "has-focus");       \
	g_object_unref (w);				\
	} while (0)
#endif

#ifndef HAVE_GTK_WIDGET_SET_CAN_DEFAULT
#define gtk_widget_set_can_default(w,t)					\
	do {								\
	if (t) GTK_WIDGET_SET_FLAGS ((w), GTK_CAN_DEFAULT);	\
		else GTK_WIDGET_UNSET_FLAGS ((w), GTK_CAN_DEFAULT);	\
	} while (0)
#endif

#ifndef HAVE_GTK_WIDGET_SET_CAN_FOCUS
#define gtk_widget_set_can_focus(w,t)					\
	do {								\
		if (t) GTK_WIDGET_SET_FLAGS ((w), GTK_CAN_FOCUS);	\
		else GTK_WIDGET_UNSET_FLAGS ((w), GTK_CAN_FOCUS);	\
	} while (0)		     
#endif

#ifndef HAVE_GTK_WIDGET_SET_RECEIVES_DEFAULT
#define gtk_widget_set_receives_default(w,t)				\
	do {								\
		if (t) GTK_WIDGET_SET_FLAGS ((w), GTK_RECEIVES_DEFAULT); \
		else GTK_WIDGET_UNSET_FLAGS ((w), GTK_RECEIVES_DEFAULT); \
	} while (0)
#endif

#ifndef HAVE_GTK_WINDOW_GET_DEFAULT_WIDGET
#       define gtk_window_get_default_widget(w) (w)->default_widget
#endif
