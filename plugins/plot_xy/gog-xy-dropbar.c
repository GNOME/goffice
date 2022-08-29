/*
 * gog-dropbar.c
 *
 * Copyright (C) 2005
 *	Jean Brefort (jean.brefort@normalesup.org)
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
#include "gog-xy-dropbar.h"

#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>

static GogObjectClass *gog_xy_dropbar_parent_klass;

static GType gog_xy_dropbar_view_get_type (void);
static GType gog_xy_dropbar_series_get_type (void);

enum {
	XY_DROPBAR_PROP_FILL_0,
	XY_DROPBAR_PROP_FILL_BEFORE_GRID,
	XY_DROPBAR_PROP_HORIZONTAL,
	XY_DROPBAR_PROP_WIDTH
};

static void
gog_xy_dropbar_plot_clear_formats (GogXYDropBarPlot *plot)
{
	go_format_unref (plot->x.fmt);
	plot->x.fmt = NULL;

	go_format_unref (plot->y.fmt);
	plot->y.fmt = NULL;
}

static void
gog_xy_dropbar_set_property (GObject *obj, guint param_id,
		     GValue const *value, GParamSpec *pspec)
{
	GogPlot *plot = GOG_PLOT (obj);
	GogXYDropBarPlot *dropbar = GOG_XY_DROPBAR_PLOT (obj);
	switch (param_id) {
	case XY_DROPBAR_PROP_FILL_BEFORE_GRID:
		plot->rendering_order = (g_value_get_boolean (value))?
					 GOG_PLOT_RENDERING_BEFORE_GRID:
					 GOG_PLOT_RENDERING_LAST;
		break;
	case XY_DROPBAR_PROP_HORIZONTAL:
		dropbar->horizontal = g_value_get_boolean (value);
		break;
	case XY_DROPBAR_PROP_WIDTH:
		dropbar->width = g_value_get_double (value);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return;
	}
	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
}

static void
gog_xy_dropbar_get_property (GObject *obj, guint param_id,
		     GValue *value, GParamSpec *pspec)
{
	GogPlot *plot = GOG_PLOT (obj);
	GogXYDropBarPlot *dropbar = GOG_XY_DROPBAR_PLOT (obj);

	switch (param_id) {
	case XY_DROPBAR_PROP_FILL_BEFORE_GRID:
		g_value_set_boolean (value, plot->rendering_order == GOG_PLOT_RENDERING_BEFORE_GRID);
		break;
	case XY_DROPBAR_PROP_HORIZONTAL:
		g_value_set_boolean (value, dropbar->horizontal);
		break;
	case XY_DROPBAR_PROP_WIDTH:
		g_value_set_double (value, dropbar->width);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
gog_xy_dropbar_finalize (GObject *obj)
{
	gog_xy_dropbar_plot_clear_formats (GOG_XY_DROPBAR_PLOT (obj));
	G_OBJECT_CLASS (gog_xy_dropbar_parent_klass)->finalize (obj);
}

#ifdef GOFFICE_WITH_GTK
static void
display_before_grid_cb (GtkToggleButton *btn, GObject *obj)
{
	g_object_set (obj, "before-grid", gtk_toggle_button_get_active (btn), NULL);
}

static void
horizontal_cb (GtkToggleButton *btn, GObject *obj)
{
	g_object_set (obj, "horizontal", gtk_toggle_button_get_active (btn), NULL);
}

static void
value_changed_cb (GtkSpinButton *btn, GObject *obj)
{
	g_object_set (obj, "width", gtk_spin_button_get_value (btn), NULL);
}
#endif

static void
gog_xy_dropbar_populate_editor (GogObject *obj,
			     GOEditor *editor,
                             GogDataAllocator *dalloc,
                             GOCmdContext *cc)
{
#ifdef GOFFICE_WITH_GTK
	GogXYDropBarPlot *dropbar = GOG_XY_DROPBAR_PLOT (obj);
	GtkBuilder *gui =
		go_gtk_builder_load ("res:go:plot_xy/gog-xy-dropbar-prefs.ui",
				    GETTEXT_PACKAGE, cc);

	if (gui != NULL) {
		GtkWidget *w = go_gtk_builder_get_widget (gui, "before-grid");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
				(GOG_PLOT (obj))->rendering_order == GOG_PLOT_RENDERING_BEFORE_GRID);
		g_signal_connect (G_OBJECT (w),
			"toggled",
			G_CALLBACK (display_before_grid_cb), obj);
		w = go_gtk_builder_get_widget (gui, "horizontal");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), dropbar->horizontal);
		g_signal_connect (G_OBJECT (w),
			"toggled",
			G_CALLBACK (horizontal_cb), obj);
		w = go_gtk_builder_get_widget (gui, "width-btn");
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), dropbar->width);
		g_signal_connect (G_OBJECT (w),
			"value_changed",
			G_CALLBACK (value_changed_cb), obj);
		w = go_gtk_builder_get_widget (gui, "gog-xy-dropbar-prefs");
		go_editor_add_page (editor, w, _("Properties"));
		g_object_unref (gui);
	}

#endif
	gog_xy_dropbar_parent_klass->populate_editor (obj, editor, dalloc, cc);
};

static void
gog_xy_dropbar_plot_update (GogObject *obj)
{
	GogXYDropBarPlot *model = GOG_XY_DROPBAR_PLOT (obj);
	GogSeries const *series = NULL;
	double x_min, x_max, y_min, y_max, tmp_min, tmp_max;
	GSList *ptr;
	unsigned xaxis, yaxis;

	if (model->horizontal) {
		xaxis = 1;
		yaxis = 0;
	} else {
		xaxis = 0;
		yaxis = 1;
	}
	x_min = y_min =  DBL_MAX;
	x_max = y_max = -DBL_MAX;
	gog_xy_dropbar_plot_clear_formats (model);
	for (ptr = model->base.series ; ptr != NULL ; ptr = ptr->next) {
		series = ptr->data;
		if (!gog_series_is_valid (GOG_SERIES (series)))
			continue;

		go_data_get_bounds (series->values[1].data, &tmp_min, &tmp_max);
		if (y_min > tmp_min) y_min = tmp_min;
		if (y_max < tmp_max) y_max = tmp_max;
		if (model->y.fmt == NULL) {
			model->y.fmt = go_data_preferred_fmt (series->values[1].data);
			model->y.date_conv = go_data_date_conv (series->values[1].data);
		}
		go_data_get_bounds (series->values[2].data, &tmp_min, &tmp_max);
		if (y_min > tmp_min)
			y_min = tmp_min;
		if (y_max < tmp_max)
			y_max = tmp_max;

		if (series->values[0].data != NULL) {
			go_data_get_bounds (series->values[0].data, &tmp_min, &tmp_max);

			if (!go_finite (tmp_min) || !go_finite (tmp_max) ||
			    tmp_min > tmp_max) {
				tmp_min = 0;
				tmp_max = go_data_get_vector_size (series->values[1].data);
			} else if (model->x.fmt == NULL) {
				model->x.fmt = go_data_preferred_fmt (series->values[0].data);
				model->x.date_conv = go_data_date_conv (series->values[0].data);
			}
		} else {
			tmp_min = 0;
			tmp_max = go_data_get_vector_size (series->values[1].data);
		}

		if (x_min > tmp_min)
			x_min = tmp_min;
		if (x_max < tmp_max)
			x_max = tmp_max;
	}
	/* make room for bar width, this is approximate since real width is larger */
	tmp_max = (x_max - x_min) * model->width / 200.;
	x_min -= tmp_max;
	x_max += tmp_max;
	if (model->x.minima != x_min || model->x.maxima != x_max) {
		model->x.minima = x_min;
		model->x.maxima = x_max;
		gog_axis_bound_changed (model->base.axis[xaxis], GOG_OBJECT (model));
	}
	if (model->y.minima != y_min || model->y.maxima != y_max) {
		model->y.minima = y_min;
		model->y.maxima = y_max;
		gog_axis_bound_changed (model->base.axis[yaxis], GOG_OBJECT (model));
	}
	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
	if (gog_xy_dropbar_parent_klass->update)
		gog_xy_dropbar_parent_klass->update (obj);
}

