/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-theme.c :
 *
 * Copyright (C) 2003-2004 Jody Goldberg (jody@gnome.org)
 * Copyright (C) 2010 Jean Brefort (jean.brefort@normalesup.org)
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
#include <goffice/goffice-priv.h>
#include <goffice/graph/gog-theme.h>
#include <goffice/graph/gog-object.h>
#include <goffice/utils/go-color.h>
#include <goffice/utils/go-gradient.h>
#include <goffice/utils/go-units.h>
#include <goffice/utils/go-marker.h>
#include <gsf/gsf-input-gio.h>

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>
#include <string.h>

/**
 * SECTION:gog-theme
 * @short_description: a list of default styles to apply to appropriate graph elements.
 * 
 * The library provides two hard coded themes, "Default", and "Guppi". Other themes
 * are described in files, some of which might be distributed with the library.
 * 
 * A file defining a theme is an xml file with a &lt;GogTheme&gt; root node. The contents
 * must be: _name|name+, _description?|description*, GOStyle+.
 * 
 * _name and name nodes:
 *
 * The _name node should be used for themes distributed with goffice, localized
 * names will be in *.po files and only the default name for "C" locale needs to
 * be there. Other files need at least one name node for the default name, and
 * might have some translated names with an appropriate "xml:lang" attribute.
 * 
 * _description and description nodes:
 *
 * These work just like name nodes. The difference is that no description node is
 * mandatory. A theme can work perfectly without a description.
 *
 * GOStyle nodes:
 *
 * These nodes actually define the theme. Attributes and contents are:
 * attributes: class, role.
 * contents: (line|outline)?, fill?, marker?, font?, text_layout?
 *
 * The attributes define the target for the style. You might have a class and
 * a role attribute, or just one of them. If the role attribute is given, the class attribute,
 * if given, represents the class of the parent object.
 * A GOStyle node with no class or role will be used
 * as default when another style is missing. If several such nodes exist, the
 * last one will be used. If no default style exists, the "Default" theme is applied for
 * missing nodes.
 * The list of GOStyle nodes might be:
 * <table style="border-spacing:1cm 2mm">
 * <thead><tr><td>class</td><td>role</td><td>contents</td><td>comments</td></tr></thead>
 * <tr><td>GogGraph</td><td></td><td>outline, fill</td></tr>
 * <tr><td>GogGraph</td><td>Title</td><td>outline, fill, font, text_layout</td><td>The graph title</td></tr>
 * <tr><td>GogChart</td><td></td><td>outline, fill</td></tr>
 * <tr><td>GogChart</td><td>Title</td><td>outline, fill, font, text_layout</td><td>The chart title</td></tr>
 * <tr><td>GogLegend</td><td></td><td>outline, fill, font</td></tr>
 * <tr><td>GogAxis</td><td></td><td>line, font, text_layout</td></tr>
 * <tr><td>GogAxisLine</td><td></td><td>line, font</td></tr>
 * <tr><td>GogGrid</td><td></td><td>outline, fill</td><td>GogGrid is actually the back plane</td></tr>
 * <tr><td> </td><td>MajorGrid</td><td>line, fill</td></tr>
 * <tr><td> </td><td>MinorGrid</td><td>line, fill</td></tr>
 * <tr><td>GogLabel</td><td></td><td>outline, fill, font, text_layout</td></tr>
 * <tr><td>GogSeries</td><td></td><td>line, fill, marker</td><td>One is needed for each entry in the palette</td></tr>
 * <tr><td>GogTrendLine</td><td></td><td>line, fill</td></tr>
 * <tr><td>GogReqEqn</td><td></td><td>line, fill, font, text_layout</td></tr>
 * </table>
 *
 * The line and outline nodes are actually the same so using line in place of outline is
 * not an issue. A color is specified either using the format RR:GG::BB:AA or a string as defined
 * in rgb.txt, so black can be specified as "black" or "00:00:00:FF".
 *
 * "line" or "outline" node:
 *
 * <table style="border-spacing:1cm 2mm">
 * <thead><tr><td>attribute</td><td>value</td></tr><td>comments</td></thead>
 * <tr><td>dash</td><td>one of "none", "solid", "s-dot", "s-dash-dot", "s-dash-dot-dot", "dash-dot-dot-dot", "dot",
 * "s-dash", "dash", "l-dash", "dash-dot", or "dash-dot-dot"</td></tr>
 * <tr><td>color</td><td>any color as described above</td></tr>
 * <tr><td>width</td><td>float</td><td>not always taken into account</td></tr>
 * </table>
 *
 * "fill" node
 *
 * contents: (pattern|gradient)?
 * <table style="border-spacing:1cm 2mm">
 * <thead><tr><td>attribute</td><td>value</td><td>comments</td></tr></thead>
 * <tr><td>type</td><td>one of "none", "pattern", or "gradient"</td></tr>
 * </table>
 *
 * "pattern" node
 *
 * Should be included in the fill node if type is pattern.
 * <table style="border-spacing:1cm 2mm">
 * <thead><tr><td>attribute</td><td>value</td></tr></thead>
 * <tr><td>type</td><td>one of "solid", "grey75", "grey50", "grey25", "grey12.5",
 * "grey6.25", "horiz", "vert", "rev-diag", "diag", "diag-cross", "thick-diag-cross",
 * "thin-horiz", "thin-vert", "thin-rev-diag", "thin-diag", "thin-horiz-cross",
 * "thin-diag-cross", "foreground-solid", "small-circles","semi-circles", "thatch",
 * "large-circles", or "bricks"</td></tr>
 * <tr><td>fore</td><td>any color as described above</td></tr>
 * <tr><td>back</td><td>any color as described above</td></tr>
 * </table>
 *
 * "gradient" node
 *
 * Should be included in the fill node if type is gradient.
 * <table style="border-spacing:1cm 2mm">
 * <thead><tr><td>attribute</td><td>value</td><td>comments</td></tr></thead>
 * <tr><td>direction</td><td>one of "n-s", "s-n", "n-s-mirrored", "s-n-mirrored",
 * "w-e", "e-w", "w-e-mirrored", "e-w-mirrored", "nw-se", "se-nw", "nw-se-mirrored",
 * "se-nw-mirrored", "ne-sw", "sw-ne", "sw-ne-mirrored", or "ne-sw-mirrored" </td><td></td></tr>
 * <tr><td>start_color</td><td>any color as described above</td><td></td></tr>
 * <tr><td>brightness</td><td>float</td><td>meaningful only for monocolor gradients</td></tr>
 * <tr><td>end_color</td><td>any color as described above</td><td>meaningful only for bicolor gradients</td></tr>
 * </table>
 *
 * "marker" node
 *
 * <table style="border-spacing:1cm 2mm">
 * <thead><tr><td>attribute</td><td>value</td><td>comments</td></tr></thead>
 * <tr><td>shape</td><td>one of "none", "square", "diamond", "triangle-down",
 * "triangle-up", "triangle-right", "triangle-left", "circle", "x", "cross",
 * "asterisk", "bar", "half-bar", "butterfly", "hourglass", or "lefthalf-bar"</td><td></td></tr>
 * <tr><td>fill-color</td><td>any color as described above</td><td></td></tr>
 * <tr><td>outline-color</td><td>any color as described above</td><td></td></tr>
 * <tr><td>size</td><td>float</td><td>not always taken into account</td></tr>
 * </table>
 *
 * "font" node
 * <table style="border-spacing:1cm 2mm">
 * <thead><tr><td>attribute</td><td>value</td></tr></thead>
 * <tr><td>color</td><td>any color as described above</td></tr>
 * <tr><td>font</td><td>a string describing the font such as "Sans 10"</td></tr>
 * </table>
 *
 * "text_layout" node
 * <table style="border-spacing:1cm 2mm">
 * <thead><tr><td>attribute</td><td>value</td><td>comments</td></tr></thead>
 * <tr><td>angle</td><td>float</td><td>expressed in degrees</td></tr>
 * </table>
 **/

