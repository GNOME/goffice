/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * goc-line.h :
 *
 * Copyright (C) 2009 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GOC_LINE_H
#define GOC_LINE_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

struct _GocLine {
	GocStyledItem base;

	/* using these to avoid confusion with x0 and others in GocItem */
	double startx, starty, endx, endy;

	GOArrow start_arrow, end_arrow;
};

typedef GocStyledItemClass GocLineClass;

#define GOC_TYPE_LINE	(goc_line_get_type ())
#define GOC_LINE(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOC_TYPE_LINE, GocLine))
#define GOC_IS_LINE(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOC_TYPE_LINE))

GType goc_line_get_type (void);

G_END_DECLS

#endif  /* GOC_LINE_H */
