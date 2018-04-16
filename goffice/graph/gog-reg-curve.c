/*
 * gog-reg-curve.c :
 *
 * Copyright (C) 2005 Jean Brefort (jean.brefort@normalesup.org)
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

/**
 * GogRegCurveDrawingBounds:
 * @GOG_REG_CURVE_DRAWING_BOUNDS_NONE: no limits.
 * @GOG_REG_CURVE_DRAWING_BOUNDS_ABSOLUTE: absolute limits.
 * @GOG_REG_CURVE_DRAWING_BOUNDS_RELATIVE: limits relative to the data range.
 **/

/**
 * GogRegCurveClass:
 * @base: base class
 * @get_value_at: returns the calculated value.
 * @get_equation: gets the regresion equation as a string.
 * @populate_editor: populates the editor.
 **/

#define GOG_REG_CURVE_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GOG_TYPE_REG_CURVE, GogRegCurveClass))

static GObjectClass *reg_curve_parent_klass;

static GType gog_reg_curve_view_get_type (void);

enum {
	REG_CURVE_PROP_0,
	REG_CURVE_PROP_SKIP_INVALID,
	REG_CURVE_PROP_DRAWING_BOUNDS
};

static void
gog_reg_curve_init_style (GogStyledObject *gso, GOStyle *style)
{
	style->interesting_fields = GO_STYLE_LINE;
	gog_theme_fillin_style (gog_object_get_theme (GOG_OBJECT (gso)),
		style, GOG_OBJECT (gso), 0, GO_STYLE_LINE);
}

#ifdef GOFFICE_WITH_GTK

struct reg_curve_closure {
	GtkWidget *al1, *al2, *rl1, *rl2, *fe1, *fe2;
	GObject *rc;
};

static void
skip_invalid_toggled_cb (GtkToggleButton* btn, GObject *obj)
{
	g_object_set (obj, "skip-invalid", gtk_toggle_button_get_active (btn), NULL);
}


static void
limits_changed_cb (GtkComboBox* btn, struct reg_curve_closure *cl)
{
	GogRegCurveDrawingBounds db = gtk_combo_box_get_active (btn);
	switch (db) {
	case GOG_REG_CURVE_DRAWING_BOUNDS_NONE:
		gtk_widget_set_sensitive (cl->al1, FALSE);
		gtk_widget_set_sensitive (cl->al2, FALSE);
		gtk_widget_set_sensitive (cl->rl1, FALSE);
		gtk_widget_set_sensitive (cl->rl2, FALSE);
		gtk_widget_set_sensitive (cl->fe1, FALSE);
		gtk_widget_set_sensitive (cl->fe2, FALSE);
		break;
	case GOG_REG_CURVE_DRAWING_BOUNDS_ABSOLUTE:
		gtk_widget_set_sensitive (cl->al1, TRUE);
		gtk_widget_set_sensitive (cl->al2, TRUE);
		gtk_widget_show (cl->al1);
		gtk_widget_show (cl->al2);
		gtk_widget_hide (cl->rl1);
		gtk_widget_hide (cl->rl2);
		gtk_widget_set_sensitive (cl->fe1, TRUE);
		gtk_widget_set_sensitive (cl->fe2, TRUE);
		break;
	case GOG_REG_CURVE_DRAWING_BOUNDS_RELATIVE:
		gtk_widget_set_sensitive (cl->rl1, TRUE);
		gtk_widget_set_sensitive (cl->rl2, TRUE);
		gtk_widget_hide (cl->al1);
		gtk_widget_hide (cl->al2);
		gtk_widget_show (cl->rl1);
		gtk_widget_show (cl->rl2);
		gtk_widget_set_sensitive (cl->fe1, TRUE);
		gtk_widget_set_sensitive (cl->fe2, TRUE);
		break;
	}
	GOG_REG_CURVE (cl->rc)->drawing_bounds = db;
	gog_object_emit_changed (GOG_OBJECT (cl->rc), FALSE);
}

