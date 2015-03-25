/*
 * gog-axis.h :
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

#ifndef GOG_AXIS_H
#define GOG_AXIS_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

typedef enum {
	GOG_AXIS_POLAR_UNIT_DEGREES,
	GOG_AXIS_POLAR_UNIT_RADIANS,
	GOG_AXIS_POLAR_UNIT_GRADS,
	GOG_AXIS_POLAR_UNIT_MAX
} GogAxisPolarUnit;

typedef enum {
	GOG_AXIS_ELEM_MIN = 0,
	GOG_AXIS_ELEM_MAX,
	GOG_AXIS_ELEM_MAJOR_TICK,
	GOG_AXIS_ELEM_MINOR_TICK,
	GOG_AXIS_ELEM_CROSS_POINT,
	GOG_AXIS_ELEM_MAX_ENTRY
} GogAxisElemType;

typedef enum {
	GOG_AXIS_METRICS_INVALID = -1,
	GOG_AXIS_METRICS_DEFAULT,
	GOG_AXIS_METRICS_ABSOLUTE,
	GOG_AXIS_METRICS_RELATIVE,
	GOG_AXIS_METRICS_RELATIVE_TICKS,
	GOG_AXIS_METRICS_MAX
} GogAxisMetrics;

typedef struct _GogAxisMap GogAxisMap;

GType		  gog_axis_map_get_type (void);
GogAxisMap*   gog_axis_map_new	 	  (GogAxis *axis, double offset, double length);
double	      gog_axis_map 		  (GogAxisMap *map, double value);
double	      gog_axis_map_to_view	  (GogAxisMap *map, double value);
double	      gog_axis_map_derivative_to_view (GogAxisMap *map, double value);
double	      gog_axis_map_from_view	  (GogAxisMap *map, double value);
gboolean      gog_axis_map_finite	  (GogAxisMap *map, double value);
double	      gog_axis_map_get_baseline	  (GogAxisMap *map);
void 	      gog_axis_map_get_extents 	  (GogAxisMap *map, double *start, double *stop);
void 	      gog_axis_map_get_real_extents (GogAxisMap *map, double *start, double *stop);
void	      gog_axis_map_get_bounds 	  (GogAxisMap *map, double *minimum, double *maximum);
void 	      gog_axis_map_get_real_bounds (GogAxisMap *map, double *minimum, double *maximum);
void 	      gog_axis_map_free		  (GogAxisMap *map);
gboolean      gog_axis_map_is_valid 	  (GogAxisMap *map);
gboolean      gog_axis_map_is_inverted 	  (GogAxisMap *map);
gboolean      gog_axis_map_is_discrete 	  (GogAxisMap *map);

/*****************************************************************************/

#define GOG_TYPE_AXIS	(gog_axis_get_type ())
#define GOG_AXIS(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_AXIS, GogAxis))
#define GOG_IS_AXIS(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_AXIS))

GType gog_axis_get_type (void);

GogAxisType   gog_axis_get_atype 	  (GogAxis const *axis);
gboolean      gog_axis_is_center_on_ticks (GogAxis const *axis);
gboolean      gog_axis_is_discrete        (GogAxis const *axis);
gboolean      gog_axis_is_inverted	  (GogAxis const *axis);
gboolean      gog_axis_get_bounds 	  (GogAxis const *axis,
					   double *minima, double *maxima);
void	      gog_axis_set_bounds 	  (GogAxis *axis,
					   double minimum, double maximum);
void 	      gog_axis_set_extents 	  (GogAxis *axis, double start, double stop);
void          gog_axis_get_effective_span (GogAxis const *axis,
					   double *start, double *end);
GOFormat     *gog_axis_get_format	  (GogAxis const *axis);
GOFormat     *gog_axis_get_effective_format (GogAxis const *axis);
gboolean      gog_axis_set_format	  (GogAxis *axis, GOFormat *fmt);
const GODateConventions *gog_axis_get_date_conv (GogAxis const *axis);
unsigned      gog_axis_get_ticks 	  (GogAxis *axis, GogAxisTick **ticks);
GOData	     *gog_axis_get_labels	  (GogAxis const *axis,
					   GogPlot **plot_that_labeled_axis);

double 	      gog_axis_get_entry 	  (GogAxis const *axis, GogAxisElemType i,
					   gboolean *user_defined);

void 	      gog_axis_add_contributor	  (GogAxis *axis, GogObject *contrib);
void 	      gog_axis_del_contributor	  (GogAxis *axis, GogObject *contrib);
GSList const *gog_axis_contributors	  (GogAxis *axis);
void	      gog_axis_clear_contributors (GogAxis *axis);
void	      gog_axis_bound_changed	  (GogAxis *axis, GogObject *contrib);

void          gog_axis_data_get_bounds    (GogAxis *axis, GOData *data, double *minimum, double *maximum);

gboolean      gog_axis_is_zero_important  (GogAxis *axis);

GogGridLine  *gog_axis_get_grid_line 	  (GogAxis *axis, gboolean major);

void	      		gog_axis_set_polar_unit		(GogAxis *axis, GogAxisPolarUnit unit);
GogAxisPolarUnit	gog_axis_get_polar_unit		(GogAxis *axis);
double 			gog_axis_get_polar_perimeter 	(GogAxis *axis);
double 			gog_axis_get_circular_rotation 	(GogAxis *axis);
GogAxisColorMap const *gog_axis_get_color_map	(GogAxis *axis);
GogColorScale *gog_axis_get_color_scale (GogAxis *axis);
GogAxisMetrics gog_axis_get_metrics (GogAxis const *axis);
GogAxis	      *gog_axis_get_ref_axis (GogAxis const *axis);
double		   gog_axis_get_major_ticks_distance (GogAxis const *axis);
/* private */
void _gog_axis_set_color_scale (GogAxis *axis, GogColorScale *scale);

G_END_DECLS

#endif /* GOG_AXIS_H */
