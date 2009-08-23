/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-theme.c :
 *
 * Copyright (C) 2003-2004 Jody Goldberg (jody@gnome.org)
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
#include <goffice/graph/gog-theme.h>
#include <goffice/graph/gog-object.h>
#include <goffice/utils/go-color.h>
#include <goffice/utils/go-gradient.h>
#include <goffice/utils/go-units.h>
#include <goffice/utils/go-marker.h>

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>
#include <string.h>

typedef void (*GogThemeStyleMap) (GOStyle *style, unsigned ind);

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
	char 		*load_from_file; /* optionally NULL */
	GHashTable	*elem_hash_by_role;
	GHashTable	*elem_hash_by_class;
	GHashTable	*class_aliases;
	GOStyle	*default_style;
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

	g_free (theme->name); theme->name = NULL;
	g_free (theme->load_from_file); theme->load_from_file = NULL;
	if (theme->elem_hash_by_role)
		g_hash_table_destroy (theme->elem_hash_by_role);
	if (theme->elem_hash_by_class)
		g_hash_table_destroy (theme->elem_hash_by_class);
	if (theme->class_aliases)
		g_hash_table_destroy (theme->class_aliases);

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

	if (theme->load_from_file != NULL) {
		/* FIXME: Parse some XML */
		g_warning ("[GogTheme] Theme from XML file not implemented");
	}

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

	return elem;
}

/**
 * gog_theme_fillin_style :
 * @theme: #GogTheme
 * @style: #GOStyle to initialize
 * @obj: #GogObject The object associated with @style
 * @ind: an optional index
 * @complete_overwrite: boolean
 *
 * Fill in the auto aspects of @style based on @theme 's element for objects of
 * type/role similar to @obj with index @ind.  If @complete_overwrite is used,
 * fillin the entire style, not just the auto portions.
 **/
void
gog_theme_fillin_style (GogTheme const *theme, 
			GOStyle *style,
			GogObject const *obj, 
			int ind, 
			gboolean complete_overwrite)
{
	GogThemeElement *elem = gog_theme_find_element (theme, obj);

	g_return_if_fail (elem != NULL);

	if (complete_overwrite)
		go_style_assign (style, elem->style);
	else
		go_style_apply_theme (style, elem->style);

/* FIXME FIXME FIXME we should handle the applicability here not in the map */
	if (ind >= 0 && elem->map)
		(elem->map) (style, ind);
}

GogTheme *
gog_theme_new (char const *name)
{
	GogTheme *theme = g_object_new (GOG_TYPE_THEME, NULL);
	theme->name = g_strdup (name);
	return theme;
}

GogTheme *
gog_theme_new_from_file (char const *name, char const *file)
{
	GogTheme *theme = gog_theme_new (name);
	theme->load_from_file = g_strdup (file);
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
	if (role_id != NULL)
		g_hash_table_insert (theme->elem_hash_by_role,
			(gpointer)elem, elem);
	else if (klass_name != NULL)
		g_hash_table_insert (theme->elem_hash_by_class,
			(gpointer)klass_name, elem);
	else {
		if (theme->default_style)
			g_object_unref (theme->default_style);
		theme->default_style = style;
		g_free (elem);
	}
}

