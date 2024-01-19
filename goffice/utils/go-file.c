/*
 * go-file.c :
 *
 * Copyright (C) 2004,2009-2012 Morten Welinder (terra@gnome.org)
 * Copyright (C) 2003, Red Hat, Inc.
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
#include <goffice/goffice.h>
#include <gsf/gsf-input-memory.h>
#include <gsf/gsf-input-stdio.h>
#include <gsf/gsf-output-stdio.h>
#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>
#include <gsf/gsf-input-gio.h>
#include <gsf/gsf-output-gio.h>
#ifdef G_OS_WIN32
#include <windows.h>
#include <io.h>
#endif
#ifdef GOFFICE_WITH_GTK
#include <gtk/gtk.h>
#endif
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef HAVE_GRP_H
#include <grp.h>
#endif
#include <time.h>

/**
 * GOFilePermissions:
 * @owner_read: owner can read.
 * @owner_write: owner can write.
 * @owner_execute: owner can execute.
 * @group_read: group can read.
 * @group_write: group can write.
 * @group_execute: group can execute.
 * @others_read: everybody can read.
 * @others_write: everybody can write.
 * @others_execute: everybody can execute.
 **/

/**
 * GODotDot:
 * @GO_DOTDOT_SYNTACTIC: assume no symlinks.
 * @GO_DOTDOT_TEST: check.
 * @GO_DOTDOT_LEAVE: leave alone.
 **/

#ifdef G_OS_WIN32
typedef HRESULT (WINAPI *FindMimeFromData_t) (LPBC pBC,
					      LPCWSTR pwzUrl,
					      LPVOID pBuffer,
					      DWORD cbSize,
					      LPCWSTR pwzMimeProposed,
					      DWORD dwMimeFlags,
					      LPWSTR *ppwzMimeOut,
					      DWORD dwReserved);
#define FMFD_ENABLEMIMESNIFFING 2

static FindMimeFromData_t
find_mime_from_data (void)
{
	HMODULE urlmon;
	static FindMimeFromData_t result = NULL;
	static gboolean beenhere = FALSE;

	if (!beenhere) {
		urlmon = LoadLibrary ("urlmon.dll");
		if (urlmon != NULL)
			result = (FindMimeFromData_t) GetProcAddress (urlmon, "FindMimeFromData");
		beenhere = TRUE;
	}

	return result;
}

#endif
/* ------------------------------------------------------------------------- */

/*
 * Convert an escaped URI into a filename.
 */
char *
go_filename_from_uri (char const *uri)
{
	return g_filename_from_uri (uri, NULL, NULL);
}

/*
 * Convert a filename into an escaped URI.
 */
char *
go_filename_to_uri (char const *filename)
{
	char *simp, *uri;

	g_return_val_if_fail (filename != NULL, NULL);

	simp = go_filename_simplify (filename, GO_DOTDOT_TEST, TRUE);

	uri = g_filename_to_uri (simp, NULL, NULL);
	g_free (simp);
	return uri;
}

char *
go_filename_simplify (char const *filename, GODotDot dotdot,
		      gboolean make_absolute)
{
	char *simp, *p, *q;

	g_return_val_if_fail (filename != NULL, NULL);

	if (make_absolute && !g_path_is_absolute (filename)) {
		/*
		 * FIXME: this probably does not work for "c:foo" on
		 * Win32.
		 */
		char *current_dir = g_get_current_dir ();
		simp = g_build_filename (current_dir, filename, NULL);
		g_free (current_dir);
	} else
		simp = g_strdup (filename);

	for (p = q = simp; *p;) {
		if (p != simp &&
		    G_IS_DIR_SEPARATOR (p[0]) &&
		    G_IS_DIR_SEPARATOR (p[1])) {
			/* "//" --> "/", except initially.  */
			p++;
			continue;
		}

		if (G_IS_DIR_SEPARATOR (p[0]) &&
		    p[1] == '.' &&
		    G_IS_DIR_SEPARATOR (p[2])) {
			/* "/./" -> "/".  */
			p += 2;
			continue;
		}

		if (G_IS_DIR_SEPARATOR (p[0]) &&
		    p[1] == '.' &&
		    p[2] == '.' &&
		    G_IS_DIR_SEPARATOR (p[3])) {
			if (p == simp) {
				/* "/../" --> "/" initially.  */
				p += 3;
				continue;
			} else if (p == simp + 1) {
				/* Nothing, leave "//../" initially alone.  */
			} else {
				/*
				 * "prefix/dir/../" --> "prefix/" if
				 * "dir" is an existing directory (not
				 * a symlink).
				 */
				gboolean isdir;

				switch (dotdot) {
				case GO_DOTDOT_SYNTACTIC:
					isdir = TRUE;
					break;
				case GO_DOTDOT_TEST: {
					GStatBuf statbuf;
					char savec = *q;
					/*
					 * Terminate the path so far so we can
					 * it.  Restore because "p" loops over
					 * the same.
					 */
					*q = 0;
					isdir = (g_lstat (simp, &statbuf) == 0) &&
						1;/*S_ISDIR (statbuf.st_mode);*/
					*q = savec;
					break;
				}
				default:
					isdir = FALSE;
					break;
				}

				if (isdir) {
					do {
						g_assert (q != simp);
						q--;
					} while (!G_IS_DIR_SEPARATOR (*q));
					p += 3;
					continue;
				} else {
					/*
					 * Do nothing.
					 *
					 * Maybe the prefix does not
					 * exist, or maybe it is not
					 * a directory (for example
					 * because it is a symlink).
					 */
				}
			}
		}

		*q++ = *p++;
	}
	*q = 0;

	return simp;
}

