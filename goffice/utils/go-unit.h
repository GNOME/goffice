/*
 * go-unit.h - Units support
 *
 * Copyright (C) 2014 Jean Brefort (jean.brefort@normalesup.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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

#ifndef GO_UNIT_H
#define GO_UNIT_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

typedef struct _GoUnit GoUnit;

typedef enum  {
	GO_UNIT_UNKNOWN = -1,
	GO_UNIT_METER,
	GO_UNIT_CENTIMETER,
	GO_UNIT_INCH,
	GO_UNIT_POINT,
	GO_UNIT_MAX
} GoUnitId;

GType go_unit_get_type (void);

char const *go_unit_get_symbol (GoUnit const *unit);
GoUnitId go_unit_get_id (GoUnit const *unit);
double go_unit_convert (GoUnit const *from, GoUnit const *to, double value);
GoUnit const *go_unit_get_from_symbol (char const *symbol);
GoUnit const *go_unit_get (GoUnitId id);
GoUnit const *go_unit_define (char const *symbol, char const *dim, double factor_to_SI);

/* private */
void _go_unit_init (void);
void _go_unit_shutdown (void);

G_END_DECLS

#endif /* GO_UNIT_H */
