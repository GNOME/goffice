/*
 * gog-legend.c :
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
#include <goffice/graph/gog-legend.h>
#include <goffice/graph/gog-outlined-object.h>
#include <goffice/graph/gog-view.h>
#include <goffice/graph/gog-renderer.h>
#include <goffice/graph/gog-chart.h>
#include <goffice/utils/go-style.h>
#include <goffice/graph/gog-theme.h>
#include <goffice/utils/go-color.h>
#include <goffice/utils/go-styled-object.h>
#include <goffice/math/go-math.h>
#include <goffice/utils/go-marker.h>
#include <goffice/utils/go-path.h>
#include <goffice/utils/go-persist.h>
#include <goffice/utils/go-units.h>

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

#include <math.h>

/**
 * SECTION: gog-legend
 * @short_description: A legend.
 *
 * A legend shows the name of each #GogSeries and which colors they
 * have in the plot. To use, add in the role "Legend" of a #GogChart.
 */

struct _GogLegend {
	GogOutlinedObject base;

	double	 swatch_size_pts;
	double	 swatch_padding_pts;
	gulong	 chart_cardinality_handle;
	gulong	 chart_child_name_changed_handle;
	unsigned cached_count;
	gboolean names_changed;
};

typedef GogStyledObjectClass GogLegendClass;

enum {
	LEGEND_PROP_0,
	LEGEND_SWATCH_SIZE_PTS,
	LEGEND_SWATCH_PADDING_PTS
};

static GType gog_legend_view_get_type (void);

static GObjectClass *parent_klass;

