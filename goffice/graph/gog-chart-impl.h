/*
 * gog-chart-impl.h : implementation details for charts
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

#ifndef GOG_CHART_IMPL_H
#define GOG_CHART_IMPL_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

struct _GogChart {
	GogOutlinedObject	 base;

	GSList  *plots;
	unsigned int full_cardinality, visible_cardinality;
	gboolean cardinality_valid;

	/* use a simple grid layout to position charts within graph */
	unsigned int x_pos, y_pos;
	unsigned int cols, rows; 		/* if == 0, chart is not positionned */
	/* actual chart position in graph grid after layout validation */
	unsigned int x_pos_actual, y_pos_actual;

	GogObject *grid;
	GSList  *axes;
	GogAxisSet axis_set;

	GogViewAllocation plot_area;
	gboolean	  is_plot_area_manual;
};
typedef GogOutlinedObjectClass GogChartClass;

#define GOG_CHART_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST ((k), GOG_TYPE_CHART, GogChartClass))
#define GOG_IS_CHART_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GOG_TYPE_CHART))

/* protected */

G_END_DECLS

#endif /* GOG_CHART_IMPL_H */
