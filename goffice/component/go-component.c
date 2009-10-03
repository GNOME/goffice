/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-component.c :
 *
 * Copyright (C) 2005 Jean Brefort (jean.brefort@normalesup.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
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
#include <goffice/component/goffice-component.h>
#include <goffice/component/go-component.h>

#include <gsf/gsf-impl-utils.h>

/**
 * GOComponentClass:
 * @parent_class: the parent object class.
 * @edit: callback for component edition.
 * @get_data: returns the data embedded in the component.
 * @mime_type_set: sets the mime type.
 * @set_data: sets the data embedded in the component.
 * @set_default_size: sets the default size for the component.
 * @set_size: sets the requested size.
 * @set_window: sets the window if the component uses a #GdkWindow. The new
 * window is stored in the @window field of #GOComponent.
 * @render: displays the contents.
 * @changed: callback called when the component contents changed.
 *
 * The component base object class.
 **/

#define GO_COMPONENT_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GO_TYPE_COMPONENT, GOComponentClass))

enum {
	COMPONENT_PROP_0,
	COMPONENT_PROP_MIME_TYPE,
	COMPONENT_PROP_WIDTH,
	COMPONENT_PROP_ASCENT,
	COMPONENT_PROP_DESCENT,
	COMPONENT_PROP_HEIGHT,
};

enum {
	CHANGED,
	LAST_SIGNAL
};
static gulong go_component_signals [LAST_SIGNAL] = { 0, };

static GObjectClass *component_parent_klass;

static void go_component_set_property (GObject *obj, guint param_id,
		       GValue const *value, GParamSpec *pspec)
{
	GOComponent *component = GO_COMPONENT (obj);
	char const *str;

	switch (param_id) {
	case COMPONENT_PROP_MIME_TYPE:
		g_free (component->mime_type);
		str = g_value_get_string (value);
		if (str == NULL)
			break;
		component->mime_type = g_strdup (str);
		if (GO_COMPONENT_GET_CLASS (obj)->mime_type_set != NULL)
			GO_COMPONENT_GET_CLASS (obj)->mime_type_set (component);
		break;
	case COMPONENT_PROP_WIDTH:
		component->width = g_value_get_double (value);
		break;
	case COMPONENT_PROP_ASCENT:
		component->ascent = g_value_get_double (value);
		break;
	case COMPONENT_PROP_DESCENT:
		component->descent = g_value_get_double (value);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
}

static void
go_component_get_property (GObject *obj, guint param_id,
		       GValue *value, GParamSpec *pspec)
{
	GOComponent *component = GO_COMPONENT (obj);

	switch (param_id) {
	case COMPONENT_PROP_MIME_TYPE:
		g_value_set_string (value, component->mime_type);
		break;
	case COMPONENT_PROP_WIDTH:
		g_value_set_double (value, component->width);
		break;
	case COMPONENT_PROP_ASCENT:
		g_value_set_double (value, component->ascent);
		break;
	case COMPONENT_PROP_DESCENT:
		g_value_set_double (value, component->descent);
		break;
	case COMPONENT_PROP_HEIGHT:
		g_value_set_double (value, component->ascent + component->descent);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
go_component_finalize (GObject *obj)
{
	GOComponent *component = GO_COMPONENT (obj);

	g_free (component->mime_type);

	(*component_parent_klass->finalize) (obj);
}

static void
go_component_class_init (GOComponentClass *klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) klass;

	component_parent_klass = g_type_class_peek_parent (klass);
	gobject_klass->finalize		= go_component_finalize;
	gobject_klass->set_property	= go_component_set_property;
	gobject_klass->get_property	= go_component_get_property;

	g_object_class_install_property (gobject_klass, COMPONENT_PROP_MIME_TYPE,
		g_param_spec_string ("mime-type", "mime-type", "mime type of the content of the component",
			NULL, G_PARAM_READWRITE));
	g_object_class_install_property (gobject_klass, COMPONENT_PROP_WIDTH,
		g_param_spec_double ("width", "Width",
			"Component width",
			0.0, G_MAXDOUBLE, 0.,
			G_PARAM_READWRITE));
	g_object_class_install_property (gobject_klass, COMPONENT_PROP_ASCENT,
		g_param_spec_double ("ascent", "Ascent",
			"Component ascent",
			0.0, G_MAXDOUBLE, 0.,
			G_PARAM_READWRITE));
	g_object_class_install_property (gobject_klass, COMPONENT_PROP_DESCENT,
		g_param_spec_double ("descent", "Descent",
			"Component descent",
			0.0, G_MAXDOUBLE, 0.,
			G_PARAM_READWRITE));
	g_object_class_install_property (gobject_klass, COMPONENT_PROP_HEIGHT,
		g_param_spec_double ("height", "Height",
			"Component height",
			0.0, G_MAXDOUBLE, 0.,
			G_PARAM_READABLE));

	go_component_signals [CHANGED] = g_signal_new ("changed",
		G_TYPE_FROM_CLASS (klass),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GOComponentClass, changed),
		NULL, NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
}

static void
go_component_init (GOComponent *component)
{
	component->mime_type = NULL;
	component->needs_window = FALSE;
	component->editable = FALSE;
	component->resizable = FALSE;
	component->window = NULL;
}

GSF_CLASS_ABSTRACT (GOComponent, go_component,
		    go_component_class_init, go_component_init,
		    G_TYPE_OBJECT)

void
go_component_set_default_size (GOComponent *component, double width, double ascent, double descent)
{
	GOComponentClass *klass;

	g_return_if_fail (GO_IS_COMPONENT (component));

	klass = GO_COMPONENT_GET_CLASS(component);
	component->default_width = width;
	if (component->width == 0.)
		component->width = width;
	component->default_ascent = ascent;
	if (component->ascent == 0.)
		component->ascent = ascent;
	component->default_descent = descent;
	if (component->descent == 0.)
		component->descent = descent;
	if (component->height == 0)
		component->height = component->ascent + component->descent;
	if (klass->set_default_size)
		klass->set_default_size (component);
}

/**
 * go_component_needs_window:
 * @component: #GOComponent
 *
 * Returns: TRUE  if the component uses its own #GtkWindow.
 **/

gboolean
go_component_needs_window (GOComponent *component)
{
	g_return_val_if_fail (GO_IS_COMPONENT (component), FALSE);
	return component->needs_window;
}

void
go_component_set_window (GOComponent *component, GdkWindow *window)
{
	GOComponentClass *klass;

	g_return_if_fail (GO_IS_COMPONENT (component));

	klass = GO_COMPONENT_GET_CLASS(component);
	component->window = window;
	if (klass->set_window)
		klass->set_window (component);
}

void
go_component_set_data (GOComponent *component, char const *data, int length)
{
	GOComponentClass *klass;

	g_return_if_fail (GO_IS_COMPONENT (component));

	component->data = data;
	component->length = length;

	klass = GO_COMPONENT_GET_CLASS(component);
	if (klass->set_data)
		klass->set_data (component);
}

gboolean
go_component_get_data (GOComponent *component, gpointer *data, int *length,
		       GDestroyNotify *clearfunc, gpointer *user_data)
{
	GOComponentClass *klass;

	g_return_val_if_fail (GO_IS_COMPONENT (component), FALSE);

	klass = GO_COMPONENT_GET_CLASS(component);
	if (klass->get_data)
		return klass->get_data (component, data, length, clearfunc, user_data);
	return FALSE;
}

void
go_component_set_size (GOComponent *component, double width, double height)
{
	GOComponentClass *klass;

	g_return_if_fail (GO_IS_COMPONENT (component));

	if (!component->resizable)
		return;
	klass = GO_COMPONENT_GET_CLASS(component);
	component->width = width;
	component->height = height;
	if (klass->set_size)
		klass->set_size (component);
}

void
go_component_render (GOComponent *component, cairo_t *cr,
			double width, double height)
{
	GOComponentClass *klass;

	g_return_if_fail (GO_IS_COMPONENT (component));

	klass = GO_COMPONENT_GET_CLASS(component);
	if (klass->render)
		klass->render (component, cr, width, height);
}

gboolean
go_component_is_resizable (GOComponent *component)
{
	g_return_val_if_fail (GO_IS_COMPONENT (component), FALSE);
	return component->resizable;
}

gboolean
go_component_is_editable (GOComponent *component)
{
	g_return_val_if_fail (GO_IS_COMPONENT (component), FALSE);
	return component->editable;
}

GtkWindow *go_component_edit (GOComponent *component)
{
	GOComponentClass *klass;

	g_return_val_if_fail (GO_IS_COMPONENT (component), NULL);

	klass = GO_COMPONENT_GET_CLASS(component);
	if (component->editable && klass->edit)
		return klass->edit (component);
	return NULL;
}

void
go_component_emit_changed (GOComponent *component)
{
	g_return_if_fail (GO_IS_COMPONENT (component));

	g_signal_emit (G_OBJECT (component),
		go_component_signals [CHANGED], 0);
}

static GOCmdContext *goc_cc;

void
go_component_set_command_context (GOCmdContext *cc)
{
	if (goc_cc)
		g_object_unref (goc_cc);
	goc_cc = cc;
	if (goc_cc)
		g_object_ref (goc_cc);
}

GOCmdContext *
go_component_get_command_context (void)
{
	return goc_cc;
}
