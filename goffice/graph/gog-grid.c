/*
 * gog-grid.c
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

#include <goffice/goffice-config.h>
#include <goffice/graph/gog-grid.h>
#include <goffice/graph/gog-chart.h>
#include <goffice/graph/gog-chart-map.h>
#include <goffice/graph/gog-chart-map-3d.h>
#include <goffice/utils/go-styled-object.h>
#include <goffice/utils/go-style.h>
#include <goffice/graph/gog-view.h>
#include <goffice/graph/gog-theme.h>
#include <goffice/graph/gog-renderer.h>
#include <goffice/utils/go-persist.h>

#include <goffice/math/go-math.h>

#include <glib/gi18n-lib.h>

#include <gsf/gsf-impl-utils.h>

#ifdef GOFFICE_WITH_GTK
#include <gtk/gtk.h>
#endif

/**
 * GogGridType:
 * @GOG_GRID_UNKNOWN: unkown, should not occur.
 * @GOG_GRID_XY: XY plane.
 * @GOG_GRID_YZ: YZ plane.
 * @GOG_GRID_ZX: ZY plane.
 * @GOG_GRID_TYPES: last defined, should not occur.
 *
 * Used for base planes in 3d plots.
 **/

struct _GogGrid {
	GogStyledObject	base;
	GogGridType type;
};
typedef GogStyledObjectClass GogGridClass;

static GType gog_grid_view_get_type (void);
static GObjectClass *parent_klass;
static GogViewClass *gview_parent_klass;

enum {
	GRID_PROP_0,
	GRID_PROP_TYPE,
};

static void
gog_grid_init_style (GogStyledObject *gso, GOStyle *style)
{
	style->interesting_fields = GO_STYLE_FILL | GO_STYLE_OUTLINE;
	gog_theme_fillin_style (gog_object_get_theme (GOG_OBJECT (gso)),
		style, GOG_OBJECT (gso), 0, GO_STYLE_FILL | GO_STYLE_OUTLINE);
}

GogGridType
gog_grid_get_gtype (GogGrid const *grid)
{
	g_return_val_if_fail (GOG_IS_GRID (grid), GOG_GRID_UNKNOWN);
	return grid->type;
}

void
gog_grid_set_gtype (GogGrid *grid, GogGridType type)
{
	g_return_if_fail (GOG_IS_GRID (grid));
	grid->type = type;
}

static void
gog_grid_set_property (GObject *obj, guint param_id,
		       GValue const *value, GParamSpec *pspec)
{
	GogGrid *grid = GOG_GRID (obj);

	if (param_id == GRID_PROP_TYPE)
		grid->type = g_value_get_int (value);
	else
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
}

static void
gog_grid_get_property (GObject *obj, guint param_id,
		       GValue *value, GParamSpec *pspec)
{
	GogGrid const *grid = GOG_GRID (obj);

	if (param_id == GRID_PROP_TYPE)
		g_value_set_int (value, grid->type);
	else
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
}

static void
gog_grid_class_init (GObjectClass *gobject_klass)
{
	GogObjectClass *gog_klass = (GogObjectClass *) gobject_klass;
	GogStyledObjectClass *style_klass = (GogStyledObjectClass *) gog_klass;

	parent_klass = g_type_class_peek_parent (gobject_klass);
	gobject_klass->set_property = gog_grid_set_property;
	gobject_klass->get_property = gog_grid_get_property;

	g_object_class_install_property (gobject_klass, GRID_PROP_TYPE,
		g_param_spec_int ("type", _("Type"),
			_("Numerical type of this backplane"),
			GOG_GRID_UNKNOWN, GOG_GRID_TYPES, GOG_GRID_UNKNOWN,
			GSF_PARAM_STATIC | G_PARAM_READWRITE
			| GO_PARAM_PERSISTENT));

	gog_klass->view_type	= gog_grid_view_get_type ();
	style_klass->init_style = gog_grid_init_style;
}

static void
gog_grid_init (GogGrid *grid)
{
	grid->type = GOG_GRID_UNKNOWN;
}

GSF_CLASS (GogGrid, gog_grid,
	   gog_grid_class_init, gog_grid_init,
	   GOG_TYPE_STYLED_OBJECT)

/************************************************************************/

