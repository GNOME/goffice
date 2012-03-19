/*
 * gog-trend-line.h :
 *
 * Copyright (C) 2006 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GOG_TREND_LINE_H
#define GOG_TREND_LINE_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

struct _GogTrendLine {
	GogStyledObject  base;
};
typedef struct _GogTrendLine GogTrendLine;

typedef GogStyledObjectClass GogTrendLineClass;

#define GOG_TYPE_TREND_LINE	(gog_trend_line_get_type ())
#define GOG_TREND_LINE(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_TREND_LINE, GogTrendLine))
#define GOG_IS_TREND_LINE(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_TREND_LINE))

GType gog_trend_line_get_type (void);

GogTrendLine *gog_trend_line_new_by_name  (char const *id);
GogTrendLine *gog_trend_line_new_by_type  (GogTrendLineType const *type);
gboolean      gog_trend_line_has_legend (GogTrendLine const *line);

G_END_DECLS

#endif /* GOG_TREND_LINE_H */
