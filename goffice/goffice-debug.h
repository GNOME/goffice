/*
 * goffice-debug.h:
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

#ifndef GOFFICE_DEBUG_H
#define GOFFICE_DEBUG_H

/*
 * Use "#define PLUGIN_DEBUG x" to enable some plugin related debugging
 * messages.
#undef PLUGIN_DEBUG
 * Define PLUGIN_ALWAYS_LOAD to disable loading on demand feature
 */

/* #define NO_DEBUG_CHARTS */
#ifndef NO_DEBUG_CHARTS
#define gog_debug(level, code)	do { if (goffice_graph_debug_level > level) { code } } while (0)
#else
#define gog_debug(level, code)
#endif
extern int goffice_graph_debug_level;

#endif
