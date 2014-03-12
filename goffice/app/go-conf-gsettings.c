/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) version 3.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA.
 */

#include <string.h>

struct _GOConfNode {
	gchar *path;
	gchar *id;
	gchar *key;
	GSettings *settings;
	unsigned ref_count;
};

static GHashTable *installed_schemas, *closures;
void
_go_conf_init (void)
{
	char const * const *schemas = g_settings_list_schemas ();
	char const * const *cur = schemas;
	installed_schemas = g_hash_table_new (g_str_hash, g_str_equal);
	while (*cur) {
		g_hash_table_insert (installed_schemas, (gpointer) *cur, GUINT_TO_POINTER (TRUE));
		cur++;
	}
	closures = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) go_conf_closure_free);
}

void
_go_conf_shutdown (void)
{
	g_hash_table_destroy (installed_schemas);
	g_hash_table_destroy (closures);
}

static gchar *
go_conf_format_id (gchar const *key)
{
	char *cur, *res;
	if (!key)
		return NULL;
	res = g_strdup (key);
	/* replace all slashes by dots */
	cur = res;
	while ((cur = strchr (cur, '/')) && *cur)
		*cur = '.';
	/* replace all underscores by hyphens */
	cur = res;
	while ((cur = strchr (cur, '_')) && *cur)
		*cur = '-';
	cur = res;
	while (*cur) {
		*cur = g_ascii_tolower (*cur);
		cur++;
	}
	return res;
}

GOConfNode *
go_conf_get_node (GOConfNode *parent, gchar const *key)
{
	GOConfNode *node;
	char *formatted;

	g_return_val_if_fail (parent || key, NULL);

	formatted = go_conf_format_id (key);
	node = g_new0 (GOConfNode, 1);
	node->ref_count = 1;
	if (parent) {
		if (key && !parent->key) {
			node->path = g_strconcat (parent->path, "/", key, NULL);
			node->id = g_strconcat (parent->id, ".", formatted, NULL);
		} else {
			node->path = g_strdup (parent->path);
			node->id = g_strdup (parent->id);
			node->key = g_strdup (key? key: parent->key);
		}
	} else {
		if (key[0] == '/') {
			node->path = g_strdup (key);
			node->id = g_strconcat ("org.gnome", formatted, NULL);
		} else {
			node->path = g_strconcat ("/apps/", key, NULL);
			node->id = g_strconcat ("org.gnome.", formatted, NULL);
		}
	}
	node->settings = g_hash_table_lookup (installed_schemas, node->id)? g_settings_new (node->id): NULL;
	g_free (formatted);
	if (!node->settings) {
		char *last_dot = strrchr (node->id, '.');
		*last_dot = 0;
		node->settings = g_hash_table_lookup (installed_schemas, node->id)? g_settings_new (node->id): NULL;
		if (node->settings) {
			g_free (node->key);
			node->key = g_strdup (last_dot + 1);
		} else {
			go_conf_free_node (node);
			node = NULL;
		}
	}
	return node;
}

void
go_conf_free_node (GOConfNode *node)
{
	if (!node)
		return;

	g_return_if_fail (node->ref_count > 0);

	node->ref_count--;
	if (node->ref_count > 0)
		return;

	if (node->settings)
		g_object_unref (node->settings);
	g_free (node->path);
	g_free (node->id);
	g_free (node->key);
	g_free (node);
}

void
go_conf_sync (GOConfNode *node)
{
	g_settings_sync ();
}

void
go_conf_set_bool (GOConfNode *node, gchar const *key, gboolean val)
{
	GOConfNode *real_node = go_conf_get_node (node, key);
	if (!real_node) {
		d (g_warning ("Unable to set key '%s'", key));
		return;
	}
	g_settings_set_boolean (real_node->settings, real_node->key, val);
	go_conf_free_node (real_node);
}

void
go_conf_set_int (GOConfNode *node, gchar const *key, gint val)
{
	GOConfNode *real_node = go_conf_get_node (node, key);
	if (!real_node) {
		d (g_warning ("Unable to set key '%s'", key));
		return;
	}
	g_settings_set_int (real_node->settings, real_node->key, val);
	go_conf_free_node (real_node);
}

void
go_conf_set_double (GOConfNode *node, gchar const *key, gdouble val)
{
	GOConfNode *real_node = go_conf_get_node (node, key);
	if (!real_node) {
		d (g_warning ("Unable to set key '%s'", key));
		return;
	}
	g_settings_set_double (real_node->settings, real_node->key, val);
	go_conf_free_node (real_node);
}

void
go_conf_set_string (GOConfNode *node, gchar const *key, gchar const *str)
{
	GOConfNode *real_node = go_conf_get_node (node, key);
	if (!real_node) {
		d (g_warning ("Unable to set key '%s'", key));
		return;
	}
	g_settings_set_string (real_node->settings, real_node->key, str);
	go_conf_free_node (real_node);
}

