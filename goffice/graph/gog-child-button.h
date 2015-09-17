/*
 * gog_child-button.h
 *
 * Copyright (C) 2000-2004 Jody Goldberg (jody@gnome.org)
 * Copyright (C) 2007 Emmanuel Pacaud (emmanuel.pacaud@lapp.in2p3.fr)
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

#ifndef GOG_CHILD_BUTTON_H
#define GOG_CHILD_BUTTON_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

typedef struct _GogChildButton 	  	GogChildButton;
typedef struct _GogChildButtonClass   	GogChildButtonClass;

#define GOG_TYPE_CHILD_BUTTON		 (gog_child_button_get_type ())
#define GOG_CHILD_BUTTON(obj)		 (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOG_TYPE_CHILD_BUTTON, GogChildButton))
#define GOG_CHILD_BUTTON_CLASS(klass)	 (G_TYPE_CHECK_CLASS_CAST ((klass), GOG_TYPE_CHILD_BUTTON, GogChildButtonClass))
#define GOG_IS_CHILD_BUTTON(obj)	 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOG_TYPE_CHILD_BUTTON))
#define GOG_IS_CHILD_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GOG_TYPE_CHILD_BUTTON))
#define GOG_CHILD_BUTTON_GET_CLASS(obj)	 (G_TYPE_INSTANCE_GET_CLASS ((obj), GOG_TYPE_CHILD_BUTTON, GogChildButtonClass))

GType		 gog_child_button_get_type 	(void);

GtkWidget	*gog_child_button_new 		(void);
void 		 gog_child_button_set_object 	(GogChildButton *child_button, GogObject *gog_object);

G_END_DECLS

#endif /* GOG_CHILD_BUTTON_H */
