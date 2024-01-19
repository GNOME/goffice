/*
 * go-styled-object.c:
 *
 * Copyright (C) 2009 Jean Brefort (jean.brefort@normalesup.org)
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#include <goffice/goffice-config.h>
#include <goffice/utils/go-styled-object.h>

/**
 * SECTION:go-styled-object
 * @short_description: Objects with style
 *
 * The common GInterface for all objects owning a GOStyle.
 **/


/**
 * GOStyledObjectClass:
 * @base: base class
 * @set_style: sets the object style.
 * @get_style: returns the object current style.
 * @get_auto_style: returns the default style for the object.
 * @style_changed: called when the style changed.
 * @apply_theme: apply the current theme if any to the object style.
 * @get_document: returns the #GODoc associated to the object if any.
 **/

GType
go_styled_object_get_type (void)
{
	static GType go_styled_object_type = 0;

	if (!go_styled_object_type) {
		static GTypeInfo const go_styled_object_info = {
			sizeof (GOStyledObjectClass),	/* class_size */
			NULL,		/* base_init */
			NULL,		/* base_finalize */
		};

		go_styled_object_type = g_type_register_static (G_TYPE_INTERFACE,
			"GOStyledObject", &go_styled_object_info, 0);
	}

	return go_styled_object_type;
}

/**
 * go_styled_object_set_style:
 * @gso: a #GOStyledObject
 * @style: a #GOStyle
 *
 * Sets a new style for @gso, and emits "style-changed" signal. This function
 * does not take ownership of @style.
 *
 * The best way to change the style is to set the "style" property.
 *
 * This function will fail if the new style and the previous style are the same.
 * In that case, the function will always return false:
 * <informalexample>
 *  <programlisting>
 *      style = go_styled_object_get_style (gso);
 *      style->line.width = 2;
 *      size_changed = go_styled_object_set_style (gso, style);
 *  </programlisting>
 * </informalexample>
 * In this sample, the call to go_styled_object_set_style() is just useless. You
 * need to check yourself if you really change the size, call
 * go_styled_object_style_changed() to trigger the "style-changed" event.
 * So the following code is much better:
 * <informalexample>
 *  <programlisting>
 *      style = go_styled_object_get_style (gso);
 *      if (style->line.width != 2.) {
 *	      style->line.width = 2;
 *	      go_styled_object_style_changed (gso);
 *	      size_changed = true;
 *      } else
 *	      size_changed = FALSE;
 *  </programlisting>
 * </informalexample>
 * or even better:
 * <informalexample>
 *  <programlisting>
 *      style = go_style_dup (go_styled_object_get_style (gso));
 *      style->line.width = 2;
 *      size_changed = go_styled_object_set_style (gso, style);
 *      g_object_unref (style);
 *  </programlisting>
 * </informalexample>
 * return value: %TRUE if new style may lead to change of object size, which
 * happens when changing font size for example.
 **/
gboolean
go_styled_object_set_style (GOStyledObject *gso, GOStyle *style)
{
	GOStyledObjectClass *klass = GO_STYLED_OBJECT_GET_CLASS (gso);
	g_return_val_if_fail (klass != NULL, FALSE);
	g_return_val_if_fail (style != NULL, FALSE);
	return (klass->set_style)?
		klass->set_style (gso, style): FALSE;
}

/**
 * go_styled_object_get_style:
 * @gso: a #GOStyledObject
 *
 * Simply an accessor function that returns @gso->style, without referencing it.
 *
 * Returns: (transfer none) (nullable): the styled object's #GOStyle
 **/
GOStyle*
go_styled_object_get_style (GOStyledObject *gso)
{
	GOStyledObjectClass *klass = GO_STYLED_OBJECT_GET_CLASS (gso);
	g_return_val_if_fail (klass != NULL, NULL);
	return (klass->get_style)?
		klass->get_style (gso): NULL;
}

/**
 * go_styled_object_get_auto_style:
 * @gso: a #GOStyledObject
 *
 * This function returns a new style that is initialized with the auto values for @gso.
 * Caller is responsible for the result.
 *
 * Returns: (transfer full) (nullable): a new #GOStyle
 **/
GOStyle*
go_styled_object_get_auto_style (GOStyledObject *gso)
{
	GOStyledObjectClass *klass = GO_STYLED_OBJECT_GET_CLASS (gso);
	g_return_val_if_fail (klass != NULL, NULL);
	return (klass->get_auto_style)?
		klass->get_auto_style (gso): NULL;
}

/**
 * go_styled_object_style_changed:
 * @gso: a #GOStyledObject
 *
 * Called when the style changed. Might emit a signal if meaningful.
 **/
void
go_styled_object_style_changed (GOStyledObject *gso)
{
	GOStyledObjectClass *klass = GO_STYLED_OBJECT_GET_CLASS (gso);
	g_return_if_fail (klass != NULL);
	if (klass->style_changed)
		klass->style_changed (gso);
}

/**
 * go_styled_object_apply_theme:
 * @gso: a #GOStyledObject
 * @style: a #GOStyle that will be themed
 *
 * Apply appropriate theme @style if meaningful, i.e. properties with
 * auto flag set to %TRUE should be changed to default theme value.
 **/
void
go_styled_object_apply_theme (GOStyledObject *gso, GOStyle *style)
{
	GOStyledObjectClass *klass = GO_STYLED_OBJECT_GET_CLASS (gso);
	g_return_if_fail (klass != NULL);
	if (klass->apply_theme)
		klass->apply_theme (gso, style);
}
/**
 * go_styled_object_get_document:
 * @gso: a #GOStyledObject
 *
 * A #GODoc is necessary to store images. If no GODoc is associated with the
 * object, image filling will not be supported.
 * Returns: (transfer none) (nullable): the #GODoc associated with the
 * object if any.
 **/
GODoc *
go_styled_object_get_document (GOStyledObject *gso)
{
	GOStyledObjectClass *klass = GO_STYLED_OBJECT_GET_CLASS (gso);
	g_return_val_if_fail (klass != NULL, NULL);
	return (klass->get_document)?
		klass->get_document (gso): NULL;
}

/**
 * go_styled_object_set_cairo_line:
 * @so: #GOStyledObject
 * @cr: #cairo_t
 *
 * Prepares the cairo context @cr to draw a line according to the
 * item style and canvas scale.
 *
 * Returns: %TRUE if the line is not invisible
 **/
gboolean
go_styled_object_set_cairo_line (GOStyledObject const *so, cairo_t *cr)
{
	GOStyle const *style;

	g_return_val_if_fail (GO_IS_STYLED_OBJECT (so), FALSE);
	style = go_styled_object_get_style (GO_STYLED_OBJECT (so));
	return go_style_set_cairo_line (style, cr);
}

/**
 * go_styled_object_fill:
 * @so: #GOStyledObject
 * @cr: #cairo_t
 * @preserve: whether the current path should be preserved
 *
 * fills the current path according to the
 * item style and canvas scale.
 *
 **/
void
go_styled_object_fill (GOStyledObject const *so, cairo_t *cr, gboolean preserve)
{
	GOStyle const *style;

	g_return_if_fail (GO_IS_STYLED_OBJECT (so));
	style = go_styled_object_get_style (GO_STYLED_OBJECT (so));
	go_style_fill (style, cr, preserve);
}
