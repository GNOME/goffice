/*
 * gog-smoothed-curve.h :
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

#ifndef GOG_SMOOTHED_CURVE_H
#define GOG_SMOOTHED_CURVE_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

struct  _GogSmoothedCurve {
	GogTrendLine	base;

	GogSeries 	  *series;
	double *x, *y;
	unsigned int nb;
	GogDatasetElement *name;
};

typedef struct {
	GogTrendLineClass base;
	int max_dim;

} GogSmoothedCurveClass;

#define GOG_TYPE_SMOOTHED_CURVE	(gog_smoothed_curve_get_type ())
#define GOG_SMOOTHED_CURVE(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_SMOOTHED_CURVE, GogSmoothedCurve))
#define GOG_IS_SMOOTHED_CURVE(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_SMOOTHED_CURVE))

GType gog_smoothed_curve_get_type (void);

G_END_DECLS

#endif /* GOG_SMOOTHED_CURVE_H */
