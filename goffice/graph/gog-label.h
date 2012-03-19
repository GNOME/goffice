/*
 * gog-label.h :
 *
 * Copyright (C) 2003-2004 Jody Goldberg (jody@gnome.org)
 * Copyright (C) 2005      Jean Brefort (jean.brefort@normalesup.org)
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
#ifndef GOG_LABEL_H
#define GOG_LABEL_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

typedef struct {
	GogOutlinedObject base;

	gboolean	  allow_markup;
	gboolean	  rotate_frame;
	gboolean	  rotate_bg;
	gboolean	  allow_wrap;
} GogText;

typedef struct {
	GogOutlinedObjectClass base;

	char *(*get_str)    (GogText *text);
	PangoAttrList *(*get_markup)    (GogText *text);
} GogTextClass;

#define GOG_TYPE_TEXT		(gog_text_get_type ())
#define GOG_TEXT(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_TEXT, GogText))
#define GOG_IS_TEXT(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_TEXT))
#define GOG_TEXT_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GOG_TYPE_TEXT, GogTextClass))

GType gog_text_get_type (void);
char *gog_text_get_str (GogText *text);
PangoAttrList *gog_text_get_markup (GogText *text);

#define GOG_TYPE_LABEL	(gog_label_get_type ())
#define GOG_LABEL(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_LABEL, GogLabel))
#define GOG_IS_LABEL(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_LABEL))

GType gog_label_get_type (void);

#define GOG_TYPE_REG_EQN	(gog_reg_eqn_get_type ())
#define GOG_REG_EQN(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_REG_EQN, GogRegEqn))
#define GOG_IS_REG_EQN(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_REG_EQN))

GType gog_reg_eqn_get_type (void);

G_END_DECLS

#endif /* GOG_LABEL_H */
