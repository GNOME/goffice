/*
 * go-selector.h :
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

#ifndef GO_SELECTOR_H
#define GO_SELECTOR_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

#define GO_TYPE_SELECTOR		(go_selector_get_type ())
#define GO_SELECTOR(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GO_TYPE_SELECTOR, GOSelector))
#define GO_SELECTOR_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GO_TYPE_SELECTOR, GOSelectorClass))
#define GO_IS_SELECTOR(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GO_TYPE_SELECTOR))
#define GO_IS_SELECTOR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GO_TYPE_SELECTOR))
#define GO_SELECTOR_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GO_TYPE_SELECTOR, GOSelectorClass))

typedef struct _GOSelector 	  GOSelector;
typedef struct _GOSelectorPrivate GOSelectorPrivate;
typedef struct _GOSelectorClass   GOSelectorClass;

struct _GOSelector
{
	GtkBox parent;

	GOSelectorPrivate *priv;
};

struct _GOSelectorClass
{
	GtkBoxClass parent_class;

	/* signals */
	void (*activate)		(GtkWidget *selector);
};

GType		 go_selector_get_type (void);

GtkWidget	*go_selector_new 		(GOPalette *palette);

int 		 go_selector_get_active 	(GOSelector *selector, gboolean *is_auto);
gboolean 	 go_selector_set_active 	(GOSelector *selector, int index);

void 		 go_selector_update_swatch 	(GOSelector *selector);
void 		 go_selector_activate 		(GOSelector *selector);

gpointer 	 go_selector_get_user_data 	(GOSelector *selector);

typedef void		(*GOSelectorDndDataReceived)	(GOSelector *selector, guchar const *data);
typedef gpointer	(*GOSelectorDndDataGet)		(GOSelector *selector);
typedef void		(*GOSelectorDndFillIcon)	(GOSelector *selector, GdkPixbuf *pixbuf);

void 			go_selector_setup_dnd 		(GOSelector *selector,
							 char const *dnd_target,
							 int dnd_length,
							 GOSelectorDndDataGet data_get,
							 GOSelectorDndDataReceived data_received,
							 GOSelectorDndFillIcon fill_icon);

G_END_DECLS

#endif /* GO_SELECTOR_H */

