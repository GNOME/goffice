/*
 * go-pie.c
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
#include "gog-pie.h"
#include <goffice/graph/gog-view.h>
#include <goffice/graph/gog-renderer.h>
#include <goffice/graph/gog-theme.h>
#include <goffice/data/go-data.h>
#include <goffice/math/go-math.h>
#include <goffice/utils/go-color.h>
#include <goffice/utils/go-persist.h>
#include <goffice/utils/go-style.h>
#include <goffice/utils/go-styled-object.h>
#include <goffice/app/module-plugin-defs.h>

#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>
#include <math.h>
#include <string.h>

#include "embedded-stuff.c"

static GObjectClass *ppe_parent_klass;

typedef GogSeriesElementClass GogPieSeriesElementClass;
enum {
	ELEMENT_PROP_0,
	ELEMENT_SEPARATION,
};

static void
gog_pie_series_element_set_property (GObject *obj, guint param_id,
				     GValue const *value, GParamSpec *pspec)
{
	GogPieSeriesElement *pse = GOG_PIE_SERIES_ELEMENT (obj);

	switch (param_id) {
	case ELEMENT_SEPARATION:
		pse->separation = g_value_get_double (value);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}

	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
}

static void
gog_pie_series_element_get_property (GObject *obj, guint param_id,
				     GValue *value, GParamSpec *pspec)
{
	GogPieSeriesElement *pse = GOG_PIE_SERIES_ELEMENT (obj);

	switch (param_id) {
	case ELEMENT_SEPARATION:
		g_value_set_double (value, pse->separation);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

#ifdef GOFFICE_WITH_GTK
extern gpointer gog_pie_series_element_pref (GogPieSeriesElement *element, GOCmdContext *cc);
static gpointer
gog_pie_series_element_populate_editor (GogObject *gobj,
					GOEditor *editor,
					GOCmdContext *cc)
{
	GtkWidget *widget = gog_pie_series_element_pref (GOG_PIE_SERIES_ELEMENT (gobj), cc);
	go_editor_add_page (editor, widget, _("Settings"));
	g_object_unref (widget);
	return widget;
}
#endif

static void
gog_pie_series_element_class_init (GogPieSeriesElementClass *klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) klass;

	gobject_klass->set_property = gog_pie_series_element_set_property;
	gobject_klass->get_property = gog_pie_series_element_get_property;

	ppe_parent_klass 	= g_type_class_peek_parent (klass);
#ifdef GOFFICE_WITH_GTK
	klass->gse_populate_editor	= gog_pie_series_element_populate_editor;
#endif

	g_object_class_install_property (gobject_klass, ELEMENT_SEPARATION,
		g_param_spec_double ("separation",
			_("Separation"),
			_("Amount a slice is extended as a percentage of the radius"),
			0, 1000, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
}

GSF_DYNAMIC_CLASS (GogPieSeriesElement, gog_pie_series_element,
	gog_pie_series_element_class_init, NULL /*gog_pie_series_element_init*/,
	GOG_TYPE_SERIES_ELEMENT)

/*****************************************************************************/

typedef enum {
	GOG_SHOW_NEGS_SKIP,
	GOG_SHOW_NEGS_ABSOLUTE,
	GOG_SHOW_NEGS_WHITE,
	GOG_SHOW_NEGS_INVERTED, /* not used for now */
	GOG_SHOW_NEGS_MAX
} GogShowNegsMode;

static struct {
	GogShowNegsMode mode;
	char const *name;
} neg_modes [GOG_SHOW_NEGS_MAX] = {
	{GOG_SHOW_NEGS_SKIP, "skip"},
	{GOG_SHOW_NEGS_ABSOLUTE, "absolute"},
	{GOG_SHOW_NEGS_WHITE, "white"},
	{GOG_SHOW_NEGS_INVERTED, "inverted"}
};

static GogShowNegsMode
gog_show_neg_mode_from_str (char const *name)
{
	unsigned i;
	GogShowNegsMode ret = GOG_SHOW_NEGS_ABSOLUTE;

	for (i = 0; i < GO_LINE_MAX; i++) {
		if (strcmp (neg_modes[i].name, name) == 0) {
			ret = neg_modes[i].mode;
			break;
		}
	}
	return ret;
}

static char const *
gog_show_neg_mode_as_str (GogShowNegsMode mode)
{
	unsigned i;
	char const *ret = "absolute";

	for (i = 0; i < GOG_SHOW_NEGS_MAX; i++) {
		if (neg_modes[i].mode == mode) {
			ret = neg_modes[i].name;
			break;
		}
	}
	return ret;
}

typedef struct {
	GogPlotClass	base;
} GogPiePlotClass;

enum {
	PLOT_PROP_0,
	PLOT_PROP_INITIAL_ANGLE,
	PLOT_PROP_DEFAULT_SEPARATION,
	PLOT_PROP_IN_3D,
	PLOT_PROP_SPAN,
	PLOT_PROP_SHOW_NEGS
};

GOFFICE_PLUGIN_MODULE_HEADER;

static GObjectClass *pie_parent_klass;
static GType gog_pie_view_get_type (void);

