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

struct _GogColorScale {
	GogStyledObject base;
	GogAxis *color_axis; /* the color or pseudo-3d axis */
	GogAxis *axis; /* the axis used to display the scale */
	gboolean horizontal;
	double width; /* will actually be height if horizontal */
};
typedef GogStyledObjectClass GogColorScaleClass;

static GObjectClass *parent_klass;
static GType gog_color_scale_view_get_type (void);

static void
gog_color_scale_init_style (GogStyledObject *gso, GOStyle *style)
{
	style->interesting_fields = GO_STYLE_LINE | GO_STYLE_FONT | GO_STYLE_TEXT_LAYOUT;
	gog_theme_fillin_style (gog_object_get_theme (GOG_OBJECT (gso)),
				style, GOG_OBJECT (gso), 0, GO_STYLE_LINE |
	                        GO_STYLE_FONT | GO_STYLE_TEXT_LAYOUT);
}

enum {
	COLOR_SCALE_PROP_0,
	COLOR_SCALE_PROP_HORIZONTAL,
	COLOR_SCALE_PROP_WIDTH
};

static void
gog_color_scale_set_property (GObject *obj, guint param_id,
		       GValue const *value, GParamSpec *pspec)
{
	GogColorScale *scale = GOG_COLOR_SCALE (obj);

	switch (param_id) {
	case COLOR_SCALE_PROP_HORIZONTAL:
		scale->horizontal = g_value_get_boolean (value);
		g_object_set (G_OBJECT (scale->axis), "type",
		              scale->horizontal? GOG_AXIS_X: GOG_AXIS_Y, NULL);
		break;
	case COLOR_SCALE_PROP_WIDTH:
		scale->width = g_value_get_double (value);
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

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
}

static void
gog_color_scale_finalize (GObject *obj)
{
	GogColorScale *scale = GOG_COLOR_SCALE (obj);

	g_object_unref (G_OBJECT (scale->axis));

	parent_klass->finalize (obj);
}

static void
gog_color_scale_class_init (GObjectClass *gobject_klass)
{
	GogObjectClass *gog_klass = (GogObjectClass *) gobject_klass;
	GogStyledObjectClass *style_klass = (GogStyledObjectClass *) gog_klass;

	parent_klass = g_type_class_peek_parent (gobject_klass);
	/* GObjectClass */
	gobject_klass->finalize = gog_color_scale_finalize;
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
			0., 255., 10.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	gog_klass->view_type	= gog_color_scale_view_get_type ();

	style_klass->init_style = gog_color_scale_init_style;
}

static void
gog_color_scale_init (GogColorScale *scale)
{
	scale->width = 10;
	scale->axis = (GogAxis *) g_object_new (GOG_TYPE_AXIS,
	                                        "type", GOG_AXIS_Y,
	                                        NULL);
}

GSF_CLASS (GogColorScale, gog_color_scale,
	   gog_color_scale_class_init, gog_color_scale_init,
	   GOG_TYPE_STYLED_OBJECT)

/************************************************************************/

typedef GogView		GogColorScaleView;
typedef GogViewClass	GogColorScaleViewClass;

#define GOG_TYPE_COLOR_SCALE_VIEW	(gog_color_scale_view_get_type ())
#define GOG_COLOR_SCALE_VIEW(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_COLOR_SCALE_VIEW, GogColorScaleView))
#define GOG_IS_COLOR_SCALE_VIEW(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_COLOR_SCALE_VIEW))

static void
gog_color_scale_view_size_request (GogView *v,
                                   GogViewRequisition const *available,
                                   GogViewRequisition *req)
{
}

static void
gog_color_scale_view_render (GogView *view, GogViewAllocation const *bbox)
{
}

static void
gog_color_scale_view_class_init (GogColorScaleViewClass *gview_klass)
{
	GogViewClass *view_klass    = (GogViewClass *) gview_klass;

	view_klass->size_request = gog_color_scale_view_size_request;
	view_klass->render = gog_color_scale_view_render;
}

static GSF_CLASS (GogColorScaleView, gog_color_scale_view,
	   gog_color_scale_view_class_init, NULL,
	   GOG_TYPE_VIEW)
