/*
 * gog-styled-object.c : A base class for objects that have associated styles
 *
 * Copyright (C) 2003-2004 Jody Goldberg (jody@gnome.org)
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
#include <gsf/gsf-impl-utils.h>
#include <string.h>

enum {
	STYLED_OBJECT_PROP_0,
	STYLED_OBJECT_PROP_STYLE
};

enum {
	STYLE_CHANGED,
	LAST_SIGNAL
};

/*
 * GogStyle:
 * This is a mere copy of GOStyle but is needed for file
 * format compatibility
 */
struct _GogStyle {
	GOStyle base;
};
typedef GObjectClass GogStyleClass;
GType gog_style_get_type (void);
GSF_CLASS (GogStyle, gog_style,
	   NULL, NULL,
	   GO_TYPE_STYLE);

/**
 * GogStyledObjectClass:
 * @base: base class
 * @init_style: intiializes the style.
 * @style_changed: implemens the style-changed signal
 **/

static void gog_styled_object_style_changed (GOStyledObject *obj);

static gulong gog_styled_object_signals [LAST_SIGNAL] = { 0, };
static GObjectClass *parent_klass;

static void
gog_styled_object_document_changed (GogObject *obj, GODoc *doc)
{
	GogStyledObject *gso = GOG_STYLED_OBJECT (obj);
	GOStyle *style = gso->style;
	if ((style->interesting_fields & GO_STYLE_FILL) &&
	    (style->fill.type == GO_STYLE_FILL_IMAGE) &&
	    (style->fill.image.image != NULL)) {
		GOImage *image;
		char const *cur_id = go_image_get_name (style->fill.image.image);
		char *id = NULL;
		/* remove the (nnn) modifier if any */
		if (cur_id) {
			int i = strlen (cur_id) - 1;
			id = g_strdup (cur_id);
			if (id[i] == ')') {
				i--;
				while (id[i] >= '0' && id[i] <= '9')
					i--;
				if (id[i] == '(')
					id[i] = 0;
			}
		}
		image = go_doc_add_image (doc, id, style->fill.image.image);
		g_free (id);
		if (image != style->fill.image.image) {
			g_object_unref (style->fill.image.image);
			style->fill.image.image = g_object_ref (image);
		}
	}
}

