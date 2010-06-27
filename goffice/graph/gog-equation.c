/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-equation.c
 *
 * Copyright (C) 2008-2009 Emmanuel Pacaud <emmanuel@gnome.org>
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

#include <lsmmathmldocument.h>
#include <lsmdomparser.h>
#include <lsmmathmlmathelement.h>

#include <goffice/goffice-config.h>
#include <goffice/graph/gog-equation.h>

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

enum {
	EQUATION_PROP_0,
	EQUATION_PROP_ITEX,
	EQUATION_PROP_INLINE_MODE
};

struct _GogEquation {
	GogOutlinedObject base;

	char *itex;
	gboolean inline_mode;
	LsmMathmlDocument *mathml;
};

typedef struct {
	GogOutlinedObjectClass base;
} GogEquationClass;

static GObjectClass *equation_parent_klass;

GType gog_equation_view_get_type (void);

#ifdef GOFFICE_WITH_GTK

static void
cb_equation_buffer_changed (GtkTextBuffer *buffer, GogEquation *equation)
{
	GtkTextIter start;
	GtkTextIter end;

	gtk_text_buffer_get_bounds (buffer, &start, &end);
	g_object_set (G_OBJECT (equation), "itex", gtk_text_buffer_get_text (buffer, &start, &end, FALSE), NULL);
}
static void
cb_inline_mode_check_toggled (GtkToggleButton *button, GogEquation *equation)
{
	g_object_set (G_OBJECT (equation), "inline-mode", gtk_toggle_button_get_active (button), NULL);
}

static void
gog_equation_populate_editor (GogObject *obj,
			      GOEditor *editor,
			      G_GNUC_UNUSED GogDataAllocator *dalloc,
			      GOCmdContext *cc)
{
	GogEquation *equation = GOG_EQUATION (obj);
	GtkBuilder *gui;
	GtkWidget *widget;
	GtkTextBuffer *buffer;
	static guint equation_pref_page = 0;

	gui = go_gtk_builder_new ("gog-equation-prefs.ui", GETTEXT_PACKAGE, cc);
	g_return_if_fail (gui != NULL);

	widget = go_gtk_builder_get_widget (gui, "equation_text");
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
	gtk_text_buffer_set_text (GTK_TEXT_BUFFER (buffer), equation->itex != NULL ? equation->itex: "", -1);
	g_signal_connect (G_OBJECT (buffer), "changed", G_CALLBACK (cb_equation_buffer_changed), obj);

	widget = go_gtk_builder_get_widget (gui, "compact_mode_check");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), equation->inline_mode);
	g_signal_connect (G_OBJECT (widget), "toggled", G_CALLBACK (cb_inline_mode_check_toggled), obj);


	widget = go_gtk_builder_get_widget (gui, "gog_equation_prefs");

	go_editor_add_page (editor, widget, _("Equation"));
	g_object_unref (gui);

	(GOG_OBJECT_CLASS(equation_parent_klass)->populate_editor) (obj, editor, dalloc, cc);

	go_editor_set_store_page (editor, &equation_pref_page);
}

#endif

static void
_update_equation_style (GogEquation *equation, const GOStyle *style)
{
	LsmMathmlStyle *math_style;
	LsmMathmlMathElement *math_element;
	PangoFontDescription *font_description;

	if (equation->mathml == NULL)
		return;

	math_element = lsm_mathml_document_get_root_element (equation->mathml);
	if (math_element == NULL)
		return;

	math_style = lsm_mathml_math_element_get_default_style (math_element);
	if (math_style == NULL)
		return;

	lsm_mathml_style_set_math_color (math_style,
				      GO_COLOR_DOUBLE_R (style->font.color),
				      GO_COLOR_DOUBLE_G (style->font.color),
				      GO_COLOR_DOUBLE_B (style->font.color),
				      GO_COLOR_DOUBLE_A (style->font.color));

	font_description = style->font.font->desc;
	if (font_description != NULL) {
		LsmMathmlVariant math_variant;

		if (pango_font_description_get_weight (font_description) >= PANGO_WEIGHT_BOLD) {
			if (pango_font_description_get_style (font_description) == PANGO_STYLE_NORMAL)
				math_variant = LSM_MATHML_VARIANT_BOLD;
			else
				math_variant = LSM_MATHML_VARIANT_BOLD_ITALIC;
		} else {
			if (pango_font_description_get_style (font_description) == PANGO_STYLE_NORMAL)
				math_variant = LSM_MATHML_VARIANT_NORMAL;
			else
				math_variant = LSM_MATHML_VARIANT_ITALIC;
		}

		lsm_mathml_style_set_math_family (math_style, pango_font_description_get_family (font_description));
		lsm_mathml_style_set_math_size_pt
			(math_style, pango_units_to_double (pango_font_description_get_size (font_description)));
		lsm_mathml_style_set_math_variant (math_style, math_variant);
	}

	lsm_mathml_math_element_set_default_style (math_element, math_style);
}

