/*
 * gog-surface.h
 *
 * Copyright (C) 2004-2005 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GOG_SURFACE_H
#define GOG_SURFACE_H

#include "gog-xyz.h"

G_BEGIN_DECLS

/*-----------------------------------------------------------------------------
 *
 * GogSurfacePlot
 *
 *-----------------------------------------------------------------------------
 */

typedef GogXYZPlot GogSurfacePlot;
typedef GogXYZPlotClass GogSurfacePlotClass;

#define GOG_TYPE_SURFACE_PLOT	(gog_surface_plot_get_type ())
#define GOG_SURFACE_PLOT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_SURFACE_PLOT, GogSurfacePlot))
#define GOG_IS_SURFACE_PLOT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_SURFACE_PLOT))

GType gog_surface_plot_get_type (void);
void  gog_surface_plot_register_type   (GTypeModule *module);

void  gog_surface_view_register_type   (GTypeModule *module);

G_END_DECLS

#endif /* GOG_SURFACE_H */
