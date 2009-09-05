/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-chart-map-3d.c :
 *
 * Copyright (C) 2007 Jean Brefort <jean.brefort@normalesup.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
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

#include <goffice/goffice-config.h>
#include <goffice/goffice.h>

struct _GogChartMap3D {
	GogChart 		*chart;
	GogViewAllocation	 area;
	gpointer	 	 data;
	GogAxisMap		*axis_map[3];
	gboolean		 is_valid;
	GOMatrix3x3		 mat;

	void 	 (*map_3D_to_view) 	(GogChartMap3D *map, double x, double y, double z, double *u, double *v, double *w);
};

static void
null_map_3D (GogChartMap3D *map, double x, double y, double z, double *u, double *v, double *w)
{
	g_warning ("[GogChartMap::map_3D] not implemented");
}

static void
xyz_map_3D_to_view (GogChartMap3D *map, double x, double y, double z, double *u, double *v, double *w)
{
	Gog3DBox *box = GOG_3D_BOX (gog_object_get_child_by_name (GOG_OBJECT (map->chart), "3D-Box"));
	x = gog_axis_map_to_view (map->axis_map[0], x);
	y = gog_axis_map_to_view (map->axis_map[1], y);
	z = gog_axis_map_to_view (map->axis_map[2], z);
	go_matrix3x3_transform (&box->mat, x, y, z, &x, &y, &z);
	if (box->fov > 0.) {
	    x /= (1. - y / box->r) * box->ratio;
	    z /= (1. - y / box->r) * box->ratio;
	} else {
	    x /= box->ratio;
	    z /= box->ratio;
	}
	if (u)
	    *u = map->area.x + map->area.w / 2. * (1. + x / map->area.w);
	if (v)
	    *v = map->area.y + map->area.h / 2. * (1. - z / map->area.h);
	if (w)
	    *w = y;
}

/**
 * gog_chart_3d_map_new:
 * @chart: a #GogChart with 3D support
 * @area: area allocated to chart
 * @axis0: 1st dimension axis
 * @axis1: 2nd dimension axis
 * @axis2: 3rd dimension axis
 * @fill_area: does chart fill allocated area
 *
 * Creates a new #GogChartMap3D, used for conversion from data space
 * to canvas space.
 *
 * returns: a new #GogChartMap3D object.
 **/

GogChartMap3D*
gog_chart_map_3d_new (GogChart *chart, GogViewAllocation const *area,
			GogAxis *axis0, GogAxis *axis1, GogAxis *axis2)
{
	GogChartMap3D *map;
	GogAxisSet axis_set;
	Gog3DBox *box;

	g_return_val_if_fail (GOG_IS_CHART (chart), NULL);

	map = g_new (GogChartMap3D, 1);

	g_object_ref (chart);
	map->chart = chart;
	map->area = *area;
	map->data = NULL;
	map->is_valid = FALSE;
	box = GOG_3D_BOX (gog_object_get_child_by_name (GOG_OBJECT (chart), "3D-Box"));

	axis_set = gog_chart_get_axis_set (chart);
	switch (axis_set & GOG_AXIS_SET_FUNDAMENTAL) {
	case GOG_AXIS_SET_XYZ: {
		map->axis_map[0] = gog_axis_map_new (axis0, -box->dx, 2 * box->dx);
		map->axis_map[1] = gog_axis_map_new (axis1, -box->dy, 2 * box->dy);
		map->axis_map[2] = gog_axis_map_new (axis2, -box->dz, 2 * box->dz);

		map->data = NULL;
		map->map_3D_to_view = xyz_map_3D_to_view;

		map->is_valid = gog_axis_map_is_valid (map->axis_map[0]) &&
			gog_axis_map_is_valid (map->axis_map[1]) &&
			gog_axis_map_is_valid (map->axis_map[2]);

		break;
	}
	default:
		g_warning ("[Chart3D::map_new] not implemented for this axis set (%i)",
			   axis_set);
		map->map_3D_to_view = null_map_3D;
		break;
	}

	return map;
}

/**
 * gog_chart_map_3d_to_view:
 * @map: a #GogChartMap3D
 * @x: data x value
 * @y: data y value
 * @z: data y value
 * @u: placeholder for x converted value
 * @v: placeholder for y converted value
 * @w: placeholder for z converted value
 *
 * Converts a 3D coordinate from data space to canvas space.
 **/
void
gog_chart_map_3d_to_view (GogChartMap3D *map, double x, double y, double z, double *u, double *v, double *w)
{
	(map->map_3D_to_view) (map, x, y, z, u, v, w);
}

/**
 * gog_chart_map_3d_get_axis_map:
 * @map: a #GogChartMap3D
 * @index: axis index
 *
 * Convenience function which returns one of the associated axis_map.
 *
 * Valid values are in range [0..2].
 *
 * returns: a #GogAxisMap.
 **/

GogAxisMap *
gog_chart_map_3d_get_axis_map (GogChartMap3D *map, unsigned int i)
{
	g_return_val_if_fail (map != NULL, NULL);
	g_return_val_if_fail (i < 3, NULL);

	return map->axis_map[i];
}

/**
 * gog_chart_map_3d_is_valid:
 * @map: a #GogChartMap3D
 *
 * Tests if @map was correctly initialized.
 *
 * returns: %TRUE if @map is valid.
 **/

gboolean
gog_chart_map_3d_is_valid (GogChartMap3D *map)
{
	g_return_val_if_fail (map != NULL, FALSE);

	return map->is_valid;
}

/**
 * gog_chart_map_3d_free:
 * @map: a #GogChartMap3D
 *
 * Frees @map object.
 **/

void
gog_chart_map_3d_free (GogChartMap3D *map)
{
	int i;

	g_return_if_fail (map != NULL);

	for (i = 0; i < 3; i++)
		if (map->axis_map[i] != NULL)
			gog_axis_map_free (map->axis_map[i]);

	g_free (map->data);
	g_object_unref (map->chart);
	g_free (map);
}
