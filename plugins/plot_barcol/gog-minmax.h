/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-minmax.h
 *
 * Copyright (C) 2005
 *	Jean Brefort (jean.brefort@normalesup.org)
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#ifndef GOG_MINMAX_H
#define GOG_MINMAX_H

#include "gog-1.5d.h"

G_BEGIN_DECLS


typedef struct {
	GogPlot1_5d	base;

	gboolean horizontal;
	int	 gap_percentage;
	gboolean default_style_has_markers;

} GogMinMaxPlot;
typedef GogPlot1_5dClass	GogMinMaxPlotClass;

#define GOG_MINMAX_PLOT_TYPE	(gog_minmax_plot_get_type ())
#define GOG_MINMAX_PLOT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_MINMAX_PLOT_TYPE, GogMinMaxPlot))
#define GOG_IS_PLOT_MINMAX(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_MINMAX_PLOT_TYPE))

GType gog_minmax_plot_get_type (void);

G_END_DECLS

#endif	/* GOG_MINMAX_H */
