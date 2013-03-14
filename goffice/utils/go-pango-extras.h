#ifndef GO_PANGO_EXTRAS_H
#define GO_PANGO_EXTRAS_H

#include <pango/pango.h>

#ifdef GOFFICE_WITH_GTK
#include <gtk/gtk.h>
#endif
#include <goffice/goffice.h>

G_BEGIN_DECLS

typedef struct {
	PangoAttribute attr;
	gboolean val;
} GOPangoAttrSuperscript;

typedef struct {
	PangoAttribute attr;
	gboolean val;
} GOPangoAttrSubscript;


void go_pango_attr_list_open_hole (PangoAttrList *tape, guint pos, guint len);
void go_pango_attr_list_erase (PangoAttrList *attrs, guint pos, guint len);
void go_pango_attr_list_unset (PangoAttrList  *list,
			       gint start, gint end,
			       PangoAttrType type);
gboolean go_pango_attr_list_is_empty (const PangoAttrList *attrs);

#ifdef GOFFICE_WITH_GTK
void go_load_pango_attributes_into_buffer (PangoAttrList *markup,
					   GtkTextBuffer *buffer,
					   gchar const *str);
void go_create_std_tags_for_buffer (GtkTextBuffer *buffer);
#endif

PangoAttrList *go_pango_translate_attributes (PangoAttrList *attrs);
void go_pango_translate_layout (PangoLayout *layout);
PangoAttribute *go_pango_attr_subscript_new (gboolean val);
PangoAttribute *go_pango_attr_superscript_new (gboolean val);
PangoAttrType go_pango_attr_subscript_get_attr_type (void);
PangoAttrType go_pango_attr_superscript_get_attr_type (void);
#ifndef GOFFICE_DISABLE_DEPRECATED
GOFFICE_DEPRECATED_FOR(go_pango_attr_subscript_get_attr_type)
PangoAttrType go_pango_attr_subscript_get_type (void);
GOFFICE_DEPRECATED_FOR(go_pango_attr_superscript_get_attr_type)
PangoAttrType go_pango_attr_superscript_get_type (void);
#endif

char *go_pango_attrs_to_markup (PangoAttrList *attrs, char const *text);

int go_pango_measure_string (PangoContext *context,
			     PangoFontDescription const *font_desc,
			     char const *str);


G_END_DECLS

#endif /* GO_PANGO_EXTRAS_H */
