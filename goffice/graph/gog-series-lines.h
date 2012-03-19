/*
 * gog-series-lines.h :
 *
 * Copyright (C) 2005 Jean Brefort (jean.brefort@normalesup.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef GOG_SERIES_LINES_H
#define GOG_SERIES_LINES_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

#define GOG_TYPE_SERIES_LINES		(gog_series_lines_get_type ())
#define GOG_SERIES_LINES(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_SERIES_LINES, GogSeriesLines))
#define GOG_IS_SERIES_LINES(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_SERIES_LINES))

GType 	gog_series_lines_get_type 	(void);
void 	gog_series_lines_stroke 	(GogSeriesLines *lines, GogRenderer *rend,
					 GogViewAllocation const *bbox, GOPath *path, gboolean invert);
void	gog_series_lines_use_markers (GogSeriesLines *lines, gboolean use_markers);

G_END_DECLS

 #endif	/* GOG_SERIES_LINES_H */
