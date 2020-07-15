/*
 * gog-object-xml.c :
 *
 * Copyright (C) 2003-2005 Jody Goldberg (jody@gnome.org)
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
#include <glib/gi18n-lib.h>

#include <string.h>
#include <stdlib.h>

/* GogGrid object was used with "Grid" role name by GogChart. We've changed the role
 * name to "Backplane", since that's what the user see in the UI, and that's what GogGrid
 * really is. We do the substitution on save/load for backward compatibility. */

#define GOG_BACKPLANE_OLD_ROLE_NAME	"Grid"
#define GOG_BACKPLANE_NEW_ROLE_NAME	"Backplane"

void
gog_object_set_arg (char const *name, char const *val, GogObject *obj)
{
	GParamSpec *pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (obj), name);
	GType prop_type;
	GValue res = { 0 };
	gboolean success = TRUE;

	if (pspec == NULL) {
		g_warning ("unknown property `%s' for class `%s'",
			   name, G_OBJECT_TYPE_NAME (obj));
		return;
	}

	prop_type = G_PARAM_SPEC_VALUE_TYPE (pspec);
	if (val == NULL &&
	    G_TYPE_FUNDAMENTAL (prop_type) != G_TYPE_BOOLEAN) {
		g_warning ("could not convert NULL to type `%s' for property `%s'",
			   g_type_name (prop_type), pspec->name);
		return;
	}
	if (!gsf_xml_gvalue_from_str (&res, prop_type, val))
		success = FALSE;

	if (!success) {
		g_warning ("could not convert string to type `%s' for property `%s'",
			   g_type_name (prop_type), pspec->name);
	} else
		g_object_set_property (G_OBJECT (obj), name, &res);
	g_value_unset (&res);
}

static void
gog_object_write_property_sax (GogObject const *obj, GParamSpec *pspec, GsfXMLOut *output)
{
	GObject *val_obj;
	GType    prop_type = G_PARAM_SPEC_VALUE_TYPE (pspec);
	GValue	 value = { 0 };

	g_value_init (&value, prop_type);
	g_object_get_property (G_OBJECT (obj), pspec->name, &value);

	/* No need to save default values */
	if (((pspec->flags & GOG_PARAM_POSITION) &&
	     gog_object_is_default_position_flags (obj, pspec->name)) ||
	    (!(pspec->flags & GOG_PARAM_FORCE_SAVE) &&
	     !(pspec->flags & GOG_PARAM_POSITION) &&
	     g_param_value_defaults (pspec, &value))) {
		g_value_unset (&value);
		return;
	}

	switch (G_TYPE_FUNDAMENTAL (prop_type)) {
	case G_TYPE_CHAR:
	case G_TYPE_UCHAR:
	case G_TYPE_BOOLEAN:
	case G_TYPE_INT:
	case G_TYPE_UINT:
	case G_TYPE_LONG:
	case G_TYPE_ULONG:
	case G_TYPE_ENUM:
	case G_TYPE_FLAGS: {
		GValue str = { 0 };
		g_value_init (&str, G_TYPE_STRING);
		g_value_transform (&value, &str);
		gsf_xml_out_start_element (output, "property");
		gsf_xml_out_add_cstr_unchecked (output, "name", pspec->name);
		gsf_xml_out_add_cstr (output, NULL, g_value_get_string (&str));
		gsf_xml_out_end_element (output); /* </property> */
		g_value_unset (&str);
		break;
	}

	case G_TYPE_FLOAT:
	case G_TYPE_DOUBLE: {
		GValue vd = { 0 };
		GString *str = g_string_new (NULL);

		g_value_init (&vd, G_TYPE_DOUBLE);
		g_value_transform (&value, &vd);
		go_dtoa (str, "!g", g_value_get_double (&vd));
		g_value_unset (&vd);

		gsf_xml_out_start_element (output, "property");
		gsf_xml_out_add_cstr_unchecked (output, "name", pspec->name);
		gsf_xml_out_add_cstr (output, NULL, str->str);
		gsf_xml_out_end_element (output); /* </property> */
		g_string_free (str, TRUE);
		break;
	}

	case G_TYPE_STRING: {
		char const *str = g_value_get_string (&value);
		if (str != NULL) {
			gsf_xml_out_start_element (output, "property");
			gsf_xml_out_add_cstr_unchecked (output, "name", pspec->name);
			gsf_xml_out_add_cstr (output, NULL, str);
			gsf_xml_out_end_element (output); /* </property> */
		}
		break;
	}

	case G_TYPE_OBJECT:
		val_obj = g_value_get_object (&value);
		if (val_obj != NULL) {
			if (GO_IS_PERSIST (val_obj)) {
				gsf_xml_out_start_element (output, "property");
				gsf_xml_out_add_cstr_unchecked (output, "name", pspec->name);
				go_persist_sax_save (GO_PERSIST (val_obj), output);
				gsf_xml_out_end_element (output); /* </property> */
			} else
				g_warning ("How are we supposed to persist this ??");
		}
		break;

	default:
		g_warning ("I could not persist property \"%s\", since type \"%s\" is unhandled.",
			   g_param_spec_get_name (pspec), g_type_name (G_TYPE_FUNDAMENTAL(prop_type)));
	}
	g_value_unset (&value);
}

