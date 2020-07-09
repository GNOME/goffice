/*
 * io-context.c : Place holder for an io error context.
 *   It is intended to become a place to handle errors
 *   as well as storing non-fatal warnings.
 *
 * Authors:
 *	Jody Goldberg <jody@gnome.org>
 *	Zbigniew Chyla <cyba@gnome.pl>
 *
 * (C) 2000-2005 Jody Goldberg
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
#include "io-context-priv.h"
#include "go-cmd-context.h"
#include <goffice/utils/go-file.h>
#include <gsf/gsf-impl-utils.h>
#ifdef GOFFICE_WITH_GTK
#include <gtk/gtk.h>
#endif
#include <glib/gi18n-lib.h>

#define PROGRESS_UPDATE_STEP        0.01
#define PROGRESS_UPDATE_STEP_END    (1.0 / 400)
#define PROGRESS_UPDATE_PERIOD_SEC  0.20

#define IOC_CLASS(ioc) GO_IO_CONTEXT_CLASS(G_OBJECT_GET_CLASS(ioc))

enum {
	IOC_PROP_0,
	IOC_PROP_EXEC_LOOP,
};

static void
go_io_context_init (GOIOContext *ioc)
{
	ioc->impl = NULL;
	ioc->info = NULL;
	ioc->error_occurred = FALSE;
	ioc->warning_occurred = FALSE;

	ioc->progress_ranges = NULL;
	ioc->progress_min = 0.0;
	ioc->progress_max = 1.0;
	ioc->last_progress = -1.0;
	ioc->last_time = 0.0;
	ioc->helper.helper_type = GO_PROGRESS_HELPER_NONE;
	ioc->exec_main_loop = TRUE;
}

static void
ioc_finalize (GObject *obj)
{
	GOIOContext *ioc;

	g_return_if_fail (GO_IS_IO_CONTEXT (obj));

	ioc = GO_IO_CONTEXT (obj);
	g_slist_free_full (ioc->info, (GDestroyNotify) go_error_info_free);
	if (ioc->impl) {
		go_cmd_context_progress_set (ioc->impl, 0.0);
		go_cmd_context_progress_message_set (ioc->impl, NULL);
		g_object_unref (ioc->impl);
	}

	g_list_free_full (ioc->progress_ranges, (GFreeFunc)g_free);
	ioc->progress_ranges = NULL;

	G_OBJECT_CLASS (g_type_class_peek (G_TYPE_OBJECT))->finalize (obj);
}

static void
io_context_set_property (GObject *obj, guint param_id,
		       GValue const *value, GParamSpec *pspec)
{
	GOIOContext *ioc = GO_IO_CONTEXT (obj);

	switch (param_id) {
	case IOC_PROP_EXEC_LOOP:
		ioc->exec_main_loop = g_value_get_boolean (value);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
}

static void
io_context_get_property (GObject *obj, guint param_id,
		       GValue *value, GParamSpec *pspec)
{
	GOIOContext *ioc = GO_IO_CONTEXT (obj);

	switch (param_id) {
	case IOC_PROP_EXEC_LOOP:
		g_value_set_boolean (value, ioc->exec_main_loop);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
}

static char *
ioc_get_password (GOCmdContext *cc, char const *filename)
{
	GOIOContext *ioc = (GOIOContext *)cc;
	return go_cmd_context_get_password (ioc->impl, filename);
}

static void
ioc_set_sensitive (GOCmdContext *cc, gboolean sensitive)
{
	(void)cc; (void)sensitive;
}

static void
ioc_error_error (GOCmdContext *cc, GError *err)
{
	go_io_error_string (GO_IO_CONTEXT (cc), err->message);
}

static void
ioc_error_go_error_info_ (G_GNUC_UNUSED GOCmdContext *ctxt,
		      GOErrorInfo *error)
{
	/* TODO what goes here */
	go_error_info_print (error);
}

void
go_io_error_string (GOIOContext *context, const gchar *str)
{
	GOErrorInfo *error;

	g_return_if_fail (context != NULL);
	g_return_if_fail (str != NULL);

	error = go_error_info_new_str (str);
	go_io_error_info_set (context, error);
}

static void
go_io_context_gnm_cmd_context_init (GOCmdContextClass *cc_class)
{
	cc_class->get_password	   = ioc_get_password;
	cc_class->set_sensitive	   = ioc_set_sensitive;
	cc_class->error.error      = ioc_error_error;
	cc_class->error.error_info = ioc_error_go_error_info_;
}

static void
go_io_context_class_init (GObjectClass *klass)
{
	klass->finalize = ioc_finalize;
	klass->set_property = io_context_set_property;
	klass->get_property = io_context_get_property;
	g_object_class_install_property (klass, IOC_PROP_EXEC_LOOP,
		g_param_spec_boolean ("exec-main-loop", _("exec-main-loop"),
			_("Execute main loop iteration"),
			TRUE, G_PARAM_READWRITE));
}

GSF_CLASS_FULL (GOIOContext, go_io_context,
		NULL, NULL, go_io_context_class_init, NULL,
		go_io_context_init, G_TYPE_OBJECT, 0,
		GSF_INTERFACE (go_io_context_gnm_cmd_context_init, GO_TYPE_CMD_CONTEXT))

