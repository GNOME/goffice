/*
 * go-component.c :
 *
 * Copyright (C) 2005-2010 Jean Brefort (jean.brefort@normalesup.org)
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
#include <goffice/component/goffice-component.h>
#include <goffice/component/go-component.h>
#include <gsf/gsf-libxml.h>
#include <gsf/gsf-impl-utils.h>
#include <gsf/gsf-input.h>
#include <gsf/gsf-output-memory.h>
#include <gio/gio.h>
#include <cairo-svg.h>
#include <string.h>

#ifdef GOFFICE_WITH_LIBRSVG
#include <librsvg/rsvg.h>
#ifdef LIBRSVG_CHECK_VERSION
#define NEEDS_LIBRSVG_CAIRO_H !LIBRSVG_CHECK_VERSION(2,36,2)
#else
#define NEEDS_LIBRSVG_CAIRO_H 1
#endif
#if NEEDS_LIBRSVG_CAIRO_H
#include <librsvg/rsvg-cairo.h>
#endif
#endif /* GOFFICE_WITH_LIBRSVG */

struct _GOComponentPrivate {
	gboolean is_inline; /* if set, the object will be displayed in compact mode
	 					if meaningful for the component (e.g. equations) */
	gboolean use_font_from_app; /* if set, the font is set by the calling
	 					application */
};

/**
 * GOSnapshotType:
 * @GO_SNAPSHOT_NONE: no snapshot.
 * @GO_SNAPSHOT_SVG: svg snapshot.
 * @GO_SNAPSHOT_PNG: png snapshot.
 **/

static struct {
	GOSnapshotType type;
	char const *name;
} snapshot_types[GO_SNAPSHOT_MAX] = {
	{ GO_SNAPSHOT_NONE, "none"},
	{ GO_SNAPSHOT_SVG, "svg"},
	{ GO_SNAPSHOT_PNG, "png"}
};

static GOSnapshotType
go_snapshot_type_from_string (char const *name)
{
	unsigned i;
	GOSnapshotType ret = GO_SNAPSHOT_NONE;

	for (i = 0; i < GO_SNAPSHOT_MAX; i++) {
		if (strcmp (snapshot_types[i].name, name) == 0) {
			ret = snapshot_types[i].type;
			break;
		}
	}
	return ret;
}

/**
 * GOComponentClass:
 * @parent_class: the parent object class.
 * @edit: callback for component edition.
 * @get_data: returns the data embedded in the component.
 * @mime_type_set: sets the mime type.
 * @set_data: sets the data embedded in the component.
 * @set_default_size: sets the default size for the component.
 * @set_size: sets the requested size.
 * @render: displays the contents.
 * @set_font: sets the default font for the component.
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
	COMPONENT_PROP_INLINE
};

enum {
	CHANGED,
	LAST_SIGNAL
};
static gulong go_component_signals [LAST_SIGNAL] = { 0, };

enum {
	GO_COMPONENT_SNAPSHOT_NONE,
	GO_COMPONENT_SNAPSHOT_SVG,
	GO_COMPONENT_SNAPSHOT_PNG,
};

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
	case COMPONENT_PROP_INLINE:
		component->priv->is_inline = g_value_get_boolean (value);
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
	case COMPONENT_PROP_INLINE:
		g_value_set_boolean (value, component->priv->is_inline);
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

	if (component->destroy_notify != NULL) {
		component->destroy_notify ((component->destroy_data)?
									component->destroy_data:
									(void *) component->data);
		component->destroy_notify = NULL;
	}
	if (component->cc) {
		g_object_unref (component->cc);
		component->cc = NULL;
	}
	g_free (component->priv);

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
			NULL, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)));
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
	g_object_class_install_property (gobject_klass, COMPONENT_PROP_INLINE,
		g_param_spec_boolean ("inline", "Inline",
			"Whether the component should be displayed in-line",
			FALSE, G_PARAM_READWRITE));

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
	component->editable = FALSE;
	component->resizable = FALSE;
	component->priv = g_new0 (GOComponentPrivate, 1);
}

GSF_CLASS_ABSTRACT (GOComponent, go_component,
		    go_component_class_init, go_component_init,
		    G_TYPE_OBJECT)

/******************************************************************************
 * GOComponentSnapshot: component class used when actual class is not         *
 * available, just displays the snapshot                                      *
 ******************************************************************************/