static void
gog_reg_curve_populate_editor (GogObject	*gobj,
			       GOEditor	*editor,
			       GogDataAllocator	*dalloc,
			       GOCmdContext	*cc)
{
	GtkWidget *w;
	GtkGrid *grid;
	GtkBuilder *gui;
	GogDataset *set = GOG_DATASET (gobj);
	struct reg_curve_closure *cl;
	GogRegCurveDrawingBounds db = GOG_REG_CURVE (gobj)->drawing_bounds;

	gui = go_gtk_builder_load_internal ("res:go:graph/gog-reg-curve-prefs.ui", GETTEXT_PACKAGE, cc);
	if (gui == NULL)
		return;

	go_editor_add_page (editor,
			     go_gtk_builder_get_widget (gui, "reg-curve-prefs"),
			     _("Details"));

	cl = g_new0 (struct reg_curve_closure, 1);
	cl->rc = G_OBJECT (gobj);
	grid = GTK_GRID (gtk_builder_get_object (gui, "reg-curve-prefs"));
	w = GTK_WIDGET (gog_data_allocator_editor (dalloc, set, -1, GOG_DATA_SCALAR));
	gtk_widget_show (w);
	gtk_grid_attach (grid, w, 1, 0, 2, 1);
	w = GTK_WIDGET (gog_data_allocator_editor (dalloc, set, 0, GOG_DATA_SCALAR));
	gtk_widget_show (w);
	gtk_grid_attach (grid, w, 1, 2, 2, 1);
	w = GTK_WIDGET (gog_data_allocator_editor (dalloc, set, 1, GOG_DATA_SCALAR));
	gtk_widget_show (w);
	gtk_grid_attach (grid, w, 1, 3, 2, 1);
	w = go_gtk_builder_get_widget (gui, "skip-invalid");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
					(GOG_REG_CURVE (gobj))->skip_invalid);
	g_signal_connect (G_OBJECT (w), "toggled",
		G_CALLBACK (skip_invalid_toggled_cb), gobj);
	cl->fe1 = GTK_WIDGET (gog_data_allocator_editor (dalloc, set, 2, GOG_DATA_SCALAR));
	gtk_grid_attach (grid, cl->fe1, 1, 6, 2, 1);
	cl->fe2 = GTK_WIDGET (gog_data_allocator_editor (dalloc, set, 3, GOG_DATA_SCALAR));
	gtk_grid_attach (grid, cl->fe2, 1, 7, 2, 1);
	cl->al1 = go_gtk_builder_get_widget (gui, "low-lbl");
	cl->al2 = go_gtk_builder_get_widget (gui, "high-lbl");
	cl->rl1 = go_gtk_builder_get_widget (gui, "first-lbl");
	cl->rl2 = go_gtk_builder_get_widget (gui, "last-lbl");

	gtk_widget_set_sensitive (cl->fe1, db != GOG_REG_CURVE_DRAWING_BOUNDS_NONE);
	gtk_widget_set_sensitive (cl->fe2, db != GOG_REG_CURVE_DRAWING_BOUNDS_NONE);
	gtk_widget_show (cl->fe1);
	gtk_widget_show (cl->fe2);
	switch (db) {
	case GOG_REG_CURVE_DRAWING_BOUNDS_NONE:
	        gtk_widget_set_sensitive (cl->al1, FALSE);
		gtk_widget_set_sensitive (cl->al2, FALSE);
		gtk_widget_hide (cl->rl1);
		gtk_widget_hide (cl->rl2);
		break;
	case GOG_REG_CURVE_DRAWING_BOUNDS_ABSOLUTE:
		gtk_widget_hide (cl->rl1);
		gtk_widget_hide (cl->rl2);
		break;
	case GOG_REG_CURVE_DRAWING_BOUNDS_RELATIVE:
		gtk_widget_hide (cl->al1);
		gtk_widget_hide (cl->al2);
		break;
	}
	w = go_gtk_builder_get_widget (gui, "draw-limits-box");
	gtk_combo_box_set_active (GTK_COMBO_BOX (w), db);
	g_signal_connect (G_OBJECT (w), "changed",
		G_CALLBACK (limits_changed_cb), cl);
	if ((GOG_REG_CURVE_GET_CLASS (gobj))->populate_editor != NULL)
		(GOG_REG_CURVE_GET_CLASS (gobj))->populate_editor (GOG_REG_CURVE (gobj), grid);

	g_object_set_data_full (G_OBJECT (grid), "rc-closure", cl, g_free);
	g_object_unref (gui);

	(GOG_OBJECT_CLASS (reg_curve_parent_klass)->populate_editor) (gobj, editor, dalloc, cc);
}
#endif

