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

#include <glib/gstdio.h>
#include <errno.h>
#include <string.h>

#define BOOL_GROUP    "Booleans"
#define INT_GROUP     "Ints"
#define DOUBLE_GROUP  "Doubles"
#define STRING_GROUP  "Strings"
#define STRLIST_GROUP "StringLists"

struct _GOConfNode {
	gchar *path;
	GKeyFile *key_file;
	unsigned ref_count;
};

static GHashTable *key_files = NULL;

static GHashTable *key_monitor_by_id = NULL;
static GHashTable *key_monitors_by_path = NULL;
static guint key_monitors_id = 0;

static void
signal_monitors (const char *real_key)
{
	GSList *cls;

	for (cls = g_hash_table_lookup (key_monitors_by_path, real_key);
	     cls;
	     cls = cls->next) {
		GOConfClosure *cl = cls->data;
#if 0
		g_printerr ("Signal for %s to %p\n",
			    cl->real_key, cl->monitor);
#endif
		cl->monitor (cl->node, cl->real_key, cl->data);
	}
}


static gchar *get_rc_filename (char const *root_name)
{
	const gchar *home = g_get_home_dir ();
	gchar *fname = NULL;
	gchar *rcname = g_strconcat (".", root_name, "rc", NULL);

	if (home != NULL)
		fname = g_build_filename (home, rcname, NULL);

	g_free (rcname);
	return fname;
}

static void dump_key_data_to_file_cb (char const *rcfile, GKeyFile *key_file)
{
	FILE *fp = NULL;
	gchar *key_data;

	fp = g_fopen (rcfile, "w");
	if (fp == NULL) {
		g_warning ("Couldn't write configuration info to %s", rcfile);
		return;
	}

	key_data = g_key_file_to_data (key_file, NULL, NULL);

	if (key_data != NULL) {
		fputs (key_data, fp);
		g_free (key_data);
	}

	fclose (fp);
}

static gchar *
go_conf_get_real_key (GOConfNode const *key, gchar const *subkey)
{
	return key ? g_strconcat ((key)->path, subkey? "/": NULL , subkey, NULL) :
		     g_strdup (subkey);
}

void
_go_conf_init (void)
{
	key_files = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) g_key_file_free);
}

void
_go_conf_shutdown (void)
{
	if (!key_files)
		return;
	g_hash_table_foreach (key_files, (GHFunc) dump_key_data_to_file_cb, NULL);
	g_hash_table_destroy (key_files);
	key_files = NULL;
}

