/*
 * gog-view.h : A sized render engine for an item.
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
#ifndef GOG_VIEW_H
#define GOG_VIEW_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

/*****************************************************************************/

typedef struct _GogToolAction GogToolAction;

typedef struct {
	char const 	*name;

	/* GdkCursorType	 cursor_type; Not compatible with --without-gtk */
	int	 	cursor_type;

	gboolean 	(*point) 	(GogView *view, double x, double y, GogObject **object);
	void 		(*render)	(GogView *view);
	void 		(*init)		(GogToolAction *action);
	void 		(*move)		(GogToolAction *action, double x, double y);
	void		(*double_click)	(GogToolAction *action);
	void 		(*destroy)	(GogToolAction *action);
} GogTool;

struct _GogToolAction {
	double 		 start_x, start_y;
	GogView 	*view;
	GogTool 	*tool;
	gpointer	 data;
	/* private */
	unsigned int ref_count;
};

GType        gog_tool_action_get_type   (void);
GogToolAction 	*gog_tool_action_new 	(GogView *view, GogTool *tool, double x, double y);
void 		 gog_tool_action_move 		(GogToolAction *action, double x, double y);
void 		 gog_tool_action_double_click 	(GogToolAction *action);
void 		 gog_tool_action_free 		(GogToolAction *action);

/*****************************************************************************/

struct _GogView {
	GObject	 base;

	GogObject *model;

	GogRenderer *renderer;  /* not NULL */
	GogView	    *parent;	/* potentially NULL */
	GSList	    *children;

	GogViewAllocation  allocation;	/* in renderer units */
	GogViewAllocation  residual;	/* left over after compass children are placed */
	unsigned allocation_valid: 1;  /* adjust our layout when child changes size */
	unsigned child_allocations_valid: 1;  /* some children need to adjust their layout */
	unsigned being_updated: 1;

	GSList	*toolkit; 	/* List of GogTool */
	void		*_priv; /* for future use */
};

typedef struct {
	GObjectClass	base;

	unsigned int clip; 	/* Automaticaly clip to object bounding box */

	/* Virtuals */
	void	 (*state_init)    (GogView *view);
	void	 (*padding_request) 		(GogView *view, GogViewAllocation const *bbox,
						 GogViewPadding *padding);
	void	 (*size_request)    		(GogView *view, GogViewRequisition const *available,
						 GogViewRequisition *requisition);
	void	 (*size_allocate)   		(GogView *view, GogViewAllocation const *allocation);

	void	 (*render)        		(GogView *view, GogViewAllocation const *bbox);

	void	 (*build_toolkit)		(GogView *view);
	char    *(*get_tip_at_point)		(GogView *view, double x, double y);
	void	 (*natural_size)    		(GogView *view, GogViewRequisition *req);
	/*<private>*/
	void	 (*reserved1)		(GogView *view);
	void	 (*reserved2)		(GogView *view);
} GogViewClass;

#define GOG_TYPE_VIEW		(gog_view_get_type ())
#define GOG_VIEW(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_VIEW, GogView))
#define GOG_IS_VIEW(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_VIEW))
#define GOG_VIEW_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST ((k), GOG_TYPE_VIEW, GogViewClass))
#define GOG_IS_VIEW_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GOG_TYPE_VIEW))
#define GOG_VIEW_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GOG_TYPE_VIEW, GogViewClass))

GType      gog_view_get_type (void);

GogObject *gog_view_get_model	     (GogView const *view);
void	   gog_view_render	     (GogView *view, GogViewAllocation const *bbox);
void       gog_view_queue_redraw     (GogView *view);
void       gog_view_queue_resize     (GogView *view);
void	   gog_view_padding_request  (GogView *view, GogViewAllocation const *bbox, GogViewPadding *padding);
void       gog_view_size_request     (GogView *view, GogViewRequisition const *available,
				      GogViewRequisition *requisition);
void       gog_view_size_allocate    (GogView *view, GogViewAllocation const *allocation);
gboolean   gog_view_update_sizes     (GogView *view);
GogView   *gog_view_find_child_view  (GogView const *container,
				      GogObject const *target_model);
void       gog_view_get_natural_size (GogView *view, GogViewRequisition *requisition);

GSList const	*gog_view_get_toolkit		(GogView *view);
void 		 gog_view_render_toolkit 	(GogView *view);
GogTool		*gog_view_get_tool_at_point 	(GogView *view, double x, double y,
						 GogObject **gobj);
GogView 	*gog_view_get_view_at_point	(GogView *view, double x, double y,
						 GogObject **obj, GogTool **tool);
char		*gog_view_get_tip_at_point      (GogView *view, double x, double y);

    /* protected */
void gog_view_size_child_request (GogView *view,
				  GogViewRequisition const *available,
				  GogViewRequisition *req,
				  GogViewRequisition *min_req);

G_END_DECLS

#endif /* GOG_VIEW_H */
