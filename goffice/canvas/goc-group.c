/*
 * goc-group.c :
 *
 * Copyright (C) 2008-2009 Jean Brefort (jean.brefort@normalesup.org)
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

#include <goffice/goffice-config.h>
#include <goffice/goffice.h>

#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>

/**
 * GocGroupClass:
 * @base: base class.
 **/

static GocItemClass *parent_klass;

struct GocGroupPriv {
	unsigned frozen;
	GPtrArray *children;
};

enum {
	GROUP_PROP_0,
	GROUP_PROP_X,
	GROUP_PROP_Y,
};
/**
 * SECTION:goc-group
 * @short_description: Group item
 *
 * A #GocGroup is a #GocItem which just contains other items.
 *
 * The contents
 * of the canvas are stored as a tree where #GocGroup items are branches and
 * other items are leafs.
 **/
static void
goc_group_set_property (GObject *gobject, guint param_id,
			GValue const *value, GParamSpec *pspec)
{
	GocGroup *group = GOC_GROUP (gobject);

	switch (param_id) {
	case GROUP_PROP_X:
		group->x = g_value_get_double (value);
		break;

	case GROUP_PROP_Y:
		group->y = g_value_get_double (value);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
	goc_item_bounds_changed (GOC_ITEM (gobject));
}

static void
goc_group_get_property (GObject *gobject, guint param_id,
			GValue *value, GParamSpec *pspec)
{
	GocGroup *group = GOC_GROUP (gobject);

	switch (param_id) {
	case GROUP_PROP_X:
		g_value_set_double (value, group->x);
		break;

	case GROUP_PROP_Y:
		g_value_set_double (value, group->y);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
}

static void
goc_group_update_bounds (GocItem *item)
{
	GocGroup *group = GOC_GROUP (item);
	GPtrArray *children = group->priv->children;
	double x0, y0, x1, y1;
	if (group->priv->frozen)
		return;
	item->x0 = item->y0 = G_MAXDOUBLE;
	item->x1 = item->y1 = -G_MAXDOUBLE;
	if (children->len > 0) {
		unsigned ui;
		for (ui = 0; ui < children->len; ui++) {
			GocItem *child = g_ptr_array_index (children, ui);
			goc_item_get_bounds (child, &x0, &y0, &x1, &y1);
			if (x0 < item->x0)
				item->x0 = x0;
			if (y0 < item->y0)
				item->y0 = y0;
			if (x1 > item->x1)
				item->x1 = x1;
			if (y1 > item->y1)
				item->y1 = y1;
		}
		item->x0 += group->x;
		item->y0 += group->y;
		item->x1 += group->x;
		item->y1 += group->y;
	}
	if (group->clip_path) {
	}
}

static gboolean
goc_group_draw_region (GocItem const *item, cairo_t *cr,
		      double x0, double y0,
		      double x1, double y1)
{
	GocGroup *group = GOC_GROUP (item);
	GPtrArray *children = group->priv->children;
	unsigned ui;
	if (children->len == 0)
		return TRUE;

	cairo_save (cr);
	if (group->clip_path) {
		cairo_translate (cr, group->x , group->y);
		cairo_set_fill_rule (cr, group->clip_rule);
		go_path_to_cairo (group->clip_path, GO_PATH_DIRECTION_FORWARD, cr);
		cairo_clip (cr);
	}
	x0 -= group->x;
	y0 -= group->y;
	x1 -= group->x;
	y1 -= group->y;
	for (ui = 0; ui < children->len; ui++) {
		double x, y, x_, y_;
		GocItem *item = g_ptr_array_index (children, ui);
		if (!goc_item_is_visible (item))
			continue;

		goc_item_get_bounds (item, &x, &y, &x_, &y_);
		if (x <= x1 && x_ >= x0 && y <= y1 && y_ >= y0) {
			if (!goc_item_draw_region (item, cr, x0, y0, x1, y1))
				goc_item_draw (item, cr);
		}
	}
	cairo_restore (cr);
	return TRUE;
}

/* we just need the distance method to know if an event occured on an item
 so we don't need to know exact distances when they are large enough, to avoid
 recalculate a lot of complex distnaces and to optimize, everything more than
 some thershold (in pixels) will be considered at infinite */

#define GOC_THRESHOLD   10 /* 10 pixels should be enough */

static double
goc_group_distance (GocItem *item, double x, double y, GocItem **near_item)
{
	GocGroup *group = GOC_GROUP (item);
	GPtrArray *children = group->priv->children;
	double result = G_MAXDOUBLE, dist;
	unsigned ui;
	GocItem *cur_item;
	double th = GOC_THRESHOLD / item->canvas->pixels_per_unit;
	x -= group->x;
	y -= group->y;
	for (ui = children->len; ui-- > 0; ) {
		GocItem *it = g_ptr_array_index (children, ui);
		if (!it->visible || it->x0 > x + th || it->x1 < x - th
		    || it->y0 > y + th || it->y1 < y - th)
			continue;
		dist = goc_item_distance (it, x, y, &cur_item);
		if (dist < result) {
			*near_item = cur_item;
			result = dist;
		}
		if (result == 0.)
			break;
	}
	/* check if the click is not outside the clipping region */
	if (group->clip_path) {
	}
	return result;
}

static void
goc_group_realize (GocItem *item)
{
	GocGroup *group = GOC_GROUP (item);
	GPtrArray *children = group->priv->children;
	unsigned ui;

	for (ui = 0; ui < children->len; ui++) {
		GocItem *child = g_ptr_array_index (children, ui);
		_goc_item_realize (child);
	}

	parent_klass->realize (item);
}

static void
goc_group_unrealize (GocItem *item)
{
	GocGroup *group = GOC_GROUP (item);
	GPtrArray *children = group->priv->children;
	unsigned ui;

	parent_klass->unrealize (item);

	for (ui = 0; ui < children->len; ui++) {
		GocItem *child = g_ptr_array_index (children, ui);
		_goc_item_unrealize (child);
	}
}

static void
goc_group_notify_scrolled (GocItem *item)
{
	GocGroup *group = GOC_GROUP (item);
	GPtrArray *children = group->priv->children;
	unsigned ui;

	for (ui = 0; ui < children->len; ui++) {
		GocItem *child = g_ptr_array_index (children, ui);
		GocItemClass *klass = GOC_ITEM_GET_CLASS (child);
		if (klass->notify_scrolled)
			klass->notify_scrolled (child);
	}
}

static void
goc_group_dispose (GObject *obj)
{
	GocGroup *group = GOC_GROUP (obj);
	goc_group_clear (group);
	((GObjectClass*)parent_klass)->dispose (obj);
}

static void
goc_group_finalize (GObject *obj)
{
	GocGroup *group = GOC_GROUP (obj);
	g_ptr_array_free (group->priv->children, TRUE);
	g_free (group->priv);
	((GObjectClass*)parent_klass)->finalize (obj);
}

static void
goc_group_copy (GocItem *dest, GocItem *source)
{
	GocGroup *src = GOC_GROUP (source), *dst = GOC_GROUP (dest);
	unsigned ui;

	dst->x = src->x;
	dst->y = src->y;
	dst->clip_path = go_path_copy (src->clip_path);
	dst->clip_rule = src->clip_rule;
	goc_group_freeze (dst, TRUE);
	for (ui = 0; ui < src->priv->children->len; ui++)
		goc_item_duplicate (g_ptr_array_index (src->priv->children, ui), dst);
	goc_group_freeze (dst, FALSE);
}

static void
goc_group_class_init (GocItemClass *item_klass)
{
	GObjectClass *obj_klass = (GObjectClass*) item_klass;
	parent_klass = g_type_class_peek_parent (item_klass);

	obj_klass->get_property = goc_group_get_property;
	obj_klass->set_property = goc_group_set_property;
	obj_klass->dispose = goc_group_dispose;
	obj_klass->finalize = goc_group_finalize;
	g_object_class_install_property (obj_klass, GROUP_PROP_X,
		g_param_spec_double ("x",
			_("x"),
			_("The group horizontal offset"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, GROUP_PROP_Y,
		g_param_spec_double ("y",
			_("y"),
			_("The group vertical offset"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));

	item_klass->draw_region = goc_group_draw_region;
	item_klass->update_bounds = goc_group_update_bounds;
	item_klass->distance = goc_group_distance;
	item_klass->realize = goc_group_realize;
	item_klass->unrealize = goc_group_unrealize;
	item_klass->notify_scrolled = goc_group_notify_scrolled;
	item_klass->copy = goc_group_copy;
}

static void
goc_group_init (GocGroup *group)
{
	group->priv = g_new0 (struct GocGroupPriv, 1);
	group->priv->children = g_ptr_array_new ();
}

GSF_CLASS (GocGroup, goc_group,
	   goc_group_class_init, goc_group_init,
	   GOC_TYPE_ITEM)

/**
 * goc_group_new:
 * @parent: #GocGroup
 *
 * Creates a new #GocGroup as a child of @parent.
 * Returns: (transfer none): the newly created #GocGroup.
 **/
GocGroup*
goc_group_new (GocGroup *parent)
{
	GocGroup *group;

	g_return_val_if_fail (GOC_IS_GROUP (parent), NULL);

	group = GOC_GROUP (g_object_new (GOC_TYPE_GROUP, NULL));
	g_return_val_if_fail (group != NULL, NULL);

	goc_group_add_child (parent, GOC_ITEM (group));

	return group;
}

/**
 * goc_group_clear:
 * @group: #GocGroup
 *
 * Destroys all @group children.
 **/
void
goc_group_clear (GocGroup *group)
{
	GPtrArray *children;

	g_return_if_fail (GOC_IS_GROUP (group));

	goc_group_freeze (group, TRUE);

	children = group->priv->children;
	while (children->len > 0) {
		unsigned len = children->len;
		GocItem *child = g_ptr_array_index (children, len - 1);

		goc_item_destroy (child);

		if (children->len >= len) {
			/* The most likely trigger of this is a dispose
			   method that doesn't chain up to the parent
			   class' dispose.  */
			g_warning ("Trouble clearing child %p from group %p\n",
				   child,
				   group);
			// Brutal:
			g_ptr_array_set_size (children, len - 1);
		}
	}

	goc_group_freeze (group, TRUE);
}

// Provide just enough of the old ->children fields that it can be used
// to test for empty and inspection of only child.
static void
goc_group_fake_xchildren (GocGroup *group)
{
	g_list_free (group->Xchildren);
	group->Xchildren = group->priv->children->len
		? g_list_prepend (NULL, goc_group_get_child (group, 0))
		: NULL;
}


/**
 * goc_group_add_child:
 * @parent: #GocGroup
 * @item: #GocItem
 *
 * Adds @item as a new child to @parent.
 **/
void
goc_group_add_child (GocGroup *parent, GocItem *item)
{
	GocCanvas *old_canvas;

	g_return_if_fail (GOC_IS_GROUP (parent));
	g_return_if_fail (GOC_IS_ITEM (item));

	if (item->parent == parent)
		return;

	/* Remove from current group.  */
	if (item->parent != NULL)
		goc_group_remove_child (item->parent, item);

	old_canvas = item->canvas;

	/* Insert into new group.  */
	g_ptr_array_add (parent->priv->children, item);
	item->parent = parent;
	item->canvas = parent->base.canvas;

	goc_group_fake_xchildren (parent);

	/* Notify of changes.  */
	if (old_canvas && item->canvas != old_canvas)
		_goc_canvas_remove_item (old_canvas, item);
	g_object_notify (G_OBJECT (item), "parent");
	if (item->canvas != old_canvas)
		g_object_notify (G_OBJECT (item), "canvas");

	if (GOC_ITEM (parent)->realized)
		_goc_item_realize (item);
	goc_item_bounds_changed (GOC_ITEM (parent));
}

/**
 * goc_group_remove_child:
 * @parent: #GocGroup
 * @item: #GocItem
 *
 * Removes @item from @parent. This function will fail if @item is not a
 * child of @parent.
 **/
void
goc_group_remove_child (GocGroup *parent, GocItem *item)
{
	int n;

	g_return_if_fail (GOC_IS_GROUP (parent));
	g_return_if_fail (GOC_IS_ITEM (item));
	g_return_if_fail (item->parent == parent);

	if (item->canvas)
		_goc_canvas_remove_item (item->canvas, item);
	if (GOC_ITEM (parent)->realized)
		_goc_item_unrealize (item);
	n = goc_group_find_child (parent, item);
	g_ptr_array_remove_index (parent->priv->children, n);
	item->parent = NULL;
	item->canvas = NULL;
	goc_group_fake_xchildren (parent);
	g_object_notify (G_OBJECT (item), "parent");
	g_object_notify (G_OBJECT (item), "canvas");
	goc_item_bounds_changed (GOC_ITEM (parent));
}

/**
 * goc_group_get_children:
 * @group: #GocGroup
 *
 * Returns: (transfer container) (element-type GocItem): An array of
 * the items in @group.
 **/
GPtrArray *
goc_group_get_children (GocGroup *group)
{
	g_return_val_if_fail (GOC_IS_GROUP (group), NULL);

	g_ptr_array_ref (group->priv->children);
	return group->priv->children;
}

/**
 * goc_group_get_child:
 * @group: #GocGroup
 * @n: number
 *
 * Returns: (transfer none) (nullable): The @n'th item, zero-bases, in the
 * group or %NULL if @n is too big.
 **/
GocItem *
goc_group_get_child (GocGroup *group, unsigned n)
{
	g_return_val_if_fail (GOC_IS_GROUP (group), NULL);

	return n >= group->priv->children->len
		? NULL
		: g_ptr_array_index (group->priv->children, n);
}


/**
 * goc_group_find_child:
 * @group: #GocGroup
 * @item: #GocItem
 *
 * Returns: The index of @item in @group, or -1 if @item is not in @group.
 **/
int
goc_group_find_child (GocGroup *group, GocItem *item)
{
	unsigned ui;
	GPtrArray *children = group->priv->children;

	if (item->parent != group)
		return -1;

	if (children->len > 1 &&
	    item == g_ptr_array_index (children, children->len - 1)) {
		// Very common case for large groups
		return children->len - 1;
	}

	for (ui = 0; ui < children->len; ui++) {
		if (item == g_ptr_array_index (children, ui))
			return ui;
	}

	g_warning ("Item not in group?");
	return -1;
}


/**
 * goc_group_adjust_bounds:
 * @group: #GocGroup
 * @x0: first horizontal coordinate
 * @y0: first vertical coordinate
 * @x1: last horizontal coordinate
 * @y1: last vertical coordinate
 *
 * Adds @group horizontal offset to @x0 and @x1, and vertical offset to @y0
 * and @y1. This function is called recursively so that when returning @x0,
 * @y0, @x1, and @y1 are absolute coordinates in canvas space,
 **/
void
goc_group_adjust_bounds (GocGroup const *group, double *x0, double *y0, double *x1, double *y1)
{
	GocGroup *parent;
	g_return_if_fail (GOC_IS_GROUP (group));
	*x0 += group->x;
	*y0 += group->y;
	*x1 += group->x;
	*y1 += group->y;
	parent = GOC_ITEM (group)->parent;
	if (parent)
		goc_group_adjust_bounds (parent, x0, y0, x1, y1);
}

/**
 * goc_group_adjust_coords:
 * @group: #GocGroup
 * @x: horizontal coordinate
 * @y: vertical coordinate
 *
 * Adds @group horizontal offset to @x0, and vertical offset to @y0.
 * This function is called recursively so that when returning @x0 and
 * @y0 are absolute coordinates in canvas space,
 **/
void
goc_group_adjust_coords (GocGroup const *group, double *x, double *y)
{
	GocGroup *parent;
	g_return_if_fail (GOC_IS_GROUP (group));
	*x += group->x;
	*y += group->y;
	parent = GOC_ITEM (group)->parent;
	if (parent)
		goc_group_adjust_coords (parent, x, y);
}

/**
 * goc_group_cairo_transform: (skip)
 * @group: #GocGroup
 * @cr: #cairo_t
 * @x: horizontal coordinate
 * @y: vertical coordinate
 *
 * Translates @cr current context so that operations start at (@x,@y), which
 * are @group relative coordinates, and is scaled according to the containing
 * #GocCanvas current scale (see goc_canvas_get_pixels_per_unit()). The
 * translation takes all @group ancestors into account.
 *
 * This function does not call cairo_save().
 **/
void
goc_group_cairo_transform (GocGroup const *group, cairo_t *cr, double x, double y)
{
	GocGroup *parent;

	g_return_if_fail (GOC_IS_GROUP (group));
	parent = GOC_ITEM (group)->parent;
	if (parent)
		goc_group_cairo_transform (parent, cr, x + group->x, y + group->y);
	else {
		GocCanvas *canvas = GOC_ITEM (group)->canvas;
		if (canvas) {
			if (canvas->direction == GOC_DIRECTION_RTL)
				cairo_translate (cr, canvas->width / canvas->pixels_per_unit - (x - canvas->scroll_x1), y - canvas->scroll_y1);
			else
				cairo_translate (cr, x - canvas->scroll_x1, y - canvas->scroll_y1);
		} else
			cairo_translate (cr, x, y);
	}
}

/**
 * goc_group_set_clip_path: (skip)
 * @group: #GocGroup
 * @clip_path: #GOPath
 * @clip_rule: #cairo_fill_rule_t
 *
 * Clips the drawing inside @path.
 */
void
goc_group_set_clip_path (GocGroup *group, GOPath *clip_path, cairo_fill_rule_t clip_rule)
{
	g_return_if_fail (GOC_IS_GROUP (group));
	group->clip_path = clip_path;
	group->clip_rule = clip_rule;
	goc_item_bounds_changed (GOC_ITEM (group));
}

void
goc_group_freeze (GocGroup *group, gboolean freeze)
{
	g_return_if_fail (GOC_IS_GROUP (group));

	if (freeze) {
		group->priv->frozen++;
	} else {
		group->priv->frozen--;
		if (!group->priv->frozen)
			goc_group_update_bounds ((GocItem*) group);
	}
}
