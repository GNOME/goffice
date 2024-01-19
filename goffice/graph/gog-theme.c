/*
 * gog-theme.c:
 *
 * Copyright (C) 2003-2004 Jody Goldberg (jody@gnome.org)
 * Copyright (C) 2010 Jean Brefort (jean.brefort@normalesup.org)
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
#include <goffice/goffice-priv.h>
#include <goffice/graph/gog-theme.h>
#include <gsf/gsf-input-gio.h>
#include <gsf/gsf-output-gio.h>

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
 * A file defining a theme is an xml file with a &lt;GogTheme&gt; root node.
 * The root element needs to have an Id which should be unique. The uuid command
 * provides such Ids.
 *
 * The contents must be: _name|name+, _description?|description*, GOStyle+.
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
 * node attributes: class, role.
 * node contents: (line|outline)?, fill?, marker?, font?, text_layout?
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
 * <tr><td>GogTrendLine</td><td></td><td>line</td></tr>
 * <tr><td>GogRegEqn</td><td></td><td>outline, fill, font, text_layout</td></tr>
 * <tr><td>GogSeriesLabels</td><td></td><td>outline, fill, font, text_layout</td></tr>
 * <tr><td>GogRegEqn</td><td></td><td>outline, fill, font, text_layout</td></tr>
 * <tr><td>GogSeriesLabel</td><td></td><td>outline, fill, font, text_layout</td></tr>
 * <tr><td>GogDataLabel</td><td></td><td>outline, fill, font, text_layout</td></tr>
 * <tr><td>GogEquation</td><td></td><td>outline, fill, font, text_layout</td></tr>
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
	char 		 *klass_name;
	char 		 *role_id;
	GOStyle   	 *style;
	GogThemeStyleMap  map;
} GogThemeElement;

struct _GogTheme {
	GObject	base;

	char		*id;
	char 		*name;
	char 		*description;
	char		*uri;
	GoResourceType type;
	GHashTable	*names;
	GHashTable	*descs;
	GHashTable	*elem_hash_by_role;
	GHashTable	*elem_hash_by_class;
	GHashTable	*class_aliases;
	GOStyle	*default_style;
	GPtrArray	*palette;
	GogAxisColorMap *cm, *dcm; /* cm: color map for color axis, dcm stands for discrete color map */
	gboolean built_color_map; /* %TRUE if color map is built from the palette */
	gboolean writeable; /* %TRUE if theme can be edited */
	char		*path; /* file path if any */
};

typedef GObjectClass GogThemeClass;

static GObjectClass *parent_klass;

static GSList	    *themes;
static GogTheme	    *default_theme = NULL;
static GHashTable   *global_class_aliases = NULL;

enum {
	GOG_THEME_PROP_0,
	GOG_THEME_PROP_TYPE
};

