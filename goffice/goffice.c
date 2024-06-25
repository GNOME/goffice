/*
 * goffice.c : pull all the parts together
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
#include <goffice/goffice-priv.h>
#include <gsf/gsf-utils.h>

#include "goffice-paths.h"

#include <libintl.h>

#ifdef G_OS_WIN32
#define STRICT
/* pickup HMODULE */
#include <windows.h>
#undef STRICT
#endif

#ifdef GTKOSXAPPLICATION
#include <gtkosxapplication.h>
#endif

#ifdef GOFFICE_WITH_LASEM
#include <goffice/graph/gog-equation.h>
#endif

#include "embedded-stuff.c"

int goffice_graph_debug_level = 0;

static char const *libgoffice_data_dir   = GOFFICE_DATADIR;
static char const *libgoffice_icon_dir   = GOFFICE_ICONDIR;
static char const *libgoffice_locale_dir = GOFFICE_LOCALEDIR;
static char const *libgoffice_lib_dir    = GOFFICE_LIBDIR;
static char const *libgoffice_extern_plugin_dir = GOFFICE_EXTERNPLUGINDIR;

/**
 * SECTION: goffice
 * @Title: Gettings Started with GOffice
 *
 * GOffice is a #GObject based C library. It provides easy API access to
 * creating and manipulating graphs and canvases. See #GogGraph and #GocCanvas.
 * It also includes a number of utilities and widgets that might be useful in
 * office productivity software and similar.
 *
 * The GOffice code was originally a part of Gnumeric, but was split into a
 * separate library in 2005. Today it is being used by projects such as
 * Gnumeric, Gnucash, Abiword (optional), GChemUtils and more.
 *
 * GOffice is cross platform, with support for Windows, OSX, GNU/Linux,
 * and other Unix-like systems. It can be built with or without GTK+
 * integration support.
 *
 * To include GOffice use:
 * <informalexample>
 * <programlisting>
 * #include &lt;goffice/goffice.h&gt;
 * </programlisting>
 * </informalexample>
 * GOffice provides a pkg-config metadata file named
 * "libgoffice-$major.$minor.pc", where $major and $minor denote which version
 * GOffice is. So to link against GOffice 0.10 series, use:
 * <informalexample>
 * <programlisting>
 * pkg-config --libs libgoffice-0.10
 * </programlisting>
 * </informalexample>
 *
 * API and ABI compatability is maintained per minor release series, e.g:
 * all 0.8.x releases are ABI/API compatible with previous 0.8.x releases.
 *
 * Bindings for other languages than C and C++ are available through GObject
 * Introspection. To start using GOffice from Python:
 * <informalexample>
 * <programlisting>
 * from gi.repository import GOffice
 * GOffice.init()
 * </programlisting>
 * </informalexample>
 */

gchar const *
go_sys_data_dir (void)
{
	return libgoffice_data_dir;
}

gchar const *
go_sys_icon_dir (void)
{
	return libgoffice_icon_dir;
}

gchar const *
go_sys_lib_dir (void)
{
	return libgoffice_lib_dir;
}

gchar const *
go_sys_extern_plugin_dir (void)
{
	return libgoffice_extern_plugin_dir;
}

#ifdef GOFFICE_WITH_GTK
static void
install_icons (void)
{
	GdkPixbuf *pixbuf = go_gdk_pixbuf_load_from_file ("res:go:utils/unknown_image.png");
	int size = gdk_pixbuf_get_width (pixbuf);
	gtk_icon_theme_add_builtin_icon ("unknown_image",
					 size,
					 pixbuf);
	g_object_unref (pixbuf);
}
#endif


/**
 * libgoffice_init:
 *
 * Initialize GOffice.
 *
 * This function can be called several times; each call will
 * increment a reference counter. Code that calls this function should call
 * libgoffice_shutdown() when done to decrement the counter.
 */
