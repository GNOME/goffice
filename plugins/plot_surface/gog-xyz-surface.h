/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-xyz-surface.h
 *
 * Copyright (C) 2004-2005 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GOG_XYZ_SURFACE_H
#define GOG_XYZ_SURFACE_H

#include "gog-contour.h"
#include "gog-surface.h"

G_BEGIN_DECLS

/*-----------------------------------------------------------------------------
 *
 * GogXYZContourPlot
 *
 *-----------------------------------------------------------------------------
 */


typedef struct {
	GogContourPlot base;
	GogDatasetElement grid[2];       /* for preset cols and rows */
} GogXYZContourPlot;
typedef GogContourPlotClass GogXYZContourPlotClass;

#define GOG_TYPE_XYZ_CONTOUR_PLOT	(gog_xyz_contour_plot_get_type ())
#define GOG_XYZ_CONTOUR_PLOT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_XYZ_CONTOUR_PLOT, GogXYZContourPlot))
#define GOG_IS_XYZ_CONTOUR_PLOT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_XYZ_CONTOUR_PLOT))

GType gog_xyz_contour_plot_get_type (void);

void  gog_xyz_contour_plot_register_type   (GTypeModule *module);

/*-----------------------------------------------------------------------------
 *
 * GogXYZSurfacePlot
 *
 *-----------------------------------------------------------------------------
 */

typedef struct {
	GogSurfacePlot base;
	GogDatasetElement grid[2];       /* for preset cols and rows */
} GogXYZSurfacePlot;
typedef GogSurfacePlotClass GogXYZSurfacePlotClass;

#define GOG_TYPE_XYZ_SURFACE_PLOT	(gog_xyz_surface_plot_get_type ())
#define GOG_XYZ_SURFACE_PLOT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_XYZ_SURFACE_PLOT, GogXYZSurfacePlot))
#define GOG_IS_XYZ_SURFACE_PLOT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_XYZ_SURFACE_PLOT))

GType gog_xyz_surface_plot_get_type (void);

void  gog_xyz_surface_plot_register_type   (GTypeModule *module);

G_END_DECLS

#endif /* GOG_XYZ_SURFACE_H */