static void
gog_theme_set_property (GObject *gobject, guint param_id,
                                 GValue const *value, GParamSpec *pspec)
{
	GogTheme *theme = GOG_THEME (gobject);

	switch (param_id) {
	case GOG_THEME_PROP_TYPE:
		theme->type = g_value_get_enum (value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
}

static void
gog_theme_get_property (GObject *gobject, guint param_id,
                        GValue *value, GParamSpec *pspec)
{
	GogTheme *theme = GOG_THEME (gobject);

	switch (param_id) {
	case GOG_THEME_PROP_TYPE:
		g_value_set_enum (value, theme->type);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
}

static void
gog_theme_element_free (GogThemeElement *elem)
{
	g_object_unref (elem->style);
	g_free (elem->klass_name);
	g_free (elem->role_id);
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

	g_free (theme->id);
	g_free (theme->name);
	g_free (theme->uri);
	g_free (theme->description);
	if (theme->names)
		g_hash_table_destroy (theme->names);
	if (theme->descs)
		g_hash_table_destroy (theme->descs);
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
	if (theme->cm)
		g_object_unref (theme->cm);
	if (theme->dcm)
		g_object_unref (theme->dcm);

	(parent_klass->finalize) (obj);
}

static void
gog_theme_class_init (GogThemeClass *klass)
{
	GObjectClass *gobject_klass   = (GObjectClass *) klass;

	parent_klass = g_type_class_peek_parent (klass);
	gobject_klass->finalize	    = gog_theme_finalize;
	gobject_klass->set_property = gog_theme_set_property;
	gobject_klass->get_property = gog_theme_get_property;
	g_object_class_install_property (gobject_klass, GOG_THEME_PROP_TYPE,
		g_param_spec_enum ("resource-type",
			_("Resource type"),
			_("The resource type for the theme"),
			go_resource_type_get_type (), GO_RESOURCE_INVALID,
		        GSF_PARAM_STATIC | G_PARAM_READWRITE |G_PARAM_CONSTRUCT_ONLY));
}

static void
gog_theme_init (GogTheme *theme)
{
	theme->names = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
	theme->descs = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
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

/***************
 * Output code *
 ***************/

static void
save_name_cb (char const *lang, char const *name, GsfXMLOut *output)
{
	gsf_xml_out_start_element (output, "name");
	if (strcmp (lang, "C"))
		gsf_xml_out_add_cstr_unchecked (output, "xml:lang", lang);
	gsf_xml_out_add_cstr_unchecked (output, NULL, name);
	gsf_xml_out_end_element (output);
}

static void
save_desc_cb (char const *lang, char const *name, GsfXMLOut *output)
{
	gsf_xml_out_start_element (output, "description");
	if (strcmp (lang, "C"))
		gsf_xml_out_add_cstr_unchecked (output, "xml:lang", lang);
	gsf_xml_out_add_cstr_unchecked (output, NULL, name);
	gsf_xml_out_end_element (output);
}

static void
save_elem_cb (G_GNUC_UNUSED char const *key, GogThemeElement *elt, GsfXMLOut *output)
{
	if (elt->klass_name && !strcmp (elt->klass_name, "GogSeries"))
		return;
	gsf_xml_out_start_element (output, "GOStyle");
	if (elt->klass_name)
		gsf_xml_out_add_cstr_unchecked (output, "class", elt->klass_name);
	if (elt->role_id)
		gsf_xml_out_add_cstr_unchecked (output, "role", elt->role_id);
	go_persist_sax_save (GO_PERSIST (elt->style), output);
	gsf_xml_out_end_element (output);
}

static void
save_series_style_cb (GOPersist *gp, GsfXMLOut *output)
{
	gsf_xml_out_start_element (output, "GOStyle");
	gsf_xml_out_add_cstr_unchecked (output, "class", "GogSeries");
	go_persist_sax_save (gp, output);
	gsf_xml_out_end_element (output);
}

static void gog_theme_save (GogTheme const *theme);

static void
gog_theme_build_uri (GogTheme *theme)
{
	char *filename, *full_name;
	filename = g_strconcat (theme->id, ".theme", NULL);
	full_name = g_build_filename (g_get_home_dir (), ".goffice", "themes", filename, NULL);
	theme->uri = go_filename_to_uri (full_name);
	g_free (filename);
	g_free (full_name);
}

static void
gog_theme_sax_save (GOPersist const *gp, GsfXMLOut *output)
{
	GogTheme *theme;

	g_return_if_fail (GOG_IS_THEME (gp));

	theme = GOG_THEME (gp);
	if (output == NULL) {
		g_return_if_fail (theme->uri == NULL);
		gog_theme_build_uri (theme);
		theme->type = GO_RESOURCE_RW;
		gog_theme_save (theme);
		return;
	}
	gsf_xml_out_add_cstr_unchecked (output, "id", theme->id);
	g_hash_table_foreach (theme->names, (GHFunc) save_name_cb, output);
	g_hash_table_foreach (theme->descs, (GHFunc) save_desc_cb, output);
	g_hash_table_foreach (theme->elem_hash_by_class, (GHFunc) save_elem_cb, output);
	g_hash_table_foreach (theme->elem_hash_by_role, (GHFunc) save_elem_cb, output);
	if (theme->palette)
		g_ptr_array_foreach (theme->palette, (GFunc) save_series_style_cb, output);
	if (theme->cm && gog_axis_color_map_get_resource_type (theme->cm) == GO_RESOURCE_CHILD) {
		gsf_xml_out_start_element (output, "GogAxisColorMap");
		gsf_xml_out_add_cstr_unchecked (output, "type", (theme->cm == theme->dcm)? "both": "gradient");
		go_persist_sax_save (GO_PERSIST (theme->cm), output);
		gsf_xml_out_end_element (output);
	}
	if (theme->dcm && theme->dcm != theme->cm && gog_axis_color_map_get_resource_type (theme->dcm) == GO_RESOURCE_CHILD) {
		gsf_xml_out_start_element (output, "GogAxisColorMap");
		gsf_xml_out_add_cstr_unchecked (output, "type", "discrete");
		go_persist_sax_save (GO_PERSIST (theme->dcm), output);
		gsf_xml_out_end_element (output);
	}
}

static void
gog_theme_save (GogTheme const *theme)
{
	GsfOutput *output = gsf_output_gio_new_for_uri (theme->uri, NULL);
	GsfXMLOut *xml = gsf_xml_out_new (output);
	gsf_xml_out_start_element (xml, "GogTheme");
	gog_theme_sax_save (GO_PERSIST (theme), xml);
	gsf_xml_out_end_element (xml);
	g_object_unref (xml);
	g_object_unref (output);
}

/**
 * gog_theme_save_to_home_dir:
 * @theme: the #GogTheme to save
 *
 * Writes the theme to the user directory so that it becomes permanently
 * available.
 **/
void
gog_theme_save_to_home_dir (GogTheme *theme)
{
	g_return_if_fail (GOG_IS_THEME (theme) && theme->type == GO_RESOURCE_EXTERNAL && theme->uri == NULL);
	gog_theme_build_uri (theme);
	gog_theme_save (theme);
	theme->type = GO_RESOURCE_RW;
}

/**************
 * Input code *
 **************/

struct theme_load_state {
	GogTheme *theme;
	char *name, *desc, *lang;
	unsigned name_lang_score;
	unsigned desc_lang_score;
	char const * const *langs;
	GSList *garbage;
};

static void
theme_start (GsfXMLIn *xin, xmlChar const **attrs)
{
	struct theme_load_state	*state = (struct theme_load_state *) xin->user_state;
	if (state->theme ==  NULL) {
		state->theme = g_object_new (GOG_TYPE_THEME, NULL);
		for (; attrs && *attrs; attrs +=2)
			if (!strcmp ((char const *) *attrs, "id")) {
				state->theme->id = g_strdup ((char const *) attrs[1]);
				break;
			}
	}
}

static void
name_start (GsfXMLIn *xin, xmlChar const **attrs)
{
	struct theme_load_state	*state = (struct theme_load_state *) xin->user_state;
	unsigned i;
	if (state->theme->name) /* the theme has already been loaded from elsewhere */
		return;
	for (i = 0; attrs != NULL && attrs[i] && attrs[i+1] ; i += 2)
		if (0 == strcmp (attrs[i], "xml:lang"))
			state->lang = g_strdup (attrs[i+1]);
}

static void
name_end (GsfXMLIn *xin, G_GNUC_UNUSED GsfXMLBlob *blob)
{
	struct theme_load_state	*state = (struct theme_load_state *) xin->user_state;
	char *name = NULL;
	if (state->theme->name) /* the theme has already been loaded from elsewhere */
		return;
	if (xin->content->str == NULL)
		return;
	name = g_strdup (xin->content->str);
	if (state->lang == NULL)
	state->lang = g_strdup ("C");
	if (state->name_lang_score > 0 && state->langs[0] != NULL) {
		unsigned i;
		for (i = 0; i < state->name_lang_score && state->langs[i] != NULL; i++) {
			if (strcmp (state->langs[i], state->lang) == 0) {
				g_free (state->name);
				state->name = g_strdup (name);
				state->name_lang_score = i;
			}
		}
	}
	g_hash_table_replace (state->theme->names, state->lang, name);
	state->lang = NULL;
}

static void
desc_end (GsfXMLIn *xin, G_GNUC_UNUSED GsfXMLBlob *blob)
{
	struct theme_load_state	*state = (struct theme_load_state *) xin->user_state;
	char *desc = NULL;
	if (state->theme->name) /* the theme has already been loaded from elsewhere */
		return;
	if (xin->content->str == NULL)
		return;
	desc = g_strdup (xin->content->str);
	if (state->lang == NULL)
	state->lang = g_strdup ("C");
	if (state->desc_lang_score > 0 && state->langs[0] != NULL) {
		unsigned i;
		for (i = 0; i < state->desc_lang_score && state->langs[i] != NULL; i++) {
			if (strcmp (state->langs[i], state->lang) == 0) {
				g_free (state->desc);
				state->desc = g_strdup (desc);
				state->desc_lang_score = i;
			}
		}
	}
	g_hash_table_replace (state->theme->descs, state->lang, desc);
	state->lang = NULL;
}

enum { /* used in theme editor to display the right snapshot */
	SNAPSHOT_GRAPH,
	SNAPSHOT_TRENDLINE,
	SNAPSHOT_SERIES,
	SNAPSHOT_SERIESLABELS,
	SNAPSHOT_SERIESLINES,
#ifdef GOFFICE_WITH_LASEM
	SNAPSHOT_EQUATION,
#endif
};

typedef struct {
	char const *klass_name;
	char const *role_id;
	char const *label;
	GOStyleFlag flags;
	unsigned snapshot;
} GogThemeRoles;

static GogThemeRoles roles[] = {
	{"GogGraph", NULL, N_("Graph"), GO_STYLE_FILL | GO_STYLE_OUTLINE, SNAPSHOT_GRAPH},
	{"GogGraph", "Title", N_("Graph title"), GO_STYLE_OUTLINE | GO_STYLE_FILL | GO_STYLE_FONT | GO_STYLE_TEXT_LAYOUT, SNAPSHOT_GRAPH},
	{"GogChart", NULL, N_("Chart"), GO_STYLE_FILL | GO_STYLE_OUTLINE, SNAPSHOT_GRAPH},
	{"GogChart", "Title", N_("Chart title"), GO_STYLE_OUTLINE | GO_STYLE_FILL | GO_STYLE_FONT | GO_STYLE_TEXT_LAYOUT, SNAPSHOT_GRAPH},
	{"GogLegend", NULL, N_("Legend"), GO_STYLE_OUTLINE | GO_STYLE_FILL | GO_STYLE_FONT, SNAPSHOT_GRAPH},
	{"GogAxis", NULL, N_("Axis"), GO_STYLE_LINE | GO_STYLE_FONT | GO_STYLE_TEXT_LAYOUT, SNAPSHOT_GRAPH},
	{"GogAxisLine", NULL, N_("Axis line"), GO_STYLE_LINE | GO_STYLE_FONT, SNAPSHOT_GRAPH},
	{"GogGrid", NULL, N_("Backplane"), GO_STYLE_FILL | GO_STYLE_OUTLINE, SNAPSHOT_GRAPH},
	{"GogGrid", "MajorGrid", N_("Major grid"), GO_STYLE_LINE | GO_STYLE_FILL, SNAPSHOT_GRAPH},
	{"GogGrid", "MinorGrid", N_("Minor grid"), GO_STYLE_LINE | GO_STYLE_FILL, SNAPSHOT_GRAPH},
	{"GogLabel", NULL, N_("Label"), GO_STYLE_OUTLINE | GO_STYLE_FILL | GO_STYLE_FONT | GO_STYLE_TEXT_LAYOUT, SNAPSHOT_GRAPH},
	{"GogTrendLine", NULL, N_("Trend line"), GO_STYLE_LINE, SNAPSHOT_TRENDLINE},
	{"GogRegEqn", NULL, N_("Regression equation"), GO_STYLE_OUTLINE | GO_STYLE_FILL | GO_STYLE_FONT | GO_STYLE_TEXT_LAYOUT, SNAPSHOT_TRENDLINE},
#ifdef GOFFICE_WITH_LASEM
	{"GogEquation", NULL, N_("Equation"), GO_STYLE_OUTLINE | GO_STYLE_FILL | GO_STYLE_FONT | GO_STYLE_TEXT_LAYOUT, SNAPSHOT_EQUATION},
#endif
	{"GogSeriesLine", NULL, N_("Series lines"), GO_STYLE_LINE, SNAPSHOT_SERIESLINES},
	{"GogSeries", NULL, N_("Series"), GO_STYLE_LINE | GO_STYLE_FILL | GO_STYLE_MARKER, SNAPSHOT_SERIES},
	{"GogSeriesLabels", NULL, N_("Series labels"), GO_STYLE_OUTLINE | GO_STYLE_FILL | GO_STYLE_FONT | GO_STYLE_TEXT_LAYOUT, SNAPSHOT_SERIESLABELS},
	{"GogDataLabel", NULL, N_("Data label"), GO_STYLE_OUTLINE | GO_STYLE_FILL | GO_STYLE_FONT | GO_STYLE_TEXT_LAYOUT, SNAPSHOT_SERIESLABELS},
	{"GogColorScale", NULL, N_("Color scale"), GO_STYLE_OUTLINE, SNAPSHOT_GRAPH}
};

static void
gog_theme_add_element (GogTheme *theme, GOStyle *style,
		       GogThemeStyleMap	map,
		       char *klass_name, char *role_id)
{
	GogThemeElement *elem;
	unsigned i;

	g_return_if_fail (theme != NULL);

	elem = g_new0 (GogThemeElement, 1);
	elem->klass_name = klass_name;
	elem->role_id  = role_id;
	elem->style = style;
	elem->map   = map;

	/* sets the needed insteresting_fields in style */
	for (i = 0; i < G_N_ELEMENTS (roles); i++) {
		if ((klass_name && roles[i].klass_name && strcmp (klass_name, roles[i].klass_name))
		    || (klass_name != NULL && roles[i].klass_name == NULL))
			continue;
		if ((role_id && roles[i].role_id && strcmp (role_id, roles[i].role_id))
		    || (role_id == NULL && roles[i].role_id != NULL) ||
		    (role_id != NULL && roles[i].role_id == NULL))
			continue;
		style->interesting_fields = roles[i].flags;
		break;
	}
	if (i == G_N_ELEMENTS (roles))
		g_warning ("[GogTheme]: unregistered style class=%s role=%s\n",klass_name, role_id);
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

static void
elem_start (GsfXMLIn *xin, G_GNUC_UNUSED xmlChar const **attrs)
{
	struct theme_load_state	*state = (struct theme_load_state *) xin->user_state;
	char *role = NULL, *class_name = NULL;
	GOStyle *style;
	unsigned i;

	if (state->theme == NULL)
		return;
	if (state->theme->name) { /* the theme has already been loaded from elsewhere */
		style = go_style_new (); /* still load the style to avoid warnings */
		go_persist_prep_sax (GO_PERSIST (style), xin, attrs);
		state->garbage = g_slist_prepend (state->garbage, style);
		return;
	}
	for (i = 0; attrs != NULL && attrs[i] && attrs[i+1] ; i += 2)
		if (0 == strcmp (attrs[i], "class"))
			class_name = g_strdup (attrs[i+1]);
		else if (0 == strcmp (attrs[i], "role"))
			role = g_strdup (attrs[i+1]);
	style = go_style_new ();
	go_style_clear_auto (style);
	go_persist_prep_sax (GO_PERSIST (style), xin, attrs);

	if (class_name && !strcmp (class_name, "GogSeries")) {
		if (state->theme->palette ==  NULL)
			state->theme->palette = g_ptr_array_new ();
		g_ptr_array_add (state->theme->palette, style);
		g_free (class_name);
	} else
		gog_theme_add_element (state->theme, style, NULL, class_name, role);
}

static void
color_map_start (GsfXMLIn *xin, G_GNUC_UNUSED xmlChar const **attrs)
{
	struct theme_load_state	*state = (struct theme_load_state *) xin->user_state;
	GogAxisColorMap *map;
	if (state->theme == NULL)
		return;
	if (state->theme->name) /* the theme has already been loaded from elsewhere */
		return;
	map = g_object_new (GOG_TYPE_AXIS_COLOR_MAP, "resource-type", GO_RESOURCE_CHILD, NULL);
	for (; attrs && *attrs; attrs += 2)
		if (!strcmp ((char const *) *attrs, "type")) {
			if (strcmp ((char const *) attrs[1], "discrete")) {
				if (state->theme->cm != NULL) {
					g_warning ("[GogTheme]: extra GogAxisColorMap found.");
					g_object_unref (map);
					return;
				}
				state->theme->cm =  map;
				if (strcmp ((char const *) attrs[1], "both"))
					return;
				g_object_ref (map);
			}
			if (state->theme->dcm != NULL) {
				g_warning ("[GogTheme]: extra GogAxisColorMap found.");
				g_object_unref (map);
				return;
			}
			state->theme->dcm = map;
		}
}

static GsfXMLInNode const theme_dtd[] = {
	GSF_XML_IN_NODE (THEME, THEME, -1, "GogTheme", GSF_XML_NO_CONTENT, theme_start, NULL),
		GSF_XML_IN_NODE (THEME, NAME, -1, "name", GSF_XML_CONTENT, name_start, name_end),
		GSF_XML_IN_NODE (THEME, UNAME, -1, "_name", GSF_XML_CONTENT, name_start, name_end),
		GSF_XML_IN_NODE (THEME, DESCRIPTION, -1, "description", GSF_XML_CONTENT, name_start, desc_end),
		GSF_XML_IN_NODE (THEME, UDESCRIPTION, -1, "_description", GSF_XML_CONTENT, name_start, desc_end),
		GSF_XML_IN_NODE (THEME, STYLE, -1, "GOStyle", GSF_XML_NO_CONTENT, elem_start, NULL),
		GSF_XML_IN_NODE (THEME, COLORMAP, -1, "GogAxisColormap", GSF_XML_NO_CONTENT, color_map_start, NULL),
	GSF_XML_IN_NODE_END
};
static GsfXMLInDoc *xml = NULL;

static void map_area_series_solid_palette (GOStyle *, unsigned , GogTheme const *);
static void map_area_series_solid_default (GOStyle *, unsigned , GogTheme const *);
static void
theme_loaded (struct theme_load_state *state)
{
	/* initialize a dummy GogSeries style */
	GOStyle *style = go_style_new ();
	go_style_clear_auto (style);
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0; /* hairline */
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_PATTERN;
	if (state->theme->palette && state->theme->palette->len > 0)
		gog_theme_add_element (state->theme, style,
			   map_area_series_solid_palette, g_strdup ("GogSeries"), NULL);
	else
		gog_theme_add_element (state->theme, style,
			   map_area_series_solid_default, g_strdup ("GogSeries"), NULL);
	if (state->theme->dcm == NULL) {
		if (state->theme->palette) {
			GOColor *colors = g_new (GOColor, state->theme->palette->len);
			unsigned i;
			for (i = 0; i < state->theme->palette->len; i++)
				colors[i] = GO_STYLE (g_ptr_array_index (state->theme->palette, i))->fill.pattern.back;
			state->theme->dcm = gog_axis_color_map_from_colors ("Default",
				                                                state->theme->palette->len,
				                                                colors,
				                                                GO_RESOURCE_GENERATED);
			g_free (colors);
		} else {
			GogTheme *theme = gog_theme_registry_lookup ("Default");
			state->theme->dcm = g_object_ref (theme->dcm);
		}
	}
	state->theme->name = state->name;
	state->theme->description = state->desc;
}

static void
parse_done_cb (GsfXMLIn *xin, struct theme_load_state *state)
{
	if (state->theme->name == NULL)
		theme_loaded (state);
	else {
		g_free (state->name);
		g_free (state->desc);
	}
	g_free (state->lang);
	g_slist_free_full (state->garbage, g_object_unref);
	g_free (state);
}

static void
gog_theme_prep_sax (GOPersist *gp, GsfXMLIn *xin, xmlChar const **attrs)
{
	struct theme_load_state *state;

	g_return_if_fail (GOG_IS_THEME (gp));

	state = g_new (struct theme_load_state, 1);
	state->theme = GOG_THEME (gp);
	state->name = NULL;
	state->desc = NULL;
	state->lang = NULL;
	state->langs = g_get_language_names ();
	state->name_lang_score = G_MAXINT;
	state->desc_lang_score = G_MAXINT;
	state->garbage = NULL;
	if (!xml)
		xml = gsf_xml_in_doc_new (theme_dtd, NULL);
	gsf_xml_in_push_state (xin, xml, state, (GsfXMLInExtDtor) parse_done_cb, attrs);
}

static void
gog_theme_persist_init (GOPersistClass *iface)
{
	iface->prep_sax = gog_theme_prep_sax;
	iface->sax_save = gog_theme_sax_save;
}

GSF_CLASS_FULL (GogTheme, gog_theme,
                NULL, NULL, gog_theme_class_init, NULL,
                gog_theme_init, G_TYPE_OBJECT, 0,
                GSF_INTERFACE (gog_theme_persist_init, GO_TYPE_PERSIST))


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
		key.role_id = (char *) obj->role->id;
		key.klass_name = (char *) G_OBJECT_TYPE_NAME (obj->parent);
		elem = g_hash_table_lookup (theme->elem_hash_by_role, &key);
	}

	if (elem == NULL && obj->role != NULL) {
		GogThemeElement key;

		/* Search by generic role */
		key.role_id = (char *) obj->role->id;
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
 * gog_theme_fillin_style:
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

	if (relevant_fields == GO_STYLE_ALL) {
		go_style_assign (style, elem->style);
		go_style_force_auto (style);
	} else
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
gog_theme_new (char const *name, GoResourceType type)
{
	GogTheme *theme = g_object_new (GOG_TYPE_THEME, NULL);
	theme->name = g_strdup (_(name));
	if (type == GO_RESOURCE_NATIVE)
		theme->id = g_strdup (name);
	theme->type = type;
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

/**
 * gog_theme_get_id:
 * @theme: a #GogTheme
 *
 * Retrieves the theme Id.
 * Returns: the GogTheme Id.
 **/
char const *
gog_theme_get_id (GogTheme const *theme)
{
	g_return_val_if_fail (GOG_IS_THEME (theme), "");
	return theme->id;
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
 * gog_theme_get_descrition:
 * @theme: a #GogTheme
 *
 * Returns: the localized GogTheme description.
 **/

char const *
gog_theme_get_description (GogTheme const *theme)
{
	g_return_val_if_fail (GOG_IS_THEME (theme), "");
	return theme->description;
}

/**
 * gog_themep_get_resource_type:
 * @theme: a #GogTheme
 *
 * Retrieves the resource type for @theme.
 * Returns: the resource type.
 **/
GoResourceType
gog_theme_get_resource_type (GogTheme const *theme)
{
	g_return_val_if_fail (GOG_IS_THEME (theme), GO_RESOURCE_INVALID);
	return theme->type;
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

	if (shape >= G_N_ELEMENTS (shape_palette))
		shape %= G_N_ELEMENTS (shape_palette);

	if (mark->auto_shape)
		go_marker_set_shape (mark->mark, shape_palette [shape]);
	if (mark->auto_outline_color)
		go_marker_set_outline_color (mark->mark,
			palette [palette_index]);
	if (mark->auto_fill_color)
		go_marker_set_fill_color (mark->mark, palette [palette_index]);
}

static GOColor const default_palette [] = {
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

static void
map_area_series_solid_default (GOStyle *style, unsigned ind, G_GNUC_UNUSED GogTheme const *theme)
{
	unsigned palette_index = ind;
	if (palette_index >= G_N_ELEMENTS (default_palette))
		palette_index %= G_N_ELEMENTS (default_palette);
	if (style->fill.auto_back) {
		style->fill.pattern.back = default_palette [palette_index];

		/* force the brightness to reinterpolate */
		if (style->fill.type == GO_STYLE_FILL_GRADIENT &&
		    style->fill.gradient.brightness >= 0)
			go_style_set_fill_brightness (style,
				style->fill.gradient.brightness);
	}

	palette_index += 8;
	if (palette_index >= G_N_ELEMENTS (default_palette))
		palette_index -= G_N_ELEMENTS (default_palette);
	if (style->line.auto_color && !(style->disable_theming & GO_STYLE_LINE))
		style->line.color = default_palette [palette_index];
	if (!(style->disable_theming & GO_STYLE_MARKER))
		map_marker (&style->marker, ind, palette_index, default_palette);
}

static GOColor const guppi_palette[] = {
	0xff3000ff, 0x80ff00ff, 0x00ffcfff, 0x2000ffff,
	0xff008fff, 0xffbf00ff, 0x00ff10ff, 0x009fffff,
	0xaf00ffff, 0xff0000ff, 0xafff00ff, 0x00ff9fff,
	0x0010ffff, 0xff00bfff, 0xff8f00ff, 0x20ff00ff,
	0x00cfffff, 0x8000ffff, 0xff0030ff, 0xdfff00ff,
	0x00ff70ff, 0x0040ffff, 0xff00efff, 0xff6000ff,
	0x50ff00ff, 0x00ffffff, 0x5000ffff, 0xff0060ff,
	0xffef00ff, 0x00ff40ff, 0x0070ffff, 0xdf00ffff
};

static void
map_area_series_solid_guppi (GOStyle *style, unsigned ind, G_GNUC_UNUSED GogTheme const *theme)
{
	unsigned palette_index = ind;
	if (palette_index >= G_N_ELEMENTS (guppi_palette))
		palette_index %= G_N_ELEMENTS (guppi_palette);
	if (style->fill.auto_back) {
		style->fill.pattern.back = guppi_palette [palette_index];
		if (style->fill.type == GO_STYLE_FILL_GRADIENT &&
		    style->fill.gradient.brightness >= 0)
			/* force the brightness to reinterpolate */
			go_style_set_fill_brightness (style,
				style->fill.gradient.brightness);
	}
	if (style->line.auto_color && !(style->disable_theming & GO_STYLE_LINE))
		style->line.color = guppi_palette [palette_index];
	if (!(style->disable_theming & GO_STYLE_MARKER))
		map_marker (&style->marker, ind, palette_index, guppi_palette);
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
 * @is_default: bool
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
 * Returns: (transfer none): a #GogTheme from theme registry.
 **/
GogTheme *
gog_theme_registry_lookup (char const *name)
{
	GSList *ptr;
	GogTheme *theme = default_theme;

	if (name != NULL) {
		for (ptr = themes ; ptr != NULL ; ptr = ptr->next) {
			theme = ptr->data;
			if (!strcmp (theme->id, name))
				return theme;
		}
		if (strlen (name) != 36 || name[8] != '-' || name[13] != '-' || name[18] !='-' || name[23] != '-') {
			/* name does not seem to be an uuid, migth be the theme name (needed for compatibility) */
			char const *found_name;
			for (ptr = themes ; ptr != NULL ; ptr = ptr->next) {
				theme = ptr->data;
				found_name = g_hash_table_lookup (theme->names, "C");
				if (found_name && !strcmp (found_name, name))
					return theme;
			}
		}
		/* create an empty theme */
		theme = g_object_new (GOG_TYPE_THEME, "resource-type", GO_RESOURCE_EXTERNAL, NULL);
		theme->id = g_strdup (name);
		gog_theme_registry_add (theme, FALSE);
	}
	return theme;
}

/**
 * gog_theme_registry_get_theme_names:
 *
 * Returns: (element-type utf8) (transfer container): a newly allocated theme name list from theme registry.
 **/
GSList *
gog_theme_registry_get_theme_names (void)
{
	GogTheme *theme;
	GSList *names = NULL;
	GSList *ptr;

	for (ptr = themes; ptr != NULL; ptr = ptr->next) {
		theme = ptr->data;
		names = g_slist_append (names, theme->id);
	}

	return names;
}

/**************************************************************************/

static void
build_predefined_themes (void)
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
		g_hash_table_insert (global_class_aliases,
			(gpointer)"GogSeriesLabels", (gpointer)"GogLabel");
		g_hash_table_insert (global_class_aliases,
			(gpointer)"GogDataLabel", (gpointer)"GogLabel");
#ifdef GOFFICE_WITH_LASEM
		g_hash_table_insert (global_class_aliases,
			(gpointer)"GogEquation", (gpointer)"GogLabel");
#endif
	}

	/* An MS Excel-ish theme */
	theme = gog_theme_new (N_("Default"), GO_RESOURCE_NATIVE);
	theme->description = g_strdup (_("An MS Excel like theme"));
	gog_theme_registry_add (theme, TRUE);

	/* graph */
	style = go_style_new ();
	go_style_clear_auto (style);
	style->line.dash_type = GO_LINE_NONE;
	style->line.width = 0;
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_NONE;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_WHITE);
	gog_theme_add_element (theme, style, NULL, g_strdup ("GogGraph"), NULL);

	/* chart */
	style = go_style_new ();
	go_style_clear_auto (style);
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0; /* hairline */
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_PATTERN;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_WHITE);
	gog_theme_add_element (theme, style, NULL,  g_strdup ("GogChart"), NULL);

	/* Legend */
	style = go_style_new ();
	go_style_clear_auto (style);
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0; /* hairline */
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_PATTERN;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_WHITE);
	gog_theme_add_element (theme, style, NULL,  g_strdup ("GogLegend"), NULL);

	/* Axis */
	style = go_style_new ();
	go_style_clear_auto (style);
	style->line.width = 0; /* hairline */
	style->line.color = GO_COLOR_BLACK;
	gog_theme_add_element (theme, style, NULL,  g_strdup ("GogAxis"), NULL);

	/* AxisLine */
	style = go_style_new ();
	go_style_clear_auto (style);
	style->line.width = 0; /* hairline */
	style->line.color = GO_COLOR_BLACK;
	gog_theme_add_element (theme, style, NULL,  g_strdup ("GogAxisLine"), NULL);

	/* Grid */
	style = go_style_new ();
	go_style_clear_auto (style);
	style->fill.type  = GO_STYLE_FILL_PATTERN;
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 1.;
	style->line.color = 0X848284ff;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_GREY (0xd0));
	gog_theme_add_element (theme, style, NULL,  g_strdup ("GogGrid"), NULL);

	/* GridLine */
	style = go_style_new ();
	go_style_clear_auto (style);
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0.4;
	style->line.color = GO_COLOR_BLACK;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_FROM_RGBA (0xE0, 0xE0, 0xE0, 0xE0));
	style->fill.type = GO_STYLE_FILL_NONE;
	gog_theme_add_element (theme, style, NULL, NULL,  g_strdup ("MajorGrid"));
	style = go_style_new ();
	go_style_clear_auto (style);
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0.2;
	style->line.color = GO_COLOR_BLACK;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_FROM_RGBA (0xE0, 0xE0, 0xE0, 0xE0));
	style->fill.type = GO_STYLE_FILL_NONE;
	gog_theme_add_element (theme, style, NULL, NULL,  g_strdup ("MinorGrid"));

	/* Series */
	style = go_style_new ();
	go_style_clear_auto (style);
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0; /* hairline */
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_PATTERN;
	/* FIXME: not really true, will want to split area from line */
	gog_theme_add_element (theme, style,
		map_area_series_solid_default,  g_strdup ("GogSeries"), NULL);

	/* Chart titles */
	style = go_style_new ();
	go_style_clear_auto (style);
	style->line.dash_type = GO_LINE_NONE;
	style->line.width = 0.;
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_NONE;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_WHITE);
	go_style_set_font_desc (style, pango_font_description_from_string ("Sans Bold 12"));
	gog_theme_add_element (theme, style, NULL,  g_strdup ("GogChart"), g_strdup ("Title"));

	/* labels */
	style = go_style_new ();
	go_style_clear_auto (style);
	style->line.dash_type = GO_LINE_NONE;
	style->line.width = 0.;
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_NONE;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_WHITE);
	go_style_set_font_desc (style, pango_font_description_from_string ("Sans 10"));
	gog_theme_add_element (theme, style, NULL,  g_strdup ("GogLabel"), NULL);

	/* regression curves */
	style = go_style_new ();
	go_style_clear_auto (style);
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 1;
	style->line.color = GO_COLOR_BLACK;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_FROM_RGBA (0x00, 0x00, 0x00, 0x20));
	style->fill.type = GO_STYLE_FILL_NONE;
	gog_theme_add_element (theme, style,
		NULL,  g_strdup ("GogTrendLine"), NULL);

	/* regression equations */
	style = go_style_new ();
	go_style_clear_auto (style);
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0; /* hairline */
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_PATTERN;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_WHITE);
	gog_theme_add_element (theme, style,
		NULL,  g_strdup ("GogRegEqn"), NULL);

	/* series labels */
	style = go_style_new ();
	go_style_clear_auto (style);
	style->line.dash_type = GO_LINE_NONE;
	style->line.width = 0; /* none */
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_NONE;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_WHITE);
	go_style_set_font_desc (style, pango_font_description_from_string ("Sans 6"));
	gog_theme_add_element (theme, style, NULL,  g_strdup ("GogSeriesLabels"), NULL);

	/* data label */
	style = go_style_new ();
	style->line.dash_type = GO_LINE_NONE;
	style->line.width = 0; /* none */
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_NONE;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_WHITE);
	go_style_set_font_desc (style, pango_font_description_from_string ("Sans 6"));
	gog_theme_add_element (theme, style, NULL,  g_strdup ("GogDataLabel"), NULL);

	/* Color scale */
	style = go_style_new ();
	go_style_clear_auto (style);
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0; /* hairline */
	style->line.color = GO_COLOR_BLACK;
	gog_theme_add_element (theme, style, NULL,  g_strdup ("GogColorScale"), NULL);

