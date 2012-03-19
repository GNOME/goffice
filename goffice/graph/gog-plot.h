/*
 * gog-plot.h :
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
#ifndef GOG_PLOT_H
#define GOG_PLOT_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

typedef struct {
	struct _GogPlotBound {
		double minima, maxima;
	} val, logical;
	gboolean is_discrete;
	gboolean center_on_ticks;
	GOFormat *fmt;

	const GODateConventions *date_conv;
} GogPlotBoundInfo;

#define GOG_TYPE_PLOT	(gog_plot_get_type ())
#define GOG_PLOT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_PLOT, GogPlot))
#define GOG_IS_PLOT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_PLOT))

GType	  gog_plot_get_type (void);
GogPlot  *gog_plot_new_by_type	(GogPlotType const *type);
GogPlot  *gog_plot_new_by_name	(char const *id);

void	  gog_plot_request_cardinality_update 	(GogPlot *plot);
void 	  gog_plot_update_cardinality 		(GogPlot *plot, int index_num);
void	  gog_plot_get_cardinality 		(GogPlot *plot,
						 unsigned *full, unsigned *visible);
void      gog_plot_foreach_elem    (GogPlot *plot, gboolean only_visible,
				    GogEnumFunc handler, gpointer data);
GSList const *gog_plot_get_series  (GogPlot const *plot);
GOData	 *gog_plot_get_axis_bounds (GogPlot *plot, GogAxisType axis,
				    GogPlotBoundInfo *bounds);

gboolean  gog_plot_supports_vary_style_by_element (GogPlot const *plot);

GogSeries	  *gog_plot_new_series	  (GogPlot *plot);
GogPlotDesc const *gog_plot_description	  (GogPlot const *plot);

GogAxisSet gog_plot_axis_set_pref	(GogPlot const *plot);
gboolean   gog_plot_axis_set_is_valid	(GogPlot const *plot, GogAxisSet type);
gboolean   gog_plot_axis_set_assign	(GogPlot *plot, GogAxisSet type);
void	   gog_plot_axis_clear		(GogPlot *plot, GogAxisSet filter);
GogAxis	  *gog_plot_get_axis		(GogPlot const *plot, GogAxisType type);
void	   gog_plot_set_axis		(GogPlot *plot, GogAxis *axis);

void 	   gog_plot_update_3d 		(GogPlot *plot);

void	   gog_plot_guru_helper		(GogPlot *plot);
void	   gog_plot_clear_series	(GogPlot *plot);

int        gog_plot_view_get_data_at_point (GogPlotView *view,
					    double x, double y, GogSeries **series);
double	   gog_plot_get_percent_value   (GogPlot *plot, unsigned series, unsigned index);

G_END_DECLS

#endif /* GOG_PLOT_H */
