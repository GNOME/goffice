/*
 * gog-graph-impl.h : the top level container for charts
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

#ifndef GOG_GRAPH_IMPL_H
#define GOG_GRAPH_IMPL_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

struct _GogGraph {
	GogOutlinedObject	 base;

	GogTheme *theme;
	GSList	 *charts;
	GSList	 *data;

	unsigned int num_cols, num_rows;
	double	  width, height;

	guint	  idle_handler;

	GHashTable *data_refs;

	GODoc *doc;
};

typedef struct {
	GogOutlinedObjectClass base;

	/* signals */
	void (*add_data)    (GogGraph *graph, GOData *input);
	void (*remove_data) (GogGraph *graph, GOData *input);
} GogGraphClass;

#define GOG_GRAPH_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST ((k), GOG_TYPE_GRAPH, GogGraphClass))
#define GOG_IS_GRAPH_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GOG_TYPE_GRAPH))

/* protected */
gboolean gog_graph_request_update (GogGraph *graph);
void     gog_graph_force_update   (GogGraph *graph);

G_END_DECLS

#endif /* GOG_GRAPH_GROUP_IMPL_H */
