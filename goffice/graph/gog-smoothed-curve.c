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
#include <goffice/goffice.h>

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

static GType gog_smoothed_curve_view_get_type (void);

static GObjectClass *smoothed_curve_parent_klass;

static void
gog_smoothed_curve_init_style (GogStyledObject *gso, GOStyle *style)
{
	style->interesting_fields = GO_STYLE_LINE;
	gog_theme_fillin_style (gog_object_get_theme (GOG_OBJECT (gso)),
		style, GOG_OBJECT (gso), 0, GO_STYLE_LINE);
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

GSF_CLASS_ABSTRACT (GogSmoothedCurve, gog_smoothed_curve,
	   gog_smoothed_curve_class_init, gog_smoothed_curve_init,
	   GOG_TYPE_TREND_LINE)

/****************************************************************************/

typedef GogView		GogSmoothedCurveView;
typedef GogViewClass	GogSmoothedCurveViewClass;

#define GOG_TYPE_SMOOTHED_CURVE_VIEW	(gog_smoothed_curve_view_get_type ())
#define GOG_SMOOTHED_CURVE_VIEW(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_SMOOTHED_CURVE_VIEW, GogSmoothedCurveView))
#define GOG_IS_SMOOTHED_CURVE_VIEW(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_SMOOTHED_CURVE_VIEW))

/*static GogViewClass *smoothed_curve_view_parent_klass; */

static void
gog_smoothed_curve_view_render (GogView *view, GogViewAllocation const *bbox)
{
	GogSmoothedCurve *curve = GOG_SMOOTHED_CURVE (view->model);
	GogSeries *series = GOG_SERIES ((GOG_OBJECT (curve))->parent);
	GogPlot *plot = series->plot;
	GogChart *chart = GOG_CHART (GOG_OBJECT (plot)->parent);
	GogChartMap *chart_map;
	GOStyle *style;
	GOPath *path;

	if (curve->nb == 0 || curve->x == NULL || curve->y == NULL)
		return;

	chart_map = gog_chart_map_new (chart, &view->residual,
				       plot->axis[GOG_AXIS_X],
				       plot->axis[GOG_AXIS_Y],
				       NULL, FALSE);
	if (!gog_chart_map_is_valid (chart_map)) {
		gog_chart_map_free (chart_map);
		return;
	}

	gog_renderer_push_clip_rectangle (view->renderer, view->residual.x, view->residual.y,
					  view->residual.w, view->residual.h);

	path = gog_chart_map_make_path (chart_map, curve->x, curve->y, curve->nb, GO_LINE_INTERPOLATION_LINEAR, FALSE, NULL);
	style = GOG_STYLED_OBJECT (curve)->style;
	gog_renderer_push_style (view->renderer, style);
	gog_renderer_stroke_serie (view->renderer, path);
	gog_renderer_pop_style (view->renderer);
	go_path_free (path);
	gog_renderer_pop_clip (view->renderer);

	gog_chart_map_free (chart_map);
}

static void
gog_smoothed_curve_view_class_init (GogSmoothedCurveViewClass *gview_klass)
{
	GogViewClass *view_klass    = (GogViewClass *) gview_klass;

	view_klass->render	  = gog_smoothed_curve_view_render;
}

static GSF_CLASS (GogSmoothedCurveView, gog_smoothed_curve_view,
		  gog_smoothed_curve_view_class_init, NULL,
		  GOG_TYPE_VIEW)