static void
gog_pie_plot_set_property (GObject *obj, guint param_id,
			   GValue const *value, GParamSpec *pspec)
{
	GogPiePlot *pie = GOG_PIE_PLOT (obj);

	switch (param_id) {
	case PLOT_PROP_INITIAL_ANGLE: {
		double a = g_value_get_double (value);
		a = fmod (a, 360);
		if (a < 0) a += 360;
		pie->initial_angle = a;
		break;
	}
	case PLOT_PROP_DEFAULT_SEPARATION: {
		double f = g_value_get_double (value);
		pie->default_separation = CLAMP (f, 0., 5.);
		break;
	}
	case PLOT_PROP_IN_3D:
		pie->in_3d = g_value_get_boolean (value);
		break;
	case PLOT_PROP_SPAN:
		pie->span = g_value_get_double (value);
		break;
	case PLOT_PROP_SHOW_NEGS : {
		GSList *ptr = GOG_PLOT (obj)->series;
		pie->show_negatives = gog_show_neg_mode_from_str (g_value_get_string (value));
		/* we need to update all the series */
		while (ptr) {
			gog_object_request_update (GOG_OBJECT (ptr->data));
			ptr = ptr->next;
		}
		break;
	}

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}

	/* none of the attributes triggers a size change yet.
	 * When we add data labels we'll need it */
	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
}