GOConfNode *
go_conf_get_node (GOConfNode *parent, gchar const *key)
{
	GOConfNode *node;

	node = g_new (GOConfNode, 1);
	node->path = go_conf_get_real_key (parent, key);
	node->ref_count = 1;
	if (parent)
		node->key_file = parent->key_file;
	else {
		char *end = strchr (key, '/'), *name, *rcfile;
		name = (end)? g_strndup (key, end - key): g_strdup (key);
		rcfile = get_rc_filename (name);
		g_free (name);
		if (rcfile != NULL) {
			GKeyFile *key_file = g_hash_table_lookup (key_files, rcfile);
			if (key_file == NULL) {
				key_file = g_key_file_new ();
				g_key_file_load_from_file (key_file, rcfile, G_KEY_FILE_NONE, NULL);
				g_hash_table_insert (key_files, rcfile, key_file);
			} else
				g_free (rcfile);
			node->key_file = key_file;
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

	g_free (node->path);
	g_free (node);
}

void
go_conf_set_bool (GOConfNode *node, gchar const *key, gboolean val)
{
	gchar *real_key = go_conf_get_real_key (node, key);

	g_key_file_set_boolean (node->key_file, BOOL_GROUP, real_key, val);
	signal_monitors (real_key);
	g_free (real_key);
}

void
go_conf_set_int (GOConfNode *node, gchar const *key, gint val)
{
	gchar *real_key = go_conf_get_real_key (node, key);

	g_key_file_set_integer (node->key_file, INT_GROUP, real_key, val);
	signal_monitors (real_key);
	g_free (real_key);
}

void
go_conf_set_double (GOConfNode *node, gchar const *key, gdouble val)
{
	gchar *real_key = go_conf_get_real_key (node, key);
	gchar str[G_ASCII_DTOSTR_BUF_SIZE];

	g_ascii_dtostr (str, sizeof (str), val);
	g_key_file_set_value (node->key_file, DOUBLE_GROUP, real_key, str);
	signal_monitors (real_key);
	g_free (real_key);
}

void
go_conf_set_string (GOConfNode *node, gchar const *key, char const *str)
{
	gchar *real_key = go_conf_get_real_key (node, key);

	g_key_file_set_string (node->key_file, STRING_GROUP, real_key, str);
	signal_monitors (real_key);
	g_free (real_key);
}

void
go_conf_set_str_list (GOConfNode *node, gchar const *key, GSList *list)
{
	gchar *real_key;
	gchar **strs = NULL;
	int i, ns;

	real_key = go_conf_get_real_key (node, key);
	ns = g_slist_length (list);

	/* +1 to ensure we don't get a NULL */
	strs = g_new (gchar *, ns + 1);

	for (i = 0; i < ns; i++) {
		const gchar *lstr = list->data;
		strs[i] = g_strdup (lstr);
		list = list->next;
	}

	g_key_file_set_string_list (node->key_file, STRLIST_GROUP, real_key,
				    (gchar const **const) strs, ns);
	signal_monitors (real_key);
	g_free (real_key);

	for (i = 0; i < ns; i++)
		g_free (strs[i]);
	g_free (strs);
}

gboolean
go_conf_get_bool (GOConfNode *node, gchar const *key)
{
	gboolean val;
	gchar *real_key;

	if (!node)
		return FALSE;
	real_key = go_conf_get_real_key (node, key);
	val = g_key_file_get_boolean (node->key_file, BOOL_GROUP, real_key, NULL);
	g_free (real_key);

	return val;
}

gint
go_conf_get_int	(GOConfNode *node, gchar const *key)
{
	gboolean val;
	gchar *real_key;

	if (!node)
		return 0;
	real_key = go_conf_get_real_key (node, key);
	val = g_key_file_get_integer (node->key_file, INT_GROUP, real_key, NULL);
	g_free (real_key);

	return val;
}

gdouble
go_conf_get_double (GOConfNode *node, gchar const *key)
{
	gchar *ptr;
	gchar *real_key;
	gdouble val;

	if (!node)
		return 0.;
	real_key = go_conf_get_real_key (node, key);
	ptr = g_key_file_get_value (node->key_file, DOUBLE_GROUP, real_key, NULL);
	g_free (real_key);
	if (ptr) {
		val = g_ascii_strtod (ptr, NULL);
		g_free (ptr);
		if (errno != ERANGE)
			return val;
	}

	return 0.0;
}

/**
 * go_conf_get_string:
 * @node: #GOConfNode
 * @key: non-%NULL string.
 *
 * Returns: the string value of @node's @key child as a string which the called needs to free
 **/
gchar *
go_conf_get_string (GOConfNode *node, gchar const *key)
{
	gchar *real_key;
	gchar *val = NULL;

	if (!node)
		return NULL;
	real_key = go_conf_get_real_key (node, key);
	val = g_key_file_get_string (node->key_file, STRING_GROUP, real_key, NULL);
	g_free (real_key);

	return val;
}

GSList *
go_conf_get_str_list (GOConfNode *node, gchar const *key)
{
	gchar *real_key;
	GSList *list = NULL;
	gchar **str_list;
	gsize i, nstrs;

	if (!node)
		return NULL;
	real_key = go_conf_get_real_key (node, key);
	str_list = g_key_file_get_string_list (node->key_file, STRLIST_GROUP, real_key, &nstrs, NULL);
	g_free (real_key);

	if (str_list != NULL) {
		for (i = 0; i < nstrs; i++) {
			if (str_list[i][0]) {
				list = g_slist_append (list, g_strcompress (str_list[i]));
			}
		}
		g_strfreev (str_list);
	}

	return list;
}

gboolean
go_conf_load_bool (GOConfNode *node, gchar const *key, gboolean default_val)
{
	gchar *real_key;
	gboolean val;
	GError *err = NULL;

	if (!node)
		return default_val;

	real_key = go_conf_get_real_key (node, key);
	val = g_key_file_get_boolean (node->key_file, BOOL_GROUP, real_key, &err);
	if (err) {
		val = default_val;
		g_error_free (err);
#if 0
		d (g_warning ("%s: using default value '%s'", real_key, default_val ? "true" : "false"));
#endif
	}

	g_free (real_key);
	return val;
}

int
go_conf_load_int (GOConfNode *node, gchar const *key,
		  gint minima, gint maxima,
		  gint default_val)
{
	gchar *real_key;
	int val;
	GError *err = NULL;

	if (!node)
		return default_val;

	real_key = go_conf_get_real_key (node, key);
	val = g_key_file_get_integer (node->key_file, INT_GROUP, real_key, &err);

	if (err) {
		val = default_val;
		g_error_free(err);
#if 0
		d (g_warning ("%s: using default value %d", real_key, default_val));
#endif
	} else if (val < minima || val > maxima) {
		val = default_val;
	}

	g_free (real_key);
	return val;
}

double
go_conf_load_double (GOConfNode *node, gchar const *key,
		     gdouble minima, gdouble maxima,
		     gdouble default_val)
{
	gchar *real_key;
	gchar *ptr;
	double val;
	GError *err = NULL;

	if (!node)
		return default_val;

	real_key = go_conf_get_real_key (node, key);
	ptr = g_key_file_get_value (node->key_file, DOUBLE_GROUP, real_key, &err);

	if (err) {
		val = default_val;
		g_error_free (err);
	} else {
		val = g_ascii_strtod (ptr, NULL);
		if (val < minima || val > maxima) {
			val = default_val;
		}
	}

	g_free(ptr);
	g_free (real_key);
	return val;
}

char *
go_conf_load_string (GOConfNode *node, gchar const *key)
{
	gchar *real_key;
	char *val = NULL;
	GError *err = NULL;

	if (!node)
		return NULL;

	real_key = go_conf_get_real_key (node, key);
	val = g_key_file_get_string (node->key_file, STRING_GROUP, real_key, &err);

	if (err) {
#if 0
		g_warning (err->message);
#endif
		g_error_free (err);
	}

	g_free (real_key);
	return val;
}

GSList *
go_conf_load_str_list (GOConfNode *node, gchar const *key)
{
	return go_conf_get_str_list (node, key);
}

void
go_conf_sync (GOConfNode *node)
{
}

void
go_conf_remove_monitor (guint monitor_id)
{
	GOConfClosure *cl = key_monitor_by_id
		? g_hash_table_lookup (key_monitor_by_id,
				       GUINT_TO_POINTER (monitor_id))
		: NULL;
	GSList *cls;

	g_return_if_fail (cl != NULL);

	g_hash_table_remove (key_monitor_by_id,
			     GUINT_TO_POINTER (monitor_id));
	if (g_hash_table_size (key_monitor_by_id) == 0) {
		g_hash_table_destroy (key_monitor_by_id);
		key_monitor_by_id = NULL;
	}

	cls = g_hash_table_lookup (key_monitors_by_path, cl->real_key);
	cls = g_slist_remove (cls, cl);
	if (cls)
		g_hash_table_replace (key_monitors_by_path,
				      g_strdup (cl->real_key),
				      cls);
	else
		g_hash_table_remove (key_monitors_by_path, cl->real_key);
	go_conf_closure_free (cl);

	if (g_hash_table_size (key_monitors_by_path) == 0) {
		g_hash_table_destroy (key_monitors_by_path);
		key_monitors_by_path = NULL;
	}
}

guint
go_conf_add_monitor (GOConfNode *node, gchar const *key,
		     GOConfMonitorFunc monitor, gpointer data)
{
	GOConfClosure *cl;
	GSList *cls;
	guint res;

	g_return_val_if_fail (node || key, 0);
	g_return_val_if_fail (monitor != NULL, 0);

	res = ++key_monitors_id;

	cl = g_new (GOConfClosure, 1);
	cl->monitor = monitor;
	cl->node = node;
	cl->data = data;
	cl->key = g_strdup (key);
	cl->real_key = go_conf_get_real_key (node, key);

	if (!key_monitor_by_id)
		key_monitor_by_id =
			g_hash_table_new (g_direct_hash, g_direct_equal);
	g_hash_table_insert (key_monitor_by_id,
			     GUINT_TO_POINTER (res),
			     cl);

	if (!key_monitors_by_path)
		key_monitors_by_path =
			g_hash_table_new_full
			(g_str_hash, g_str_equal, g_free, NULL);

	cls = g_hash_table_lookup (key_monitors_by_path, cl->real_key);
	cls = g_slist_prepend (cls, cl);
	g_hash_table_replace (key_monitors_by_path,
			      g_strdup (cl->real_key),
			      cls);

	return res;
}