typedef struct {
	GOComponent base;
	gpointer *image;
} GOComponentSnapshot;
typedef GOComponentClass GOComponentSnapshotClass;

GType go_component_snapshot_get_type (void);

static GObjectClass *snapshot_parent_klass;

static void
go_component_snapshot_render (GOComponent *component, cairo_t *cr,
			double width, double height)
{
	GOComponentSnapshot *snapshot = (GOComponentSnapshot *) component;
	switch (component->snapshot_type) {
#ifdef GOFFICE_WITH_LIBRSVG
	case GO_SNAPSHOT_SVG:
		/* NOTE: we might use lasem here, and also use a GOSvg image */
		if (snapshot->image == NULL) {
			GError *err = NULL;
			snapshot->image = (void *) rsvg_handle_new_from_data (
							component->snapshot_data,
							component->snapshot_length,
							&err);
			if (err) {
				g_error_free (err);
				if (snapshot->image)
					g_object_unref (snapshot->image);
				snapshot->image = NULL;
			}
		}
		if (snapshot->image != NULL) {
			RsvgDimensionData dim;
			double scalex = 1., scaley = 1.;
			cairo_save (cr);
			rsvg_handle_get_dimensions (RSVG_HANDLE (snapshot->image), &dim);
			cairo_user_to_device_distance (cr, &scalex, &scaley);
			cairo_scale (cr, width * scalex / dim.width,
						 height * scaley / dim.height);
			rsvg_handle_render_cairo (RSVG_HANDLE (snapshot->image), cr);
			cairo_restore (cr);
		}
		break;
#endif /* GOFFICE_WITH_LIBRSVG */
	case GO_SNAPSHOT_PNG: {
		if (snapshot->image == NULL) {
			GInputStream *in = g_memory_input_stream_new_from_data (
						component->snapshot_data,
						component->snapshot_length,
						NULL);
			GError *err = NULL;
			GdkPixbuf *pixbuf = gdk_pixbuf_new_from_stream (in, NULL, &err);
			if (err) {
				g_error_free (err);
			} else
				snapshot->image = (void *) go_pixbuf_new_from_pixbuf (pixbuf);
			if (pixbuf)
				g_object_unref (pixbuf);

		}
		if (snapshot->image != NULL) {
			int w, h;
			double scalex = 1., scaley = 1.;
			cairo_save (cr);
			g_object_get (snapshot->image, "width", &w, "height", &h, NULL);
			cairo_user_to_device_distance (cr, &scalex, &scaley);
			cairo_scale (cr, width / w / scalex, height / h / scaley);
			cairo_move_to (cr, 0, 0);
			go_image_draw (GO_IMAGE (snapshot->image), cr);
			cairo_restore (cr);
		} else {
			cairo_rectangle (cr, 0, 0, width, height);
			cairo_set_source_rgb (cr, 1., 1., 1.);
			cairo_fill (cr);
		}
		break;
	}
	default:
		break;
	}
}

static void
go_component_shapshot_finalize (GObject *obj)
{
	GOComponentSnapshot *snapshot = (GOComponentSnapshot *) obj;

	if (G_IS_OBJECT (snapshot->image))
		g_object_unref (snapshot->image);

	(*snapshot_parent_klass->finalize) (obj);
}

static void
go_component_snapshot_class_init (GOComponentClass *klass)
{
	snapshot_parent_klass = g_type_class_peek_parent (klass);
	((GObjectClass *) klass)->finalize = go_component_shapshot_finalize;
	klass->render = go_component_snapshot_render;
}

