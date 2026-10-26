/*
 * go-cmd-context.c : Error dispatch utilities.
 *
 * Author:
 * 	Jody Goldberg <jody@gnome.org>
 *
 * (C) 1999-2004 Jody Goldberg
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
#include "go-cmd-context-impl.h"
#include <goffice/app/goffice-app.h>
#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

/**
 * GOCmdContextClass:
 * @base: base interface.
 * @get_password: retrieves a password.
 * @set_sensitive: sets sensitivity.
 * @progress_set: sets the progress value.
 * @progress_message_set: sets the ptogress message.
 **/

#define GCC_CLASS(o) (G_TYPE_INSTANCE_GET_INTERFACE ((o), GO_TYPE_CMD_CONTEXT, GOCmdContextClass))

static GError *
format_message (GQuark id, char const *message)
{
	char const *msg = message ? message : "";
	return g_error_new_literal (id, 0, msg);
}

/**
 * go_cmd_context_error:
 * @context: a #GOCmdContext
 * @err: (transfer full): a #GError
 *
 * Report @err to @context.
 **/
void
go_cmd_context_error (GOCmdContext *context, GError *err)
{
	g_return_if_fail (GO_IS_CMD_CONTEXT (context));
	GCC_CLASS (context)->error.error (context, err);
}

/**
 * go_cmd_context_error_info:
 * @context: a #GOCmdContext
 * @stack: (transfer none) (nullable): a #GOErrorInfo
 *
 * Report @stack to @context.
 **/
void
go_cmd_context_error_info (GOCmdContext *context, GOErrorInfo *stack)
{
	g_return_if_fail (GO_IS_CMD_CONTEXT (context));
	GCC_CLASS (context)->error.error_info (context, stack);
}

/**
 * go_cmd_context_error_info_list:
 * @context: a #GOCmdContext
 * @stack: (element-type GOErrorInfo) (transfer none) (nullable): a list of #GOErrorInfo
 *
 * Sends a list of errors to the command context.
 **/
void
go_cmd_context_error_info_list (GOCmdContext *context, GSList *stack)
{
	g_return_if_fail (GO_IS_CMD_CONTEXT (context));
	GCC_CLASS (context)->error.error_info_list (context, stack);
}

/**
 * go_cmd_context_error_system:
 * @context: a #GOCmdContext
 * @msg: (nullable): an error message
 *
 * Report a system error to @context.
 **/
void
go_cmd_context_error_system (GOCmdContext *context, char const *msg)
{
	g_return_if_fail (GO_IS_CMD_CONTEXT (context));

	GError *err = format_message (go_error_system (), msg);
	go_cmd_context_error (context, err);
	g_error_free (err);
}

/**
 * go_cmd_context_error_import:
 * @context: a #GOCmdContext
 * @msg: (nullable): an error message
 *
 * Report an import error to @context.
 **/
void
go_cmd_context_error_import (GOCmdContext *context, char const *msg)
{
	g_return_if_fail (GO_IS_CMD_CONTEXT (context));

	GError *err = format_message (go_error_import (), msg);
	go_cmd_context_error (context, err);
	g_error_free (err);
}

/**
 * go_cmd_context_error_export:
 * @context: a #GOCmdContext
 * @msg: (nullable): an error message
 *
 * Report an export error to @context.
 **/
void
go_cmd_context_error_export (GOCmdContext *context, char const *msg)
{
	g_return_if_fail (GO_IS_CMD_CONTEXT (context));

	GError *err = format_message (go_error_export (), msg);
	go_cmd_context_error (context, err);
	g_error_free (err);
}

/**
 * go_cmd_context_error_invalid:
 * @context: a #GOCmdContext
 * @msg: the name of the invalid item
 * @val: the invalid value
 *
 * Report an invalid value error to @context.
 **/
void
go_cmd_context_error_invalid (GOCmdContext *context, char const *msg, char const *val)
{
	g_return_if_fail (GO_IS_CMD_CONTEXT (context));

	GError *err = g_error_new (go_error_invalid(), 0, _("Invalid %s: '%s'"), msg, val);
	go_cmd_context_error (context, err);
	g_error_free (err);
}

