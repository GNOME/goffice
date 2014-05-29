/*
 * go-unit.c - Units support
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

#include <goffice/utils/go-unit.h>
#include <string.h>

static GHashTable *units_hash = NULL;
static GPtrArray *custom_units;

struct _GoUnit {
	char *symbol;
	char *dim;
	double factor_to_SI; /* how many SI units is this one: 0.0254 for inch */
	unsigned Id;
};

static int last_unit = GO_UNIT_MAX;

GoUnit units[GO_UNIT_MAX] = {
	{(char *) "m", (char *) "L", 1., GO_UNIT_METER},
	{(char *) "cm", (char *) "L", 0.01, GO_UNIT_CENTIMETER},
	{(char *) "in", (char *) "L", 0.0254, GO_UNIT_INCH},
	{(char *) "pt", (char *) "L", 0.0254/72, GO_UNIT_POINT}
};

char const *
go_unit_get_symbol (GoUnit const *unit)
{
	g_return_val_if_fail (unit != NULL, NULL);
	return unit->symbol;
}

GoUnitId
go_unit_get_id (GoUnit const *unit)
{
	g_return_val_if_fail (unit != NULL, GO_UNIT_UNKNOWN);
	return unit->Id;
}

double
go_unit_convert (GoUnit const *from, GoUnit const *to, double value)
{
	g_return_val_if_fail (from != NULL && to != NULL, go_nan);
	if (strcmp (from->dim, to->dim))
		return go_nan;
	return value * from->factor_to_SI / to->factor_to_SI;
}

GoUnit const *go_unit_get_from_symbol (char const *symbol)
{
	if (units_hash == NULL)
		_go_unit_init ();
	return (GoUnit const *) g_hash_table_lookup (units_hash, symbol);
}

GoUnit const *
go_unit_get (GoUnitId Id)
{
	if (Id < 0)
		return NULL;
	if (Id < GO_UNIT_MAX)
		return units + Id;
	else if (custom_units != NULL && Id < last_unit)
		return g_ptr_array_index (custom_units, Id - GO_UNIT_MAX);
	return NULL;
}

GoUnit const *
go_unit_define (char const *symbol, char const *dim, double factor_to_SI)
{
	GoUnit *unit;
	if (units_hash == NULL)
		_go_unit_init ();
	unit = (GoUnit *) go_unit_get_from_symbol (symbol);
	if (unit == NULL) {
		unit = (GoUnit *) g_new (GoUnit, 1);
		unit->symbol = g_strdup (symbol);
		unit->dim = g_strdup (dim);
		unit->factor_to_SI = factor_to_SI;
		unit->Id = last_unit++;
		g_hash_table_replace (units_hash, unit->symbol, unit);
		if (custom_units == NULL)
			custom_units = g_ptr_array_new ();
		g_ptr_array_add (custom_units, unit);
	}
	return unit;
}

static void
go_unit_destroy (GoUnit *unit)
{
	if (unit != NULL && unit->Id >= GO_UNIT_MAX) {
		g_free (unit->symbol);
		g_free (unit->dim);
		g_free (unit);
	}
}

void
_go_unit_init ()
{
	GoUnitId Id;
	if (units_hash != NULL)
		return;
	g_hash_table_new_full (g_str_hash, g_str_equal, NULL, (GDestroyNotify) go_unit_destroy);
	for (Id = GO_UNIT_METER; Id < GO_UNIT_MAX; Id++)
		g_hash_table_insert (units_hash, units[Id].symbol, units + Id);
}

void _go_unit_shutdown ()
{
	if (units_hash == NULL)
		return;
	g_hash_table_destroy (units_hash);
	units_hash = NULL;
	if (custom_units != NULL) {
		g_ptr_array_unref (custom_units);
		custom_units = NULL;
	}
}
