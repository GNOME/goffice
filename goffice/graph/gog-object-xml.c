/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-object-xml.c :
 *
 * Copyright (C) 2003-2005 Jody Goldberg (jody@gnome.org)
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
#include <goffice/graph/gog-object-xml.h>
#include <goffice/graph/gog-object.h>
#include <goffice/graph/gog-plot.h>
#include <goffice/graph/gog-trend-line.h>
#include <goffice/graph/gog-data-set.h>
#include <goffice/data/go-data.h>
#include <goffice/utils/go-color.h>

#include <string.h>
#include <stdlib.h>

GType
gog_persist_get_type (void)
{
	static GType gog_persist_type = 0;

	if (!gog_persist_type) {
		static GTypeInfo const gog_persist_info = {
			sizeof (GogPersistClass),	/* class_size */
			NULL,		/* base_init */
			NULL,		/* base_finalize */
		};

		gog_persist_type = g_type_register_static (G_TYPE_INTERFACE,
			"GogPersist", &gog_persist_info, 0);
	}

	return gog_persist_type;
}

gboolean
gog_persist_dom_load (GogPersist *gp, xmlNode *node)
{
	g_return_val_if_fail (IS_GOG_PERSIST (gp), FALSE);
	return GOG_PERSIST_GET_CLASS (gp)->dom_load (gp, node);
}

void
gog_persist_sax_save (GogPersist const *gp, GsfXMLOut *output)
{
	g_return_if_fail (IS_GOG_PERSIST (gp));
	GOG_PERSIST_GET_CLASS (gp)->sax_save (gp, output);
}
void
gog_persist_prep_sax (GogPersist *gp, GsfXMLIn *xin, xmlChar const **attrs)
{
	g_return_if_fail (IS_GOG_PERSIST (gp));
	GOG_PERSIST_GET_CLASS (gp)->prep_sax (gp, xin, attrs);
}

static void
gog_object_set_arg_full (char const *name, char const *val, GogObject *obj, xmlNode *xml_node)
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

	if (G_TYPE_FUNDAMENTAL (prop_type) == G_TYPE_OBJECT) {
		g_value_init (&res, prop_type);
		if (g_type_is_a (prop_type, G_TYPE_OBJECT)) {
			xmlChar *type_name;
			GType    type = 0;
			GObject *val_obj;

			success = FALSE;
			type_name = xmlGetProp (xml_node, 
						(xmlChar const *) "type");
			if (type_name != NULL) {
				type = g_type_from_name (type_name);
			}
			xmlFree (type_name);
			if (type != 0) {
				val_obj = g_object_new (type, NULL);
				if (IS_GOG_PERSIST (val_obj) &&
				    gog_persist_dom_load (GOG_PERSIST (val_obj), xml_node)) {
					g_value_set_object (&res, val_obj);
					success = TRUE;
				}
				g_object_unref (val_obj);
			}
		}
	} else if (!gsf_xml_gvalue_from_str (&res, prop_type, val))
		success = FALSE;

	if (!success) {
		g_warning ("could not convert string to type `%s' for property `%s'",
			   g_type_name (prop_type), pspec->name);
	} else
		g_object_set_property (G_OBJECT (obj), name, &res);
	g_value_unset (&res);
}

void
gog_object_set_arg (char const *name, char const *val, GogObject *obj)
{
	gog_object_set_arg_full (name, val, obj, NULL);
}

