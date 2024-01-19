/*
 * gog-axis-color-map.c
 *
 * Copyright (C) 2012 Jean Brefort (jean.brefort@normalesup.org)
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
#include <goffice/goffice.h>
#include <goffice/goffice-priv.h>

#include <gsf/gsf-input.h>
#include <gsf/gsf-output-gio.h>
#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#ifdef GOFFICE_WITH_GTK
#include <gtk/gtk.h>
#endif
#include <sys/stat.h>

/**
 * SECTION: gog-axis-color-map
 * @short_description: map values to colors.
 *
 * Used to map color and pseudo-3d axes values to the actual color. The first
 * color maps 0 and the last a positive integer returned by
 * gog_axis_color_map_get_max(). For color axes, these integer values must
 * themselves be mapped to the minimum and maximum of the axis (unless the
 * axis is inverted). For pseudo-3d axes, successive colors are obtained for
 * integer values, cycling to the first color when the colors number is not
 * large enough.
 **/
struct _GogAxisColorMap {
	GObject base;
	char *id, *name;
	char *uri;
	GHashTable *names;
	GoResourceType type;
	unsigned size; /* colors number */
	unsigned allocated; /* only useful when editing */
	unsigned *limits;
	GOColor *colors;
};
typedef GObjectClass GogAxisColorMapClass;

static GObjectClass *parent_klass;

enum {
	GOG_AXIS_COLOR_MAP_PROP_0,
	GOG_AXIS_COLOR_MAP_PROP_TYPE
};