GOIOContext *
go_io_context_new (GOCmdContext *cc)
{
	GOIOContext *ioc;

	g_return_val_if_fail (GO_IS_CMD_CONTEXT (cc), NULL);

	ioc = g_object_new (GO_TYPE_IO_CONTEXT, NULL);
	/* The cc is optional for subclasses, but mandatory in this class. */
	ioc->impl = cc;
	g_object_ref (ioc->impl);

	return ioc;
}

void
go_io_error_unknown (GOIOContext *context)
{
	g_return_if_fail (context != NULL);

	context->error_occurred = TRUE;
}

void
go_io_error_info_set (GOIOContext *context, GOErrorInfo *error)
{
	g_return_if_fail (context != NULL);
	g_return_if_fail (error != NULL);

	context->info = g_slist_prepend (context->info, error);

	if (go_error_info_peek_severity (error) < GO_ERROR)
		context->warning_occurred = TRUE;
	else
		context->error_occurred = TRUE;
}

void
go_io_error_push (GOIOContext *context, GOErrorInfo *error)
{
	g_return_if_fail (context != NULL);
	g_return_if_fail (error != NULL);

	if (context->info == NULL)
		go_io_error_info_set (context, error);
	else {
		GOErrorInfo *info = context->info->data;
		go_error_info_add_details (error, info);
		context->info->data = error;
	}
}

void
go_io_error_display (GOIOContext *context)
{
	GOCmdContext *cc;

	g_return_if_fail (context != NULL);

	if (context->info != NULL) {
		if (context->impl)
			cc = context->impl;
		else
			cc = GO_CMD_CONTEXT (context);
		go_cmd_context_error_info_list
			(cc, context->info);
	}
}

/* TODO: Rename to go_io_info_clear */
void
go_io_error_clear (GOIOContext *context)
{
	g_return_if_fail (context != NULL);

	context->error_occurred = FALSE;
	context->warning_occurred = FALSE;
	g_slist_free_full (context->info, (GDestroyNotify) go_error_info_free);
	context->info = NULL;
}

gboolean
go_io_error_occurred (GOIOContext *context)
{
	return context->error_occurred;
}

gboolean
go_io_warning_occurred (GOIOContext *context)
{
	return context->warning_occurred;
}

void
go_io_progress_update (GOIOContext *ioc, gdouble f)
{
	gboolean at_end;

	g_return_if_fail (GO_IS_IO_CONTEXT (ioc));

	if (ioc->progress_ranges != NULL) {
		f = f * (ioc->progress_max - ioc->progress_min)
		    + ioc->progress_min;
	}

	at_end = (f - ioc->last_progress > PROGRESS_UPDATE_STEP_END &&
		  f + PROGRESS_UPDATE_STEP > 1);
	/* The use of fabs here means we can set progress back if we need to. */
	if (at_end || fabs (f - ioc->last_progress) >= PROGRESS_UPDATE_STEP) {
		double t = g_get_monotonic_time () / 1000000.0;

		if (at_end || t - ioc->last_time >= PROGRESS_UPDATE_PERIOD_SEC) {
			GOCmdContext *cc;

			if (ioc->impl)
				cc = ioc->impl;
			else
				cc = GO_CMD_CONTEXT (ioc);
			go_cmd_context_progress_set (cc, f);
			ioc->last_time = t;
			ioc->last_progress = f;
		}
	}

#ifdef GOFFICE_WITH_GTK
	/* FIXME : abstract this into the workbook control */
	if (ioc->exec_main_loop)
		while (gtk_events_pending ())
			gtk_main_iteration_do (FALSE);
#endif
}

void
go_io_progress_message (GOIOContext *ioc, const gchar *msg)
{
	GOCmdContext *cc;

	g_return_if_fail (GO_IS_IO_CONTEXT (ioc));

	if (ioc->impl)
		cc = ioc->impl;
	else
		cc = GO_CMD_CONTEXT (ioc);
	go_cmd_context_progress_message_set (cc, msg);
}

void
go_io_progress_range_push (GOIOContext *ioc, gdouble min, gdouble max)
{
	GOProgressRange *r;
	gdouble new_min, new_max;

	g_return_if_fail (GO_IS_IO_CONTEXT (ioc));

	r = g_new (GOProgressRange, 1);
	r->min = min;
	r->max = max;
	ioc->progress_ranges = g_list_append (ioc->progress_ranges, r);

	new_min = min / (ioc->progress_max - ioc->progress_min)
	          + ioc->progress_min;
	new_max = max / (ioc->progress_max - ioc->progress_min)
	          + ioc->progress_min;
	ioc->progress_min = new_min;
	ioc->progress_max = new_max;
}

