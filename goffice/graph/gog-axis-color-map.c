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

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#ifdef GOFFICE_WITH_GTK
#include <gtk/gtk.h>
#endif

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
	char *name, *local_name;
	GHashTable *names;
	unsigned size; /* colors number */
	unsigned *limits;
	GOColor *colors;
};
typedef GObjectClass GogAxisColorMapClass;

static GObjectClass *parent_klass;

static void
gog_axis_color_map_finalize (GObject *obj)
{
	GogAxisColorMap *map = GOG_AXIS_COLOR_MAP (obj);
	g_free (map->name);
	map->name = NULL;
	g_free (map->local_name);
	map->local_name = NULL;
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
}

static void
gog_axis_color_map_init (GogAxisColorMap *map)
{
}

GSF_CLASS (GogAxisColorMap, gog_axis_color_map,
	   gog_axis_color_map_class_init, gog_axis_color_map_init,
	   G_TYPE_OBJECT)

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
 * gog_axis_color_map_get_snapshot:
 * @map: a #GogAxisMap
 *
 * Retrieves the color map name.
 * Returns: (transfer none): the map name.
 **/
char const *
gog_axis_color_map_get_name (GogAxisColorMap const *map)
{
	g_return_val_if_fail (GOG_IS_AXIS_COLOR_MAP (map), NULL);
	return map->name;
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
	cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
	GdkPixbuf *pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, width, height);
	cairo_t *cr = cairo_create (surface);
	unsigned i, max = gog_axis_color_map_get_max (map);

	g_return_val_if_fail (GOG_IS_AXIS_COLOR_MAP (map), NULL);
	if (discrete) {
		GOColor color;
		unsigned t0 = 0, t1, maxt = horizontal? width: height;
		max++;
		for (i = 0; i < max; i++) {
			t1 = (i + 1) * maxt / max;
			color = gog_axis_color_map_get_color (map, i);
			if (horizontal)
				cairo_rectangle (cr, t0, 0., t1, height);
			else
				cairo_rectangle (cr, 0., t0, width, t1);
			cairo_set_source_rgba (cr, GO_COLOR_TO_CAIRO (color));
			cairo_fill (cr);
			t0 = t1;
		}
	} else {
		double x1, y1;
		cairo_pattern_t *pattern;
		if (horizontal) {
			x1 = width;
			y1 = 0.;
		} else {
			x1 = 0.;
			y1 = height;
		}
		pattern = cairo_pattern_create_linear (0., 0., x1, y1);
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

	go_cairo_convert_data_to_pixbuf (gdk_pixbuf_get_pixels (pixbuf),
	                                 cairo_image_surface_get_data (surface),
	                                 width, height, width * 4);
	cairo_destroy (cr);
	cairo_surface_destroy (surface);
	return pixbuf;
}

#ifdef GOFFICE_WITH_GTK
/**
 * gog_axis_color_map_edit:
 * @map: a #GogAxisColorMap or %NULL
 * @cc: a #GOCmdContext
 *
 * Opens a dialog to edit the color map. If @map is %NULL, creates a new one
 * unless the user cancels the edition.
 * Returns: (transfer none): the edited color map.
 **/