/*
 * Simplify a potentially non-local path using only slashes.
 */
static char *
simplify_path (char const *uri)
{
	char *simp, *p, *q;

	simp = g_strdup (uri);

	for (p = q = simp; *p;) {
		if (p[0] == '/' && p[1] == '/') {
			/* "//" --> "/".  */
			p++;
			continue;
		}

		if (p[0] == '/' && p[1] == '.' && p[2] == '/') {
			/* "/./" -> "/".  */
			p += 2;
			continue;
		}

		if (p[0] == '/' && p[1] == '.' && p[2] == '.' && p[3] == '/') {
			if (p == simp) {
				/* "/../" --> "/" initially.  */
				p += 3;
				continue;
			} else {
				/* Leave alone */
			}
		}

		*q++ = *p++;
	}
	*q = 0;

	return simp;
}

static char *
simplify_host_path (char const *uri, size_t hstart)
{
	char const *slash = strchr (uri + hstart, '/');
	char *simp, *psimp;
	size_t pstart;

	if (!slash)
		return g_strdup (uri);

	pstart = slash + 1 - uri;
	psimp = simplify_path (slash + 1);
	simp = g_new (char, pstart + strlen (psimp) + 1);
	memcpy (simp, uri, pstart);
	strcpy (simp + pstart, psimp);
	g_free (psimp);
	return simp;
}

char *
go_url_simplify (char const *uri)
{
	char *simp, *p;

	g_return_val_if_fail (uri != NULL, NULL);

	if (g_ascii_strncasecmp (uri, "file:///", 8) == 0) {
		char *filename = go_filename_from_uri (uri);
		char *simp = filename ? go_filename_to_uri (filename) : NULL;
		g_free (filename);
		return simp;
	}

	if (g_ascii_strncasecmp (uri, "http://", 7) == 0)
		simp = simplify_host_path (uri, 7);
	else if (g_ascii_strncasecmp (uri, "https://", 8) == 0)
		simp = simplify_host_path (uri, 8);
	else if (g_ascii_strncasecmp (uri, "ftp://", 6) == 0)
		simp = simplify_host_path (uri, 6);
	else
		simp = g_strdup (uri);

	/* Lower-case protocol name.  */
	for (p = simp; g_ascii_isalpha (*p); p++)
		*p = g_ascii_tolower (*p);

	return simp;
}


/*
 * More or less the same as gnome_vfs_uri_make_full_from_relative.
 */
char *
go_url_resolve_relative (char const *ref_uri, char const *rel_uri)
{
	char *simp, *uri;

	size_t len;

	g_return_val_if_fail (ref_uri != NULL, NULL);
	g_return_val_if_fail (rel_uri != NULL, NULL);

	len = strlen (ref_uri);

	/* FIXME: This doesn't work if ref_uri starts with a slash.  */

	uri = g_new (char, len + strlen (rel_uri) + 1);
	memcpy (uri, ref_uri, len + 1);
	while (len > 0 && uri[len - 1] != '/')
		len--;
	if (len == 0) {
		g_free (uri);
		return NULL;
	}

	strcpy (uri + len, rel_uri);

	simp = go_url_simplify (uri);
	g_free (uri);
	return simp;
}

/* ------------------------------------------------------------------------- */

