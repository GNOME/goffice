/*
 * go-component.h :
 *
 * Copyright (C) 2005-2010 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GO_COMPONENT_H
#define GO_COMPONENT_H

#include <glib-object.h>
#include <goffice/goffice.h>
#include <goffice/component/goffice-component.h>

G_BEGIN_DECLS

GO_VAR_DECL double GOCXres, GOCYres;

typedef enum {
	GO_SNAPSHOT_NONE,
	GO_SNAPSHOT_SVG,
	GO_SNAPSHOT_PNG,
	/* <private> */
	GO_SNAPSHOT_MAX
} GOSnapshotType;

typedef struct _GOComponentPrivate GOComponentPrivate;
struct _GOComponent {
	GObject parent;

	/*protected*/
	char *mime_type;
	double width, ascent, descent, height;
	double default_width, default_ascent, default_descent;
	gboolean resizable, editable;
	char const *data;
	GDestroyNotify destroy_notify;
	gpointer destroy_data;
	int length;
	GOSnapshotType snapshot_type;
	void *snapshot_data;
	size_t snapshot_length;
	GtkWindow *editor;
	GOCmdContext *cc;
	GOComponentPrivate *priv;
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
	void (*render) (GOComponent *component, cairo_t *cr,
			    double width, double height);
	gboolean (*set_font) (GOComponent *component, PangoFontDescription const *desc);
	/*<private>*/
	void (*reserved1) (void);
	void (*reserved2) (void);
	void (*reserved3) (void);
	void (*reserved4) (void);

	/* signals */
	void (*changed) (GOComponent* component);
	/*<private>*/
	void (*reserved_signal1) (void);
	void (*reserved_signal2) (void);
	void (*reserved_signal3) (void);
	void (*reserved_signal4) (void);
};

typedef struct _GOComponentClass GOComponentClass;

#define GO_TYPE_COMPONENT	(go_component_get_type ())
#define GO_COMPONENT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_COMPONENT, GOComponent))
#define GO_IS_COMPONENT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_COMPONENT))

GType	  go_component_get_type (void);
GOComponent  *go_component_new_by_mime_type	(char const *mime_type);
GOComponent  *go_component_new_from_uri	(char const *uri);

void go_component_set_default_size (GOComponent *component,
				    double width, double ascent, double descent);
#if 0
void go_component_set_window (GOComponent *component, GdkWindow *window);
#endif
void go_component_set_data (GOComponent *component,
			    char const *data, int length);
gboolean go_component_get_data (GOComponent *component, gpointer *data, int *length,
				GDestroyNotify *clearfunc, gpointer *user_data);
char const *go_component_get_mime_type (GOComponent *component);
void go_component_set_size (GOComponent *component, double width, double height);
gboolean go_component_is_resizable (GOComponent *component);
gboolean go_component_is_editable (GOComponent *component);
GtkWindow* go_component_edit (GOComponent *component);
void go_component_stop_editing (GOComponent *component);
void go_component_emit_changed (GOComponent *component);

void go_component_set_command_context (GOComponent *component, GOCmdContext *cc);
GOCmdContext *go_component_get_command_context (GOComponent *component);
void go_component_set_default_command_context (GOCmdContext *cc);
void go_component_render (GOComponent *component, cairo_t *cr, double width, double height);
void go_component_get_size (GOComponent *component, double *width, double *height);
gboolean go_component_set_font (GOComponent *component, PangoFontDescription const *desc);

void go_component_write_xml_sax (GOComponent *component, GsfXMLOut *output);
typedef void (*GOComponentSaxHandler)(GOComponent *component, gpointer user_data);
void go_component_sax_push_parser (GsfXMLIn *xin, xmlChar const **attrs,
				       GOComponentSaxHandler handler, gpointer user_data);

GOSnapshotType go_component_build_snapshot (GOComponent *component);
void const *go_component_get_snapshot (GOComponent *component, GOSnapshotType *type, size_t *length);

gboolean go_component_export_image (GOComponent *component, GOImageFormat format,
                                GsfOutput *output, double x_dpi, double y_dpi);
GOComponent *go_component_duplicate (GOComponent const *component);

void go_component_set_inline (GOComponent *component, gboolean is_inline);
gboolean go_component_get_inline (GOComponent *component);
void go_component_set_use_font_from_app (GOComponent *component, gboolean use_font_from_app);
gboolean go_component_get_use_font_from_app (GOComponent *component);

G_END_DECLS

#endif	/* GOFFICE_COMPONENT_H */
