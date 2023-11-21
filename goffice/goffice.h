/*
 * goffice.h :
 *
 * Copyright (C) 2003-2005 Jody Goldberg (jody@gnome.org)
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
#ifndef GOFFICE_H
#define GOFFICE_H

#include <glib.h>
#include <glib-object.h>
#include <gsf/gsf.h>
#include <goffice/goffice-features.h>

#ifdef GOFFICE_WITH_DECIMAL64
#ifdef __GI_SCANNER__
// Wrong, of course, but the introspection scanner does not know _Decimal64
typedef double _Decimal64;
#endif
#endif

#ifdef GOFFICE_WITH_GTK
#include <gtk/gtk.h>
#endif
#include <cairo/cairo.h>
#include <pango/pango.h>

#ifndef GO_VAR_DECL
#  ifdef WIN32
#    ifdef GOFFICE_COMPILATION
#      define GO_VAR_DECL extern __declspec(dllexport)
#    else
#      define GO_VAR_DECL extern __declspec(dllimport)
#    endif
#  else
#    define GO_VAR_DECL extern
#  endif
#endif /* GO_VAR_DECL */

#ifdef G_DEPRECATED_FOR
#define GOFFICE_DEPRECATED_FOR(f) G_DEPRECATED_FOR(f)
#else
#define GOFFICE_DEPRECATED_FOR(f)
#endif

#include <goffice/app/goffice-app.h>

G_BEGIN_DECLS

void libgoffice_init     (void);
void libgoffice_shutdown (void);

G_END_DECLS

#endif /* GOFFICE_H */