void
go_conf_set_str_list (GOConfNode *node, gchar const *key, GSList *list)
{
	GOConfNode *real_node = go_conf_get_node (node, key);
	gssize n = g_slist_length (list);
	char const ** strs;
	GSList *ptr = list;
	unsigned i = 0;

	if (!real_node) {
		d (g_warning ("Unable to set key '%s'", key));
		return;
	}
	strs = g_new (char const*, n + 1);
	for (; ptr; ptr = ptr->next)
		strs[i++] = (char const *) ptr->data;
	strs[n] = NULL;
	g_settings_set_strv (real_node->settings, real_node->key, strs);
	g_free (strs);
	go_conf_free_node (real_node);
}

static GVariant *
go_conf_get (GOConfNode *node, gchar const *key, GVariantType const *t)
{
	GVariant *res = g_settings_get_value (node->settings, key);
	if (res == NULL) {
		d (g_warning ("Unable to load key '%s'", key));
		return NULL;
	}

	if (!g_variant_is_of_type (res, t)) {
		char *tstr = g_variant_type_dup_string (t);
		g_warning ("Expected `%s' got `%s' for key %s",
			   tstr,
			   g_variant_get_type_string (res),
			   key);
		g_free (tstr);
		g_variant_unref (res);
		return NULL;
	}

	return res;
}

gboolean
go_conf_load_bool (GOConfNode *node, gchar const *key, gboolean default_val)
{
	gboolean res;
	GVariant *val = NULL;
	if (node) {
		if (key && !strchr (key, '/') &&  !strchr (key, '.'))
			val = go_conf_get (node, key, G_VARIANT_TYPE_BOOLEAN);
		else if (node->key)
			val = go_conf_get (node, node->key, G_VARIANT_TYPE_BOOLEAN);
	}
	if (val == NULL) {
		GOConfNode *real_node = go_conf_get_node (node, key);
		val = real_node? go_conf_get (real_node, real_node->key, G_VARIANT_TYPE_BOOLEAN): NULL;
		go_conf_free_node (real_node);
	}

	if (val != NULL) {
		res = g_variant_get_boolean (val);
		g_variant_unref (val);
	} else {
		d (g_warning ("Using default value '%s'", default_val ? "true" : "false"));
		return default_val;
	}
	return res;
}

gint
go_conf_load_int (GOConfNode *node, gchar const *key, gint minima, gint maxima, gint default_val)
{
	gint res = -1;
	GVariant *val = NULL;
	if (node) {
		if (key && !strchr (key, '/') &&  !strchr (key, '.'))
			val = go_conf_get (node, key, G_VARIANT_TYPE_INT32);
		else if (node->key)
			val = go_conf_get (node, node->key, G_VARIANT_TYPE_INT32);
	}
	if (val == NULL) {
		GOConfNode *real_node = go_conf_get_node (node, key);
		val = real_node? go_conf_get (real_node, real_node->key, G_VARIANT_TYPE_INT32): NULL;
		go_conf_free_node (real_node);
	}
	if (val != NULL) {
		res = g_variant_get_int32 (val);
		g_variant_unref (val);
		if (res < minima || maxima < res) {
			g_warning ("Invalid value '%d' for %s.  If should be >= %d and <= %d",
				   res, key, minima, maxima);
			val = NULL;
		}
	} else {
		d (g_warning ("Using default value '%d'", default_val));
		return default_val;
	}
	return res;
}

gdouble
go_conf_load_double (GOConfNode *node, gchar const *key,
		     gdouble minima, gdouble maxima, gdouble default_val)
{
        gdouble res = -1;
	GVariant *val = NULL;
	if (node) {
		if (key && !strchr (key, '/') &&  !strchr (key, '.'))
			val = go_conf_get (node, key, G_VARIANT_TYPE_DOUBLE);
		else if (node->key)
			val = go_conf_get (node, node->key, G_VARIANT_TYPE_DOUBLE);
	}
	if (val == NULL) {
		GOConfNode *real_node = go_conf_get_node (node, key);
		val = real_node? go_conf_get (real_node, real_node->key, G_VARIANT_TYPE_DOUBLE): NULL;
		go_conf_free_node (real_node);
	}
	if (val != NULL) {
		res = g_variant_get_double (val);
		g_variant_unref (val);
		if (res < minima || maxima < res) {
			g_warning ("Invalid value '%g' for %s.  If should be >= %g and <= %g",
				   res, key, minima, maxima);
			val = NULL;
		}
	} else {
		d (g_warning ("Using default value '%g'", default_val));
		return default_val;
	}
	return res;
}

