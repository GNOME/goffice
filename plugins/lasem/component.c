/*
 * Lasem GOffice component
 * component.c
 *
 * Copyright (C) 2014 by Jean Bréfort <jean.brefort@normalesup.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#include <goffice/goffice-config.h>
#include <goffice/goffice.h>
#include <goffice/app/module-plugin-defs.h>
#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>
#include <lsmdom.h>
#include <string.h>

#ifndef HAVE_LSM_ITEX_TO_MATHML
/* Lasem - SVG and Mathml library
 *
 * Copyright © 2013 Emmanuel Pacaud
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <https://www.gnu.org/licenses/>.
 *
 * Author:
 * 	Emmanuel Pacaud <emmanuel@gnome.org>
 */

extern char * itex2MML_parse (const char * buffer, unsigned long length);
extern void   itex2MML_free_string (char * str);

static char *
lsm_itex_to_mathml (const char *itex, int size)
{
	char *mathml;

	if (itex == NULL)
		return NULL;

	if (size < 0)
		size = strlen (itex);

	mathml = itex2MML_parse (itex, size);
	if (mathml == NULL)
		return NULL;

	if (mathml[0] == '\0') {
		itex2MML_free_string (mathml);
		return NULL;
	}

	return mathml;
}

static void
lsm_itex_free_mathml_buffer (char *mathml)
{
	if (mathml == NULL)
		return;

	itex2MML_free_string (mathml);
}
#endif

GOPluginModuleDepend const go_plugin_depends [] = {
	{ "goffice", GOFFICE_API_VERSION }
};
GOPluginModuleHeader const go_plugin_header =
	{ GOFFICE_MODULE_PLUGIN_MAGIC_NUMBER, G_N_ELEMENTS (go_plugin_depends) };

G_MODULE_EXPORT void go_plugin_init (GOPlugin *plugin, GOCmdContext *cc);
G_MODULE_EXPORT void go_plugin_shutdown (GOPlugin *plugin, GOCmdContext *cc);

typedef struct {
	GOComponent parent;

	char *itex;
	char *font;
	GOColor color;
	gboolean compact;
	PangoFontDescription *desc;

	LsmDomDocument *mathml;
	LsmDomNode *math_element;
	LsmDomNode *style_element;
	LsmDomNode *itex_element;
	LsmDomNode *itex_string;
	LsmDomView *view;
} GoLasemComponent;

typedef GOComponentClass GoLasemComponentClass;

#define GO_TYPE_LASEM_COMPONENT	(go_lasem_component_get_type ())
#define GO_LASEM_COMPONENT(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_LASEM_COMPONENT, GoLasemComponent))
#define GO_IS_LASEM_COMPONENT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_LASEM_COMPONENT))

GType go_lasem_component_get_type (void);

static GObjectClass *parent_klass;

enum {
	LASEM_PROP_0,
	LASEM_PROP_ITEX,
	LASEM_PROP_COLOR,
	LASEM_PROP_FONT,
};

static void
go_lasem_component_update_style (GoLasemComponent *equation)
{
	LsmDomElement *style_element;
	char *value;
	double width, height, baseline;

	if (equation->style_element == NULL) {
		LsmDomNode *child;
		equation->style_element = LSM_DOM_NODE (lsm_dom_document_create_element (equation->mathml, "mstyle"));
		/* put all document children into the mstyle element */
		while ((child = lsm_dom_node_get_first_child (equation->math_element))) {
			lsm_dom_node_remove_child (equation->math_element, child);
			lsm_dom_node_append_child (equation->style_element, child);
		}
		lsm_dom_node_append_child (equation->math_element, equation->style_element);
	}
	style_element = LSM_DOM_ELEMENT (equation->style_element);
	lsm_dom_element_set_attribute (LSM_DOM_ELEMENT (equation->style_element), "displaystyle",
				       equation->compact ? "false" : "true");
	if (pango_font_description_get_weight (equation->desc) >= PANGO_WEIGHT_BOLD) {
		if (pango_font_description_get_style (equation->desc) == PANGO_STYLE_NORMAL)
			lsm_dom_element_set_attribute (style_element, "mathvariant", "bold");
		else
			lsm_dom_element_set_attribute (style_element, "mathvariant", "bold-italic");
	} else {
		if (pango_font_description_get_style (equation->desc) == PANGO_STYLE_NORMAL)
			lsm_dom_element_set_attribute (style_element, "mathvariant", "normal");
		else
			lsm_dom_element_set_attribute (style_element, "mathvariant", "italic");
	}

	lsm_dom_element_set_attribute (style_element, "mathfamily",
				       pango_font_description_get_family (equation->desc));

	value = g_strdup_printf ("%gpt", pango_units_to_double (
			pango_font_description_get_size (equation->desc)));
	lsm_dom_element_set_attribute (style_element, "mathsize", value);
	g_free (value);
	value = g_strdup_printf ("#%02x%02x%02x",
				 GO_COLOR_UINT_R (equation->color),
				 GO_COLOR_UINT_G (equation->color),
				 GO_COLOR_UINT_B (equation->color));
	lsm_dom_element_set_attribute (style_element, "mathcolor", value);
	g_free (value);
	if (equation->view) {
	lsm_dom_view_get_size (equation->view, &width, &height, &baseline);
		equation->parent.width = width / 72.;
		equation->parent.height = height / 72.;
		equation->parent.ascent = baseline / 72.;
		equation->parent.descent = equation->parent.height - equation->parent.ascent;
	}
}

