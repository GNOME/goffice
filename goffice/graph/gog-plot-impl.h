/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-plot-impl.h : implementation details for the abstract 'plot' interface
 *
 * Copyright (C) 2003-2004 Jody Goldberg (jody@gnome.org)
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

#ifndef GOG_PLOT_IMPL_H
#define GOG_PLOT_IMPL_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

struct _GogPlotDesc {
	unsigned  num_series_max;
	unsigned  num_axis;
	GogSeriesDesc series;
};

struct _GogPlot {
	GogObject	 base;

	GSList		*series;
	unsigned	 full_cardinality, visible_cardinality;
	gboolean	 cardinality_valid;
	unsigned	 index_num;
	gboolean	 vary_style_by_element;
	union {
		GogPlotRenderingOrder rendering_order;
		gboolean render_before_axes; /* Deprecated */
	};
	gchar		*plot_group;
	char		*guru_hints;

	GOLineInterpolation	interpolation;

	GogAxis		*axis[GOG_AXIS_TYPES];

	/* Usually a copy from the class but it's here to allow a GogPlotType to
	 * override things without requiring a completely new class */
	GogPlotDesc	desc;
};

typedef struct {
	GogObjectClass base;

	GogPlotDesc	desc;
	GType		series_type;

	GogAxisSet	axis_set;

	/* Virtuals */

	GOData	  *(*axis_get_bounds) 	(GogPlot *plot, GogAxisType axis,
					 GogPlotBoundInfo *bounds);

	gboolean   (*supports_vary_style_by_element) (GogPlot const *plot);

	/* %TRUE if the plot prefers to display series in reverse order for
	 * legends (e.g. stacked plots want top element to be the last series) */
	gboolean   (*enum_in_reverse) (GogPlot const *plot);

	void       (*foreach_elem)    	(GogPlot *plot, gboolean only_visible,
					 GogEnumFunc handler, gpointer data);

	void       (*update_3d)		(GogPlot *plot);
	void	   (*guru_helper)	(GogPlot *plot, char const *hint);
} GogPlotClass;

#define GOG_PLOT_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST ((k), GOG_TYPE_PLOT, GogPlotClass))
#define GOG_IS_PLOT_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), GOG_TYPE_PLOT))
#define GOG_PLOT_ITEM_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GOG_TYPE_PLOT, GogPlotClass))

/* protected */

/*****************************************************************************/

#define GOG_TYPE_PLOT_VIEW	(gog_plot_view_get_type ())
#define GOG_PLOT_VIEW(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_PLOT_VIEW, GogPlotView))
#define GOG_IS_PLOT_VIEW(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_PLOT_VIEW))
#define GOG_PLOT_VIEW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GOG_TYPE_PLOT_VIEW, GogPlotViewClass))

struct _GogPlotView {
	GogView base;
};
typedef struct {
	GogViewClass base;
	int     (*get_data_at_point)    (GogPlotView *view, double x, double y, GogSeries **series);
} GogPlotViewClass;
GType gog_plot_view_get_type (void);

G_END_DECLS

#endif /* GOG_PLOT_GROUP_IMPL_H */
