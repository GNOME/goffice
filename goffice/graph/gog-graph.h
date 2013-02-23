/*
 * gog-graph.h :
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
#ifndef GOG_GRAPH_H
#define GOG_GRAPH_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

#define GOG_TYPE_GRAPH	(gog_graph_get_type ())
#define GOG_GRAPH(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_GRAPH, GogGraph))
#define GOG_IS_GRAPH(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_GRAPH))

GType gog_graph_get_type (void);

gboolean  gog_graph_validate_chart_layout (GogGraph *graph);
unsigned  gog_graph_num_cols	  (GogGraph const *graph);
unsigned  gog_graph_num_rows	  (GogGraph const *graph);

/* convenience wrappers */
GogGraph *gog_graph_dup	  	  (GogGraph const *graph);
GogTheme *gog_graph_get_theme	  (GogGraph const *graph);
void	  gog_graph_set_theme	  (GogGraph *graph, GogTheme *theme);

/* data management */
GSList   *gog_graph_get_data	  (GogGraph const *graph);

/* internal routines for use by series */
GOData   *gog_graph_ref_data   	  (GogGraph *graph, GOData *dat);
void      gog_graph_unref_data    (GogGraph *graph, GOData *dat);

void	  gog_graph_get_size 	  (GogGraph *graph, double *width, double *height);
void      gog_graph_set_size      (GogGraph *graph, double width, double height);

#define GOG_TYPE_GRAPH_VIEW	(gog_graph_view_get_type ())
#define GOG_GRAPH_VIEW(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_GRAPH_VIEW, GogGraphView))
#define GOG_IS_GRAPH_VIEW(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_GRAPH_VIEW))

GType gog_graph_view_get_type (void);

GogView *gog_graph_view_get_selection 	(GogGraphView *gview);
void 	 gog_graph_view_set_selection   (GogGraphView *gview, GogObject *gobj);


GSList 	 *gog_graph_get_supported_image_formats	(void);
gboolean  gog_graph_export_image 		(GogGraph *graph, GOImageFormat format,
						 GsfOutput *output, double x_dpi, double y_dpi);

#ifdef GOFFICE_WITH_GTK
#include <goffice/gtk/goffice-gtk.h>
void  	 gog_graph_view_handle_event 	(GogGraphView *gview, GdkEvent *event, double x_offset, double y_offset);
#endif

void 	 gog_graph_render_to_cairo 	(GogGraph *graph, cairo_t *cairo, double w, double h);
GODoc	*gog_graph_get_document		(GogGraph *graph);

G_END_DECLS

#endif /* GOG_GRAPH_H */
