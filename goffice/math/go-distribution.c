/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-distribution.c
 *
 * Copyright (C) 2007-2008 Jean Brefort (jean.brefort@normalesup.org)
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

#include <goffice/goffice-config.h>
#include "go-distribution.h"
#include "go-math.h"
#include "go-R.h"
#include <goffice/utils/go-persist.h>
#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#ifndef DOUBLE

#define DOUBLE double
#define SUFFIX(_n) _n

#define DISTRIBUTION_FIRST_PATH
#ifdef GOFFICE_WITH_LONG_DOUBLE
#include "go-distribution.c"
#undef DISTRIBUTION_FIRST_PATH
#undef DOUBLE
#undef SUFFIX

#ifdef HAVE_SUNMATH_H
#include <sunmath.h>
#endif
#define DOUBLE long double
#define SUFFIX(_n) _n ## l
#endif
#define DISTRIBUTION_LAST_PATH

#endif

#ifdef DISTRIBUTION_FIRST_PATH

enum {
	DIST_PROP_0,
	DIST_PROP_LOCATION,
	DIST_PROP_SCALE
};

static struct
{
	GODistributionType type;
	char const *name;
} DistributionTypes [] = {
	{GO_DISTRIBUTION_NORMAL, N_("Normal")},
	{GO_DISTRIBUTION_UNIFORM, N_("Uniform")},
	{GO_DISTRIBUTION_CAUCHY, N_("Cauchy")},
	{GO_DISTRIBUTION_WEIBULL, N_("Weibull")},
	{GO_DISTRIBUTION_LOGNORMAL, N_("Lognormal")}
};

char const *go_distribution_type_to_string (GODistributionType type)
{
	return DistributionTypes[type].name;
}

GODistributionType go_distribution_type_from_string (char const *name)
{
	int i = -1;
	while (++i < GO_DISTRIBUTION_MAX)
		if (!strcmp (DistributionTypes[i].name, name))
			return i;
	return GO_DISTRIBUTION_INVALID;
}

#define GO_DISTRIBUTION_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GO_TYPE_DISTRIBUTION, GODistributionClass))

struct _GODistribution {
	GObject	base;

	double location, scale;
};

typedef struct {
	GObjectClass	base;
	GODistributionType dist_type;

	double (*get_density) (GODistribution *dist, double x);
	double (*get_cumulative) (GODistribution *dist, double x);
	double (*get_ppf) (GODistribution *dist, double x);

#ifdef GOFFICE_WITH_LONG_DOUBLE
	long double (*get_densityl) (GODistribution *dist, long double x);
	long double (*get_cumulativel) (GODistribution *dist, long double x);
	long double (*get_ppfl) (GODistribution *dist, long double x);
#endif

} GODistributionClass;

