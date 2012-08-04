/*
 * gog-chart-map.h :
 *
 * Copyright (C) 2006-2007 Emmanuel Pacaud <emmanuel.pacaud@lapp.in2p3.fr>
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

#ifndef GOG_CHART_MAP_H
#define GOG_CHART_MAP_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

typedef struct {
	double cx, cy;
	double rx, ry;
	double th0, th1;
} GogChartMapPolarData;

typedef struct _GogChartMap GogChartMap;

GType		 gog_chart_map_get_type (void);
GogChartMap *gog_chart_map_new 		(GogChart *chart, GogViewAllocation const *area,
						 GogAxis *axis0, GogAxis *axis1, GogAxis *axis2,
						 gboolean fill_area);
void 		 gog_chart_map_2D_to_view	(GogChartMap *map, double x, double y, double *u, double *v);
double		 gog_chart_map_2D_derivative_to_view (GogChartMap *map, double deriv, double x, double y) ;
void		 gog_chart_map_view_to_2D       (GogChartMap *map, double x, double y, double *u, double *v);
GogAxisMap	*gog_chart_map_get_axis_map 	(GogChartMap *map, unsigned int index);
gboolean	 gog_chart_map_is_valid 	(GogChartMap *map);
void		 gog_chart_map_free 		(GogChartMap *map);

GOPath 		*gog_chart_map_make_path 	(GogChartMap *map, double const *x, double const *y,
						 int n_points, GOLineInterpolation interpolation,
						 gboolean skip_invalid, gpointer data);
GOPath 		*gog_chart_map_make_close_path 	(GogChartMap *map, double const *x, double const *y,
						 int n_points,
						 GogSeriesFillType fill_type);

GogChartMapPolarData *gog_chart_map_get_polar_parms (GogChartMap *map);

G_END_DECLS

#endif /* GOG_CHART_MAP_H */