static void
gog_reg_curve_get_property (GObject *obj, guint param_id,
		       GValue *value, GParamSpec *pspec)
{
	GogRegCurve *rc = GOG_REG_CURVE (obj);
	switch (param_id) {
	case REG_CURVE_PROP_SKIP_INVALID:
		g_value_set_boolean (value, rc->skip_invalid);
		break;
	case REG_CURVE_PROP_DRAWING_BOUNDS:
		switch (rc->drawing_bounds) {
		case GOG_REG_CURVE_DRAWING_BOUNDS_NONE:
			g_value_set_string (value, "none");
			break;
		case GOG_REG_CURVE_DRAWING_BOUNDS_ABSOLUTE:
			g_value_set_string (value, "absolute");
			break;
		case GOG_REG_CURVE_DRAWING_BOUNDS_RELATIVE:
			g_value_set_string (value, "relative");
			break;
		}
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
	gog_object_emit_changed (GOG_OBJECT (rc), FALSE);
}

static void
gog_reg_curve_set_property (GObject *obj, guint param_id,
		       GValue const *value, GParamSpec *pspec)
{
	GogRegCurve *rc = GOG_REG_CURVE (obj);
	switch (param_id) {
	case REG_CURVE_PROP_SKIP_INVALID:
		rc->skip_invalid = g_value_get_boolean (value);
		gog_object_request_update (GOG_OBJECT (obj));
		break;
	case REG_CURVE_PROP_DRAWING_BOUNDS: {
		char const *str = g_value_get_string (value);
		if (!strcmp (str, "absolute"))
			rc->drawing_bounds = GOG_REG_CURVE_DRAWING_BOUNDS_ABSOLUTE;
		else if (!strcmp (str, "relative"))
			rc->drawing_bounds = GOG_REG_CURVE_DRAWING_BOUNDS_RELATIVE;
		else
			rc->drawing_bounds = GOG_REG_CURVE_DRAWING_BOUNDS_NONE;
		gog_object_emit_changed (GOG_OBJECT (rc), FALSE);
		break;
	}

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
}

static void
gog_reg_curve_finalize (GObject *obj)
{
	GogRegCurve *rc = GOG_REG_CURVE (obj);
	if (rc->bounds != NULL) {
		gog_dataset_finalize (GOG_DATASET (obj));
		g_free (rc->bounds - 1); /* aliased pointer */
		rc->bounds = NULL;
	}
	g_free (rc->a);
	rc->a = NULL;
	g_free (rc->equation);
	rc->equation = NULL;
	(*reg_curve_parent_klass->finalize) (obj);
}

static char const *
gog_reg_curve_type_name (GogObject const *gobj)
{
	return N_("Regression Curve");
}

static void
gog_reg_curve_class_init (GogObjectClass *gog_klass)
{
	static GogObjectRole const roles[] = {
		{ N_("Equation"), "GogRegEqn",	0,
		  GOG_POSITION_ANY_MANUAL, GOG_POSITION_MANUAL, GOG_OBJECT_NAME_BY_ROLE,
		  NULL, NULL, NULL, NULL, NULL, NULL },
	};
	GObjectClass *gobject_klass = (GObjectClass *) gog_klass;
	GogStyledObjectClass *style_klass = (GogStyledObjectClass *) gog_klass;
	GogRegCurveClass *reg_curve_klass = (GogRegCurveClass *) gog_klass;
	reg_curve_parent_klass = g_type_class_peek_parent (gog_klass);

	gobject_klass->get_property = gog_reg_curve_get_property;
	gobject_klass->set_property = gog_reg_curve_set_property;
	gobject_klass->finalize	    = gog_reg_curve_finalize;
#ifdef GOFFICE_WITH_GTK
	gog_klass->populate_editor  = gog_reg_curve_populate_editor;
#endif
	style_klass->init_style = gog_reg_curve_init_style;
	gog_klass->type_name	= gog_reg_curve_type_name;
	gog_klass->view_type	= gog_reg_curve_view_get_type ();
	gog_object_register_roles (gog_klass, roles, G_N_ELEMENTS (roles));

	reg_curve_klass->get_value_at = NULL;
	reg_curve_klass->get_equation = NULL;
#ifdef GOFFICE_WITH_GTK
	reg_curve_klass->populate_editor = NULL;
#endif

	g_object_class_install_property (gobject_klass, REG_CURVE_PROP_SKIP_INVALID,
		g_param_spec_boolean ("skip-invalid",
			_("Skip invalid"),
			_("Skip invalid data"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, REG_CURVE_PROP_DRAWING_BOUNDS,
		g_param_spec_string ("drawing-bounds",
			_("Drawing bounds"),
			_("How the regression line should be limited, acceptable values are \"none\", \"absolute\", and \"relative\"."),
			"none",
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
}

static void
gog_reg_curve_init (GogRegCurve *reg_curve)
{
	reg_curve->ninterp = 100;
	reg_curve->bounds = g_new0 (GogDatasetElement,5) + 1;
}

static void
gog_reg_curve_dataset_dims (GogDataset const *set, int *first, int *last)
{
	*first = -1;
	*last = 3;
}

static GogDatasetElement *
gog_reg_curve_dataset_get_elem (GogDataset const *set, int dim_i)
{
	GogRegCurve const *rc = GOG_REG_CURVE (set);
	g_return_val_if_fail (4 > dim_i, NULL);
	g_return_val_if_fail (dim_i >= -1, NULL);
	return rc->bounds + dim_i;
}

static void
gog_reg_curve_dataset_dim_changed (GogDataset *set, int dim_i)
{
	if (dim_i == -1) {
		GogRegCurve const *rc = GOG_REG_CURVE (set);
		GOData *name_src = rc->bounds[-1].data;
		char *name = (name_src != NULL)
			? go_data_get_scalar_string (name_src) : NULL;
		gog_object_set_name (GOG_OBJECT (set), name, NULL);
	} else
		gog_object_request_update (GOG_OBJECT (set));
}

static void
gog_reg_curve_dataset_init (GogDatasetClass *iface)
{
	iface->get_elem	   = gog_reg_curve_dataset_get_elem;
	iface->dims	   = gog_reg_curve_dataset_dims;
	iface->dim_changed	   = gog_reg_curve_dataset_dim_changed;
}

GSF_CLASS_FULL (GogRegCurve, gog_reg_curve,
	   NULL, NULL, gog_reg_curve_class_init, NULL,
	   gog_reg_curve_init, GOG_TYPE_TREND_LINE, G_TYPE_FLAG_ABSTRACT,
		GSF_INTERFACE (gog_reg_curve_dataset_init, GOG_TYPE_DATASET))

static double
gog_reg_curve_get_value_at (GogRegCurve *reg_curve, double x)
{
	return (GOG_REG_CURVE_GET_CLASS (reg_curve))->get_value_at (reg_curve, x);
}

gchar const*
gog_reg_curve_get_equation (GogRegCurve *reg_curve)
{
	return (GOG_REG_CURVE_GET_CLASS (reg_curve))->get_equation (reg_curve);
}

double
gog_reg_curve_get_R2 (GogRegCurve *reg_curve)
{
	return reg_curve->R2;
}

void
gog_reg_curve_get_bounds (GogRegCurve *reg_curve, double *xmin, double *xmax)
{
	if (reg_curve->bounds[0].data) {
		*xmin = go_data_get_scalar_value (reg_curve->bounds[0].data);
		if (*xmin == go_nan || !go_finite (*xmin))
			*xmin = -DBL_MAX;
	} else
		*xmin = -DBL_MAX;
	if (reg_curve->bounds[1].data) {
		*xmax = go_data_get_scalar_value (reg_curve->bounds[1].data);
		if (*xmax == go_nan || !go_finite (*xmax))
			*xmax = DBL_MAX;
	} else
		*xmax = DBL_MAX;
}

/****************************************************************************/

typedef GogView		GogRegCurveView;
typedef GogViewClass	GogRegCurveViewClass;

#define GOG_TYPE_REG_CURVE_VIEW	(gog_reg_curve_view_get_type ())
#define GOG_REG_CURVE_VIEW(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_REG_CURVE_VIEW, GogRegCurveView))
#define GOG_IS_REG_CURVE_VIEW(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_REG_CURVE_VIEW))

static GogViewClass *reg_curve_view_parent_klass;

static void
gog_reg_curve_view_render (GogView *view, GogViewAllocation const *bbox)
{
	GogRegCurve *rc = GOG_REG_CURVE (view->model);
	GogSeries *series = GOG_SERIES ((GOG_OBJECT (rc))->parent);
	GogPlot *plot = series->plot;
	GogChart *chart = GOG_CHART (GOG_OBJECT (plot)->parent);
	GogAxisMap *x_map;
	GogChartMap *chart_map;
	GOStyle *style;
	GOPath *path;
	GSList *ptr;
	double *x, *y;
	double min, max, delta_x;
	int i;

	chart_map = gog_chart_map_new (chart, &view->residual,
				       plot->axis[GOG_AXIS_X],
				       plot->axis[GOG_AXIS_Y],
				       NULL, FALSE);
	if (!gog_chart_map_is_valid (chart_map)) {
		gog_chart_map_free (chart_map);
		return;
	}

	x_map = gog_chart_map_get_axis_map (chart_map, 0);

	gog_renderer_push_clip_rectangle (view->renderer, view->residual.x, view->residual.y,
					  view->residual.w, view->residual.h);

	x = g_new (double, rc->ninterp + 1);
	y = g_new (double, rc->ninterp + 1);
	switch (rc->drawing_bounds) {
	case GOG_REG_CURVE_DRAWING_BOUNDS_NONE:
		min = gog_axis_map_from_view (x_map, view->residual.x);
		max = gog_axis_map_from_view (x_map, view->residual.x + view->residual.w);
		delta_x = view->residual.w / rc->ninterp;
		for (i = 0; i <= rc->ninterp; i++) {
			x[i] = gog_axis_map_from_view (x_map, i * delta_x + view->residual.x);
			y[i] = gog_reg_curve_get_value_at (rc, x[i]);
		}
		break;
	case GOG_REG_CURVE_DRAWING_BOUNDS_ABSOLUTE: {
		if (rc->bounds[2].data) {
			min = go_data_get_scalar_value (rc->bounds[2].data);
			if (min == go_nan || !go_finite (min))
				min = gog_axis_map_from_view (x_map, view->residual.x);

		} else
			min = gog_axis_map_from_view (x_map, view->residual.x);
		if (rc->bounds[3].data) {
			max = go_data_get_scalar_value (rc->bounds[3].data);
			if (max == go_nan || !go_finite (max))
				max = gog_axis_map_from_view (x_map, view->residual.x + view->residual.w);

		} else
			max = gog_axis_map_from_view (x_map, view->residual.x + view->residual.w);
		break;
	}
	case GOG_REG_CURVE_DRAWING_BOUNDS_RELATIVE: {
		double val;
		GOData *data = NULL;
		GogSeries *series = GOG_SERIES (gog_object_get_parent (GOG_OBJECT (rc)));
		GogSeriesDesc *desc = &GOG_PLOT_GET_CLASS (series->plot)->desc.series;
		int i;
		/* first, look for the index values */
		for (i = 0; i < (int) desc->num_dim; i++)
			if (desc->dim[i].val_type == GOG_DIM_INDEX) {
				data = series->values[i].data;
				break;
			}
		if (data)
			go_data_get_bounds (data, &min, &max);
		else {
			min = 1.;
			max = series->num_elements;
		}
		if (rc->bounds[0].data) {
			val = go_data_get_scalar_value (rc->bounds[0].data);
			if (val != go_nan && go_finite (val))
				min = val;
		}
		if (rc->bounds[1].data) {
			val = go_data_get_scalar_value (rc->bounds[1].data);
			if (val != go_nan && go_finite (val))
				max = val;
		}
		if (rc->bounds[2].data) {
			val = go_data_get_scalar_value (rc->bounds[2].data);
			if (val != go_nan && go_finite (val))
				min -= val;

		}
		if (rc->bounds[3].data) {
			val = go_data_get_scalar_value (rc->bounds[3].data);
			if (val != go_nan && go_finite (val))
				max += val;

		}
		break;
	}
	}

	delta_x = (max - min) / rc->ninterp;
	for (i = 0; i <= rc->ninterp; i++) {
		x[i] = min + i * delta_x;
		y[i] = gog_reg_curve_get_value_at (rc, x[i]);
	}
	path = gog_chart_map_make_path (chart_map, x, y, rc->ninterp + 1, GO_LINE_INTERPOLATION_CUBIC_SPLINE, FALSE, NULL);
	style = GOG_STYLED_OBJECT (rc)->style;
	gog_renderer_push_style (view->renderer, style);
	gog_renderer_stroke_serie (view->renderer, path);
	gog_renderer_pop_style (view->renderer);
	go_path_free (path);

	g_free (x);
	g_free (y);

	gog_renderer_pop_clip (view->renderer);

	gog_chart_map_free (chart_map);

	for (ptr = view->children ; ptr != NULL ; ptr = ptr->next)
		gog_view_render	(ptr->data, bbox);
}

static void
gog_reg_curve_view_size_allocate (GogView *view, GogViewAllocation const *allocation)
{
	GSList *ptr;

	for (ptr = view->children; ptr != NULL; ptr = ptr->next)
		gog_view_size_allocate (GOG_VIEW (ptr->data), allocation);
	(reg_curve_view_parent_klass->size_allocate) (view, allocation);
}

static void
gog_reg_curve_view_class_init (GogRegCurveViewClass *gview_klass)
{
	GogViewClass *view_klass    = (GogViewClass *) gview_klass;
	reg_curve_view_parent_klass = g_type_class_peek_parent (gview_klass);

	view_klass->render	  = gog_reg_curve_view_render;
	view_klass->size_allocate = gog_reg_curve_view_size_allocate;
}

static GSF_CLASS (GogRegCurveView, gog_reg_curve_view,
		  gog_reg_curve_view_class_init, NULL,
		  GOG_TYPE_VIEW)
