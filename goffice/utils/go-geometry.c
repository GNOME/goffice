/*
 * go-geometry.c: A collection of geometry related functions.
 *
 * Copyright (C) 2005 Emmanuel Pacaud <emmanuel.pacaud@univ-poitiers.fr>
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

#include <goffice/goffice-config.h>

#include <goffice/math/go-math.h>
#include <goffice/utils/go-geometry.h>

#include <glib/gi18n-lib.h>

#define dist(x0,y0,x1,y1) hypot((x0) - (x1),(y0) - (y1))

/**
 * go_geometry_cartesian_to_polar:
 * @x: cartesian coordinate
 * @y: cartesian coordinate
 * @rho: (out): polar coordinate
 * @theta: (out): polar coordinate
 *
 * Converts cartesion coordinates to polar coordinates.
 **/
void
go_geometry_cartesian_to_polar (double x, double y, double *rho, double *theta)
{
	*rho = hypot (x, y);
	*theta = atan2 (y, x);
}

/**
 * go_geometry_point_to_segment:
 * @xp: point coordinate
 * @yp: point coordinate
 * @xs: segment start coordinate
 * @ys: segment start coordinate
 * @w: extent of segment
 * @h: extent of segment
 *
 * Returns: the distance between a point and a segment.
 **/
double
go_geometry_point_to_segment (double xp, double yp, double xs, double ys, double w, double h)
{
	double c1, c2, b;

	c1 = w * (xp - xs) + h * (yp - ys);
	if (c1 <= 0.0)
		return dist (xp, yp, xs, ys);

	c2 = w * w + h * h;
	if (c2 <= c1)
		return dist (xp, yp, xs + w, ys + h);

	b = c1 / c2;
	return dist (xp, yp, xs + b * w, ys + b * h);
}

/**
 * go_geometry_AABR_add:
 * @aabr0: (inout): a #GOGeometryAABR
 * @aabr1: a #GOGeometryAABR
 *
 * Computes the Axis Aligned Bounding Rectangle of aabr0 and aabr1,
 * and stores result in aabr0.
 **/
void
go_geometry_AABR_add (GOGeometryAABR *aabr0, GOGeometryAABR const *aabr1)
{
	double min, max;

	min = MIN (aabr0->x, aabr1->x);
	max = MAX (aabr0->x + aabr0->w, aabr1->x + aabr1->w);
	aabr0->x = min;
	aabr0->w = max - min;

	min = MIN (aabr0->y, aabr1->y);
	max = MAX (aabr0->y + aabr0->h, aabr1->y + aabr1->h);
	aabr0->y = min;
	aabr0->h = max - min;
}

/**
 * go_geometry_OBR_to_AABR:
 * @obr: a #GOGeometryOBR
 * @aabr: (out): a #GOGeometryAABR
 *
 * Stores Axis Aligned Bounding Rectangle of @obr in @aabr.
 **/
void
go_geometry_OBR_to_AABR (GOGeometryOBR const *obr, GOGeometryAABR *aabr)
{
	double cos_alpha = cos (obr->alpha);
	double sin_alpha = sin (obr->alpha);

	aabr->w = fabs (obr->w * cos_alpha) + fabs (obr->h * sin_alpha);
	aabr->h = fabs (obr->w * sin_alpha) + fabs (obr->h * cos_alpha);
	aabr->x = obr->x - aabr->w / 2.0 ;
	aabr->y = obr->y - aabr->h / 2.0 ;
}

/**
 * go_geometry_test_OBR_overlap:
 * @obr0: a #GOGeometryOBR
 * @obr1: a #GOGeometryOBR
 *
 * Overlap test of Oriented Bounding Rectangles by the separating axis method.
 *
 * Return value: %TRUE if OOBRs overlap
 **/
gboolean
go_geometry_test_OBR_overlap (GOGeometryOBR const *obr0, GOGeometryOBR const *obr1)
{
        double TL, pa, pb;
        double cos_delta, sin_delta;
	double a00, a01, a10, a11;
	double alpha, delta;

        cos_delta = fabs (cos (obr1->alpha - obr0->alpha));
        sin_delta = fabs (sin (obr1->alpha - obr0->alpha));

	go_geometry_cartesian_to_polar (obr1->x - obr0->x,
					obr1->y - obr0->y,
					&delta, &alpha);

	a00 = fabs (obr0->w / 2.0);
	a01 = fabs (obr0->h / 2.0);
	a10 = fabs (obr1->w / 2.0);
	a11 = fabs (obr1->h / 2.0);

        /* Separating axis parallel to obr0->w */
        TL = fabs (delta * cos (alpha - obr0->alpha));
        pa = a00;
        pb = a10 * cos_delta + a11 * sin_delta;
        if (TL > pa + pb) return FALSE;

        /* Separating axis parallel to obr->h */
        TL = fabs (delta * sin (alpha - obr0->alpha));
        pa = a01;
        pb = a10 * sin_delta + a11 * cos_delta;
        if (TL > pa + pb) return FALSE;

        /* Separating axis parallel to obr1->w */
        TL = fabs (delta * cos (obr1->alpha - alpha));
        pa = a00 * cos_delta + a01 * sin_delta;
        pb = a10;
        if (TL > pa + pb) return FALSE;

        /* Separating axis parallel to obr1->h */
        TL = fabs (delta * sin (obr1->alpha - alpha));
        pa = a00 * sin_delta + a01 * cos_delta;
        pb = a11;
        if (TL > pa + pb) return FALSE;

        return TRUE;
}

