/*
 * gog-color-scale.h
 *
 * Copyright (C) 2012 Jean Brefort (jean.brefort@normalesup.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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

#ifdef GOFFICE_WITH_GTK
#include <gtk/gtk.h>
#endif

/**
 * SECTION: gog-color-scale
 * @short_description: Displays the color scale used by an axis.
 *
 * A color scale has two parts: an axis, and a rectangle filled by the colors
 * corresponding to the axis scale. It can be displayed horizontally or
 * vertically.
 **/
struct _GogColorScale {
	GogStyledObject base;
	GogAxis *color_axis; /* the color or pseudo-3d axis */
	gboolean horizontal;
	gboolean axis_at_low;	/* axis position on low coordinates side */
	double width; /* will actually be height of the colored rectangle if
	 			   * horizontal */
	int tick_size;
};
typedef GogStyledObjectClass GogColorScaleClass;

static GObjectClass *parent_klass;
static GType gog_color_scale_view_get_type (void);

static void
gog_color_scale_init_style (GogStyledObject *gso, GOStyle *style)
{
	style->interesting_fields = GO_STYLE_LINE | GO_STYLE_FILL |
	                        GO_STYLE_FONT | GO_STYLE_TEXT_LAYOUT;
	gog_theme_fillin_style (gog_object_get_theme (GOG_OBJECT (gso)),
	                        style, GOG_OBJECT (gso), 0,
	                        GO_STYLE_LINE | GO_STYLE_FILL |
	                        GO_STYLE_FONT | GO_STYLE_TEXT_LAYOUT);
}

static void
gog_color_scale_set_orientation (GogColorScale *scale, gboolean horizontal)
{
	GogObjectPosition pos;
	GogObject *gobj = GOG_OBJECT (scale);
	GSList *l, *ptr;
	GOStyle *style;
	scale->horizontal = horizontal;
	/* if the position is automatic, try to adjust accordingly */
	pos = gog_object_get_position_flags (gobj, GOG_POSITION_MANUAL);
	if (!pos) {
		gboolean changed = TRUE;
		pos = gog_object_get_position_flags (gobj, GOG_POSITION_COMPASS);
		switch (pos) {
		case GOG_POSITION_N:
			pos = GOG_POSITION_W;
			break;
		case GOG_POSITION_S:
			pos = GOG_POSITION_E;
			break;
		case GOG_POSITION_E:
			pos = GOG_POSITION_S;
			break;
		case GOG_POSITION_W:
			pos = GOG_POSITION_N;
			break;
		default:
			changed = FALSE;
		}
		if (changed)
			gog_object_set_position_flags (gobj, pos, GOG_POSITION_COMPASS);
	}
	pos = gog_object_get_position_flags (gobj, GOG_POSITION_ANY_MANUAL_SIZE);
	if (pos) {
		GogViewAllocation alloc;
		double buf;
		switch (pos) {
		case GOG_POSITION_MANUAL_W:
			gog_object_set_position_flags (gobj, GOG_POSITION_MANUAL_H,
				                           GOG_POSITION_ANY_MANUAL_SIZE);
			break;
		case GOG_POSITION_MANUAL_W_ABS:
			gog_object_set_position_flags (gobj, GOG_POSITION_MANUAL_H_ABS,
				                           GOG_POSITION_ANY_MANUAL_SIZE);
			break;
		case GOG_POSITION_MANUAL_H:
			gog_object_set_position_flags (gobj, GOG_POSITION_MANUAL_W,
				                           GOG_POSITION_ANY_MANUAL_SIZE);
			break;
		case GOG_POSITION_MANUAL_H_ABS:
			gog_object_set_position_flags (gobj, GOG_POSITION_MANUAL_W_ABS,
				                           GOG_POSITION_ANY_MANUAL_SIZE);
			break;
		default:
			break;
		}
		gog_object_get_manual_position (gobj, &alloc);
		buf = alloc.w;
		alloc.w = alloc.h;
		alloc.h = buf;
       	gog_object_set_manual_position (gobj, &alloc);
	}
	l = gog_object_get_children (gobj, NULL);
	for (ptr = l; ptr; ptr = ptr->next) {
		if (GO_IS_STYLED_OBJECT (ptr->data)) {
			style = go_style_dup (go_styled_object_get_style (ptr->data));
			go_styled_object_set_style (ptr->data, style);
			g_object_unref (style);
		}
	}
	g_slist_free (l);
}