#ifdef GOFFICE_WITH_LASEM
	/* Equations */
	style = go_style_new ();
	go_style_clear_auto (style);
	style->line.dash_type = GO_LINE_NONE;
	style->line.width = 0; /* hairline */
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_NONE;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_WHITE);
	go_style_set_font_desc (style, pango_font_description_from_string ("Sans 10"));
	gog_theme_add_element (theme, style,
		NULL,  g_strdup ("GogEquation"), NULL);
#endif
	/* builds the default discrete color map */
	theme->dcm = gog_axis_color_map_from_colors (N_("Theme"),
	                                             G_N_ELEMENTS (default_palette),
	                                             default_palette, GO_RESOURCE_GENERATED);

/* Guppi */
	theme = gog_theme_new (N_("Guppi"), GO_RESOURCE_NATIVE);
	theme->description = g_strdup (_("Guppi theme"));
	gog_theme_registry_add (theme, FALSE);

	/* graph */
	style = go_style_new ();
	go_style_clear_auto (style);
	style->line.dash_type = GO_LINE_NONE;
	style->line.width = 0;
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_GRADIENT;
	style->fill.gradient.dir   = GO_GRADIENT_N_TO_S;
	style->fill.pattern.fore = GO_COLOR_BLUE;
	style->fill.pattern.back = GO_COLOR_BLACK;
	gog_theme_add_element (theme, style, NULL,  g_strdup ("GogGraph"), NULL);

	/* chart */
	style = go_style_new ();
	go_style_clear_auto (style);
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0; /* hairline */
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_PATTERN;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_WHITE);
	gog_theme_add_element (theme, style, NULL,  g_strdup ("GogChart"), NULL);

	/* legend */
	style = go_style_new ();
	go_style_clear_auto (style);
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0; /* hairline */
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_PATTERN;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_WHITE);
	gog_theme_add_element (theme, style, NULL,  g_strdup ("GogLegend"), NULL);

	/* Axis */
	style = go_style_new ();
	go_style_clear_auto (style);
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0.; /* hairline */
	style->line.color = GO_COLOR_GREY (0x20);
	gog_theme_add_element (theme, style, NULL,  g_strdup ("GogAxis"), NULL);

	/* AxisLine */
	style = go_style_new ();
	go_style_clear_auto (style);
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0.; /* hairline */
	style->line.color = GO_COLOR_GREY (0x20);
	gog_theme_add_element (theme, style, NULL,  g_strdup ("GogAxisLine"), NULL);

	/* Grid */
	style = go_style_new ();
	go_style_clear_auto (style);
	style->fill.type  = GO_STYLE_FILL_PATTERN;
	style->line.dash_type = GO_LINE_NONE;
	style->line.color = GO_COLOR_BLACK;
	style->line.width = 0;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_GREY (0xd0));
	gog_theme_add_element (theme, style, NULL,  g_strdup ("GogGrid"), NULL);

	/* GridLine */
	style = go_style_new ();
	go_style_clear_auto (style);
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0.; /* hairline */
	style->line.color = GO_COLOR_GREY (0x96);
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_FROM_RGBA (0xE0, 0xE0, 0xE0, 0xE0));
	style->fill.type = GO_STYLE_FILL_NONE;
	gog_theme_add_element (theme, style, NULL, NULL,  g_strdup ("MajorGrid"));
	style = go_style_new ();
	go_style_clear_auto (style);
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0.; /* hairline */
	style->line.color = GO_COLOR_GREY (0xC0);
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_FROM_RGBA (0xE0, 0xE0, 0xE0, 0xE0));
	style->fill.type = GO_STYLE_FILL_NONE;
	gog_theme_add_element (theme, style, NULL, NULL,  g_strdup ("MinorGrid"));

	/* series */
	style = go_style_new ();
	go_style_clear_auto (style);
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0.; /* hairline */
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_PATTERN;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_GREY (0x20));
	/* FIXME: not really true, will want to split area from line */
	gog_theme_add_element (theme, style,
		map_area_series_solid_guppi,  g_strdup ("GogSeries"), NULL);

	/* labels */
	style = go_style_new ();
	go_style_clear_auto (style);
	style->line.dash_type = GO_LINE_NONE;
	style->line.width = 0; /* none */
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_NONE;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_WHITE);
	gog_theme_add_element (theme, style, NULL,  g_strdup ("GogLabel"), NULL);

	/* trend lines */
	style = go_style_new ();
	go_style_clear_auto (style);
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 1;
	style->line.color = GO_COLOR_BLACK;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_FROM_RGBA (0x00, 0x00, 0x00, 0x20));
	style->fill.type = GO_STYLE_FILL_NONE;
	gog_theme_add_element (theme, style,
		NULL,  g_strdup ("GogTrendLine"), NULL);

	/* regression equations */
	style = go_style_new ();
	go_style_clear_auto (style);
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0; /* hairline */
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_PATTERN;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_WHITE);
	gog_theme_add_element (theme, style,
		NULL,  g_strdup ("GogRegEqn"), NULL);

	/* series labels */
	style = go_style_new ();
	go_style_clear_auto (style);
	style->line.dash_type = GO_LINE_NONE;
	style->line.width = 0; /* none */
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_NONE;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_WHITE);
	go_style_set_font_desc (style, pango_font_description_from_string ("Sans 6"));
	gog_theme_add_element (theme, style, NULL,  g_strdup ("GogSeriesLabels"), NULL);

	/* data label */
	style = go_style_new ();
	go_style_clear_auto (style);
	style->line.dash_type = GO_LINE_NONE;
	style->line.width = 0; /* none */
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_NONE;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_WHITE);
	go_style_set_font_desc (style, pango_font_description_from_string ("Sans 6"));
	gog_theme_add_element (theme, style, NULL,  g_strdup ("GogDataLabel"), NULL);

	/* Color scale */
	style = go_style_new ();
	go_style_clear_auto (style);
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0; /* hairline */
	style->line.color = GO_COLOR_BLACK;
	gog_theme_add_element (theme, style, NULL,  g_strdup ("GogColorScale"), NULL);