static void
gog_axis_color_map_set_property (GObject *gobject, guint param_id,
                                 GValue const *value, GParamSpec *pspec)
{
	GogAxisColorMap *map = GOG_AXIS_COLOR_MAP (gobject);

	switch (param_id) {
	case GOG_AXIS_COLOR_MAP_PROP_TYPE:
		map->type = g_value_get_enum (value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
}

static void
gog_axis_color_map_get_property (GObject *gobject, guint param_id,
                                 GValue *value, GParamSpec *pspec)
{
	GogAxisColorMap *map = GOG_AXIS_COLOR_MAP (gobject);

	switch (param_id) {
	case GOG_AXIS_COLOR_MAP_PROP_TYPE:
		g_value_set_enum (value, map->type);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
}

static void
gog_axis_color_map_finalize (GObject *obj)
{
	GogAxisColorMap *map = GOG_AXIS_COLOR_MAP (obj);
	g_free (map->id);
	map->id = NULL;
	g_free (map->name);
	map->name = NULL;
	g_free (map->uri);
	g_free (map->limits);
	map->limits = NULL;
	g_free (map->colors);
	map->colors = NULL;
	if (map->names)
		g_hash_table_destroy (map->names);
	map->names = NULL;
	parent_klass->finalize (obj);
}

static void
gog_axis_color_map_class_init (GObjectClass *gobject_klass)
{
	parent_klass = g_type_class_peek_parent (gobject_klass);
	/* GObjectClass */
	gobject_klass->finalize = gog_axis_color_map_finalize;
	gobject_klass->set_property = gog_axis_color_map_set_property;
	gobject_klass->get_property = gog_axis_color_map_get_property;
	g_object_class_install_property (gobject_klass, GOG_AXIS_COLOR_MAP_PROP_TYPE,
		g_param_spec_enum ("resource-type",
			_("Resource type"),
			_("The resource type for the color map"),
			go_resource_type_get_type (), GO_RESOURCE_INVALID,
		        GSF_PARAM_STATIC | G_PARAM_READWRITE |G_PARAM_CONSTRUCT_ONLY));
}

static void
gog_axis_color_map_init (GogAxisColorMap *map)
{
	map->names = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
}

static void
save_name_cb (char const *lang, char const *name, GsfXMLOut *output)
{
	gsf_xml_out_start_element (output, "name");
	if (strcmp (lang, "C"))
		gsf_xml_out_add_cstr_unchecked (output, "xml:lang", lang);
	gsf_xml_out_add_cstr (output, NULL, name);
	gsf_xml_out_end_element (output);
}

static void gog_axis_color_map_save (GogAxisColorMap const *map);

static void
build_uri (GogAxisColorMap *map)
{
	char *filename, *full_name;
	filename = g_strconcat (map->id, ".map", NULL);
	full_name = g_build_filename (g_get_home_dir (), ".goffice", "colormaps", filename, NULL);
	map->uri = go_filename_to_uri (full_name);
	g_free (filename);
	g_free (full_name);
}

static void
gog_axis_color_map_write (GOPersist const *gp, GsfXMLOut *output)
{
	unsigned i;
	char *buf;
	GogAxisColorMap *map;
	g_return_if_fail (GOG_IS_AXIS_COLOR_MAP (gp));

	map = GOG_AXIS_COLOR_MAP (gp);
	if (output == NULL) {
		g_return_if_fail (map->uri == NULL);
		build_uri (map);
		map->type = GO_RESOURCE_RW;
		gog_axis_color_map_save (map);
		return;
	}
	gsf_xml_out_add_cstr_unchecked (output, "id", map->id);
	g_hash_table_foreach (map->names, (GHFunc) save_name_cb, output);
	for (i = 0; i < map->size; i++) {
		gsf_xml_out_start_element (output, "color-stop");
		gsf_xml_out_add_uint (output, "bin", map->limits[i]);
		buf = go_color_as_str (map->colors[i]);
		gsf_xml_out_add_cstr_unchecked (output, "color", buf);
		g_free (buf);
		gsf_xml_out_end_element (output);
	}
}

struct _color_stop {
	unsigned bin;
	GOColor color;
};

struct color_map_load_state {
	GogAxisColorMap *map;
	char *lang, *name;
	unsigned name_lang_score;
	char const * const *langs;
	GSList *color_stops;
};

static void
color_stop_start (GsfXMLIn *xin, xmlChar const **attrs)
{
	struct color_map_load_state	*state = (struct color_map_load_state *) xin->user_state;
	GOColor color;
	unsigned bin = 0;
	char *end;
	gboolean color_found = FALSE;
	gboolean bin_found = FALSE;

	if (state->map->name)
		return;
	for (; attrs != NULL && *attrs ; attrs += 2)
		if (0 == strcmp (*attrs, "bin")) {
			bin = strtoul (attrs[1], &end, 10);
			if (*end == 0)
				bin_found = TRUE;
		} else if (0 == strcmp (*attrs, "color"))
			color_found = go_color_from_str (attrs[1], &color);
	if (color_found && bin_found) {
		struct _color_stop* stop = g_new (struct _color_stop, 1);
		stop->bin = bin;
		stop->color = color;
		state->color_stops = g_slist_append (state->color_stops, stop);
	} else
		g_warning ("[GogAxisColorMap]: Invalid color stop");
}

static void
map_start (GsfXMLIn *xin, xmlChar const **attrs)
{
	struct color_map_load_state	*state = (struct color_map_load_state *) xin->user_state;
	if (state->map ==  NULL) {
		state->map = g_object_new (GOG_TYPE_AXIS_COLOR_MAP, NULL);
		for (; attrs && *attrs; attrs +=2)
			if (!strcmp ((char const *) *attrs, "id")) {
				state->map->id = g_strdup ((char const *) attrs[1]);
				break;
			}
	}
}

static void
name_start (GsfXMLIn *xin, xmlChar const **attrs)
{
	struct color_map_load_state	*state = (struct color_map_load_state *) xin->user_state;
	unsigned i;
	if (state->map->name)
		return;
	for (i = 0; attrs != NULL && attrs[i] && attrs[i+1] ; i += 2)
		if (0 == strcmp (attrs[i], "xml:lang"))
			state->lang = g_strdup (attrs[i+1]);
}

static void
name_end (GsfXMLIn *xin, G_GNUC_UNUSED GsfXMLBlob *blob)
{
	struct color_map_load_state	*state = (struct color_map_load_state *) xin->user_state;
	char *name = NULL;
	if (state->map->name)
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
	g_hash_table_replace (state->map->names, state->lang, name);
	state->lang = NULL;
}

static int
color_stops_cmp (struct _color_stop *first, struct _color_stop *second)
{
	return (first->bin < second->bin)? -1: (int) (first->bin - second->bin);
}

static GsfXMLInNode const color_map_dtd[] = {
	GSF_XML_IN_NODE (THEME, THEME, -1, "GogAxisColorMap", GSF_XML_NO_CONTENT, map_start, NULL),
		GSF_XML_IN_NODE (THEME, NAME, -1, "name", GSF_XML_CONTENT, name_start, name_end),
		GSF_XML_IN_NODE (THEME, UNAME, -1, "_name", GSF_XML_CONTENT, name_start, name_end),
		GSF_XML_IN_NODE (THEME, STOP, -1, "color-stop", GSF_XML_CONTENT, color_stop_start, NULL),
	GSF_XML_IN_NODE_END
};
static GsfXMLInDoc *xml = NULL;

static void
gog_axis_color_map_save (GogAxisColorMap const *map)
{
	GsfOutput *output = gsf_output_gio_new_for_uri (map->uri, NULL);
	GsfXMLOut *xml;
	if (output == NULL) {
		char *dir = go_dirname_from_uri (map->uri, TRUE);
		int res = g_mkdir_with_parents (dir, 0777);
		g_free (dir);
		if (res < 0) {
			g_warning ("[GogAxisColorMap]: Could not save color map to %s", map->uri);
			return;
		}
		output = gsf_output_gio_new_for_uri (map->uri, NULL);
	}
	xml = gsf_xml_out_new (output);
	gsf_xml_out_start_element (xml, "GogAxisColorMap");
	gog_axis_color_map_write (GO_PERSIST (map), xml);
	gsf_xml_out_end_element (xml);
	g_object_unref (xml);
	g_object_unref (output);
}

static void
color_map_loaded (struct color_map_load_state *state, char const *uri, gboolean delete_invalid)
{
	GSList *ptr;
	if (!state->map || state->map->name)
		return;
	state->map->name = state->name;
	/* populates the colors */
	/* first sort the color list according to bins */
	ptr = state->color_stops = g_slist_sort (state->color_stops, (GCompareFunc) color_stops_cmp);
	if (((struct _color_stop *) ptr->data)->bin != 0) {
		g_warning ("[GogAxisColorMap]: Invalid color map in %s", uri);
		if (delete_invalid) {
			g_object_unref (state->map);
			state->map = NULL;
		}
	} else {
		unsigned cur_bin, n = 0;
		state->map->allocated = g_slist_length (state->color_stops);
		state->map->limits = g_new (unsigned, state->map->allocated);
		state->map->colors = g_new (GOColor, state->map->allocated);
		while (ptr) {
			cur_bin = state->map->limits[n] = ((struct _color_stop *) ptr->data)->bin;
			state->map->colors[n++] = ((struct _color_stop *) ptr->data)->color;
			do (ptr = ptr->next);
			while (ptr && ((struct _color_stop *) ptr->data)->bin == cur_bin);
		}
		state->map->size = n; /* we drop duplicate bins */
		if (state->map->id == NULL) {
			if (state->map->uri) {
				state->map->id = go_uuid ();
				gog_axis_color_map_save (state->map);
			} else {
				g_warning ("[GogAxisColorMap]: Map without Id in %s", uri);
				if (delete_invalid) {
					g_object_unref (state->map);
					state->map = NULL;
				}
			}
		}
	}
	g_slist_free_full (state->color_stops, g_free);
	g_free (state->lang);
}

static void
parse_done_cb (GsfXMLIn *xin, struct color_map_load_state *state)
{
	color_map_loaded (state, gsf_input_name (gsf_xml_in_get_input (xin)), FALSE);
	g_free (state);
}

static void
gog_axis_color_map_prep_sax (GOPersist *gp, GsfXMLIn *xin, xmlChar const **attrs)
{
	struct color_map_load_state *state;

	g_return_if_fail (GOG_IS_AXIS_COLOR_MAP (gp));

	state = g_new (struct color_map_load_state, 1);
	state->map = GOG_AXIS_COLOR_MAP (gp);
	state->name = NULL;
	state->lang = NULL;
	state->langs = g_get_language_names ();
	state->name_lang_score = G_MAXINT;
	state->color_stops = NULL;
	if (!xml)
		xml = gsf_xml_in_doc_new (color_map_dtd, NULL);
	gsf_xml_in_push_state (xin, xml, state, (GsfXMLInExtDtor) parse_done_cb, attrs);
}

static void
gog_axis_color_map_persist_init (GOPersistClass *iface)
{
	iface->prep_sax = gog_axis_color_map_prep_sax;
	iface->sax_save = gog_axis_color_map_write;
}

GSF_CLASS_FULL (GogAxisColorMap, gog_axis_color_map,
                NULL, NULL, gog_axis_color_map_class_init, NULL,
                gog_axis_color_map_init, G_TYPE_OBJECT, 0,
                GSF_INTERFACE (gog_axis_color_map_persist_init, GO_TYPE_PERSIST))

/**
 * gog_axis_color_map_get_color:
 * @map: a #GogAxisMap
	 * @x: the value to map
 *
 * Maps @x to a color.
 * Returns: the found color.
 **/
GOColor
gog_axis_color_map_get_color (GogAxisColorMap const *map, double x)
{
	unsigned n = 1;
	double t;
	g_return_val_if_fail (GOG_IS_AXIS_COLOR_MAP (map), (GOColor) 0x00000000);
	if (x < 0. || map->size == 0)
		return (GOColor) 0x00000000;
	if (map->size == 1)
		return map->colors[0];
	if (x > map->limits[map->size-1])
		x -= floor (x / (map->limits[map->size-1] + 1)) * (map->limits[map->size-1] + 1);
	while (n < map->size && x > map->limits[n] + 1e-10)
		n++;
	t = (x - map->limits[n-1]) / (map->limits[n] - map->limits[n-1]);
	return GO_COLOR_INTERPOLATE (map->colors[n-1], map->colors[n], t);
}

/**
 * gog_axis_color_map_get_max:
 * @map: a #GogAxisMap
 *
 * Retrieves the value corresponding to the last color in the map. The first
 * always corresponds to 0.
 * Returns: the maximum value.
 **/
unsigned
gog_axis_color_map_get_max (GogAxisColorMap const *map)
{
	g_return_val_if_fail (GOG_IS_AXIS_COLOR_MAP (map), 0);
	return (map->size > 0)? map->limits[map->size-1]: 0;
}

/**
 * gog_axis_color_map_get_id:
 * @map: a #GogAxisMap
 *
 * Retrieves the color map name.
 * Returns: (transfer none): the map name.
 **/
char const *
gog_axis_color_map_get_id (GogAxisColorMap const *map)
{
	g_return_val_if_fail (GOG_IS_AXIS_COLOR_MAP (map), NULL);
	return map->id;
}

/**
 * gog_axis_color_map_get_name:
 * @map: a #GogAxisMap
 *
 * Retrieves the color map localized name.
 * Returns: (transfer none): the map name.
 **/
char const *
gog_axis_color_map_get_name (GogAxisColorMap const *map)
{
	g_return_val_if_fail (GOG_IS_AXIS_COLOR_MAP (map), NULL);
	return map->name;
}

/**
 * gog_axis_color_map_get_resource_type:
 * @map: a #GogAxisMap
 *
 * Retrieves the resource type for @map.
 * Returns: the resource type.
 **/
GoResourceType
gog_axis_color_map_get_resource_type (GogAxisColorMap const *map)
{
	g_return_val_if_fail (GOG_IS_AXIS_COLOR_MAP (map), GO_RESOURCE_INVALID);
	return map->type;
}

/**
 * gog_axis_color_map_get_snapshot:
 * @map: a #GogAxisMap
 * @discrete: whether to use constant colors between each stop or a gradient.
 * @horizontal: whether to get an horizontal or a vertical snapshot.
 * @width: the pixbuf width.
 * @height: the pixbuf height.
 *
 * Builds a snapshot of the color map.
 * Returns: (transfer full): the new #GdkPixbuf.
 **/
GdkPixbuf *
gog_axis_color_map_get_snapshot (GogAxisColorMap const *map,
                                 gboolean discrete, gboolean horizontal,
                                 unsigned width, unsigned height)
{
	cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 16, 16);
	GdkPixbuf *pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, width, height);
	cairo_t *cr = cairo_create (surface);
	cairo_pattern_t *pattern;

	g_return_val_if_fail (GOG_IS_AXIS_COLOR_MAP (map), NULL);
	/* first fill with a "transparent" background */
	cairo_rectangle (cr, 0., 0., 16., 16.);
	cairo_set_source_rgba (cr, GO_COLOR_TO_CAIRO (GO_COLOR_GREY(0x33)));
	cairo_fill (cr);
	cairo_rectangle (cr, 0., 8., 8., 8.);
	cairo_set_source_rgba (cr, GO_COLOR_TO_CAIRO (GO_COLOR_GREY(0x66)));
	cairo_fill (cr);
	cairo_rectangle (cr, 8., 0., 8., 8.);
	cairo_fill (cr);
	cairo_destroy (cr);
	pattern = cairo_pattern_create_for_surface (surface);
	cairo_surface_destroy (surface);
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
	cr = cairo_create (surface);
	cairo_rectangle (cr, 0., 0., width, height);
	cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);
	cairo_set_source (cr, pattern);
	cairo_fill (cr);
	cairo_pattern_destroy (pattern);
	gog_axis_color_map_to_cairo (map, cr, discrete, horizontal, width, height);

	go_cairo_convert_data_to_pixbuf (gdk_pixbuf_get_pixels (pixbuf),
	                                 cairo_image_surface_get_data (surface),
	                                 width, height, width * 4);
	cairo_destroy (cr);
	cairo_surface_destroy (surface);
	return pixbuf;
}


