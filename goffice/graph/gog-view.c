/*
 * gog-view.c :
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

#include <goffice/goffice-config.h>
#include <goffice/goffice.h>
#include <goffice/goffice-debug.h>

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

/**
 * SECTION: gog-view
 * @short_description: Visual representation of a #GogObject.
 *
 * A #GogView displays a #GogObject using a #GogRenderer. The views of a graph
 * form a tree that mirrors the tree of the #GogObject models: when a child is
 * added to or removed from a model, the matching child view is created or
 * destroyed, and the order of the child views follows the order of the
 * model's children.
 *
 * The layout is done in two steps. A view tells its parent how much room it
 * wants with gog_view_size_request() (and gog_view_padding_request()), then
 * the parent gives it a position and a size with gog_view_size_allocate().
 * Allocations are expressed in renderer units. Once the layout is up to date
 * (see gog_view_update_sizes()), gog_view_render() draws a view and its
 * children.
 *
 * Each view also has a toolkit, a list of #GogTool that the user interface
 * can use to select, move or resize the object displayed by the view.
 **/

/**
 * GogViewClass:
 * @base: base class.
 * @clip: If %TRUE, clip drawings to the view allocation.
 * @state_init: called once the view has both a model and a renderer, after
 * the views of the model's children have been created.
 * @padding_request: computes the padding needed around the area in which the
 * view is laid out, see gog_view_padding_request().
 * @size_request: computes the size the view would like to have, see
 * gog_view_size_request().
 * @size_allocate: assigns a position and a size to the view and lays out its
 * children, see gog_view_size_allocate().
 * @render: draws the view and its children, see gog_view_render(). The
 * default implementation renders the children.
 * @build_toolkit: fills the view's toolkit with #GogTool, called by
 * gog_view_get_toolkit() the first time the toolkit is needed.
 * @get_tip_at_point: returns a newly allocated tip string for a position, or
 * %NULL, see gog_view_get_tip_at_point().
 * @natural_size: computes the natural size of the view.
 **/

/**
 * GogViewAllocation:
 * @w: width.
 * @h: height.
 * @x: horizontal position.
 * @y: vertical position.
 *
 * A rectangle, in renderer units.
 **/

/**
 * GogViewPadding:
 * @wr: right padding.
 * @hb: bottom padding.
 * @wl: left padding.
 * @ht: top padding.
 **/

/**
 * GogViewRequisition:
 * @w: width.
 * @h: height.
 **/

static GogViewAllocation *
gog_view_allocation_copy (GogViewAllocation const *alloc)
{
	return go_memdup (alloc, sizeof (GogViewAllocation));
}

/**
 * gog_view_allocation_get_type:
 *
 * Returns: the #GType of #GogViewAllocation.
 **/
GType
gog_view_allocation_get_type (void)
{
	static GType type = 0;

	if (type == 0)
		type = g_boxed_type_register_static
			("GogViewAllocation",
			 (GBoxedCopyFunc)gog_view_allocation_copy,
			 (GBoxedFreeFunc)g_free);

	return type;
}

/*****************************************************************************/

/**
 * SECTION: gog-tool
 * @short_description: Interactive tools for #GogView.
 *
 * A #GogTool describes an interaction that the user can have with a
 * #GogView, such as selecting, moving or resizing the object the view
 * displays. The tools that apply to a view are listed in its toolkit, see
 * gog_view_get_toolkit().
 *
 * When the user starts using a tool, a #GogToolAction is created with
 * gog_tool_action_new() to keep track of the interaction. It is then told
 * about the pointer movements with gog_tool_action_move() and about double
 * clicks with gog_tool_action_double_click(), and released with
 * gog_tool_action_free() when the interaction is over.
 **/

/**
 * GogTool:
 * @name: tool name, marked for translation.
 * @cursor_type: pointer cursor type for the tool.
 * @point: tests whether the tool applies to a view at a given position. It
 * can store in its last argument the object the tool applies to; when it does
 * not, the model of the view is used.
 * @render: displays the tool for a view, for example a selection rectangle or
 * a resizing grip.
 * @init: initializes an action, called when the action is created.
 * @move: callback for pointer move, receives the new pointer position.
 * @double_click: callback on double click.
 * @destroy: destroys what @init allocated for an action. The @data member of
 * the action is released with g_free() afterwards.
 **/

#ifdef GOFFICE_WITH_GTK
static gboolean
gog_tool_select_object_point (GogView *view, double x, double y, G_GNUC_UNUSED GogObject **gobj)
{
	return (x >= view->allocation.x &&
		x <= (view->allocation.x + view->allocation.w) &&
		y >= view->allocation.y &&
		y <= (view->allocation.y + view->allocation.h));
}

static void
gog_tool_select_object_render (GogView *view)
{
	GogViewAllocation rect = view->allocation;
	GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (view->model));
	double line_width = gog_renderer_line_size (view->renderer, style->line.width);

	rect.x -= line_width / 2.0;
	rect.y -= line_width / 2.0;
	rect.w += line_width;
	rect.h += line_width;

	gog_renderer_draw_selection_rectangle (view->renderer, &rect);
}

static GogTool gog_tool_select_object = {
	N_("Select object"),
	GDK_LEFT_PTR,
	gog_tool_select_object_point,
	gog_tool_select_object_render,
	NULL /* init */,
        NULL /* move */,
	NULL /* double_click */,
	NULL /* destroy */
};

