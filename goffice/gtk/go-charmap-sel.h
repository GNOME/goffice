/*
 *  Copyright (C) 2003 Andreas J. Guelzow
 *
 *  based on code by:
 *  Copyright (C) 2000 Marco Pesenti Gritti
 *  from the galeon code base
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef GNUMERIC_WIDGET_CHARMAP_SELECTOR_H
#define GNUMERIC_WIDGET_CHARMAP_SELECTOR_H

G_BEGIN_DECLS

#include <gui-gnumeric.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtk.h>

#define CHARMAP_SELECTOR_TYPE        (charmap_selector_get_type ())
#define CHARMAP_SELECTOR(obj)        (G_TYPE_CHECK_INSTANCE_CAST((obj), CHARMAP_SELECTOR_TYPE, CharmapSelector))
#define IS_CHARMAP_SELECTOR(obj)     (G_TYPE_CHECK_INSTANCE_TYPE((obj), CHARMAP_SELECTOR_TYPE))

typedef struct _CharmapSelector CharmapSelector;

typedef enum {
     CHARMAP_SELECTOR_TO_UTF8 = 0,
     CHARMAP_SELECTOR_FROM_UTF8
} CharmapSelectorTestDirection;

GType        charmap_selector_get_type (void);
GtkWidget *  charmap_selector_new (CharmapSelectorTestDirection test);

gchar const *charmap_selector_get_encoding (CharmapSelector *cs);
gboolean     charmap_selector_set_encoding (CharmapSelector *cs, const char *enc);

void         charmap_selector_set_sensitive (CharmapSelector *cs, gboolean sensitive);

const char  *charmap_selector_get_encoding_name (CharmapSelector *cs, const char *enc);

G_END_DECLS

#endif /* GNUMERIC_WIDGET_CHARMAP_SELECTOR_H */