/**
 * gog_axis_color_map_to_cairo:
 * @map: a #GogAxisMap
 * @cr: a cairo context.
 * @discrete: whether to use constant colors between each stop or a gradient.
 * @horizontal: whether to get an horizontal or a vertical snapshot.
 * @width: the rectangle width.
 * @height: the rectangle height.
 *
 * When @discrete is larger than 1, it will be interpreted as the number of
 * major ticks used. The number of colors will then be @discrete − 1.
 * Draws a snapshot of the color map inside the rectangle.
 **/
void
gog_axis_color_map_to_cairo (GogAxisColorMap const *map, cairo_t *cr,
                             unsigned discrete, gboolean horizontal,
                             double width, double height)
{
	unsigned i, max;

	g_return_if_fail (GOG_IS_AXIS_COLOR_MAP (map));
	max = gog_axis_color_map_get_max (map);
	if (discrete) {
		GOColor color;
		double t0, maxt, step, start, scale = 1;
		if (discrete > 1) {
			if (discrete > max + 1) { /* we need to have at least two colors */
				scale = (double) gog_axis_color_map_get_max (map) / (discrete - 2);
				max = discrete - 2;
			}
		}
		max++;
		if (horizontal) {
			maxt = width;
			step = maxt / max;
			start = 0;
		} else {
			maxt = height;
			step = -maxt / max;
			start = height;
		}
		for (i = 0; i < max; i++) {
			t0 = start + i * step;
			color = gog_axis_color_map_get_color (map, i * scale);
			if (horizontal)
				cairo_rectangle (cr, t0, 0., step, height);
			else
				cairo_rectangle (cr, 0., t0, width, step);
			cairo_set_source_rgba (cr, GO_COLOR_TO_CAIRO (color));
			cairo_fill (cr);
		}
	} else {
		cairo_pattern_t *pattern;
		pattern = (horizontal)?
					cairo_pattern_create_linear (0., 0., width, 0.):
					cairo_pattern_create_linear (0., height, 0., 0.);
		for (i = 0; i < map->size; i++) {
			cairo_pattern_add_color_stop_rgba (pattern,
			                                   (double) map->limits[i] / (double) max,
			                                   GO_COLOR_TO_CAIRO (map->colors[i]));
		}
		cairo_rectangle (cr, 0., 0., width, height);
		cairo_set_source (cr, pattern);
		cairo_fill (cr);
		cairo_pattern_destroy (pattern);
	}
}