static void
gog_equation_update (GogObject *obj)
{
	GogEquation *equation = GOG_EQUATION (obj);
	LsmMathmlDocument *mathml;
	GString *itex;
	char *itex_iter;
	char *prev_char = '\0';
	size_t size_utf8;
	unsigned int i;
	int n_unclosed_braces = 0;
	int j;
	gboolean is_blank = TRUE;
	gboolean add_dash = FALSE;

	if (equation->itex != NULL && !g_utf8_validate (equation->itex, -1, NULL)) {
		g_free (equation->itex);
		equation->itex = NULL;
	}

	if (equation->itex != NULL) {
		size_utf8 = g_utf8_strlen (equation->itex, -1);

		if (size_utf8 > 0) {
			for (i = 0, itex_iter = equation->itex;
			     i < size_utf8;
			     i++, itex_iter = g_utf8_next_char (itex_iter)) {
				if (*itex_iter != ' ') {
					is_blank = FALSE;

					if (*itex_iter == '{' && (prev_char == NULL || *prev_char != '\\'))
						n_unclosed_braces++;
					else if (*itex_iter == '}' && (prev_char != NULL || *prev_char != '\\'))
						n_unclosed_braces--;
				}

				prev_char = itex_iter;
			}
			if (prev_char != NULL && (*prev_char == '^' || *prev_char == '_'))
				add_dash = TRUE;

		}
	}

	if (equation->inline_mode)
		itex = g_string_new ("$");
	else
		itex = g_string_new ("$$");
	g_string_append (itex, equation->itex);
	if (add_dash)
		g_string_append_c (itex, '-');
	for (j = 0; j < n_unclosed_braces; j++)
		g_string_append_c (itex, '}');
	if (equation->inline_mode)
		itex = g_string_append (itex, "$");
	else
		itex = g_string_append (itex, "$$");

	mathml = lsm_mathml_document_new_from_itex (itex->str, itex->len, NULL);

	/* Keep the last valid mathml document if the itex -> mathml conversion fails.
	 * It keep the equation from disappearing when the current equation entry is not a
	 * well formed itex expression. */

	if (mathml != NULL) {
		if (lsm_mathml_document_get_root_element (mathml) != NULL || is_blank) {
			if (equation->mathml != NULL)
				g_object_unref (equation->mathml);

			equation->mathml = mathml;

			_update_equation_style (equation,
						go_styled_object_get_style (GO_STYLED_OBJECT (equation)));
		} else
			g_object_unref (mathml);
	}

	g_string_free (itex, TRUE);
}

static void
gog_equation_set_property (GObject *obj, guint param_id,
			   GValue const *value, GParamSpec *pspec)
{
	GogEquation *equation = GOG_EQUATION (obj);

	switch (param_id) {
		case EQUATION_PROP_ITEX:
			g_free (equation->itex);
			equation->itex = g_value_dup_string (value);
			break;
		case EQUATION_PROP_INLINE_MODE:
			equation->inline_mode = g_value_get_boolean (value);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
			return;
	}

	gog_object_emit_changed (GOG_OBJECT (obj), TRUE);
	gog_object_request_update (GOG_OBJECT (obj));
}

static void
gog_equation_get_property (GObject *obj, guint param_id,
			   GValue *value, GParamSpec *pspec)
{
	GogEquation *equation = GOG_EQUATION (obj);

	switch (param_id) {
		case EQUATION_PROP_ITEX :
			g_value_set_string (value, equation->itex);
			break;
		case EQUATION_PROP_INLINE_MODE:
			g_value_set_boolean (value, equation->inline_mode);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
			break;
	}
}

static void
gog_equation_style_changed (GogStyledObject *gso, GOStyle const *new_style)
{
	_update_equation_style (GOG_EQUATION (gso), new_style);
}

static void
gog_equation_init_style (GogStyledObject *gso, GOStyle *style)
{
	style->interesting_fields =
		GO_STYLE_OUTLINE |
		GO_STYLE_FILL |
		GO_STYLE_TEXT_LAYOUT |
		GO_STYLE_FONT;

	gog_theme_fillin_style (gog_object_get_theme (GOG_OBJECT (gso)),
				style, GOG_OBJECT (gso), 0, GO_STYLE_OUTLINE |
				GO_STYLE_FILL | GO_STYLE_FONT);
}

static void
gog_equation_finalize (GObject *object)
{
	GogEquation *equation = GOG_EQUATION (object);

	if (equation->mathml)
		g_object_unref (equation->mathml);
	g_free (equation->itex);
	((GObjectClass *) equation_parent_klass)->finalize (object);
}

static void
gog_equation_class_init (GogEquationClass *klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) klass;
	GogObjectClass *gog_klass = (GogObjectClass *) klass;
	GogStyledObjectClass *styled_klass = (GogStyledObjectClass *) klass;

	equation_parent_klass = g_type_class_peek_parent (klass);

	gobject_klass->finalize     = gog_equation_finalize;
	gobject_klass->set_property = gog_equation_set_property;
	gobject_klass->get_property = gog_equation_get_property;
	styled_klass->style_changed = gog_equation_style_changed;