static void
gog_object_write_property_sax (GogObject const *obj, GParamSpec *pspec, GsfXMLOut *output)
{
	GObject *val_obj;
	GType    prop_type = G_PARAM_SPEC_VALUE_TYPE (pspec);
	GValue	 value = { 0 };

	g_value_init (&value, prop_type);
	g_object_get_property  (G_OBJECT (obj), pspec->name, &value);

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
	case G_TYPE_FLOAT:
	case G_TYPE_DOUBLE:
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
			if (IS_GOG_PERSIST (val_obj)) {
				gsf_xml_out_start_element (output, "property");
				gsf_xml_out_add_cstr_unchecked (output, "name", pspec->name);
				gog_persist_sax_save (GOG_PERSIST (val_obj), output);
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
gog_dataset_dom_load (GogDataset *set, xmlNode *node)
{
	xmlNode *ptr;
	xmlChar *id, *val, *type;

	for (ptr = node->xmlChildrenNode ; ptr != NULL ; ptr = ptr->next) {
		if (xmlIsBlankNode (ptr) || ptr->name == NULL)
			continue;
		if (!strcmp (ptr->name, "data"))
			break;
	}
	if (ptr == NULL)
		return;
	for (ptr = ptr->xmlChildrenNode ; ptr != NULL ; ptr = ptr->next) {
		if (xmlIsBlankNode (ptr) || ptr->name == NULL)
			continue;
		if (!strcmp (ptr->name, "dimension")) {
			id   = xmlGetProp (ptr, (xmlChar const *) "id");
			type = xmlGetProp (ptr, (xmlChar const *) "type");
			val  = xmlNodeGetContent (ptr);
			if (id != NULL && type != NULL && val != NULL) {
				unsigned dim_id = strtoul (id, NULL, 0);
				GOData *dat = g_object_new (g_type_from_name (type), NULL);
				if (dat != NULL && go_data_from_str (dat, val))
					gog_dataset_set_dim (set, dim_id, dat, NULL);
			}

			if (id != NULL)	  xmlFree (id);
			if (type != NULL) xmlFree (type);
			if (val != NULL)  xmlFree (val);
		}
	}
}

static void
gog_dataset_sax_save (GogDataset const *set, GsfXMLOut *output)
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

		gsf_xml_out_start_element (output, "dimension");
		gsf_xml_out_add_int (output, "id", i);
		gsf_xml_out_add_cstr (output, "type", 
			G_OBJECT_TYPE_NAME (dat));
		tmp = go_data_as_str (dat);
		gsf_xml_out_add_cstr (output, NULL, tmp);
		g_free (tmp);
		gsf_xml_out_end_element (output); /* </dimension> */
	}
	gsf_xml_out_end_element (output); /* </data> */

}

void
gog_object_write_xml_sax (GogObject const *obj, GsfXMLOut *output)
{
	guint	     n;
	GParamSpec **props;
	GSList	    *ptr;

	g_return_if_fail (IS_GOG_OBJECT (obj));

	gsf_xml_out_start_element (output, "GogObject");

	/* Primary details */
	if (obj->role != NULL)
		gsf_xml_out_add_cstr (output, "role", obj->role->id);
	if (obj->explicitly_typed_role || obj->role == NULL)
		gsf_xml_out_add_cstr (output, "type", G_OBJECT_TYPE_NAME (obj));

	/* properties */
	props = g_object_class_list_properties (G_OBJECT_GET_CLASS (obj), &n);
	while (n-- > 0)
		if (props[n]->flags & GOG_PARAM_PERSISTENT)
			gog_object_write_property_sax (obj, props[n], output);

	g_free (props);

	if (IS_GOG_PERSIST (obj))	/* anything special for this class */
		gog_persist_sax_save (GOG_PERSIST (obj), output);
	if (IS_GOG_DATASET (obj))	/* convenience to save data */
		gog_dataset_sax_save (GOG_DATASET (obj), output);

	/* the children */
	for (ptr = obj->children; ptr != NULL ; ptr = ptr->next)
		gog_object_write_xml_sax (ptr->data, output);

	gsf_xml_out_end_element (output); /* </GogObject> */
}

GogObject *
gog_object_new_from_xml (GogObject *parent, xmlNode *node)
{
	xmlChar   *role, *name, *val, *type_name;
	xmlNode   *ptr;
	GogObject *res = NULL;
	gboolean explicitly_typed_role = FALSE;

	type_name = xmlGetProp (node, (xmlChar const *) "type");
	if (type_name != NULL) {
		GType type = g_type_from_name (type_name);
		if (type == 0) {
			GogPlot *plot = gog_plot_new_by_name (type_name);
			if (plot)
				res = GOG_OBJECT (plot);
			else
				res = GOG_OBJECT (gog_trend_line_new_by_name (type_name));
		} else
			res = g_object_new (type, NULL);
		xmlFree (type_name);
		explicitly_typed_role = TRUE;
		g_return_val_if_fail (res != NULL, NULL);
	}
	role = xmlGetProp (node, (xmlChar const *) "role");
	if (role == NULL) {
		g_return_val_if_fail (parent == NULL, NULL);
	} else {
		res = gog_object_add_by_name (parent, role, res);
		xmlFree (role);
	}

	g_return_val_if_fail (res != NULL, NULL);

	res->explicitly_typed_role = explicitly_typed_role;

	if (IS_GOG_PERSIST (res))
		gog_persist_dom_load (GOG_PERSIST (res), node);
	if (IS_GOG_DATASET (res))	/* convenience to save data */
		gog_dataset_dom_load (GOG_DATASET (res), node);

	for (ptr = node->xmlChildrenNode ; ptr != NULL ; ptr = ptr->next) {
		if (xmlIsBlankNode (ptr) || ptr->name == NULL)
			continue;
		if (!strcmp (ptr->name, "property")) {
			name = xmlGetProp (ptr, "name");
			if (name == NULL) {
				g_warning ("missing name for property entry");
				continue;
			}
			val = xmlNodeGetContent (ptr);
			gog_object_set_arg_full (name, val, res, ptr);
			xmlFree (val);
			xmlFree (name);
		} else if (!strcmp (ptr->name, "GogObject"))
			gog_object_new_from_xml (res, ptr);
	}
	return res;
}