static void
gog_axis_color_map_set_name (GogAxisColorMap *map, char const *name)
{
	g_free (map->name);
	g_hash_table_remove_all (map->names);
	map->name = g_strdup (name);
	g_hash_table_insert (map->names, g_strdup ("C"), g_strdup (name));
}

/**
 * gog_axis_color_map_dup:
 * @map: a #GogAxisColorMap
 *
 * Duplicates the color map.
 * Returns: (transfer full): the new color map.
 **/
GogAxisColorMap *
gog_axis_color_map_dup (GogAxisColorMap const *map)
{
	unsigned i;
	GogAxisColorMap *new_map = g_object_new (GOG_TYPE_AXIS_COLOR_MAP,
	                                         "resource-type", GO_RESOURCE_RW,
	                                         NULL);
	gog_axis_color_map_set_name (new_map, _("New map"));
	new_map->id = go_uuid ();
	build_uri (new_map);
	new_map->size = new_map->allocated = map->size;
	new_map->limits = g_new (unsigned, map->size);
	new_map->colors = g_new (GOColor, map->size);
	for (i = 0; i < map->size; i++) {
		new_map->limits[i] = map->limits[i];
		new_map->colors[i] = map->colors[i];
	}
	return new_map;
}

static GSList *color_maps;

/**
 * gog_axis_color_map_registry_add:
 * @map: a #GogAxisColorMap
 *
 * Keep a pointer to @map in graph color maps registry.
 * This function does not add a reference to @map.
 **/
