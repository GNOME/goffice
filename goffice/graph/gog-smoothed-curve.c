/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-smoothed-curve.c :  
 *
 * Copyright (C) 2006 Jean Brefort (jean.brefort@normalesup.org)
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
#include "gog-smoothed-curve.h"
#include <goffice/graph/gog-axis.h>
#include <goffice/graph/gog-plot-impl.h>
#include <goffice/graph/gog-plot-engine.h>
#include <goffice/graph/gog-renderer.h>
#include <goffice/graph/gog-series-impl.h>
#include <goffice/graph/gog-style.h>
#include <goffice/graph/gog-theme.h>
#include <goffice/graph/gog-view.h>
#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

static GType gog_smoothed_curve_view_get_type (void);

static GObjectClass *smoothed_curve_parent_klass;

static void
gog_smoothed_curve_init_style (GogStyledObject *gso, GogStyle *style)
{
	style->interesting_fields = GOG_STYLE_LINE;
	gog_theme_fillin_style (gog_object_get_theme (GOG_OBJECT (gso)),
		style, GOG_OBJECT (gso), 0, FALSE);
}

static void
gog_smoothed_curve_finalize (GObject *obj)
{
	GogSmoothedCurve *curve = GOG_SMOOTHED_CURVE (obj);
	g_free (curve->x);
	g_free (curve->y);
	(*smoothed_curve_parent_klass->finalize) (obj);
}

static char const *
gog_smoothed_curve_type_name (GogObject const *gobj)
{
	return N_("Smoothed Curve");
}

static void
gog_smoothed_curve_class_init (GogObjectClass *gog_klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) gog_klass;
	GogStyledObjectClass *style_klass = (GogStyledObjectClass *) gog_klass;
	smoothed_curve_parent_klass = g_type_class_peek_parent (gog_klass);

	gobject_klass->finalize = gog_smoothed_curve_finalize;

	style_klass->init_style = gog_smoothed_curve_init_style;
	gog_klass->type_name	= gog_smoothed_curve_type_name;
	gog_klass->view_type	= gog_smoothed_curve_view_get_type ();
}

static void
gog_smoothed_curve_init (GogSmoothedCurve *curve)
{
	curve->nb = 0;
	curve->x = curve->y = NULL;
}

GSF_CLASS (GogSmoothedCurve, gog_smoothed_curve,
	   gog_smoothed_curve_class_init, gog_smoothed_curve_init,
	   GOG_TREND_LINE_TYPE)

/****************************************************************************/

typedef GogView		GogSmoothedCurveView;
typedef GogViewClass	GogSmoothedCurveViewClass;

#define GOG_SMOOTHED_CURVE_VIEW_TYPE	(gog_smoothed_curve_view_get_type ())
#define GOG_SMOOTHED_CURVE_VIEW(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_SMOOTHED_CURVE_VIEW_TYPE, GogSmoothedCurveView))
#define IS_GOG_SMOOTHED_CURVE_VIEW(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_SMOOTHED_CURVE_VIEW_TYPE))

/*static GogViewClass *smoothed_curve_view_parent_klass; */

static void
gog_smoothed_curve_view_render (GogView *view, GogViewAllocation const *bbox)
{
	GogSmoothedCurve *curve = GOG_SMOOTHED_CURVE (view->model);
	GogSeries *series = GOG_SERIES ((GOG_OBJECT (curve))->parent);
	GogPlot *plot = series->plot;
	GogAxisMap *x_map, *y_map;
	double *x, *y;
	GogStyle *style;
	ArtVpath *path;
	unsigned i;
/*	GSList *ptr; */

	if (curve->nb == 0 || curve->x == NULL || curve->y == NULL)
		return;

	x_map = gog_axis_map_new (plot->axis[0], 
				  view->residual.x , view->residual.w);
	y_map = gog_axis_map_new (plot->axis[1], 
				  view->residual.y + view->residual.h, 
				  -view->residual.h);

	if (!(gog_axis_map_is_valid (x_map) &&
	      gog_axis_map_is_valid (y_map))) {
		gog_axis_map_free (x_map);
		gog_axis_map_free (y_map);
		return;
	}
	
	gog_renderer_push_clip (view->renderer, 
		gog_renderer_get_rectangle_vpath (&view->residual)); 
	
	x = g_new (double, curve->nb);
	y = g_new (double, curve->nb);
	for (i = 0; i < curve->nb; i++) {
		x[i] = gog_axis_map_to_view (x_map, curve->x[i]);
		y[i] = gog_axis_map_to_view (y_map, curve->y[i]);
	}

	path = go_line_build_vpath (x, y, curve->nb);
	style = GOG_STYLED_OBJECT (curve)->style;
	gog_renderer_push_style (view->renderer, style);
	gog_renderer_draw_path (view->renderer, path);

	gog_renderer_pop_style (view->renderer);
	g_free (x);
	g_free (y);
	art_free (path);
	gog_axis_map_free (x_map);
	gog_axis_map_free (y_map);

	gog_renderer_pop_clip (view->renderer);
/*	
	for (ptr = view->children ; ptr != NULL ; ptr = ptr->next)
		gog_view_render	(ptr->data, bbox);*/
}

/*static void
gog_smoothed_curve_view_size_allocate (GogView *view, GogViewAllocation const *allocation)
{
	GSList *ptr;

	for (ptr = view->children; ptr != NULL; ptr = ptr->next)
		gog_view_size_allocate (GOG_VIEW (ptr->data), allocation);
	(smoothed_curve_view_parent_klass->size_allocate) (view, allocation);
}*/

static void
gog_smoothed_curve_view_class_init (GogSmoothedCurveViewClass *gview_klass)
{
	GogViewClass *view_klass    = (GogViewClass *) gview_klass;
/*	smoothed_curve_view_parent_klass = g_type_class_peek_parent (gview_klass); */

	view_klass->render	  = gog_smoothed_curve_view_render;
/*	view_klass->size_allocate = gog_smoothed_curve_view_size_allocate; */
}

static GSF_CLASS (GogSmoothedCurveView, gog_smoothed_curve_view,
		  gog_smoothed_curve_view_class_init, NULL,
		  GOG_VIEW_TYPE)