typedef GogView		GogGridView;
typedef GogViewClass	GogGridViewClass;

#define GOG_TYPE_GRID_VIEW	(gog_grid_view_get_type ())
#define GOG_GRID_VIEW(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_GRID_VIEW, GogGridView))
#define GOG_IS_GRID_VIEW(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_GRID_VIEW))


static void
gog_grid_view_xyz_render (GogGrid *grid, GogView *view,
			  GogChart *chart, GogViewAllocation const *plot_area)
{
	GogAxisMap *a_map = NULL;
	GOPath *path;
	unsigned int i;
	GogAxis *xaxis, *yaxis, *zaxis;
	GSList *axes;
	GogChartMap3D *c_map;
	double ax, ay, az, bx, by, bz;
	double *px[] = {&ax, &ax, &bx, &bx, &ax, &ax, &bx, &bx};
	double *py[] = {&ay, &by, &by, &ay, &ay, &by, &by, &ay};
	double *pz[] = {&az, &az, &az, &az, &bz, &bz, &bz, &bz};
	double rx[8], ry[8], rz[8];

	/* Note: Anti-clockwise order in each face,
	 * important for calculating normals */
	const int faces[] = {
		3, 2, 1, 0, /* Bottom */
		4, 5, 6, 7, /* Top */
		0, 1, 5, 4, /* Left */
		2, 3, 7, 6, /* Right */
		1, 2, 6, 5, /* Front */
		0, 4, 7, 3  /* Back */
	};
	int bp_faces[] = {0, 0};
	int fv[] = {0, 0, 0, 0, 0, 0};

	axes  = gog_chart_get_axes (chart, GOG_AXIS_X);
	xaxis = GOG_AXIS (axes->data);
	g_slist_free (axes);
	axes  = gog_chart_get_axes (chart, GOG_AXIS_Y);
	yaxis = GOG_AXIS (axes->data);
	g_slist_free (axes);
	axes  = gog_chart_get_axes (chart, GOG_AXIS_Z);
	zaxis = GOG_AXIS (axes->data);
	g_slist_free (axes);
	c_map = gog_chart_map_3d_new (view, plot_area,
		xaxis, yaxis, zaxis);

	a_map = gog_chart_map_3d_get_axis_map (c_map, 0);
	gog_axis_map_get_bounds (a_map, &ax, &bx);
	a_map = gog_chart_map_3d_get_axis_map (c_map, 1);
	gog_axis_map_get_bounds (a_map, &ay, &by);
	a_map = gog_chart_map_3d_get_axis_map (c_map, 2);
	gog_axis_map_get_bounds (a_map, &az, &bz);

	/* Projecting vertices */
	for (i = 0; i < 8; ++i)
		gog_chart_map_3d_to_view (c_map, *px[i], *py[i], *pz[i],
		                          &rx[i], &ry[i], &rz[i]);
	gog_chart_map_3d_free (c_map);

	/* Determining visibility of each face */
	for (i = 0; i < 24; i += 4) {
		int A = faces[i];
		int B = faces[i + 1];
		int C = faces[i + 3];
		double tmp = (rx[B] - rx[A]) * (ry[C] - ry[A])
		           - (ry[B] - ry[A]) * (rx[C] - rx[A]);
		if (tmp < 0)
			fv[i / 4] = 1;
	}

	switch (grid->type) {
	case GOG_GRID_XY:
		bp_faces[0] = 0;
		bp_faces[1] = 4;
		break;
	case GOG_GRID_YZ:
		bp_faces[0] = 8;
		bp_faces[1] = 12;
		break;
	case GOG_GRID_ZX:
		bp_faces[0] = 16;
		bp_faces[1] = 20;
		break;
	default:
		return;
	}

	path = go_path_new ();
	go_path_set_options (path, GO_PATH_OPTIONS_SHARP);
	for (i = 0; i < 2; ++i) {
		int face = bp_faces[i];
		if (fv[face / 4] == 0)
			continue;
		go_path_move_to (path, rx[faces[face]],
		                 ry[faces[face]]);
		go_path_line_to (path, rx[faces[face + 1]],
		                 ry[faces[face + 1]]);
		go_path_line_to (path, rx[faces[face + 2]],
		                 ry[faces[face + 2]]);
		go_path_line_to (path, rx[faces[face + 3]],
		                 ry[faces[face + 3]]);
		go_path_close (path);
	}
	gog_renderer_draw_shape (view->renderer, path);
	go_path_free (path);
}