static char *
make_rel (char const *uri, char const *ref_uri,
	  char const *uri_host, char const *slash)
{
	char const *p, *q;
	int n;
	GString *res;

	if (!slash)
		return NULL;

	if (uri_host != NULL &&
	    strncmp (uri_host, ref_uri + (uri_host - uri), slash - uri_host))
		return NULL;

	for (p = slash; *p; p++) {
		if (*p != ref_uri[p - uri])
			break;
		else if (*p == '/')
			slash = p;
	}
	/* URI components agree until slash.  */

	/* Find out the number of '/' in uri after slash.  */
	n = 0;
	q = slash;
	while (1) {
		q = strchr (q + 1, '/');
		if (q)
			n++;
		else
			break;
	}

	res = g_string_new (NULL);
	while (n-- > 0)
		g_string_append (res, "../");
	g_string_append (res, slash + 1);
	return g_string_free (res, FALSE);
}

char *
go_url_make_relative (char const *uri, char const *ref_uri)
{
	int i;

	/* Check that protocols are the same.  */
	for (i = 0; 1; i++) {
		char c = uri[i];
		char rc = ref_uri[i];

		if (c == 0)
			return NULL;

		if (c == ':') {
			if (rc == ':')
				break;
			return NULL;
		}

		if (g_ascii_tolower (c) != g_ascii_tolower (rc))
			return NULL;
	}

	if (g_ascii_strncasecmp (uri, "file:///", 8) == 0)
		return make_rel (uri, ref_uri, NULL, uri + 7);  /* Yes, 7.  */

	if (g_ascii_strncasecmp (uri, "http://", 7) == 0)
		return make_rel (uri, ref_uri, uri + 7, strchr (uri + 7, '/'));

	if (g_ascii_strncasecmp (uri, "https://", 8) == 0)
		return make_rel (uri, ref_uri, uri + 8, strchr (uri + 8, '/'));

	if (g_ascii_strncasecmp (uri, "ftp://", 6) == 0)
		return make_rel (uri, ref_uri, uri + 6, strchr (uri + 6, '/'));

	return NULL;
}

/* ------------------------------------------------------------------------- */

static gboolean
is_fd_uri (char const *uri, int *fd)
{
	unsigned long ul;
	char *end;

	if (g_ascii_strncasecmp (uri, "fd://", 5))
		return FALSE;
	uri += 5;
	if (!g_ascii_isdigit (*uri))
		return FALSE;  /* Space, for example.  */

	ul = strtoul (uri, &end, 10);

	/* Accept a terminating slash because go_shell_arg_to_uri will add
	   one. */
	if (*end == '/') end++;

	if (*end != 0 || ul > INT_MAX)
		return FALSE;

	*fd = (int)ul;
	return TRUE;
}

/* ------------------------------------------------------------------------- */

/*
 * Convert a shell argv entry (assumed already translated into filename
 * encoding) to an escaped URI.
 */
char *
go_shell_arg_to_uri (char const *arg)
{
	GFile *file;
	gchar *tmp;
	int fd;

	/*
	 * We do this explicitly because gio under Win32 likes to map
	 * things to file://  It does not hurt anywhere.
	 */
	if (is_fd_uri (arg, &fd))
		return g_strdup (arg);

	file = g_file_new_for_commandline_arg (arg);
	tmp = g_file_get_uri (file);
	g_object_unref (file);
	return tmp;
}

/**
 * go_basename_from_uri:
 * @uri: The uri
 *
 * Decode the final path component.  Returns as UTF-8 encoded suitable
 * for display.
 *
 * Returns: (transfer full): UTF-8 encoded basename.
 **/
char *
go_basename_from_uri (char const *uri)
{
	GFile *file = g_file_new_for_uri (uri);
	char *res = g_file_get_basename (file);
	g_object_unref (file);
	return res;
}

/**
 * go_dirname_from_uri:
 * @uri: target
 * @brief: if %TRUE, hide "file://" if present.
 *
 * Decode the all but the final path component.  Returns as UTF-8 encoded
 * suitable for display.
 *
 * Returns: (transfer full): UTF-8 encoded directory part.
 **/
char *
go_dirname_from_uri (char const *uri, gboolean brief)
{
	char *dirname_utf8, *dirname;
	char *uri_dirname = g_path_get_dirname (uri);
	dirname = uri_dirname ? go_filename_from_uri (uri_dirname) : NULL;
	g_free (uri_dirname);

	if (dirname) {
		char *temp = g_strconcat ("file://", dirname, NULL);
		g_free (dirname);
		dirname = temp;
	}

	if (brief && dirname &&
	    g_ascii_strncasecmp (dirname, "file:///", 8) == 0) {
		char *temp = g_strdup (dirname + 7);
		g_free (dirname);
		dirname = temp;
	}

	dirname_utf8 = dirname ? g_filename_display_name (dirname) : NULL;
	g_free (dirname);
	return dirname_utf8;
}