static void
update_equation (GoLasemComponent *equation)
{
	if (equation->itex != NULL && !g_utf8_validate (equation->itex, -1, NULL)) {
		g_free (equation->itex);
		equation->itex = NULL;
	}
	if (equation->itex != NULL) {
		if (equation->itex_element == NULL) {
			if (equation->mathml != NULL)
				g_object_unref (equation->mathml);
			equation->mathml = lsm_dom_implementation_create_document (NULL, "math");
			equation->math_element = LSM_DOM_NODE (lsm_dom_document_create_element (equation->mathml, "math"));
			equation->style_element = LSM_DOM_NODE (lsm_dom_document_create_element (equation->mathml, "mstyle"));
			equation->itex_element = LSM_DOM_NODE (lsm_dom_document_create_element (equation->mathml, "lasem:itex"));
			equation->itex_string = LSM_DOM_NODE (lsm_dom_document_create_text_node (equation->mathml, equation->itex));

			lsm_dom_node_append_child (LSM_DOM_NODE (equation->mathml), equation->math_element);
			lsm_dom_node_append_child (equation->math_element, equation->style_element);
			lsm_dom_node_append_child (equation->style_element, equation->itex_element);
			lsm_dom_node_append_child (equation->itex_element, equation->itex_string);
			if (equation->view != NULL)
				g_object_unref (equation->view);
			equation->view = lsm_dom_document_create_view (equation->mathml);
		} else
			lsm_dom_node_set_node_value (equation->itex_string, equation->itex);
		go_lasem_component_update_style (equation);
	}
}

#ifdef GOFFICE_WITH_GTK

struct LasemEditorState {
	GoLasemComponent *equation;
	GoMathEditor *gme;
	GOFontSel *fontsel;
};

static void
response_cb (GtkWidget *w, int id, struct LasemEditorState *state)
{
	if (id == GTK_RESPONSE_ACCEPT) {
		if (state->equation->desc)
			pango_font_description_free (state->equation->desc);
		state->equation->desc = go_font_sel_get_font_desc (state->fontsel);
		g_free (state->equation->font);
		state->equation->font = pango_font_description_to_string (state->equation->desc);
		state->equation->color = go_font_sel_get_color (state->fontsel);
		g_free (state->equation->itex);
		state->equation->itex = go_math_editor_get_itex (state->gme);
		state->equation->compact = go_math_editor_get_inline (state->gme);
		update_equation (state->equation);
		go_component_emit_changed (GO_COMPONENT (state->equation));
	}
	gtk_widget_destroy (w);
}

static GtkWindow *
go_lasem_component_edit (GOComponent *component)
{
	GtkWidget *label, *notebook, *w, *window;
	GtkContainer *c;
	struct LasemEditorState *state = g_new0 (struct LasemEditorState, 1);

	state->equation = GO_LASEM_COMPONENT (component);
	window = gtk_dialog_new_with_buttons (_("Equation editor"), NULL, 0,
	                                      GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL,
	                                      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
	                                      NULL);
	c = GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (window)));
	notebook = gtk_notebook_new ();
	g_object_set (notebook, "border-width", 6, NULL);
	gtk_container_add (c, notebook);
	label = gtk_label_new (_("Contents"));
	w = go_math_editor_new ();
	state->gme = GO_MATH_EDITOR (w);
	go_math_editor_set_itex (state->gme, state->equation->itex);
	go_math_editor_set_inline (state->gme, state->equation->compact);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), w, label);
	/* add a font selector if needed */
	if (!go_component_get_use_font_from_app (component)) {
		w = g_object_new (GO_TYPE_FONT_SEL, "show-color", TRUE,"border-width", 12, NULL);
		go_font_sel_set_font_desc (GO_FONT_SEL (w), state->equation->desc);
		state->fontsel = (GOFontSel *) w;
		label = gtk_label_new (_("Font"));
		gtk_notebook_append_page (GTK_NOTEBOOK (notebook), w, label);
	}
	/* add a text layout page and show it if not inline */
	gtk_widget_show_all ((GtkWidget *) window);
	if (go_component_get_inline (component))
		gtk_widget_hide (w);
	g_signal_connect (window, "response", G_CALLBACK (response_cb), state);
	g_object_set_data_full (G_OBJECT (window), "state", state, g_free);
	return GTK_WINDOW (window);
}
#endif