static void
gog_dataset_sax_save (GogDataset const *set, GsfXMLOut *output, gpointer user)
{
	GOData  *dat;
	char    *tmp;
	int      i, last;

	gsf_xml_out_start_element (output, "data");
	gog_dataset_dims (set, &i, &last);
	for ( ; i <= last ; i++) {
		dat = gog_dataset_get_dim (set, i);
		if (dat == NULL)
			continue;

		tmp = go_data_serialize (dat, user);
		/* only save the data if there is some valid content, see #46 */
		if (tmp == NULL || *tmp == 0)
			continue;
		gsf_xml_out_start_element (output, "dimension");
		gsf_xml_out_add_int (output, "id", i);
		gsf_xml_out_add_cstr (output, "type",
			G_OBJECT_TYPE_NAME (dat));
		gsf_xml_out_add_cstr (output, NULL, tmp);
		g_free (tmp);
		gsf_xml_out_end_element (output); /* </dimension> */
	}
	gsf_xml_out_end_element (output); /* </data> */

}

void
gog_object_write_xml_sax (GogObject const *obj, GsfXMLOut *output, gpointer user)
{
	guint	     n;
	GParamSpec **props;
	GSList	    *ptr;

	g_return_if_fail (GOG_IS_OBJECT (obj));

	gsf_xml_out_start_element (output, "GogObject");

	/* Primary details */
	if (obj->role != NULL) {
		if (strcmp (obj->role->id, GOG_BACKPLANE_NEW_ROLE_NAME) == 0)
			gsf_xml_out_add_cstr (output, "role", GOG_BACKPLANE_OLD_ROLE_NAME);
		else
			gsf_xml_out_add_cstr (output, "role", obj->role->id);
	}
	if (obj->explicitly_typed_role || obj->role == NULL)
		gsf_xml_out_add_cstr (output, "type", G_OBJECT_TYPE_NAME (obj));

	/* properties */
	props = g_object_class_list_properties (G_OBJECT_GET_CLASS (obj), &n);
	while (n-- > 0)
		if (props[n]->flags & GO_PARAM_PERSISTENT)
			gog_object_write_property_sax (obj, props[n], output);

	g_free (props);

	if (GO_IS_PERSIST (obj))	/* anything special for this class */
		go_persist_sax_save (GO_PERSIST (obj), output);
	if (GOG_IS_DATASET (obj))	/* convenience to save data */
		gog_dataset_sax_save (GOG_DATASET (obj), output, user);

	/* the children */
	for (ptr = obj->children; ptr != NULL ; ptr = ptr->next)
		gog_object_write_xml_sax (ptr->data, output, user);

	gsf_xml_out_end_element (output); /* </GogObject> */
}

typedef struct {
	GogObject	*obj;
	GSList		*obj_stack;
	GParamSpec	*prop_spec;
	gboolean	 prop_pushed_obj;
	GOData		*dimension;
	int		 dimension_id;

	GogObjectSaxHandler handler;
	gpointer user_data;
	gpointer user_unserialize;
} GogXMLReadState;

/**
 * gog_xml_read_state_get_obj:
 * @xin: #GsfXMLIn
 *
 * Returns: (transfer none): the laset rerad #GogObject
 **/