/* ------------------------------------------------------------------------- */

static GsfInput *
open_plain_file (char const *path, GError **err)
{
	GsfInput *input = gsf_input_mmap_new (path, NULL);
	if (input != NULL)
		return input;
	/* Only report error if stdio fails too */
	return gsf_input_stdio_new (path, err);
}


/**
 * go_file_open:
 * @uri: target uri
 * @err: #GError
 *
 * Try all available methods to open a file or return an error
 * Returns: (transfer full): non-%NULL on success
 **/
GsfInput *
go_file_open (char const *uri, GError **err)
{
	char *filename;
	int fd;

	if (err != NULL)
		*err = NULL;
	g_return_val_if_fail (uri != NULL, NULL);

	if (uri[0] == G_DIR_SEPARATOR) {
		g_warning ("Got plain filename %s in go_file_open.", uri);
		return open_plain_file (uri, err);
	}

	filename = go_filename_from_uri (uri);
	if (filename) {
		GsfInput *result = open_plain_file (filename, err);
		g_free (filename);
		return result;
	}

	if (is_fd_uri (uri, &fd)) {
		int fd2 = dup (fd);
		FILE *fil = fd2 != -1 ? fdopen (fd2, "rb") : NULL;
		GsfInput *result = fil ? gsf_input_stdio_new_FILE (uri, fil, FALSE) : NULL;

		if (!result)
			g_set_error (err, gsf_output_error_id (), 0,
				     "Unable to read from %s", uri);
		return result;
	}

	/* for now, GIO seems unable to retrieve files properly using http(s)
	 * see https://bugzilla.gnome.org/show_bug.cgi?id=724970 */
	if (!g_ascii_strncasecmp (uri, "http://", 7) || !g_ascii_strncasecmp (uri, "https://", 8))
		return gsf_input_http_new (uri, err);

	return gsf_input_gio_new_for_uri (uri, err);
}

/**
 * go_file_create:
 * @uri: target uri
 * @err: #GError
 *
 * Creates a file or return an error. If the file already exists, it is
 * replaced.
 * Returns: (transfer full): non-%NULL on success
 **/
GsfOutput *
go_file_create (char const *uri, GError **err)
{
	char *filename;
	int fd;

	g_return_val_if_fail (uri != NULL, NULL);

	filename = go_filename_from_uri (uri);
	if (filename) {
		GsfOutput *result = gsf_output_stdio_new (filename, err);
		g_free (filename);
		return result;
	}

	if (is_fd_uri (uri, &fd)) {
		int fd2 = dup (fd);
		FILE *fil = fd2 != -1 ? fdopen (fd2, "wb") : NULL;
		GsfOutput *result = fil ? gsf_output_stdio_new_FILE (uri, fil, FALSE) : NULL;

		if (!result)
			g_set_error (err, gsf_output_error_id (), 0,
				     "Unable to write to %s", uri);
		return result;
	}

	return gsf_output_gio_new_for_uri (uri, err);
}

/* ------------------------------------------------------------------------- */
/* Adapted from gtkfilechooserdefault.c.  Unfortunately it is static there.  */

/**
 * go_file_split_urls:
 * @data: a string filled with a list of URIs separated by new lines.
 *
 * Splits the chain into a list of URIs. Lines starting with '#' are ignored.
 * Returns: (element-type utf8) (transfer full): the URIs list.
 **/
GSList *
go_file_split_urls (char const *data)
{
  GSList *uris;
  char const *p, *q;

  uris = NULL;

  p = data;

  /* We don't actually try to validate the URI according to RFC
   * 2396, or even check for allowed characters - we just ignore
   * comments and trim whitespace off the ends.  We also
   * allow LF delimination as well as the specified CRLF.
   *
   * We do allow comments like specified in RFC 2483.
   */
  while (p)
    {
      if (*p != '#')
	{
	  while (g_ascii_isspace (*p))
	    p++;

	  q = p;
	  while (*q && (*q != '\n') && (*q != '\r'))
	    q++;

	  if (q > p)
	    {
	      q--;
	      while (q > p && g_ascii_isspace (*q))
		q--;

	      if (q > p)
		uris = g_slist_prepend (uris, g_strndup (p, q - p + 1));
	    }
	}
      p = strchr (p, '\n');
      if (p)
	p++;
    }

  uris = g_slist_reverse (uris);
  return uris;
}