typedef void (*GogThemeStyleMap) (GOStyle *style, unsigned ind, GogTheme const *theme);

typedef struct {
	/* If role is non-null, klass_name specifies the container class,
	 * If role is null, klass_name specifies object type */
	char const 	 *klass_name;
	char const 	 *role_id;
	GOStyle   	 *style;
	GogThemeStyleMap  map;
} GogThemeElement;

struct _GogTheme {
	GObject	base;

	char 		*name;
	char 		*local_name;
	char 		*description;
	GHashTable	*elem_hash_by_role;
	GHashTable	*elem_hash_by_class;
	GHashTable	*class_aliases;
	GOStyle	*default_style;
	GPtrArray	*palette;
};

typedef GObjectClass GogThemeClass;

static GObjectClass *parent_klass;

static GSList	    *themes;
static GogTheme	    *default_theme = NULL;
static GHashTable   *global_class_aliases = NULL;

static void
gog_theme_element_free (GogThemeElement *elem)
{
	g_object_unref (elem->style);
	g_free (elem);
}

static guint
gog_theme_element_hash (GogThemeElement const *elem)
{
	return g_str_hash (elem->role_id);
}

static gboolean
gog_theme_element_eq (GogThemeElement const *a, GogThemeElement const *b)
{
	if (!g_str_equal (a->role_id, b->role_id))
		return FALSE;
	if (a->klass_name == NULL)
		return b->klass_name == NULL;
	if (b->klass_name == NULL)
		return FALSE;
	return g_str_equal (a->klass_name, b->klass_name);
}

static void
gog_theme_finalize (GObject *obj)
{
	GogTheme *theme = GOG_THEME (obj);

	themes = g_slist_remove (themes, theme);

	g_free (theme->name); theme->name = NULL;
	g_free (theme->local_name); theme->local_name = NULL;
	g_free (theme->description); theme->description = NULL;
	if (theme->elem_hash_by_role)
		g_hash_table_destroy (theme->elem_hash_by_role);
	if (theme->elem_hash_by_class)
		g_hash_table_destroy (theme->elem_hash_by_class);
	if (theme->class_aliases)
		g_hash_table_destroy (theme->class_aliases);
	if (theme->palette) {
		unsigned i;
		for (i = 0; i < theme->palette->len; i++)
			g_object_unref (g_ptr_array_index (theme->palette, i));
		g_ptr_array_free (theme->palette, TRUE);
	}

	(parent_klass->finalize) (obj);
}