char const *
gog_theme_get_name (GogTheme const *theme)
{
	g_return_val_if_fail (GOG_IS_THEME (theme), "");
	return theme->name;
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
map_area_series_solid_default (GOStyle *style, unsigned ind)
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
map_area_series_solid_guppi (GOStyle *style, unsigned ind)
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

/**************************************************************************/

/**
 * gog_theme_registry_add:
 * @theme: a #GogTheme
 * @is_default : bool
 *
 * Keep a pointer to @theme in graph theme registry. 
 * This function does not add a reference to @theme.
 **/ 
void
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

void
_gog_themes_init (void)
{
	GogTheme *theme;
	GOStyle *style;

	if (NULL == global_class_aliases) {
		global_class_aliases = g_hash_table_new (
			g_str_hash, g_str_equal);
		g_hash_table_insert (global_class_aliases,
			(gpointer)"GogSeriesElement", (gpointer)"GogSeries");
	}

	/* An MS Excel-ish theme */
	theme = gog_theme_new (N_("Default"));
	gog_theme_registry_add (theme, TRUE);

	/* graph */
	style = go_style_new ();
	style->outline.dash_type = GO_LINE_NONE;
	style->outline.width = 0;
	style->outline.color = RGBA_BLACK;
	style->fill.type = GO_STYLE_FILL_NONE;
	go_pattern_set_solid (&style->fill.pattern, RGBA_WHITE);
	gog_theme_add_element (theme, style, NULL, "GogGraph", NULL);

	/* chart */
	style = go_style_new ();
	style->outline.dash_type = GO_LINE_SOLID;
	style->outline.width = 0; /* hairline */
	style->outline.color = RGBA_BLACK;
	style->fill.type = GO_STYLE_FILL_PATTERN;
	go_pattern_set_solid (&style->fill.pattern, RGBA_WHITE);
	gog_theme_add_element (theme, style, NULL, "GogChart", NULL);

	/* Legend */
	style = go_style_new ();
	style->outline.dash_type = GO_LINE_SOLID;
	style->outline.width = 0; /* hairline */
	style->outline.color = RGBA_BLACK;
	style->fill.type = GO_STYLE_FILL_PATTERN;
	go_pattern_set_solid (&style->fill.pattern, RGBA_WHITE);
	gog_theme_add_element (theme, style, NULL, "GogLegend", NULL);

	/* Axis */
	style = go_style_new ();
	style->line.width = 0; /* hairline */
	style->line.color = RGBA_BLACK;
	gog_theme_add_element (theme, style, NULL, "GogAxis", NULL);

	/* AxisLine */
	style = go_style_new ();
	style->line.width = 0; /* hairline */
	style->line.color = RGBA_BLACK;
	gog_theme_add_element (theme, style, NULL, "GogAxisLine", NULL);

	/* Grid */
	style = go_style_new ();
	style->fill.type  = GO_STYLE_FILL_PATTERN;
	style->outline.dash_type = GO_LINE_SOLID;
	style->outline.width = 1.;
	style->outline.color = 0X848284ff;
	go_pattern_set_solid (&style->fill.pattern, RGBA_GREY (0xd0));
	gog_theme_add_element (theme, style, NULL, "GogGrid", NULL);

	/* GridLine */
	style = go_style_new ();
	style->outline.dash_type = GO_LINE_SOLID;
	style->line.width = 0.4; 
	style->line.color = RGBA_BLACK;
	go_pattern_set_solid (&style->fill.pattern, RGBA_TO_UINT (0xE0, 0xE0, 0xE0, 0xE0));
	style->fill.type = GO_STYLE_FILL_NONE;
	gog_theme_add_element (theme, style, NULL, NULL, "MajorGrid");
	style = go_style_new ();
	style->outline.dash_type = GO_LINE_SOLID;
	style->line.width = 0.2; 
	style->line.color = RGBA_BLACK; 
	go_pattern_set_solid (&style->fill.pattern, RGBA_TO_UINT (0xE0, 0xE0, 0xE0, 0xE0));
	style->fill.type = GO_STYLE_FILL_NONE;
	gog_theme_add_element (theme, style, NULL, NULL, "MinorGrid");
	
	/* Series */
	style = go_style_new ();
	style->outline.dash_type = GO_LINE_SOLID;
	style->outline.width = 0; /* hairline */
	style->outline.color = RGBA_BLACK;
	style->fill.type = GO_STYLE_FILL_PATTERN;
	/* FIXME : not really true, will want to split area from line */
	gog_theme_add_element (theme, style,
		map_area_series_solid_default, "GogSeries", NULL);

	/* Chart titles */
	style = go_style_new ();
	style->outline.dash_type = GO_LINE_NONE;
	style->outline.width = 0.;
	style->outline.color = RGBA_BLACK;
	style->fill.type = GO_STYLE_FILL_NONE;
	go_pattern_set_solid (&style->fill.pattern, RGBA_WHITE);
	go_style_set_font_desc (style, pango_font_description_from_string ("Sans Bold 12"));
	gog_theme_add_element (theme, style, NULL, "GogChart", "Title");

	/* labels */
	style = go_style_new ();
	style->outline.dash_type = GO_LINE_NONE;
	style->outline.width = 0.;
	style->outline.color = RGBA_BLACK;
	style->fill.type = GO_STYLE_FILL_NONE;
	go_pattern_set_solid (&style->fill.pattern, RGBA_WHITE);
	go_style_set_font_desc (style, pango_font_description_from_string ("Sans 10"));
	gog_theme_add_element (theme, style, NULL, "GogLabel", NULL);

	/* regression curves */
	style = go_style_new ();
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 1;
	style->line.color = RGBA_BLACK;
	go_pattern_set_solid (&style->fill.pattern, RGBA_TO_UINT (0x00, 0x00, 0x00, 0x20));
	style->fill.type = GO_STYLE_FILL_NONE;
	gog_theme_add_element (theme, style,
		NULL, "GogTrendLine", NULL);

	/* regression equations */
	style = go_style_new ();
	style->outline.dash_type = GO_LINE_SOLID;
	style->outline.width = 0; /* hairline */
	style->outline.color = RGBA_BLACK;
	style->fill.type = GO_STYLE_FILL_PATTERN;
	go_pattern_set_solid (&style->fill.pattern, RGBA_WHITE);
	gog_theme_add_element (theme, style,
		NULL, "GogRegEqn", NULL);

#ifdef GOFFICE_WITH_LASEM
	/* Equations */
	style = go_style_new ();
	style->outline.dash_type = GO_LINE_NONE;
	style->outline.width = 0; /* hairline */
	style->outline.color = RGBA_BLACK;
	style->fill.type = GO_STYLE_FILL_NONE;
	go_pattern_set_solid (&style->fill.pattern, RGBA_WHITE);
	go_style_set_font_desc (style, pango_font_description_from_string ("Sans 10"));
	gog_theme_add_element (theme, style,
		NULL, "GogEquation", NULL);
#endif

	/* series lines */
	style = go_style_new ();
	gog_theme_add_element (theme, style,
		map_area_series_solid_default, "GogSeriesLines", NULL);

/* Guppi */
	theme = gog_theme_new (N_("Guppi"));
	gog_theme_registry_add (theme, FALSE);

	/* graph */
	style = go_style_new ();
	style->outline.dash_type = GO_LINE_NONE;
	style->outline.width = 0; 
	style->outline.color = RGBA_BLACK; 
	style->fill.type = GO_STYLE_FILL_GRADIENT;
	style->fill.gradient.dir   = GO_GRADIENT_N_TO_S;
	style->fill.pattern.fore = RGBA_BLUE;
	style->fill.pattern.back = RGBA_BLACK;
	gog_theme_add_element (theme, style, NULL, "GogGraph", NULL);

	/* chart */
	style = go_style_new ();
	style->outline.dash_type = GO_LINE_SOLID;
	style->outline.width = 0; /* hairline */
	style->outline.color = RGBA_BLACK;
	style->fill.type = GO_STYLE_FILL_PATTERN;
	go_pattern_set_solid (&style->fill.pattern, RGBA_WHITE);
	gog_theme_add_element (theme, style, NULL, "GogChart", NULL);

	/* legend */
	style = go_style_new ();
	style->outline.dash_type = GO_LINE_SOLID;
	style->outline.width = 0; /* hairline */
	style->outline.color = RGBA_BLACK;
	style->fill.type = GO_STYLE_FILL_PATTERN;
	go_pattern_set_solid (&style->fill.pattern, RGBA_WHITE);
	gog_theme_add_element (theme, style, NULL, "GogLegend", NULL);

	/* Axis */
	style = go_style_new ();
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0.; /* hairline */
	style->line.color = RGBA_GREY (0x20);
	gog_theme_add_element (theme, style, NULL, "GogAxis", NULL);
	
	/* AxisLine */
	style = go_style_new ();
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0.; /* hairline */
	style->line.color = RGBA_GREY (0x20);
	gog_theme_add_element (theme, style, NULL, "GogAxisLine", NULL);

	/* Grid */
	style = go_style_new ();
	style->fill.type  = GO_STYLE_FILL_PATTERN;
	style->outline.dash_type = GO_LINE_NONE;
	style->outline.color = RGBA_BLACK;
	style->outline.width = 0;
	go_pattern_set_solid (&style->fill.pattern, RGBA_GREY (0xd0));
	gog_theme_add_element (theme, style, NULL, "GogGrid", NULL);

	/* GridLine */
	style = go_style_new ();
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0.; /* hairline */
	style->line.color = RGBA_GREY (0x96);
	go_pattern_set_solid (&style->fill.pattern, RGBA_TO_UINT (0xE0, 0xE0, 0xE0, 0xE0));
	style->fill.type = GO_STYLE_FILL_NONE;
	gog_theme_add_element (theme, style, NULL, NULL, "MajorGrid");
	style = go_style_new ();
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 0.; /* hairline */
	style->line.color = RGBA_GREY (0xC0);
	go_pattern_set_solid (&style->fill.pattern, RGBA_TO_UINT (0xE0, 0xE0, 0xE0, 0xE0));
	style->fill.type = GO_STYLE_FILL_NONE;
	gog_theme_add_element (theme, style, NULL, NULL, "MinorGrid");
	
	/* series */
	style = go_style_new ();
	style->outline.dash_type = GO_LINE_SOLID;
	style->outline.width = 0.; /* hairline */
	style->outline.color = RGBA_BLACK;
	style->fill.type = GO_STYLE_FILL_PATTERN;
	go_pattern_set_solid (&style->fill.pattern, RGBA_GREY (0x20));
	/* FIXME : not really true, will want to split area from line */
	gog_theme_add_element (theme, style,
		map_area_series_solid_guppi, "GogSeries", NULL);

	/* labels */
	style = go_style_new ();
	style->outline.dash_type = GO_LINE_NONE;
	style->outline.width = 0; /* none */
	style->outline.color = RGBA_BLACK;
	style->fill.type = GO_STYLE_FILL_NONE;
	go_pattern_set_solid (&style->fill.pattern, RGBA_WHITE);
	gog_theme_add_element (theme, style, NULL, "GogLabel", NULL);

	/* trend lines */
	style = go_style_new ();
	style->line.dash_type = GO_LINE_SOLID;
	style->line.width = 1;
	style->line.color = RGBA_BLACK;
	go_pattern_set_solid (&style->fill.pattern, RGBA_TO_UINT (0x00, 0x00, 0x00, 0x20));
	style->fill.type = GO_STYLE_FILL_NONE;
	gog_theme_add_element (theme, style,
		NULL, "GogTrendLine", NULL);

	/* regression equations */
	style = go_style_new ();
	style->outline.dash_type = GO_LINE_SOLID;
	style->outline.width = 0; /* hairline */
	style->outline.color = RGBA_BLACK;
	style->fill.type = GO_STYLE_FILL_PATTERN;
	go_pattern_set_solid (&style->fill.pattern, RGBA_WHITE);
	gog_theme_add_element (theme, style,
		NULL, "GogRegEqn", NULL);

#ifdef GOFFICE_WITH_LASEM
	/* Equations */
	style = go_style_new ();
	style->outline.dash_type = GO_LINE_NONE;
	style->outline.width = 0; /* hairline */
	style->outline.color = RGBA_BLACK;
	style->fill.type = GO_STYLE_FILL_NONE;
	go_pattern_set_solid (&style->fill.pattern, RGBA_WHITE);
	go_style_set_font_desc (style, pango_font_description_from_string ("Sans 10"));
	gog_theme_add_element (theme, style,
		NULL, "GogEquation", NULL);
#endif

	/* series lines */
	style = go_style_new ();
	gog_theme_add_element (theme, style,
		NULL, "GogSeriesLines", NULL);
}

void
_gog_themes_shutdown (void)
{
	GSList *ptr;

	if (default_theme != NULL)
		g_object_unref (default_theme);
	for (ptr = themes; ptr != NULL ; ptr = ptr->next)
		g_object_unref (ptr->data);
}