typedef struct {
	GogViewAllocation 	parent_allocation;
	GogViewAllocation	start_position;
	gboolean		valid; /* FALSE if the parent allocation can't be used as a scale */
} MoveObjectData;

static gboolean
gog_tool_move_object_point (GogView *view, double x, double y, G_GNUC_UNUSED GogObject **gobj)
{
	if (view->model->role == NULL)
		return FALSE;

	if ((view->model->role->allowable_positions &
	    (GOG_POSITION_MANUAL)) == 0)
		return FALSE;

	return (x >= view->allocation.x &&
		x <= (view->allocation.x + view->allocation.w) &&
		y >= view->allocation.y &&
		y <= (view->allocation.y + view->allocation.h));
}

static void
gog_tool_move_object_init (GogToolAction *action)
{
	MoveObjectData *data = g_new0 (MoveObjectData, 1);
	GogView *parent = action->view->parent;

	action->data = data;

	/* Positions are relative to the size of the parent.  Without a parent,
	 * or with one that has no (or an undefined, i.e. NaN) size, there is
	 * nothing to divide by: leave the object alone. */
	data->valid = parent != NULL &&
		parent->allocation.w > 0 && parent->allocation.h > 0;
	if (!data->valid)
		return;

	data->parent_allocation = parent->allocation;
	gog_object_get_manual_position (action->view->model, &data->start_position);

	if ((gog_object_get_position_flags (action->view->model, GOG_POSITION_MANUAL) &
	     GOG_POSITION_MANUAL) == 0) {
		data->start_position.x = action->view->allocation.x / data->parent_allocation.w;
		data->start_position.y = action->view->allocation.y / data->parent_allocation.h;
		data->start_position.w = action->view->allocation.w / data->parent_allocation.w;
		data->start_position.h = action->view->allocation.h / data->parent_allocation.h;
	}
}

static void
gog_tool_move_object_move (GogToolAction *action, double x, double y)
{
	GogViewAllocation position;
	MoveObjectData *data = action->data;

	if (!data->valid)
		return;

	position.x = data->start_position.x + (x - action->start_x) / data->parent_allocation.w;
	position.y = data->start_position.y + (y - action->start_y) / data->parent_allocation.h;
	position.w = data->start_position.w;
	position.h = data->start_position.h;
	gog_object_set_manual_position (action->view->model, &position);
	gog_object_set_position_flags (action->view->model, GOG_POSITION_MANUAL, GOG_POSITION_MANUAL);
}

static void
gog_tool_move_object_double_click (GogToolAction *action)
{
	gog_object_set_position_flags (action->view->model, 0, GOG_POSITION_MANUAL);
}


static GogTool gog_tool_move_object = {
	N_("Move"),
	GDK_FLEUR,
	gog_tool_move_object_point,
	NULL /* render */,
	gog_tool_move_object_init,
	gog_tool_move_object_move,
	gog_tool_move_object_double_click,
	NULL /* destroy */
};

static gboolean
gog_tool_resize_object_point (GogView *view, double x, double y, G_GNUC_UNUSED GogObject **gobj)
{
	if (GOG_MANUAL_SIZE_AUTO == gog_object_get_manual_size_mode (view->model))
		return FALSE;

	return gog_renderer_in_grip (x, y,
				     view->allocation.x + view->allocation.w,
				     view->allocation.y + view->allocation.h);
}

static void
gog_tool_resize_object_render (GogView *view)
{
	if (GOG_MANUAL_SIZE_AUTO == gog_object_get_manual_size_mode (view->model))
		return;

	gog_renderer_draw_grip (view->renderer,
				view->allocation.x + view->allocation.w,
				view->allocation.y + view->allocation.h);
}

static void
gog_tool_resize_object_move (GogToolAction *action, double x, double y)
{
	GogViewAllocation position;
	MoveObjectData *data = action->data;

	if (!data->valid)
		return;

	position.x = data->start_position.x;
	position.y = data->start_position.y;
	position.w = data->start_position.w + (x - action->start_x) / data->parent_allocation.w;
	position.h = data->start_position.h + (y - action->start_y) / data->parent_allocation.h;
	gog_object_set_manual_position (action->view->model, &position);
	gog_object_set_position_flags (action->view->model, GOG_POSITION_MANUAL, GOG_POSITION_MANUAL);
}

static GogTool gog_tool_resize_object = {
	N_("Resize object"),
	GDK_BOTTOM_RIGHT_CORNER,
	gog_tool_resize_object_point,
	gog_tool_resize_object_render,
	gog_tool_move_object_init,
	gog_tool_resize_object_move,
	NULL /* double-click */,
	NULL /* destroy */
};
#endif

/*****************************************************************************/

/**
 * GogToolAction:
 * @start_x: initial pointer horizontal position, in renderer units.
 * @start_y: initial pointer vertical position, in renderer units.
 * @view: the #GogView the tool is used on. The action does not hold a
 * reference on it.
 * @tool: the #GogTool being used.
 * @data: private data of the tool, set by its @init method and released with
 * g_free() when the action is destroyed.
 * @ref_count: internal.
 **/

static GogToolAction *
gog_tool_action_ref (GogToolAction *action)
{
	action->ref_count++;
	return action;
}

/**
 * gog_tool_action_get_type:
 *
 * Returns: the #GType of #GogToolAction.
 **/
