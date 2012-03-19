/*
 * gog-probability-plot.h
 *
 * Copyright (C) 2007 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GOG_PROBABILITY_PLOT_H
#define GOG_PROBABILITY_PLOT_H

#include <goffice/graph/gog-plot-impl.h>
#include <goffice/math/go-distribution.h>
#include <goffice/utils/go-format.h>

G_BEGIN_DECLS

typedef struct {
	GogPlot	base;

	GODistribution *dist;
	GODistributionType dist_type;
	struct {
		double minima, maxima;
		GOFormat *fmt;
	} x, y;
	struct {
		char *prop_name;
		GogDatasetElement *elem;
	} shape_params[2];
	gboolean data_as_yvals;
} GogProbabilityPlot;

#define GOG_TYPE_PROBABILITY_PLOT	(gog_probability_plot_get_type ())
#define GOG_PROBABILITY_PLOT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_PROBABILITY_PLOT, GogProbabilityPlot))
#define GOG_IS_PROBABILITY_PLOT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_PROBABILITY_PLOT))

GType gog_probability_plot_get_type (void);

G_END_DECLS

#endif