static void
gog_theme_class_init (GogThemeClass *klass)
{
	GObjectClass *gobject_klass   = (GObjectClass *) klass;

	parent_klass = g_type_class_peek_parent (klass);
	gobject_klass->finalize	    = gog_theme_finalize;
}

static void
gog_theme_init (GogTheme *theme)
{
	theme->elem_hash_by_role =
		g_hash_table_new_full ((GHashFunc) gog_theme_element_hash,
				       (GCompareFunc) gog_theme_element_eq,
					NULL, (GDestroyNotify) gog_theme_element_free);

	theme->elem_hash_by_class =
		g_hash_table_new_full (g_str_hash, g_str_equal,
				       NULL, (GDestroyNotify) gog_theme_element_free);

	theme->class_aliases =
		g_hash_table_new (g_str_hash, g_str_equal);
}

GSF_CLASS (GogTheme, gog_theme,
	   gog_theme_class_init, gog_theme_init,
	   G_TYPE_OBJECT)


static GogThemeElement *
gog_theme_find_element (GogTheme const *theme, GogObject const *obj)
{
	GogThemeElement *elem = NULL;
	GObjectClass *klass = NULL;	/* make gcc happy */
	char const *name;

	if (theme == NULL)
		theme = default_theme;
	g_return_val_if_fail (theme != NULL, NULL);

	/* FIXME: Restore hash search caching. */

	/* Search by role */
	if (elem == NULL && obj->role != NULL && obj->parent != NULL) {
		GogThemeElement key;

		/* Search by specific role */
		key.role_id = obj->role->id;
		key.klass_name = G_OBJECT_TYPE_NAME (obj->parent);
		elem = g_hash_table_lookup (theme->elem_hash_by_role, &key);
	}

	if (elem == NULL && obj->role != NULL) {
		GogThemeElement key;

		/* Search by generic role */
		key.role_id = obj->role->id;
			key.klass_name = NULL;
			elem = g_hash_table_lookup (theme->elem_hash_by_role, &key);
	}

	/* Still not found ? Search by object type */
	if (elem == NULL) {
		klass = G_OBJECT_GET_CLASS (obj);
		do {
			name = G_OBJECT_CLASS_NAME (klass);

			elem = g_hash_table_lookup (	/* Is the type known ? */
				theme->elem_hash_by_class, name);
			if (elem == NULL) {		/* Is this a local alias ? */
				name = g_hash_table_lookup (
					theme->class_aliases, name);
				if (name != NULL)
					elem = g_hash_table_lookup (
						theme->elem_hash_by_class, name);
			}
			if (elem == NULL &&
			    global_class_aliases != NULL) { /* Is this a global alias */
				name = g_hash_table_lookup (
					global_class_aliases, G_OBJECT_CLASS_NAME (klass));
				if (name != NULL)
					elem = g_hash_table_lookup (
						theme->elem_hash_by_class, name);
			}
		} while (elem == NULL &&
			 (klass = g_type_class_peek_parent (klass)) != NULL);
	}

	/* still not found, use default theme */
	if (elem == NULL && theme != default_theme)
		elem = gog_theme_find_element (default_theme, obj);

	return elem;
}

/**
 * gog_theme_fillin_style :
 * @theme: #GogTheme
 * @style: #GOStyle to initialize
 * @obj: #GogObject The object associated with @style
 * @ind: an optional index
 * @relevant_fields: GOStyleFlag
 *
 * Fill in the auto aspects of @style based on @theme 's element for objects of
 * type/role similar to @obj with index @ind.  If @relevant_fields is GO_STYLE_ALL,
 * fillin the entire style, not just the auto portions included in @relevant_fields.
 **/
void
gog_theme_fillin_style (GogTheme const *theme,
			GOStyle *style,
			GogObject const *obj,
			int ind,
			GOStyleFlag relevant_fields)
{
	GogThemeElement *elem = gog_theme_find_element (theme, obj);

	g_return_if_fail (elem != NULL);

	if (relevant_fields == GO_STYLE_ALL)
		go_style_assign (style, elem->style);
	else
		go_style_apply_theme (style, elem->style, relevant_fields);

/* FIXME FIXME FIXME we should handle the applicability here not in the map */
	/* ensure only relevant fields are changed */
	if (ind >= 0 && elem->map) {
		/* ensure only relevant fields are changed */
		GOStyleFlag flags = style->disable_theming;
		style->disable_theming = GO_STYLE_ALL ^ relevant_fields;
		(elem->map) (style, ind, theme);
		style->disable_theming = flags;
	}
}

static GogTheme *
gog_theme_new (char const *name)
{
	GogTheme *theme = g_object_new (GOG_TYPE_THEME, NULL);
	theme->name = g_strdup (name);
	return theme;
}