/*
 * Return the real name of the owner of a URI.  The result will be in
 * UTF-8 and the caller must free the result.
 */
gchar *
go_file_get_owner_name (char const *uri)
{
	char const *name;
	GString *nameutf8 = NULL;
	GFile *file = g_file_new_for_uri (uri);
	GError *error = NULL;
	GFileInfo *info = g_file_query_info (file,
					     G_FILE_ATTRIBUTE_OWNER_USER,
					     G_FILE_QUERY_INFO_NONE, NULL, &error);

	if (error) {
		g_error_free (error);
		return NULL;
	}

	name = g_file_info_get_attribute_string (info, G_FILE_ATTRIBUTE_OWNER_USER);
	(void) go_guess_encoding (name, strlen (name),
				  NULL, &nameutf8, NULL);
	g_object_unref (info);
	return (nameutf8 ? g_string_free (nameutf8, FALSE) : NULL);
}

/*
 * Return the group name of the owner of a URI.  The result will be in
 * UTF-8 and the caller must free the result.
 */
gchar *
go_file_get_group_name (char const *uri)
{
	char const *name;
	GString *nameutf8 = NULL;
	GFile *file = g_file_new_for_uri (uri);
	GError *error = NULL;
	GFileInfo *info = g_file_query_info (file,
					     G_FILE_ATTRIBUTE_OWNER_GROUP,
					     G_FILE_QUERY_INFO_NONE, NULL, &error);

	if (error) {
		g_error_free (error);
		return NULL;
	}

	name = g_file_info_get_attribute_string (info, G_FILE_ATTRIBUTE_OWNER_GROUP);
	(void) go_guess_encoding (name, strlen (name),
				  NULL, &nameutf8, NULL);
	g_object_unref (info);
	return (nameutf8 ? g_string_free (nameutf8, FALSE) : NULL);
}

static GOFilePermissions *
go_file_permissions_copy (GOFilePermissions *file_permissions)
{
	GOFilePermissions *res = g_new (GOFilePermissions, 1);
	memcpy (res, file_permissions, sizeof(GOFilePermissions));
	return res;
}

GType
go_file_permissions_get_type (void)
{
	static GType t = 0;

	if (t == 0) {
		t = g_boxed_type_register_static ("GOFilePermissions",
			 (GBoxedCopyFunc)go_file_permissions_copy,
			 (GBoxedFreeFunc)g_free);
	}
	return t;
}

GOFilePermissions *
go_get_file_permissions (char const *uri)
{
	GOFilePermissions * file_permissions = NULL;

	/* Try getting unix mode */
	GFile *file = g_file_new_for_uri (uri);
	GError *error = NULL;
	GFileInfo *info = g_file_query_info (file,
					     G_FILE_ATTRIBUTE_UNIX_MODE,
					     G_FILE_QUERY_INFO_NONE, NULL, &error);
	if (error) {
		g_error_free (error);
		error = NULL;
	   	info = g_file_query_info (file,
					  G_FILE_ATTRIBUTE_ACCESS_CAN_READ","
					  G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE","
					  G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE,
					  G_FILE_QUERY_INFO_NONE, NULL, &error);
		if (error)
			g_error_free (error);
		else {
			file_permissions = g_new0 (GOFilePermissions, 1);

			/* Owner  Permissions */
			file_permissions->owner_read    = g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ);
			file_permissions->owner_write   = g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE);
			file_permissions->owner_execute = g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE);
		}
	} else {
		mode_t mode = (mode_t) g_file_info_get_attribute_uint32 (info, G_FILE_ATTRIBUTE_UNIX_MODE);
		file_permissions = g_new0 (GOFilePermissions, 1);

		/* Owner  Permissions */
		file_permissions->owner_read    = ((mode & S_IRUSR) != 0);
		file_permissions->owner_write   = ((mode & S_IWUSR) != 0);
		file_permissions->owner_execute = ((mode & S_IXUSR) != 0);

#ifndef G_OS_WIN32
		/* Group  Permissions */
		file_permissions->group_read    = ((mode & S_IRGRP) != 0);
		file_permissions->group_write   = ((mode & S_IWGRP) != 0);
		file_permissions->group_execute = ((mode & S_IXGRP) != 0);

		/* Others Permissions */
		file_permissions->others_read    = ((mode & S_IROTH) != 0);
		file_permissions->others_write   = ((mode & S_IWOTH) != 0);
		file_permissions->others_execute = ((mode & S_IXOTH) != 0);
#else
		file_permissions->group_read    =
		file_permissions->group_write   =
		file_permissions->group_execute =
		file_permissions->others_read    =
		file_permissions->others_write   =
		file_permissions->others_execute = FALSE;
#endif /* G_OS_WIN32 */
	}

	if (info)
		g_object_unref (info);
	g_object_unref (file);

	return file_permissions;
}

