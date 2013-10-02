/*
 * go-styled-object.h :
 *
 * Copyright (C) 2009 JJean Brefort (jean.brefort@normalesup.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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

#ifndef GO_STYLED_OBJECT_H
#define GO_STYLED_OBJECT_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

typedef struct {
	GTypeInterface		   base;

	gboolean  (*set_style) 	    (GOStyledObject *gso, GOStyle *style);
	GOStyle	 *(*get_style)	    (GOStyledObject *gso);
	GOStyle	 *(*get_auto_style) (GOStyledObject *gso);
	void	  (*style_changed)  (GOStyledObject *gso);
	void	  (*apply_theme)	(GOStyledObject *gso, GOStyle *style);
	GODoc    *(*get_document)   (GOStyledObject *gso);
} GOStyledObjectClass;

#define GO_TYPE_STYLED_OBJECT		(go_styled_object_get_type ())
#define GO_STYLED_OBJECT(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_STYLED_OBJECT, GOStyledObject))
#define GO_IS_STYLED_OBJECT(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_STYLED_OBJECT))
#define GO_STYLED_OBJECT_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST ((k), GO_TYPE_STYLED_OBJECT, GOStyledObjectClass))
#define GO_IS_STYLED_OBJECT_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), GO_TYPE_STYLED_OBJECT))
#define GO_STYLED_OBJECT_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_INTERFACE ((o), GO_TYPE_STYLED_OBJECT, GOStyledObjectClass))

GType go_styled_object_get_type (void);

gboolean  go_styled_object_set_style 	   (GOStyledObject *gso, GOStyle *style);
GOStyle	 *go_styled_object_get_style	   (GOStyledObject *gso);
GOStyle	 *go_styled_object_get_auto_style  (GOStyledObject *gso);
void	  go_styled_object_style_changed   (GOStyledObject *gso);
void	  go_styled_object_apply_theme	   (GOStyledObject *gso, GOStyle *style);
GODoc	 *go_styled_object_get_document	   (GOStyledObject *gso);
gboolean  go_styled_object_set_cairo_line  (GOStyledObject const *so, cairo_t *cr);
void      go_styled_object_fill		   (GOStyledObject const *so, cairo_t *cr, gboolean preserve);
G_END_DECLS

#endif