#ifdef GOFFICE_WITH_GTK

static void
position_fill_cb (GtkComboBoxText *box, GogColorScale *scale)
{
	if (scale->horizontal) {
		gtk_combo_box_text_append_text (box, _("Top"));
		gtk_combo_box_text_append_text (box, _("Bottom"));
	} else {
		gtk_combo_box_text_append_text (box, _("Left"));
		gtk_combo_box_text_append_text (box, _("Right"));
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (box), (scale->axis_at_low)? 0: 1);
}

static void
position_changed_cb (GtkComboBox *box, GogColorScale *scale)
{
	scale->axis_at_low = gtk_combo_box_get_active (box) == 0;
	gog_object_emit_changed (GOG_OBJECT (scale), FALSE);
}

static void
direction_changed_cb (GtkComboBox *box, GogColorScale *scale)
{
	GtkComboBoxText *text;
	gog_color_scale_set_orientation (scale, gtk_combo_box_get_active (box) == 0);
	g_signal_emit_by_name (scale, "update-editor");
	text = GTK_COMBO_BOX_TEXT (g_object_get_data (G_OBJECT (box), "position"));
	g_signal_handlers_block_by_func (text, position_changed_cb, scale);
	gtk_list_store_clear (GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (text))));
	position_fill_cb (text, scale);
	g_signal_handlers_unblock_by_func (text, position_changed_cb, scale);
}

static void
width_changed_cb (GtkSpinButton *btn, GogColorScale *scale)
{
	scale->width = gtk_spin_button_get_value_as_int (btn);
	gog_object_emit_changed (GOG_OBJECT (scale), TRUE);
}

static void
gog_color_scale_populate_editor (GogObject *gobj,
			   GOEditor *editor,
			   G_GNUC_UNUSED GogDataAllocator *dalloc,
			   GOCmdContext *cc)
{
	GogColorScale *scale = GOG_COLOR_SCALE (gobj);
	GtkBuilder *gui;
	GtkWidget *w, *w_;

	gui = go_gtk_builder_load_internal ("res:go:graph/gog-color-scale-prefs.ui", GETTEXT_PACKAGE, cc);
	if (gui == NULL)
		return;

	w = go_gtk_builder_get_widget (gui, "direction-btn");
	gtk_combo_box_set_active (GTK_COMBO_BOX (w), (scale->horizontal)? 0: 1);
	g_signal_connect (w, "changed", G_CALLBACK (direction_changed_cb), scale);

	w_ = go_gtk_builder_get_widget (gui, "position-btn");
	g_object_set_data (G_OBJECT (w), "position", w_);
	position_fill_cb (GTK_COMBO_BOX_TEXT (w_), scale);
	g_signal_connect (w_, "changed", G_CALLBACK (position_changed_cb), scale);

	w = go_gtk_builder_get_widget (gui, "width-btn");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), scale->width);
	g_signal_connect (w, "changed", G_CALLBACK (width_changed_cb), scale);

	go_editor_add_page (editor, go_gtk_builder_get_widget (gui, "color-scale-prefs"), _("Details"));
	g_object_unref (gui);
	((GogObjectClass *) parent_klass)->populate_editor (gobj, editor, dalloc, cc);
}
#endif

static GogManualSizeMode
gog_color_scale_get_manual_size_mode (GogObject *obj)
{
	return GOG_COLOR_SCALE (obj)->horizontal? GOG_MANUAL_SIZE_WIDTH: GOG_MANUAL_SIZE_HEIGHT;
}

enum {
	COLOR_SCALE_PROP_0,
	COLOR_SCALE_PROP_HORIZONTAL,
	COLOR_SCALE_PROP_WIDTH,
	COLOR_SCALE_PROP_AXIS,
	COLOR_SCALE_PROP_TICK_SIZE_PTS
};

