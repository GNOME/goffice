/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-component.h :  
 *
 * Copyright (C) 2005 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GO_COMPONENT_H
#define GO_COMPONENT_H

#include <glib-object.h>
#include <gdk/gdkwindow.h>
#include <libgnomeprint/gnome-print.h>

G_BEGIN_DECLS

extern double GOCXres, GOCYres;

struct _GOComponent
{
	GObject parent;

/*protected*/
	char *mime_type;
	double width, ascent, descent, height;
	double default_width, default_ascent, default_descent;
	gboolean needs_window, resizable, editable;
	GdkWindow *window;
	GdkPixbuf *pixbuf;
};

struct _GOComponentClass
{
	GObjectClass parent_class;

	void (*draw) (GOComponent *component, int width_pixels, int height_pixels);
	gboolean (*edit) (GOComponent *component);
	gboolean (*get_data) (GOComponent *component, gpointer *data, int *length,
									void (**clearfunc) (gpointer));
	void (*mime_type_set) (GOComponent* component);
	void (*print) (GOComponent *component, GnomePrintContext *gpc,
												double width, double height);
	void (*set_data) (GOComponent *component, char const *data, int length);
	void (*set_default_size) (GOComponent* component);
	void (*set_pixbuf) (GOComponent *component);
	void (*set_size) (GOComponent *component);
	void (*set_window) (GOComponent *component);
	char *(*export_to_svg) (GOComponent *component);

	/* signals */
	void (*changed) (GOComponent* component);
};

typedef struct _GOComponentClass GOComponentClass;

#define GO_COMPONENT_TYPE	(go_component_get_type ())
#define GO_COMPONENT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GO_COMPONENT_TYPE, GOComponent))
#define IS_GO_COMPONENT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_COMPONENT_TYPE))

GType	  go_component_get_type (void);
GOComponent  *go_component_new_by_type	(GOComponentType const *type);
GOComponent  *go_component_new_by_name	(char const *id);
GOComponent  *go_component_new_by_mime_type	(char const *mime_type);

void go_component_set_default_size (GOComponent *component,
								double width, double ascent, double descent);
gboolean go_component_needs_window (GOComponent *component);
void go_component_set_window (GOComponent *component, GdkWindow *window);
void go_component_set_pixbuf (GOComponent *component, GdkPixbuf *pixbuf);
void go_component_set_data (GOComponent *component,
												char const *data, int length);
gboolean go_component_get_data (GOComponent *component, gpointer *data, int *length,
									void (**clearfunc) (gpointer));
void go_component_set_size (GOComponent *component,
												double width, double height);
void go_component_draw (GOComponent *component, int width_pixels, int height_pixels);
gboolean go_component_is_resizable (GOComponent *component);
gboolean go_component_is_editable (GOComponent *component);
gboolean go_component_edit (GOComponent *component);
void go_component_print (GOComponent *component, GnomePrintContext *gpc,
												double width, double height);
void go_component_emit_changed (GOComponent *component);
char *go_component_export_to_svg (GOComponent *component);

G_END_DECLS

#endif	/* GOFFICE_COMPONENT_H */