GType
gog_tool_action_get_type (void)
{
	static GType t = 0;

	if (t == 0) {
		t = g_boxed_type_register_static ("GogToolAction",
			 (GBoxedCopyFunc)gog_tool_action_ref,
			 (GBoxedFreeFunc)gog_tool_action_free);
	}
	return t;
}

/**
 * gog_tool_action_new:
 * @view: the #GogView on which @tool is used
 * @tool: the #GogTool being used
 * @x: horizontal position of the pointer, in renderer units
 * @y: vertical position of the pointer, in renderer units
 *
 * Starts an interaction with @tool on @view and calls the init method of
 * @tool, if any. The initial pointer position is kept so that later moves can
 * be interpreted relatively to it. The action does not hold a reference on
 * @view, so it must not outlive it.
 *
 * Returns: (transfer full) (nullable): a new #GogToolAction, to be released
 * with gog_tool_action_free(), or %NULL if the arguments are invalid.
 **/
GogToolAction *
gog_tool_action_new (GogView *view, GogTool *tool, double x, double y)
{
	GogToolAction *action;

	g_return_val_if_fail (GOG_IS_VIEW (view), NULL);
	g_return_val_if_fail (tool != NULL, NULL);

	action = g_new0 (GogToolAction, 1);

	action->tool = tool;
	action->view = view;
	action->data = NULL;
	action->start_x = x;
	action->start_y = y;
	action->ref_count = 1;

	if (tool->init != NULL)
		(tool->init) (action);

	return action;
}

/**
 * gog_tool_action_move:
 * @action: a #GogToolAction
 * @x: new horizontal position of the pointer, in renderer units
 * @y: new vertical position of the pointer, in renderer units
 *
 * Tells @action that the pointer has moved, by calling the move method of its
 * tool, if any.
 **/
void
gog_tool_action_move (GogToolAction *action, double x, double y)
{
	g_return_if_fail (action != NULL);

	if (action->tool->move != NULL)
		(action->tool->move) (action, x, y);
}

/**
 * gog_tool_action_double_click:
 * @action: a #GogToolAction
 *
 * Tells @action that a double click happened, by calling the double_click
 * method of its tool, if any.
 **/
void
gog_tool_action_double_click (GogToolAction *action)
{
	g_return_if_fail (action != NULL);

	if (action->tool->double_click != NULL)
		(action->tool->double_click) (action);
}

/**
 * gog_tool_action_free:
 * @action: a #GogToolAction
 *
 * Drops a reference on @action. When the last reference is dropped, the
 * destroy method of the tool is called, if any, then the data of @action and
 * @action itself are freed.
 **/
void
gog_tool_action_free (GogToolAction *action)
{
	g_return_if_fail (action != NULL);

	if (action->ref_count-- > 1)
		return;

	if (action->tool->destroy != NULL)
		(action->tool->destroy) (action);

	g_free (action->data);
	g_free (action);
}

/****/

/* this should be per model */
#define PAD_HACK	4	/* pts */

enum {
	GOG_VIEW_PROP_0,
	GOG_VIEW_PROP_PARENT,
	GOG_VIEW_PROP_MODEL
};

static GObjectClass *parent_klass;

static void
cb_child_added (GogObject *parent, GogObject *child,
		GogView *view)
{
	GogView *child_view;

	g_return_if_fail (view->model == parent);

	child_view = gog_object_new_view (child, view);
	if (child_view)
		g_object_unref (child_view);
	gog_view_queue_resize (view);
}

static void
cb_remove_child (GogObject *parent, GogObject *child,
		 GogView *view)
{
	GSList *ptr = view->children;
	GogObjectClass const *klass;
	GogView *tmp;

	g_return_if_fail (view->model == parent);

	gog_view_queue_resize (view);
	for (; ptr != NULL ; ptr = ptr->next) {
		tmp = GOG_VIEW (ptr->data);

		g_return_if_fail (tmp != NULL);

		if (tmp->model == child) {
			view->children = g_slist_remove (view->children, tmp);
			tmp->parent = NULL;
			g_object_unref (tmp);
			return;
		}
	}

	/* The object may not create a view */
	klass = GOG_OBJECT_GET_CLASS (child);
	if (klass->view_type != 0)
		g_warning ("%s (%p) saw %s(%p) being removed from %s(%p) for which I didn't have a child",
			   G_OBJECT_TYPE_NAME (view), view,
			   G_OBJECT_TYPE_NAME (child), child,
			   G_OBJECT_TYPE_NAME (parent), parent);
}

static void
cb_model_changed (GogObject *model, gboolean resized, GogView *view)
{
	gog_debug (0, g_warning ("model %s(%p) for view %s(%p) changed %d",
		   G_OBJECT_TYPE_NAME (model), model,
		   G_OBJECT_TYPE_NAME (view), view, resized););
	if (resized)
		gog_view_queue_resize (view);
	else
		gog_view_queue_redraw (view);
}

