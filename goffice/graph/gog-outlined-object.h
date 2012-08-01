/*
 * gog-outlined-object.h : some utility classes for objects with outlines.
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
#ifndef GOG_OUTLINED_OBJECT_H
#define GOG_OUTLINED_OBJECT_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

typedef struct {
	GogStyledObject	base;
	double	 padding_pts;
} GogOutlinedObject;

typedef	GogStyledObjectClass GogOutlinedObjectClass;

#define GOG_TYPE_OUTLINED_OBJECT  (gog_outlined_object_get_type ())
#define GOG_OUTLINED_OBJECT(o)	  (G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_OUTLINED_OBJECT, GogOutlinedObject))
#define GOG_IS_OUTLINED_OBJECT(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_OUTLINED_OBJECT))

GType  gog_outlined_object_get_type (void);
double gog_outlined_object_get_pad  (GogOutlinedObject const *goo);

/****************************************************************************/

typedef struct {
	GogView base;
} GogOutlinedView;

typedef struct {
	GogViewClass	base;
	gboolean	call_parent_render;
} GogOutlinedViewClass;

#define GOG_TYPE_OUTLINED_VIEW  	(gog_outlined_view_get_type ())
#define GOG_OUTLINED_VIEW(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_OUTLINED_VIEW, GogOutlinedView))
#define GOG_IS_OUTLINED_VIEW(o) 	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_OUTLINED_VIEW))
#define GOG_OUTLINED_VIEW_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GOG_TYPE_OUTLINED_VIEW, GogOutlinedViewClass))

GType   gog_outlined_view_get_type (void);

G_END_DECLS

#endif /* GOG_OUTLINED_VIEW_H */
