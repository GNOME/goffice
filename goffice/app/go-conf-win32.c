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

#include <windows.h>

#ifndef ERANGE
/* mingw has not defined ERANGE (yet), MSVC has it though */
# define ERANGE 34
#endif

struct _GOConfNode {
	HKEY hKey;
	gchar *path;
	unsigned ref_count;
};

void
_go_conf_init (void)
{
}

void
_go_conf_shutdown (void)
{
}

static gchar *
go_conf_get_real_key (GOConfNode const *key, gchar const *subkey)
{
	return key
		? (subkey
		   ? g_strconcat (key->path, "/", subkey, NULL)
		   : g_strdup (key->path))
		: g_strdup (subkey);
}

static gchar *
go_conf_get_win32_key (GOConfNode const *key, gchar const *subkey)
{
	gchar *real_key = go_conf_get_real_key (key, subkey);
	gchar *p;

	for (p = real_key; *p; p++)
		if (*p == '/')
			*p = '\\';

	return real_key;
}


static gboolean
go_conf_win32_get_node (HKEY *phKey, gchar const *win32_key, gboolean *is_new)
{
	gchar *full_key;
	LONG ret;
	DWORD disposition;

	*phKey = (HKEY)0;

	full_key = g_strconcat ("Software\\", win32_key, NULL);
	ret = RegCreateKeyEx (HKEY_CURRENT_USER, full_key,
			      0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
			      NULL, phKey, &disposition);
	if (ret != ERROR_SUCCESS)
		g_printerr ("Failed to create [%s]\n", full_key);
	g_free (full_key);

	if (is_new)
		*is_new = disposition == REG_CREATED_NEW_KEY;

	return ret == ERROR_SUCCESS;
}

static gboolean
go_conf_win32_set (GOConfNode *node, gchar const *key,
		   gint type, gconstpointer data, gint size)
{
	gchar *win32_key = go_conf_get_win32_key (node, key);
	gchar *last_sep = strrchr (win32_key, '\\');
	HKEY hKey;
	gboolean ok;
	LONG ret;

	if (!last_sep)
		return FALSE; /* Shouldn't happen.  */

	*last_sep = 0;
	key = last_sep + 1;
	ok = go_conf_win32_get_node (&hKey, win32_key, NULL);
	if (!ok) {
		g_free (win32_key);
		return FALSE;
	}

	ret = RegSetValueEx (hKey, key, 0, type, data, size);
	RegCloseKey (hKey);

	if (ret != ERROR_SUCCESS)
		g_printerr ("Failed to write registry key %s\n", win32_key);

	g_free (win32_key);

	return TRUE;
}

static gboolean
go_conf_win32_get (GOConfNode *node, gchar const *key,
		   gulong *type, guchar **data, gulong *size,
		   gboolean do_realloc, gboolean complain)
{
	gchar *win32_key = go_conf_get_win32_key (node, key);
	gchar *last_sep = strrchr (win32_key, '\\');
	HKEY hKey;
	gboolean ok;
	LONG ret;

	if (!last_sep)
		return FALSE; /* Shouldn't happen.  */

	*last_sep = 0;
	key = last_sep + 1;
	ok = go_conf_win32_get_node (&hKey, win32_key, NULL);
	if (!ok) {
		g_free (win32_key);
		return FALSE;
	}

	if (!*data && do_realloc) {
		ret = RegQueryValueEx (hKey, key, NULL, type, NULL, size);
		if (ret != ERROR_SUCCESS)
			goto out;
		*data = g_new (guchar, *size);
	}

	while ((ret = RegQueryValueEx (hKey, key, NULL, type, *data, size)) ==
	       ERROR_MORE_DATA &&
	       do_realloc)
		*data = g_realloc (*data, *size);

	RegCloseKey (hKey);

out:
	*last_sep = '\\';

	if (ret != ERROR_SUCCESS && complain) {
		LPTSTR msg_buf;

		FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
			       FORMAT_MESSAGE_FROM_SYSTEM |
			       FORMAT_MESSAGE_IGNORE_INSERTS,
			       NULL,
			       ret,
			       MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
			       (LPTSTR) &msg_buf,
			       0,
			       NULL);
		g_warning ("Unable to get registry key %s because %s",
			   win32_key, msg_buf);
		LocalFree (msg_buf);
	}

	g_free (win32_key);
	return ret == ERROR_SUCCESS;
}

static const size_t WIN32_MAX_REG_KEYNAME_LEN = 256;
static const size_t WIN32_MAX_REG_VALUENAME_LEN = 32767;
static const size_t WIN32_INIT_VALUE_DATA_LEN = 2048;