static void
gog_pie_plot_get_property (GObject *obj, guint param_id,
			  GValue *value, GParamSpec *pspec)
{
	GogPiePlot *pie = GOG_PIE_PLOT (obj);

	switch (param_id) {
	case PLOT_PROP_INITIAL_ANGLE:
		g_value_set_double (value, pie->initial_angle);
		break;
	case PLOT_PROP_DEFAULT_SEPARATION:
		g_value_set_double (value, pie->default_separation);
		break;
	case PLOT_PROP_IN_3D:
		g_value_set_boolean (value, pie->in_3d);
		break;
	case PLOT_PROP_SPAN:
		g_value_set_double (value, pie->span);
		break;
	case PLOT_PROP_SHOW_NEGS:
		g_value_set_string (value, gog_show_neg_mode_as_str (pie->show_negatives));
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static char const *
gog_pie_plot_type_name (G_GNUC_UNUSED GogObject const *item)
{
	return N_("PlotPie");
}

#ifdef GOFFICE_WITH_GTK
extern gpointer gog_pie_plot_pref (GogPiePlot *pie, GOCmdContext *cc);
static void
gog_pie_plot_populate_editor (GogObject *item,
			      GOEditor *editor,
			      G_GNUC_UNUSED GogDataAllocator *dalloc,
			      GOCmdContext *cc)
{
	GtkWidget *widget = gog_pie_plot_pref (GOG_PIE_PLOT (item), cc);
	go_editor_add_page (editor,
			     widget,
			     _("Properties"));
	g_object_unref (widget);

	(GOG_OBJECT_CLASS(pie_parent_klass)->populate_editor) (item, editor, dalloc, cc);
}
#endif

static void
gog_pie_plot_update (GogObject *obj)
{
	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
}

static void
gog_pie_plot_class_init (GogPlotClass *plot_klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) plot_klass;
	GogObjectClass *gog_klass = (GogObjectClass *) plot_klass;

	pie_parent_klass = g_type_class_peek_parent (plot_klass);
	gobject_klass->set_property = gog_pie_plot_set_property;
	gobject_klass->get_property = gog_pie_plot_get_property;

	gog_klass->update	= gog_pie_plot_update;
	gog_klass->type_name	= gog_pie_plot_type_name;
#ifdef GOFFICE_WITH_GTK
	gog_klass->populate_editor = gog_pie_plot_populate_editor;
#endif
	gog_klass->view_type	= gog_pie_view_get_type ();

	g_object_class_install_property (gobject_klass, PLOT_PROP_INITIAL_ANGLE,
		g_param_spec_double ("initial-angle",
			_("Initial angle"),
			_("Degrees clockwise from 12 O'Clock."),
			-G_MAXFLOAT, G_MAXFLOAT, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, PLOT_PROP_DEFAULT_SEPARATION,
		g_param_spec_double ("default-separation",
			_("Default separation"),
			_("Default amount a slice is extended as a percentage of the radius"),
			0, G_MAXFLOAT, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, PLOT_PROP_IN_3D,
		g_param_spec_boolean ("in-3d",
			_("In 3D"),
			_("Draw 3DS wedges"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (gobject_klass, PLOT_PROP_SPAN,
		g_param_spec_double ("span",
			_("Span"),
			_("Total angle used as a percentage of the full circle"),
			10., 100., 100.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, PLOT_PROP_SHOW_NEGS,
		g_param_spec_string ("show-negs",
			_("Show negative values"),
			_("How negative values are displayed"),
			"absolute",
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	{
		static GogSeriesDimDesc dimensions[] = {
			{ N_("Labels"), GOG_SERIES_SUGGESTED, TRUE,
			  GOG_DIM_LABEL, GOG_MS_DIM_CATEGORIES },
			{ N_("Values"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_VALUES }
		};
		plot_klass->desc.series.dim = dimensions;
		plot_klass->desc.series.num_dim = G_N_ELEMENTS (dimensions);
		plot_klass->desc.series.style_fields = GO_STYLE_OUTLINE | GO_STYLE_FILL;
	}
	plot_klass->desc.num_series_max = 1;
	plot_klass->series_type  = gog_pie_series_get_type ();
}

static void
gog_pie_plot_init (GogPiePlot *pie)
{
	pie->base.vary_style_by_element = TRUE;
	pie->span = 100.;
	pie->show_negatives = GOG_SHOW_NEGS_ABSOLUTE;
}

GSF_DYNAMIC_CLASS (GogPiePlot, gog_pie_plot,
	gog_pie_plot_class_init, gog_pie_plot_init,
	GOG_TYPE_PLOT)

#ifdef GOFFICE_WITH_GTK

static void
gog_pie_plot_set_default_separation (GogPiePlot *pie, double separation)
{
	g_return_if_fail (GOG_PIE_PLOT (pie) != NULL);

	pie->default_separation = CLAMP (separation, 0.0, 5.0);
	gog_object_emit_changed (GOG_OBJECT (pie), FALSE);
}

#endif

/*****************************************************************************/

enum {
	RING_PLOT_PROP_0,
	RING_PLOT_PROP_CENTER_SIZE,
};

typedef GogPiePlotClass	GogRingPlotClass;
static GObjectClass *ring_parent_klass;

static void
gog_ring_plot_set_property (GObject *obj, guint param_id,
			    GValue const *value, GParamSpec *pspec)
{
	GogRingPlot *ring = GOG_RING_PLOT (obj);

	switch (param_id) {
	case RING_PLOT_PROP_CENTER_SIZE:
		ring->center_size = g_value_get_double (value);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}

	/* none of the attributes triggers a size change yet.
	 * When we add data labels we'll need it */
	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
}

static void
gog_ring_plot_get_property (GObject *obj, guint param_id,
			    GValue *value, GParamSpec *pspec)
{
	GogRingPlot *ring = GOG_RING_PLOT (obj);

	switch (param_id) {
	case RING_PLOT_PROP_CENTER_SIZE:
		g_value_set_double (value, ring->center_size);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static char const *
gog_ring_plot_type_name (G_GNUC_UNUSED GogObject const *item)
{
	return N_("PlotRing");
}

#ifdef GOFFICE_WITH_GTK
extern gpointer gog_ring_plot_pref (GogRingPlot *ring, GOCmdContext *cc);
static void
gog_ring_plot_populate_editor (GogObject *item,
			       GOEditor *editor,
		      G_GNUC_UNUSED GogDataAllocator *dalloc,
		      GOCmdContext *cc)
{
	GtkWidget *widget = gog_ring_plot_pref (GOG_RING_PLOT (item), cc);
	go_editor_add_page (editor,
			     widget,
			     _("Properties"));
	g_object_unref (widget);
}
#endif

static void
gog_ring_plot_class_init (GogPiePlotClass *pie_plot_klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) pie_plot_klass;
	GogObjectClass *gog_klass = (GogObjectClass *) pie_plot_klass;
	GogPlotClass *plot_klass = (GogPlotClass *) pie_plot_klass;

	ring_parent_klass = g_type_class_peek_parent (pie_plot_klass);
	gobject_klass->set_property = gog_ring_plot_set_property;
	gobject_klass->get_property = gog_ring_plot_get_property;

	gog_klass->type_name	= gog_ring_plot_type_name;
#ifdef GOFFICE_WITH_GTK
	gog_klass->populate_editor = gog_ring_plot_populate_editor;
#endif

	g_object_class_install_property (gobject_klass, RING_PLOT_PROP_CENTER_SIZE,
		g_param_spec_double ("center-size",
			_("Center-size"),
			_("Size of the center hole as a percentage of the radius"),
			0, 100., 0.5,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	plot_klass->desc.num_series_max = G_MAXINT;
	plot_klass->desc.series.style_fields = GO_STYLE_OUTLINE | GO_STYLE_FILL;
}

static void
gog_ring_plot_init (GogRingPlot *ring)
{
	ring->center_size = 0.5;
}

GSF_DYNAMIC_CLASS (GogRingPlot, gog_ring_plot,
	gog_ring_plot_class_init, gog_ring_plot_init,
	GOG_TYPE_PIE_PLOT)

/*****************************************************************************/

#ifdef GOFFICE_WITH_GTK
typedef struct {
	double x, y, r;
	double start_pos;
       	double start_distance;
} MovePieData;

static gboolean
find_element (GogView *view, double cx, double cy, double x, double y,
	      unsigned int *index, GogPieSeries **series)
{
	GogPiePlot *pie = GOG_PIE_PLOT (view->model);
	GSList *ptr;
	double *vals, scale, len = 0, theta;

	*series = NULL;
	*index = 0;

	for (ptr = pie->base.series ; ptr != NULL ; ptr = ptr->next)
		if (gog_series_is_valid (GOG_SERIES (*series = ptr->data)))
			break;
	if (ptr == NULL)
		return FALSE;

	theta = (atan2 (y - cy, x - cx)
		 * 180 / M_PI - pie->initial_angle + 90.) / pie->span / 3.6;
	if (theta < 0)
		theta += 1.;

	vals = go_data_get_values ((*series)->base.values[1].data);
	scale = 1 / (*series)->total;
	for (*index = 0 ; *index < (*series)->base.num_elements; (*index)++) {
		len = vals[*index] * scale;
		if (len < 0.)
			len = pie->show_negatives? -len: 0.;
		if (go_finite (len) && len > 1e-3) {
			theta -= len;
			if (theta < 0)
				break;
		}
	}
	return TRUE;
}

static gboolean
gog_tool_move_pie_point (GogView *view, double x, double y, GogObject **gobj)
{
	GogPieSeries *series;
	double r = view->allocation.h, cx, cy;
	unsigned int index;

	if (r > view->allocation.w)
		r = view->allocation.w;
	r /= 2.;
	cx = view->allocation.x + view->allocation.w/2.;
	cy = view->allocation.y + view->allocation.h/2.;

	if (hypot (x - cx, y - cy) > fabs (r))
		return FALSE;

	if (find_element (view, cx, cy, x, y, &index, &series))
		*gobj = GOG_OBJECT (gog_series_get_element (GOG_SERIES (series), index));

	return TRUE;
}

static void
gog_tool_move_pie_render (GogView *view)
{
	GogViewAllocation rectangle;
	double r = view->allocation.h;

	if (r > view->allocation.w)
		r = view->allocation.w;

	rectangle.x = view->allocation.x + (view->allocation.w - r) / 2.0;
	rectangle.y = view->allocation.y + (view->allocation.h - r) / 2.0;
	rectangle.w = r;
	rectangle.h = r;

	gog_renderer_draw_selection_rectangle (view->renderer, &rectangle);
}

static void
gog_tool_move_pie_init (GogToolAction *action)
{
	GogPiePlot *pie = GOG_PIE_PLOT (action->view->model);
	MovePieData *data = g_new0 (MovePieData, 1);
	GogViewAllocation area = action->view->allocation;

	data->r = area.h;
	if (data->r > area.w)
		data->r = area.w;
	data->r /= 2.;
	data->start_pos = (data->r * (0.5 + pie->default_separation)) / (1 + pie->default_separation);
	data->x = area.x + area.w / 2.;
	data->y = area.y + area.h / 2.;
	data->start_distance = hypot (action->start_x - data->x, action->start_y - data->y);

	action->data = data;
}

static void
gog_tool_move_pie_move (GogToolAction *action, double x, double y)
{
	GogPiePlot *pie = GOG_PIE_PLOT (action->view->model);
	MovePieData *data = action->data;
	double pos, separation;

	pos = data->start_pos -
		((x - action->start_x) * (data->x - action->start_x) +
		 (y - action->start_y) * (data->y - action->start_y)) /
		data->start_distance;
	separation = (pos - 0.5 * data->r) / (data->r - pos);

	gog_pie_plot_set_default_separation (pie, separation);
}

static void
gog_tool_move_pie_double_click (GogToolAction *action)
{
	MovePieData *data = action->data;
	GogPieSeries *series;
	GogObject *obj;
	unsigned int index;

	if (!find_element (action->view, data->x, data->y,
			   action->start_x, action->start_y,
			   &index, &series))
		return;

	obj = GOG_OBJECT (gog_series_get_element (GOG_SERIES (series), index));
	if (obj == NULL) {
		obj = g_object_new (gog_pie_series_element_get_type (),
				    "index", index, NULL);
		gog_object_add_by_name (GOG_OBJECT (series), "Point", obj);
	}
}

static GogTool gog_tool_move_pie = {
	N_("Move pie"),
	GDK_FLEUR,
	gog_tool_move_pie_point,
	gog_tool_move_pie_render,
	gog_tool_move_pie_init,
	gog_tool_move_pie_move,
	gog_tool_move_pie_double_click,
	NULL /*destroy*/
};
#endif

/*****************************************************************************/

typedef GogPlotView		GogPieView;
typedef GogPlotViewClass	GogPieViewClass;

static GogViewClass *pie_view_parent_klass;

static int
gog_pie_view_get_data_at_point (GogPlotView *view, double x, double y, GogSeries **series)
{
	GogPiePlot const *model = GOG_PIE_PLOT (view->base.model);
	GogPieSeries const *pseries = NULL;
	double r_tot, r_cur, r_int, r_ext, cx, cy, r, th, th0, theta, scale, *vals;
	double separated_cx, separated_cy;
	unsigned int index, k;
	double center_radius;
	double center_size = 0.0;
	unsigned num_series = 0;
	double default_sep, len;
	double separation_max = 0., separation;
	GSList *ptr;
	double outline_width_max = 0.;
	gboolean has_hole;
	GogPieSeriesElement *gpse;
	GList const *overrides;
	GogShowNegsMode mode = model->show_negatives;
	gboolean negative;
	GOStyle *style;

	*series = NULL;
	/* compute number of valid series */
	for (ptr = model->base.series ; ptr != NULL ; ptr = ptr->next) {
		if (!gog_series_is_valid (GOG_SERIES (ptr->data)))
			continue;
		pseries = ptr->data;
		if ((style = go_styled_object_get_style (GO_STYLED_OBJECT (pseries))))
			outline_width_max = MAX (outline_width_max, gog_renderer_line_size (view->base.renderer, style->line.width));
		for (overrides = gog_series_get_overrides (GOG_SERIES (pseries));
		     overrides != NULL;
		     overrides = overrides->next) {
			separation = GOG_PIE_SERIES_ELEMENT (overrides->data)->separation;
			if (separation_max < separation)
				separation_max = separation;
			style = go_styled_object_get_style (GO_STYLED_OBJECT (overrides->data));
			if (outline_width_max < style->line.width)
				outline_width_max = style->line.width;
		}
		num_series++;
	}
	if (separation_max < -model->default_separation)
		separation_max = -model->default_separation;

	if (num_series <= 0)
		return -1;

	if (GOG_IS_RING_PLOT (model))
		center_size = GOG_RING_PLOT(model)->center_size;
	else if (num_series > 1)
		num_series = 1;
	/* calculate things when span < 100. */
	if (model->span < 100.) {
		double xmin, xmax, ymin, ymax, ratio;
		double begin, cur, end;
		if (num_series == 1) {
			pseries = model->base.series->data;
			begin = (model->initial_angle + pseries->initial_angle) * M_PI / 180. - M_PI / 2.;
			end = begin + model->span * M_PI / 50.;
		} else {
			/* WARNING: this code has not been checked */
			ptr = model->base.series;
			while (!gog_series_is_valid (GOG_SERIES (ptr->data)))
				ptr = ptr->next;
			pseries = ptr->data;
			begin = end = pseries->initial_angle;
			for (ptr = ptr->next; ptr != NULL; ptr = ptr->next) {
				if (!gog_series_is_valid (GOG_SERIES (ptr->data)))
					continue;
				pseries = ptr->data;
				cur = pseries->initial_angle;
				if (cur < begin)
					begin = cur;
				if (cur > end)
					end = cur;
			}
			begin = (model->initial_angle + begin) * M_PI / 180. - M_PI / 2.;
			end = ((model->initial_angle + end) / 180. + model->span / 50.) * M_PI - M_PI / 2.;
		}
		cur = ceil (begin / M_PI * 2 - 1e-10) * M_PI / 2;
		xmin = xmax = cos (begin);
		ymin = ymax = sin (begin);
		while (cur < end) {
			cx = cos (cur);
			cy = sin (cur);
			if (cx > xmax)
				xmax = cx;
			if (cx < xmin)
				xmin = cx;
			if (cy > ymax)
				ymax = cy;
			if (cy < ymin)
				ymin = cy;
			cur += M_PI / 2;
		}
		cx = cos (end);
		cy = sin (end);
		if (cx > xmax)
			xmax = cx;
		if (cx < xmin)
			xmin = cx;
		if (cy > ymax)
			ymax = cy;
		if (cy < ymin)
			ymin = cy;
		/* we ensure that the center will be visible */
		if (xmin > 0.)
			xmin = 0.;
		if (xmax < 0.)
			xmax = 0.;
		if (ymin > 0.)
			ymin = 0.;
		if (ymax < 0.)
			ymax = 0.;
		ratio = (ymax - ymin) / (xmax - xmin);
		if (view->base.allocation.h > view->base.allocation.w * ratio) {
			r_tot = view->base.allocation.w * MAX (xmax, -xmin) / (xmax - xmin);
			cx = view->base.allocation.x - view->base.allocation.w * xmin / (xmax - xmin);
			cy = view->base.allocation.y +
				(view->base.allocation.h + view->base.allocation.w * ratio) / 2.
				- view->base.allocation.w * ratio * ymax / (ymax - ymin);
		} else {
			r_tot = view->base.allocation.h * MAX (ymax, -ymin) / (ymax - ymin);
			cx = view->base.allocation.x +
				(view->base.allocation.w - view->base.allocation.h / ratio) / 2.
				- view->base.allocation.h / ratio * xmin / (xmax - xmin);
			cy = view->base.allocation.y - view->base.allocation.h * ymin / (ymax - ymin);
		}
		r_tot /= 1. + model->default_separation + separation_max;
	} else {
		/* centre things */
		cx = view->base.allocation.x + view->base.allocation.w/2.;
		cy = view->base.allocation.y + view->base.allocation.h/2.;

		r_tot = view->base.allocation.h;
		if (r_tot > view->base.allocation.w)
			r_tot = view->base.allocation.w;
		r_tot /= 2. * (1. + model->default_separation + separation_max);
	}

	r = hypot (x - cx, y - cy);

	default_sep = r_tot * model->default_separation;
	center_radius = r_tot * center_size;
	r_cur = r_tot * (1. - center_size);

	if (r < center_radius)
		return -1;

	index = 1;
	th0 = (model->initial_angle + pseries->initial_angle) * M_PI / 180. - M_PI / 2.;
	th = atan2 (y - cy, x - cx);
	if (th < th0)
		th += 2 * M_PI;

	for (ptr = model->base.series ; ptr != NULL ; ptr = ptr->next) {
		pseries = ptr->data;

		if (!gog_series_is_valid (GOG_SERIES (pseries)))
			continue;
		if (index > num_series) /* people snuck extra series into a pie */
			break;

		if (num_series == index)
			r_cur -= outline_width_max / 2.0;
		has_hole = center_radius > 0. || index > 1;
		r_int = (has_hole)? (center_radius + r_cur * ((double) index - 1.0) / (double) num_series): 0.;
		r_ext = center_radius + r_cur * (double) index / (double) num_series;

		theta = th0;

		scale = 2 * M_PI / 100 * model->span / pseries->total;
		vals = go_data_get_values (pseries->base.values[1].data);

		overrides = gog_series_get_overrides (GOG_SERIES (pseries));

		for (k = 0 ; k < pseries->base.num_elements; k++) {
			len = vals[k] * scale;
			negative = len < 0;
			if (negative) {
				if (mode == GOG_SHOW_NEGS_SKIP) {
					if ((overrides != NULL) &&
					    (GOG_SERIES_ELEMENT (overrides->data)->index == k))
						overrides = overrides->next;
					continue;
				}
				len = -len;
			}
			if (!go_finite (len) || len < 1e-3) {
				if ((overrides != NULL) &&
				    (GOG_SERIES_ELEMENT (overrides->data)->index == k))
					overrides = overrides->next;
				continue;
			}

			gpse = NULL;
			if ((overrides != NULL) &&
			    (GOG_SERIES_ELEMENT (overrides->data)->index == k)) {
				gpse = GOG_PIE_SERIES_ELEMENT (overrides->data);
				overrides = overrides->next;
			    }

			/* only separate the outer ring */
			separated_cx = cx;
			separated_cy = cy;
			if (num_series == index && (default_sep > 0. || gpse != NULL)) {

				separation = default_sep;

				if (gpse != NULL)
					separation += gpse->separation * r_tot;

				separated_cx += separation * cos (theta + len/2.);
				separated_cy += separation * sin (theta + len/2.);
				r = hypot (x - separated_cx, y - separated_cy);
				th = atan2 (y - separated_cy, x - separated_cx);
				if (th < th0)
					th += 2 * M_PI;
			}
			theta += len;
			if (r > r_int && r <= r_ext && th > theta - len && th <= theta) {
				*series = GOG_SERIES (pseries);
				return k;
			}
			if (gpse) {
				r = hypot (x - cx, y - cy);
				th = atan2 (y - cy, x - cx);
				if (th < th0)
					th += 2 * M_PI;
			}
		}
		index ++;
	}
	return -1;
}

#define MAX_ARC_SEGMENTS 64

static void
gog_pie_view_render (GogView *view, GogViewAllocation const *bbox)
{
	GogPiePlot const *model = GOG_PIE_PLOT (view->model);
	GogPieSeries const *series = NULL;
	double separated_cx, separated_cy, cx, cy, r, theta, len, *vals, scale;
	double default_sep;
	unsigned k;
	GOPath *path;
	GogTheme *theme = gog_object_get_theme (GOG_OBJECT (model));
	GOStyle *style, *white_style = NULL, *elt_style = NULL;
	GSList *ptr;
	unsigned num_series = 0;
	unsigned index;
	double center_radius;
	double center_size = 0.0;
	double r_ext, r_int, r_tot;
	double outline_width_max;
	gboolean has_hole;
	double separation_max, separation;
	GogPieSeriesElement *gpse;
	GList const *overrides;
	GogShowNegsMode mode = model->show_negatives;
	gboolean negative;

	/* compute number of valid series */
	for (ptr = model->base.series ; ptr != NULL ; ptr = ptr->next) {
		series = ptr->data;
		if (!gog_series_is_valid (GOG_SERIES (ptr->data)))
			continue;
		num_series++;
	}

	if (num_series <= 0)
		return;

	separation_max = .0;
	outline_width_max = .0;
	if ((style = go_styled_object_get_style (GO_STYLED_OBJECT (series))))
		outline_width_max = gog_renderer_line_size (view->renderer, style->line.width);
	if (mode == GOG_SHOW_NEGS_WHITE) {
		white_style = go_style_dup (style);
		white_style->fill.type = GO_STYLE_FILL_PATTERN;
		go_pattern_set_solid (&white_style->fill.pattern, GO_COLOR_WHITE);
	}
	for (overrides = gog_series_get_overrides (GOG_SERIES (series));
	     overrides != NULL;
	     overrides = overrides->next) {
		separation = GOG_PIE_SERIES_ELEMENT (overrides->data)->separation;
		if (separation_max < separation)
			separation_max = separation;
		style = go_styled_object_get_style (GO_STYLED_OBJECT (overrides->data));
		if (outline_width_max < style->line.width)
			outline_width_max = style->line.width;
	}
	if (separation_max < -model->default_separation)
		separation_max = -model->default_separation;

	if (GOG_IS_RING_PLOT (model))
		center_size = GOG_RING_PLOT(model)->center_size;
	else if (num_series > 1)
		num_series = 1;

	/* calculate things when span < 100. */
	if (model->span < 100.) {
		double xmin, xmax, ymin, ymax, ratio;
		double begin, cur, end;
		if (num_series == 1) {
			begin = (model->initial_angle + series->initial_angle) * M_PI / 180. - M_PI / 2.;
			end = begin + model->span * M_PI / 50.;
		} else {
			/* WARNING: this code has not been checked */
			ptr = model->base.series;
			while (!gog_series_is_valid (GOG_SERIES (ptr->data)))
				ptr = ptr->next;
			series = ptr->data;
			begin = end = series->initial_angle;
			for (ptr = ptr->next; ptr != NULL; ptr = ptr->next) {
				if (!gog_series_is_valid (GOG_SERIES (ptr->data)))
					continue;
				series = ptr->data;
				cur = series->initial_angle;
				if (cur < begin)
					begin = cur;
				if (cur > end)
					end = cur;
			}
			begin = (model->initial_angle + begin) * M_PI / 180. - M_PI / 2.;
			end = ((model->initial_angle + end) / 180. + model->span / 50.) * M_PI - M_PI / 2.;
		}
		cur = ceil (begin / M_PI * 2 - 1e-10) * M_PI / 2;
		xmin = xmax = cos (begin);
		ymin = ymax = sin (begin);
		while (cur < end) {
			cx = cos (cur);
			cy = sin (cur);
			if (cx > xmax)
				xmax = cx;
			if (cx < xmin)
				xmin = cx;
			if (cy > ymax)
				ymax = cy;
			if (cy < ymin)
				ymin = cy;
			cur += M_PI / 2;
		}
		cx = cos (end);
		cy = sin (end);
		if (cx > xmax)
			xmax = cx;
		if (cx < xmin)
			xmin = cx;
		if (cy > ymax)
			ymax = cy;
		if (cy < ymin)
			ymin = cy;
		/* we ensure that the center will be visible */
		if (xmin > 0.)
			xmin = 0.;
		if (xmax < 0.)
			xmax = 0.;
		if (ymin > 0.)
			ymin = 0.;
		if (ymax < 0.)
			ymax = 0.;
		ratio = (ymax - ymin) / (xmax - xmin);
		if (view->allocation.h > view->allocation.w * ratio) {
			r_tot = view->allocation.w * MAX (xmax, -xmin) / (xmax - xmin);
			cx = view->allocation.x - view->allocation.w * xmin / (xmax - xmin);
			cy = view->allocation.y +
				(view->allocation.h + view->allocation.w * ratio) / 2.
				- view->allocation.w * ratio * ymax / (ymax - ymin);
		} else {
			r_tot = view->allocation.h * MAX (ymax, -ymin) / (ymax - ymin);
			cx = view->allocation.x +
				(view->allocation.w - view->allocation.h / ratio) / 2.
				- view->allocation.h / ratio * xmin / (xmax - xmin);
			cy = view->allocation.y - view->allocation.h * ymin / (ymax - ymin);
		}
		r_tot /= 1. + model->default_separation + separation_max;
	} else {
		/* centre things */
		cx = view->allocation.x + view->allocation.w/2.;
		cy = view->allocation.y + view->allocation.h/2.;

		r_tot = view->allocation.h;
		if (r_tot > view->allocation.w)
			r_tot = view->allocation.w;
		r_tot /= 2. * (1. + model->default_separation + separation_max);
	}

	path = go_path_new ();

	default_sep = r_tot * model->default_separation;
	center_radius = r_tot * center_size;
	r = r_tot * (1. - center_size);

	index = 1;
	for (ptr = model->base.series ; ptr != NULL ; ptr = ptr->next) {
		series = ptr->data;

		if (!gog_series_is_valid (GOG_SERIES (series)))
			continue;
		if (index > num_series) /* people snuck extra series into a pie */
			break;

		if (num_series == index)
			r -= outline_width_max / 2.0;

		has_hole = center_radius > 0. || index > 1;
		r_int = center_radius + r * ((double)index - 1.0) / (double)num_series;
		r_ext = center_radius + r * (double)index / (double)num_series;

		theta = (model->initial_angle + series->initial_angle) * M_PI / 180. - M_PI / 2.;

		scale = 2 * M_PI / 100 * model->span / series->total;
		vals = go_data_get_values (series->base.values[1].data);

		style = GOG_STYLED_OBJECT (series)->style;
		if (model->base.vary_style_by_element || mode == GOG_SHOW_NEGS_WHITE)
			style = go_style_dup (style);
		gog_renderer_push_style (view->renderer, style);

		overrides = gog_series_get_overrides (GOG_SERIES (series));
		for (k = 0 ; k < series->base.num_elements; k++) {
			len = vals[k] * scale;
			negative = len < 0;
			if (negative) {
				if (mode == GOG_SHOW_NEGS_SKIP) {
					if ((overrides != NULL) &&
					    (GOG_SERIES_ELEMENT (overrides->data)->index == k))
						overrides = overrides->next;
					continue;
				}
				len = -len;
			}
			if (!go_finite (len) || len < 1e-3) {
				if ((overrides != NULL) &&
				    (GOG_SERIES_ELEMENT (overrides->data)->index == k))
					overrides = overrides->next;
				continue;
			}

			gpse = NULL;
			if ((overrides != NULL) &&
			    (GOG_SERIES_ELEMENT (overrides->data)->index == k)) {
				gpse = GOG_PIE_SERIES_ELEMENT (overrides->data);
				overrides = overrides->next;
				if (negative && mode == GOG_SHOW_NEGS_WHITE) {
					elt_style = go_style_dup (go_styled_object_get_style (
							GO_STYLED_OBJECT (gpse)));
					elt_style->fill.type = GO_STYLE_FILL_PATTERN;
					go_pattern_set_solid (&elt_style->fill.pattern, GO_COLOR_WHITE);
					gog_renderer_push_style (view->renderer, elt_style);
				} else
					gog_renderer_push_style (view->renderer,
						go_styled_object_get_style (
							GO_STYLED_OBJECT (gpse)));
			} else {
				if (negative && mode == GOG_SHOW_NEGS_WHITE)
					gog_renderer_push_style (view->renderer, white_style);
				else if (model->base.vary_style_by_element)
					gog_theme_fillin_style (theme, style, GOG_OBJECT (series),
								model->base.index_num + k, GO_STYLE_FILL);
			}

			/* only separate the outer ring */
			separated_cx = cx;
			separated_cy = cy;
			if (num_series == index && (default_sep > 0. || gpse != NULL)) {

				separation = default_sep;

				if (gpse != NULL)
					separation += gpse->separation * r_tot;

				separated_cx += separation * cos (theta + len/2.);
				separated_cy += separation * sin (theta + len/2.);
			}
			theta += len;

			go_path_ring_wedge (path, separated_cx, separated_cy,
					    r_ext, r_ext,
					    has_hole ? r_int : 0.0,
					    has_hole ? r_int : 0.0,
					    theta - len,
					    theta);
			gog_renderer_draw_shape (view->renderer, path);
			go_path_clear (path);

			if (gpse != NULL || (negative && mode == GOG_SHOW_NEGS_WHITE)) {
				gog_renderer_pop_style (view->renderer);
				if (elt_style) {
					g_object_unref (elt_style);
					elt_style = NULL;
				}
			}
		}

		gog_renderer_pop_style (view->renderer);
		if (model->base.vary_style_by_element || mode == GOG_SHOW_NEGS_WHITE)
			g_object_unref (style);
		if (white_style)
			g_object_unref (white_style);

		index ++;
	}

	go_path_free (path);
}

static void
gog_pie_view_build_toolkit (GogView *view)
{
	(GOG_VIEW_CLASS (pie_view_parent_klass)->build_toolkit) (view);
#ifdef GOFFICE_WITH_GTK
	view->toolkit = g_slist_prepend (view->toolkit, &gog_tool_move_pie);
#endif
}

static char*
gog_pie_view_get_tip_at_point (GogView *view, double x, double y)
{
	GogPieSeries *series = NULL;
	int index = gog_pie_view_get_data_at_point (GOG_PLOT_VIEW (view), x, y, (GogSeries** )&series);
	char *label, *ret;
	double *vals, value;

	if (index < 0)
		return NULL;
	vals = go_data_get_values (series->base.values[1].data);
	value = fabs (vals[index]);
	label = series->base.values[0].data? go_data_get_vector_string (series->base.values[0].data, index): NULL;
	if (label && *label)
		ret = g_strdup_printf (_("%s: %g (%.2f%%)"), label, value, value * 100 / series->total);
	else
		ret = g_strdup_printf (_("%g (%.2f%%)"), value, value * 100 / series->total);
	g_free (label);
	return ret;
}

static void
gog_pie_view_class_init (GogViewClass *view_klass)
{
	GogPlotViewClass *pv_klass = (GogPlotViewClass *) view_klass;
	pie_view_parent_klass = g_type_class_peek_parent (view_klass);

	view_klass->render 		= gog_pie_view_render;
	view_klass->build_toolkit 	= gog_pie_view_build_toolkit;
	view_klass->get_tip_at_point    = gog_pie_view_get_tip_at_point;
	pv_klass->get_data_at_point     = gog_pie_view_get_data_at_point;
}

GSF_DYNAMIC_CLASS (GogPieView, gog_pie_view,
	gog_pie_view_class_init, NULL,
	GOG_TYPE_PLOT_VIEW)

/*****************************************************************************/

typedef GogSeriesClass GogPieSeriesClass;
enum {
	SERIES_PROP_0,
	SERIES_PROP_INITIAL_ANGLE,
	SERIES_PROP_SEPARATION,
};

static GogObjectClass *series_parent_klass;
static void
gog_pie_series_set_property (GObject *obj, guint param_id,
			     GValue const *value, GParamSpec *pspec)
{
	GogPieSeries *pie = GOG_PIE_SERIES (obj);

	switch (param_id) {
	case SERIES_PROP_INITIAL_ANGLE:
		pie->initial_angle = g_value_get_double (value);
		break;
	case SERIES_PROP_SEPARATION:
		pie->separation = g_value_get_double (value);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
	/* none of the attributes triggers a size change yet.
	 * When we add data labels we'll need it */
	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
}

static void
gog_pie_series_get_property (GObject *obj, guint param_id,
			  GValue *value, GParamSpec *pspec)
{
	GogPieSeries *pie = GOG_PIE_SERIES (obj);

	switch (param_id) {
	case SERIES_PROP_INITIAL_ANGLE:
		g_value_set_double (value, pie->initial_angle);
		break;
	case SERIES_PROP_SEPARATION:
		g_value_set_double (value, pie->separation);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
gog_pie_series_update (GogObject *obj)
{
	double *vals = NULL, total;
	int len = 0;
	GogPieSeries *series = GOG_PIE_SERIES (obj);
	unsigned old_num = series->base.num_elements;
	GogShowNegsMode mode = GOG_PIE_PLOT (series->base.plot)->show_negatives;

	if (series->base.values[1].data != NULL) {
		vals = go_data_get_values (series->base.values[1].data);
		len = go_data_get_vector_size (series->base.values[1].data);
	}
	series->base.num_elements = len;

	for (total = 0. ; len-- > 0 ;) {
		double val = vals[len];
		if (go_finite (val)) {
			if (val < 0)
				val = (mode == GOG_SHOW_NEGS_SKIP)? 0.: -val;
			total += val;
		}
	}
	series->total = total;

	/* queue plot for redraw */
	gog_object_request_update (GOG_OBJECT (series->base.plot));
	if (old_num != series->base.num_elements)
		gog_plot_request_cardinality_update (series->base.plot);

	if (series_parent_klass->update)
		series_parent_klass->update (obj);
}

static void
gog_pie_series_class_init (GObjectClass *gobject_klass)
{
	GogObjectClass *gog_klass = (GogObjectClass *)gobject_klass;
	GogSeriesClass *series_klass = (GogSeriesClass *)gobject_klass;

	series_parent_klass = g_type_class_peek_parent (gobject_klass);
	gog_klass->update = gog_pie_series_update;
	series_klass->series_element_type = GOG_TYPE_PIE_SERIES_ELEMENT;

	gobject_klass->set_property = gog_pie_series_set_property;
	gobject_klass->get_property = gog_pie_series_get_property;

	g_object_class_install_property (gobject_klass, SERIES_PROP_INITIAL_ANGLE,
		g_param_spec_double ("initial-angle",
			_("Initial-angle"),
			_("Degrees clockwise from 12 O'Clock"),
			0, G_MAXFLOAT, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (gobject_klass, SERIES_PROP_SEPARATION,
		g_param_spec_double ("separation",
			_("Separation"),
			_("Default amount a slice is extended as a percentage of the radius"),
			0, G_MAXFLOAT, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
}

GSF_DYNAMIC_CLASS (GogPieSeries, gog_pie_series,
	   gog_pie_series_class_init, NULL,
	   GOG_TYPE_SERIES)

G_MODULE_EXPORT void
go_plugin_init (GOPlugin *plugin, GOCmdContext *cc)
{
	GTypeModule *module = go_plugin_get_type_module (plugin);
	gog_pie_series_element_register_type (module);
	gog_pie_plot_register_type (module);
	gog_pie_view_register_type (module);
	gog_pie_series_register_type (module);
	gog_ring_plot_register_type (module);

	register_embedded_stuff ();
}

G_MODULE_EXPORT void
go_plugin_shutdown (GOPlugin *plugin, GOCmdContext *cc)
{
	unregister_embedded_stuff ();
}