/* make the list of view children match the models order */
static void
cb_model_reordered (GogView *view)
{
	GSList *ptr, *tmp, *new_order = NULL;
	GSList *old_children = view->children;
	GSList *available = g_slist_copy (old_children);

	for (ptr = view->model->children; ptr != NULL ; ptr = ptr->next) {
		GSList *found = NULL;
		for (tmp = available; tmp != NULL; tmp = tmp->next) {
			if (GOG_VIEW (tmp->data)->model == ptr->data) {
				found = tmp;
				break;
			}
		}
		if (found != NULL) {
			new_order = g_slist_prepend (new_order, found->data);
			available = g_slist_delete_link (available, found);
		}
	}

	for (tmp = available; tmp != NULL; tmp = tmp->next) {
		GogView *child = GOG_VIEW (tmp->data);
		child->parent = NULL;
		g_object_unref (child);
	}
	g_slist_free (available);

	g_slist_free (old_children);
	view->children = g_slist_reverse (new_order);
}

static void
gog_view_set_property (GObject *gobject, guint param_id,
		       GValue const *value, GParamSpec *pspec)
{
	GogView *view = GOG_VIEW (gobject);
	gboolean init_state = (view->renderer == NULL || view->model == NULL);

	switch (param_id) {
	case GOG_VIEW_PROP_PARENT:
		g_return_if_fail (view->parent == NULL);

		view->parent = GOG_VIEW (g_value_get_object (value));
		if (view->parent != NULL) {
			view->renderer = view->parent->renderer;
			g_object_ref (view);
			view->parent->children = g_slist_prepend (view->parent->children, view);
			cb_model_reordered (view->parent);
		}
		break;

	case GOG_VIEW_PROP_MODEL:
		g_return_if_fail (view->model == NULL);

		view->model = GOG_OBJECT (g_value_get_object (value));
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}

	/* renderer set via parent or manually */
	if (init_state && view->renderer != NULL && view->model != NULL) {
		GogViewClass *klass = GOG_VIEW_GET_CLASS (view);
		GSList *ptr = view->model->children;

		for ( ;ptr != NULL ; ptr = ptr->next) {
			GogView *child_view = gog_object_new_view (ptr->data, view);
			if (child_view)
				g_object_unref (child_view);
		}

		g_signal_connect_object (G_OBJECT (view->model),
			"child_added",
			G_CALLBACK (cb_child_added), view, 0);
		g_signal_connect_object (G_OBJECT (view->model),
			"child_removed",
			G_CALLBACK (cb_remove_child), view, 0);
		g_signal_connect_object (G_OBJECT (view->model),
			"changed",
			G_CALLBACK (cb_model_changed), view, 0);
		g_signal_connect_object (G_OBJECT (view->model),
			"children-reordered",
			G_CALLBACK (cb_model_reordered), view, G_CONNECT_SWAPPED);

		if (klass->state_init != NULL)
			(klass->state_init) (view);
	}
}

static void
gog_view_finalize (GObject *obj)
{
	GogView *tmp, *view = GOG_VIEW (obj);
	GSList *ptr;

	if (view->parent != NULL)
		view->parent->children = g_slist_remove (view->parent->children, view);

	for (ptr = view->children; ptr != NULL ; ptr = ptr->next) {
		tmp = GOG_VIEW (ptr->data);
		/* not really necessary, but helpful during initial deployment
		 * when not everything has a view yet */
		if (tmp != NULL) {
			tmp->parent = NULL; /* short circuit */
			g_object_unref (tmp);
		}
	}
	g_slist_free (view->children);
	view->children = NULL;

	g_slist_free (view->toolkit);
	view->toolkit = NULL;

	(*parent_klass->finalize) (obj);
}

static void
gog_view_padding_request_real (GogView *view, GogViewAllocation const *bbox, GogViewPadding *padding)
{
	GSList *ptr;
	GogView *child;
	GogViewPadding child_padding;

	for (ptr = view->children; ptr != NULL ; ptr = ptr->next) {
		child = ptr->data;
		if (GOG_POSITION_IS_PADDING (child->model->position)) {
			gog_view_padding_request (child, bbox, &child_padding);
			padding->wr = MAX (padding->wr, child_padding.wr);
			padding->wl = MAX (padding->wl, child_padding.wl);
			padding->hb = MAX (padding->hb, child_padding.hb);
			padding->ht = MAX (padding->ht, child_padding.ht);
		}
	}
}

static void
gog_view_size_request_real (G_GNUC_UNUSED GogView *view,
			    G_GNUC_UNUSED GogViewRequisition const *available,
			    GogViewRequisition *req)
{
	req->w = req->h = 1.;
}