static void
gog_axis_color_map_registry_add (GogAxisColorMap *map)
{
	g_return_if_fail (GOG_IS_AXIS_COLOR_MAP (map));

	/* TODO: Check for duplicated names and for already
	 * registered map */

	color_maps = g_slist_append (color_maps, map);
}

#ifdef GOFFICE_WITH_GTK

/**
 * gog_axis_color_map_compare:
 *
 * Returns: %TRUE if the maps are different.
 **/
static gboolean
gog_axis_color_map_compare (GogAxisColorMap const *map1, GogAxisColorMap const *map2)
{
	unsigned i;
	if (strcmp (map1->name, map2->name))
		return TRUE;
	if (map1->size != map2->size)
		return TRUE;
	for (i = 0; i < map1->size; i++)
		if (map1->limits[i] != map2->limits[i] || map1->colors[i] != map2->colors[i])
		return TRUE;
	return FALSE;
}

struct color_map_edit_state {
	GtkWidget *color_selector, *discrete, *continuous;
	GtkBuilder *gui;
	GogAxisColorMap *map, *orig;
	unsigned bin;
};

static void
update_snapshots (struct color_map_edit_state *state)
{
	GdkPixbuf *pixbuf;
	pixbuf = gog_axis_color_map_get_snapshot (state->map, TRUE, TRUE, 200, 24);
	gtk_image_set_from_pixbuf (GTK_IMAGE (state->discrete), pixbuf);
	gtk_widget_show (state->discrete);
	g_object_unref (pixbuf);
	pixbuf = gog_axis_color_map_get_snapshot (state->map, FALSE, TRUE, 200, 24);
	gtk_image_set_from_pixbuf (GTK_IMAGE (state->continuous), pixbuf);
	g_object_unref (pixbuf);
	gtk_widget_set_sensitive (go_gtk_builder_get_widget (state->gui, "save"),
	                          gog_axis_color_map_compare (state->map, state->orig));
}