GogAxisColorMap *
gog_axis_color_map_edit (GogAxisColorMap *map, GOCmdContext *cc)
{
	GtkBuilder *gui = go_gtk_builder_load_internal ("res:go:graph/gog-axis-color-map-prefs.ui", GETTEXT_PACKAGE, cc);
	GtkWidget *top_level = go_gtk_builder_get_widget (gui, "gog-axis-color-map-prefs");
	unsigned response;
	gboolean created = FALSE;

	if (map == NULL) {
		GOColor color = GO_COLOR_BLUE;
		map = gog_axis_color_map_from_colors ("New map", 1, &color);
		created = TRUE;
	} else {
	}

	response = gtk_dialog_run (GTK_DIALOG (top_level));
	if (response == 1) {
	} else {
		if (created)
			g_object_unref (map);
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
 * @colors: the colors.
 *
 * Creates a color map using @colors.
 * Returns: (transfer full): the newly created color map.
 **/
GogAxisColorMap *
gog_axis_color_map_from_colors (char const *name, unsigned nb, GOColor const *colors)
{
	unsigned i;
	GogAxisColorMap *color_map = g_object_new (GOG_TYPE_AXIS_COLOR_MAP, NULL);
	color_map->name = g_strdup (name);
	color_map->size = nb;
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

struct _color_stop {
	unsigned bin;
	GOColor color;
};

struct color_map_load_state {
	GogAxisColorMap *map;
	char *lang, *local_name;
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
name_start (GsfXMLIn *xin, xmlChar const **attrs)
{
	struct color_map_load_state	*state = (struct color_map_load_state *) xin->user_state;
	unsigned i;
	for (i = 0; attrs != NULL && attrs[i] && attrs[i+1] ; i += 2)
		if (0 == strcmp (attrs[i], "xml:lang"))
			state->lang = g_strdup (attrs[i+1]);
}

static void
name_end (GsfXMLIn *xin, G_GNUC_UNUSED GsfXMLBlob *blob)
{
	struct color_map_load_state	*state = (struct color_map_load_state *) xin->user_state;
	char *name = NULL;
	if (xin->content->str == NULL)
		return;
	name = g_strdup (xin->content->str);
	if (state->map == NULL) {
		state->map = g_object_new (GOG_TYPE_AXIS_COLOR_MAP, NULL);
		state->map->names = g_hash_table_new_full (g_str_hash, g_str_equal,
		                                           g_free, g_free);
	}
	if (state->lang == NULL) {
		state->map->name = name;
	} else {
		if (state->name_lang_score > 0 && state->langs[0] != NULL) {
			unsigned i;
			for (i = 0; i < state->name_lang_score && state->langs[i] != NULL; i++) {
				if (strcmp (state->langs[i], state->lang) == 0) {
					g_free (state->local_name);
					state->local_name = g_strdup (name);
					state->name_lang_score = i;
				}
			}
		}
		g_hash_table_replace (state->map->names, state->lang, name); 
		state->lang = NULL;
	}
}

static int
color_stops_cmp (struct _color_stop *first, struct _color_stop *second)
{
	return (first->bin < second->bin)? -1: (int) (first->bin - second->bin);
}

static void
color_map_load_from_uri (char const *uri)
{
	static GsfXMLInNode const color_map_dtd[] = {
		GSF_XML_IN_NODE (THEME, THEME, -1, "GogAxisColorMap", GSF_XML_NO_CONTENT, NULL, NULL),
			GSF_XML_IN_NODE (THEME, NAME, -1, "name", GSF_XML_CONTENT, name_start, name_end),
			GSF_XML_IN_NODE (THEME, UNAME, -1, "_name", GSF_XML_CONTENT, name_start, name_end),
			GSF_XML_IN_NODE (THEME, STOP, -1, "color-stop", GSF_XML_CONTENT, color_stop_start, NULL),
		GSF_XML_IN_NODE_END
	};
	struct color_map_load_state	state;
	GsfXMLInDoc *xml;
	GsfInput *input = go_file_open (uri, NULL);

	if (input == NULL) {
		g_warning ("[GogAxisColorMap]: Could not open %s", uri);
		return;
	}
	state.map = NULL;
	state.lang = state.local_name = NULL;
	state.langs = g_get_language_names ();
	state.name_lang_score = G_MAXINT;
	state.color_stops = NULL;
	xml = gsf_xml_in_doc_new (color_map_dtd, NULL);
	if (!gsf_xml_in_doc_parse (xml, input, &state))
		g_warning ("[GogAxisColorMap]: Could not parse %s", uri);
	if (state.map != NULL) {
		GSList *ptr;
		state.map->local_name = state.local_name;
		/* populates the colors */
		/* first sort the color list according to bins */
		ptr = state.color_stops = g_slist_sort (state.color_stops, (GCompareFunc) color_stops_cmp);
		if (((struct _color_stop *) ptr->data)->bin != 0) {
			g_warning ("[GogAxisColorMap]: Invalid color map in %s", uri);
			g_object_unref (state.map);
		} else {
			unsigned cur_bin, n = 0;
			state.map->size = g_slist_length (state.color_stops);
			state.map->limits = g_new (unsigned, state.map->size);
			state.map->colors = g_new (GOColor, state.map->size);
			while (ptr) {
				cur_bin = state.map->limits[n] = ((struct _color_stop *) ptr->data)->bin;
				state.map->colors[n++] = ((struct _color_stop *) ptr->data)->color;
				do (ptr = ptr->next);
				while (ptr && ((struct _color_stop *) ptr->data)->bin == cur_bin);
			}
			state.map->size = n; /* we drop duplicate bins */
			gog_axis_color_map_registry_add (state.map);
		}
	} else
		g_free (state.local_name);
	g_slist_free_full (state.color_stops, g_free);
	g_free (state.lang);
	gsf_xml_in_doc_free (xml);
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
		char *mime_type = go_get_mime_type (uri);
		if (!strcmp (mime_type, "application/xml")) /* we don't have a better one */
			color_map_load_from_uri (uri);
		g_free (mime_type);
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
 * Type of the callback to pass to gog_axis_color_map_foreach() and
 * go_doc_foreach_color_map() to iterate through color maps.
 **/

/**
 * gog_axis_color_map_foreach:
 * @handler: (scope call): a #GogAxisColorMapHandler
 * @user_data: data to pass to @handler
 *
 * Executes @handler to each color map installed on the system. This function
 * should not be called directly, call go_doc_foreach_color_map() instead.
 **/
void
gog_axis_color_map_foreach (GogAxisColorMapHandler handler, gpointer user_data)
{
	GSList *ptr;
	for (ptr = color_maps; ptr; ptr = ptr->next)
		handler ((GogAxisColorMap *) (ptr->data), user_data);
}

/**
 * gog_axis_color_map_get_from_name:
 * @name: the color map name to search for
 *
 * Retrieves the color map whose name is @name.
 * Returns: (transfer none): the found color map or %NULL.
 **/
GogAxisColorMap const *
gog_axis_color_map_get_from_name (char const *name)
{
	GSList *ptr;
	for (ptr = color_maps; ptr; ptr = ptr->next)
		if (!strcmp (((GogAxisColorMap *) (ptr->data))->name, name))
		    return (GogAxisColorMap *) ptr->data;
	return NULL;
}

void
_gog_axis_color_maps_init (void)
{
	char *path;

	/* Default color map */
	color_map = g_object_new (GOG_TYPE_AXIS_COLOR_MAP, NULL);
	color_map->name = g_strdup (N_("Default"));
	color_map->size = 5;
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
}
