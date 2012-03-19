/*
 * gog-chart.h :
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

#ifndef GOG_CHART_H
#define GOG_CHART_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

GogAxisSet gog_axis_set_from_str (char const *str);

#define GOG_TYPE_CHART	(gog_chart_get_type ())
#define GOG_CHART(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_CHART, GogChart))
#define GOG_IS_CHART(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_CHART))

GType gog_chart_get_type (void);

gboolean  gog_chart_get_position  (GogChart const *chart, unsigned *x, unsigned *y,
				   unsigned *cols, unsigned *rows);
void	  gog_chart_set_position  (GogChart *chart, unsigned int x, unsigned int y,
				   unsigned int cols, unsigned int rows);
gboolean  gog_chart_get_plot_area (GogChart *chart, GogViewAllocation *plot_area);
void 	  gog_chart_set_plot_area (GogChart *chart, GogViewAllocation const *plot_area);

void	 gog_chart_request_cardinality_update (GogChart *chart);
void	 gog_chart_get_cardinality (GogChart *chart,
				    unsigned *full, unsigned *visible);
void	 gog_chart_foreach_elem	   (GogChart *chart, gboolean only_visible,
				    GogEnumFunc handler, gpointer data);
GSList	*gog_chart_get_plots	   (GogChart const *chart);

GogAxisSet gog_chart_get_axis_set    (GogChart const *chart);
gboolean gog_chart_axis_set_is_valid (GogChart const *chart, GogAxisSet type);
gboolean gog_chart_axis_set_assign   (GogChart *chart, GogAxisSet type);
GSList	*gog_chart_get_axes	     (GogChart const *chart, GogAxisType target);

GogGrid *gog_chart_get_grid	     (GogChart const *chart);

/* View utils */
GogViewAllocation const *gog_chart_view_get_plot_area (GogView const *view);

gboolean gog_chart_is_3d 	(GogChart const *chart);

G_END_DECLS

#endif /* GOG_CHART_H */
