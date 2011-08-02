#ifndef GO_PANGO_EXTRAS_H
#define GO_PANGO_EXTRAS_H

#include <pango/pango.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

void go_pango_attr_list_open_hole (PangoAttrList *tape, guint pos, guint len);
void go_pango_attr_list_erase (PangoAttrList *attrs, guint pos, guint len);
void go_pango_attr_list_unset (PangoAttrList  *list,
			       gint start, gint end,
			       PangoAttrType type);
gboolean go_pango_attr_list_is_empty (const PangoAttrList *attrs);

void go_load_pango_attributes_into_buffer (PangoAttrList *markup,
					   GtkTextBuffer *buffer,
					   gchar const *str);
void go_create_std_tags_for_buffer (GtkTextBuffer *buffer);

G_END_DECLS

#endif /* GO_PANGO_EXTRAS_H */
