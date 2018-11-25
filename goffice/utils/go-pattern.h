/*
 * go-pattern.h :
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
#ifndef GO_PATTERN_H
#define GO_PATTERN_H

#include <goffice/goffice.h>
#include <libxml/tree.h>
#include <cairo.h>

G_BEGIN_DECLS

struct _GOPattern {
	GOColor	 fore, back;
	unsigned int pattern;
};

/* Useful for themes to explicitly name the pattern */
typedef enum {
	GO_PATTERN_SOLID,
	GO_PATTERN_GREY75,
	GO_PATTERN_GREY50,
	GO_PATTERN_GREY25,
	GO_PATTERN_GREY125,
	GO_PATTERN_GREY625,
	GO_PATTERN_HORIZ,
	GO_PATTERN_VERT,
	GO_PATTERN_REV_DIAG,
	GO_PATTERN_DIAG,
	GO_PATTERN_DIAG_CROSS,
	GO_PATTERN_THICK_DIAG_CROSS,
	GO_PATTERN_THIN_HORIZ,
	GO_PATTERN_THIN_VERT,
	GO_PATTERN_THIN_REV_DIAG,
	GO_PATTERN_THIN_DIAG,
	GO_PATTERN_THIN_HORIZ_CROSS,
	GO_PATTERN_THIN_DIAG_CROSS,
	GO_PATTERN_FOREGROUND_SOLID,
	GO_PATTERN_SMALL_CIRCLES,
	GO_PATTERN_SEMI_CIRCLES,
	GO_PATTERN_THATCH,
	GO_PATTERN_LARGE_CIRCLES,
	GO_PATTERN_BRICKS,
	GO_PATTERN_MAX
} GOPatternType;

GOPatternType	 go_pattern_from_str     (char const *name);
char const	*go_pattern_as_str       (GOPatternType pattern);
gboolean	 go_pattern_is_solid     (GOPattern const *pat, GOColor *color);
void		 go_pattern_set_solid    (GOPattern *pat, GOColor fore);
#ifndef GOFFICE_DISABLE_DEPRECATED
GOFFICE_DEPRECATED_FOR(go_pattern_create_cairo_pattern)
guint8 const 	*go_pattern_get_pattern  (GOPattern const *pat);
GOFFICE_DEPRECATED_FOR(go_pattern_create_cairo_pattern)
xmlChar		*go_pattern_get_svg_path (GOPattern const *pattern, double *width, double *height);
#endif

cairo_pattern_t *go_pattern_create_cairo_pattern (GOPattern const *pattern, cairo_t *cr);

G_END_DECLS

#endif /* GO_PATTERN_H */
