/*
 * gog-xyz-surface.h
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

#ifndef GOG_XYZ_SURFACE_H
#define GOG_XYZ_SURFACE_H

#include "gog-contour.h"
#include "gog-matrix.h"
#include "gog-surface.h"

G_BEGIN_DECLS

/*-----------------------------------------------------------------------------
 *
 * GogXYZContourPlot
 *
 *-----------------------------------------------------------------------------
 */
char const *missing_as_string (unsigned n);
unsigned missing_as_value (char const *name);


typedef struct {
	GogContourPlot base;
	GogDatasetElement grid[2];       /* for preset cols and rows */
	unsigned missing_as;
} GogXYZContourPlot;
typedef GogContourPlotClass GogXYZContourPlotClass;

#define GOG_TYPE_XYZ_CONTOUR_PLOT	(gog_xyz_contour_plot_get_type ())
#define GOG_XYZ_CONTOUR_PLOT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_XYZ_CONTOUR_PLOT, GogXYZContourPlot))
#define GOG_IS_XYZ_CONTOUR_PLOT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_XYZ_CONTOUR_PLOT))

GType gog_xyz_contour_plot_get_type (void);

void  gog_xyz_contour_plot_register_type   (GTypeModule *module);

/*-----------------------------------------------------------------------------
 *
 * GogXYZMatrixPlot
 *
 *-----------------------------------------------------------------------------
 */


typedef struct {
	GogMatrixPlot base;
	GogDatasetElement grid[2];       /* for preset cols and rows */
	unsigned missing_as;
} GogXYZMatrixPlot;
typedef GogMatrixPlotClass GogXYZMatrixPlotClass;

#define GOG_TYPE_XYZ_MATRIX_PLOT	(gog_xyz_matrix_plot_get_type ())
#define GOG_XYZ_MATRIX_PLOT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_XYZ_MATRIX_PLOT, GogXYZMatrixPlot))
#define GOG_IS_XYZ_MATRIX_PLOT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_XYZ_MATRIX_PLOT))

GType gog_xyz_matrix_plot_get_type (void);

void  gog_xyz_matrix_plot_register_type   (GTypeModule *module);

/*-----------------------------------------------------------------------------
 *
 * GogXYZSurfacePlot
 *
 *-----------------------------------------------------------------------------
 */

typedef struct {
	GogSurfacePlot base;
	GogDatasetElement grid[2];       /* for preset cols and rows */
	unsigned missing_as;
} GogXYZSurfacePlot;
typedef GogSurfacePlotClass GogXYZSurfacePlotClass;

#define GOG_TYPE_XYZ_SURFACE_PLOT	(gog_xyz_surface_plot_get_type ())
#define GOG_XYZ_SURFACE_PLOT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_XYZ_SURFACE_PLOT, GogXYZSurfacePlot))
#define GOG_IS_XYZ_SURFACE_PLOT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_XYZ_SURFACE_PLOT))

GType gog_xyz_surface_plot_get_type (void);

void  gog_xyz_surface_plot_register_type   (GTypeModule *module);

/*-----------------------------------------------------------------------------
 *
 * GogXYContourPlot
 *
 *-----------------------------------------------------------------------------
 */


typedef struct {
	GogContourPlot base;
	GogDatasetElement grid[2];       /* for preset cols and rows */
	gboolean as_density;
} GogXYContourPlot;
typedef GogContourPlotClass GogXYContourPlotClass;

#define GOG_TYPE_XY_CONTOUR_PLOT	(gog_xy_contour_plot_get_type ())
#define GOG_XY_CONTOUR_PLOT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_XY_CONTOUR_PLOT, GogXYContourPlot))
#define GOG_IS_XY_CONTOUR_PLOT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_XY_CONTOUR_PLOT))

GType gog_xy_contour_plot_get_type (void);
void  gog_xy_contour_plot_register_type   (GTypeModule *module);

/*-----------------------------------------------------------------------------
 *
 * GogXYMatrixPlot
 *
 *-----------------------------------------------------------------------------
 */

typedef struct {
	GogMatrixPlot base;
	GogDatasetElement grid[2];       /* for preset cols and rows */
	gboolean as_density;
} GogXYMatrixPlot;
typedef GogMatrixPlotClass GogXYMatrixPlotClass;

#define GOG_TYPE_XY_MATRIX_PLOT	(gog_xy_matrix_plot_get_type ())
#define GOG_XY_MATRIX_PLOT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_XY_MATRIX_PLOT, GogXYMatrixPlot))
#define GOG_IS_XY_MATRIX_PLOT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_XY_MATRIX_PLOT))

GType gog_xy_matrix_plot_get_type (void);
void  gog_xy_matrix_plot_register_type   (GTypeModule *module);

/*-----------------------------------------------------------------------------
 *
 * GogXYSurfacePlot
 *
 *-----------------------------------------------------------------------------
 */

typedef struct {
	GogSurfacePlot base;
	GogDatasetElement grid[2];       /* for preset cols and rows */
	gboolean as_density;
} GogXYSurfacePlot;
typedef GogSurfacePlotClass GogXYSurfacePlotClass;

#define GOG_TYPE_XY_SURFACE_PLOT	(gog_xy_surface_plot_get_type ())
#define GOG_XY_SURFACE_PLOT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_XY_SURFACE_PLOT, GogXYSurfacePlot))
#define GOG_IS_XY_SURFACE_PLOT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_XY_SURFACE_PLOT))

GType gog_xy_surface_plot_get_type (void);
void  gog_xy_surface_plot_register_type   (GTypeModule *module);

G_END_DECLS

#endif /* GOG_XYZ_SURFACE_H */