void
go_io_progress_range_pop (GOIOContext *ioc)
{
	GList *l;

	g_return_if_fail (GO_IS_IO_CONTEXT (ioc));
	g_return_if_fail (ioc->progress_ranges != NULL);

	l = g_list_last (ioc->progress_ranges);
	ioc->progress_ranges= g_list_remove_link (ioc->progress_ranges, l);
	g_free (l->data);
	g_list_free_1 (l);

	ioc->progress_min = 0.0;
	ioc->progress_max = 1.0;
	for (l = ioc->progress_ranges; l != NULL; l = l->next) {
		GOProgressRange *r = l->data;
		gdouble new_min, new_max;

		new_min = r->min / (ioc->progress_max - ioc->progress_min)
		          + ioc->progress_min;
		new_max = r->max / (ioc->progress_max - ioc->progress_min)
		          + ioc->progress_min;
		ioc->progress_min = new_min;
		ioc->progress_max = new_max;
	}
}

void
go_io_value_progress_set (GOIOContext *ioc, gint total, gint step)
{
	g_return_if_fail (GO_IS_IO_CONTEXT (ioc));
	g_return_if_fail (total >= 0);

	ioc->helper.helper_type = GO_PROGRESS_HELPER_VALUE;
	ioc->helper.v.value.total = MAX (total, 1);
	ioc->helper.v.value.last = -step;
	ioc->helper.v.value.step = step;
}

void
go_io_value_progress_update (GOIOContext *ioc, gint value)
{
	gdouble complete;
	gint step, total;

	g_return_if_fail (GO_IS_IO_CONTEXT (ioc));
	g_return_if_fail (ioc->helper.helper_type == GO_PROGRESS_HELPER_VALUE);

	total = ioc->helper.v.value.total;
	step = ioc->helper.v.value.step;

	if (value - ioc->helper.v.value.last < step &&
	    value + step < total) {
		return;
	}
	ioc->helper.v.value.last = value;

	complete = (gdouble)value / total;
	go_io_progress_update (ioc, complete);
}

void
go_io_count_progress_set (GOIOContext *ioc, gint total, gint step)
{
	g_return_if_fail (GO_IS_IO_CONTEXT (ioc));
	g_return_if_fail (total >= 0);

	ioc->helper.helper_type = GO_PROGRESS_HELPER_COUNT;
	ioc->helper.v.count.total = MAX (total, 1);
	ioc->helper.v.count.last = -step;
	ioc->helper.v.count.current = 0;
	ioc->helper.v.count.step = step;
}

void
go_io_count_progress_update (GOIOContext *ioc, gint inc)
{
	gdouble complete;
	gint current, step, total;

	g_return_if_fail (GO_IS_IO_CONTEXT (ioc));
	g_return_if_fail (ioc->helper.helper_type == GO_PROGRESS_HELPER_COUNT);

	current = (ioc->helper.v.count.current += inc);
	step = ioc->helper.v.count.step;
	total = ioc->helper.v.count.total;

	if (current - ioc->helper.v.count.last < step && current + step < total) {
		return;
	}
	ioc->helper.v.count.last = current;

	complete = (gdouble)current / total;
	go_io_progress_update (ioc, complete);
}

void
go_io_progress_unset (GOIOContext *ioc)
{
	g_return_if_fail (GO_IS_IO_CONTEXT (ioc));

	ioc->helper.helper_type = GO_PROGRESS_HELPER_NONE;
}

void
go_io_context_set_num_files (GOIOContext *ioc, guint count)
{
	GOIOContextClass *klass = IOC_CLASS(ioc);
	g_return_if_fail (klass != NULL);
	if (klass->set_num_files != NULL)
		klass->set_num_files (ioc, count);
}

/**
 * go_io_context_processing_file:
 * @ioc: #GOIOContext
 * @uri: An escaped uri (eg "foo\%20bar")
 **/
void
go_io_context_processing_file (GOIOContext *ioc, char const *uri)
{
	char *basename;
	GOIOContextClass *klass = IOC_CLASS(ioc);

	g_return_if_fail (klass != NULL);

	basename = go_basename_from_uri (uri); /* unescape the uri */
	if (basename != NULL && klass->processing_file != NULL)
		klass->processing_file (ioc, basename);
	g_free (basename);
}

void
go_io_warning (G_GNUC_UNUSED GOIOContext *context,
		char const *fmt, ...)
{
	va_list args;

	va_start (args, fmt);
	go_io_warning_varargs (context, fmt, args);
	va_end (args);
}

void
go_io_warning_varargs (GOIOContext *context, char const *fmt, va_list args)
{
	context->info = g_slist_prepend
		(context->info, go_error_info_new_vprintf
		 (GO_WARNING, fmt, args));
	context->warning_occurred = TRUE;
}

void
go_io_warning_unknown_font (GOIOContext *context,
			     G_GNUC_UNUSED char const *font_name)
{
	g_return_if_fail (GO_IS_IO_CONTEXT (context));
}

void
go_io_warning_unknown_function	(GOIOContext *context,
				 G_GNUC_UNUSED char const *funct_name)
{
	g_return_if_fail (GO_IS_IO_CONTEXT (context));
}

void
go_io_warning_unsupported_feature (GOIOContext *context, char const *feature)
{
	g_return_if_fail (GO_IS_IO_CONTEXT (context));
	g_warning ("%s : are not supported yet", feature);
}