static void
erase_cb (struct color_map_edit_state *state)
{
	unsigned i;
	for (i = 0; i < state->map->size && state->bin > state->map->limits[i]; i++);
	state->map->size--;
	memmove (state->map->limits + i, state->map->limits + i + 1,
	         sizeof (unsigned) * (state->map->size - i));
	memmove (state->map->colors + i, state->map->colors + i + 1,
	         sizeof (GOColor) * (state->map->size - i));
	gtk_widget_set_sensitive (go_gtk_builder_get_widget (state->gui, "erase"),
	                          FALSE);
	go_color_selector_set_color (GO_SELECTOR (state->color_selector),
	                             state->map->colors[state->map->size - 1]);
	/* update the snapshots */
	update_snapshots (state);
}

static void
define_cb (struct color_map_edit_state *state)
{
	unsigned i;
	for (i = 0; i < state->map->size && state->bin > state->map->limits[i]; i++);
	if (i < state->map->size && state->bin == state->map->limits[i]) {
		state->map->colors[i] = go_color_selector_get_color (GO_SELECTOR (state->color_selector), NULL);
	} else {
		/* we need to insert the new color stop */
		state->map->size++;
		if (state->map->allocated < state->map->size) {
			state->map->limits = g_renew (unsigned, state->map->limits,
			                              state->map->size);
			state->map->colors = g_renew (GOColor, state->map->colors,
				                          state->map->size);
			state->map->allocated = state->map->size;
		}
		if (i < state->map->size - 1) {
			memmove (state->map->limits + i + 1, state->map->limits + i,
			         sizeof (unsigned) * (state->map->size - i - 1));
			memmove (state->map->colors + i + 1, state->map->colors + i,
			         sizeof (GOColor) * (state->map->size - i - 1));
		}
		state->map->limits[i] = state->bin;
		state->map->colors[i] = go_color_selector_get_color (GO_SELECTOR (state->color_selector), NULL);
		gtk_widget_set_sensitive (go_gtk_builder_get_widget (state->gui, "erase"),
		                          TRUE);
	}
	/* update the snapshots */
	update_snapshots (state);
}

static void
bin_changed_cb (GtkSpinButton *btn, struct color_map_edit_state *state)
{
	unsigned i;
	state->bin = gtk_spin_button_get_value (btn);
	for (i = 0; i < state->map->size && state->bin > state->map->limits[i]; i++);
	if (i < state->map->size) {
		gtk_widget_set_sensitive (go_gtk_builder_get_widget (state->gui, "erase"),
		                           i > 0 && state->bin == state->map->limits[i]);
		go_color_selector_set_color (GO_SELECTOR (state->color_selector),
		                             gog_axis_color_map_get_color (state->map, i));
	} else {
		gtk_widget_set_sensitive (go_gtk_builder_get_widget (state->gui, "erase"),
		                          FALSE);
		go_color_selector_set_color (GO_SELECTOR (state->color_selector),
		                             state->map->colors[state->map->size - 1]);
	}
}

/**
 * gog_axis_color_map_edit:
 * @map: a #GogAxisColorMap or %NULL
 * @cc: a #GOCmdContext or %NULL
 *
 * Opens a dialog to edit the color map. If @map is %NULL, creates a new one
 * unless the user cancels the edition.
 * Returns: (transfer none): the edited color map.
 **/