static void
gog_color_scale_set_property (GObject *obj, guint param_id,
		       GValue const *value, GParamSpec *pspec)
{
	GogColorScale *scale = GOG_COLOR_SCALE (obj);

	switch (param_id) {
	case COLOR_SCALE_PROP_HORIZONTAL:
		gog_color_scale_set_orientation (scale, g_value_get_boolean (value));
		break;
	case COLOR_SCALE_PROP_WIDTH:
		scale->width = g_value_get_double (value);
		break;
	case COLOR_SCALE_PROP_AXIS: {
		GogChart *chart = GOG_CHART (gog_object_get_parent (GOG_OBJECT (obj)));
		GSList *ptr;
		char const *buf = g_value_get_string (value);
		GogAxisType	 type;
		int id;
		GogAxis *axis = NULL;
		if (!strncmp (buf, "color", 5)) {
			type = GOG_AXIS_COLOR;
			buf += 5;
		} else if (!strncmp (buf, "3d", 2)) {
			type = GOG_AXIS_PSEUDO_3D;
			buf += 2;
		} else {
			unsigned index;
			/* kludge: old file searching for 3D in all known locales */
			type = (strstr (buf, "3D") != NULL || strstr (buf , "3d") !=NULL ||
					strstr (buf, "3Д") != NULL || strstr (buf, "3Δ") != NULL ||
					strstr (buf, "ترويسات متعلقة بالمحور") != NULL)?
					GOG_AXIS_PSEUDO_3D: GOG_AXIS_COLOR;
			index = strlen (buf) - 1;
			while (index > 0 && buf[index] <= '9' && buf[index] >= '0')
				index--;
			buf += index + 1;	
		}
		id = atoi (buf);
		if (id < 1) /* this should not happen */
				id = 1;
		for (ptr = gog_chart_get_axes (chart, type); ptr && ptr->data; ptr = ptr->next) {
			if ((unsigned ) id == gog_object_get_id (GOG_OBJECT (ptr->data))) {
			    axis = GOG_AXIS (ptr->data);
				break;
			}
		}
		gog_color_scale_set_axis (scale, axis);
		break;
	}
	case COLOR_SCALE_PROP_TICK_SIZE_PTS:
		scale->tick_size = g_value_get_int (value);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}

	gog_object_emit_changed (GOG_OBJECT (obj), TRUE);
}