static void
go_conf_win32_clone_full (HKEY hSrcKey, const gchar *key, HKEY hDstKey,
			  gchar *subkey, gchar *value_name,
			  GString *data)
{
	gint i;
	HKEY hSrcSK, hDstSK;
	LONG ret;

	if (RegOpenKeyEx (hSrcKey, key, 0, KEY_READ, &hSrcSK) != ERROR_SUCCESS)
		return;

	ret = ERROR_SUCCESS;
	for (i = 0; ret == ERROR_SUCCESS; ++i) {
		DWORD name_size = WIN32_MAX_REG_KEYNAME_LEN;
		FILETIME ft;
		ret = RegEnumKeyEx (hSrcSK, i, subkey, &name_size, NULL, NULL, NULL, &ft);
		if (ret != ERROR_SUCCESS)
			continue;

		if (RegCreateKeyEx (hDstKey, subkey, 0, NULL, 0, KEY_WRITE,
				    NULL, &hDstSK, NULL) == ERROR_SUCCESS) {
			go_conf_win32_clone_full (hSrcSK, subkey, hDstSK,
						  subkey, value_name, data);
			RegCloseKey (hDstSK);
		}
	}

	ret = ERROR_SUCCESS;
	for (i = 0; ret == ERROR_SUCCESS; ++i) {
		DWORD name_size = WIN32_MAX_REG_KEYNAME_LEN;
		DWORD data_size = data->len;
		DWORD type;
		while ((ret = RegEnumValue (hSrcSK, i, value_name, &name_size,
					    NULL, &type,
					    data->str, &data_size)) ==
		       ERROR_MORE_DATA)
			g_string_set_size (data, data_size);
		if (ret != ERROR_SUCCESS)
			continue;

		RegSetValueEx (hDstKey, value_name, 0, type,
			       data->str, data_size);
	}

	RegCloseKey (hSrcSK);
}

static void
go_conf_win32_clone (HKEY hSrcKey, const gchar *key, HKEY hDstKey)
{
	char *subkey = g_malloc (WIN32_MAX_REG_KEYNAME_LEN);
	char *value_name = g_malloc (WIN32_MAX_REG_VALUENAME_LEN);
	GString *data = g_string_sized_new (WIN32_INIT_VALUE_DATA_LEN);
	go_conf_win32_clone_full (hSrcKey, key, hDstKey,
				  subkey, value_name, data);
	g_string_free (data, TRUE);
	g_free (value_name);
	g_free (subkey);
}

