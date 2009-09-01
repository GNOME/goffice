/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-color.h
 *
 * Copyright (C) 1999, 2000 EMC Capital Management, Inc.
 *
 * Developed by Jon Trowbridge <trow@gnu.org> and
 * Havoc Pennington <hp@pobox.com>.
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

#ifndef GO_COLOR_H
#define GO_COLOR_H

#include <goffice/goffice.h>

#ifdef GOFFICE_WITH_GTK
#include <gdk/gdk.h>
#endif

G_BEGIN_DECLS

typedef struct {
	GOColor		 color;
	char const 	*name;	/* english name - eg. "white" */
} GONamedColor;

/*
  Some convenient macros for drawing into an RGB buffer.
  Beware of side effects, code-bloat, and all of the other classic
  cpp-perils...
*/

#define GO_GDK_TO_UINT(c)	GO_RGBA_TO_UINT(((c).red>>8), ((c).green>>8), ((c).blue>>8), 0xff)

#define GO_RGB_TO_UINT(r,g,b)	((((guint)(r))<<16)|(((guint)(g))<<8)|((guint)(b)))
#define GO_RGB_TO_RGBA(x,a)	(((x) << 8) | ((((guint)a) & 0xff)))
#define GO_RGB_WHITE  GO_RGB_TO_UINT(0xff, 0xff, 0xff)
#define GO_RGB_BLACK   GO_RGB_TO_UINT(0x00, 0x00, 0x00)
#define GO_RGB_RED     GO_RGB_TO_UINT(0xff, 0x00, 0x00)
#define GO_RGB_GREEN   GO_RGB_TO_UINT(0x00, 0xff, 0x00)
#define GO_RGB_BLUE   GO_RGB_TO_UINT(0x00, 0x00, 0xff)
#define GO_RGB_YELLOW  GO_RGB_TO_UINT(0xff, 0xff, 0x00)
#define GO_RGB_VIOLET  GO_RGB_TO_UINT(0xff, 0x00, 0xff)
#define GO_RGB_CYAN    GO_RGB_TO_UINT(0x00, 0xff, 0xff)
#define GO_RGB_GREY(x) GO_RGB_TO_UINT(x,x,x)

#define GO_RGBA_TO_UINT(r,g,b,a)	((((guint)(r))<<24)|(((guint)(g))<<16)|(((guint)(b))<<8)|(guint)(a))
#define GO_RGBA_WHITE  GO_RGB_TO_RGBA(GO_RGB_WHITE, 0xff)
#define GO_RGBA_BLACK GO_RGB_TO_RGBA(GO_RGB_BLACK, 0xff)
#define GO_RGBA_RED    GO_RGB_TO_RGBA(GO_RGB_RED, 0xff)
#define GO_RGBA_GREEN  GO_RGB_TO_RGBA(GO_RGB_GREEN, 0xff)
#define GO_RGBA_BLUE   GO_RGB_TO_RGBA(GO_RGB_BLUE, 0xff)
#define GO_RGBA_YELLOW GO_RGB_TO_RGBA(GO_RGB_YELLOW, 0xff)
#define GO_RGBA_VIOLET GO_RGB_TO_RGBA(GO_RGB_VIOLET, 0xff)
#define GO_RGBA_CYAN   GO_RGB_TO_RGBA(GO_RGB_CYAN, 0xff)
#define GO_RGBA_GREY(x) GO_RGB_TO_RGBA(GO_RGB_GREY(x), 0xff)

#define GO_UINT_RGBA_R(x) (((guint)(x))>>24)
#define GO_UINT_RGBA_G(x) ((((guint)(x))>>16)&0xff)
#define GO_UINT_RGBA_B(x) ((((guint)(x))>>8)&0xff)
#define GO_UINT_RGBA_A(x) (((guint)(x))&0xff)
#define GO_UINT_RGBA_CHANGE_R(x, r) (((x)&(~(0xff<<24)))|(((r)&0xff)<<24))
#define GO_UINT_RGBA_CHANGE_G(x, g) (((x)&(~(0xff<<16)))|(((g)&0xff)<<16))
#define GO_UINT_RGBA_CHANGE_B(x, b) (((x)&(~(0xff<<8)))|(((b)&0xff)<<8))
#define GO_UINT_RGBA_CHANGE_A(x, a) (((x)&(~0xff))|((a)&0xff))
#define GO_UINT_TO_RGB(u,r,g,b) \
{ (*(r)) = ((u)>>16)&0xff; (*(g)) = ((u)>>8)&0xff; (*(b)) = (u)&0xff; }
#define GO_UINT_TO_RGBA(u,r,g,b,a) \
{GO_UINT_TO_RGB(((u)>>8),r,g,b); (*(a)) = (u)&0xff; }
#define GO_MONO_INTERPOLATE(v1, v2, t) ((gint)go_rint((v2)*(t)+(v1)*(1-(t))))
#define GO_UINT_INTERPOLATE(c1, c2, t) \
  GO_RGBA_TO_UINT( GO_MONO_INTERPOLATE(GO_UINT_RGBA_R(c1), GO_UINT_RGBA_R(c2), t), \
		GO_MONO_INTERPOLATE(GO_UINT_RGBA_G(c1), GO_UINT_RGBA_G(c2), t), \
		GO_MONO_INTERPOLATE(GO_UINT_RGBA_B(c1), GO_UINT_RGBA_B(c2), t), \
		GO_MONO_INTERPOLATE(GO_UINT_RGBA_A(c1), GO_UINT_RGBA_A(c2), t) )

#define GO_DOUBLE_RGBA_R(x) (double)GO_UINT_RGBA_R(x)/255.0
#define GO_DOUBLE_RGBA_G(x) (double)GO_UINT_RGBA_G(x)/255.0
#define GO_DOUBLE_RGBA_B(x) (double)GO_UINT_RGBA_B(x)/255.0
#define GO_DOUBLE_RGBA_A(x) (double)GO_UINT_RGBA_A(x)/255.0

#define GO_COLOR_TO_CAIRO(x) GO_DOUBLE_RGBA_R(x),GO_DOUBLE_RGBA_G(x),GO_DOUBLE_RGBA_B(x),GO_DOUBLE_RGBA_A(x)

gboolean  go_color_from_str (char const *str, GOColor *res);
gchar    *go_color_as_str   (GOColor color);
PangoAttribute *go_color_to_pango (GOColor color, gboolean is_fore);
#ifdef GOFFICE_WITH_GTK
GdkColor *go_color_to_gdk   (GOColor color, GdkColor *res);
#endif

G_END_DECLS

#endif /* GO_COLOR_H */