#if 0
void
gog_theme_register_file (char const *name, char const *file)
{
	GogTheme *theme = gog_theme_new (name);
	theme->load_from_file = g_strdup (file);
}

static void
gog_theme_add_alias (GogTheme *theme, char const *from, char const *to)
{
	g_hash_table_insert (theme->class_aliases, (gpointer)from, (gpointer)to);
}
#endif

static void
gog_theme_add_element (GogTheme *theme, GOStyle *style,
		       GogThemeStyleMap	map,
		       char const *klass_name, char const *role_id)
{
	GogThemeElement *elem;

	g_return_if_fail (theme != NULL);

	elem = g_new0 (GogThemeElement, 1);
	elem->klass_name = klass_name;
	elem->role_id  = role_id;
	elem->style = style;
	elem->map   = map;

	/* Never put an element into both by_role_id & by_class_name */
	if (role_id != NULL) {
		if (g_hash_table_lookup (theme->elem_hash_by_role, (gpointer)elem) == NULL)
			g_hash_table_insert (theme->elem_hash_by_role,
				(gpointer)elem, elem);
		else {
			g_object_unref (style);
			g_free (elem);
		}
	} else if (klass_name != NULL) {
		if (g_hash_table_lookup (theme->elem_hash_by_class, (gpointer)klass_name) == NULL)
			g_hash_table_insert (theme->elem_hash_by_class,
				(gpointer)klass_name, elem);
		else {
			g_object_unref (style);
			g_free (elem);
		}

	} else {
		if (theme->default_style)
			g_object_unref (theme->default_style);
		theme->default_style = style;
		g_free (elem);
	}
}

/**
 * gog_theme_get_name:
 * @theme: a #GogTheme
 *
 * Returns: the GogTheme name.
 **/

char const *
gog_theme_get_name (GogTheme const *theme)
{
	g_return_val_if_fail (GOG_IS_THEME (theme), "");
	return theme->name;
}

/**
 * gog_theme_get_local_name:
 * @theme: a #GogTheme
 *
 * Returns: the localized GogTheme name.
 **/

char const *
gog_theme_get_local_name (GogTheme const *theme)
{
	g_return_val_if_fail (GOG_IS_THEME (theme), "");
	return (theme->local_name)? theme->local_name: _(theme->name);
}

/**
 * gog_theme_get_descrition:
 * @theme: a #GogTheme
 *
 * Returns: the localized GogTheme decription.
 **/

char const *
gog_theme_get_description (GogTheme const *theme)
{
	g_return_val_if_fail (GOG_IS_THEME (theme), "");
	return theme->description;
}

/**************************************************************************/

static void
map_marker (GOStyleMark *mark, unsigned shape, unsigned palette_index,
	    GOColor const *palette)
{
	static GOMarkerShape const shape_palette [] = {
		GO_MARKER_DIAMOND,	GO_MARKER_SQUARE,
		GO_MARKER_TRIANGLE_UP,	GO_MARKER_X,
		GO_MARKER_ASTERISK,	GO_MARKER_CIRCLE,
		GO_MARKER_CROSS,	GO_MARKER_HALF_BAR,
		GO_MARKER_BAR
	};
	static gboolean const shape_is_fill_transparent [] = {
		TRUE,	TRUE,
		TRUE,	FALSE,
		FALSE,	TRUE,
		FALSE,	TRUE,
		TRUE
	};

	if (shape >= G_N_ELEMENTS (shape_palette))
		shape %= G_N_ELEMENTS (shape_palette);

	if (mark->auto_shape)
		go_marker_set_shape (mark->mark, shape_palette [shape]);
	if (mark->auto_outline_color)
		go_marker_set_outline_color (mark->mark,
			palette [palette_index]);
	if (mark->auto_fill_color)
		go_marker_set_fill_color (mark->mark,
			shape_is_fill_transparent [shape]
			? palette [palette_index] : 0);
}

static void
map_area_series_solid_default (GOStyle *style, unsigned ind, G_GNUC_UNUSED GogTheme const *theme)
{
	static GOColor const palette [] = {
		0x9c9cffff, 0x9c3163ff, 0xffffceff, 0xceffffff, 0x630063ff,
		0xff8080ff, 0x0063ceff, 0xceceffff, 0x000080ff, 0xff00ffff,
		0xffff00ff, 0x00ffffff, 0x800080ff, 0x800000ff, 0x008080ff,
		0x0000ffff, 0x00ceffff, 0xceffffff, 0xceffceff, 0xffff9cff,
		0x9cceffff, 0xff9cceff, 0xce9cffff, 0xffce9cff, 0x3163ffff,
		0x31ceceff, 0x9cce00ff, 0xffce00ff, 0xff9c00ff, 0xff6300ff,
		0x63639cff, 0x949494ff, 0x003163ff, 0x319c63ff, 0x003100ff,
		0x313100ff, 0x9c3100ff, 0x9c3163ff, 0x31319cff, 0x313131ff,
		0xffffffff, 0xff0000ff, 0x00ff00ff, 0x0000ffff, 0xffff00ff,
		0xff00ffff, 0x00ffffff, 0x800000ff, 0x008000ff, 0x000080ff,
		0x808000ff, 0x800080ff, 0x008080ff, 0xc6c6c6ff, 0x808080ff
	};

	unsigned palette_index = ind;
	if (palette_index >= G_N_ELEMENTS (palette))
		palette_index %= G_N_ELEMENTS (palette);
	if (style->fill.auto_back) {
		style->fill.pattern.back = palette [palette_index];

		/* force the brightness to reinterpolate */
		if (style->fill.type == GO_STYLE_FILL_GRADIENT &&
		    style->fill.gradient.brightness >= 0)
			go_style_set_fill_brightness (style,
				style->fill.gradient.brightness);
	}

	palette_index += 8;
	if (palette_index >= G_N_ELEMENTS (palette))
		palette_index -= G_N_ELEMENTS (palette);
	if (style->line.auto_color && !(style->disable_theming & GO_STYLE_LINE))
		style->line.color = palette [palette_index];
	if (!(style->disable_theming & GO_STYLE_MARKER))
		map_marker (&style->marker, ind, palette_index, palette);
}

