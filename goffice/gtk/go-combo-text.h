/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef _GO_COMBO_TEXT_H_
#define _GO_COMBO_TEXT_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GO_TYPE_COMBO_TEXT	(go_combo_text_get_type ())
#define GO_COMBO_TEXT(obj)	(G_TYPE_CHECK_INSTANCE_CAST (obj, GO_TYPE_COMBO_TEXT, GOComboText))
#define GO_IS_COMBO_TEXT(obj)	(G_TYPE_CHECK_INSTANCE_TYPE (obj, GO_TYPE_COMBO_TEXT))

typedef struct _GOComboText	   GOComboText;

typedef enum {		/* begin the search from : */
	GO_COMBO_TEXT_FROM_TOP,	/* the top of the list */
	GO_COMBO_TEXT_CURRENT,		/* the current selection */
	GO_COMBO_TEXT_NEXT		/* the next element after current */
} GOComboTextSearch;

GType      go_combo_text_get_type	 (void);
GtkWidget *go_combo_text_new		 (GCompareFunc cmp_func);
GtkWidget *go_combo_text_new_default     (void);
GtkWidget *go_combo_text_get_entry	 (GOComboText *ct);

gboolean   go_combo_text_set_text	 (GOComboText *ct, const gchar *text,
					  GOComboTextSearch start);
void	   go_combo_text_add_item	 (GOComboText *ct, const gchar *label);

G_END_DECLS

#endif /* _GO_COMBO_TEXT_H_ */
