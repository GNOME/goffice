/*
 * go-svg.h - SVG image support
 *
 * Copyright (C) 2011 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GO_SVG_H
#define GO_SVG_H
#ifdef GOFFICE_WITH_LIBRSVG

#include <goffice/goffice.h>

G_BEGIN_DECLS

#define GO_TYPE_SVG	(go_svg_get_type ())
#define GO_SVG(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_SVG, GOSvg))
#define GO_IS_SVG(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_SVG))

GType go_svg_get_type (void);

GOImage *go_svg_new_from_file (char const *filename, GError **error);
GOImage *go_svg_new_from_data (char const *data, size_t length, GError **error);

G_END_DECLS

#endif /* GOFFICE_WITH_LIBRSVG */
#endif /* GO_SVG_H */
