/*
 * gog-styled-object.h :
 *
 * Copyright (C) 2003-2004 Jody Goldberg (jody@gnome.org)
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
#ifndef GOG_STYLED_OBJECT_H
#define GOG_STYLED_OBJECT_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

/* deprecated, just there for file compatibility and to make gtk-doc happy */
typedef struct _GogStyle GogStyle;

struct _GogStyledObject {
	GogObject	base;

	GOStyle	*style;
};

typedef struct {
	GogObjectClass base;

	/* virtual */
	void	  (*init_style)     	(GogStyledObject *obj, GOStyle *style);

	/* signal */
	void (*style_changed) (GogStyledObject *obj, GOStyle const *new_style);
} GogStyledObjectClass;

#define GOG_TYPE_STYLED_OBJECT	(gog_styled_object_get_type ())
#define GOG_STYLED_OBJECT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_STYLED_OBJECT, GogStyledObject))
#define GOG_IS_STYLED_OBJECT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_STYLED_OBJECT))
#define GOG_STYLED_OBJECT_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GOG_TYPE_STYLED_OBJECT, GogStyledObjectClass))

GType     gog_styled_object_get_type (void);
GogStyle	 *gog_style_new (void);

G_END_DECLS

#endif /* GOG_STYLED_OBJECT_H */