static void
gog_color_scale_get_property (GObject *obj, guint param_id,
		       GValue *value, GParamSpec *pspec)
{
	GogColorScale *scale = GOG_COLOR_SCALE (obj);

	switch (param_id) {
	case COLOR_SCALE_PROP_HORIZONTAL:
		g_value_set_boolean (value, scale->horizontal);
		break;
	case COLOR_SCALE_PROP_WIDTH:
		g_value_set_double (value, scale->width);
		break;
	case COLOR_SCALE_PROP_AXIS: {
		char buf[16];
		if (gog_axis_get_atype (scale->color_axis) == GOG_AXIS_COLOR)
			snprintf (buf, 16, "color%u", gog_object_get_id (GOG_OBJECT (scale->color_axis)));
		else
			snprintf (buf, 16, "3d%u", gog_object_get_id (GOG_OBJECT (scale->color_axis)));
		g_value_set_string (value, buf);
		break;
	}
	case COLOR_SCALE_PROP_TICK_SIZE_PTS:
		g_value_set_int (value, scale->tick_size);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
}

static void
gog_color_scale_class_init (GObjectClass *gobject_klass)
{
	GogObjectClass *gog_klass = (GogObjectClass *) gobject_klass;
	GogStyledObjectClass *style_klass = (GogStyledObjectClass *) gog_klass;
	static GogObjectRole const roles[] = {
		{ N_("Label"), "GogLabel", 0,
		  GOG_POSITION_SPECIAL|GOG_POSITION_ANY_MANUAL, GOG_POSITION_SPECIAL, GOG_OBJECT_NAME_BY_ROLE,
		  NULL, NULL, NULL, NULL, NULL, NULL, { -1 } }
	};

	parent_klass = g_type_class_peek_parent (gobject_klass);
	/* GObjectClass */
	gobject_klass->get_property = gog_color_scale_get_property;
	gobject_klass->set_property = gog_color_scale_set_property;
	g_object_class_install_property (gobject_klass, COLOR_SCALE_PROP_HORIZONTAL,
		g_param_spec_boolean ("horizontal", _("Horizontal"),
			_("Whether to display the scale horizontally"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, COLOR_SCALE_PROP_WIDTH,
		g_param_spec_double ("width", _("Width"),
			_("Color scale thickness."),
			1., 255., 10.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, COLOR_SCALE_PROP_AXIS,
		g_param_spec_string ("axis",
			_("Axis"),
			_("Reference to the color or pseudo-3d axis"),
			NULL,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, COLOR_SCALE_PROP_TICK_SIZE_PTS,
		g_param_spec_int ("tick-size-pts",
			_("Tick size"),
			_("Size of the tick marks, in points"),
			0, 20, 4,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	gog_klass->get_manual_size_mode = gog_color_scale_get_manual_size_mode;
	gog_klass->view_type	= gog_color_scale_view_get_type ();
#ifdef GOFFICE_WITH_GTK
	gog_klass->populate_editor = gog_color_scale_populate_editor;
#endif
	gog_object_register_roles (gog_klass, roles, G_N_ELEMENTS (roles));
	style_klass->init_style = gog_color_scale_init_style;
}

static void
gog_color_scale_init (GogColorScale *scale)
{
	scale->width = 10;
	scale->tick_size = 4;
}

GSF_CLASS (GogColorScale, gog_color_scale,
	   gog_color_scale_class_init, gog_color_scale_init,
	   GOG_TYPE_STYLED_OBJECT)

/**
 * gog_color_scale_get_axis:
 * @scale: #GogColorScale
 *
 * Gets the axis mapping to the colors and associated with @scale
 * Returns: (transfer none): the associated axis.
 **/
GogAxis *
gog_color_scale_get_axis (GogColorScale *scale)
{
	g_return_val_if_fail (GOG_IS_COLOR_SCALE (scale), NULL);
	return scale->color_axis;
}

/**
 * gog_color_scale_set_axis:
 * @scale: #GogColorScale
 * @axis: a color or pseudo-3d axis
 *
 * Associates the axis with @scale.
 **/
void
gog_color_scale_set_axis (GogColorScale *scale, GogAxis *axis)
{
	g_return_if_fail (GOG_IS_COLOR_SCALE (scale));
	if (scale->color_axis == axis)
		return;
	if (scale->color_axis != NULL)
		_gog_axis_set_color_scale (scale->color_axis, NULL);
	scale->color_axis = axis;
	if (axis)
		_gog_axis_set_color_scale (axis, scale);
}

/************************************************************************/

typedef struct {
	GogView	base;
	GogViewAllocation scale_area;
} GogColorScaleView;
typedef GogViewClass	GogColorScaleViewClass;

static GogViewClass *view_parent_class;

#define GOG_TYPE_COLOR_SCALE_VIEW	(gog_color_scale_view_get_type ())
#define GOG_COLOR_SCALE_VIEW(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_COLOR_SCALE_VIEW, GogColorScaleView))
#define GOG_IS_COLOR_SCALE_VIEW(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_COLOR_SCALE_VIEW))

static void
gog_color_scale_view_size_request (GogView *v,
                                   GogViewRequisition const *available,
                                   GogViewRequisition *req)
{
	GogColorScale *scale = GOG_COLOR_SCALE (v->model);
	GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (scale));
	double line_width, tick_size, label_padding, max_width = 0., max_height = 0.;
	double extra_h = 0., extra_w = 0.;
	unsigned nb_ticks, i;
	GogAxisTick *ticks;
	GOGeometryAABR txt_aabr;
	GOGeometryOBR txt_obr;
	GSList *ptr;
	GogView *child;
	GogObjectPosition pos;
	GogViewRequisition child_req;

	gog_renderer_push_style (v->renderer, style);
	gog_renderer_get_text_OBR (v->renderer, "0", TRUE, &txt_obr, -1.);
	label_padding = txt_obr.h * .15;
	nb_ticks = gog_axis_get_ticks (scale->color_axis, &ticks);
	for (i = 0; i < nb_ticks; i++)
		if (ticks[i].type == GOG_AXIS_TICK_MAJOR) {
			gog_renderer_get_gostring_AABR (v->renderer, ticks[i].str, &txt_aabr, 0.);
			if (txt_aabr.w > max_width)
				max_width = txt_aabr.w;
			if (txt_aabr.h > max_height)
				max_height = txt_aabr.h;
		}

	if (go_style_is_line_visible (style)) {
		line_width = gog_renderer_line_size (v->renderer, style->line.width);
		tick_size = gog_renderer_pt2r (v->renderer, scale->tick_size);
	} else
		line_width = tick_size = 0.;
	if (scale->horizontal) {
		req->w = available->w;
		req->h = gog_renderer_pt2r (v->renderer, scale->width)
				 + 2 * line_width + tick_size + label_padding + max_height;
	} else {
		req->h = available->h;
		req->w = gog_renderer_pt2r (v->renderer, scale->width)
				 + 2 * line_width + tick_size + label_padding + max_width;
	}
	gog_renderer_pop_style (v->renderer);
	for (ptr = v->children; ptr != NULL ; ptr = ptr->next) {
		child = ptr->data;
		pos = child->model->position;
		if (!(pos & GOG_POSITION_MANUAL)) {
			gog_view_size_request (child, available, &child_req);
			if (scale->horizontal)
				extra_h += child_req.h;
			else
				extra_w += child_req.w;
		} else {
		}
	}
	req->w += extra_w;
	req->h += extra_h;
}

static void
gog_color_scale_view_size_allocate (GogView *view, GogViewAllocation const *bbox)
{
	GogColorScale *scale = GOG_COLOR_SCALE (view->model);
	GSList *ptr;
	GogView *child;
	GogObjectPosition pos = view->model->position, child_pos;
	GogViewAllocation child_alloc, res = *bbox;
	GogViewRequisition available, req;
	for (ptr = view->children; ptr != NULL ; ptr = ptr->next) {
		child = ptr->data;
		child_pos = child->model->position;
		available.w = res.w;
		available.h = res.h;
		if (child_pos & GOG_POSITION_MANUAL) {
			gog_view_size_request (child, &available, &req);
			child_alloc = gog_object_get_manual_allocation (gog_view_get_model (child),
			                                                bbox, &req);
		} else if (child_pos == GOG_POSITION_SPECIAL) {
			gog_view_size_request (child, &available, &req);
			if (scale->horizontal) {
				if (pos & GOG_POSITION_N) {
					child_alloc.y = res.y;
					res.y += req.h;
				} else {
					child_alloc.y = res.y + res.h - req.h;
				}
				res.h -= req.h;
				child_alloc.x = res.x + (res.w - req.w) / 2.;
			} else {
				if (pos & GOG_POSITION_W) {
					child_alloc.x = res.x;
					res.x += req.w;
				} else {
					child_alloc.x = res.x + res.w - req.w;
				}
				res.w -= req.w;
				child_alloc.y = res.y + (res.h - req.h) / 2.;
				child_alloc.w = req.w;
				child_alloc.h = req.h;
			}
			child_alloc.w = req.w;
			child_alloc.h = req.h;
		} else {
			g_warning ("[GogColorScaleView::size_allocate] unexpected position %x for child %p of %p",
				   pos, child, view);
			continue;
		}
		gog_view_size_allocate (child, &child_alloc);
	}
	view->residual = res;
}

static void
gog_color_scale_view_render (GogView *view, GogViewAllocation const *bbox)
{
	GogColorScale *scale = GOG_COLOR_SCALE (view->model);
	unsigned nb_ticks, nb_maj_ticks = 0, i, j, l;
	GogAxisTick *ticks;
	GogViewAllocation scale_area, label_pos;
	GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (scale));
	double line_width, tick_size, label_padding, width, pos, start, stop;
	GOGeometryOBR txt_obr;
	GOGeometryOBR *obrs = NULL;
	GogAxisMap *map;
	GOPath *path;
	gboolean is_line_visible;
	double min, max, first, last = 0., hf = 0., hl = 0.;
	GOAnchorType anchor;
	gboolean discrete = gog_axis_get_atype (scale->color_axis) == GOG_AXIS_PSEUDO_3D;
	GogAxisColorMap const *cmap = gog_axis_get_color_map (scale->color_axis);

	gog_renderer_push_style (view->renderer, style);
	nb_ticks = gog_axis_get_ticks (scale->color_axis, &ticks);
	for (i = 0; i < nb_ticks; i++)
		if (ticks[i].type == GOG_AXIS_TICK_MAJOR)
			nb_maj_ticks++;
	gog_renderer_get_text_OBR (view->renderer, "0", TRUE, &txt_obr, -1.);
	label_padding = txt_obr.h * .15;
	width = gog_renderer_pt2r (view->renderer, scale->width);
	obrs = g_new0 (GOGeometryOBR, nb_maj_ticks);
	gog_axis_get_bounds (scale->color_axis, &min, &max);
	first = min - 1.;
	/* evaluate labels size, and find first and last label */
	for (i = 0, j = 0; i < nb_ticks; i++)
		if (ticks[i].type == GOG_AXIS_TICK_MAJOR) {
			gog_renderer_get_gostring_OBR (view->renderer, ticks[i].str, obrs + j, -1.);
			if (first < min) {
				first = last = ticks[i].position;
				hf = hl = scale->horizontal? obrs[j].w: obrs[j].h;
			} else if (ticks[i].position > last) {
				last = ticks[i].position;
				hl = scale->horizontal? obrs[j].w: obrs[j].h;
			} else if (ticks[i].position < first) {
				first = ticks[i].position;
				hf = scale->horizontal? obrs[j].w: obrs[j].h;
			}
			j++;
		}
	hf /= 2.;
	hl /= 2.;

	/* evaluate color scale area */
	if ((is_line_visible = go_style_is_line_visible (style))) {
		line_width = gog_renderer_line_size (view->renderer, style->line.width);
		tick_size = gog_renderer_pt2r (view->renderer, scale->tick_size);
	} else
		line_width = tick_size = 0.;
	if (min == max || first == last) {
		/* avoid infinites and nans later, this might happen if no data are available */
		if (min < first)
			first = min;
		else
			min = first;
		if (max > last)
			last = max;
		else
			max = last;
	}
	if (scale->horizontal) {
		scale_area.x = view->residual.x;
		/* we make sure that we have enough room to display the last and first labels */
		pos = (scale_area.w = view->residual.w) - hf - hl;
		stop = hl - pos * (max - last) / (max - min);
		start = hf - pos * (first - min) / (max - min);
		if (start < 0)
			start = 0.;
		if (stop < 0.)
			stop = 0.;
		/* we might actually remove slightly more than needed */
		scale_area.w -= start + stop;
		scale_area.x += gog_axis_is_inverted (scale->color_axis)? stop: start;
		scale_area.y = (scale->axis_at_low)?
			view->residual.y + view->residual.h - width - 2 * line_width:
			view->residual.y;
		scale_area.h = width + 2 * line_width;
	} else {
		scale_area.x = (scale->axis_at_low)?
			view->residual.x + view->residual.w - width - 2 * line_width:
			view->residual.x;
		scale_area.w = width + 2 * line_width;
		scale_area.y = view->residual.y;
		/* we make sure that we have enough room to display the last and first labels */
		pos = (scale_area.h = view->residual.h) - hf -hl;
		stop = hl - pos * (max - last) / (max - min);
		start = hf - pos * (first - min) / (max - min);
		if (start < 0)
			start = 0.;
		if (stop < 0.)
			stop = 0.;
		/* we might actually remove slightly more than needed */
		scale_area.h -= start + stop;
		scale_area.y += gog_axis_is_inverted (scale->color_axis)? start: stop;
	}

	gog_renderer_stroke_rectangle (view->renderer, &scale_area);
	scale_area.x += line_width / 2.;
	scale_area.w -= line_width;
	scale_area.y += line_width / 2.;
	scale_area.h -= line_width;
	if (scale->horizontal)
		map = gog_axis_map_new (scale->color_axis, scale_area.x, scale_area.w);
	else
		map = gog_axis_map_new (scale->color_axis, scale_area.y + scale_area.h, -scale_area.h);
	if (discrete) {
		GOStyle *dstyle;
		double *limits, epsilon, sc, *x, *w;
		/* some of the code was copied from gog-contour.c to be consistent */
		i = j = 0;
		epsilon = (max - min) / nb_ticks * 1e-10; /* should avoid rounding errors */
		while (ticks[i].type != GOG_AXIS_TICK_MAJOR)
			i++;
		if (ticks[i].position - min > epsilon) {
			limits = g_new (double, nb_maj_ticks + 2);
			limits[j++] = min;
		} else
			limits = g_new (double, nb_maj_ticks + 1);
		for (; i < nb_ticks; i++)
			if (ticks[i].type == GOG_AXIS_TICK_MAJOR)
				limits[j++] = ticks[i].position;
		if (j == 0 || max - limits[j - 1] > epsilon)
			limits[j] = max;
		else
			j--;
		sc = (j > gog_axis_color_map_get_max (cmap) && j > 1)? (double) gog_axis_color_map_get_max (cmap) / (j - 1): 1.;
		dstyle = go_style_dup (style);
		dstyle->interesting_fields = GO_STYLE_FILL | GO_STYLE_OUTLINE;
		dstyle->fill.type = GO_STYLE_FILL_PATTERN;
		dstyle->fill.pattern.pattern = GO_PATTERN_SOLID;
		gog_renderer_push_style (view->renderer, dstyle);
		/* using label_pos as drawing rectangle */
		if (scale->horizontal) {
			x = &label_pos.x;
			w = &label_pos.w;
			label_pos.y = scale_area.y;
			label_pos.h = scale_area.h;
		} else {
			x = &label_pos.y;
			w = &label_pos.h;
			label_pos.x = scale_area.x;
			label_pos.w = scale_area.w;
		}
		if (gog_axis_is_inverted (scale->color_axis)) {
			for (i = 0; i < j; i++) {
				dstyle->fill.pattern.back = (j < 2)? GO_COLOR_WHITE: gog_axis_color_map_get_color (cmap, i * sc);
				*x = gog_axis_map_to_view (map, limits[j - i - 1]);
				*w = gog_axis_map_to_view (map, limits[j - i]) - *x;
				gog_renderer_fill_rectangle (view->renderer, &label_pos);
			}
			if (limits[i - j] - min > epsilon) {
				dstyle->fill.pattern.back = (j < 2)? GO_COLOR_WHITE: gog_axis_color_map_get_color (cmap, i * sc);
				*x = gog_axis_map_to_view (map, min);
				*w = gog_axis_map_to_view (map, limits[i - j]) - *x;
				gog_renderer_fill_rectangle (view->renderer, &label_pos);
			}
		} else {
			if (epsilon < limits[0] - min) {
				dstyle->fill.pattern.back = (j < 2)? GO_COLOR_WHITE: gog_axis_color_map_get_color (cmap, 0);
				*x = gog_axis_map_to_view (map, min);
				*w = gog_axis_map_to_view (map, limits[0]) - *x;
				gog_renderer_fill_rectangle (view->renderer, &label_pos);
				i = 1;
				j++;
			} else
				i = 0;
			for (; i < j; i++) {
				dstyle->fill.pattern.back = (j < 2)? GO_COLOR_WHITE: gog_axis_color_map_get_color (cmap, i * sc);
				*x = gog_axis_map_to_view (map, limits[i]);
				*w = gog_axis_map_to_view (map, limits[i + 1]) - *x;
				gog_renderer_fill_rectangle (view->renderer, &label_pos);
			}
		}
		gog_renderer_pop_style (view->renderer);
		g_free (limits);
		g_object_unref (dstyle);
	} else
		gog_renderer_draw_color_map (view->renderer, cmap,
		                             FALSE, scale->horizontal, &scale_area);
	/* draw ticks */
	if (scale->horizontal) {
		if (scale->axis_at_low) {
			stop = scale_area.y - line_width;
			start = stop - tick_size;
			anchor = GO_ANCHOR_SOUTH;
			if (discrete)
				stop += scale_area.h;
		} else {
			stop = scale_area.y + scale_area.h + line_width;
			start = stop + tick_size;
			anchor = GO_ANCHOR_NORTH;
			if (discrete)
				stop -= scale_area.h;
		}
		j = l = 0;
		for (i = 0; i < nb_ticks; i++)
			/* Only major ticks are displayed, at least for now */
			if (ticks[i].type == GOG_AXIS_TICK_MAJOR) {
				pos = gog_axis_map_to_view (map, ticks[i].position);
				if (is_line_visible) {
					path = go_path_new ();
					go_path_move_to (path, pos, start);
					go_path_line_to (path, pos, stop);
					gog_renderer_stroke_shape (view->renderer, path);
					go_path_free (path);
				}
				label_pos.x = pos;
				label_pos.y = start + ((scale->axis_at_low)? -label_padding: label_padding);
				obrs[j].x = label_pos.x - obrs[j].w / 2.;
				obrs[j].y = pos;
				if (j == 0 || !go_geometry_test_OBR_overlap (obrs + j, obrs + l)) {
					gog_renderer_draw_gostring (view->renderer, ticks[i].str,
						                        &label_pos, anchor,
							                    GO_JUSTIFY_CENTER, -1.);
					l = j;
				}
				j++;
			}
	} else {
		if (scale->axis_at_low) {
			stop = scale_area.x - line_width;
			start = stop - tick_size;
			anchor = GO_ANCHOR_EAST;
			if (discrete)
				stop += scale_area.w;
		} else {
			stop = scale_area.x + scale_area.w + line_width;
			start = stop + tick_size;
			anchor = GO_ANCHOR_WEST;
			if (discrete)
				stop -= scale_area.w;
		}
		j = l = 0;
		for (i = 0; i < nb_ticks; i++)
			/* Only major ticks are displayed, at least for now */
			if (ticks[i].type == GOG_AXIS_TICK_MAJOR) {
				pos = gog_axis_map_to_view (map, ticks[i].position);
				if (is_line_visible) {
					path = go_path_new ();
					go_path_move_to (path, start, pos);
					go_path_line_to (path, stop, pos);
					gog_renderer_stroke_shape (view->renderer, path);
					go_path_free (path);
				}
				label_pos.x = start + ((scale->axis_at_low)? -label_padding: label_padding);
				label_pos.y = pos;
				obrs[j].x = label_pos.x;
				obrs[j].y = pos - obrs[j].h / 2.;
				if (j == 0 || !go_geometry_test_OBR_overlap (obrs + j, obrs + l)) {
					gog_renderer_draw_gostring (view->renderer, ticks[i].str,
						                        &label_pos, anchor,
							                    GO_JUSTIFY_CENTER, -1.);
					l = j;
				}
				j++;
			}
	}
	g_free (obrs);
	gog_axis_map_free (map);
	gog_renderer_pop_style (view->renderer);
	view_parent_class->render (view, bbox);
}

static void
gog_color_scale_view_class_init (GogColorScaleViewClass *gview_klass)
{
	GogViewClass *view_klass    = (GogViewClass *) gview_klass;

	view_parent_class = g_type_class_peek_parent (gview_klass);
	view_klass->size_request = gog_color_scale_view_size_request;
	view_klass->render = gog_color_scale_view_render;
	view_klass->size_allocate = gog_color_scale_view_size_allocate;
}

static GSF_CLASS (GogColorScaleView, gog_color_scale_view,
	   gog_color_scale_view_class_init, NULL,
	   GOG_TYPE_VIEW)