#ifdef GOFFICE_WITH_LASEM
	/* Equations */
	style = go_style_new ();
	go_style_clear_auto (style);
	style->line.dash_type = GO_LINE_NONE;
	style->line.width = 0; /* hairline */
	style->line.color = GO_COLOR_BLACK;
	style->fill.type = GO_STYLE_FILL_NONE;
	go_pattern_set_solid (&style->fill.pattern, GO_COLOR_WHITE);
	go_style_set_font_desc (style, pango_font_description_from_string ("Sans 10"));
	gog_theme_add_element (theme, style,
		NULL,  g_strdup ("GogEquation"), NULL);
#endif

	theme->dcm = gog_axis_color_map_from_colors (N_("Theme"),
	                                             G_N_ELEMENTS (guppi_palette),
	                                             guppi_palette, GO_RESOURCE_GENERATED);
}

static void
theme_load_from_uri (char const *uri)
{
	struct theme_load_state	state;
	GsfInput *input = go_file_open (uri, NULL);

	if (input == NULL) {
		g_warning ("[GogTheme]: Could not open %s", uri);
		return;
	}
	state.theme = NULL;
	state.desc = state.lang = state.name = NULL;
	state.langs = g_get_language_names ();
	state.name_lang_score = state.desc_lang_score = G_MAXINT;
	if (!xml)
		xml = gsf_xml_in_doc_new (theme_dtd, NULL);
	if (!gsf_xml_in_doc_parse (xml, input, &state))
		g_warning ("[GogTheme]: Could not parse %s", uri);
	if (state.theme != NULL) {
		if (!go_file_access (uri, GO_W_OK)) {
			state.theme->uri = g_strdup (uri);
			if (state.theme->id == NULL) {
				state.theme->id = go_uuid ();
				gog_theme_save (state.theme);
			}
			state.theme->type = GO_RESOURCE_RW;
		} else {
			if (state.theme->id == NULL) {
				/* just duplicate the name, anyway, this should never occur */
				char *name = g_hash_table_lookup (state.theme->names, "C");
				if (name)
					state.theme->id = g_strdup (name);
				else
					g_warning ("[GogTheme]: Theme with no Id in %s", uri);
			}
			state.theme->type = GO_RESOURCE_RO;
		}
		theme_loaded (&state);
		gog_theme_registry_add (state.theme, FALSE);
	} else {
		g_free (state.name);
		g_free (state.desc);
	}
	g_free (state.lang);
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

	_gog_axis_color_maps_init ();
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

	g_slist_free_full (g_slist_copy (themes), g_object_unref);
	g_slist_free (themes);
	g_hash_table_destroy (global_class_aliases);
	_gog_axis_color_maps_shutdown ();
	themes = NULL;
	if (xml)
		gsf_xml_in_doc_free (xml);
}