void
go_set_file_permissions (char const *uri, GOFilePermissions * file_permissions)
{
	guint32 permissions = 0;
	GFile *file = g_file_new_for_uri (uri);
	GError *error = NULL;

	/* Set owner permissions */
	if (file_permissions->owner_read == TRUE)
		permissions |= S_IRUSR;

	if (file_permissions->owner_write == TRUE)
		permissions |= S_IWUSR;

	if (file_permissions->owner_execute == TRUE)
		permissions |= S_IXUSR;

#ifndef G_OS_WIN32
	/* Set group permissions */
	if (file_permissions->group_read == TRUE)
		permissions |= S_IRGRP;

	if (file_permissions->group_write == TRUE)
		permissions |= S_IWGRP;

	if (file_permissions->group_execute == TRUE)
		permissions |= S_IXGRP;

	/* Set others permissions */
	if (file_permissions->others_read == TRUE)
		permissions |= S_IROTH;

	if (file_permissions->others_write == TRUE)
		permissions |= S_IWOTH;

	if (file_permissions->others_execute == TRUE)
		permissions |= S_IXOTH;
#endif

	/* Try setting unix mode */
	g_file_set_attribute_uint32 (file,
				     G_FILE_ATTRIBUTE_UNIX_MODE,
				     permissions,
				     G_FILE_QUERY_INFO_NONE,
				     NULL, &error);
	if (error) {
		/* Try setting access flags */
		GFileInfo *info = g_file_info_new ();
		g_error_free (error);
		g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ, file_permissions->owner_read);
		g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE, file_permissions->owner_write);
		g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE, file_permissions->owner_execute);
		g_file_set_attributes_from_info (file,
						 info,
						 G_FILE_QUERY_INFO_NONE,
						 NULL, NULL);
	}

	g_object_unref (file);
}

typedef enum {
	GO_FILE_DATE_TYPE_ACCESSED = 0,
	GO_FILE_DATE_TYPE_MODIFIED,
	GO_FILE_DATE_TYPE_CHANGED
} GOFileDateType;

static time_t
go_file_get_date (char const *uri, GOFileDateType type)
{
	time_t tm = -1;
	GFile *file = g_file_new_for_uri (uri);
	GError *error = NULL;
	GFileInfo *info = NULL;

	switch (type) {
	case GO_FILE_DATE_TYPE_ACCESSED:
		info = g_file_query_info (file,
					  G_FILE_ATTRIBUTE_TIME_ACCESS,
					  G_FILE_QUERY_INFO_NONE, NULL, &error);
		if (error) {
			g_error_free (error);
			break;
		}
		tm = (time_t) g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_TIME_ACCESS);
		break;
	case GO_FILE_DATE_TYPE_MODIFIED:
		info = g_file_query_info (file,
					  G_FILE_ATTRIBUTE_TIME_MODIFIED,
					  G_FILE_QUERY_INFO_NONE, NULL, &error);
		if (error) {
			g_error_free (error);
			break;
		}
		tm = (time_t) g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_TIME_MODIFIED);
		break;
	case GO_FILE_DATE_TYPE_CHANGED:
		info = g_file_query_info (file,
					  G_FILE_ATTRIBUTE_TIME_CHANGED,
					  G_FILE_QUERY_INFO_NONE, NULL, &error);
		if (error) {
			g_error_free (error);
			break;
		}
		tm = (time_t) g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_TIME_CHANGED);
		break;
	}

	if (info)
		g_object_unref (info);
	g_object_unref (file);

	return tm;
}

time_t
go_file_get_date_accessed (char const *uri)
{
	return go_file_get_date (uri, GO_FILE_DATE_TYPE_ACCESSED);
}

time_t
go_file_get_date_modified (char const *uri)
{
	return go_file_get_date (uri, GO_FILE_DATE_TYPE_MODIFIED);
}