static void
gog_legend_set_property (GObject *obj, guint param_id,
			 GValue const *value, GParamSpec *pspec)
{
	GogLegend *legend = GOG_LEGEND (obj);

	switch (param_id) {
	case LEGEND_SWATCH_SIZE_PTS :
		legend->swatch_size_pts = g_value_get_double (value);
		break;
	case LEGEND_SWATCH_PADDING_PTS :
		legend->swatch_padding_pts = g_value_get_double (value);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
}

static void
gog_legend_get_property (GObject *obj, guint param_id,
			 GValue *value, GParamSpec *pspec)
{
	GogLegend *legend = GOG_LEGEND (obj);

	switch (param_id) {
	case LEGEND_SWATCH_SIZE_PTS :
		g_value_set_double (value, legend->swatch_size_pts);
		break;
	case LEGEND_SWATCH_PADDING_PTS :
		g_value_set_double (value, legend->swatch_padding_pts);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
cb_chart_names_changed (GogLegend *legend)
{
	if (legend->names_changed)
		return;
	legend->names_changed = TRUE;
	gog_object_request_update (GOG_OBJECT (legend));
}

static void
gog_legend_parent_changed (GogObject *obj, gboolean was_set)
{
	GogObjectClass *gog_object_klass = GOG_OBJECT_CLASS (parent_klass);
	GogLegend *legend = GOG_LEGEND (obj);

	if (was_set) {
		if (legend->chart_cardinality_handle == 0)
			legend->chart_cardinality_handle =
				g_signal_connect_object (G_OBJECT (obj->parent),
					"notify::cardinality-valid",
					G_CALLBACK (gog_object_request_update),
					legend, G_CONNECT_SWAPPED);
		if (legend->chart_child_name_changed_handle == 0)
			legend->chart_child_name_changed_handle =
				g_signal_connect_object (G_OBJECT (obj->parent),
					"child-name-changed",
					G_CALLBACK (cb_chart_names_changed),
					legend, G_CONNECT_SWAPPED);
	} else {
		if (legend->chart_cardinality_handle != 0) {
			g_signal_handler_disconnect (G_OBJECT (obj->parent),
				legend->chart_cardinality_handle);
			legend->chart_cardinality_handle = 0;
		}
		if (legend->chart_child_name_changed_handle != 0) {
			g_signal_handler_disconnect (G_OBJECT (obj->parent),
				legend->chart_child_name_changed_handle);
			legend->chart_child_name_changed_handle = 0;
		}
	}

	gog_object_klass->parent_changed (obj, was_set);
}

static void
gog_legend_update (GogObject *obj)
{
	GogLegend *legend = GOG_LEGEND (obj);
	unsigned visible;
	gog_chart_get_cardinality (GOG_CHART (obj->parent), NULL, &visible);
	if (legend->cached_count != visible)
		legend->cached_count = visible;
	else if (!legend->names_changed)
		return;
	legend->names_changed = FALSE;
	gog_object_emit_changed	(obj, TRUE);
}

static void
gog_legend_populate_editor (GogObject *gobj,
			    GOEditor *editor,
			    GogDataAllocator *dalloc,
			    GOCmdContext *cc)
{
	static guint legend_pref_page = 0;

	(GOG_OBJECT_CLASS(parent_klass)->populate_editor) (gobj, editor, dalloc, cc);
	go_editor_set_store_page (editor, &legend_pref_page);
}

static void
gog_legend_init_style (GogStyledObject *gso, GOStyle *style)
{
	style->interesting_fields = GO_STYLE_OUTLINE | GO_STYLE_FILL | GO_STYLE_FONT;
	gog_theme_fillin_style (gog_object_get_theme (GOG_OBJECT (gso)),
		style, GOG_OBJECT (gso), 0, GO_STYLE_OUTLINE | GO_STYLE_FILL |
	        GO_STYLE_FONT);
}

static void
gog_legend_class_init (GogLegendClass *klass)
{
	static GogObjectRole const roles[] = {
		{ N_("Title"), "GogLabel",	0,
		  GOG_POSITION_COMPASS, GOG_POSITION_N|GOG_POSITION_ALIGN_CENTER, GOG_OBJECT_NAME_BY_ROLE,
		  NULL, NULL, NULL, NULL, NULL, NULL },
	};

	GObjectClass         *gobject_klass = (GObjectClass *) klass;
	GogObjectClass       *gog_klass     = (GogObjectClass *) klass;
	GogStyledObjectClass *style_klass   = (GogStyledObjectClass *) klass;

	parent_klass = g_type_class_peek_parent (klass);

	gobject_klass->set_property = gog_legend_set_property;
	gobject_klass->get_property = gog_legend_get_property;
	gog_klass->parent_changed   = gog_legend_parent_changed;
	gog_klass->update           = gog_legend_update;
	gog_klass->populate_editor  = gog_legend_populate_editor;
	gog_klass->view_type        = gog_legend_view_get_type ();
	style_klass->init_style     = gog_legend_init_style;

	gog_object_register_roles (gog_klass, roles, G_N_ELEMENTS (roles));

	g_object_class_install_property (gobject_klass, LEGEND_SWATCH_SIZE_PTS,
		g_param_spec_double ("swatch-size-pts",
			_("Swatch Size pts"),
			_("size of the swatches in pts."),
			0, G_MAXDOUBLE, 0,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, LEGEND_SWATCH_PADDING_PTS,
		g_param_spec_double ("swatch-padding-pts",
			_("Swatch Padding pts"),
			_("padding between the swatches in pts."),
			0, G_MAXDOUBLE, 0,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
}

static void
gog_legend_init (GogLegend *legend)
{
	legend->swatch_size_pts = GO_CM_TO_PT ((double).25);
	legend->swatch_padding_pts = GO_CM_TO_PT ((double).2);
	legend->cached_count = 0;
}

GSF_CLASS (GogLegend, gog_legend,
	   gog_legend_class_init, gog_legend_init,
	   GOG_TYPE_OUTLINED_OBJECT)

#define	GLV_ELEMENT_HEIGHT_EM		1.2	/* Legend element height, in font size */
#define GLV_LINE_WIDTH_MAX_DEFAULT_EM	0.5	/* Maximum sample line width, in font size */
#define GLV_LINE_LENGTH_EM		4.0	/* Sample line length, in font size */
#define GLV_SAMPLE_DASH_LENGTH		1.2	/* Minimum dash sequence length */
#define GLV_SWATCH_LENGTH_EM		1.0	/* Swatch height/width, in font size */
#define GLV_SWATCH_SIZE_MIN_EM		0.25	/* Minimum swatch size, in font size */
#define GLV_SWATCH_SPACING_EM		0.5	/* Space between swatch and label, in font size */
#define GLV_ELEMENT_SPACING_EM		1.0	/* Space between legend elements, in font size */

typedef struct {
	GogOutlinedView	base;
	gboolean	is_vertical;
	double		element_width;
	double		element_height;
	unsigned	element_per_blocks;
	double 		swatch_w;
	double 		swatch_h;
	unsigned	num_blocks;
	gboolean	uses_lines;
	double		label_offset;
	double		font_size;
} GogLegendView;

typedef GogOutlinedViewClass	GogLegendViewClass;

#define GOG_TYPE_LEGEND_VIEW	(gog_legend_view_get_type ())
#define GOG_LEGEND_VIEW(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_LEGEND_VIEW, GogLegendView))
#define GOG_IS_LEGEND_VIEW(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_LEGEND_VIEW))

static GogViewClass *lview_parent_klass;

static void
cb_size_elements (unsigned i, GOStyle const *style,
		  char const *name, PangoAttrList *l, GogLegendView *glv)
{
	GogView *view = GOG_VIEW (glv);
	GOGeometryAABR aabr;

	if (l) {
		GOString *str = go_string_new_rich (name, -1, l, NULL);
		gog_renderer_get_gostring_AABR (view->renderer, str, &aabr, -1.);
		go_string_unref (str);
	} else
		gog_renderer_get_text_AABR (view->renderer, name, FALSE, &aabr, -1.);

	if (glv->element_width < aabr.w)
		glv->element_width = aabr.w;
	if (glv->element_height < aabr.h)
		glv->element_height = aabr.h;
	if (!glv->uses_lines && (style->interesting_fields & GO_STYLE_LINE))
		glv->uses_lines = TRUE;
}

static void
gog_legend_view_size_request (GogView *v,
			      GogViewRequisition const *available,
			      GogViewRequisition *req)
{
	GogChart *chart = GOG_CHART (v->model->parent);
	GogLegendView *glv = GOG_LEGEND_VIEW (v);
	GogLegend *l = GOG_LEGEND (v->model);
	GogViewRequisition child_req, residual, frame_req;
	GOStyle *style;
	double available_space, element_size;
	unsigned num_elements;

	residual = *available;
	req->w = req->h = 0;
	gog_view_size_child_request (v, available, req, &child_req);
	frame_req.w = frame_req.h = 0;
	lview_parent_klass->size_request (v, available, &frame_req);
	residual.w -= req->w + frame_req.w;
	residual.h -= req->h + frame_req.h;

	glv->is_vertical = gog_object_get_position_flags (GOG_OBJECT (l), GOG_POSITION_COMPASS) &
		(GOG_POSITION_E | GOG_POSITION_W);

	gog_chart_get_cardinality (chart, NULL, &num_elements);

	if (num_elements < 1) {
		/* If there's no child requisition and no element, there's no
		 * point in displaying a legend box. */
		if (go_sub_epsilon (child_req.w) <= 0.0 &&
		    go_sub_epsilon (child_req.h) <= 0.0) {
			req->w = req->h = -1;
		} else {
			req->w = child_req.w + frame_req.w;
			req->h = child_req.h + frame_req.h;
		}
		return;
	}

	style = go_styled_object_get_style (GO_STYLED_OBJECT (l));
	gog_renderer_push_style (v->renderer, style);

	glv->font_size = pango_font_description_get_size (style->font.font->desc) / PANGO_SCALE;
	glv->swatch_w = gog_renderer_pt2r_x (v->renderer, glv->font_size);
	glv->swatch_h = gog_renderer_pt2r_y (v->renderer, glv->font_size);

	glv->element_width = 0.;
	glv->element_height = GLV_ELEMENT_HEIGHT_EM * glv->swatch_h;
	glv->uses_lines = FALSE;

	gog_chart_foreach_elem (chart, TRUE,
		(GogEnumFunc) cb_size_elements, glv);

	gog_renderer_pop_style (v->renderer);

	glv->label_offset = (glv->uses_lines ?
			     GLV_LINE_LENGTH_EM + GLV_SWATCH_SPACING_EM :
			     GLV_SWATCH_LENGTH_EM + GLV_SWATCH_SPACING_EM) * glv->swatch_w;
	glv->element_width += glv->label_offset + glv->swatch_w * GLV_ELEMENT_SPACING_EM;

	available_space = glv->is_vertical ? residual.h : residual.w;
	element_size = glv->is_vertical ? glv->element_height : glv->element_width;

	glv->element_per_blocks = available_space > 0. ? floor (available_space / element_size) : 0;

	if (glv->element_per_blocks < 1) {
	       	req->w = req->h = -1;
		return;
	}

	glv->num_blocks = floor ((num_elements - 1) / glv->element_per_blocks) + 1;

	if (glv->is_vertical) {
		req->h += MIN (glv->element_per_blocks, num_elements) * glv->element_height;
		req->w += glv->num_blocks * glv->element_width - glv->swatch_w * GLV_ELEMENT_SPACING_EM;
	} else {
		req->h += glv->num_blocks * glv->element_height;
		req->w += MIN (glv->element_per_blocks, num_elements) * glv->element_width
			- glv->swatch_w * GLV_ELEMENT_SPACING_EM;
	}

	req->w = frame_req.w + MAX (child_req.w, req->w);
	req->h = frame_req.h + MAX (child_req.h, req->h);
}

typedef struct {
	double size_max;
	double size_min;
	double line_length;
	double line_width_max;
	double hairline_width;
	double line_scale;
} SwatchScaleClosure;

static void
cb_swatch_scale (unsigned i, GOStyle const *style, char const *name,
		 PangoAttrList *l, SwatchScaleClosure *data)
{
	GOStyleLine const *line = NULL;
	double size;
	double maximum, scale;

	size = go_marker_get_size (style->marker.mark);
	if (data->size_max < size)
		data->size_max = size;
	if (data->size_min > size)
		data->size_min = size;

	if (style->interesting_fields & (GO_STYLE_LINE | GO_STYLE_OUTLINE))
		line = &style->line;

	if (line == NULL ||
	    line->width <= data->hairline_width)
		return;

	if (line->dash_type != GO_LINE_NONE && line->dash_type != GO_LINE_SOLID)
		maximum = data->line_length / (GLV_SAMPLE_DASH_LENGTH
					       * go_line_dash_get_length (line->dash_type));
	else
		maximum = line->width;

	if (maximum > data->line_width_max)
		maximum = data->line_width_max;

	if (maximum > data->hairline_width)
		scale = (maximum - data->hairline_width) / (line->width - data->hairline_width);
	else
		scale = 0;
	if (data->line_scale > scale)
		data->line_scale = scale;
	if (l)
		pango_attr_list_unref (l);
}

typedef struct {
	int count;
	GogView const *view;
	double x, y;
	double element_step_x, element_step_y;
	double block_step_x, block_step_y;
	GogViewAllocation swatch;
	double swatch_scale_a, swatch_scale_b;
	double line_scale_a, line_scale_b;
	double hairline_width;
} RenderClosure;

static void
cb_render_elements (unsigned index, GOStyle const *base_style, char const *name,
		    PangoAttrList *l, RenderClosure *data)
{
	GogView const *view = data->view;
	GogLegendView *glv = GOG_LEGEND_VIEW (view);
	GogRenderer *renderer = view->renderer;
	GOStyle *style = NULL;
	GogViewAllocation pos, rectangle;
	double half_width, y;
	GOPath *line_path;

	if (data->count > 0) {
		if ((data->count % glv->element_per_blocks) != 0) {
			data->x += data->element_step_x;
			data->y += data->element_step_y;
		} else {
			data->x += data->block_step_x;
			data->y += data->block_step_y;
		}
	}
	data->count++;

	if ((base_style->interesting_fields & (GO_STYLE_LINE | GO_STYLE_OUTLINE)) == GO_STYLE_LINE) { /* line and marker */
		style = go_style_dup (base_style);
		g_return_if_fail (style != NULL);
		if (style->line.width > data->hairline_width)
			style->line.width = style->line.width * data->line_scale_a + data->line_scale_b;

		gog_renderer_push_style (renderer, style);
		half_width = 0.5 * gog_renderer_line_size (renderer, style->line.width);
		line_path = go_path_new ();
		go_path_set_options (line_path, GO_PATH_OPTIONS_SHARP);
		y = data->y + glv->element_height / 2.;
		go_path_move_to (line_path, data->x + half_width, y);
		go_path_line_to (line_path, data->x + data->swatch.w * GLV_LINE_LENGTH_EM - half_width, y);
		if (style->interesting_fields & GO_STYLE_FILL) {
			rectangle.x = data->x - half_width;
			rectangle.y = y;
			rectangle.w = data->swatch.w * GLV_LINE_LENGTH_EM + 2.0 * half_width;
			rectangle.h = glv->element_height / 2.0;
			gog_renderer_fill_rectangle (renderer, &rectangle);
		}
		gog_renderer_stroke_serie (renderer, line_path);
		go_path_free (line_path);
		if (base_style->interesting_fields & GO_STYLE_MARKER) {
			go_marker_set_size (style->marker.mark,
						go_marker_get_size (style->marker.mark) *
						data->swatch_scale_a + data->swatch_scale_b);
			gog_renderer_draw_marker (renderer, data->x + data->swatch.w  * GLV_LINE_LENGTH_EM * 0.5, y);
		}
	} else if (base_style->interesting_fields & GO_STYLE_FILL) {					/* area swatch */
		style = go_style_dup (base_style);
		if (style->line.width > data->hairline_width)
			style->line.width =
				0.5 * (data->hairline_width + style->line.width) * data->line_scale_a +
				data->line_scale_b;

		rectangle = data->swatch;
		rectangle.x += data->x;
		rectangle.y += data->y;

		gog_renderer_push_style (renderer, style);
		gog_renderer_draw_rectangle (renderer, &rectangle);
	} else if (base_style->interesting_fields & GO_STYLE_MARKER) {					/* markers only */
		style = go_style_dup (base_style);
		g_return_if_fail (style != NULL);
		y = data->y + glv->element_height / 2.;
		gog_renderer_push_style (renderer, style);
		go_marker_set_size (style->marker.mark,
				    go_marker_get_size (style->marker.mark) *
				    data->swatch_scale_a + data->swatch_scale_b);
		/* Might be not the best horizontal position, but seems to work not that bad */
		gog_renderer_draw_marker (renderer, data->x + glv->label_offset * 0.5, y);
	} else {
		g_warning ("Series with no valid style in legend? Please file a bug report.");
		return;
	}
	gog_renderer_pop_style (renderer);

	pos.x = data->x + glv->label_offset;
	pos.y = data->y + glv->element_height / 2.0;
	pos.w = pos.h = -1;
	if (l) {
		GOString *str = go_string_new_rich (name, -1, l, NULL);
		gog_renderer_draw_gostring (view->renderer, str, &pos,
		                            GO_ANCHOR_W, GO_JUSTIFY_LEFT, -1.);
		go_string_unref (str);
	} else
		gog_renderer_draw_text (renderer, name, &pos,
		                        GO_ANCHOR_W, FALSE, GO_JUSTIFY_LEFT, -1.);

	if (style != base_style && style != NULL)
		g_object_unref (style);
}

static void
gog_legend_view_render (GogView *v, GogViewAllocation const *bbox)
{
	GogLegendView *glv = GOG_LEGEND_VIEW (v);
	GogLegend *l = GOG_LEGEND (v->model);
	GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (l));
	RenderClosure data;
	SwatchScaleClosure swatch_data;
	double swatch_size_min;
	double swatch_size_max;
	double hairline_width;

	(lview_parent_klass->render) (v, bbox);

	if (glv->element_per_blocks < 1)
		return;

	gog_renderer_push_clip_rectangle (v->renderer, v->residual.x, v->residual.y,
					  v->residual.w, v->residual.h);

	gog_renderer_push_style (v->renderer, style);

	hairline_width = gog_renderer_get_hairline_width_pts (v->renderer);
	swatch_data.size_max = 0.0;
	swatch_data.size_min = glv->font_size;
	swatch_data.line_scale = 1.0;
	swatch_data.line_width_max = GLV_LINE_WIDTH_MAX_DEFAULT_EM * glv->font_size;
	swatch_data.line_length = GLV_LINE_LENGTH_EM * glv->font_size;
	swatch_data.hairline_width = hairline_width;

	gog_chart_foreach_elem (GOG_CHART (v->model->parent), TRUE,
		(GogEnumFunc) cb_swatch_scale, &swatch_data);

	swatch_size_max = glv->font_size;
	swatch_size_min = glv->font_size * GLV_SWATCH_SIZE_MIN_EM;
	if (swatch_data.size_max < swatch_size_max)
		swatch_data.size_max = swatch_size_max;
	if (swatch_data.size_min > swatch_size_min)
		swatch_data.size_min = swatch_size_min;
	if (go_sub_epsilon (fabs (swatch_data.size_max - swatch_data.size_min)) > 0.0) {
		data.swatch_scale_a = (swatch_size_max - swatch_size_min) /
			(swatch_data.size_max - swatch_data.size_min);
		data.swatch_scale_b = - swatch_data.size_min *
			data.swatch_scale_a +swatch_size_min;
	} else {
		data.swatch_scale_a = 1.0;
		data.swatch_scale_b = 0.0;
	}
	data.line_scale_a = swatch_data.line_scale;
	data.line_scale_b = - hairline_width * data.line_scale_a + hairline_width;

	data.hairline_width = hairline_width;

/*	if (glv->uses_lines) {
		data.line_path[0].code = ART_MOVETO;
		data.line_path[1].code = ART_LINETO;
		data.line_path[2].code = ART_END;
	}*/

	data.count = 0;
	data.view = v;
	data.x = v->residual.x;
	data.y = v->residual.y;
	data.element_step_x = glv->is_vertical ? 0 : glv->element_width;
	data.element_step_y = glv->is_vertical ? glv->element_height : 0;
	data.block_step_x = glv->is_vertical ?
		+ glv->element_width :
		- glv->element_width * (glv->element_per_blocks - 1);
	data.block_step_y = glv->is_vertical ?
		- glv->element_height * (glv->element_per_blocks - 1) :
		+ glv->element_height;
	data.swatch.w = glv->swatch_w;
	data.swatch.h = glv->swatch_h;
	data.swatch.x = (glv->label_offset - 1.5 * data.swatch.w) * .5;
	data.swatch.y = (glv->element_height - data.swatch.h) * .5;

	gog_chart_foreach_elem (GOG_CHART (v->model->parent), TRUE,
		(GogEnumFunc) cb_render_elements, &data);

	gog_renderer_pop_style (v->renderer);

	gog_renderer_pop_clip (v->renderer);
}

static void
gog_legend_view_class_init (GogLegendViewClass *gview_klass)
{
	GogViewClass *view_klass    = (GogViewClass *) gview_klass;

	lview_parent_klass = g_type_class_peek_parent (gview_klass);
	view_klass->size_request    = gog_legend_view_size_request;
	view_klass->render	    = gog_legend_view_render;
}

static GSF_CLASS (GogLegendView, gog_legend_view,
		  gog_legend_view_class_init, NULL,
		  GOG_TYPE_OUTLINED_VIEW)