GSF_CLASS (GOComponentSnapshot, go_component_snapshot,
	go_component_snapshot_class_init, NULL,
	GO_TYPE_COMPONENT)

/******************************************************************************/

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

void
go_component_set_data (GOComponent *component, char const *data, int length)
{
	GOComponentClass *klass;

	g_return_if_fail (GO_IS_COMPONENT (component));

	if (component->destroy_notify) {
		component->destroy_notify (component->destroy_data);
		component->destroy_notify = NULL;
		component->destroy_data = NULL;
	}
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
	gboolean res =  FALSE;

	g_return_val_if_fail (GO_IS_COMPONENT (component), FALSE);

	if (component->destroy_notify != NULL) {
		component->destroy_notify (component->destroy_data);
		component->destroy_notify = NULL;
	}

	klass = GO_COMPONENT_GET_CLASS (component);
	if (klass->get_data)
		res = klass->get_data (component, data, length, clearfunc, user_data);
	if (res) {
		component->data = (char const *) *data;
		component->length = *length;
	}
	return res;
}

char const *
go_component_get_mime_type (GOComponent *component)
{
	return component->mime_type;
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
	/* clear the snapshot */
	g_free (component->snapshot_data);
	component->snapshot_data = NULL;
	component->snapshot_length = 0;
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

static void
editor_destroy_cb (GOComponent *component)
{
	component->editor = NULL;
}

/**
 * go_component_edit:
 * @component: #GOComponent
 *
 * Opens a top level window editor for the component if it can be edited.
 * Returns: (transfer none): the editor window or %NULL
 */
GtkWindow *
go_component_edit (GOComponent *component)
{
	GOComponentClass *klass;

	g_return_val_if_fail (GO_IS_COMPONENT (component), NULL);

	if (component->editor)
		return component->editor;
	klass = GO_COMPONENT_GET_CLASS(component);
	if (component->editable && klass->edit) {
		component->editor = klass->edit (component);
		if (component->editor)
			g_signal_connect_swapped (component->editor, "destroy",
			                          G_CALLBACK (editor_destroy_cb),
			                          component);
	}
	return component->editor;
}

void go_component_stop_editing (GOComponent *component)
{
	g_return_if_fail (GO_IS_COMPONENT (component));
	if (component->editor)
		gtk_widget_destroy (GTK_WIDGET (component->editor));
	component->editor = NULL;
}

void
go_component_emit_changed (GOComponent *component)
{
	g_return_if_fail (GO_IS_COMPONENT (component));

	g_free (component->snapshot_data);
	component->snapshot_data = NULL;
	component->snapshot_length = 0;
	/* invalidate the old data if any */
	if (component->destroy_notify != NULL) {
		component->destroy_notify ((component->destroy_data)?
									component->destroy_data:
									(void *) component->data);
		component->destroy_notify = NULL;
		component->destroy_data = NULL;
	}
	component->data = NULL;
	component->length = 0;

	g_signal_emit (G_OBJECT (component),
		go_component_signals [CHANGED], 0);
}

static GOCmdContext *goc_cc = NULL;

void
go_component_set_command_context (GOComponent *component, GOCmdContext *cc)
{
	if (cc == component->cc)
		return;
	if (component->cc)
		g_object_unref (component->cc);
	component->cc = cc;
	if (component->cc)
		g_object_ref (component->cc);
}

/**
 * go_component_get_command_context:
 * @component: #GOComponent
 *
 * Returns: (transfer none): the command context used by the component or the
 * default command context if the argument is NULL.
 */
GOCmdContext *
go_component_get_command_context (GOComponent *component)
{
	return (component && component->cc)? component->cc: goc_cc;
}

void
go_component_set_default_command_context (GOCmdContext *cc)
{
	if (cc == goc_cc)
		return;
	if (goc_cc)
		g_object_unref (goc_cc);
	goc_cc = cc;
	if (goc_cc)
		g_object_ref (goc_cc);
}

void
go_component_get_size (GOComponent *component, double *width, double *height)
{
	*width = component->width;
	if (component->height == 0.)
		component->height = component->ascent + component->descent;
	*height = component->height;
}

void
go_component_write_xml_sax (GOComponent *component, GsfXMLOut *output)
{
	guint i, nbprops;
	GType    prop_type;
	GValue	 value;
	GParamSpec **specs = g_object_class_list_properties (
				G_OBJECT_GET_CLASS (component), &nbprops);

	gsf_xml_out_start_element (output, "GOComponent");
	gsf_xml_out_add_cstr (output, "mime-type", component->mime_type);
	gsf_xml_out_add_float (output, "width", component->width, 3);
	gsf_xml_out_add_float (output, "height", component->height, 3);
	/* save needed component specific properties */
	for (i = 0; i < nbprops; i++)
		if (specs[i]->flags & GO_PARAM_PERSISTENT) {
			prop_type = G_PARAM_SPEC_VALUE_TYPE (specs[i]);
			memset (&value, 0, sizeof (value));
			g_value_init (&value, prop_type);
			g_object_get_property  (G_OBJECT (component), specs[i]->name, &value);
			if (!g_param_value_defaults (specs[i], &value))
				gsf_xml_out_add_gvalue (output, specs[i]->name, &value);
			g_value_unset (&value);
		}
	gsf_xml_out_start_element (output, "data");
	if (component->length == 0)
		go_component_get_data (component, (void **) &component->data, &component->length,
		                       &component->destroy_notify, &component->destroy_data);
	gsf_xml_out_add_base64 (output, NULL, component->data, component->length);
	gsf_xml_out_end_element (output);
	if (component->snapshot_type != GO_SNAPSHOT_NONE && component->snapshot_data == NULL)
		go_component_build_snapshot (component);
	if (component->snapshot_data != NULL) {
		gsf_xml_out_start_element (output, "snapshot");
		gsf_xml_out_add_cstr (output, "type", component->snapshot_type == GO_SNAPSHOT_SVG? "svg": "png");
		gsf_xml_out_add_base64 (output, NULL, component->snapshot_data, component->snapshot_length);
		gsf_xml_out_end_element (output);
	}
	gsf_xml_out_end_element (output);
}

typedef struct {
	GOComponent	*component;
	unsigned	 dimension_id;

	GOComponentSaxHandler handler;
	gpointer user_data;
} GOCompXMLReadState;

static void
_go_component_start (GsfXMLIn *xin, xmlChar const **attrs)
{
	GOCompXMLReadState *state = (GOCompXMLReadState *) xin->user_state;
	char const *mime_type = NULL;
	int i;
	double width = 1., height = 1.;

	for (i = 0; attrs != NULL && attrs[i] && attrs[i+1] ; i += 2)
		if (0 == strcmp (attrs[i], "mime-type"))
			mime_type = (char const *) attrs[i+1];
		else if (0 == strcmp (attrs[i], "width"))
			width = go_ascii_strtod ((char const *) attrs[i+1], NULL);
		else if (0 == strcmp (attrs[i], "height"))
			height = go_ascii_strtod ((char const *) attrs[i+1], NULL);

	g_return_if_fail (mime_type);
	state->component = go_component_new_by_mime_type (mime_type);
	if (!state->component) {
		state->component = g_object_new (go_component_snapshot_get_type (), NULL);
		state->component->mime_type = g_strdup (mime_type);
		state->component->width = width;
		state->component->height = height;
	} else for (i = 0; attrs != NULL && attrs[i] && attrs[i+1] ; i += 2) {
		GParamSpec *prop_spec;
		GValue res;
		memset (&res, 0, sizeof (res));
		prop_spec = g_object_class_find_property (
				G_OBJECT_GET_CLASS (state->component), attrs[i]);
		if (prop_spec && (prop_spec->flags & GO_PARAM_PERSISTENT) &&
			gsf_xml_gvalue_from_str (&res,
				G_TYPE_FUNDAMENTAL (G_PARAM_SPEC_VALUE_TYPE (prop_spec)),
				attrs[i+1])) {
			g_object_set_property (G_OBJECT (state->component), attrs[i], &res);
			g_value_unset (&res);
		}
	}

}

static void
_go_component_load_data (GsfXMLIn *xin, G_GNUC_UNUSED GsfXMLBlob *unknown)
{
	GOCompXMLReadState *state = (GOCompXMLReadState *) xin->user_state;
	GOComponentClass *klass;
	GOComponent *component = state->component;
	size_t length;

	g_return_if_fail (component);

	component->data = component->destroy_data = g_base64_decode (xin->content->str, &length);
	component->destroy_notify = g_free;
	component->length = length;
	klass = GO_COMPONENT_GET_CLASS (component);
	if (klass->set_data)
		klass->set_data (component);
}

static void
_go_component_start_snapshot (GsfXMLIn *xin, xmlChar const **attrs)
{
	GOCompXMLReadState *state = (GOCompXMLReadState *) xin->user_state;
	int i;

	for (i = 0; attrs != NULL && attrs[i] && attrs[i+1] ; i += 2)
		if (0 == strcmp (attrs[i], "type"))
			state->component->snapshot_type = go_snapshot_type_from_string ((char const *) attrs[i+1]);
}

static void
_go_component_load_snapshot (GsfXMLIn *xin, G_GNUC_UNUSED GsfXMLBlob *unknown)
{
	GOCompXMLReadState *state = (GOCompXMLReadState *) xin->user_state;
	size_t length;
	state->component->snapshot_data = g_base64_decode (xin->content->str, &length);
	state->component->snapshot_length = length;
}

static void
_go_component_sax_parser_done (GsfXMLIn *xin, GOCompXMLReadState *state)
{
	(*state->handler) (state->component, state->user_data);
	g_free (state);
}

/**
 * go_component_sax_push_parser:
 * @xin: #GsfInput
 * @attrs: the current node attributes.
 * @handler: (scope call): #GOComponentSaxHandler
 * @user_data: data to pass to @handler
 *
 * Loads the component from the xml stream. @handler will be called when done.
 */
void
go_component_sax_push_parser (GsfXMLIn *xin, xmlChar const **attrs,
				GOComponentSaxHandler handler, gpointer user_data)
{
	static GsfXMLInNode const dtd[] = {
	  GSF_XML_IN_NODE (GO_COMP, GO_COMP, -1, "GOComponent",	GSF_XML_NO_CONTENT, &_go_component_start, NULL),
	    GSF_XML_IN_NODE (GO_COMP, GO_COMP_DATA, -1, "data", GSF_XML_CONTENT, NULL, &_go_component_load_data),
	    GSF_XML_IN_NODE (GO_COMP, GO_COMP_SNAPSHOT, -1, "snapshot", GSF_XML_CONTENT, &_go_component_start_snapshot, &_go_component_load_snapshot),
	  GSF_XML_IN_NODE_END
	};
	static GsfXMLInDoc *doc = NULL;
	GOCompXMLReadState *state;

	if (NULL == doc) {
		doc = gsf_xml_in_doc_new (dtd, NULL);
		go_xml_in_doc_dispose_on_exit (&doc);
	}
	state = g_new0 (GOCompXMLReadState, 1);
	state->handler = handler;
	state->user_data = user_data;
	gsf_xml_in_push_state (xin, doc, state,
		(GsfXMLInExtDtor) _go_component_sax_parser_done, attrs);
}

struct write_state {
	size_t length;
	GsfOutput *output;
};

static cairo_status_t
gsf_output_from_cairo (struct write_state *state, unsigned char *data, unsigned int length)
{
	if (gsf_output_write (state->output, length, data)) {
		state->length += length;
		return CAIRO_STATUS_SUCCESS;
	} else
		return CAIRO_STATUS_WRITE_ERROR;
}

GOSnapshotType
go_component_build_snapshot (GOComponent *component)
{
	cairo_surface_t *surface;
	cairo_t *cr;
	cairo_status_t status;
	GOSnapshotType res;
	struct write_state state;

	g_return_val_if_fail (GO_IS_COMPONENT (component), GO_SNAPSHOT_NONE);

	state.output = gsf_output_memory_new ();
	state.length = 0;

	switch (component->snapshot_type) {
	case GO_SNAPSHOT_SVG:
		surface = cairo_svg_surface_create_for_stream (
                                     (cairo_write_func_t) gsf_output_from_cairo,
                                     &state,
                                     component->width * 72,
                                     component->height * 72);
		cr = cairo_create (surface);
		go_component_render (component, cr, component->width * 72, component->height * 72);
		break;
	case GO_SNAPSHOT_PNG:
		surface = cairo_image_surface_create (
		                                     CAIRO_FORMAT_ARGB32,
		                                     component->width * 300,
		                                     component->height * 300);
		cr = cairo_create (surface);
		go_component_render (component, cr, component->width * 300, component->height * 300);
		cairo_surface_write_to_png_stream (surface,
		                     (cairo_write_func_t) gsf_output_from_cairo,
		                     &state);
		break;
	default:
		return GO_SNAPSHOT_NONE;
	}
	if (cairo_surface_status (surface) == CAIRO_STATUS_SUCCESS)
		res = component->snapshot_type;
	else
		res = GO_SNAPSHOT_NONE;

	cairo_surface_destroy (surface);
	status = cairo_status (cr);
	cairo_destroy (cr);
	if (status == CAIRO_STATUS_SUCCESS && state.length > 0) {
		component->snapshot_length = state.length;
		component->snapshot_data = g_new (char, state.length);
		memcpy(component->snapshot_data, gsf_output_memory_get_bytes ((GsfOutputMemory *) state.output), state.length);
	}
	g_object_unref (state.output);
	return res;
}

GOComponent  *
go_component_new_from_uri (char const *uri)
{
	char *mime_type;
	GsfInput *input;
	GOComponent *component;
	GError *err;
	size_t length;
	char *data;

	g_return_val_if_fail (uri && *uri, NULL);

	mime_type = go_get_mime_type (uri);
	if (!mime_type)
		return NULL;
	component = go_component_new_by_mime_type (mime_type);
	g_free (mime_type);
	input = go_file_open (uri, &err);
	if (err) {
		g_error_free (err);
		return NULL;
	}
	length = gsf_input_size (input);
	data = g_new (guint8, length);
	gsf_input_read (input, length, data);
	go_component_set_data (component, data, length);
	component->destroy_notify = g_free;
	component->destroy_data = data;
	return component;
}

/**
 * go_component_get_snapshot:
 * @component: #GOComponent
 * @type: #GOSnapshotType
 * @length: where to store the data length
 *
 * Returns a snapshot is either svg or png format for the component.
 * Returns: (transfer none): the snapshot as an arry of bytes.
 */
void const *
go_component_get_snapshot (GOComponent *component, GOSnapshotType *type, size_t *length)
{
	if (component->snapshot_data == NULL)
		go_component_build_snapshot (component);
	if (type)
		*type = component->snapshot_type;
	if (length)
		*length = component->snapshot_length;
	return component->snapshot_data;
}

/**
 * go_component_set_font:
 * @component: #GOComponent
 * @desc: #PangoFontDescription
 *
 * Sets the font the component should use. Not all components will actually
 * changed the font they use.
 * Returns: %TRUE if size changed.
 */

gboolean go_component_set_font (GOComponent *component, PangoFontDescription const *desc)
{
	GOComponentClass *klass = GO_COMPONENT_GET_CLASS (component);
	if (klass->set_font)
		return klass->set_font (component, desc);
	return FALSE;
}

/**
 * go_component_export_image:
 * @component: a #GOComponent
 * @format: image format for export
 * @output: a #GsfOutput stream
 * @x_dpi: x resolution of exported graph
 * @y_dpi: y resolution of exported graph
 *
 * Exports an image of @graph in given @format, writing results in a #GsfOutput stream.
 * If export format type is a bitmap one, it computes image size with x_dpi, y_dpi and
 * @graph size (see @gog_graph_get_size()).
 *
 * returns: %TRUE if export succeed.
 **/

gboolean
go_component_export_image (GOComponent *component, GOImageFormat format, GsfOutput *output,
			double x_dpi, double y_dpi)
{
	return FALSE;
}

/**
 * go_component_duplicate:
 * @component: a #GOComponent
 *
 * Duplicates the component.
 * Returns: (transfer full): the duplicated component.
 */
GOComponent *
go_component_duplicate (GOComponent const *component)
{
	GOComponent *res;
	GValue	 value;
	guint i, nbprops;
	GType    prop_type;
	GParamSpec **specs;
	void *new_data;

	g_return_val_if_fail (GO_IS_COMPONENT (component), NULL);

	res = go_component_new_by_mime_type (component->mime_type);
	res->width = component->width;
	res->height = component->height;
	/* first copy needed component specific properties */
	specs = g_object_class_list_properties (G_OBJECT_GET_CLASS (component), &nbprops);
	for (i = 0; i < nbprops; i++)
		if (specs[i]->flags & GO_PARAM_PERSISTENT) {
			prop_type = G_PARAM_SPEC_VALUE_TYPE (specs[i]);
			memset (&value, 0, sizeof (value));
			g_value_init (&value, prop_type);
			g_object_get_property  (G_OBJECT (component), specs[i]->name, &value);
			if (!g_param_value_defaults (specs[i], &value))
				g_object_set_property (G_OBJECT (res), specs[i]->name, &value);
			g_value_unset (&value);
		}
	/* and now the data */
	new_data = go_memdup (component->data, component->length);
	go_component_set_data (res, new_data, component->length);
	res->destroy_notify = g_free;
	res->destroy_data = new_data;
	res->priv = g_new (GOComponentPrivate, 1);
	res->priv->is_inline = component->priv->is_inline;
	return res;
}

/**
 * go_component_set_inline:
 * @component: a #GOComponent
 * @is_inline: whether the component should be displayed in-line
 *
 * Sets the in-line or not nature of the component. Default is %FALSE.
 **/
void
go_component_set_inline (GOComponent *component, gboolean is_inline)
{
	g_return_if_fail (GO_IS_COMPONENT (component));
	component->priv->is_inline = is_inline;
}

/**
 * go_component_get_inline:
 * @component: a #GOComponent
 *
 * Returns the in-line or not nature of the component.
 * Returns: whether the component is displayed in-line
 **/
gboolean
go_component_get_inline (GOComponent *component)
{
	g_return_val_if_fail (GO_IS_COMPONENT (component), FALSE);
	return component->priv->is_inline;
}

/**
 * go_component_set_use_font_from_app:
 * @component: a #GOComponent
 * @use_font_from_app: whether the component should use the font from the
 * calling application or use its own font.
 *
 * Sets the source of the font that the component should use. Default is %FALSE.
 **/
void
go_component_set_use_font_from_app (GOComponent *component, gboolean use_font_from_app)
{
	g_return_if_fail (GO_IS_COMPONENT (component));
	component->priv->use_font_from_app = use_font_from_app;
}

/**
 * go_component_get_use_font_from_app:
 * @component: a #GOComponent
 *
 * Returns whether the component should use the font from the calling
 * application or use its own font.
 * Returns: whether the component should use the font from the calling
 * application
 **/
gboolean
go_component_get_use_font_from_app (GOComponent *component)
{
	g_return_val_if_fail (GO_IS_COMPONENT (component), FALSE);
	return component->priv->use_font_from_app;
}