static gboolean
go_lasem_component_get_data (GOComponent *component, gpointer *data, int *length,
                             GDestroyNotify *clearfunc, gpointer *user_data)
{
	GoLasemComponent *equation = GO_LASEM_COMPONENT (component);
	if (equation->itex) {
		char *buf;
		char const *start, *end;
		if (equation->compact)
			start = end = "$";
		else {
			start = "\\[";
			end = "\\]";
		}
		buf = g_strconcat (start, equation->itex, end, NULL);
		*data = lsm_itex_to_mathml (buf, strlen (buf));
		g_free (buf);
		*length = strlen (*data);
		*clearfunc = (GDestroyNotify) lsm_itex_free_mathml_buffer;
		*user_data = *data;
		return TRUE;
	}
	return FALSE;
}

static void
go_lasem_component_set_data (GOComponent *component)
{
	GoLasemComponent *equation = GO_LASEM_COMPONENT (component);
	LsmDomNodeList *list;
	LsmDomNode *node;
	unsigned i, max;

	g_object_unref (equation->mathml);
	if (equation->view) {
		g_object_unref (equation->view);
		equation->view = NULL;
	}
	equation->math_element = NULL;
	equation->style_element = NULL;
	equation->mathml = lsm_dom_document_new_from_memory (component->data, component->length, NULL);
	if (equation->mathml == NULL)
		return;
	if (equation->itex == NULL) {
		char buf[component->length + 1];
		memcpy (buf, component->data, component->length);
		buf[component->length] = 0;
		go_mathml_to_itex (buf, &equation->itex, NULL, &equation->compact, NULL);
	}
	equation->math_element = LSM_DOM_NODE (lsm_dom_document_get_document_element (equation->mathml));
	list = lsm_dom_node_get_child_nodes (equation->math_element);
	max = lsm_dom_node_list_get_length (list);
	for (i = 0; i < max; i++) {
		node = lsm_dom_node_list_get_item (list, i);
		if (!strcmp (lsm_dom_node_get_node_name (node), "mstyle"))
			equation->style_element = node;
	}
	equation->view = lsm_dom_document_create_view (equation->mathml);
	go_lasem_component_update_style (equation);
}

static void
go_lasem_component_render (GOComponent *component, cairo_t *cr,
                           double width, double height)
{
	GoLasemComponent *equation = GO_LASEM_COMPONENT (component);
	double zoom;

	if (equation->mathml == NULL || component->height == 0 || component->width == 0)
		return;
	zoom = MAX (width / component->width, height / component->height) / 72.;
	cairo_save (cr);
	cairo_scale (cr,zoom, zoom);
	lsm_dom_view_render (equation->view, cr, 0., 0.);
	cairo_restore (cr);
}

static gboolean
go_lasem_component_set_font (GOComponent *component, PangoFontDescription const *desc)
{
	GoLasemComponent *equation = GO_LASEM_COMPONENT (component);
	if (desc != NULL) {
		g_free (equation->font);
		if (equation->desc)
			pango_font_description_free (equation->desc);
		equation->font = pango_font_description_to_string (desc);
		equation->desc = pango_font_description_copy (desc);
		go_lasem_component_update_style (equation);
	}
	return TRUE;
}

static void
go_lasem_component_init (GOComponent *component)
{
	GoLasemComponent *equation = GO_LASEM_COMPONENT (component);
	PangoFontDescription *desc;
	equation->itex = NULL;

	equation->mathml = lsm_dom_implementation_create_document (NULL, "math");
	equation->math_element = LSM_DOM_NODE (lsm_dom_document_create_element (equation->mathml, "math"));
	equation->style_element = LSM_DOM_NODE (lsm_dom_document_create_element (equation->mathml, "mstyle"));

	lsm_dom_node_append_child (LSM_DOM_NODE (equation->mathml), equation->math_element);
	lsm_dom_node_append_child (equation->math_element, equation->style_element);

	component->resizable = FALSE;
	component->editable = TRUE;
	component->snapshot_type = GO_SNAPSHOT_SVG;
	equation->color = GO_COLOR_BLACK;
	desc = pango_font_description_from_string ("Sans 10");
	go_lasem_component_set_font (component, desc);
	pango_font_description_free (desc);
}