static GOData *
gog_xy_dropbar_plot_axis_get_bounds (GogPlot *plot, GogAxisType axis,
			     GogPlotBoundInfo *bounds)
{
	GogXYDropBarPlot *model = GOG_XY_DROPBAR_PLOT (plot);

	if ((model->horizontal && axis == GOG_AXIS_Y) || (!model->horizontal && axis == GOG_AXIS_X)) {
		GSList *ptr;

		bounds->val.minima = model->x.minima;
		bounds->val.maxima = model->x.maxima;
		bounds->is_discrete = model->x.minima > model->x.maxima ||
			!go_finite (model->x.minima) ||
			!go_finite (model->x.maxima);
		if (bounds->fmt == NULL && model->x.fmt != NULL)
			bounds->fmt = go_format_ref (model->x.fmt);
		if (model->x.date_conv)
			bounds->date_conv = model->x.date_conv;

		for (ptr = plot->series; ptr != NULL ; ptr = ptr->next)
			if (gog_series_is_valid (GOG_SERIES (ptr->data)))
				return GOG_SERIES (ptr->data)->values[0].data;
		return NULL;
	}

	if ((model->horizontal && axis == GOG_AXIS_X) || (!model->horizontal && axis == GOG_AXIS_Y)) {
		bounds->val.minima = model->y.minima;
		bounds->val.maxima = model->y.maxima;
		if (bounds->fmt == NULL && model->y.fmt != NULL)
			bounds->fmt = go_format_ref (model->y.fmt);
		if (model->y.date_conv)
			bounds->date_conv = model->y.date_conv;
	}
	return NULL;
}