static void
gog_styled_object_set_property (GObject *obj, guint param_id,
				GValue const *value, GParamSpec *pspec)
{
	GogStyledObject *gso = GOG_STYLED_OBJECT (obj);
	gboolean resize = FALSE;

	switch (param_id) {

	case STYLED_OBJECT_PROP_STYLE :
		resize = go_styled_object_set_style (GO_STYLED_OBJECT (gso),
	      			g_value_get_object (value));
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
	gog_object_emit_changed (GOG_OBJECT (obj), resize);
}

static void
gog_styled_object_get_property (GObject *obj, guint param_id,
				GValue *value, GParamSpec *pspec)
{
	GogStyledObject *gso = GOG_STYLED_OBJECT (obj);

	switch (param_id) {
	case STYLED_OBJECT_PROP_STYLE :
		g_value_set_object (value, gso->style);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
gog_styled_object_finalize (GObject *obj)
{
	GogStyledObject *gso = GOG_STYLED_OBJECT (obj);

	if (gso->style != NULL) {
		g_object_unref (gso->style);
		gso->style = NULL;
	}

	(*parent_klass->finalize) (obj);
}

#ifdef GOFFICE_WITH_GTK
static void
styled_object_populate_editor (GogObject *gobj,
			       GOEditor *editor,
			       GogDataAllocator *dalloc,
		      GOCmdContext *cc)
{
	GOStyledObject *gso = GO_STYLED_OBJECT (gobj);
	GOStyle *style = go_style_dup (go_styled_object_get_style (gso));

	if (style->interesting_fields != 0) {
		GOStyle *default_style = go_styled_object_get_auto_style (gso);
		go_style_populate_editor (style, editor, default_style, cc, G_OBJECT (gso), TRUE);
		g_object_unref (default_style);
	}
	g_object_unref (style);

	(GOG_OBJECT_CLASS(parent_klass)->populate_editor) (gobj, editor, dalloc, cc);
}
#endif

static void
gog_styled_object_parent_changed (GogObject *obj, gboolean was_set)
{
	GogObjectClass *gog_object_klass = GOG_OBJECT_CLASS (parent_klass);
	if (was_set) {
		GogStyledObject *gso = GOG_STYLED_OBJECT (obj);
		gog_theme_fillin_style (gog_object_get_theme (GOG_OBJECT (gso)),
			gso->style, GOG_OBJECT (gso), 0, GO_STYLE_ALL);
		go_styled_object_apply_theme (GO_STYLED_OBJECT (gso), gso->style);
	}
	gog_object_klass->parent_changed (obj, was_set);
}

static void
gog_styled_object_init_style (GogStyledObject *gso, GOStyle *style)
{
	style->interesting_fields = GO_STYLE_OUTLINE | GO_STYLE_FILL; /* default */
	gog_theme_fillin_style (gog_object_get_theme (GOG_OBJECT (gso)),
		style, GOG_OBJECT (gso), 0, style->interesting_fields);
}

static void
gog_styled_object_class_init (GogObjectClass *gog_klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) gog_klass;
	GogStyledObjectClass *style_klass = (GogStyledObjectClass *) gog_klass;

	/**
	 * GogStyledObject::style-changed:
	 * @object: the object on which the signal is emitted
	 * @style: The new #GOStyle
	 *
	 * The ::style-changed signal is emitted after the style has been changed
	 * by a call to go_styled_object_set_style().
	 **/
	gog_styled_object_signals [STYLE_CHANGED] = g_signal_new ("style-changed",
		G_TYPE_FROM_CLASS (gog_klass),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GogStyledObjectClass, style_changed),
		NULL, NULL,
		g_cclosure_marshal_VOID__OBJECT,
		G_TYPE_NONE,
		1, G_TYPE_OBJECT);

	parent_klass = g_type_class_peek_parent (gog_klass);

	gobject_klass->set_property = gog_styled_object_set_property;
	gobject_klass->get_property = gog_styled_object_get_property;
	gobject_klass->finalize	    = gog_styled_object_finalize;
#ifdef GOFFICE_WITH_GTK
	gog_klass->populate_editor  = styled_object_populate_editor;
#endif
	gog_klass->parent_changed   = gog_styled_object_parent_changed;
	gog_klass->document_changed = gog_styled_object_document_changed;
	style_klass->init_style	    = gog_styled_object_init_style;

	g_object_class_install_property (gobject_klass, STYLED_OBJECT_PROP_STYLE,
		g_param_spec_object ("style",
			_("Style"),
			_("A pointer to the GOStyle object"),
			gog_style_get_type (),
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
}

static void
gog_styled_object_init (GogStyledObject *gso)
{
	gso->style = GO_STYLE (g_object_new (gog_style_get_type (), NULL)); /* use the defaults */
}

static gboolean
gog_styled_object_set_style (GOStyledObject *gso,
			     GOStyle *style)
{
	gboolean resize;
	GogStyledObject *ggso;

	g_return_val_if_fail (GO_IS_STYLED_OBJECT (gso), FALSE);
	ggso = GOG_STYLED_OBJECT (gso);
	if (ggso->style == style)
		return FALSE;
	style = go_style_dup (style);

	/* which fields are we interested in for this object */
	go_styled_object_apply_theme (gso, style);
	resize = go_style_is_different_size (ggso->style, style);
	if (ggso->style != NULL)
		g_object_unref (ggso->style);
	ggso->style = style;
	go_styled_object_style_changed (gso);

	return resize;
}

static GOStyle *
gog_styled_object_get_style (GOStyledObject *gso)
{
	g_return_val_if_fail (GO_IS_STYLED_OBJECT (gso), NULL);
	return GOG_STYLED_OBJECT (gso)->style;
}

static GOStyle *
gog_styled_object_get_auto_style (GOStyledObject *gso)
{
	GOStyle *res = go_style_dup (GOG_STYLED_OBJECT (gso)->style);
	go_style_force_auto (res);
	go_styled_object_apply_theme (gso, res);
	return res;
}

static void
gog_styled_object_apply_theme (GOStyledObject *gso, GOStyle *style)
{
	GogGraph const *graph = gog_object_get_graph (GOG_OBJECT (gso));
	if (graph != NULL) {
		GogStyledObjectClass *klass = GOG_STYLED_OBJECT_GET_CLASS (gso);
		(klass->init_style) (GOG_STYLED_OBJECT (gso), style);
	}
}

static void
gog_styled_object_style_changed (GOStyledObject *gso)
{
	g_signal_emit (G_OBJECT (gso),
		gog_styled_object_signals [STYLE_CHANGED], 0, GOG_STYLED_OBJECT (gso)->style);
}

static GODoc*
gog_styled_object_get_document (GOStyledObject *gso)
{
	GODoc *doc = NULL;
	GogGraph *graph = gog_object_get_graph (GOG_OBJECT (gso));
	g_object_get (graph, "document", &doc, NULL);
	return doc;
}

static void
gog_styled_object_so_init (GOStyledObjectClass *iface)
{
	iface->set_style = gog_styled_object_set_style;
	iface->get_style = gog_styled_object_get_style;
	iface->get_auto_style = gog_styled_object_get_auto_style;
	iface->style_changed = gog_styled_object_style_changed;
	iface->apply_theme = gog_styled_object_apply_theme;
	iface->get_document = gog_styled_object_get_document;
}

GSF_CLASS_FULL (GogStyledObject, gog_styled_object, NULL, NULL,
	   gog_styled_object_class_init, NULL, gog_styled_object_init,
	   GOG_TYPE_OBJECT, G_TYPE_FLAG_ABSTRACT,
	   GSF_INTERFACE (gog_styled_object_so_init, GO_TYPE_STYLED_OBJECT))

/**
 * gog_style_new: (skip)
 *
 * Returns: (transfer full): the new #GogStyle
 **/
GogStyle *
gog_style_new (void)
{
	return g_object_new (gog_style_get_type (), NULL);
}
