/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-conf.c:
 *
 * Author:
 *	Andreas J. Guelzow <aguelzow@taliesin.ca>
 *
 * (C) Copyright 2002-2005 Andreas J. Guelzow <aguelzow@taliesin.ca>
 *
 * Introduced the concept of "node" and implemented the win32 backend
 * by Ivan, Wong Yat Cheung <email@ivanwong.info>, 2005
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <goffice-config.h>
#include <goffice/goffice.h>
#include <go-conf.h>

#define NO_DEBUG_GCONF
#ifndef NO_DEBUG_GCONF
#define d(code)	{ code; }
#else
#define d(code)
#endif

typedef struct {
	GOConfMonitorFunc monitor;
	GOConfNode *node;
	gpointer data;
	char *key;
	char *real_key;
} GOConfClosure;

static void
go_conf_closure_free (GOConfClosure *cls)
{
	g_free (cls->key);
	g_free (cls->real_key);
	g_free (cls);
}

#ifdef GOFFICE_WITH_GCONF
#include "go-conf-gconf.c"
#elif defined GOFFICE_WITH_WINREG
#include "go-conf-win32.c"
#elif defined GOFFICE_WITH_GSETTINGS
#include "go-conf-gsettings.c"
#else
#include "go-conf-keyfile.c"
#endif

gchar *
go_conf_get_enum_as_str (GOConfNode *node, gchar const *key)
{
	return go_conf_get_string (node, key);
}
int
go_conf_load_enum (GOConfNode *node, gchar const *key, GType t, int default_val)
{
	int	 res;
	gchar   *val_str = go_conf_load_string (node, key);
	gboolean use_default = TRUE;

	if (NULL != val_str) {
		GEnumClass *enum_class = G_ENUM_CLASS (g_type_class_ref (t));
		GEnumValue *enum_value = g_enum_get_value_by_nick (enum_class, val_str);
		if (NULL == enum_value)
			enum_value = g_enum_get_value_by_name (enum_class, val_str);

		if (NULL != enum_value) {
			use_default = FALSE;
			res = enum_value->value;
		} else {
			g_warning ("Unknown value '%s' for %s", val_str, key);
		}

		g_type_class_unref (enum_class);
		g_free (val_str);

	}

	if (use_default) {
		d (g_warning ("Using default value '%d'", default_val));
		return default_val;
	}
	return res;
}

void
go_conf_set_enum (GOConfNode *node, gchar const *key, GType t, gint val)
{
	GEnumClass *enum_class = G_ENUM_CLASS (g_type_class_ref (t));
	GEnumValue *enum_value = g_enum_get_value (enum_class, val);
	go_conf_set_string (node, key, enum_value->value_nick);
	g_type_class_unref (enum_class);
}

