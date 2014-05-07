/*
 * gog-3d-box.h :
 *
 * Copyright (C) 2007 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GOG_3D_BOX_H
#define GOG_3D_BOX_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

struct _Gog3DBox {
	GogObject base;

	double fov, psi, theta, phi; /* field of view and orientation */
	GOMatrix3x3 mat; /* the matrix based on psi, theta, and phi */
};

#define GOG_3D_BOX_TYPE		(gog_3d_box_get_type ())
#define GOG_3D_BOX(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_3D_BOX_TYPE, Gog3DBox))
#define GOG_IS_3D_BOX(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_3D_BOX_TYPE))

GType 	gog_3d_box_get_type 	(void);

struct _Gog3DBoxView {
	GogView base;

	double dx, dy, dz; /* box size */
	double r; /* distance from view point to the center of the box */
	double ratio; /* ratio to use to be certain that everything fits
	in the view */
};
typedef struct _Gog3DBoxView Gog3DBoxView;

#define GOG_3D_BOX_VIEW_TYPE		(gog_3d_box_view_get_type ())
#define GOG_3D_BOX_VIEW(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_3D_BOX_VIEW_TYPE, Gog3DBoxView))
#define GOG_IS_3D_BOX_VIEW(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_3D_BOX_VIEW_TYPE))

GType 	gog_3d_box_view_get_type 	(void);

G_END_DECLS

#endif /* GOG_3D_BOX_H */
