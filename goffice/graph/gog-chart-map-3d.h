/*
 * gog-chart-map-3d.h :
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

#ifndef GOG_CHART_MAP_3D_H
#define GOG_CHART_MAP_3D_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

typedef struct _GogChartMap3D GogChartMap3D;

GType		 gog_chart_map_3d_get_type (void);
GogChartMap3D 	*gog_chart_map_3d_new 		(GogView *view, GogViewAllocation const *area,
						 GogAxis *axis0, GogAxis *axis1, GogAxis *axis2);
void 		 gog_chart_map_3d_to_view	(GogChartMap3D *map, double x, double y, double z, double *u, double *v, double *w);
GogAxisMap	*gog_chart_map_3d_get_axis_map 	(GogChartMap3D *map, unsigned int index);
gboolean	 gog_chart_map_3d_is_valid 	(GogChartMap3D *map);
void		 gog_chart_map_3d_free 		(GogChartMap3D *map);

G_END_DECLS

#endif /* GOG_CHART_MAP_3D_H */