static void
gog_view_size_allocate_real (GogView *view, GogViewAllocation const *allocation)
{
	GSList *ptr;
	GogView *child;
	GogObjectPosition pos;
	GogViewRequisition req, available;
	GogViewAllocation tmp, res = *allocation, align;
	double const pad_h = gog_renderer_pt2r_y (view->renderer, PAD_HACK);
	double const pad_w = gog_renderer_pt2r_x (view->renderer, PAD_HACK);

	for (ptr = view->children; ptr != NULL ; ptr = ptr->next) {
		child = ptr->data;

		pos = child->model->position;
		if (pos & GOG_POSITION_MANUAL) {
			available.w = res.w;
			available.h = res.h;

			gog_view_size_request (child, &available, &req);
			tmp = gog_object_get_manual_allocation (gog_view_get_model (child),
								allocation, &req);
			gog_view_size_allocate (child, &tmp);
		} else if (pos & GOG_POSITION_COMPASS) {
			gboolean vertical = TRUE;

			/* Dead simple */
			available.w = res.w;
			available.h = res.h;
			gog_view_size_request (child, &available, &req);
			if (req.h > res.h)
				req.h = res.h;
			if (req.w > res.w)
				req.w = res.w;
			tmp = res;

			if (pos & GOG_POSITION_N) {
				if (req.h > 0) {
					res.y += req.h + pad_h;
					res.h -= req.h + pad_h;
				} else
					req.h = 0;
				tmp.h  = req.h;
				vertical = FALSE;
			} else if (pos & GOG_POSITION_S) {
				if (req.h > 0) {
					res.h -= req.h + pad_h;
					tmp.y  = res.y + res.h + pad_h;
				} else
					req.h = 0;
				tmp.h  = req.h;
				vertical = FALSE;
			}/* else
				tmp.h = req.h;*/

				if (pos & GOG_POSITION_E) {
					if (req.w > 0) {
						res.w -= req.w + pad_w;
						tmp.x  = res.x + res.w + pad_w;
					} else
						req.w = 0;
					tmp.w  = req.w;
					/* For NE & NW only alignment fill makes sense */
					if (pos & (GOG_POSITION_N|GOG_POSITION_S))
						pos = GOG_POSITION_ALIGN_FILL;
				} else if (pos & GOG_POSITION_W) {
					if (req.w > 0) {
						res.x += req.w + pad_w;
						res.w -= req.w + pad_w;
					} else
						req.w = 0;
					tmp.w  = req.w;
					/* For NE & NW only alignment fill makes sense */
					if (pos & (GOG_POSITION_N|GOG_POSITION_S))
						pos = GOG_POSITION_ALIGN_FILL;
 				}/* else
					tmp.w = req.w;*/

				/* the following line adjust the manual sizes if needed */
				align = gog_object_get_manual_allocation (gog_view_get_model (child),
				                                          allocation, &req);
				req.h = align.h;
				req.w = align.w;
				pos &= GOG_POSITION_ALIGNMENT;
				if (GOG_POSITION_ALIGN_FILL != pos) {
					if (vertical) {
						if (GOG_POSITION_ALIGN_END == pos) {
							if (tmp.h >= req.h)
								tmp.y += tmp.h - req.h;
						} else if (GOG_POSITION_ALIGN_CENTER == pos) {
							if (tmp.h >= req.h)
								tmp.y += (tmp.h - req.h) / 2.;
						}
						tmp.h = req.h;
					} else {
						if (GOG_POSITION_ALIGN_END == pos) {
							if (tmp.w >= req.w)
								tmp.x += tmp.w - req.w;
						} else if (GOG_POSITION_ALIGN_CENTER == pos) {
							if (tmp.w >= req.w)
								tmp.x += (tmp.w - req.w) / 2.;
						}
						tmp.w = req.w;
					}
				}

			gog_view_size_allocate (child, &tmp);
		} else if (!(GOG_POSITION_IS_SPECIAL (pos)) &&
			   !(GOG_POSITION_IS_PADDING (pos)))
			g_warning ("[GogView::size_allocate_real] unexpected position %x for child %p of %p",
				   pos, child, view);
	}

	view->residual = res;
}

/* A simple default implementation */
static void
gog_view_render_real (GogView *view, GogViewAllocation const *bbox)
{
	GSList *ptr;
	for (ptr = view->children ; ptr != NULL ; ptr = ptr->next)
		gog_view_render	(ptr->data, bbox);
}

static void
gog_view_build_toolkit (GogView *view)
{
#ifdef GOFFICE_WITH_GTK
	view->toolkit = g_slist_prepend (view->toolkit, &gog_tool_select_object);
	view->toolkit = g_slist_prepend (view->toolkit, &gog_tool_move_object);
	view->toolkit = g_slist_prepend (view->toolkit, &gog_tool_resize_object);
#endif
}

static void
gog_view_class_init (GogViewClass *view_klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) view_klass;

	parent_klass = g_type_class_peek_parent (view_klass);
	gobject_klass->set_property  = gog_view_set_property;
	gobject_klass->finalize	     = gog_view_finalize;
	view_klass->padding_request  = gog_view_padding_request_real;
	view_klass->size_request     = gog_view_size_request_real;
	view_klass->size_allocate    = gog_view_size_allocate_real;
	view_klass->render	     = gog_view_render_real;
	view_klass->build_toolkit    = gog_view_build_toolkit;
	view_klass->clip	     = FALSE;

	g_object_class_install_property (gobject_klass, GOG_VIEW_PROP_PARENT,
		g_param_spec_object ("parent",
			_("Parent"),
			_("the GogView parent"),
			GOG_TYPE_VIEW,
			GSF_PARAM_STATIC | G_PARAM_WRITABLE));
	g_object_class_install_property (gobject_klass, GOG_VIEW_PROP_MODEL,
		g_param_spec_object ("model",
			_("Model"),
			_("The GogObject this view displays"),
			GOG_TYPE_OBJECT,
			GSF_PARAM_STATIC | G_PARAM_WRITABLE));
}

static void
gog_view_init (GogView *view)
{
	view->allocation_valid  = FALSE;
	view->child_allocations_valid = FALSE;
	view->being_updated  = FALSE;
	view->model	     = NULL;
	view->parent	     = NULL;
	view->children	     = NULL;
	view->toolkit	     = NULL;
}

GSF_CLASS_ABSTRACT (GogView, gog_view,
		    gog_view_class_init, gog_view_init,
		    G_TYPE_OBJECT)