time_t
go_file_get_date_changed (char const *uri)
{
	return go_file_get_date (uri, GO_FILE_DATE_TYPE_CHANGED);
}

/**
 * go_file_get_modtime:
 * @uri: The uri
 *
 * Determine the last modification time of @uri.
 *
 * Returns: (transfer full) (nullable): the modification time of the
 * file.
 **/
GDateTime *
go_file_get_modtime (char const *uri)
{
	GDateTime *modtime = NULL;
	GFile *file = g_file_new_for_uri (uri);
	GFileInfo *info;

	info = g_file_query_info (file,
				  G_FILE_ATTRIBUTE_TIME_MODIFIED ","
				  G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC,
				  G_FILE_QUERY_INFO_NONE, NULL, NULL);
	if (info) {
#ifdef HAVE_G_FILE_INFO_GET_MODIFICATION_DATE_TIME
		modtime = g_file_info_get_modification_date_time (info);
#else
		GTimeVal tv;
		g_file_info_get_modification_time (info, &tv);
		modtime = g_date_time_new_from_timeval_utc (&tv);
#endif
		g_object_unref (info);
	}

	g_object_unref (file);

	return modtime;
}

/* ------------------------------------------------------------------------- */

/**
 * go_url_encode:
 * @uri: The uri part to be encoded
 * @type: 0: mailto, 1: file or http
 *
 * url-encode a string according to RFC 2368.
 *
 * Returns: (transfer full): encoded string.
 **/
gchar*
go_url_encode (gchar const *uri_fragment, int type)
{
	char const *good;

	g_return_val_if_fail (*uri_fragment != '\0', NULL);

	switch (type) {
	case 0: /* mailto: */
		good = ".-_@";
		break;
	case 1: /* file: or http: */
		good = G_URI_RESERVED_CHARS_ALLOWED_IN_PATH;
		break;
	default:
		return NULL;
	}

	return g_uri_escape_string (uri_fragment, good, FALSE);
}

/**
 * go_url_check_extension:
 * @uri: Uri
 * @std_ext: Standard extension for the content type
 * @new_uri: (out) (optional) (nullable): New uri
 *
 * Modifies given @uri by adding the extension @std_ext if needed.
 * If no @std_ext is given or @uri already has some extension,
 * it just copies @uri.
 *
 * Value in new_uri:  newly allocated string which you should free after
 *                    use, containing (optionally) modified uri.
 *
 * Return Value: %FALSE if the uri has an extension not matching @std_ext
 */
gboolean
go_url_check_extension (gchar const *uri,
			gchar const *std_ext,
			gchar **new_uri)
{
	gchar *base;
	gchar *user_ext;
	gboolean res;

	g_return_val_if_fail (uri != NULL, FALSE);

	res      = TRUE;
	base     = g_path_get_basename (uri);
	user_ext = strrchr (base, '.');
	if (std_ext != NULL && strlen (std_ext) > 0 && !user_ext && new_uri)
		*new_uri = g_strconcat (uri, ".", std_ext, NULL);
	else {
		if (user_ext != NULL && std_ext != NULL)
			res = !go_utf8_collate_casefold (user_ext + 1, std_ext);
		if (new_uri)
			*new_uri = g_strdup (uri);
	}
	g_free (base);

	return res;
}

/**
 * go_get_mime_type:
 * @uri: the uri.
 *
 * Returns: (transfer full): the mime type for the file.
**/
gchar *
go_get_mime_type (gchar const *uri)
{
#if defined(G_OS_WIN32)
	/* Do we really need that? */
	LPWSTR wuri, mime_type;

	wuri = g_utf8_to_utf16 (uri, -1, NULL, NULL, NULL);
	if (wuri &&
	    find_mime_from_data () &&
	    (find_mime_from_data ()) (NULL, wuri,
				      NULL, 0,
				      NULL, FMFD_ENABLEMIMESNIFFING, &mime_type, 0) == NOERROR)	{
		g_free (wuri);
		return g_utf16_to_utf8 (mime_type, -1, NULL, NULL, NULL);
	}

	g_free (wuri);

	/* We try to determine mime using FindMimeFromData().
	 * However, we are not sure whether the functions will know about
	 * the necessary mime types. In the worst wase we fall back to
	 * "text/plain"
	 */
	return g_strdup ("text/plain");
#else
	GFile *file = g_file_new_for_uri (uri);
	GError *error = NULL;
	gchar *content_type = NULL, *mime_type = NULL;
	GFileInfo *info = g_file_query_info (file,
					     G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
					     G_FILE_QUERY_INFO_NONE,
					     NULL, &error);
	g_object_unref (file);
	if (error) {
		g_error_free (error);
		/* on failure, try to guess from the file name */
		content_type = g_content_type_guess (uri, NULL, 0, NULL);
	} else
		content_type = g_strdup (g_file_info_get_content_type (info));
	if (content_type) {
		mime_type = g_content_type_get_mime_type (content_type);
		g_free (content_type);
	}
	if (info)
		g_object_unref (info);
	if (mime_type)
		return mime_type;
	return g_strdup ("application/octet-stream");
#endif
}

