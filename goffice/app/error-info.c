/*
 * error-info.c: GOErrorInfo structure.
 *
 * Author:
 *   Zbigniew Chyla (cyba@gnome.pl)
 *
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

#include <goffice/goffice-config.h>
#include <goffice/app/error-info.h>

#include <stdio.h>
#include <errno.h>

/**
 * GOSeverity:
 * @GO_WARNING: warning.
 * @GO_ERROR: error.
**/

struct _GOErrorInfo {
	gchar *msg;
	GOSeverity severity;
	GSList *details;          /* list of GOErrorInfo */
	unsigned ref_count;
};

/**
 * go_error_info_new_str:
 * @msg: (nullable): error message
 *
 * Creates a new #GOErrorInfo with severity %GO_ERROR and no details.
 *
 * Returns: (transfer full): the newly created #GOErrorInfo
 **/
GOErrorInfo *
go_error_info_new_str (char const *msg)
{
	GOErrorInfo *error = g_new (GOErrorInfo, 1);
	error->msg = g_strdup (msg);
	error->severity = GO_ERROR;
	error->details  = NULL;
	error->ref_count = 1;
	return error;
}

/**
 * go_error_info_new_vprintf:
 * @severity: severity of the error, either %GO_WARNING or %GO_ERROR
 * @msg_format: printf-style format string for the error message
 * @args: arguments for @msg_format
 *
 * Creates a new #GOErrorInfo with the given @severity and a message built
 * from @msg_format and @args.
 *
 * Returns: (transfer full): the newly created #GOErrorInfo
 **/
GOErrorInfo *
go_error_info_new_vprintf (GOSeverity severity, char const *msg_format,
			va_list args)
{
	GOErrorInfo *error;

	g_return_val_if_fail (severity >= GO_WARNING, NULL);
	g_return_val_if_fail (severity <= GO_ERROR, NULL);

	error = g_new (GOErrorInfo, 1);
	error->msg = g_strdup_vprintf (msg_format, args);
	error->severity = severity;
	error->details = NULL;
	error->ref_count = 1;
	return error;
}


/**
 * go_error_info_new_printf:
 * @msg_format: printf-style format string for the error message
 * @...: arguments for @msg_format
 *
 * Creates a new #GOErrorInfo with severity %GO_ERROR and a message built
 * from @msg_format and the provided arguments.
 *
 * Returns: (transfer full): the newly created #GOErrorInfo
 **/
GOErrorInfo *
go_error_info_new_printf (char const *msg_format, ...)
{
	GOErrorInfo *error;
	va_list args;

	va_start (args, msg_format);
	error = go_error_info_new_vprintf (GO_ERROR, msg_format, args);
	va_end (args);

	return error;
}

/**
 * go_error_info_new_str_with_details:
 * @msg: (nullable): error message
 * @details: (transfer full) (nullable): #GOErrorInfo to add
 *
 * Creates a new #GOErrorInfo from @msg and an existing #GOErrorInfo
 * instance to add to the message.
 *
 * Returns: (transfer full): the newly created #GOErrorInfo
 **/
GOErrorInfo *
go_error_info_new_str_with_details (char const *msg, GOErrorInfo *details)
{
	GOErrorInfo *error = go_error_info_new_str (msg);
	go_error_info_add_details (error, details);
	return error;
}

/**
 * go_error_info_new_str_with_details_list:
 * @msg: (nullable): error message
 * @details: (element-type GOErrorInfo) (transfer full) (nullable): a list of
 *           #GOErrorInfo to add
 *
 * Creates a new #GOErrorInfo from @msg and a list of existing #GOErrorInfo
 * instances to add to the message.
 *
 * Returns: (transfer full): the newly created #GOErrorInfo
 **/
GOErrorInfo *
go_error_info_new_str_with_details_list (char const *msg, GSList *details)
{
	GOErrorInfo *error = go_error_info_new_str (msg);
	go_error_info_add_details_list (error, details);
	return error;
}

/**
 * go_error_info_new_from_error_list:
 * @errors: (element-type GOErrorInfo) (transfer full) (nullable): a list of
 *          #GOErrorInfo
 *
 * Creates a new #GOErrorInfo from @errors. If @errors contains a single
 * error, that error is returned directly; otherwise the errors are attached
 * as details to a new error with no message.
 *
 * Returns: (transfer full) (nullable): the new #GOErrorInfo, or %NULL if
 * @errors is empty
 **/
GOErrorInfo *
go_error_info_new_from_error_list (GSList *errors)
{
	GOErrorInfo *error;

	switch (g_slist_length (errors)) {
	case 0:
		error = NULL;
		break;
	case 1:
		error = errors->data;
		g_slist_free (errors);
		break;
	default:
		error = go_error_info_new_str_with_details_list (NULL, errors);
		break;
	}

	return error;
}

/**
 * go_error_info_new_from_errno:
 *
 * Creates a new #GOErrorInfo with severity %GO_ERROR and a message
 * describing the current value of `errno` (see g_strerror()).
 *
 * Returns: (transfer full): the newly created #GOErrorInfo
 **/
GOErrorInfo *
go_error_info_new_from_errno (void)
{
	return go_error_info_new_str (g_strerror (errno));
}

static GOErrorInfo *
go_error_info_deep_copy (GOErrorInfo const *error)
{
	GOErrorInfo *copy;

	g_return_val_if_fail (error != NULL, NULL);

	copy = go_error_info_new_str (error->msg);
	copy->severity = error->severity;
	copy->details = g_slist_copy_deep (error->details,
					   (GCopyFunc) go_error_info_deep_copy,
					   NULL);

	return copy;
}