/**
 * gog_view_get_model:
 * @view: #GogView
 *
 * Returns: (transfer none): the #GogObject owning the view.
 **/
GogObject *
gog_view_get_model (GogView const *view)
{
	g_return_val_if_fail (GOG_IS_VIEW (view), NULL);

	return view->model;
}

/**
 * gog_view_queue_redraw:
 * @view: a #GogView
 *
 * Requests a redraw for the entire graph.
 **/
void
gog_view_queue_redraw (GogView *view)
{
	g_return_if_fail (GOG_IS_VIEW (view));
	g_return_if_fail (view->renderer != NULL);

	gog_renderer_request_update (view->renderer);
}

/**
 * gog_view_queue_resize:
 * @view: a #GogView
 *
 * Flags a view to have its size renegotiated; should
 * be called when a model for some reason has a new size request.
 * For example, when you change the size of a legend.
 **/
void
gog_view_queue_resize (GogView *view)
{
	g_return_if_fail (GOG_IS_VIEW (view));
	g_return_if_fail (view->renderer != NULL);

	gog_renderer_request_update (view->renderer);

	do {
		view->allocation_valid = FALSE; /* in case there is no parent */
	} while (NULL != (view = view->parent) && view->allocation_valid);
}

/**
 * gog_view_padding_request:
 * @view: a #GogView
 * @bbox: the area in which @view is going to be laid out
 * @padding: (out): where to store the padding
 *
 * Determines how much room the children of @view that are positioned as
 * padding (for example the axes of a chart) need around the main area of
 * @view. @padding is zeroed before the padding_request method of the class is
 * called, so implementations only have to increase it. The default
 * implementation keeps, for each side, the largest padding requested by the
 * children.
 **/
void
gog_view_padding_request (GogView *view, GogViewAllocation const *bbox, GogViewPadding *padding)
{
	GogViewClass *klass;

	g_return_if_fail (GOG_IS_VIEW (view));
	g_return_if_fail (padding != NULL);
	g_return_if_fail (bbox != NULL);

	klass = GOG_VIEW_GET_CLASS (view);

	padding->wl = padding->wr = padding->ht = padding->hb = 0.;

	if (klass->padding_request != NULL)
		(klass->padding_request) (view, bbox, padding);
}


/**
 * gog_view_size_request:
 * @view: a #GogView
 * @available: available space.
 * @requisition: (out): where to store the #GogViewRequisition.
 *
 * Determines the desired size of a view.
 *
 * Note, that the virtual method deviates slightly from this function.  This
 * function will zero @requisition before calling the virtual method.
 *
 * Remember that the size request is not necessarily the size a view will
 * actually be allocated.
 *
 **/
void
gog_view_size_request (GogView *view,
		       GogViewRequisition const *available,
		       GogViewRequisition *requisition)
{
	GogViewClass *klass;

	g_return_if_fail (GOG_IS_VIEW (view));
	g_return_if_fail (requisition != NULL);
	g_return_if_fail (available != NULL);

	klass = GOG_VIEW_GET_CLASS (view);

	if (klass->size_request) {
		requisition->w = requisition->h = 0;
		klass->size_request (view, available, requisition);
	} else
		requisition->w = requisition->h = 1.;
}

/**
 * gog_view_size_allocate:
 * @view: a #GogView
 * @allocation: position and size to be allocated to @view
 *
 * Assign a size and position to a GogView.  Primarily used by containers.
 *
 * The size_allocate method of the class is called, which is expected to lay
 * out the children of @view, then @allocation is stored as the allocation of
 * @view and the layout of @view is considered valid. It is an error to call
 * this function while @view is already being updated.
 **/
void
gog_view_size_allocate (GogView *view, GogViewAllocation const *allocation)
{
	GogViewClass *klass;

	g_return_if_fail (GOG_IS_VIEW (view));
	g_return_if_fail (allocation != NULL);

	klass = GOG_VIEW_GET_CLASS (view);
	g_return_if_fail (klass->size_allocate != NULL);
	g_return_if_fail (!view->being_updated);

	gog_debug (0, g_warning ("size_allocate %s %p : x = %g, y = %g w = %g, h = %g",
		   G_OBJECT_TYPE_NAME (view), view,
		   allocation->x, allocation->y, allocation->w, allocation->h););

	view->being_updated = TRUE;
	(klass->size_allocate) (view, allocation);
	view->being_updated = FALSE;

	if (&view->allocation != allocation)
		view->allocation = *allocation;
	view->allocation_valid = view->child_allocations_valid = TRUE;
}

/**
 * gog_view_update_sizes:
 * @view: #GogView
 *
 * Makes sure that the layout of @view and of its children is up to date. A
 * view whose allocation was invalidated, see gog_view_queue_resize(), is
 * allocated again using its current allocation.
 *
 * Returns: %TRUE if a redraw is necessary.
 **/
gboolean
gog_view_update_sizes (GogView *view)
{
	g_return_val_if_fail (GOG_IS_VIEW (view), TRUE);
	g_return_val_if_fail (!view->being_updated, TRUE);

	if (!view->allocation_valid)
		gog_view_size_allocate (view, &view->allocation);
	else if (!view->child_allocations_valid) {
		GSList *ptr;

		view->being_updated = TRUE;
		for (ptr = view->children ; ptr != NULL ; ptr = ptr->next)
			gog_view_update_sizes (ptr->data);
		view->being_updated = FALSE;

		view->child_allocations_valid = TRUE;
	} else
		return FALSE;
	return TRUE;
}

