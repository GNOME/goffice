/*
 * gog-smoothed-curve.c :
 *
 * Copyright (C) 2006 Jean Brefort (jean.brefort@normalesup.org)
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
#include <goffice/goffice.h>

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

static GType gog_smoothed_curve_view_get_type (void);

static GObjectClass *smoothed_curve_parent_klass;

/**
 * GogSmoothedCurveClass:
 * @base: base class.
 * @max_dim: number of #GOData parameters.
 *
 * Base class for smoothed curves.
 **/
#ifdef GOFFICE_WITH_GTK
static void
gog_smoothed_curve_populate_editor (GogObject	*gobj,
			       GOEditor	*editor,
			       GogDataAllocator	*dalloc,
			       GOCmdContext	*cc)
{
	GtkWidget *w, *child;
	GtkGrid *grid;
	GogDataset *set = GOG_DATASET (gobj);

	w = gtk_grid_new ();
	grid = GTK_GRID (w);
	g_object_set (G_OBJECT (grid), "margin", 12, "column-spacing", 12,
	              "orientation", GTK_ORIENTATION_VERTICAL, NULL);
	child = gtk_label_new (_("(Name):"));
	gtk_grid_attach (grid, child, 0, 0, 1, 1);
	child = GTK_WIDGET (gog_data_allocator_editor (dalloc, set, -1, GOG_DATA_SCALAR));
	g_object_set (G_OBJECT (child), "hexpand", TRUE, NULL);
	gtk_grid_attach (grid, child, 1, 0, 1, 1);
	gtk_widget_show_all (w);
	go_editor_add_page (editor, w, _("Details"));

	(GOG_OBJECT_CLASS (smoothed_curve_parent_klass)->populate_editor) (gobj, editor, dalloc, cc);
}
#endif

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
	if (curve->name != NULL) {
		gog_dataset_finalize (GOG_DATASET (obj));
		g_free (curve->name); /* aliased pointer */
		curve->name = NULL;
	}
	(*smoothed_curve_parent_klass->finalize) (obj);
}

static char const *
gog_smoothed_curve_type_name (GogObject const *gobj)
{
	return N_("Smoothed Curve");
}

static void
gog_smoothed_curve_parent_changed (GogObject *obj, G_GNUC_UNUSED gboolean was_set)
{
	GogSmoothedCurve *sc = GOG_SMOOTHED_CURVE (obj);
	if (sc->name == NULL) {
		GogSmoothedCurveClass *klass = (GogSmoothedCurveClass *) G_OBJECT_GET_CLASS (obj);
		sc->name = g_new0 (GogDatasetElement, klass->max_dim + 2);
	}
	GOG_OBJECT_CLASS (smoothed_curve_parent_klass)->parent_changed (obj, was_set);
}

static void
gog_smoothed_curve_class_init (GogObjectClass *gog_klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) gog_klass;
	GogStyledObjectClass *style_klass = (GogStyledObjectClass *) gog_klass;
	GogSmoothedCurveClass *curve_klass = (GogSmoothedCurveClass *) gog_klass;
	smoothed_curve_parent_klass = g_type_class_peek_parent (gog_klass);

	gobject_klass->finalize = gog_smoothed_curve_finalize;

	curve_klass->max_dim = -1;
	style_klass->init_style = gog_smoothed_curve_init_style;
	gog_klass->type_name	= gog_smoothed_curve_type_name;
	gog_klass->parent_changed = gog_smoothed_curve_parent_changed;
	gog_klass->view_type	= gog_smoothed_curve_view_get_type ();

#ifdef GOFFICE_WITH_GTK
	gog_klass->populate_editor  = gog_smoothed_curve_populate_editor;
#endif
}

static void
gog_smoothed_curve_init (GogSmoothedCurve *curve)
{
	curve->nb = 0;
	curve->x = curve->y = NULL;
}

static void
gog_smoothed_curve_dataset_dims (GogDataset const *set, int *first, int *last)
{
	GogSmoothedCurveClass *klass = (GogSmoothedCurveClass *) G_OBJECT_GET_CLASS (set);

	*first = -1;
	*last = klass->max_dim;
}

static GogDatasetElement *
gog_smoothed_curve_dataset_get_elem (GogDataset const *set, int dim_i)
{
	GogSmoothedCurve const *sc = GOG_SMOOTHED_CURVE (set);
	GogSmoothedCurveClass *klass = (GogSmoothedCurveClass *) G_OBJECT_GET_CLASS (set);
	g_return_val_if_fail (dim_i >= -1 && dim_i <= klass->max_dim, NULL);
	dim_i++;
	return sc->name + dim_i;
}

static void
gog_smoothed_curve_dataset_dim_changed (GogDataset *set, int dim_i)
{
	GogSmoothedCurveClass *klass = (GogSmoothedCurveClass *) G_OBJECT_GET_CLASS (set);
	g_return_if_fail (dim_i >= -1 && dim_i <= klass->max_dim);
	{
		GogSmoothedCurve const *sc = GOG_SMOOTHED_CURVE (set);
		if (dim_i == -1) {
			GOData *name_src = sc->name->data;
			char *name = (name_src != NULL)
				? go_data_get_scalar_string (name_src) : NULL;
			gog_object_set_name (GOG_OBJECT (set), name, NULL);
		} else
			gog_object_request_update (GOG_OBJECT (set));
	}
}

static void
gog_smoothed_curve_dataset_init (GogDatasetClass *iface)
{
	iface->get_elem	   = gog_smoothed_curve_dataset_get_elem;
	iface->dims	   = gog_smoothed_curve_dataset_dims;
	iface->dim_changed	   = gog_smoothed_curve_dataset_dim_changed;
}

GSF_CLASS_FULL (GogSmoothedCurve, gog_smoothed_curve,
	   NULL, NULL, gog_smoothed_curve_class_init, NULL,
           gog_smoothed_curve_init, GOG_TYPE_TREND_LINE, G_TYPE_FLAG_ABSTRACT,
	   GSF_INTERFACE (gog_smoothed_curve_dataset_init, GOG_TYPE_DATASET))

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