/**
 * gog_theme_get_color_map:
 * @theme: #GogTheme
 * @discrete: whether the map is for a discrete axis.
 *
 * Retrieves the themed color map. Each theme has a discrete color map and a
 * continuous one.
 * Returns: (transfer none): the requested color map.
 **/
GogAxisColorMap const *
gog_theme_get_color_map (GogTheme const *theme, gboolean discrete)
{
	g_return_val_if_fail (GOG_IS_THEME (theme), NULL);
	if (discrete)
		return theme->dcm;
	else
		return (theme->cm)? theme->cm: _gog_axis_color_map_get_default ();
	return NULL;
}

/**
 * gog_theme_delete:
 * @theme: a #GogTheme
 *
 * Destroys the theme and remove it from the user directory and from the
 * database.
 * Returns: %TRUE on success.
 **/
gboolean
gog_theme_delete (GogTheme *theme)
{
	GFile *file = g_file_new_for_uri (theme->uri);
	gboolean res;
	if ((res = g_file_delete (file, NULL, NULL))) {
		themes = g_slist_remove (themes, theme);
		g_object_unref (theme);
	}
	g_object_unref (file);
	return res;
}

/*****************
 * Theme edition *
 *****************/

/**
 * gog_theme_foreach:
 * @handler: (scope call): a #GFunc using a theme as first argument
 * @user_data: data to pass to @handler
 *
 * Executes @handler to each theme installed on the system, including built-in
 * themes.
 **/