GogObject *
gog_xml_read_state_get_obj (GsfXMLIn *xin)
{
	return ((GogXMLReadState *)xin->user_state)->obj;
}

static void
gogo_dim_start (GsfXMLIn *xin, xmlChar const **attrs)
{
	GogXMLReadState *state = (GogXMLReadState *)xin->user_state;
	xmlChar const *dim_str = NULL, *type_str = NULL;
	GType type;
	int first, last;

	if (NULL == state->obj)
		return;

	g_return_if_fail (GOG_IS_DATASET (state->obj));

	for (; attrs != NULL && attrs[0] && attrs[1] ; attrs += 2)
		if (0 == strcmp (attrs[0], "id"))
			dim_str = attrs[1];
		else if (0 == strcmp (attrs[0], "type"))
			type_str = attrs[1];

	if (NULL == dim_str) {
		g_warning ("missing dimension id for class `%s'",
			   G_OBJECT_TYPE_NAME (state->obj));
		return;
	}
	state->dimension_id = strtol (dim_str, NULL, 10);
	gog_dataset_dims (GOG_DATASET (state->obj), &first, &last);
	if (state->dimension_id < first || state->dimension_id > last) {
		g_warning ("invalid dimension id %d for class `%s'",
			   state->dimension_id, G_OBJECT_TYPE_NAME (state->obj));
		return;
	}

	if (NULL == type_str) {
		g_warning ("missing type for dimension `%s' of class `%s'",
			   dim_str, G_OBJECT_TYPE_NAME (state->obj));
		return;
	}

	type = g_type_from_name (type_str);
	if (0 == type) {
		g_warning ("unknown type '%s' for dimension `%s' of class `%s'",
			   type_str, dim_str, G_OBJECT_TYPE_NAME (state->obj));
		return;
	} else if (!g_type_is_a (type, GO_TYPE_DATA) || G_TYPE_IS_ABSTRACT (type)) {
		g_warning ("type '%s' is invalid as dimension `%s' of class `%s'",
			   type_str, dim_str, G_OBJECT_TYPE_NAME (state->obj));
		return;
	}
	state->dimension = g_object_new (type, NULL);

	g_return_if_fail (state->dimension != NULL);
}

static void
gogo_dim_end (GsfXMLIn *xin, G_GNUC_UNUSED GsfXMLBlob *unknown)
{
	GogXMLReadState *state = (GogXMLReadState *)xin->user_state;

	if (NULL == state->obj)
		return;

	g_return_if_fail (GOG_IS_DATASET (state->obj));

	if (NULL != state->dimension) {
		if (go_data_unserialize (state->dimension,
					 xin->content->str,
					 state->user_unserialize))
			gog_dataset_set_dim (GOG_DATASET (state->obj),
				state->dimension_id, state->dimension, NULL);
		else
			g_object_unref (state->dimension);
		state->dimension = NULL;
	}
}

static void
gogo_prop_start (GsfXMLIn *xin, xmlChar const **attrs)
{
	GogXMLReadState *state = (GogXMLReadState *)xin->user_state;
	xmlChar const *prop_str = NULL, *type_str = NULL;
	GType prop_type;
	int i;

	if (NULL == state->obj) {
		state->prop_spec = NULL;
		return;
	}

	for (i = 0; attrs != NULL && attrs[i] && attrs[i+1] ; i += 2)
		if (0 == strcmp (attrs[i], "name"))
			prop_str = attrs[i+1];
		else if (0 == strcmp (attrs[i], "type"))
			type_str = attrs[i+1];

	if (prop_str == NULL) {
		g_warning ("missing name for property of class `%s'",
			   G_OBJECT_TYPE_NAME (state->obj));
		return;
	}
	state->prop_spec = g_object_class_find_property (
		G_OBJECT_GET_CLASS (state->obj), prop_str);
	if (state->prop_spec == NULL) {
		g_warning ("unknown property `%s' for class `%s'",
			   prop_str, G_OBJECT_TYPE_NAME (state->obj));
		return;
	}

	prop_type = G_PARAM_SPEC_VALUE_TYPE (state->prop_spec);
	if (G_TYPE_FUNDAMENTAL (prop_type) == G_TYPE_OBJECT) {
		GType type;
		GogObject *obj;

		if (NULL == type_str) {
			g_warning ("missing type for property `%s' of class `%s'",
				   prop_str, G_OBJECT_TYPE_NAME (state->obj));
			return;
		}

		type = g_type_from_name (type_str);
		if (0 == type) {
			g_warning ("unknown type '%s' for property `%s' of class `%s'",
				   type_str, prop_str, G_OBJECT_TYPE_NAME (state->obj));
			return;
		} else if (!g_type_is_a (type, prop_type) || G_TYPE_IS_ABSTRACT (type)) {
			g_warning ("invalid type '%s' for property `%s' of class `%s'",
				   type_str, prop_str, G_OBJECT_TYPE_NAME (state->obj));
			return;
		}
		obj = g_object_new (type, NULL);

		g_return_if_fail (obj != NULL);

		state->obj_stack = g_slist_prepend (state->obj_stack, state->obj);
		state->obj = obj;
		state->prop_pushed_obj = TRUE;
		if (GO_IS_PERSIST (obj))
			go_persist_prep_sax (GO_PERSIST (obj), xin, attrs);
	}
}

