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
#include "gog-dropbar.h"
#include <goffice/graph/gog-series-lines.h>
#include <goffice/graph/gog-view.h>
#include <goffice/graph/gog-renderer.h>
#include <goffice/math/go-math.h>
#include <goffice/utils/go-styled-object.h>

#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>

static GogObjectClass *gog_dropbar_parent_klass;

static GType gog_dropbar_view_get_type (void);

enum {
	DROPBAR_PROP_FILL_0,
	DROPBAR_PROP_FILL_BEFORE_GRID
};

static void
gog_dropbar_set_property (GObject *obj, guint param_id,
		     GValue const *value, GParamSpec *pspec)
{
	GogPlot *plot = GOG_PLOT (obj);
	switch (param_id) {
	case DROPBAR_PROP_FILL_BEFORE_GRID:
		plot->rendering_order = (g_value_get_boolean (value))?
					 GOG_PLOT_RENDERING_BEFORE_GRID:
					 GOG_PLOT_RENDERING_LAST;
		gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
gog_dropbar_get_property (GObject *obj, guint param_id,
		     GValue *value, GParamSpec *pspec)
{
	GogPlot *plot = GOG_PLOT (obj);

	switch (param_id) {
	case DROPBAR_PROP_FILL_BEFORE_GRID:
		g_value_set_boolean (value, plot->rendering_order == GOG_PLOT_RENDERING_BEFORE_GRID);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

#ifdef GOFFICE_WITH_GTK
static void
display_before_grid_cb (GtkToggleButton *btn, GObject *obj)
{
	g_object_set (obj, "before-grid", gtk_toggle_button_get_active (btn), NULL);
}
#endif

static void
gog_dropbar_populate_editor (GogObject *obj,
			     GOEditor *editor,
                             GogDataAllocator *dalloc,
                             GOCmdContext *cc)
{
#ifdef GOFFICE_WITH_GTK
	GtkBuilder *gui =
		go_gtk_builder_load ("res:go:plot_barcol/gog-area-prefs.ui",
				    GETTEXT_PACKAGE, cc);
	if (gui != NULL) {
		GtkWidget *w = go_gtk_builder_get_widget (gui, "before-grid");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
				(GOG_PLOT (obj))->rendering_order == GOG_PLOT_RENDERING_BEFORE_GRID);
		g_signal_connect (G_OBJECT (w),
			"toggled",
			G_CALLBACK (display_before_grid_cb), obj);
		w = go_gtk_builder_get_widget (gui, "gog-area-prefs");
		go_editor_add_page (editor, w, _("Properties"));
		g_object_unref (gui);
	}

#endif
	gog_dropbar_parent_klass->populate_editor (obj, editor, dalloc, cc);
};

static char const *
gog_dropbar_plot_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name drop bar/col plot objects
	 * eg The 2nd drop bar/col plot in a chart will be called
	 * 	PlotDropBar2 */
	return N_("PlotDropBar");
}

static void
gog_dropbar_plot_class_init (GObjectClass *gobject_klass)
{
	GogPlot1_5dClass *gog_plot_1_5d_klass = (GogPlot1_5dClass *) gobject_klass;
	GogObjectClass *gog_object_klass = (GogObjectClass *) gobject_klass;
	GogPlotClass   *plot_klass = (GogPlotClass *) gobject_klass;
	gog_dropbar_parent_klass = g_type_class_peek_parent (gobject_klass);

	gobject_klass->set_property = gog_dropbar_set_property;
	gobject_klass->get_property = gog_dropbar_get_property;
	g_object_class_install_property (gobject_klass, DROPBAR_PROP_FILL_BEFORE_GRID,
		g_param_spec_boolean ("before-grid",
			_("Displayed under the grids"),
			_("Should the plot be displayed before the grids"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	gog_object_klass->type_name	= gog_dropbar_plot_type_name;
	gog_object_klass->view_type	= gog_dropbar_view_get_type ();
	gog_object_klass->populate_editor = gog_dropbar_populate_editor;

	{
		static GogSeriesDimDesc dimensions[] = {
			{ N_("Labels"), GOG_SERIES_SUGGESTED, TRUE,
			  GOG_DIM_LABEL, GOG_MS_DIM_CATEGORIES },
			{ N_("Start"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_START },
			{ N_("End"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_END },
		};
		plot_klass->desc.series.dim = dimensions;
		plot_klass->desc.series.num_dim = G_N_ELEMENTS (dimensions);
	}

	gog_plot_1_5d_klass->update_stacked_and_percentage = NULL;
}

static void
gog_dropbar_plot_init (GogPlot1_5d *plot)
{
	plot->support_series_lines = FALSE;
	plot->support_lines = TRUE;
}

GSF_DYNAMIC_CLASS (GogDropBarPlot, gog_dropbar_plot,
	gog_dropbar_plot_class_init, gog_dropbar_plot_init,
	GOG_TYPE_BARCOL_PLOT)

/*****************************************************************************/
typedef GogPlotView		GogDropBarView;
typedef GogPlotViewClass	GogDropBarViewClass;

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
		r.y = gog_axis_map_to_view (y_map, rect->x);
		t = gog_axis_map_to_view (y_map, rect->x + rect->w);
		if (t > r.y)
			r.h = t - r.y;
		else {
			r.h = r.y - t;
			r.y = t;
		}
		if (fabs (r.w) < 1.) {
			r.w += 1.;
			r.x -= .5;
		}
		if (fabs (r.h) < 1.) {
			r.h += 1.;
			r.y -= .5;
		}
	} else {
		r.x = gog_axis_map_to_view (x_map, rect->x);
		t = gog_axis_map_to_view (x_map, rect->x + rect->w);
		if (t > r.x)
			r.w = t - r.x;
		else {
			r.w = r.x - t;
			r.x = t;
		}
		r.y = gog_axis_map_to_view (y_map, rect->y);
		t = gog_axis_map_to_view (y_map, rect->y + rect->h);
		if (t > r.y)
			r.h = t - r.y;
		else {
			r.h = r.y - t;
			r.y = t;
		}
		if (fabs (r.w) < 1.) {
			r.w += 1.;
			r.x -= .5;
		}
		if (fabs (r.h) < 1.) {
			r.h += 1.;
			r.y -= .5;
		}
	}
	gog_renderer_draw_rectangle (rend, &r);
}

static void
gog_dropbar_view_render (GogView *view, GogViewAllocation const *bbox)
{
	GogBarColPlot const *model = GOG_BARCOL_PLOT (view->model);
	GogPlot1_5d const *gog_1_5d_model = GOG_PLOT1_5D (view->model);
	GogSeries1_5d const *series;
	GogSeries const *base_series;
	GogAxisMap *x_map, *y_map, *val_map;
	GogViewAllocation work;
	double *start_vals, *end_vals;
	double x, xmapped;
	double step, offset, group_step;
	unsigned i, j;
	unsigned num_elements = gog_1_5d_model->num_elements;
	unsigned num_series = gog_1_5d_model->num_series;
	GSList *ptr;
	unsigned n, tmp;
	GOStyle *neg_style;
	GOPath **path1, **path2;
	GogObjectRole const *role = NULL;
	GogSeriesLines **lines;
	gboolean prec_valid;

	if (num_elements <= 0 || num_series <= 0)
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

	/* lines, if any will be rendered after the bars, so we build the paths
	and render them at the end */
	path1    = g_alloca (num_series * sizeof (GOPath *));
	path2    = g_alloca (num_series * sizeof (GOPath *));
	lines    = g_alloca (num_series * sizeof (GogSeriesLines *));
	j = 0;
	step = 1. - model->overlap_percentage / 100.;
	group_step = model->gap_percentage / 100.;
	work.w = 1.0 / (1. + ((num_series - 1.0) * step) + group_step);
	step *= work.w;
	offset = - (step * (num_series - 1.0) + work.w) / 2.0;


	for (ptr = gog_1_5d_model->base.series ; ptr != NULL && j < num_series; ptr = ptr->next) {
		series = ptr->data;
		base_series = GOG_SERIES (series);
		if (!gog_series_is_valid (base_series)) {
			path1[j++] = NULL;
			continue;
		}
		prec_valid = FALSE;
		neg_style = go_style_dup ((GOG_STYLED_OBJECT (series))->style);
		neg_style->line.color ^= 0xffffff00;
		neg_style->fill.pattern.back ^= 0xffffff00;
		neg_style->fill.pattern.fore ^= 0xffffff00;
		x = offset;
		start_vals = go_data_get_values (base_series->values[1].data);
		n = go_data_get_vector_size (base_series->values[1].data);
		end_vals = go_data_get_values (base_series->values[2].data);
		tmp = go_data_get_vector_size (base_series->values[2].data);
		if (n > tmp)
			n = tmp;

		if (series->has_lines) {
			if (!role)
				role = gog_object_find_role_by_name (
							GOG_OBJECT (series), "Lines");
			lines[j] = GOG_SERIES_LINES (
					gog_object_get_child_by_role (GOG_OBJECT (series), role));
			path1[j] = go_path_new ();
			path2[j] = go_path_new ();
			go_path_set_options (path1[j], GO_PATH_OPTIONS_SHARP);
			go_path_set_options (path2[j], GO_PATH_OPTIONS_SHARP);
		} else
			path1[j] = NULL;
		for (i = 0; i < n; i++) {
			x++;
			work.x = x;
			work.y = start_vals[i];
			work.h = end_vals[i] - work.y;
			val_map = (model->horizontal)? x_map: y_map;
			if (!gog_axis_map_finite (val_map, start_vals[i]) ||
				!gog_axis_map_finite (val_map, end_vals[i])) {						prec_valid = FALSE;
				prec_valid = FALSE;
				continue;
				}
			if (series->has_lines) {
				if (model->horizontal) {
					xmapped = gog_axis_map_to_view (y_map, work.x + work.w / 2.);
					if (!gog_axis_map_finite (y_map, work.x + work.w / 2.)) {
						prec_valid = FALSE;
						continue;
					}
					if (prec_valid) {
						go_path_line_to (path1[j], gog_axis_map_to_view (val_map, start_vals[i]), xmapped);
						go_path_line_to (path2[j], gog_axis_map_to_view (val_map, end_vals[i]), xmapped);
					} else {
						go_path_move_to (path1[j], gog_axis_map_to_view (val_map, start_vals[i]), xmapped);
						go_path_move_to (path2[j], gog_axis_map_to_view (val_map, end_vals[i]), xmapped);
					}
				} else {
					xmapped = gog_axis_map_to_view (x_map, work.x + work.w / 2.);
					if (!gog_axis_map_finite (x_map, work.x + work.w / 2.)) {
						prec_valid = FALSE;
						continue;
					}
					if (prec_valid) {
						go_path_line_to (path1[j], xmapped, gog_axis_map_to_view (val_map, start_vals[i]));
						go_path_line_to (path2[j], xmapped, gog_axis_map_to_view (val_map, end_vals[i]));
					} else {
						go_path_move_to (path1[j], xmapped, gog_axis_map_to_view (val_map, start_vals[i]));
						go_path_move_to (path2[j], xmapped, gog_axis_map_to_view (val_map, end_vals[i]));
					}
				}
				prec_valid = TRUE;
			}
			gog_renderer_push_style (view->renderer, (start_vals[i] <= end_vals[i])?
						GOG_STYLED_OBJECT (series)->style: neg_style);
					barcol_draw_rect (view->renderer, model->horizontal, x_map, y_map, &work);
			barcol_draw_rect (view->renderer, model->horizontal, x_map, y_map, &work);
			gog_renderer_pop_style (view->renderer);
		}
		offset += step;
		g_object_unref (neg_style);
		j++;
	}
	if (ptr != NULL || j != num_series) {
		g_warning ("Wrong series number in dropbar plot");
		num_series = j;
	}
	for (j = 0; j < num_series; j++)
		if (path1[j] != NULL) {
			gog_renderer_push_style (view->renderer,
				go_styled_object_get_style (GO_STYLED_OBJECT (lines[j])));
			gog_series_lines_stroke (lines[j], view->renderer, bbox, path1[j], TRUE);
			gog_series_lines_stroke (lines[j], view->renderer, bbox, path2[j], FALSE);
			gog_renderer_pop_style (view->renderer);
			go_path_free (path2[j]);
			go_path_free (path1[j]);
		}

	gog_axis_map_free (x_map);
	gog_axis_map_free (y_map);
}

static void
gog_dropbar_view_class_init (GogViewClass *view_klass)
{
	view_klass->render	  = gog_dropbar_view_render;
	view_klass->clip	  = TRUE;
}

GSF_DYNAMIC_CLASS (GogDropBarView, gog_dropbar_view,
	gog_dropbar_view_class_init, NULL,
	GOG_TYPE_PLOT_VIEW)