void
gog_theme_foreach (GFunc handler, gpointer user_data)
{
	g_slist_foreach (themes, handler, user_data);
}

static void
gog_theme_set_name (GogTheme *theme, char const *name)
{
	g_return_if_fail (GOG_IS_THEME (theme));
	g_free (theme->name);
	g_hash_table_remove_all (theme->names);
	theme->name = g_strdup (name);
	g_hash_table_insert (theme->names, g_strdup ("C"), g_strdup (name));
}
static void
gog_theme_set_description (GogTheme *theme, char const *desc)
{
	g_return_if_fail (GOG_IS_THEME (theme));
	g_free (theme->description);
	g_hash_table_remove_all (theme->descs);
	theme->description = g_strdup (desc);
	g_hash_table_insert (theme->descs, g_strdup ("C"), g_strdup (desc));
}

/**
 * gog_theme_dup:
 * @theme: a #GogTheme
 *
 * Duplicates @theme with a new Id.
 * Returns: (transfer full): the new theme.
 **/
GogTheme*
gog_theme_dup (GogTheme *theme)
{
	GogTheme *new_theme;
	char *desc, *name;

	g_return_val_if_fail (GOG_IS_THEME (theme), NULL);
	new_theme = g_object_new (GOG_TYPE_THEME,
	                          "resource-type", GO_RESOURCE_RW,
	                          NULL);
	new_theme->id = go_uuid ();
	gog_theme_build_uri (new_theme);
	gog_theme_set_name (new_theme, "New theme");
	name = g_hash_table_lookup (theme->names, "C");
	desc = g_strdup_printf ("New theme base on %s", name);
	gog_theme_set_description (new_theme, desc);
	g_free (desc);
	/* duplicate the styles */
	/* duplicate the color maps */
	if (theme->cm) {
		new_theme->cm = gog_axis_color_map_dup (theme->cm);
		g_object_set (G_OBJECT (new_theme->cm),
			          "resource-type", GO_RESOURCE_CHILD,
			          NULL);
	}
	if (theme->dcm &&
	    gog_axis_color_map_get_resource_type (theme->dcm) == GO_RESOURCE_CHILD) {
			new_theme->dcm = gog_axis_color_map_dup (theme->dcm);
			g_object_set (G_OBJECT (new_theme->dcm),
					      "resource-type", GO_RESOURCE_CHILD,
					      NULL);
		}
	return new_theme;
}