static void
map_area_series_solid_guppi (GOStyle *style, unsigned ind, G_GNUC_UNUSED GogTheme const *theme)
{
	static GOColor const palette[] = {
		0xff3000ff, 0x80ff00ff, 0x00ffcfff, 0x2000ffff,
		0xff008fff, 0xffbf00ff, 0x00ff10ff, 0x009fffff,
		0xaf00ffff, 0xff0000ff, 0xafff00ff, 0x00ff9fff,
		0x0010ffff, 0xff00bfff, 0xff8f00ff, 0x20ff00ff,
		0x00cfffff, 0x8000ffff, 0xff0030ff, 0xdfff00ff,
		0x00ff70ff, 0x0040ffff, 0xff00efff, 0xff6000ff,
		0x50ff00ff, 0x00ffffff, 0x5000ffff, 0xff0060ff,
		0xffef00ff, 0x00ff40ff, 0x0070ffff, 0xdf00ffff
	};

	unsigned palette_index = ind;
	if (palette_index >= G_N_ELEMENTS (palette))
		palette_index %= G_N_ELEMENTS (palette);
	if (style->fill.auto_back) {
		style->fill.pattern.back = palette [palette_index];
		if (style->fill.type == GO_STYLE_FILL_GRADIENT &&
		    style->fill.gradient.brightness >= 0)
			/* force the brightness to reinterpolate */
			go_style_set_fill_brightness (style,
				style->fill.gradient.brightness);
	}
	if (style->line.auto_color && !(style->disable_theming & GO_STYLE_LINE))
		style->line.color = palette [palette_index];
	if (!(style->disable_theming & GO_STYLE_MARKER))
		map_marker (&style->marker, ind, palette_index, palette);
}

static void
map_area_series_solid_palette (GOStyle *style, unsigned ind, GogTheme const *theme)
{
	GOStyle *src;
	if (theme->palette->len == 0)
		src = theme->default_style;
	else {
		ind %= theme->palette->len;
		src = g_ptr_array_index (theme->palette, ind);
	}
	if (src)
		go_style_apply_theme (style, src, style->interesting_fields);
}

/**************************************************************************/

/**
 * gog_theme_registry_add:
 * @theme: a #GogTheme
 * @is_default : bool
 *
 * Keep a pointer to @theme in graph theme registry.
 * This function does not add a reference to @theme.
 **/
static void
gog_theme_registry_add (GogTheme *theme, gboolean is_default)
{
	g_return_if_fail (GOG_IS_THEME (theme));

	/* TODO: Check for duplicated names and for already
	 * registered themes */

	if (is_default) {
		g_object_ref (theme);
		if (default_theme != NULL)
			g_object_unref (default_theme);
		default_theme = theme;
	}

	themes = g_slist_append (themes, theme);
}

/**
 * gog_theme_registry_lookup:
 * @name: a theme name
 *
 * Returns: a #GogTheme from theme registry.
 **/
GogTheme *
gog_theme_registry_lookup (char const *name)
{
	GSList *ptr;
	GogTheme *theme;

	if (name != NULL) {
		for (ptr = themes ; ptr != NULL ; ptr = ptr->next) {
			theme = ptr->data;
			if (!strcmp (theme->name, name))
				return theme;
		}
		g_warning ("No theme named '%s' found, using default", name);
	}
	return default_theme;
}

/**
 * gog_theme_registry_get_theme_names:
 *
 * Returns: a newly allocated theme name list from theme registry.
 **/
GSList *
gog_theme_registry_get_theme_names (void)
{
	GogTheme *theme;
	GSList *names = NULL;
	GSList *ptr;

	for (ptr = themes; ptr != NULL; ptr = ptr->next) {
		theme = ptr->data;
		names = g_slist_append (names, theme->name);
	}

	return names;
}

/**************************************************************************/