static void
gogo_prop_end (GsfXMLIn *xin, G_GNUC_UNUSED GsfXMLBlob *unknown)
{
	GogXMLReadState *state = (GogXMLReadState *)xin->user_state;
	char const *content = xin->content->str;
	GType prop_type, prop_ftype;
	GValue val = { 0 };

	if (state->obj == NULL || state->prop_spec == NULL)
		return;

	prop_type = G_PARAM_SPEC_VALUE_TYPE (state->prop_spec);
	prop_ftype = G_TYPE_FUNDAMENTAL (prop_type);
	if (prop_ftype == G_TYPE_OBJECT) {
		GogObject *obj = state->obj;

		if (!state->prop_pushed_obj)
			return;

		state->obj = state->obj_stack->data;
		state->obj_stack = g_slist_remove (state->obj_stack, state->obj);
		state->prop_pushed_obj = FALSE;

		g_value_init (&val, prop_type);
		g_value_set_object (&val, G_OBJECT (obj));
		g_object_unref (obj);
	} else {
		gboolean ok = FALSE;
		if (content == NULL && prop_ftype != G_TYPE_BOOLEAN) {
			g_warning ("could not convert NULL to type `%s' for property `%s'",
				   g_type_name (prop_type), state->prop_spec->name);
			return;
		}

		if (prop_type == G_TYPE_BOOLEAN) {
			// We've managed to save some files with translated
			// booleans.  Try to recover.
			if (g_str_equal (content, _("TRUE"))) {
				g_value_init (&val, prop_type);
				g_value_set_boolean (&val, TRUE);
				ok = TRUE;
			} else if (g_str_equal (content, _("FALSE"))) {
				g_value_init (&val, prop_type);
				g_value_set_boolean (&val, FALSE);
				ok = TRUE;
			}
		}

		if (!ok)
			ok = gsf_xml_gvalue_from_str (&val, prop_type, content);

		if (!ok) {
			g_warning ("could not convert string to type `%s' for property `%s'",
				   g_type_name (prop_type), state->prop_spec->name);
			return;
		}
	}
	g_object_set_property (G_OBJECT (state->obj),
		state->prop_spec->name, &val);
	g_value_unset (&val);
}

/* NOTE : every path through this must push something onto obj_stack. */
static void
gogo_start (GsfXMLIn *xin, xmlChar const **attrs)
{
	GogXMLReadState *state = (GogXMLReadState *)xin->user_state;
	xmlChar const *type = NULL, *role = NULL;
	GogObject *res;
	unsigned i;

	for (i = 0; attrs != NULL && attrs[i] && attrs[i+1] ; i += 2)
		if (0 == strcmp (attrs[i], "type"))
			type = attrs[i+1];
		else if (0 == strcmp (attrs[i], "role"))
			role = attrs[i+1];

	if (NULL != type) {
		GType t = g_type_from_name (type);
		if (t == 0) {
			res = (GogObject *)gog_plot_new_by_name (type);
			if (NULL == res)
				res = (GogObject *)gog_trend_line_new_by_name (type);
		} else if (g_type_is_a (t, GOG_TYPE_OBJECT) && !G_TYPE_IS_ABSTRACT (t))
			res = g_object_new (t, NULL);
		else
			res = NULL;

		if (res == NULL) {
			g_warning ("unknown type '%s'", type);
		}
		if (GOG_IS_GRAPH (res))
			((GogGraph *) res)->doc = (GODoc *) g_object_get_data (G_OBJECT (gsf_xml_in_get_input (xin)), "document");
	} else
		res = NULL;

	if (role != NULL) {
		if (strcmp (role, GOG_BACKPLANE_OLD_ROLE_NAME) == 0)
			res = gog_object_add_by_name (state->obj, GOG_BACKPLANE_NEW_ROLE_NAME, res);
		else
			res = gog_object_add_by_name (state->obj, role, res);
	}
	if (res != NULL) {
		res->explicitly_typed_role = (type != NULL);
		if (GO_IS_PERSIST (res))
			go_persist_prep_sax (GO_PERSIST (res), xin, attrs);
	}

	state->obj_stack = g_slist_prepend (state->obj_stack, state->obj);
	state->obj = res;
}

