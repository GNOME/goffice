/*
 * go-distribution.h
 *
 * Copyright (C) 2007 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GO_DISTRIBUTION_H
#define GO_DISTRIBUTION_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef enum {
	GO_DISTRIBUTION_INVALID = -1,
	GO_DISTRIBUTION_NORMAL,
	GO_DISTRIBUTION_UNIFORM,
	GO_DISTRIBUTION_CAUCHY,
	GO_DISTRIBUTION_WEIBULL,
	GO_DISTRIBUTION_LOGNORMAL,
	GO_DISTRIBUTION_MAX
} GODistributionType;

char const *go_distribution_type_to_string (GODistributionType type);
GODistributionType go_distribution_type_from_string (char const *name);

typedef struct _GODistribution GODistribution;

#define GO_TYPE_DISTRIBUTION	  	(go_distribution_get_type ())
#define GO_DISTRIBUTION(o)		(G_TYPE_CHECK_INSTANCE_CAST((o), GO_TYPE_DISTRIBUTION, GODistribution))
#define GO_IS_DISTRIBUTION(o)		(G_TYPE_CHECK_INSTANCE_TYPE((o), GO_TYPE_DISTRIBUTION))

GType go_distribution_get_type (void);

GODistribution *go_distribution_new (GODistributionType type);
GODistributionType go_distribution_get_distribution_type (GODistribution *dist);
char const *go_distribution_get_distribution_name (GODistribution *dist);

double go_distribution_get_density (GODistribution *dist, double x);
double go_distribution_get_cumulative (GODistribution *dist, double x);
double go_distribution_get_ppf (GODistribution *dist, double x);
double go_distribution_get_hazard (GODistribution *dist, double x);
double go_distribution_get_cumulative_hazard (GODistribution *dist, double x);
double go_distribution_get_survival (GODistribution *dist, double x);
double go_distribution_get_inverse_survival (GODistribution *dist, double x);

// ----------------------------------------------------------------------------

#ifdef GOFFICE_WITH_LONG_DOUBLE
long double go_distribution_get_densityl (GODistribution *dist, long double x);
long double go_distribution_get_cumulativel (GODistribution *dist, long double x);
long double go_distribution_get_ppfl (GODistribution *dist, long double x);
long double go_distribution_get_hazardl (GODistribution *dist, long double x);
long double go_distribution_get_cumulative_hazardl (GODistribution *dist, long double x);
long double go_distribution_get_survivall (GODistribution *dist, long double x);
long double go_distribution_get_inverse_survivall (GODistribution *dist, long double x);
#endif

// ----------------------------------------------------------------------------

#ifdef GOFFICE_WITH_DECIMAL64
_Decimal64 go_distribution_get_densityD (GODistribution *dist, _Decimal64 x);
_Decimal64 go_distribution_get_cumulativeD (GODistribution *dist, _Decimal64 x);
_Decimal64 go_distribution_get_ppfD (GODistribution *dist, _Decimal64 x);
_Decimal64 go_distribution_get_hazardD (GODistribution *dist, _Decimal64 x);
_Decimal64 go_distribution_get_cumulative_hazardD (GODistribution *dist, _Decimal64 x);
_Decimal64 go_distribution_get_survivalD (GODistribution *dist, _Decimal64 x);
_Decimal64 go_distribution_get_inverse_survivalD (GODistribution *dist, _Decimal64 x);
#endif

// ----------------------------------------------------------------------------

void go_distribution_scale (GODistribution *dist, double location, double scale);

void _go_distributions_init (void);

G_END_DECLS

#endif