#ifdef GOFFICE_WITH_GTK
	gog_klass->populate_editor  = gog_equation_populate_editor;
#endif
	gog_klass->update	    = gog_equation_update;

	g_object_class_install_property (gobject_klass, EQUATION_PROP_ITEX,
		g_param_spec_string ("itex",
				     _("Itex markup"),
				     _("Itex markup string"),
				     NULL,
				     GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, EQUATION_PROP_INLINE_MODE,
		g_param_spec_boolean ("inline-mode",
				      _("Inline mode"),
				      _("Inline mode selection"),
				      FALSE,
				      GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	gog_klass->view_type		= gog_equation_view_get_type ();
	styled_klass->init_style 	= gog_equation_init_style;
}

static void
gog_equation_init (GogEquation *equation)
{
	equation->itex = NULL;
	equation->inline_mode = FALSE;
	equation->mathml = NULL;
}

GSF_CLASS (GogEquation, gog_equation,
	   gog_equation_class_init, gog_equation_init,
	   GOG_TYPE_OUTLINED_OBJECT);

typedef struct {
	GogOutlinedView		 base;

	LsmMathmlView 		*mathml_view;
} GogEquationView;

typedef GogOutlinedViewClass	GogEquationViewClass;

#define GOG_TYPE_EQUATION_VIEW	(gog_equation_view_get_type ())
#define GOG_EQUATION_VIEW(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_EQUATION_VIEW, GogEquationView))
#define GOG_IS_EQUATION_VIEW(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_EQUATION_VIEW))

static GogViewClass *equation_view_parent_klass;

static void
gog_equation_view_size_request (GogView *view,
				GogViewRequisition const *available,
				GogViewRequisition *requisition)
{
	double height, width;
	GogEquation *equation;
	GogEquationView *equation_view;
	GOGeometryAABR aabr;
	GOGeometryOBR obr;

	equation = GOG_EQUATION (view->model);
	equation_view = GOG_EQUATION_VIEW (view);

	if (equation->mathml == NULL) {
		requisition->w = 0.0;
		requisition->h = 0.0;
		return;
	}

	lsm_dom_view_set_document (LSM_DOM_VIEW (equation_view->mathml_view),
				   LSM_DOM_DOCUMENT (equation->mathml));
	lsm_dom_view_get_size (LSM_DOM_VIEW (equation_view->mathml_view), &width, &height);

	obr.w = gog_renderer_pt2r_x (view->renderer, width);
	obr.h = gog_renderer_pt2r_y (view->renderer, height);
	obr.alpha = -go_styled_object_get_style (GO_STYLED_OBJECT(equation))->text_layout.angle * M_PI / 180.0;

	go_geometry_OBR_to_AABR (&obr, &aabr);
	requisition->w = aabr.w;
	requisition->h = aabr.h;

	equation_view_parent_klass->size_request (view, available, requisition);
}

static void
gog_equation_view_render (GogView *view,
			  GogViewAllocation const *bbox)
{
	GogEquation *equation;
	GogEquationView *equation_view;

	equation_view_parent_klass->render (view, bbox);

	equation = GOG_EQUATION (view->model);
	equation_view = GOG_EQUATION_VIEW (view);

	if (equation->mathml == NULL)
		return;

	lsm_dom_view_set_document (LSM_DOM_VIEW (equation_view->mathml_view),
				   LSM_DOM_DOCUMENT (equation->mathml));
	gog_renderer_push_style (view->renderer, go_styled_object_get_style (GO_STYLED_OBJECT(equation)));
	gog_renderer_draw_equation (view->renderer, equation_view->mathml_view,
				    view->residual.x + view->residual.w / 2., view->residual.y + view->residual.h / 2.);
	gog_renderer_pop_style (view->renderer);
}

static void
gog_equation_view_finalize (GObject *object)
{
	GogEquationView *view = GOG_EQUATION_VIEW (object);

	if (view->mathml_view != NULL)
		g_object_unref (view->mathml_view);
	((GObjectClass *) equation_view_parent_klass)->finalize (object);
}

static void
gog_equation_view_class_init (GogEquationViewClass *gview_klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) gview_klass;
	GogViewClass *view_klass   = (GogViewClass *) gview_klass;

	equation_view_parent_klass = g_type_class_peek_parent (gview_klass);

	gobject_klass->finalize	   = gog_equation_view_finalize;
	view_klass->size_request   = gog_equation_view_size_request;
	view_klass->render	   = gog_equation_view_render;
}

static void
gog_equation_view_init (GObject *object)
{
	GogEquationView *view = GOG_EQUATION_VIEW (object);

	view->mathml_view = lsm_mathml_view_new (NULL);
}

GSF_CLASS (GogEquationView, gog_equation_view,
	   gog_equation_view_class_init, gog_equation_view_init,
	   GOG_TYPE_OUTLINED_VIEW)