/**
 * go_get_mime_type_for_data:
 * @data: the data.
 * @data_size: the data size
 *
 * Returns: (transfer full): the mime type for the data.
**/
gchar *
go_get_mime_type_for_data (gconstpointer data, int data_size)
{
#if defined(G_OS_WIN32)
	/* Do we really need that? */
	LPWSTR mime_type;

	if (find_mime_from_data () &&
	    (find_mime_from_data ()) (NULL, NULL,
				      (LPVOID)data, data_size,
				      NULL, FMFD_ENABLEMIMESNIFFING, &mime_type, 0) == NOERROR)	{
		return g_utf16_to_utf8 (mime_type, -1, NULL, NULL, NULL);
	}

	/* We try to determine mime using FindMimeFromData().
	 * However, we are not sure whether the functions will know about
	 * the necessary mime types. In the worst wase we fall back to
	 * "text/plain" */
	return g_strdup ("text/plain");
#else
	gchar *content_type = g_content_type_guess (NULL, data, data_size, NULL);
	gchar *mime_type = NULL;
	if (content_type) {
		mime_type = g_content_type_get_mime_type (content_type);
		g_free (content_type);
	}
	if (mime_type)
		return mime_type;
	return g_strdup ("application/octet-stream");
#endif
}

/**
 * go_mime_type_get_description:
 * @mime_type: the mime type to describe.
 *
 * Returns: (transfer full): the description for the mime type. If the
 * description is not found, a copy of the mime type itself will be
 * returned.
**/
gchar *
go_mime_type_get_description (gchar const *mime_type)
{
	char *content_type = g_content_type_from_mime_type (mime_type);
	char *description = NULL;
	if (content_type) {
		description = g_content_type_get_description (content_type);
		g_free (content_type);
	}
	return description ? description : g_strdup (mime_type);
}

/* ------------------------------------------------------------------------- */

#ifdef G_OS_WIN32
static gchar **saved_args;
#endif

/**
 * go_shell_argv_to_glib_encoding:
 * @argc: arguments number
 * @argv: command line arguments
 *
 * Transform arguments to UTF-8 when needed.
 * Returns: (transfer none): the arguments in UTF-8 locale.
 **/
gchar const **
go_shell_argv_to_glib_encoding (gint argc, gchar const **argv)
{
#ifdef G_OS_WIN32
	gchar **args;

	g_return_val_if_fail (saved_args == NULL, argv);

	args = g_new (gchar *, argc + 1);
	args[argc] = NULL;

	if (G_WIN32_IS_NT_BASED ()) {
		LPWSTR *wargs;
		gint i, narg;
		GIConv conv;

		wargs = CommandLineToArgvW (GetCommandLineW (), &narg);
		conv = g_iconv_open ("utf-8", "utf-16le");
		for (i = 0; i < narg; ++i)
			args[i] = g_convert_with_iconv ((const gchar *)wargs[i],
							wcslen (wargs[i]) << 1,
							conv,
							NULL, NULL, NULL);
		g_iconv_close (conv);
	} else {
		int i;
		for (i = 0; i < argc; ++i)
			args[i] = g_locale_to_utf8 (argv[i], -1, NULL, NULL, NULL);
	}

	saved_args = args;

	return (gchar const **) args;
#else
	return argv;
#endif
}

void
go_shell_argv_to_glib_encoding_free (void)
{
#ifdef G_OS_WIN32
	/* What if someone -- like the option parser -- has taken elements
	   out of the array?  */
	g_strfreev (saved_args);
#endif
}

gint
go_file_access (char const *uri, gint mode)
{
	gint ret;
	gchar *filename;

	filename = go_filename_from_uri (uri);
	if (!filename)
		return -1;

#ifdef G_OS_WIN32
	/* FIXME FIXME FIXME Use Security API instead of checking file attributes only on NT-based environment */
#endif

	ret = g_access (filename, mode);

	g_free (filename);

	return ret;
}