static void
gogo_end (GsfXMLIn *xin, G_GNUC_UNUSED GsfXMLBlob *unknown)
{
	GogXMLReadState *state = (GogXMLReadState *)xin->user_state;

	/* Leave the top object on the stack for use in the caller */
	if (state->obj_stack->next != NULL) {
		state->obj = state->obj_stack->data;
		state->obj_stack = g_slist_remove (state->obj_stack, state->obj);
	} else {
		g_slist_free (state->obj_stack);
		state->obj_stack = NULL;
	}
}

static void
go_sax_parser_done (GsfXMLIn *xin, GogXMLReadState *state)
{
	(*state->handler) (state->obj, state->user_data);
	g_free (state);
}

static GsfXMLInNode const gog_dtd[] = {
  GSF_XML_IN_NODE (GOG_OBJ, GOG_OBJ, -1, "GogObject",	GSF_XML_NO_CONTENT, &gogo_start, &gogo_end),
    GSF_XML_IN_NODE (GOG_OBJ, GOG_OBJ_PROP, -1, "property", GSF_XML_CONTENT, &gogo_prop_start, &gogo_prop_end),
    GSF_XML_IN_NODE (GOG_OBJ, GOG_OBJ_DATA, -1, "data", GSF_XML_NO_CONTENT, NULL, NULL),
      GSF_XML_IN_NODE (GOG_OBJ_DATA, GOG_DATA_DIM, -1, "dimension", GSF_XML_CONTENT, &gogo_dim_start, &gogo_dim_end),
  GSF_XML_IN_NODE_END
};
static GsfXMLInDoc *gog_sax_doc = NULL;

/**
 * gog_object_sax_push_parser:
 * @xin: #GsfXMLIn
 * @attrs: XML attributes
 * @handler: (scope call): callback
 * @user_unserialize: user data for #GOData reading
 * @user_data: user data for @handler
 *
 * Unserializes a #GogObject using @handler when done.
 **/
void
gog_object_sax_push_parser (GsfXMLIn *xin, xmlChar const **attrs,
			    GogObjectSaxHandler	handler,
			    gpointer            user_unserialize,
			    gpointer		user_data)
{
	GogXMLReadState *state;

	if (NULL == gog_sax_doc) {
		gog_sax_doc = gsf_xml_in_doc_new (gog_dtd, NULL);
		go_xml_in_doc_dispose_on_exit (&gog_sax_doc);
	}
	state = g_new0 (GogXMLReadState, 1);
	state->handler = handler;
	state->user_data = user_data;
	state->user_unserialize = user_unserialize;
	gsf_xml_in_push_state (xin, gog_sax_doc, state,
		(GsfXMLInExtDtor) go_sax_parser_done, attrs);
}

GogObject *
gog_object_new_from_input (GsfInput *input,
                           gpointer user_unserialize)
{
	GogObject *res = NULL;
	GogXMLReadState *state = g_new0 (GogXMLReadState, 1);
	if (NULL == gog_sax_doc) {
		gog_sax_doc = gsf_xml_in_doc_new (gog_dtd, NULL);
		go_xml_in_doc_dispose_on_exit (&gog_sax_doc);
	}
	state->user_unserialize = user_unserialize;
	if (gsf_xml_in_doc_parse (gog_sax_doc, input, state)) {
		res = state->obj;
	}
	g_free (state);
	return res;
}