static GOErrorInfo *
go_error_info_unshare (GOErrorInfo *error)
{
	GOErrorInfo *copy;

	if (error == NULL || error->ref_count == 1)
		return error;

	copy = go_error_info_deep_copy (error);
	go_error_info_free (error);
	return copy;
}

/**
 * go_error_info_add_details:
 * @error: #GOErrorInfo
 * @details: (transfer full) (nullable): #GOErrorInfo to add
 *
 * Takes ownership of @details and adds it to the details of @error. If
 * @details has no message, its details are merged into @error and the empty
 * container is released. Does nothing if @details is %NULL.
 **/
void
go_error_info_add_details (GOErrorInfo *error, GOErrorInfo *details)
{
	g_return_if_fail (error != NULL);

	details = go_error_info_unshare (details);
	if (details == NULL)
		;
	else if (details->msg == NULL) {
		GSList *details_list = details->details;
		details->details = NULL;
		error->details = g_slist_concat (error->details, details_list);
		go_error_info_free (details);
	} else
		error->details = g_slist_append (error->details, details);
}

/**
 * go_error_info_add_details_list:
 * @error: #GOErrorInfo
 * @details: (element-type GOErrorInfo) (transfer full) (nullable): a list of
 *           #GOErrorInfo to add
 *
 * Takes ownership of @details and all its elements, adding them to the
 * details of @error. Empty containers in @details are replaced by their own
 * details. Does nothing if @details is %NULL.
 **/
void
go_error_info_add_details_list (GOErrorInfo *error, GSList *details)
{
	GSList *new_details_list, *l, *ll;

	g_return_if_fail (error != NULL);

	new_details_list = NULL;
	for (l = details; l != NULL; l = l->next) {
		GOErrorInfo *details_error = go_error_info_unshare (l->data);
		if (details_error == NULL)
			continue;
		if (details_error->msg == NULL) {
			GSList *details_list = details_error->details;
			details_error->details = NULL;
			for (ll = details_list; ll != NULL; ll = ll->next)
				new_details_list = g_slist_prepend (new_details_list, ll->data);
			g_slist_free (details_list);
			go_error_info_free (details_error);
		} else
			new_details_list = g_slist_prepend (new_details_list, details_error);
	}
	g_slist_free (details);
	new_details_list = g_slist_reverse (new_details_list);
	error->details = g_slist_concat (error->details, new_details_list);
}

/**
 * go_error_info_free:
 * @error: (nullable): #GOErrorInfo to free
 *
 * Releases a reference on @error. When the last reference is released, the
 * message and all attached details are freed. Does nothing if @error is %NULL.
 **/
void
go_error_info_free (GOErrorInfo *error)
{
	GSList *l;

	if (error == NULL || error->ref_count-- > 1)
		return;

	g_free (error->msg);

	for (l = error->details; l != NULL; l = l->next) {
		GOErrorInfo *ei = l->data;
		go_error_info_free (ei);
	}
	g_slist_free (error->details);

	g_free(error);
}

static void
go_error_info_print_with_offset (GOErrorInfo *error, gint offset)
{
	GSList *l;

	g_return_if_fail (error != NULL);

	if (error->msg != NULL) {
		char c = 'E';

		if (error->severity == GO_WARNING)
			c = 'W';
		g_printerr ("%*s%c %s\n", offset, "", c, error->msg);
		offset += 2;
	}
	for (l = error->details; l != NULL; l = l->next) {
		GOErrorInfo *ei = l->data;
		go_error_info_print_with_offset (ei, offset);
	}
}

/**
 * go_error_info_print:
 * @error: #GOErrorInfo to print
 *
 * Prints @error and all its details to standard error. Messages are prefixed
 * with `E` or `W` according to their severity, and details are indented
 * below their parent.
 **/
void
go_error_info_print (GOErrorInfo *error)
{
	g_return_if_fail (error != NULL);

	go_error_info_print_with_offset (error, 0);
}

/**
 * go_error_info_peek_message:
 * @error: #GOErrorInfo
 *
 * Returns: (nullable) (transfer none): the message in @error, or %NULL for an
 * error that only contains details
 **/
char const *
go_error_info_peek_message (GOErrorInfo *error)
{
	g_return_val_if_fail (error != NULL, NULL);

	return error->msg;
}

/**
 * go_error_info_peek_details:
 * @error: #GOErrorInfo
 *
 * Returns: (element-type GOErrorInfo) (transfer none) (nullable): the details
 * in @error, or %NULL if there are none
 **/
GSList *
go_error_info_peek_details (GOErrorInfo *error)
{
	g_return_val_if_fail (error != NULL, NULL);

	return error->details;
}

/**
 * go_error_info_peek_severity:
 * @error: #GOErrorInfo
 *
 * Returns: the severity of @error
 **/
GOSeverity
go_error_info_peek_severity (GOErrorInfo *error)
{
	g_return_val_if_fail (error != NULL, GO_ERROR);

	return error->severity;
}

static GOErrorInfo *
go_error_info_ref (GOErrorInfo *error)
{
	error->ref_count++;
	return error;
}

/**
 * go_error_info_get_type:
 *
 * Returns: the #GType for #GOErrorInfo
 **/
GType
go_error_info_get_type (void)
{
	static GType t = 0;

	if (t == 0) {
		t = g_boxed_type_register_static ("GOErrorInfo",
			 (GBoxedCopyFunc)go_error_info_ref,
			 (GBoxedFreeFunc)go_error_info_free);
	}
	return t;
}