static void
go_distribution_set_property (GObject *obj, guint param_id,
			      GValue const *value, GParamSpec *pspec)
{
	GODistribution *dist = GO_DISTRIBUTION (obj);

	switch (param_id) {
	case DIST_PROP_LOCATION:
		dist->location = g_value_get_double (value);
		break;
	case DIST_PROP_SCALE:
		dist->scale = g_value_get_double (value);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
}

static void
go_distribution_get_property (GObject *obj, guint param_id,
			      GValue *value, GParamSpec *pspec)
{
	GODistribution *dist = GO_DISTRIBUTION (obj);

	switch (param_id) {
	case DIST_PROP_LOCATION:
		g_value_set_double (value, dist->location);
		break;
	case DIST_PROP_SCALE:
		g_value_set_double (value, dist->scale);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		break;
	}
}

static void
go_distribution_class_init (GObjectClass *klass)
{
	klass->set_property = go_distribution_set_property;
	klass->get_property = go_distribution_get_property;
	g_object_class_install_property (klass, DIST_PROP_LOCATION,
		g_param_spec_double ("location",
			_("Location"),
			_("Location"),
			-G_MAXDOUBLE,
			G_MAXDOUBLE,
			0.0,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (klass, DIST_PROP_SCALE,
		g_param_spec_double ("scale",
			_("Scale"),
			_("Scale"),
			G_MINDOUBLE,
			G_MAXDOUBLE,
			1.0,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
}

static void
go_distribution_init (GODistribution *dist)
{
	dist->location = 0.;
	dist->scale = 1.;
}

static void
go_distribution_persist_sax_save (GOPersist const *gp, GsfXMLOut *output)
{
	GODistribution const *dist = GO_DISTRIBUTION (gp);
	GParamSpec **props;
	int n;

	g_return_if_fail (dist);
	gsf_xml_out_add_cstr_unchecked (output, "type",
		G_OBJECT_TYPE_NAME (dist));

	if (dist->location != 0.)
		gsf_xml_out_add_float (output, "location", dist->location, -1);
	if (dist->scale != 1.)
		gsf_xml_out_add_float (output, "scale", dist->scale, -1);

	/* properties */
	props = g_object_class_list_properties (G_OBJECT_GET_CLASS (dist), &n);
	while (n-- > 0)
		if (props[n]->flags & GO_PARAM_PERSISTENT) {
			GType    prop_type = G_PARAM_SPEC_VALUE_TYPE (props[n]);
			GValue	 value = { 0 };

			g_value_init (&value, prop_type);
			g_object_get_property  (G_OBJECT (dist), props[n]->name, &value);

			/* No need to save default values */
			if (g_param_value_defaults (props[n], &value)) {
				g_value_unset (&value);
				continue;
			}

			switch (G_TYPE_FUNDAMENTAL (prop_type)) {
			case G_TYPE_CHAR:
			case G_TYPE_UCHAR:
			case G_TYPE_BOOLEAN:
			case G_TYPE_INT:
			case G_TYPE_UINT:
			case G_TYPE_LONG:
			case G_TYPE_ULONG:
			case G_TYPE_FLOAT:
			case G_TYPE_DOUBLE:
			case G_TYPE_ENUM:
			case G_TYPE_FLAGS: {
				GValue str = { 0 };
				g_value_init (&str, G_TYPE_STRING);
				g_value_transform (&value, &str);
				gsf_xml_out_add_cstr (output, props[n]->name, g_value_get_string (&str));
				g_value_unset (&str);
				break;
			}

			case G_TYPE_STRING: {
				char const *str = g_value_get_string (&value);
				if (str != NULL) {
					gsf_xml_out_add_cstr (output,  props[n]->name, str);
				}
				break;
			}

			default:
				g_warning ("I could not persist property \"%s\", since type \"%s\" is unhandled.",
					   g_param_spec_get_name (props[n]), g_type_name (G_TYPE_FUNDAMENTAL(prop_type)));
			}
			g_value_unset (&value);
		}
}

static void
go_distribution_persist_prep_sax (GOPersist *gp, GsfXMLIn *xin, xmlChar const **attrs)
{
	GODistribution *dist = GO_DISTRIBUTION (gp);
	while (*attrs) {
		if (!strcmp ((char const*) *attrs, "name") || !strcmp ((char const*) *attrs, "type"));
		else if (!strcmp ((char const*) *attrs, "location"))
			dist->location = g_strtod (attrs[1], NULL);
		else if (!strcmp ((char const*) *attrs, "scale"))
			dist->scale = g_strtod (attrs[1], NULL);
		else {
			GParamSpec *spec = g_object_class_find_property (G_OBJECT_GET_CLASS (dist), *attrs);
			if (spec == NULL)
				g_warning ("unknown property `%s' for class `%s'",
					   *attrs, G_OBJECT_TYPE_NAME (gp));
			else {
				GType prop_type = G_PARAM_SPEC_VALUE_TYPE (spec);
				switch G_TYPE_FUNDAMENTAL (prop_type) {
				case G_TYPE_DOUBLE: {
					GValue value = {0};
					g_value_init (&value, G_TYPE_DOUBLE);
					g_value_set_double (&value, g_strtod (attrs[1], NULL));
					g_object_set_property (G_OBJECT (dist), *attrs, &value);
					g_value_unset (&value);
					break;
				}
				default:
					g_critical ("Unsupported property type. Please report.");
					break;
				}
			}
		}
		attrs += 2;
	}
}

static void
go_distribution_persist_init (GOPersistClass *iface)
{
	iface->prep_sax = go_distribution_persist_prep_sax;
	iface->sax_save = go_distribution_persist_sax_save;
}

GSF_CLASS_FULL (GODistribution, go_distribution, NULL,
	NULL, go_distribution_class_init, NULL, go_distribution_init,
	G_TYPE_OBJECT, G_TYPE_FLAG_ABSTRACT,
	GSF_INTERFACE (go_distribution_persist_init, GO_TYPE_PERSIST))

void
go_distribution_scale (GODistribution *dist, double location, double scale)
{
	dist->location = location;
	dist->scale = scale;
}

#endif /* DISTRIBUTION_LAST_PATH */

DOUBLE
SUFFIX (go_distribution_get_density) (GODistribution *dist, DOUBLE x)
{
	GODistributionClass *go_dist_klass;

	g_return_val_if_fail (GO_DISTRIBUTION (dist), SUFFIX (go_nan));

	go_dist_klass = GO_DISTRIBUTION_GET_CLASS (dist);
	if (go_dist_klass->SUFFIX (get_density) != NULL)
		return go_dist_klass->SUFFIX (get_density) (dist, x);
	return SUFFIX (go_nan);
}

DOUBLE
SUFFIX (go_distribution_get_cumulative) (GODistribution *dist, DOUBLE x)
{
	GODistributionClass *go_dist_klass;

	g_return_val_if_fail (GO_DISTRIBUTION (dist), SUFFIX (go_nan));

	go_dist_klass = GO_DISTRIBUTION_GET_CLASS (dist);
	if (go_dist_klass->SUFFIX (get_cumulative) != NULL)
		return go_dist_klass->SUFFIX (get_cumulative) (dist, x);
	return SUFFIX (go_nan);
}

DOUBLE
SUFFIX (go_distribution_get_ppf) (GODistribution *dist, DOUBLE x)
{
	GODistributionClass *go_dist_klass;

	g_return_val_if_fail (GO_DISTRIBUTION (dist), SUFFIX (go_nan));

	go_dist_klass = GO_DISTRIBUTION_GET_CLASS (dist);
	if (go_dist_klass->SUFFIX (get_ppf) != NULL)
		return go_dist_klass->SUFFIX (get_ppf) (dist, x);
	return SUFFIX (go_nan);
}

DOUBLE
SUFFIX (go_distribution_get_hazard) (GODistribution *dist, DOUBLE x)
{
	GODistributionClass *go_dist_klass;

	g_return_val_if_fail (GO_DISTRIBUTION (dist), SUFFIX (go_nan));

	go_dist_klass = GO_DISTRIBUTION_GET_CLASS (dist);
	if (go_dist_klass->SUFFIX (get_density) != NULL &&
	    go_dist_klass->SUFFIX (get_cumulative) != NULL)
		return go_dist_klass->SUFFIX (get_density) (dist, x) /
			(1. - go_dist_klass->SUFFIX (get_cumulative) (dist, x));
	return SUFFIX (go_nan);
}

DOUBLE
SUFFIX (go_distribution_get_cumulative_hazard) (GODistribution *dist, DOUBLE x)
{
	GODistributionClass *go_dist_klass;

	g_return_val_if_fail (GO_DISTRIBUTION (dist), SUFFIX (go_nan));

	go_dist_klass = GO_DISTRIBUTION_GET_CLASS (dist);
	if (go_dist_klass->SUFFIX (get_cumulative) != NULL)
		return SUFFIX (log) (1. - go_dist_klass->SUFFIX (get_cumulative) (dist, x));
	return SUFFIX (go_nan);
}

DOUBLE
SUFFIX (go_distribution_get_survival) (GODistribution *dist, DOUBLE x)
{
	GODistributionClass *go_dist_klass;

	g_return_val_if_fail (GO_DISTRIBUTION (dist), SUFFIX (go_nan));

	go_dist_klass = GO_DISTRIBUTION_GET_CLASS (dist);
	if (go_dist_klass->SUFFIX (get_cumulative) != NULL)
		return 1. - go_dist_klass->SUFFIX (get_cumulative) (dist, x);
	return SUFFIX (go_nan);
}

DOUBLE
SUFFIX (go_distribution_get_inverse_survival) (GODistribution *dist, DOUBLE x)
{
	GODistributionClass *go_dist_klass;

	g_return_val_if_fail (GO_DISTRIBUTION (dist), SUFFIX (go_nan));

	go_dist_klass = GO_DISTRIBUTION_GET_CLASS (dist);
	if (go_dist_klass->SUFFIX (get_ppf) != NULL)
		return 1. - go_dist_klass->SUFFIX (get_ppf) (dist, x);
	return SUFFIX (go_nan);
}

/*****************************************************************************/
/*                        Normal distribution                              */
/*****************************************************************************/
#ifdef DISTRIBUTION_FIRST_PATH

typedef struct {
	GODistribution base;

} GODistNormal;

#define GO_TYPE_DIST_NORMAL	  	(go_dist_normal_get_type ())
#define GO_DIST_NORMAL(o)		(G_TYPE_CHECK_INSTANCE_CAST((o), GO_TYPE_DIST_NORMAL, GODistNormal))
#define GO_IS_DIST_NORMAL(o)		(G_TYPE_CHECK_INSTANCE_TYPE((o), GO_TYPE_DIST_NORMAL))

GType go_dist_normal_get_type (void);

typedef GODistributionClass GODistNormalClass;

#endif /* DISTRIBUTION_FIRST_PATH */

static DOUBLE
SUFFIX (go_normal_get_density) (GODistribution *dist, DOUBLE x)
{
	return SUFFIX (go_dnorm) (x, dist->location, dist->scale, FALSE);
}

static DOUBLE
SUFFIX (go_normal_get_cumulative) (GODistribution *dist, DOUBLE x)
{
	return SUFFIX (go_pnorm) (x, dist->location, dist->scale, TRUE, FALSE);
}

static DOUBLE
SUFFIX (go_normal_get_ppf) (GODistribution *dist, DOUBLE x)
{
	return SUFFIX (go_qnorm) (x, dist->location, dist->scale, TRUE, FALSE);
}

#ifdef DISTRIBUTION_LAST_PATH

static void
go_dist_normal_class_init (GObjectClass *klass)
{
	GODistributionClass *dist_klass = (GODistributionClass *) klass;
	dist_klass->dist_type = GO_DISTRIBUTION_NORMAL;
	dist_klass->get_density = go_normal_get_density;
	dist_klass->get_cumulative = go_normal_get_cumulative;
	dist_klass->get_ppf = go_normal_get_ppf;
#ifdef GOFFICE_WITH_LONG_DOUBLE
	dist_klass->get_densityl = go_normal_get_densityl;
	dist_klass->get_cumulativel = go_normal_get_cumulativel;
	dist_klass->get_ppfl = go_normal_get_ppfl;
#endif
}

static void
go_dist_normal_init (GODistribution *dist)
{
}

GSF_CLASS (GODistNormal, go_dist_normal,
	   go_dist_normal_class_init, go_dist_normal_init,
	   GO_TYPE_DISTRIBUTION)

#endif /* DISTRIBUTION_LAST_PATH */

/*****************************************************************************/
/*                        Uniform distribution                              */
/*****************************************************************************/
#ifdef DISTRIBUTION_FIRST_PATH

typedef struct {
	GODistribution base;

} GODistUniform;

#define GO_TYPE_DIST_UNIFORM	  	(go_dist_uniform_get_type ())
#define GO_DIST_UNIFORM(o)		(G_TYPE_CHECK_INSTANCE_CAST((o), GO_TYPE_DIST_UNIFORM, GODistUniform))
#define GO_IS_DIST_UNIFORM(o)		(G_TYPE_CHECK_INSTANCE_TYPE((o), GO_TYPE_DIST_UNIFORM))

GType go_dist_uniform_get_type (void);

typedef GODistributionClass GODistUniformClass;

#endif /* DISTRIBUTION_FIRST_PATH */

static DOUBLE
SUFFIX (go_uniform_get_density) (GODistribution *dist, DOUBLE x)
{
	x = (x - dist->location) / dist->scale;
	return (x >= 0. && x < 1.)? SUFFIX (1.) / dist->scale: SUFFIX (0.);
}

static DOUBLE
SUFFIX (go_uniform_get_cumulative) (GODistribution *dist, DOUBLE x)
{
	x = (x - dist->location) / dist->scale;
	if (x < 0.)
		return SUFFIX (0.);
	else if (x >= 1.)
		return SUFFIX (1.);
	else
		return x;
}

static DOUBLE
SUFFIX (go_uniform_get_ppf) (GODistribution *dist, DOUBLE x)
{
	return x * dist->scale + dist->location;
}

#ifdef DISTRIBUTION_LAST_PATH

static void
go_dist_uniform_class_init (GObjectClass *klass)
{
	GODistributionClass *dist_klass = (GODistributionClass *) klass;
	dist_klass->dist_type = GO_DISTRIBUTION_UNIFORM;
	dist_klass->get_density = go_uniform_get_density;
	dist_klass->get_cumulative = go_uniform_get_cumulative;
	dist_klass->get_ppf = go_uniform_get_ppf;
#ifdef GOFFICE_WITH_LONG_DOUBLE
	dist_klass->get_densityl = go_uniform_get_densityl;
	dist_klass->get_cumulativel = go_uniform_get_cumulativel;
	dist_klass->get_ppfl = go_uniform_get_ppfl;
#endif
}

static void
go_dist_uniform_init (GODistribution *dist)
{
}

GSF_CLASS (GODistUniform, go_dist_uniform,
	   go_dist_uniform_class_init, go_dist_uniform_init,
	   GO_TYPE_DISTRIBUTION)

#endif /* DISTRIBUTION_LAST_PATH */

/*****************************************************************************/
/*                        Cauchy distribution                              */
/*****************************************************************************/
#ifdef DISTRIBUTION_FIRST_PATH

typedef struct {
	GODistribution base;

} GODistCauchy;

#define GO_TYPE_DIST_CAUCHY	  	(go_dist_cauchy_get_type ())
#define GO_DIST_CAUCHY(o)		(G_TYPE_CHECK_INSTANCE_CAST((o), GO_TYPE_DIST_CAUCHY, GODistCauchy))
#define GO_IS_DIST_CAUCHY(o)		(G_TYPE_CHECK_INSTANCE_TYPE((o), GO_TYPE_DIST_CAUCHY))

GType go_dist_cauchy_get_type (void);

typedef GODistributionClass GODistCauchyClass;

#endif /* DISTRIBUTION_FIRST_PATH */

static DOUBLE
SUFFIX (go_cauchy_get_density) (GODistribution *dist, DOUBLE x)
{
	return SUFFIX (go_dcauchy) (x, dist->location, dist->scale, FALSE);
}

static DOUBLE
SUFFIX (go_cauchy_get_cumulative) (GODistribution *dist, DOUBLE x)
{
	return SUFFIX (go_pcauchy) (x, dist->location, dist->scale, TRUE, FALSE);
}

static DOUBLE
SUFFIX (go_cauchy_get_ppf) (GODistribution *dist, DOUBLE x)
{
	return SUFFIX (go_qcauchy) (x, dist->location, dist->scale, TRUE, FALSE);
}

#ifdef DISTRIBUTION_LAST_PATH

static void
go_dist_cauchy_class_init (GObjectClass *klass)
{
	GODistributionClass *dist_klass = (GODistributionClass *) klass;
	dist_klass->dist_type = GO_DISTRIBUTION_CAUCHY;
	dist_klass->get_density = go_cauchy_get_density;
	dist_klass->get_cumulative = go_cauchy_get_cumulative;
	dist_klass->get_ppf = go_cauchy_get_ppf;
#ifdef GOFFICE_WITH_LONG_DOUBLE
	dist_klass->get_densityl = go_cauchy_get_densityl;
	dist_klass->get_cumulativel = go_cauchy_get_cumulativel;
	dist_klass->get_ppfl = go_cauchy_get_ppfl;
#endif
}

static void
go_dist_cauchy_init (GODistribution *dist)
{
}

GSF_CLASS (GODistCauchy, go_dist_cauchy,
	   go_dist_cauchy_class_init, go_dist_cauchy_init,
	   GO_TYPE_DISTRIBUTION)

#endif /* DISTRIBUTION_LAST_PATH */

/*****************************************************************************/
/*                        Weibull distribution                              */
/*****************************************************************************/
#ifdef DISTRIBUTION_FIRST_PATH

typedef struct {
	GODistribution base;
	double shape;
} GODistWeibull;

#define GO_TYPE_DIST_WEIBULL	  	(go_dist_weibull_get_type ())
#define GO_DIST_WEIBULL(o)		(G_TYPE_CHECK_INSTANCE_CAST((o), GO_TYPE_DIST_WEIBULL, GODistWeibull))
#define GO_IS_DIST_WEIBULL(o)		(G_TYPE_CHECK_INSTANCE_TYPE((o), GO_TYPE_DIST_WEIBULL))

GType go_dist_weibull_get_type (void);

typedef GODistributionClass GODistWeibullClass;

enum {
	WEIBULL_PROP_0,
	WEIBULL_PROP_SHAPE
};

#endif /* DISTRIBUTION_FIRST_PATH */

static DOUBLE
SUFFIX (go_weibull_get_density) (GODistribution *dist, DOUBLE x)
{
	return SUFFIX (go_dweibull) (x - dist->location, GO_DIST_WEIBULL (dist)->shape, dist->scale, FALSE) ;
}

static DOUBLE
SUFFIX (go_weibull_get_cumulative) (GODistribution *dist, DOUBLE x)
{
	return SUFFIX (go_pweibull) (x - dist->location, GO_DIST_WEIBULL (dist)->shape, dist->scale, TRUE, FALSE) ;
}

static DOUBLE
SUFFIX (go_weibull_get_ppf) (GODistribution *dist, DOUBLE x)
{
	return SUFFIX (go_qweibull) (x, GO_DIST_WEIBULL (dist)->shape, dist->scale, TRUE, FALSE) + dist->location;
}

#ifdef DISTRIBUTION_LAST_PATH

static void
go_dist_weibull_set_property (GObject *obj, guint param_id,
			      GValue const *value, GParamSpec *pspec)
{
	GODistWeibull *dist = GO_DIST_WEIBULL (obj);

	switch (param_id) {
	case WEIBULL_PROP_SHAPE:
		dist->shape = g_value_get_double (value);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
}

static void
go_dist_weibull_get_property (GObject *obj, guint param_id,
			      GValue *value, GParamSpec *pspec)
{
	GODistWeibull *dist = GO_DIST_WEIBULL (obj);

	switch (param_id) {
	case WEIBULL_PROP_SHAPE:
		g_value_set_double (value, dist->shape);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		break;
	}
}

static void
go_dist_weibull_class_init (GObjectClass *klass)
{
	GODistributionClass *dist_klass = (GODistributionClass *) klass;
	dist_klass->dist_type = GO_DISTRIBUTION_WEIBULL;
	dist_klass->get_density = go_weibull_get_density;
	dist_klass->get_cumulative = go_weibull_get_cumulative;
	dist_klass->get_ppf = go_weibull_get_ppf;
#ifdef GOFFICE_WITH_LONG_DOUBLE
	dist_klass->get_densityl = go_weibull_get_densityl;
	dist_klass->get_cumulativel = go_weibull_get_cumulativel;
	dist_klass->get_ppfl = go_weibull_get_ppfl;
#endif
	klass->set_property = go_dist_weibull_set_property;
	klass->get_property = go_dist_weibull_get_property;
	g_object_class_install_property (klass, WEIBULL_PROP_SHAPE,
		g_param_spec_double ("shape",
			_("Shape"),
			_("Shape factor"),
			G_MINDOUBLE,
			G_MAXDOUBLE,
			1.0,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
}

static void
go_dist_weibull_init (GODistWeibull *dist)
{
	dist->shape = 1.;
}

GSF_CLASS (GODistWeibull, go_dist_weibull,
	   go_dist_weibull_class_init, go_dist_weibull_init,
	   GO_TYPE_DISTRIBUTION)

#endif /* DISTRIBUTION_LAST_PATH */

/*****************************************************************************/
/*                        Lognormal distribution                              */
/*****************************************************************************/
#ifdef DISTRIBUTION_FIRST_PATH

typedef struct {
	GODistribution base;
	double shape;
} GODistLogNormal;

#define GO_TYPE_DIST_LOG_NORMAL	  	(go_dist_log_normal_get_type ())
#define GO_DIST_LOG_NORMAL(o)		(G_TYPE_CHECK_INSTANCE_CAST((o), GO_TYPE_DIST_LOG_NORMAL, GODistLogNormal))
#define GO_IS_DIST_LOG_NORMAL(o)	(G_TYPE_CHECK_INSTANCE_TYPE((o), GO_TYPE_DIST_LOG_NORMAL))

GType go_dist_log_normal_get_type (void);

typedef GODistributionClass GODistLogNormalClass;

enum {
	LNORM_PROP_0,
	LNORM_PROP_SHAPE
};

#endif /* DISTRIBUTION_FIRST_PATH */

static DOUBLE
SUFFIX (go_log_normal_get_density) (GODistribution *dist, DOUBLE x)
{
	return SUFFIX (go_dlnorm) ((x - dist->location) / dist->scale, 0., GO_DIST_LOG_NORMAL (dist)->shape, FALSE);
}

static DOUBLE
SUFFIX (go_log_normal_get_cumulative) (GODistribution *dist, DOUBLE x)
{
	return SUFFIX (go_plnorm) ((x - dist->location) / dist->scale, 0., GO_DIST_LOG_NORMAL (dist)->shape, TRUE, FALSE);
}

static DOUBLE
SUFFIX (go_log_normal_get_ppf) (GODistribution *dist, DOUBLE x)
{
	return SUFFIX (go_qlnorm) (x, 0., GO_DIST_LOG_NORMAL (dist)->shape, TRUE, FALSE) * dist->scale + dist->location;
}

#ifdef DISTRIBUTION_LAST_PATH

static void
go_dist_log_normal_set_property (GObject *obj, guint param_id,
			      GValue const *value, GParamSpec *pspec)
{
	GODistLogNormal *dist = GO_DIST_LOG_NORMAL (obj);

	switch (param_id) {
	case LNORM_PROP_SHAPE:
		dist->shape = g_value_get_double (value);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
}

static void
go_dist_log_normal_get_property (GObject *obj, guint param_id,
			      GValue *value, GParamSpec *pspec)
{
	GODistLogNormal *dist = GO_DIST_LOG_NORMAL (obj);

	switch (param_id) {
	case LNORM_PROP_SHAPE:
		g_value_set_double (value, dist->shape);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		break;
	}
}

static void
go_dist_log_normal_class_init (GObjectClass *klass)
{
	GODistributionClass *dist_klass = (GODistributionClass *) klass;
	dist_klass->dist_type = GO_DISTRIBUTION_LOGNORMAL;
	dist_klass->get_density = go_log_normal_get_density;
	dist_klass->get_cumulative = go_log_normal_get_cumulative;
	dist_klass->get_ppf = go_log_normal_get_ppf;
#ifdef GOFFICE_WITH_LONG_DOUBLE
	dist_klass->get_densityl = go_log_normal_get_densityl;
	dist_klass->get_cumulativel = go_log_normal_get_cumulativel;
	dist_klass->get_ppfl = go_log_normal_get_ppfl;
#endif
	klass->set_property = go_dist_log_normal_set_property;
	klass->get_property = go_dist_log_normal_get_property;
	g_object_class_install_property (klass, LNORM_PROP_SHAPE,
		g_param_spec_double ("shape",
			_("Shape"),
			_("Shape factor"),
			G_MINDOUBLE,
			G_MAXDOUBLE,
			1.0,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
}

static void
go_dist_log_normal_init (GODistLogNormal *dist)
{
	dist->shape = 1.;
}

GSF_CLASS (GODistLogNormal, go_dist_log_normal,
	   go_dist_log_normal_class_init, go_dist_log_normal_init,
	   GO_TYPE_DISTRIBUTION)

#endif /* DISTRIBUTION_LAST_PATH */

/*****************************************************************************/
/*                        Global functions                                   */
/*****************************************************************************/

#ifdef DISTRIBUTION_LAST_PATH

GODistribution*
go_distribution_new (GODistributionType type)
{
	switch (type) {
	case GO_DISTRIBUTION_NORMAL:
		return (GODistribution *) g_object_new (GO_TYPE_DIST_NORMAL, NULL);
	case GO_DISTRIBUTION_UNIFORM:
		return (GODistribution *) g_object_new (GO_TYPE_DIST_UNIFORM, NULL);
	case GO_DISTRIBUTION_CAUCHY:
		return (GODistribution *) g_object_new (GO_TYPE_DIST_CAUCHY, NULL);
	case GO_DISTRIBUTION_WEIBULL:
		return (GODistribution *) g_object_new (GO_TYPE_DIST_WEIBULL, NULL);
	case GO_DISTRIBUTION_LOGNORMAL:
		return (GODistribution *) g_object_new (GO_TYPE_DIST_LOG_NORMAL, NULL);
	default:
		return NULL;
	}
}

GODistributionType
go_distribution_get_distribution_type (GODistribution *dist)
{
	return GO_DISTRIBUTION_GET_CLASS (dist)->dist_type;
}

char const *
go_distribution_get_distribution_name (GODistribution *dist)
{
	return _(go_distribution_type_to_string (go_distribution_get_distribution_type (dist)));
}

void
_go_distributions_init (void)
{
	go_dist_normal_get_type ();
	go_dist_uniform_get_type ();
	go_dist_cauchy_get_type ();
	go_dist_weibull_get_type ();
	go_dist_log_normal_get_type ();
}

#endif /* DISTRIBUTION_LAST_PATH */
