/*
 * go-line.h
 *
 * Copyright (C) 2003-2004 Emmanuel Pacaud (jody@gnome.org)
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

#ifndef GOG_LINE_H
#define GOG_LINE_H

#include "gog-1.5d.h"

G_BEGIN_DECLS

typedef struct _GogLinePlot	GogLinePlot;
typedef GogPlot1_5dClass	GogLinePlotClass;

#define GOG_TYPE_LINE_PLOT	(gog_line_plot_get_type ())
#define GOG_LINE_PLOT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_LINE_PLOT, GogLinePlot))
#define GOG_IS_PLOT_LINE(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_LINE_PLOT))

GType gog_line_plot_get_type (void);
void  gog_line_plot_register_type (GTypeModule *module);

/*************************************************************************/

#define GOG_TYPE_AREA_PLOT	(gog_area_plot_get_type ())
#define GOG_AREA_PLOT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_AREA_PLOT, GogAreaPlot))
#define GOG_IS_PLOT_AREA(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_AREA_PLOT))

typedef GogLinePlot		GogAreaPlot;
typedef GogLinePlotClass	GogAreaPlotClass;

GType gog_area_plot_get_type (void);
void  gog_area_plot_register_type (GTypeModule *module);

void  gog_area_series_register_type (GTypeModule *module);
void  gog_line_series_register_type (GTypeModule *module);
void  gog_line_series_view_register_type (GTypeModule *module);
void  gog_line_series_element_register_type (GTypeModule *module);
void  gog_line_view_register_type   (GTypeModule *module);

G_END_DECLS

#endif /* GOG_LINE_H */