/**
 * go_error_system:
 *
 * Returns: #GQuark for system errors.
 **/
GQuark
go_error_system (void)
{
	static GQuark quark;
	if (!quark)
		quark = g_quark_from_static_string ("go_error_system");
	return quark;
}

/**
 * go_error_import:
 *
 * Returns: #GQuark for import errors.
 **/
GQuark
go_error_import (void)
{
	static GQuark quark;
	if (!quark)
		quark = g_quark_from_static_string ("go_error_import");
	return quark;
}

/**
 * go_error_export:
 *
 * Returns: #GQuark for export errors.
 **/
GQuark
go_error_export (void)
{
	static GQuark quark;
	if (!quark)
		quark = g_quark_from_static_string ("go_error_export");
	return quark;
}

/**
 * go_error_invalid:
 *
 * Returns: #GQuark for invalid value errors.
 **/
GQuark
go_error_invalid (void)
{
	static GQuark quark;
	if (!quark)
		quark = g_quark_from_static_string ("go_error_invalid");
	return quark;
}

/**
 * go_cmd_context_progress_set:
 * @context: a #GOCmdContext
 * @f: progress fraction between 0 and 1
 *
 * Update the progress indicator for @context to @f.
 **/
void
go_cmd_context_progress_set (GOCmdContext *context, double f)
{
	g_return_if_fail (GO_IS_CMD_CONTEXT (context));

	if (GCC_CLASS (context)->progress_set)
		GCC_CLASS (context)->progress_set (context, f);
}

/**
 * go_cmd_context_progress_message_set:
 * @context: a #GOCmdContext
 * @msg: (nullable): progress message
 *
 * Set the progress message for @context.
 **/
void
go_cmd_context_progress_message_set (GOCmdContext *context, gchar const *msg)
{
	g_return_if_fail (GO_IS_CMD_CONTEXT (context));

	if (msg == NULL)
		msg = " ";

	if (GCC_CLASS (context)->progress_message_set)
		GCC_CLASS (context)->progress_message_set (context, msg);
}

/**
 * go_cmd_context_get_password:
 * @context: a #GOCmdContext
 * @filename: (nullable): the filename for which a password is needed
 *
 * Request a password from @context.
 *
 * Returns: (transfer full) (nullable): a newly allocated password string, or %NULL
 **/
char *
go_cmd_context_get_password (GOCmdContext *context, char const *filename)
{
	g_return_val_if_fail (GO_IS_CMD_CONTEXT (context), NULL);

	return GCC_CLASS (context)->get_password
		? GCC_CLASS (context)->get_password (context, filename)
		: NULL;
}

/**
 * go_cmd_context_set_sensitive:
 * @context: a #GOCmdContext
 * @sensitive: whether the context should be sensitive
 *
 * Set sensitivity of @context.
 **/
void
go_cmd_context_set_sensitive (GOCmdContext *context, gboolean sensitive)
{
	g_return_if_fail (GO_IS_CMD_CONTEXT (context));

	if (GCC_CLASS (context)->set_sensitive)
		GCC_CLASS (context)->set_sensitive (context, sensitive);
}


static void
go_cmd_context_error_info_list_default 	(GOCmdContext *gcc, GSList *errs)
{
	if (errs == NULL)
		go_cmd_context_error_info (gcc, NULL);
	else
		go_cmd_context_error_info (gcc, g_slist_last (errs)->data);

}

static void
go_cmd_context_base_init (GOCmdContextClass *class)
{
	class->error.error_info_list = go_cmd_context_error_info_list_default;
}

GType
go_cmd_context_get_type (void)
{
	static GType go_cmd_context_type = 0;

	if (!go_cmd_context_type) {
		static GTypeInfo const go_cmd_context_info = {
			sizeof (GOCmdContextClass),	/* class_size */
			(GBaseInitFunc) go_cmd_context_base_init, /* base_init */
			NULL,				/* base_finalize */
		};

		go_cmd_context_type = g_type_register_static (G_TYPE_INTERFACE,
			"GOCmdContext", &go_cmd_context_info, 0);
	}

	return go_cmd_context_type;
}
