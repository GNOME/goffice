/*
 * smoothing/gog-moving-avg.h :
 *
 * Copyright (C) 2006 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GOG_MOVING_AVG_H
#define GOG_MOVING_AVG_H

#include <goffice/graph/gog-smoothed-curve.h>

G_BEGIN_DECLS

typedef struct {
	GogSmoothedCurve base;
	int span;
	gboolean xavg; /* use x average instead of last value */
} GogMovingAvg;
typedef GogSmoothedCurveClass GogMovingAvgClass;

#define GOG_TYPE_MOVING_AVG	(gog_moving_avg_get_type ())
#define GOG_MOVING_AVG(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_MOVING_AVG, GogMovingAvg))
#define GOG_IS_MOVING_AVG(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_MOVING_AVG))

GType gog_moving_avg_get_type (void);
void gog_moving_avg_register_type (GTypeModule *module);

G_END_DECLS

#endif	/* GOG_MOVING_AVG_H */