static void
go_lasem_component_set_property (GObject *obj, guint param_id,
                                 GValue const *value, GParamSpec *pspec)
{
	GoLasemComponent *equation = GO_LASEM_COMPONENT (obj);

	switch (param_id) {
		case LASEM_PROP_ITEX:
			g_free (equation->itex);
			equation->itex = g_value_dup_string (value);
			break;
		case LASEM_PROP_COLOR:
			if (!go_color_from_str (g_value_get_string (value), &(equation->color)))
				equation->color = GO_COLOR_BLACK;
			break;
		case LASEM_PROP_FONT: {
			PangoFontDescription *desc = pango_font_description_from_string (g_value_get_string (value));
			go_lasem_component_set_font (GO_COMPONENT (obj), desc);
			pango_font_description_free (desc);
			break;
		}

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
			return;
	}
}

static void
go_lasem_component_get_property (GObject *obj, guint param_id,
                                 GValue *value, GParamSpec *pspec)
{
	GoLasemComponent *equation = GO_LASEM_COMPONENT (obj);

	switch (param_id) {
		case LASEM_PROP_ITEX :
			g_value_set_string (value, equation->itex);
			break;
		case LASEM_PROP_COLOR: {
			char *str = go_color_as_str (equation->color);
			g_value_set_string (value, str);
			g_free (str);
		}
			break;
		case LASEM_PROP_FONT:
			g_value_set_string (value,
			                    go_component_get_use_font_from_app (GO_COMPONENT (obj))?
			                    "Sans 10": equation->font);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
			break;
	}
}

static void
go_lasem_component_finalize (GObject *object)
{
	GoLasemComponent *equation = GO_LASEM_COMPONENT (object);

	if (equation->mathml)
		g_object_unref (equation->mathml);
	if (equation->view)
		g_object_unref (equation->view);
	if (equation->desc)
		pango_font_description_free (equation->desc);
	g_free (equation->itex);
	g_free (equation->font);
	((GObjectClass *) parent_klass)->finalize (object);
}

static void
go_lasem_component_class_init (GOComponentClass *klass)
{
	GObjectClass *obj_klass = (GObjectClass *) klass;

	obj_klass->finalize = go_lasem_component_finalize;
	obj_klass->get_property = go_lasem_component_get_property;
	obj_klass->set_property = go_lasem_component_set_property;

	parent_klass = (GObjectClass*) g_type_class_peek_parent (klass);
	g_object_class_install_property (obj_klass, LASEM_PROP_ITEX,
		g_param_spec_string ("itex",
				     _("Itex markup"),
				     _("Itex markup string"),
				     NULL,
				     GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (obj_klass, LASEM_PROP_COLOR,
		g_param_spec_string ("color",
				      _("Color"),
				      _("Default font color"),
				      "0:0:0:FF",
				      GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (obj_klass, LASEM_PROP_FONT,
		g_param_spec_string ("font",
				      _("Font"),
				      _("Default font"),
				      "Sans 10",
				      GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	klass->get_data = go_lasem_component_get_data;
	klass->set_data = go_lasem_component_set_data;
	klass->render = go_lasem_component_render;
#ifdef GOFFICE_WITH_GTK
	klass->edit = go_lasem_component_edit;
#endif
	klass->set_font = go_lasem_component_set_font;
}

GSF_DYNAMIC_CLASS (GoLasemComponent, go_lasem_component,
	go_lasem_component_class_init, go_lasem_component_init,
	GO_TYPE_COMPONENT)

/*************************************************************************************/

G_MODULE_EXPORT void
go_plugin_init (GOPlugin *plugin, G_GNUC_UNUSED GOCmdContext *cc)
{
	GTypeModule *module;

	module = go_plugin_get_type_module (plugin);
	go_lasem_component_register_type (module);

	go_components_set_mime_suffix ("application/mathml+xml", "*.mml");
}

G_MODULE_EXPORT void
go_plugin_shutdown (G_GNUC_UNUSED GOPlugin *plugin,
		    G_GNUC_UNUSED GOCmdContext *cc)
{
}