/**
 * gog_view_render:
 * @view: a #GogView
 * @bbox: (nullable): the area to render, or %NULL, passed to the render
 * method of the class
 *
 * Renders @view and its children using the renderer of @view. Nothing is drawn
 * if the model of @view is invisible or if its allocation has a negative or
 * undefined size. If the class asks for it, the drawing is clipped to the
 * allocation of @view.
 **/
void
gog_view_render	(GogView *view, GogViewAllocation const *bbox)
{
	GogViewClass *klass;

	g_return_if_fail (GOG_IS_VIEW (view));
	g_return_if_fail (view->renderer != NULL);

	klass = GOG_VIEW_GET_CLASS (view);

	/* In particular this is true for NaNs.  */
	if (view->model->invisible ||
	    !(view->allocation.w >= 0 && view->allocation.h >= 0))
		return;

	if (klass->clip) {
		gog_renderer_push_clip_rectangle (view->renderer, view->allocation.x, view->allocation.y,
						  view->allocation.w, view->allocation.h);

		klass->render (view, bbox);

		gog_renderer_pop_clip (view->renderer);
	}
	else
		klass->render (view, bbox);
}

/**
 * gog_view_size_child_request:
 * @view: a #GogView
 * @available: the amount of space available in total
 * @req: (out): additional requisition
 * @min_req: (out): minimum size for displaying all children
 *
 * Computes additional requisition in @req which must be added to parent requisition,
 * and minimum requisition in @min_req which is minimum space for displaying all
 * children.  Children with a manual position are not supported yet.
 *
 **/
void
gog_view_size_child_request (GogView *view,
			     GogViewRequisition const *available,
			     GogViewRequisition *req,
			     GogViewRequisition *min_req)
{
	GSList *ptr, *list;
	GogView *child;
	GogObjectPosition pos;
	GogViewRequisition child_req;
	double pad_h, pad_w;

	g_return_if_fail (GOG_IS_VIEW (view));
	g_return_if_fail (available != NULL);
	g_return_if_fail (req != NULL);
	g_return_if_fail (min_req != NULL);

	pad_h = gog_renderer_pt2r_y (view->renderer, PAD_HACK);
	pad_w = gog_renderer_pt2r_x (view->renderer, PAD_HACK);

	req->w = req->h = min_req->w = min_req->h = 0.;

	/* walk the list in reverse */
	list = g_slist_reverse (g_slist_copy (view->children));
	for (ptr = list; ptr != NULL ; ptr = ptr->next) {
		child = ptr->data;

		pos = child->model->position;
		if (pos & GOG_POSITION_MANUAL) {
			g_warning ("manual is not supported yet");
		} else if (pos & GOG_POSITION_COMPASS) {
			/* Dead simple */
			gog_view_size_request (child, available, &child_req);

			if (pos & (GOG_POSITION_N|GOG_POSITION_S)) {
				if (child_req.h > 0) {
					req->h += child_req.h + pad_h;
					min_req->h += child_req.h + pad_h;
				}
			} else if (min_req->h < child_req.h)
				min_req->h = child_req.h;

			if (pos & (GOG_POSITION_E|GOG_POSITION_W)) {
				if (child_req.w > 0) {
					req->w += child_req.w + pad_w;
					min_req->w += child_req.w + pad_w;
				}
			} else if (min_req->w < child_req.w)
				min_req->w = child_req.w;

		} else if (!(GOG_POSITION_IS_SPECIAL (pos)))
			g_warning ("[GogView::size_child_request] unexpected position %x for child %p of %p",
				   pos, child, view);
	}
	g_slist_free (list);
}

/**
 * gog_view_find_child_view:
 * @container: #GogView
 * @target_model: #GogObject
 *
 * Find the #GogView, @container or one of its descendants, that corresponds
 * to @target_model. The model of @container must be @target_model or one of
 * its ancestors.
 *
 * Returns: (transfer none) (nullable): the view of @target_model, or %NULL on
 * error or if @target_model has no view.
 **/
GogView *
gog_view_find_child_view  (GogView const *container, GogObject const *target_model)
{
	GogObject const *obj, *old_target;
	GSList *ptr;

	g_return_val_if_fail (GOG_IS_VIEW (container), NULL);
	g_return_val_if_fail (GOG_IS_OBJECT (target_model), NULL);

	/* @container is a view for @target_models parent */
	obj = target_model;
	while (obj != NULL && container->model != obj)
		obj = obj->parent;

	g_return_val_if_fail (obj != NULL, NULL);

	for ( ; obj != target_model ; container = ptr->data) {
		/* find the parent of @target_object that should be a child of this view */
		old_target = obj;
		obj = target_model;
		while (obj != NULL && obj->parent != old_target)
			obj = obj->parent;

		g_return_val_if_fail (obj != NULL, NULL);

		for (ptr = container->children ; ptr != NULL ; ptr = ptr->next)
			if (GOG_VIEW (ptr->data)->model == obj)
				break;

		/* target_model doesn't have view */
		if (ptr == NULL)
			return NULL;
	}

	return (GogView *)container;
}

/**
 * gog_view_render_toolkit:
 * @view: #GogView
 *
 * Render toolkit elements, by calling the render method of each tool of the
 * toolkit of @view that has one.
 **/
