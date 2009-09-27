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
#include <goffice/goffice.h>
#include <goffice/component/goffice-component.h>

G_BEGIN_DECLS

GO_VAR_DECL double GOCXres, GOCYres;

struct _GOComponent {
	GObject parent;

	/*protected*/
	char *mime_type;
	double width, ascent, descent, height;
	double default_width, default_ascent, default_descent;
	gboolean needs_window, resizable, editable;
	char const *data;
	int length;
	GdkWindow *window;
};

struct _GOComponentClass {
	GObjectClass parent_class;

	GtkWindow* (*edit) (GOComponent *component);
	gboolean (*get_data) (GOComponent *component, gpointer *data, int *length,
			      GDestroyNotify *clearfunc, gpointer *user_data);
	void (*mime_type_set) (GOComponent* component);
	void (*set_data) (GOComponent *component);
	void (*set_default_size) (GOComponent* component);
	void (*set_size) (GOComponent *component);
	void (*set_window) (GOComponent *component);
	void (*render) (GOComponent *component, cairo_t *cr,
			    double width, double height);

	/* signals */
	void (*changed) (GOComponent* component);
};

typedef struct _GOComponentClass GOComponentClass;

#define GO_TYPE_COMPONENT	(go_component_get_type ())
#define GO_COMPONENT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_COMPONENT, GOComponent))
#define GO_IS_COMPONENT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_COMPONENT))

#define GOC_PARAM_PERSISTENT	(1 << (G_PARAM_USER_SHIFT+0))

GType	  go_component_get_type (void);
GOComponent  *go_component_new_by_mime_type	(char const *mime_type);

void go_component_set_default_size (GOComponent *component,
				    double width, double ascent, double descent);
gboolean go_component_needs_window (GOComponent *component);
void go_component_set_window (GOComponent *component, GdkWindow *window);
void go_component_set_data (GOComponent *component,
			    char const *data, int length);
gboolean go_component_get_data (GOComponent *component, gpointer *data, int *length,
				GDestroyNotify *clearfunc, gpointer *user_data);
void go_component_set_size (GOComponent *component, double width, double height);
gboolean go_component_is_resizable (GOComponent *component);
gboolean go_component_is_editable (GOComponent *component);
GtkWindow* go_component_edit (GOComponent *component);
void go_component_emit_changed (GOComponent *component);

void go_component_set_command_context (GOCmdContext *cc);
GOCmdContext *go_component_get_command_context (void);
void go_component_render (GOComponent *component, cairo_t *cr, double width, double height);

G_END_DECLS

#endif	/* GOFFICE_COMPONENT_H */