static void build_predefined_themes (void)
{
	GogTheme *theme;
	GOStyle *style;

	if (NULL == global_class_aliases) {
		global_class_aliases = g_hash_table_new (
			g_str_hash, g_str_equal);
		g_hash_table_insert (global_class_aliases,
			(gpointer)"GogSeriesElement", (gpointer)"GogSeries");
		g_hash_table_insert (global_class_aliases,
			(gpointer)"GogSeriesLines", (gpointer)"GogSeries");
	}

	/* An MS Excel-ish theme */
	theme = gog_theme_new (N_("Default"));
	gog_theme_registry_add (theme, TRUE);

	/* graph */
	style = go_style_new ();
	style->line.dash_type = GO_LINE_NONE;
	style->line.width = 0;
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_NONE;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_WHITE);
	gog_theme_add_element (theme, style, NULL, "GogGraph", NULL);

	/* chart */
	style = go_style_new ();
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0; /* hairline */
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_PATTERN;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_WHITE);
	gog_theme_add_element (theme, style, NULL, "GogChart", NULL);

	/* Legend */
	style = go_style_new ();
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0; /* hairline */
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_PATTERN;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_WHITE);
	gog_theme_add_element (theme, style, NULL, "GogLegend", NULL);

	/* Axis */
	style = go_style_new ();
	style->line.width = 0; /* hairline */
	style->line.color = GO_COLOR_BLACK;
	gog_theme_add_element (theme, style, NULL, "GogAxis", NULL);

	/* AxisLine */
	style = go_style_new ();
	style->line.width = 0; /* hairline */
	style->line.color = GO_COLOR_BLACK;
	gog_theme_add_element (theme, style, NULL, "GogAxisLine", NULL);

	/* Grid */
	style = go_style_new ();
	style->fill.type  = GO_STYLE_FILL_PATTERN;
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 1.;
	style->line.color = 0X848284ff;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_GREY (0xd0));
	gog_theme_add_element (theme, style, NULL, "GogGrid", NULL);

	/* GridLine */
	style = go_style_new ();
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0.4;
	style->line.color = GO_COLOR_BLACK;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_FROM_RGBA (0xE0, 0xE0, 0xE0, 0xE0));
	style->fill.type = GO_STYLE_FILL_NONE;
	gog_theme_add_element (theme, style, NULL, NULL, "MajorGrid");
	style = go_style_new ();
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0.2;
	style->line.color = GO_COLOR_BLACK;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_FROM_RGBA (0xE0, 0xE0, 0xE0, 0xE0));
	style->fill.type = GO_STYLE_FILL_NONE;
	gog_theme_add_element (theme, style, NULL, NULL, "MinorGrid");

	/* Series */
	style = go_style_new ();
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0; /* hairline */
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_PATTERN;
	/* FIXME : not really true, will want to split area from line */
	gog_theme_add_element (theme, style,
		map_area_series_solid_default, "GogSeries", NULL);

	/* Chart titles */
	style = go_style_new ();
	style->line.dash_type = GO_LINE_NONE;
	style->line.width = 0.;
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_NONE;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_WHITE);
	go_style_set_font_desc (style, pango_font_description_from_string ("Sans Bold 12"));
	gog_theme_add_element (theme, style, NULL, "GogChart", "Title");

	/* labels */
	style = go_style_new ();
	style->line.dash_type = GO_LINE_NONE;
	style->line.width = 0.;
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_NONE;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_WHITE);
	go_style_set_font_desc (style, pango_font_description_from_string ("Sans 10"));
	gog_theme_add_element (theme, style, NULL, "GogLabel", NULL);

	/* regression curves */
	style = go_style_new ();
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 1;
	style->line.color = GO_COLOR_BLACK;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_FROM_RGBA (0x00, 0x00, 0x00, 0x20));
	style->fill.type = GO_STYLE_FILL_NONE;
	gog_theme_add_element (theme, style,
		NULL, "GogTrendLine", NULL);

	/* regression equations */
	style = go_style_new ();
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0; /* hairline */
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_PATTERN;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_WHITE);
	gog_theme_add_element (theme, style,
		NULL, "GogRegEqn", NULL);

#ifdef GOFFICE_WITH_LASEM
	/* Equations */
	style = go_style_new ();
	style->line.dash_type = GO_LINE_NONE;
	style->line.width = 0; /* hairline */
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_NONE;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_WHITE);
	go_style_set_font_desc (style, pango_font_description_from_string ("Sans 10"));
	gog_theme_add_element (theme, style,
		NULL, "GogEquation", NULL);
#endif