GogAxisColorMap *
gog_axis_color_map_edit (GogAxisColorMap *map, GOCmdContext *cc)
{
	GtkBuilder *gui = go_gtk_builder_load_internal ("res:go:graph/gog-axis-color-map-prefs.ui", GETTEXT_PACKAGE, cc);
	GtkWidget *top_level = go_gtk_builder_get_widget (gui, "gog-axis-color-map-prefs"), *w;
	unsigned response;
	GdkPixbuf *pixbuf;
	GtkGrid *grid = GTK_GRID (gtk_builder_get_object (gui, "grid"));
	struct color_map_edit_state state;

	if (map == NULL) {
		GOColor color = 0; /* full transparent */
		state.map = gog_axis_color_map_from_colors ("New map", 1, &color, GO_RESOURCE_RW);
		state.map->id = go_uuid ();
		build_uri (state.map);
		state.orig = gog_axis_color_map_dup (state.map);
	} else {
		state.orig = map;
		state.map = gog_axis_color_map_from_colors (map->name, map->size,
		                                            map->colors, map->type);
	}

	state.gui = gui;
	state.bin = 0;
	gtk_adjustment_set_upper (GTK_ADJUSTMENT (gtk_builder_get_object (gui, "stop-adj")), UINT_MAX);
	pixbuf = gog_axis_color_map_get_snapshot (state.map, TRUE, TRUE, 200, 24);
	state.discrete = gtk_image_new_from_pixbuf (pixbuf);
	g_object_unref (pixbuf);
	gtk_grid_attach (grid, state.discrete, 1, 5, 3, 1);
	pixbuf = gog_axis_color_map_get_snapshot (state.map, FALSE, TRUE, 200, 24);
	state.continuous = gtk_image_new_from_pixbuf (pixbuf);
	g_object_unref (pixbuf);
	gtk_grid_attach (grid, state.continuous, 1, 6, 3, 1);
	w = go_gtk_builder_get_widget (gui, "erase");
	gtk_widget_set_sensitive (w, FALSE);
	state.color_selector = go_selector_new_color (state.map->colors[0], state.map->colors[0], "fill-color");
	gtk_grid_attach (grid, state.color_selector, 3, 2, 1, 1);
	gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (gui, "name")), state.map->name);
	gtk_widget_set_sensitive (go_gtk_builder_get_widget (gui, "save"), FALSE);
	gtk_widget_show_all (GTK_WIDGET (grid));

	/* set some signals */
	g_signal_connect_swapped (w, "clicked", G_CALLBACK (erase_cb), &state);
	g_signal_connect_swapped (gtk_builder_get_object (gui, "define"), "clicked",
	                  G_CALLBACK (define_cb), &state);
	g_signal_connect (gtk_builder_get_object (gui, "stop-btn"), "value-changed",
	                  G_CALLBACK (bin_changed_cb), &state);
	response = gtk_dialog_run (GTK_DIALOG (top_level));
	if (map == NULL)
		g_object_unref (state.orig);
	if (response == 1) {
		if (!map)
			map = state.map;
		else {
			/* copy the colors to map */
			unsigned i;
			map->size = state.map->size;
			if (map->size > map->allocated) {
				map->limits = g_new (unsigned, map->size);
				map->colors = g_new (GOColor, map->size);
				map->allocated = map->size;
			}
			for (i = 0; i < map->size; i++) {
				map->limits[i] = state.map->limits[i];
				map->colors[i] = state.map->colors[i];
			}
			g_object_unref (state.map);
		}
		gog_axis_color_map_set_name (map, gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (gui, "name"))));
		gog_axis_color_map_save (map);
		gog_axis_color_map_registry_add (map);
	} else {
		if (map == NULL)
			g_object_unref (state.map);
		map = NULL;
	}
	gtk_widget_destroy (top_level);
	g_object_unref (gui);
	return map;
}
#endif

/**
 * gog_axis_color_map_from_colors:
 * @name: color map name
 * @nb: colors number
 * @colors: the colors
 * @type: the resource type
 *
 * Creates a color map using @colors.
 * Returns: (transfer full): the newly created color map.
 **/
GogAxisColorMap *
gog_axis_color_map_from_colors (char const *name, unsigned nb,
                                GOColor const *colors, GoResourceType type)
{
	unsigned i;
	GogAxisColorMap *color_map = g_object_new (GOG_TYPE_AXIS_COLOR_MAP, NULL);
	color_map->id = g_strdup (name);
	gog_axis_color_map_set_name (color_map, name);
	color_map->type = type;
	color_map->size = color_map->allocated = nb;
	color_map->limits = g_new (unsigned, nb);
	color_map->colors = g_new (GOColor, nb);
	for (i = 0; i < nb; i++) {
		color_map->limits[i] = i;
		color_map->colors[i] = colors[i];
	}
	return color_map;
}

static GogAxisColorMap *color_map = NULL;

GogAxisColorMap const *
_gog_axis_color_map_get_default ()
{
	return color_map;
}

static void
color_map_load_from_uri (char const *uri)
{
	struct color_map_load_state state;
	GsfInput *input = go_file_open (uri, NULL);

	if (input == NULL) {
		g_warning ("[GogAxisColorMap]: Could not open %s", uri);
		return;
	}
	state.map = NULL;
	state.name = NULL;
	state.lang = NULL;
	state.langs = g_get_language_names ();
	state.name_lang_score = G_MAXINT;
	state.color_stops = NULL;
	if (!xml)
		xml = gsf_xml_in_doc_new (color_map_dtd, NULL);
	if (!gsf_xml_in_doc_parse (xml, input, &state))
		g_warning ("[GogAxisColorMap]: Could not parse %s", uri);
	if (state.map != NULL) {
		if (!go_file_access (uri, GO_W_OK)) {
			state.map->uri = g_strdup (uri);
			state.map->type = GO_RESOURCE_RW;
		} else
			state.map->type = GO_RESOURCE_RO;
		color_map_loaded (&state, uri, TRUE);
		if (state.map)
			gog_axis_color_map_registry_add (state.map);
	} else
		g_free (state.name);
	g_object_unref (input);
}

