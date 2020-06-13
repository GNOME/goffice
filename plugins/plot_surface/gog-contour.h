/*
 * gog-contour.h
 *
 * Copyright (C) 2004-2007 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GOG_CONTOUR_H
#define GOG_CONTOUR_H

#include "gog-xyz.h"

G_BEGIN_DECLS

/*-----------------------------------------------------------------------------
 *
 * GogContourPlot
 *
 *-----------------------------------------------------------------------------
 */

typedef struct {
	GogXYZPlot base;
	unsigned max_colors;
} GogContourPlot;
typedef GogXYZPlotClass GogContourPlotClass;

#define GOG_TYPE_CONTOUR_PLOT	(gog_contour_plot_get_type ())
#define GOG_CONTOUR_PLOT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_CONTOUR_PLOT, GogContourPlot))
#define GOG_IS_CONTOUR_PLOT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_CONTOUR_PLOT))

GType gog_contour_plot_get_type (void);
void  gog_contour_plot_register_type   (GTypeModule *module);

void  gog_contour_view_register_type   (GTypeModule *module);

G_END_DECLS

#endif /* GOG_CONTOUR_H */