/* Guppi */
	theme = gog_theme_new (N_("Guppi"));
	gog_theme_registry_add (theme, FALSE);

	/* graph */
	style = go_style_new ();
	style->line.dash_type = GO_LINE_NONE;
	style->line.width = 0;
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_GRADIENT;
	style->fill.gradient.dir   = GO_GRADIENT_N_TO_S;
	style->fill.pattern.fore = GO_COLOR_BLUE;
	style->fill.pattern.back = GO_COLOR_BLACK;
	gog_theme_add_element (theme, style, NULL, "GogGraph", NULL);

	/* chart */
	style = go_style_new ();
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0; /* hairline */
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_PATTERN;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_WHITE);
	gog_theme_add_element (theme, style, NULL, "GogChart", NULL);

	/* legend */
	style = go_style_new ();
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0; /* hairline */
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_PATTERN;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_WHITE);
	gog_theme_add_element (theme, style, NULL, "GogLegend", NULL);

	/* Axis */
	style = go_style_new ();
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0.; /* hairline */
	style->line.color = GO_COLOR_GREY (0x20);
	gog_theme_add_element (theme, style, NULL, "GogAxis", NULL);

	/* AxisLine */
	style = go_style_new ();
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0.; /* hairline */
	style->line.color = GO_COLOR_GREY (0x20);
	gog_theme_add_element (theme, style, NULL, "GogAxisLine", NULL);

	/* Grid */
	style = go_style_new ();
	style->fill.type  = GO_STYLE_FILL_PATTERN;
	style->line.dash_type = GO_LINE_NONE;
	style->line.color = GO_COLOR_BLACK;
	style->line.width = 0;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_GREY (0xd0));
	gog_theme_add_element (theme, style, NULL, "GogGrid", NULL);

	/* GridLine */
	style = go_style_new ();
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0.; /* hairline */
	style->line.color = GO_COLOR_GREY (0x96);
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_FROM_RGBA (0xE0, 0xE0, 0xE0, 0xE0));
	style->fill.type = GO_STYLE_FILL_NONE;
	gog_theme_add_element (theme, style, NULL, NULL, "MajorGrid");
	style = go_style_new ();
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0.; /* hairline */
	style->line.color = GO_COLOR_GREY (0xC0);
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_FROM_RGBA (0xE0, 0xE0, 0xE0, 0xE0));
	style->fill.type = GO_STYLE_FILL_NONE;
	gog_theme_add_element (theme, style, NULL, NULL, "MinorGrid");

	/* series */
	style = go_style_new ();
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0.; /* hairline */
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_PATTERN;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_GREY (0x20));
	/* FIXME : not really true, will want to split area from line */
	gog_theme_add_element (theme, style,
		map_area_series_solid_guppi, "GogSeries", NULL);

	/* labels */
	style = go_style_new ();
	style->line.dash_type = GO_LINE_NONE;
	style->line.width = 0; /* none */
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_NONE;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_WHITE);
	gog_theme_add_element (theme, style, NULL, "GogLabel", NULL);

	/* trend lines */
	style = go_style_new ();
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 1;
	style->line.color = GO_COLOR_BLACK;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_FROM_RGBA (0x00, 0x00, 0x00, 0x20));
	style->fill.type = GO_STYLE_FILL_NONE;
	gog_theme_add_element (theme, style,
		NULL, "GogTrendLine", NULL);

	/* regression equations */
	style = go_style_new ();
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0; /* hairline */
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_PATTERN;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_WHITE);
	gog_theme_add_element (theme, style,
		NULL, "GogRegEqn", NULL);

#ifdef GOFFICE_WITH_LASEM
	/* Equations */
	style = go_style_new ();
	style->line.dash_type = GO_LINE_NONE;
	style->line.width = 0; /* hairline */
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_NONE;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_WHITE);
	go_style_set_font_desc (style, pango_font_description_from_string ("Sans 10"));
	gog_theme_add_element (theme, style,
		NULL, "GogEquation", NULL);
#endif
}

struct theme_load_state {
	GogTheme *theme;
	char *desc, *lang, *local_name;
	unsigned name_lang_score;
	unsigned desc_lang_score;
	char const * const *langs;
};

static void
name_start (GsfXMLIn *xin, xmlChar const **attrs)
{
	struct theme_load_state	*state = (struct theme_load_state *) xin->user_state;
	unsigned i;
	for (i = 0; attrs != NULL && attrs[i] && attrs[i+1] ; i += 2)
		if (0 == strcmp (attrs[i], "xml:lang"))
			state->lang = g_strdup (attrs[i+1]);
}

static void
name_end (GsfXMLIn *xin, G_GNUC_UNUSED GsfXMLBlob *blob)
{
	struct theme_load_state	*state = (struct theme_load_state *) xin->user_state;
	char *name = NULL;
	if (xin->content->str == NULL)
		return;
	name = g_strdup (xin->content->str);
	if (state->lang == NULL) {
		GOStyle *style;
		state->theme = gog_theme_new (name);
		state->theme->palette = g_ptr_array_new ();
		/* initialize a dummy GogSeries style */
		style = go_style_new ();
		style->line.dash_type = GO_LINE_SOLID;
		style->line.width = 0; /* hairline */
		style->line.color = GO_COLOR_BLACK;
		style->fill.type = GO_STYLE_FILL_NONE;
		gog_theme_add_element (state->theme, style,
			map_area_series_solid_palette, "GogSeries", NULL);
	} else if (state->name_lang_score > 0 && state->langs[0] != NULL) {
		unsigned i;
		for (i = 0; i < state->name_lang_score && state->langs[i] != NULL; i++) {
			if (strcmp (state->langs[i], state->lang) == 0) {
				g_free (state->local_name);
				state->local_name = g_strdup (name);
				state->name_lang_score = i;
			}
		}
	}
	g_free (state->lang);
	state->lang = NULL;
}

