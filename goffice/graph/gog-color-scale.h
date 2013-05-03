/*
 * gog-color-scale.h
 *
 * Copyright (C) 2012 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GOG_COLOR_SCALE_H
#define GOG_COLOR_SCALE_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

#define GOG_TYPE_COLOR_SCALE	(gog_color_scale_get_type ())
#define GOG_COLOR_SCALE(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_COLOR_SCALE, GogColorScale))
#define GOG_IS_COLOR_SCALE(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_COLOR_SCALE))

GType gog_color_scale_get_type (void);
GogAxis *gog_color_scale_get_axis (GogColorScale *scale);
void gog_color_scale_set_axis (GogColorScale *scale, GogAxis *axis);

G_END_DECLS

#endif /* GOG_COLOR_SCALE_H */
