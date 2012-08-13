/*
 * gog-reg-curve.h :
 *
 * Copyright (C) 2005 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GOG_REG_CURVE_H
#define GOG_REG_CURVE_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

typedef enum {
	GOG_REG_CURVE_DRAWING_BOUNDS_NONE,
	GOG_REG_CURVE_DRAWING_BOUNDS_ABSOLUTE,
	GOG_REG_CURVE_DRAWING_BOUNDS_RELATIVE
} GogRegCurveDrawingBounds;

struct  _GogRegCurve {
	GogTrendLine	base;

	gboolean  	   weighted;
	GogDatasetElement *bounds;       /* aliased to include the name as -1 */
	gboolean	   skip_invalid; /* do not take into account invalid data */
	int		   ninterp;	 /* how many points to use for display the curve as a vpath */
	double		  *a;		 /* calculated coefficients, must be allocated by derived class */
	double		   R2;		 /* squared regression coefficient */
	char		  *equation;
	GogRegCurveDrawingBounds drawing_bounds;
	gpointer	   priv;
};

typedef struct {
	GogTrendLineClass base;

	double 		(*get_value_at) (GogRegCurve *reg_curve, double x);
	char const * 	(*get_equation) (GogRegCurve *reg_curve);
	void 		(*populate_editor) (GogRegCurve *reg_curve, gpointer table);
	/*<private>*/
	void	        (*reserved1) (void);
	void	        (*reserved2) (void);
} GogRegCurveClass;

#define GOG_TYPE_REG_CURVE	(gog_reg_curve_get_type ())
#define GOG_REG_CURVE(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_REG_CURVE, GogRegCurve))
#define GOG_IS_REG_CURVE(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_REG_CURVE))

GType gog_reg_curve_get_type (void);

gchar const *gog_reg_curve_get_equation (GogRegCurve *reg_curve);
double       gog_reg_curve_get_R2       (GogRegCurve *reg_curve);
void         gog_reg_curve_get_bounds   (GogRegCurve *reg_curve, double *xmin, double *xmax);

G_END_DECLS

#endif /* GOG_REG_CURVE_H */