static void
color_maps_load_from_dir (char const *path)
{
	GDir *dir = g_dir_open (path, 0, NULL);
	char const *d_name;

	if (dir == NULL)
		return;
	while ((d_name = g_dir_read_name (dir)) != NULL) {
		char *fullname = g_build_filename (path, d_name, NULL);
		char *uri = go_filename_to_uri (fullname);
		size_t n = strlen (uri);
		 /* checking for mime type does not look safe, so we check for the
		 extension. TRhings might fail later if the file does not describe a
		 valid color map */
		if (n >= 4 && !strcmp (uri + n - 4, ".map"))
			color_map_load_from_uri (uri);
		g_free (uri);
		g_free (fullname);
	}
	g_dir_close (dir);
}

/**
 * GogAxisColorMapHandler:
 * @map: a #GogAxisColorMap
 * @user_data: user data
 *
 * Type of the callback to pass to gog_axis_color_map_foreach()
 * to iterate through color maps.
 **/

/**
 * gog_axis_color_map_foreach:
 * @handler: (scope call): a #GogAxisColorMapHandler
 * @user_data: data to pass to @handler
 *
 * Executes @handler to each color map installed on the system or loaded from
 * a document.
 **/
void
gog_axis_color_map_foreach (GogAxisColorMapHandler handler, gpointer user_data)
{
	GSList *ptr;
	for (ptr = color_maps; ptr; ptr = ptr->next)
		handler ((GogAxisColorMap *) (ptr->data), user_data);
}

/**
 * gog_axis_color_map_get_from_id:
 * @id: the color map identifier to search for
 *
 * Retrieves the color map whose identifier is @id.
 * Returns: (transfer none): the found color map.
 **/
GogAxisColorMap const *
gog_axis_color_map_get_from_id (char const *id)
{
	GSList *ptr;
	GogAxisColorMap *map;
	for (ptr = color_maps; ptr; ptr = ptr->next)
		if (!strcmp (((GogAxisColorMap *) (ptr->data))->id, id))
		    return (GogAxisColorMap *) ptr->data;
	/* create an empty new one */
	map = g_object_new (GOG_TYPE_AXIS_COLOR_MAP, "resource-type", GO_RESOURCE_EXTERNAL, NULL);
	map->id = g_strdup (id);
	gog_axis_color_map_registry_add (map);
	return map;
}

/**
 * gog_axis_color_map_delete:
 * @map: a #GogAxisColorMap
 *
 * Destroys the color map and remove it from the user directory and from the
 * database.
 * Returns: %TRUE on success.
 **/
gboolean
gog_axis_color_map_delete (GogAxisColorMap *map)
{
	GFile *file = g_file_new_for_uri (map->uri);
	gboolean res;
	if ((res = g_file_delete (file, NULL, NULL))) {
		color_maps = g_slist_remove (color_maps, map);
		g_object_unref (map);
	}
	g_object_unref (file);
	return res;
}

void
_gog_axis_color_maps_init (void)
{
	char *path;

	/* Default color map */
	color_map = g_object_new (GOG_TYPE_AXIS_COLOR_MAP, "resource-type", GO_RESOURCE_NATIVE, NULL);
	color_map->id = g_strdup ("Default");
	color_map->name = g_strdup (N_("Default"));
	color_map->size = color_map->allocated = 5;
	color_map->limits = g_new (unsigned, 5);
	color_map->colors = g_new (GOColor, 5);
	color_map->limits[0] = 0;
	color_map->colors[0] = GO_COLOR_BLUE;
	color_map->limits[1] = 1;
	color_map->colors[1] = GO_COLOR_FROM_RGB (0, 0xff, 0xff);
	color_map->limits[2] = 2;
	color_map->colors[2] = GO_COLOR_GREEN;
	color_map->limits[3] = 4;
	color_map->colors[3] = GO_COLOR_YELLOW;
	color_map->limits[4] = 6;
	color_map->colors[4] = GO_COLOR_RED;

	/* Now load registered color maps */
	path = g_build_filename (go_sys_data_dir (), "colormaps", NULL);
	color_maps_load_from_dir (path);
	g_free (path);

	path = g_build_filename (g_get_home_dir (), ".goffice", "colormaps", NULL);
	color_maps_load_from_dir (path);
	g_free (path);
}

void
_gog_axis_color_maps_shutdown (void)
{
	g_object_unref (color_map);
	g_slist_free_full (color_maps, g_object_unref);
	if (xml)
		gsf_xml_in_doc_free (xml);
}