static int initialized = 0;
void
libgoffice_init (void)
{
	if (initialized++)
		return;

#ifdef G_OS_WIN32
	{
	gchar *dir;

#define S(s)	#s
	char const *module_name =
		"libgoffice-" S(GO_VERSION_EPOCH) "-" S(GO_VERSION_MAJOR) ".dll";
#undef S
	wchar_t *wc_module_name = g_utf8_to_utf16 (module_name, -1, NULL, NULL, NULL);
	HMODULE hmodule = GetModuleHandleW (wc_module_name);
	g_free (wc_module_name);
	dir = g_win32_get_package_installation_directory_of_module (hmodule);

	libgoffice_data_dir = g_build_filename (dir,
		"share", "goffice", GOFFICE_VERSION, NULL);
	libgoffice_icon_dir = g_build_filename (dir,
		"share", "pixmaps", "goffice", GOFFICE_VERSION, NULL);
	libgoffice_locale_dir = g_build_filename (dir,
		"share", "locale", NULL);
	libgoffice_lib_dir = g_build_filename (dir,
		"lib", "goffice", GOFFICE_VERSION, NULL);
	g_free (dir);
	}
#else
#ifdef GTKOSXAPPLICATION
    if (quartz_application_get_bundle_id ())
    {
        gchar *dir;

        dir = quartz_application_get_resource_path ();
	libgoffice_data_dir = g_build_filename (dir,
		"share", "goffice", GOFFICE_VERSION, NULL);
	libgoffice_icon_dir = g_build_filename (dir,
		"share", "pixmaps", "goffice", GOFFICE_VERSION, NULL);
	libgoffice_locale_dir = g_build_filename (dir,
		"share", "locale", NULL);
	libgoffice_lib_dir = g_build_filename (dir,
		"lib", "goffice", GOFFICE_VERSION, NULL);
	g_free (dir);
    }
#endif /* GTKOSXAPPLICATION */
#endif

	bindtextdomain (GETTEXT_PACKAGE, libgoffice_locale_dir);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	gsf_init ();

	_go_string_init ();
	_go_conf_init ();
	_go_fonts_init ();
#ifdef GOFFICE_WITH_DECIMAL64
	_go_decimal_init ();
#endif
	_go_math_init ();
	_go_rsm_init ();
	go_register_ui_files ();

	/* keep trigger happy linkers from leaving things out */
	_go_plugin_services_init ();
	_gog_plugin_services_init ();
#ifdef GOFFICE_WITH_GTK
	_goc_plugin_services_init ();
	install_icons ();
#endif
	g_type_ensure (GO_TYPE_PLUGIN_LOADER_MODULE);
	g_type_ensure (GOG_TYPE_GRAPH);
	g_type_ensure (GOG_TYPE_CHART);
	g_type_ensure (GOG_TYPE_PLOT);
	g_type_ensure (GOG_TYPE_SERIES);
	g_type_ensure (GOG_TYPE_SERIES_ELEMENT);
	g_type_ensure (GOG_TYPE_LEGEND);
	g_type_ensure (GOG_TYPE_AXIS);
	g_type_ensure (GOG_TYPE_AXIS_LINE);
	g_type_ensure (GOG_TYPE_COLOR_SCALE);
	g_type_ensure (GOG_TYPE_LABEL);
	g_type_ensure (GOG_TYPE_GRID);
	g_type_ensure (GOG_TYPE_GRID_LINE);
#ifdef GOFFICE_WITH_LASEM
	g_type_ensure (GOG_TYPE_EQUATION);
#endif
	g_type_ensure (GOG_TYPE_ERROR_BAR);
	g_type_ensure (GOG_TYPE_REG_EQN);
	g_type_ensure (GOG_TYPE_SERIES_LABELS);
	g_type_ensure (GOG_TYPE_DATA_LABEL);
	g_type_ensure (GOG_TYPE_SERIES_LINES);
	g_type_ensure (GO_TYPE_DATA_SCALAR_VAL);
	g_type_ensure (GO_TYPE_DATA_SCALAR_STR);
	g_type_ensure (GOG_3D_BOX_TYPE);
	g_type_ensure (GO_TYPE_EMF);
	g_type_ensure (GO_TYPE_PIXBUF);
	g_type_ensure (GO_TYPE_SPECTRE);
#ifdef GOFFICE_WITH_SVG
	g_type_ensure (GO_TYPE_SVG);
#endif /* GOFFICE_WITH_SVG */

	_gog_themes_init ();
	_go_number_format_init ();
	_go_currency_date_format_init ();
	_go_distributions_init ();
	initialized = TRUE;
}

/**
 * libgoffice_shutdown:
 *
 * Decrements the counter for data initialized by libgoffice_init().
 * When the counter reaches 0, the data is freed/cleaned up as appropriate.
 */
void
libgoffice_shutdown (void)
{
	if (--initialized)
		return;
	_gog_themes_shutdown ();
	_go_glib_extras_shutdown ();
	_go_fonts_shutdown ();
#ifdef GOFFICE_WITH_DECIMAL64
	_go_decimal_shutdown ();
#endif
	_go_conf_shutdown ();
#ifdef GOFFICE_WITH_GTK
	_go_gtk_shutdown ();
	_goc_plugin_services_shutdown ();
#endif
	_gog_plugin_services_shutdown ();
	_go_currency_date_format_shutdown ();
	_go_number_format_shutdown ();
	_go_string_shutdown ();
	_go_locale_shutdown ();
	_go_rsm_shutdown ();
	_go_unit_shutdown ();
#ifdef G_OS_WIN32
	/* const_cast, we created these above */
	g_free ((char *)libgoffice_data_dir);
	g_free ((char *)libgoffice_icon_dir);
	g_free ((char *)libgoffice_locale_dir);
	g_free ((char *)libgoffice_lib_dir);
#endif
	_go_libxml_extras_shutdown ();
}
