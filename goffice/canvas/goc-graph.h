/*
 * goc-graph.h :
 *
 * Copyright (C) 2009 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GOC_GRAPH_H
#define GOC_GRAPH_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

struct _GocGraph {
	GocItem base;

	double x, y, w, h;
	GogRenderer *renderer;

	struct {
		guint timer_id;
		double x, y;
	} coords;
	gpointer priv;
};

typedef GocItemClass GocGraphClass;

#define GOC_TYPE_GRAPH	(goc_graph_get_type ())
#define GOC_GRAPH(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOC_TYPE_GRAPH, GocGraph))
#define GOC_IS_GRAPH(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOC_TYPE_GRAPH))

GType goc_graph_get_type (void);

G_END_DECLS

#endif  /* GOC_GRAPH_H */