void
gog_view_render_toolkit (GogView *view)
{
	GogTool *tool;
	GSList const *ptr;

	g_return_if_fail (GOG_IS_VIEW (view));

	for (ptr = gog_view_get_toolkit (view); ptr != NULL; ptr = ptr->next) {
		tool = ptr->data;
		if (tool->render != NULL)
			(tool->render) (view);
	}
}

/**
 * gog_view_get_toolkit:
 * @view: #GogView
 *
 * The toolkit is built the first time it is needed, using the build_toolkit
 * method of the class.
 *
 * Returns: (element-type GogTool) (transfer none): toolkit associated with given view.
 **/
GSList const *
gog_view_get_toolkit (GogView *view)
{
	g_return_val_if_fail (GOG_IS_VIEW (view), NULL);

	if  (view->toolkit == NULL) {
		GogViewClass *klass = GOG_VIEW_GET_CLASS (view);
		if (klass->build_toolkit != NULL)
			(klass->build_toolkit) (view);
	}

	return view->toolkit;
}

/**
 * gog_view_get_tool_at_point:
 * @view: #GogView
 * @x: pointer horizontal position, in renderer units
 * @y: pointer vertical position, in renderer units
 * @gobj: (out) (optional) (nullable): location for the pointed object, set to
 * %NULL if there is no tool at that position
 *
 * Looks, in the order of the toolkit of @view, for the first tool that applies
 * at the given position. If that tool does not designate a particular object,
 * the model of @view is stored in @gobj.
 *
 * Returns: (transfer none) (nullable): tool under cursor for a given view, or
 * %NULL
 **/
GogTool *
gog_view_get_tool_at_point (GogView *view, double x, double y, GogObject **gobj)
{
	GogObject *current_gobj = NULL;
	GogTool *current_tool;
	GSList const *ptr;

	g_return_val_if_fail (GOG_IS_VIEW (view), NULL);

	for (ptr = gog_view_get_toolkit (view); ptr != NULL; ptr = ptr->next) {
		current_tool = ptr->data;
		if (current_tool->point != NULL &&
		    (current_tool->point) (view, x, y, &current_gobj)) {
			if (gobj != NULL)
			       *gobj = current_gobj == NULL ? view->model : current_gobj;
			return current_tool;
		}
	}

	if (gobj != NULL)
		*gobj = NULL;
	return NULL;
}

/**
 * gog_view_get_view_at_point:
 * @view: #GogView
 * @x: cursor x position, in renderer units
 * @y: cursor y position, in renderer units
 * @obj: (out) (optional) (nullable): location for the pointed object, set to
 * %NULL if there is none
 * @tool: (out) (optional) (nullable): location for the pointed tool, left
 * unchanged if there is none
 *
 * Gets view under cursor, searching recursively from @view. The children are
 * tried from the last one to the first one, before @view itself. Corresponding
 * object is stored in @obj. This object may or may not be @view->model of
 * pointed view. This function also stores tool under cursor, for the pointed
 * view.
 *
 * Returns: (transfer none) (nullable): the #GogView at x,y position, or %NULL
 **/
GogView *
gog_view_get_view_at_point (GogView *view, double x, double y, GogObject **obj, GogTool **tool)
{
	GogView *pointed_view;
	GSList const *ptr;
	GSList *list;
	GogTool *current_tool;

	g_return_val_if_fail (GOG_IS_VIEW (view), NULL);

	/* walk the list in reverse */
	list = g_slist_reverse (g_slist_copy (view->children));
	for (ptr = list; ptr != NULL; ptr = ptr->next) {
		pointed_view = gog_view_get_view_at_point (GOG_VIEW (ptr->data), x, y, obj, tool);
		if (pointed_view != NULL) {
			g_slist_free (list);
			return pointed_view;
		}
	}
	g_slist_free (list);

	if ((current_tool = gog_view_get_tool_at_point (view, x, y, obj))) {
		if (tool != NULL)
			*tool = current_tool;
		return view;
	}

	if (obj && *obj)
		*obj = NULL;
	return NULL;
}

/**
 * gog_view_get_tip_at_point:
 * @view: #GogView
 * @x: x position, in renderer units
 * @y: y position, in renderer units
 *
 * Gets a tip string related to the position as defined by (@x,@y) in @view.
 *
 * Returns: (transfer full) (nullable): the newly allocated tip string if the
 * view class supports that or %NULL.
 **/
char *
gog_view_get_tip_at_point (GogView *view, double x, double y)
{
	GogViewClass *klass;

	g_return_val_if_fail (GOG_IS_VIEW (view), NULL);

	klass = GOG_VIEW_GET_CLASS (view);
	return klass->get_tip_at_point ? klass->get_tip_at_point (view, x, y) : NULL;
}

/**
 * gog_view_get_natural_size:
 * @view: a #GogView
 * @requisition: (out): where to store the natural size
 *
 * Gets the natural size of @view, that is the size it would take without any
 * constraint from its parent.
 *
 * This is not implemented yet: the natural_size method of #GogViewClass is not
 * consulted and @requisition is always set to zero.
 **/
void
gog_view_get_natural_size (GogView *view, GogViewRequisition *requisition)
{
	g_return_if_fail (GOG_IS_VIEW (view));
	g_return_if_fail (requisition != NULL);

	requisition->w = requisition->h = 0.; /*FIXME!!!*/
}