typedef struct {
	GogObject	*obj;
	GSList		*obj_stack;
	GParamSpec	*prop_spec;
	gboolean	 prop_pushed_obj;
	GOData		*dimension;
	unsigned	 dimension_id;

	GogObjectSaxHandler	handler;
	gpointer		user_data;
} GogXMLReadState;

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

	if (NULL == state->obj)
		return;

	g_return_if_fail (IS_GOG_DATASET (state->obj));

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
	state->dimension_id = strtoul (dim_str, NULL, 10);

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

	g_return_if_fail (IS_GOG_DATASET (state->obj));

	if (NULL != state->dimension) {
		if (go_data_from_str (state->dimension, xin->content->str))
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
			g_warning ("missing type for property property `%s' of class `%s'",
				   prop_str, G_OBJECT_TYPE_NAME (state->obj));
			return;
		}

		type = g_type_from_name (type_str);
		if (0 == type) {
			g_warning ("unknown type '%s' for property property `%s' of class `%s'",
				   type_str, prop_str, G_OBJECT_TYPE_NAME (state->obj));
			return;
		}
		obj = g_object_new (type, NULL);

		g_return_if_fail (obj != NULL);

		state->obj_stack = g_slist_prepend (state->obj_stack, state->obj);
		state->obj = obj;
		state->prop_pushed_obj = TRUE;
		if (IS_GOG_PERSIST (obj))
			gog_persist_prep_sax (GOG_PERSIST (obj), xin, attrs);
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
		if (content == NULL && prop_ftype != G_TYPE_BOOLEAN) {
			g_warning ("could not convert NULL to type `%s' for property `%s'",
				   g_type_name (prop_type), state->prop_spec->name);
			return;
		}

		if (!gsf_xml_gvalue_from_str (&val, prop_ftype, content)) {
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
		} else
			res = g_object_new (t, NULL);

		if (res == NULL) {
			g_warning ("unknown type '%s'", type);
		}
	} else
		res = NULL;

	if (role != NULL)
		res = gog_object_add_by_name (state->obj, role, res);
	if (res != NULL) {
		res->explicitly_typed_role = (type != NULL);
		if (IS_GOG_PERSIST (res))
			gog_persist_prep_sax (GOG_PERSIST (res), xin, attrs);
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
	} else
		g_slist_free (state->obj_stack);
}

static void
go_sax_parser_done (GsfXMLIn *xin, GogXMLReadState *state)
{
	(*state->handler) (state->obj, state->user_data);
	g_free (state);
}

void
gog_object_sax_push_parser (GsfXMLIn *xin, xmlChar const **attrs,
			    GogObjectSaxHandler	handler,
			    gpointer		user_data)
{
	static GsfXMLInNode const dtd[] = {
	  GSF_XML_IN_NODE (GOG_OBJ, GOG_OBJ, -1, "GogObject",	GSF_XML_NO_CONTENT, &gogo_start, &gogo_end),
	    GSF_XML_IN_NODE (GOG_OBJ, GOG_OBJ_PROP, -1, "property", GSF_XML_CONTENT, &gogo_prop_start, &gogo_prop_end),
	    GSF_XML_IN_NODE (GOG_OBJ, GOG_OBJ_DATA, -1, "data", GSF_XML_NO_CONTENT, NULL, NULL),
	      GSF_XML_IN_NODE (GOG_OBJ_DATA, GOG_DATA_DIM, -1, "dimension", GSF_XML_CONTENT, &gogo_dim_start, &gogo_dim_end),
	    GSF_XML_IN_NODE (GOG_OBJ, GOG_OBJ, -1, "GogObject",	GSF_XML_NO_CONTENT, NULL, NULL),
	  GSF_XML_IN_NODE_END
	};
	static GsfXMLInDoc *doc = NULL;
	GogXMLReadState *state;

	if (NULL == doc)
		doc = gsf_xml_in_doc_new (dtd, NULL);
	state = g_new0 (GogXMLReadState, 1);
	state->handler = handler;
	state->user_data = user_data;
	gsf_xml_in_push_state (xin, doc, state,
		(GsfXMLInExtDtor) go_sax_parser_done, attrs);
}

void
go_xml_out_add_color (GsfXMLOut *output, char const *id, GOColor c)
{
	char *str = go_color_as_str (c);
	gsf_xml_out_add_cstr_unchecked (output, id, str);
	g_free (str);
}