static void
desc_end (GsfXMLIn *xin, G_GNUC_UNUSED GsfXMLBlob *blob)
{
	struct theme_load_state	*state = (struct theme_load_state *) xin->user_state;
	if (state->lang == NULL) {
		if (state->desc == NULL)
			state->desc = g_strdup (xin->content->str);
	} else if (state->desc_lang_score > 0 && state->langs[0] != NULL) {
		unsigned i;
		for (i = 0; i < state->desc_lang_score && state->langs[i] != NULL; i++) {
			if (strcmp (state->langs[i], state->lang) == 0) {
				g_free (state->desc);
				state->desc = g_strdup (xin->content->str);
				state->desc_lang_score = i;
			}
		}
	}
	g_free (state->lang);
	state->lang = NULL;
}

static void
elem_start (GsfXMLIn *xin, G_GNUC_UNUSED xmlChar const **attrs)
{
	struct theme_load_state	*state = (struct theme_load_state *) xin->user_state; 
	char const *role = NULL, *class_name = NULL;
	GOStyle *style;
	unsigned i;

	if (state->theme == NULL)
		return;
	for (i = 0; attrs != NULL && attrs[i] && attrs[i+1] ; i += 2)
		if (0 == strcmp (attrs[i], "class"))
			class_name = g_strdup (attrs[i+1]);
		else if (0 == strcmp (attrs[i], "role"))
			role = g_strdup (attrs[i+1]);
	style = go_style_new ();
	go_persist_prep_sax (GO_PERSIST (style), xin, attrs);

	if (class_name && !strcmp (class_name, "GogSeries"))
		g_ptr_array_add (state->theme->palette, style);
	else
		gog_theme_add_element (state->theme, style, NULL, class_name, role);
}

static void
theme_load_from_uri (char const *uri)
{
	static GsfXMLInNode const theme_dtd[] = {
		GSF_XML_IN_NODE (THEME, THEME, -1, "GogTheme", GSF_XML_NO_CONTENT, NULL, NULL),
			GSF_XML_IN_NODE (THEME, NAME, -1, "name", GSF_XML_CONTENT, name_start, name_end),
			GSF_XML_IN_NODE (THEME, UNAME, -1, "_name", GSF_XML_CONTENT, name_start, name_end),
			GSF_XML_IN_NODE (THEME, DESCRIPTION, -1, "description", GSF_XML_CONTENT, name_start, desc_end),
			GSF_XML_IN_NODE (THEME, UDESCRIPTION, -1, "_description", GSF_XML_CONTENT, name_start, desc_end),
			GSF_XML_IN_NODE (THEME, STYLE, -1, "GOStyle", GSF_XML_NO_CONTENT, elem_start, NULL),
		GSF_XML_IN_NODE_END
	};
	struct theme_load_state	state;
	GsfXMLInDoc *xml;
	GsfInput *input = go_file_open (uri, NULL);

	if (input == NULL)
		g_warning ("[GogTheme]: Could not open %s", uri);
	state.theme = NULL;
	state.desc = state.lang = state.local_name = NULL;
	state.langs = g_get_language_names ();
	state.name_lang_score = state.desc_lang_score = G_MAXINT;
	xml = gsf_xml_in_doc_new (theme_dtd, NULL);
	if (!gsf_xml_in_doc_parse (xml, input, &state))
		g_warning ("[GogTheme]: Could not parse %s", uri);
	if (state.theme != NULL) {
		state.theme->local_name = state.local_name;
		state.theme->description = state.desc;
		gog_theme_registry_add (state.theme, FALSE);
	} else {
		g_free (state.local_name);
		g_free (state.desc);
	}
	g_free (state.lang);
	gsf_xml_in_doc_free (xml);
	g_object_unref (input);
}

static void
themes_load_from_dir (char const *path)
{
	GDir *dir = g_dir_open (path, 0, NULL);
	char const *d_name;

	if (dir == NULL)
		return;
	while ((d_name = g_dir_read_name (dir)) != NULL) {
		char *fullname = g_build_filename (path, d_name, NULL);
		char *uri = go_filename_to_uri (fullname);
		char *mime_type = go_get_mime_type (uri);
		if (!strcmp (mime_type, "application/x-theme"))
			theme_load_from_uri (uri);
		g_free (mime_type);
		g_free (uri);
		g_free (fullname);
	}
	g_dir_close (dir);
}

void
_gog_themes_init (void)
{
	char *path;

	build_predefined_themes ();

	/* Load themes from file */
	path = g_build_filename (go_sys_data_dir (), "themes", NULL);
	themes_load_from_dir (path);
	g_free (path);

	path = g_build_filename (g_get_home_dir (), ".goffice", "themes", NULL);
	themes_load_from_dir (path);
	g_free (path);
}

void
_gog_themes_shutdown (void)
{
	if (default_theme != NULL) {
		g_object_unref (default_theme);
		default_theme = NULL;
	}

	go_slist_free_custom (g_slist_copy (themes), g_object_unref);
	g_slist_free (themes);
	themes = NULL;
}