gchar *
go_conf_load_string (GOConfNode *node, gchar const *key)
{
	char *res = NULL;
	if (node) {
		if (key && !strchr (key, '/') &&  !strchr (key, '.'))
			res = g_settings_get_string (node->settings, key);
		else if (node->key)
			res = g_settings_get_string (node->settings, node->key);
	}
	if (res == NULL) {
		GOConfNode *real_node = go_conf_get_node (node, key);
		res = (real_node)? g_settings_get_string (real_node->settings, real_node->key): NULL;
		go_conf_free_node (real_node);
	}
	return res;
}

GSList *
go_conf_load_str_list (GOConfNode *node, gchar const *key)
{
	char **strs = NULL, **ptr;
	GSList *list = NULL;

	if (node) {
		if (key && !strchr (key, '/') &&  !strchr (key, '.'))
			strs = g_settings_get_strv (node->settings, key);
		else if (node->key)
			strs = g_settings_get_strv (node->settings, node->key);
	}
	if (strs == NULL) {
		GOConfNode *real_node = go_conf_get_node (node, key);
		strs = real_node? g_settings_get_strv (node->settings, real_node->key): NULL;
		go_conf_free_node (real_node);
	}
	if (strs) {
		for (ptr = strs; *ptr; ptr++)
			list = g_slist_prepend (list, g_strdup (*ptr));
		g_strfreev (strs);
		list = g_slist_reverse (list);
	}

	return list;
}

gboolean
go_conf_get_bool (GOConfNode *node, gchar const *key)
{
	gboolean res, failed = node == NULL;
	if (!failed) {
		if (key && !strchr (key, '/') &&  !strchr (key, '.'))
			res = g_settings_get_boolean (node->settings, key);
		else if (node->key)
			res = g_settings_get_boolean (node->settings, node->key);
		else
			failed = TRUE;
	}
	if (failed) {
		GOConfNode *real_node = go_conf_get_node (node, key);
		res = (real_node)? g_settings_get_boolean (real_node->settings, real_node->key): FALSE;
		go_conf_free_node (real_node);
	}
	return res;
}

gint
go_conf_get_int	(GOConfNode *node, gchar const *key)
{
	GOConfNode *real_node = go_conf_get_node (node, key);
	gint res = (real_node)? g_settings_get_int (real_node->settings, real_node->key): 0;
	go_conf_free_node (real_node);
	return res;
}

gdouble
go_conf_get_double (GOConfNode *node, gchar const *key)
{
	GOConfNode *real_node = go_conf_get_node (node, key);
	gdouble res = (real_node)? g_settings_get_double (real_node->settings, real_node->key): 0.;
	go_conf_free_node (real_node);
	return res;
}

gchar *
go_conf_get_string (GOConfNode *node, gchar const *key)
{
	GOConfNode *real_node = go_conf_get_node (node, key);
	gchar *res = (real_node)? g_settings_get_string (real_node->settings, real_node->key): NULL;
	go_conf_free_node (real_node);
	return res;
}

GSList *
go_conf_get_str_list (GOConfNode *node, gchar const *key)
{
	return go_conf_load_str_list (node, key);
}

void
go_conf_remove_monitor (guint monitor_id)
{
	GOConfClosure *cls = g_hash_table_lookup (closures, GUINT_TO_POINTER (monitor_id));
	if (cls) {
		g_signal_handler_disconnect (cls->node->settings, monitor_id);
		g_hash_table_remove (closures, GUINT_TO_POINTER (monitor_id));
	} else
		g_warning ("unknown GOConfMonitor id.");
}

static void
cb_key_changed (GSettings *settings,
		char *key,
		GOConfClosure *cls)
{
	char *real_key;
	if (cls->key) {
		if (strcmp (cls->key, key))
			return; /* not the watched key */
		real_key = g_strdup (cls->real_key);
	} else
		real_key = g_strconcat (cls->real_key, "/", key, NULL);
	cls->monitor (cls->node, real_key , cls->data);
	g_free (real_key);
}

guint
go_conf_add_monitor (GOConfNode *node, G_GNUC_UNUSED gchar const *key,
		     GOConfMonitorFunc monitor, gpointer data)
{
	guint ret;
	GOConfClosure *cls;

	g_return_val_if_fail (node || key, 0);
	g_return_val_if_fail (monitor != NULL, 0);

	cls = g_new (GOConfClosure, 1);
	cls->monitor = monitor;
	cls->node = node;
	cls->data = data;
	cls->key = g_strdup (key? key: node->key);
	cls->real_key = (key)? g_strconcat (node->path, '/', key, NULL): g_strdup (node->path);
	ret = g_signal_connect
		(node->settings,
		 "changed", G_CALLBACK (cb_key_changed),
		 cls);

	g_hash_table_insert (closures, GUINT_TO_POINTER (ret), cls);
	return ret;
}
