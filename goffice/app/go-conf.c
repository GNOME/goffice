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
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#include <goffice-config.h>
#include <goffice/goffice.h>

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

/**
 * go_conf_node_get_type:
 *
 * Returns: the #GType for #GOConfNode.
 **/
/**
 * go_conf_get_node:
 * @parent: (nullable): parent node
 * @key: configuration key
 *
 * Returns: (transfer full): the #GOConfNode associated with @key.
 **/
/**
 * go_conf_free_node:
 * @node: (nullable): #GOConfNode
 *
 * Frees @node.
 **/
/**
 * go_conf_get_bool:
 * @node: (nullable): #GOConfNode
 * @key: configuration key
 *
 * Returns: the boolean value associated with @key.
 **/
/**
 * go_conf_get_int:
 * @node: (nullable): #GOConfNode
 * @key: configuration key
 *
 * Returns: the integer value associated with @key.
 **/
/**
 * go_conf_get_double:
 * @node: (nullable): #GOConfNode
 * @key: configuration key
 *
 * Returns: the double value associated with @key.
 **/
/**
 * go_conf_get_string:
 * @node: (nullable): #GOConfNode
 * @key: configuration key
 *
 * Returns: (transfer full) (nullable): the string value associated with
 * @key, or %NULL if @key is not set.
 **/
/**
 * go_conf_get_str_list:
 * @node: (nullable): #GOConfNode
 * @key: configuration key
 *
 * Returns: (element-type utf8) (transfer full): a list of strings associated
 * with @key.
 **/
/**
 * go_conf_get_enum_as_str:
 * @node: (nullable): #GOConfNode
 * @key: configuration key
 *
 * Returns: (transfer full) (nullable): the string value associated with
 * @key, or %NULL if @key is not set.
 **/
/**
 * go_conf_load_bool:
 * @node: (nullable): #GOConfNode
 * @key: configuration key
 * @default_val: value to use if @key is not set
 *
 * Returns: the boolean value associated with @key or @default_val.
 **/
/**
 * go_conf_load_int:
 * @node: (nullable): #GOConfNode
 * @key: configuration key
 * @minima: minimum allowed value
 * @maxima: maximum allowed value
 * @default_val: value to use if @key is not set
 *
 * Returns: the integer value associated with @key or @default_val.
 **/
/**
 * go_conf_load_double:
 * @node: (nullable): #GOConfNode
 * @key: configuration key
 * @minima: minimum allowed value
 * @maxima: maximum allowed value
 * @default_val: value to use if @key is not set
 *
 * Returns: the double value associated with @key or @default_val.
 **/
/**
 * go_conf_load_string:
 * @node: (nullable): #GOConfNode
 * @key: configuration key
 *
 * Returns: (transfer full) (nullable): the string value associated with
 * @key, or %NULL if @key is not set.
 **/
/**
 * go_conf_load_str_list:
 * @node: (nullable): #GOConfNode
 * @key: configuration key
 *
 * Returns: (element-type utf8) (transfer full): a list of strings associated
 * with @key.
 **/
/**
 * go_conf_load_enum:
 * @node: (nullable): #GOConfNode
 * @key: configuration key
 * @t: #GType of the enumeration
 * @default_val: value to use if @key is not set
 *
 * Returns: the enumeration value associated with @key or @default_val.
 **/
/**
 * go_conf_set_bool:
 * @node: #GOConfNode
 * @key: configuration key
 * @val: value to set
 *
 * Sets @val as the value for @key.
 **/
/**
 * go_conf_set_int:
 * @node: #GOConfNode
 * @key: configuration key
 * @val: value to set
 *
 * Sets @val as the value for @key.
 **/
/**
 * go_conf_set_double:
 * @node: #GOConfNode
 * @key: configuration key
 * @val: value to set
 *
 * Sets @val as the value for @key.
 **/
/**
 * go_conf_set_string:
 * @node: #GOConfNode
 * @key: configuration key
 * @str: value to set
 *
 * Sets @str as the value for @key.
 **/
/**
 * go_conf_set_str_list:
 * @node: #GOConfNode
 * @key: configuration key
 * @list: (element-type utf8): the list of strings to set.
 *
 * Sets @list as the value for @key.
 **/
/**
 * go_conf_set_enum:
 * @node: #GOConfNode
 * @key: configuration key
 * @t: #GType of the enumeration
 * @val: value to set
 *
 * Sets @val as the value for @key.
 **/
/**
 * go_conf_sync:
 * @node: (nullable): #GOConfNode
 *
 * Synchronizes configuration changes to disk.
 **/
/**
 * go_conf_remove_monitor:
 * @monitor_id: monitor ID
 *
 * Removes the monitor associated with @monitor_id.
 **/
/**
 * go_conf_add_monitor:
 * @node: (nullable): #GOConfNode
 * @key: configuration key
 * @monitor: (scope async): #GOConfMonitorFunc
 * @data: user data
 *
 * @monitor will be called whenever the value associated with @key changes.
 * Returns: the signal ID.
 **/

#if defined GOFFICE_WITH_WINREG
#include "go-conf-win32.c"
#elif defined GOFFICE_WITH_GSETTINGS
#include "go-conf-gsettings.c"
#else
#include "go-conf-keyfile.c"
#endif

static GOConfNode *
go_conf_node_ref (GOConfNode *node)
{
	node->ref_count++;
	return node;
}

GType
go_conf_node_get_type (void)
{
	static GType t = 0;

	if (t == 0) {
		t = g_boxed_type_register_static ("GOConfNode",
			 (GBoxedCopyFunc)go_conf_node_ref,
			 (GBoxedFreeFunc)go_conf_free_node);
	}
	return t;
}

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

