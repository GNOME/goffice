/*
 * gog-grid.h :
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
#ifndef GOG_GRID_H
#define GOG_GRID_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

typedef enum {
	GOG_GRID_UNKNOWN = -1,
	GOG_GRID_XY,
	GOG_GRID_YZ,
	GOG_GRID_ZX,
	GOG_GRID_TYPES
} GogGridType;

#define GOG_TYPE_GRID	(gog_grid_get_type ())
#define GOG_GRID(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_GRID, GogGrid))
#define GOG_IS_GRID(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_GRID))

GType gog_grid_get_type (void);

GogGridType	gog_grid_get_gtype (GogGrid const *grid);
void		gog_grid_set_gtype (GogGrid *grid, GogGridType type);

G_END_DECLS

#endif /* GOG_GRID_H */