static void
gog_grid_view_render (GogView *view, GogViewAllocation const *bbox)
{
	GogGrid *grid = GOG_GRID (view->model);
	GogChart *chart = GOG_CHART (gog_object_get_parent (view->model));

	gog_renderer_push_style (view->renderer, grid->base.style);
	switch (gog_chart_get_axis_set (chart)) {
		case GOG_AXIS_SET_X:
		case GOG_AXIS_SET_XY:
		case GOG_AXIS_SET_XY_COLOR:
		case GOG_AXIS_SET_XY_BUBBLE: {
			GOPath *path;

			path = go_path_new ();
			go_path_rectangle (path, view->allocation.x, view->allocation.y,
					   view->allocation.w, view->allocation.h);
			go_path_set_options (path, GO_PATH_OPTIONS_SHARP);
			gog_renderer_draw_shape (view->renderer, path);
			go_path_free (path);
			break;
			}
		case GOG_AXIS_SET_RADAR: {
			GogAxis *c_axis, *r_axis;
			GogViewAllocation const *area = gog_chart_view_get_plot_area (view->parent);
			GogChartMap *c_map;
			GogAxisMap *map;
			GogChartMapPolarData *parms;
			GOPath *path;
			GSList *axis_list;
			double position, start, stop;
			double x, y;
			unsigned step_nbr, i;

			axis_list = gog_chart_get_axes (chart, GOG_AXIS_CIRCULAR);
			if (axis_list == NULL)
				break;
			c_axis = GOG_AXIS (axis_list->data);
			g_slist_free (axis_list);

			axis_list = gog_chart_get_axes (chart, GOG_AXIS_RADIAL);
			if (axis_list == NULL)
				break;
			r_axis = GOG_AXIS (axis_list->data);
			g_slist_free (axis_list);

			c_map = gog_chart_map_new (chart, area, c_axis, r_axis, NULL, FALSE);
			parms = gog_chart_map_get_polar_parms (c_map);
			map = gog_chart_map_get_axis_map (c_map, 1);
			gog_axis_map_get_extents (map, &start, &position);

			path = go_path_new ();

			if (gog_axis_is_discrete (c_axis)) {
				map = gog_chart_map_get_axis_map (c_map, 0);
				gog_axis_map_get_extents (map, &start, &stop);
				step_nbr = go_rint (parms->th1 - parms->th0) + 1;
				for (i = 0; i <= step_nbr; i++) {
					gog_chart_map_2D_to_view (c_map, i + parms->th0, position, &x, &y);
					if (i == 0)
						go_path_move_to (path, x, y);
					else
						go_path_line_to (path, x, y);
				}
			} else {
				double a = gog_axis_map (map, position);
				go_path_pie_wedge (path, parms->cx, parms->cy,
						   parms->rx * a, parms->ry * a,
						   -parms->th1, -parms->th0);
			}
			gog_renderer_draw_shape (view->renderer, path);
			go_path_free (path);
			gog_chart_map_free (c_map);
			break;
					 }
		case GOG_AXIS_SET_XYZ:
			gog_grid_view_xyz_render (grid, view, chart,
			                          &view->allocation);
			break;
		case GOG_AXIS_SET_XY_pseudo_3d:
		case GOG_AXIS_SET_FUNDAMENTAL:
		case GOG_AXIS_SET_ALL:
		case GOG_AXIS_SET_UNKNOWN:
		case GOG_AXIS_SET_NONE:
					 break;
	}
	gog_renderer_pop_style (view->renderer);

	(gview_parent_klass->render) (view, bbox);
}

static void
gog_grid_view_class_init (GogGridViewClass *gview_klass)
{
	GogViewClass *view_klass    = (GogViewClass *) gview_klass;

	gview_parent_klass = g_type_class_peek_parent (gview_klass);
	view_klass->render = gog_grid_view_render;
}

static GSF_CLASS (GogGridView, gog_grid_view,
	   gog_grid_view_class_init, NULL,
	   GOG_TYPE_VIEW)