static char const *
gog_xy_dropbar_plot_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name drop bar/col plot objects
	 * eg The 2nd drop bar/col plot in a chart will be called
	 * 	PlotDropBar2 */
	return N_("PlotXYDropBar");
}

static void
gog_xy_dropbar_plot_class_init (GObjectClass *gobject_klass)
{
	GogObjectClass *gog_object_klass = (GogObjectClass *) gobject_klass;
	GogPlotClass   *plot_klass = (GogPlotClass *) gobject_klass;
	gog_xy_dropbar_parent_klass = g_type_class_peek_parent (gobject_klass);

	gobject_klass->set_property = gog_xy_dropbar_set_property;
	gobject_klass->get_property = gog_xy_dropbar_get_property;
	gobject_klass->finalize = gog_xy_dropbar_finalize;
	g_object_class_install_property (gobject_klass, XY_DROPBAR_PROP_FILL_BEFORE_GRID,
		g_param_spec_boolean ("before-grid",
			_("Displayed under the grids"),
			_("Should the plot be displayed before the grids"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, XY_DROPBAR_PROP_HORIZONTAL,
		g_param_spec_boolean ("horizontal",
			_("Horizontal"),
			_("Whether to use horizontal bars"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, XY_DROPBAR_PROP_WIDTH,
		g_param_spec_double ("width",
			_("Width"),
			_("Bars width as a percentage of the plot width"),
			0., 20., 5.,   /* using arbitrarily 20%. as maximum value */
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	gog_object_klass->type_name	= gog_xy_dropbar_plot_type_name;
	gog_object_klass->update	= gog_xy_dropbar_plot_update;
	gog_object_klass->view_type	= gog_xy_dropbar_view_get_type ();
	gog_object_klass->populate_editor = gog_xy_dropbar_populate_editor;

	{
		static GogSeriesDimDesc dimensions[] = {
			{ N_("Positions"), GOG_SERIES_SUGGESTED, FALSE,
			  GOG_DIM_INDEX, GOG_MS_DIM_CATEGORIES },
			{ N_("Start"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_START },
			{ N_("End"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_END },
		};
		plot_klass->desc.series.dim = dimensions;
		plot_klass->desc.series.num_dim = G_N_ELEMENTS (dimensions);
		plot_klass->desc.series.style_fields = GO_STYLE_LINE | GO_STYLE_FILL;
	}
	plot_klass->series_type  	= gog_xy_dropbar_series_get_type ();
	plot_klass->axis_set	      	= GOG_AXIS_SET_XY;
	plot_klass->axis_get_bounds   	= gog_xy_dropbar_plot_axis_get_bounds;
}

static void
gog_xy_dropbar_plot_init (GogXYDropBarPlot *plot)
{
	plot->width = 5.;
}

GSF_DYNAMIC_CLASS (GogXYDropBarPlot, gog_xy_dropbar_plot,
	gog_xy_dropbar_plot_class_init, gog_xy_dropbar_plot_init,
	GOG_TYPE_PLOT)

/*****************************************************************************/
typedef GogPlotView		GogXYDropBarView;
typedef GogPlotViewClass	GogXYDropBarViewClass;

/**
 * FIXME FIXME FIXME Wrong description
 * barcol_draw_rect :
 * @rend: #GogRenderer
 * @flip:
 * @base: #GogViewAllocation
 * @rect: #GogViewAllocation
 *
 * A utility routine to build a vpath in @rect.  @rect is assumed to be in
 * coordinates relative to @base with 0,0 as the upper left.  @flip transposes
 * @rect and rotates it to put the origin in the bottom left.  Play fast and
 * loose with coordinates to keep widths >= 1.  If we allow things to be less
 * the background bleeds through.
 **/
static void
barcol_draw_rect (GogRenderer *rend, gboolean flip,
		  GogAxisMap *x_map,
		  GogAxisMap *y_map,
		  GogViewAllocation const *rect)
{
	GogViewAllocation r;
	double t;

	if (flip) {
		r.x = gog_axis_map_to_view (x_map, rect->y);
		t = gog_axis_map_to_view (x_map, rect->y + rect->h);
		if (t > r.x)
			r.w = t - r.x;
		else {
			r.w = r.x - t;
			r.x = t;
		}
		r.h = rect->w;
		r.y = gog_axis_map_to_view (y_map, rect->x) - r.h / 2.;
	} else {
		r.w = rect->w;
		r.x = gog_axis_map_to_view (x_map, rect->x) - r.w / 2.;
		r.y = gog_axis_map_to_view (y_map, rect->y);
		t = gog_axis_map_to_view (y_map, rect->y + rect->h);
		if (t > r.y)
			r.h = t - r.y;
		else {
			r.h = r.y - t;
			r.y = t;
		}
	}
	if (fabs (r.w) < 1.) {
		r.w += 1.;
		r.x -= .5;
	}
	if (fabs (r.h) < 1.) {
		r.h += 1.;
		r.y -= .5;
	}
	gog_renderer_draw_rectangle (rend, &r);
}

static void
gog_xy_dropbar_view_render (GogView *view, GogViewAllocation const *bbox)
{
	GogXYDropBarPlot const *model = GOG_XY_DROPBAR_PLOT (view->model);
	GogPlot *plot = GOG_PLOT (model);
	GogSeries const *series;
	GogAxisMap *x_map, *y_map, *val_map, *index_map;
	GogViewAllocation work;
	double *pos_vals, *start_vals, *end_vals;
	unsigned i;
	GSList *ptr;
	unsigned n, tmp, num_series;
	GOStyle *neg_style;

	for (num_series = 0, ptr = plot->series ; ptr != NULL ; ptr = ptr->next, num_series++);
	if (num_series < 1)
		return;

	x_map = gog_axis_map_new (GOG_PLOT (model)->axis[0],
				  view->allocation.x, view->allocation.w);
	y_map = gog_axis_map_new (GOG_PLOT (model)->axis[1], view->allocation.y + view->allocation.h,
				  -view->allocation.h);

	if (!(gog_axis_map_is_valid (x_map) &&
	      gog_axis_map_is_valid (y_map))) {
		gog_axis_map_free (x_map);
		gog_axis_map_free (y_map);
		return;
	}

	work.w = view->allocation.w * model->width / 100.;
	for (ptr = plot->series ; ptr != NULL ; ptr = ptr->next) {
		series = ptr->data;
		if (!gog_series_is_valid (GOG_SERIES (series)))
			continue;
		neg_style = go_style_dup ((GOG_STYLED_OBJECT (series))->style);
		neg_style->line.color ^= 0xffffff00;
		neg_style->fill.pattern.back ^= 0xffffff00;
		neg_style->fill.pattern.fore ^= 0xffffff00;
		pos_vals = go_data_get_values (series->values[0].data);
		n = go_data_get_vector_size (series->values[0].data);
		start_vals = go_data_get_values (series->values[1].data);
		tmp = go_data_get_vector_size (series->values[1].data);
		if (n > tmp)
			n = tmp;
		end_vals = go_data_get_values (series->values[2].data);
		tmp = go_data_get_vector_size (series->values[2].data);
		if (n > tmp)
			n = tmp;

		if (model->horizontal) {
			index_map = y_map;
			val_map = x_map;
		} else {
			index_map = x_map;
			val_map = y_map;
		}
		for (i = 0; i < n; i++) {
			work.x = pos_vals[i];
			work.y = start_vals[i];
			work.h = end_vals[i] - work.y;
			if (!gog_axis_map_finite (index_map, work.x) ||
				!gog_axis_map_finite (val_map, start_vals[i]) ||
				!gog_axis_map_finite (val_map, end_vals[i]))
				continue;
			gog_renderer_push_style (view->renderer, (start_vals[i] <= end_vals[i])?
						GOG_STYLED_OBJECT (series)->style: neg_style);
			barcol_draw_rect (view->renderer, model->horizontal, x_map, y_map, &work);
			gog_renderer_pop_style (view->renderer);
		}
		g_object_unref (neg_style);
	}

	gog_axis_map_free (x_map);
	gog_axis_map_free (y_map);
}

static void
gog_xy_dropbar_view_class_init (GogViewClass *view_klass)
{
	view_klass->render	  = gog_xy_dropbar_view_render;
	view_klass->clip	  = TRUE;
}

GSF_DYNAMIC_CLASS (GogXYDropBarView, gog_xy_dropbar_view,
	gog_xy_dropbar_view_class_init, NULL,
	GOG_TYPE_PLOT_VIEW)


/*****************************************************************************/
typedef GogSeries GogXYDropBarSeries;
typedef GogSeriesClass GogXYDropBarSeriesClass;

static GogObjectClass *series_parent_klass;

static void
gog_xy_dropbar_series_update (GogObject *obj)
{
	const double *x_vals, *y_vals, *z_vals;
	GogSeries *series = GOG_SERIES (obj);
	unsigned old_num = series->num_elements;

	series->num_elements = gog_series_get_xyz_data (series,
							&x_vals, &y_vals, &z_vals);
	/* queue plot for redraw */
	gog_object_request_update (GOG_OBJECT (series->plot));
	if (old_num != series->num_elements)
		gog_plot_request_cardinality_update (series->plot);

	if (series_parent_klass->update)
		series_parent_klass->update (obj);
}

static void
gog_xy_dropbar_series_class_init (GogObjectClass *gog_klass)
{
	series_parent_klass = g_type_class_peek_parent (gog_klass);
	gog_klass->update = gog_xy_dropbar_series_update;
}

GSF_DYNAMIC_CLASS (GogXYDropBarSeries, gog_xy_dropbar_series,
	gog_xy_dropbar_series_class_init, NULL,
	GOG_TYPE_SERIES)