/**
 * go_geometry_get_rotation_type:
 * @alpha: angle in radians
 *
 * Calculates rotation type for handling of special angles (alpha = n * pi / 2)
 *
 * Returns: a #GOGeometryRotationType
 **/
GOGeometryRotationType
go_geometry_get_rotation_type (double alpha)
{
	unsigned index;

	if (alpha < 0 || alpha > 2 * M_PI)
		alpha = alpha - 2 * M_PI * floor (alpha / (2 * M_PI));

	if (fmod(alpha + GO_GEOMETRY_ANGLE_TOLERANCE, M_PI / 2.0) > 2 * GO_GEOMETRY_ANGLE_TOLERANCE)
		return GO_ROTATE_FREE;
	index = go_rint (2.0 * alpha / M_PI);
	return index < GO_ROTATE_FREE ? index : GO_ROTATE_NONE;
}

/**
 * go_geometry_calc_label_anchor:
 * @obr: bounding rectangle of label
 * @alpha: angle of axis
 *
 * Returns: computed label anchor, to be used by go_geometry_calc_label_position.
 **/
GOGeometrySide
go_geometry_calc_label_anchor (GOGeometryOBR *obr, double alpha)
{
	double dt, ds;

	dt = fabs (obr->w * sin (obr->alpha - alpha) / 2.0);
	ds = fabs (obr->h * cos (obr->alpha - alpha) / 2.0);

	return dt < ds ? GO_SIDE_TOP_BOTTOM : GO_SIDE_LEFT_RIGHT;
}

/**
 * go_geometry_calc_label_position:
 * @obr: bounding rectangle of label
 * @alpha: angle of axis
 * @offset: minimum distance between label and axis
 * @side: side of label with respect to axis
 * @anchor: where to anchor the label
 *
 * Convenience routine that computes position of a label relative to an axis.
 *
 * Returns: the computed anchor if @anchor == GO_SIDE_AUTO, or @anchor value.
 **/
GOGeometrySide
go_geometry_calc_label_position (GOGeometryOBR *obr, double alpha, double offset,
				 GOGeometrySide side, GOGeometrySide anchor)
{
	double dt, ds;
	double sinus, cosinus;

	if (side == GO_SIDE_RIGHT)
		alpha += M_PI;

       	sinus = sin (obr->alpha - alpha);
	cosinus = cos (obr->alpha - alpha);

	dt = fabs (obr->w * sinus / 2.0);
	ds = fabs (obr->h * cosinus / 2.0);

	if (anchor == GO_SIDE_AUTO)
		anchor = dt < ds ? GO_SIDE_TOP_BOTTOM : GO_SIDE_LEFT_RIGHT;

	if ((anchor & GO_SIDE_TOP_BOTTOM) != 0) {
		offset += dt;
		obr->x =  obr->h * sin (obr->alpha) / 2.0;
		obr->y = -obr->h * cos (obr->alpha) / 2.0;
		if (cosinus < 0.0) {
			obr->x = -obr->x;
			obr->y = -obr->y;
		}
	} else {
		offset += ds;
		obr->x = -obr->w * cos (obr->alpha) / 2.0;
		obr->y = -obr->w * sin (obr->alpha) / 2.0;
		if (sinus < 0.0) {
			obr->x = -obr->x;
			obr->y = -obr->y;
		}
	}
	obr->x += offset *  sin (alpha);
	obr->y += offset * -cos (alpha);

	return anchor;
}

/**
 * go_direction_is_horizontal:
 * @d: #GODirection
 *
 * Returns: %TRUE for GO_DIRECTION_LEFT and GO_DIRECTION_RIGHT.
 **/
gboolean
go_direction_is_horizontal (GODirection d)
{
	return (d & 2) != 0;
}

/**
 * go_direction_is_forward:
 * @d: #GODirection
 *
 * Returns: %TRUE for GO_DIRECTION_DOWN or GO_DIRECTION_RIGHT.
 **/
gboolean
go_direction_is_forward (GODirection d)
{
	return (d & 1) != 0;
}

static GEnumValue const directions[] = {
	{ GO_DIRECTION_NONE,	"GO_DIRECTION_NONE",	N_("none") },
	{ GO_DIRECTION_DOWN,	"GO_DIRECTION_DOWN",	N_("down") },
	{ GO_DIRECTION_UP,	"GO_DIRECTION_UP",	N_("up") },
	{ GO_DIRECTION_RIGHT,	"GO_DIRECTION_RIGHT",	N_("right") },
	{ GO_DIRECTION_LEFT,	"GO_DIRECTION_LEFT",	N_("left") },
	{ 0, NULL, NULL }
};

GType
go_direction_get_type (void)
{
	static GType etype = 0;
	if (etype == 0) {
		etype = g_enum_register_static (g_intern_static_string ("GODirection"), directions);
	}
	return etype;
}

char const *
go_direction_get_name (GODirection d)
{
	unsigned i;
	g_return_val_if_fail (d < G_N_ELEMENTS (directions), NULL);
	for (i = 0; i < G_N_ELEMENTS (directions); i++)
		if (d == (GODirection) directions[i].value)
			return _(directions[i].value_nick);
	return NULL;
}