#ifdef GOFFICE_WITH_GTK

struct theme_edit_state {
	GtkBuilder *gui;
	GogTheme *theme;
};

static void
create_toggled_cb (GtkListStore *list, char const *path)
{
	GtkTreeIter iter;
	gboolean set;
	gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (list), &iter, path);
	gtk_tree_model_get (GTK_TREE_MODEL (list), &iter, 1, &set, -1);
	gtk_list_store_set (list, &iter, 1, !set, -1);
}

/**
 * gog_theme_edit:
 * @theme: the #GogTheme to edit or %NULL to create a new one.
 * @cc: a #GOCmdContext or %NULL.
 *
 * Displays a dialog box to edit the theme. This can be done only for
 * locally installed themes that are writeable.
 * Returns: (transfer none): The edited theme or %NULL if the edition has
 * been cancelled.
 **/
GogTheme *
gog_theme_edit (GogTheme *theme, GOCmdContext *cc)
{
	struct theme_edit_state state;
	GtkWidget *w;

	if (!GOG_IS_THEME (theme)) {
		/* display a dialog box to select used roles and series number */
		GtkBuilder *gui = go_gtk_builder_load_internal ("res:go:graph/new-theme-prefs.ui", GETTEXT_PACKAGE, cc);
		int response;
		GtkWidget *w;
		GtkListStore *l = GTK_LIST_STORE (gtk_builder_get_object (gui, "classes-list"));
		GtkTreeView *tv = GTK_TREE_VIEW (gtk_builder_get_object (gui, "classes-tree"));
		GtkCellRenderer *renderer;
		GtkTreeViewColumn *column;
		GtkTreeIter iter;
		unsigned i;

		renderer = gtk_cell_renderer_text_new ();
		column = gtk_tree_view_column_new_with_attributes (_("Class"), renderer, "text", 0, NULL);
		gtk_tree_view_append_column (tv, column);
		renderer = gtk_cell_renderer_toggle_new ();
		column = gtk_tree_view_column_new_with_attributes (_("Create"), renderer, "active", 1, NULL);
		gtk_tree_view_append_column (tv, column);
		for (i = 0; i < G_N_ELEMENTS (roles); i++) {
			if (!strcmp (roles[i].klass_name, "Series"))
				continue;
			gtk_list_store_append (l, &iter);
			gtk_list_store_set (l, &iter, 0, _(roles[i].label), 1, FALSE, 2, i, -1);
		}
		g_signal_connect_swapped (renderer, "toggled", G_CALLBACK (create_toggled_cb), l);

		w = go_gtk_builder_get_widget (gui, "new-theme-prefs");
		response = gtk_dialog_run (GTK_DIALOG (w));
		gtk_widget_destroy (w);
		g_object_unref (gui);
		if (response == 1) {
			theme = gog_theme_new (_("New theme"), FALSE);
			theme->id = go_uuid ();
			theme->type = GO_RESOURCE_RW;
		} else
			return NULL;
	}

	state.theme = theme;
	state.gui = go_gtk_builder_load_internal ("res:go:graph/gog-theme-editor.ui", GETTEXT_PACKAGE, cc);

	w = go_gtk_builder_get_widget (state.gui, "gog-theme-editor");
	if (gtk_dialog_run (GTK_DIALOG (w))) {
	}
	gtk_widget_destroy (w);
	g_object_unref (state.gui);
	return NULL;
}

#endif