GOConfNode *
go_conf_get_node (GOConfNode *parent, const gchar *key)
{
	gchar *win32_key = go_conf_get_win32_key (parent, key);
	gchar *last_sep = strrchr (win32_key, '\\');
	HKEY hKey;
	gboolean is_new;
	GOConfNode *node = NULL;
	gboolean err;

	if (last_sep) {
		*last_sep = 0;
		/*
		 * This probably produces the wrong hKey because it refers
		 * to the directory, not the node.  We need to cut the last
		 * component, or else we get a new, empty, directory.
		 */
		err = go_conf_win32_get_node (&hKey, win32_key, &is_new);
		*last_sep = '\\';
	} else {
		err = TRUE;
		is_new = TRUE;
	}

	if (err) {
		if (!parent && is_new) {
			gchar *path;

			path = g_strconcat (".DEFAULT\\Software\\", win32_key, NULL);
			go_conf_win32_clone (HKEY_USERS, path, hKey);
			g_free (path);
		}
		node = g_new (GOConfNode, 1);
		node->hKey = hKey;
		node->path = win32_key;
		node->ref_count = 1;
	} else {
		g_free (win32_key);
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

	RegCloseKey (node->hKey);
	g_free (node->path);
	g_free (node);
}

void
go_conf_set_bool (GOConfNode *node, gchar const *key, gboolean val_)
{
	guchar val = val_ ? 1 : 0;

	go_conf_win32_set (node, key, REG_BINARY, &val, sizeof (val));
}

void
go_conf_set_int (GOConfNode *node, gchar const *key, gint val_)
{
	DWORD val = val_;
	go_conf_win32_set (node, key, REG_DWORD, &val, sizeof (val));
}

void
go_conf_set_double (GOConfNode *node, gchar const *key, gdouble val)
{
	gchar str[G_ASCII_DTOSTR_BUF_SIZE];

	g_ascii_dtostr (str, sizeof (str), val);
	go_conf_win32_set (node, key, REG_SZ, str, strlen (str) + 1);
}

void
go_conf_set_string (GOConfNode *node, gchar const *key, gchar const *str)
{
	go_conf_win32_set (node, key, REG_SZ, str, strlen (str) + 1);
}

void
go_conf_set_str_list (GOConfNode *node, gchar const *key, GSList *list)
{
	GString *str_list;
	GSList *l;

	str_list = g_string_new (NULL);
	for (l = list; l; l = l->next) {
		const char *s = l->data;
		if (str_list->len)
			g_string_append_c (str_list, '\n');
		g_string_append (str_list, g_strescape (s, NULL));
	}
	go_conf_win32_set (node, key, REG_SZ, str_list->str, str_list->len + 1);
	g_string_free (str_list, TRUE);
}

gboolean
go_conf_get_bool (GOConfNode *node, gchar const *key)
{
	guchar val, *ptr = &val;
	gulong type, size = sizeof (val);

	if (go_conf_win32_get (node, key, &type, &ptr, &size, FALSE, FALSE) &&
	    type == REG_BINARY)
		return val;

	return FALSE;
}

gint
go_conf_get_int	(GOConfNode *node, gchar const *key)
{
	DWORD val;
	gulong type, size = sizeof (DWORD);
	guchar *ptr = (guchar *) &val;

	if (go_conf_win32_get (node, key, &type, &ptr, &size, FALSE, FALSE) &&
	    type == REG_DWORD)
		return val;

	return 0;
}

gdouble
go_conf_get_double (GOConfNode *node, gchar const *key)
{
	gchar *ptr = go_conf_get_string (node, key);
	gdouble val;

	if (ptr) {
		val = go_ascii_strtod (ptr, NULL);
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
	DWORD type, size = 0;
	guchar *ptr = NULL;

	if (go_conf_win32_get (node, key, &type, &ptr, &size, TRUE, FALSE) &&
	    type == REG_SZ)
		return ptr;

	g_free (ptr);

	return NULL;
}

GSList *
go_conf_get_str_list (GOConfNode *node, gchar const *key)
{
	return go_conf_load_str_list (node, key);
}

static guchar *
go_conf_get (GOConfNode *node, gchar const *key, gulong expected)
{
	gulong type, size = 0;
	guchar *ptr = NULL;

	if (!go_conf_win32_get (node, key, &type, &ptr, &size, TRUE, TRUE)) {
		g_free (ptr);
		return NULL;
	}

	if (type != expected) {
		g_warning ("Expected `%lu' got `%lu' for key %s of node %s",
			   expected, type, key, node->path);
		g_free (ptr);
		return NULL;
	}

	return ptr;
}

gboolean
go_conf_load_bool (GOConfNode *node, gchar const *key,
		   gboolean default_val)
{
	guchar *val = go_conf_get (node, key, REG_BINARY);
	gboolean res;

	if (val) {
		res = (gboolean) *val;
		g_free (val);
	} else {
		d (g_warning ("Using default value '%s'", default_val ? "true" : "false"));
		return default_val;
	}

	return res;
}

gint
go_conf_load_int (GOConfNode *node, gchar const *key,
		  gint minima, gint maxima, gint default_val)
{
	guchar *val = go_conf_get (node, key, REG_DWORD);
	gint res;

	if (val) {
		res = *(gint *) val;
		g_free (val);
		if (res < minima || maxima < res) {
			g_warning ("Invalid value '%d' for %s. If should be >= %d and <= %d",
				   res, key, minima, maxima);
			val = NULL;
		}
	}
	if (!val) {
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
	gchar *val = (gchar *) go_conf_get (node, key, REG_SZ);

	if (val) {
		res = g_ascii_strtod (val, NULL);
		g_free (val);
		if (errno == ERANGE || res < minima || maxima < res) {
			g_warning ("Invalid value '%g' for %s.  If should be >= %g and <= %g",
				   res, key, minima, maxima);
			val = NULL;
		}
	}
	if (!val) {
		d (g_warning ("Using default value '%g'", default_val));
		return default_val;
	}

	return res;
}

gchar *
go_conf_load_string (GOConfNode *node, gchar const *key)
{
	return go_conf_get (node, key, REG_SZ);
}

GSList *
go_conf_load_str_list (GOConfNode *node, gchar const *key)
{
	GSList *list = NULL;
	gchar *ptr;
	gchar **str_list;
	gint i;

	if ((ptr = go_conf_get_string (node, key)) != NULL) {
		str_list = g_strsplit ((gchar const *) ptr, "\n", 0);
		for (i = 0; str_list[i]; ++i)
			list = g_slist_prepend (list, g_strcompress (str_list[i]));
		list = g_slist_reverse (list);
		g_strfreev (str_list);
		g_free (ptr);
	}

	return list;
}

void
go_conf_sync (GOConfNode *node)
{
	if (node)
		RegFlushKey (node->hKey);
}

void
go_conf_remove_monitor (guint monitor_id)
{
	(void)&go_conf_closure_free;
}

guint
go_conf_add_monitor (GOConfNode *node, gchar const *key,
		     GOConfMonitorFunc monitor, gpointer data)
{
	return 1;
}
