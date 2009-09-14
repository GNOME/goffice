/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * goc-item.c :
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
#include <gtk/gtk.h>
#include <gsf/gsf-impl-utils.h>

/**
 * GocItemClass:
 * @base: the parent class
 * @distance:
 * @draw:
 * @draw_region:
 * @move:
 * @update_bounds:
 * @parent_changed:
 * @get_operator:
 * @button_pressed:
 * @button2_pressed:
 * @button_released:
 * @motion:
 * @enter_notify:
 * @leave_notify:
 * @realize:
 * @unrealize:
 * @key_pressed:
 * @key_released:
 * @notify_scrolled:
 *
 * The base class for all canvas items.
 **/

static GObjectClass *item_parent_class;

static gboolean
goc_item_button_pressed (GocItem *item, int button, double x, double y)
{
	return (item->parent)?
		GOC_ITEM_GET_CLASS (item->parent)->button_pressed (GOC_ITEM (item->parent), button, x, y):
		FALSE;
}

static gboolean
goc_item_button2_pressed (GocItem *item, int button, double x, double y)
{
	return (item->parent)?
		GOC_ITEM_GET_CLASS (item->parent)->button2_pressed (GOC_ITEM (item->parent), button, x, y):
		FALSE;
}

static gboolean
goc_item_button_released (GocItem *item, int button, double x, double y)
{
	return (item->parent)?
		GOC_ITEM_GET_CLASS (item->parent)->button_released (GOC_ITEM (item->parent), button, x, y):
		FALSE;
}

static gboolean
goc_item_motion (GocItem *item, double x, double y)
{
	return (item->parent)?
		GOC_ITEM_GET_CLASS (item->parent)->motion (GOC_ITEM (item->parent), x, y):
		FALSE;
}

static gboolean
goc_item_enter_notify (GocItem *item, double x, double y)
{
	return (item->parent)?
		GOC_ITEM_GET_CLASS (item->parent)->enter_notify (GOC_ITEM (item->parent), x, y):
		FALSE;
}

static gboolean
goc_item_leave_notify (GocItem *item, double x, double y)
{
	return (item->parent)?
		GOC_ITEM_GET_CLASS (item->parent)->leave_notify (GOC_ITEM (item->parent), x, y):
		FALSE;
}

static void
goc_item_dispose (GObject *object)
{
	GocItem *item = GOC_ITEM (object);
	if (item->canvas) {
		if (item->canvas->last_item == item)
			item->canvas->last_item = NULL;
		if (GTK_WIDGET_REALIZED (item->canvas)) {
			item->cached_bounds = TRUE; /* avoids a call to update_bounds */
			goc_item_invalidate (item);
		}
	}

	if (item->parent != NULL)
		goc_group_remove_child (item->parent, item);

	item_parent_class->dispose (object);
}

static void
goc_item_class_init (GocItemClass *item_klass)
{
	GObjectClass *obj_klass = (GObjectClass *) item_klass;
	item_parent_class = g_type_class_peek_parent (item_klass);

	item_klass->button_pressed = goc_item_button_pressed;
	item_klass->button2_pressed = goc_item_button2_pressed;
	item_klass->button_released = goc_item_button_released;
	item_klass->motion = goc_item_motion;
	item_klass->enter_notify = goc_item_enter_notify;
	item_klass->leave_notify = goc_item_leave_notify;

	obj_klass->dispose = goc_item_dispose;
}

static void
goc_item_init (GocItem *item)
{
	item->visible = TRUE;
}

GSF_CLASS (GocItem, goc_item,
	   goc_item_class_init, goc_item_init,
	   G_TYPE_OBJECT)

GocItem*
goc_item_new (GocGroup *group, GType type, const gchar *first_arg_name, ...)
{
	GocItem *item;
	va_list args;

	g_return_val_if_fail (GOC_IS_GROUP (group), NULL);

	item = GOC_ITEM (g_object_new (type, NULL));
	g_return_val_if_fail ((item != NULL), NULL);

	goc_group_add_child (group, item);

	va_start (args, first_arg_name);
	g_object_set_valist (G_OBJECT (item), first_arg_name, args);
	va_end (args);

	if (GTK_WIDGET_REALIZED (item->canvas)) {
		GocItemClass *klass = GOC_ITEM_GET_CLASS (item);
		if (klass->realize)
			klass->realize (item);
		item->realized = TRUE;
	}

	return item;
}

void
goc_item_destroy (GocItem *item)
{
	g_object_run_dispose (G_OBJECT (item));
	g_object_unref (item);
}

void
goc_item_set (GocItem *item, const gchar *first_arg_name, ...)
{
	va_list args;

	goc_item_invalidate (item);

	va_start (args, first_arg_name);
	g_object_set_valist (G_OBJECT (item), first_arg_name, args);
	va_end (args);

	goc_item_invalidate (item);
}

double
goc_item_distance (GocItem *item, double x, double y, GocItem **near_item)
{
	GocItemClass *klass = GOC_ITEM_GET_CLASS (item);
	g_return_val_if_fail (klass != NULL, G_MAXDOUBLE);

	return (klass->distance)?
		klass->distance (item, x, y, near_item): G_MAXDOUBLE;
}

void
goc_item_draw (GocItem const *item, cairo_t *cr)
{
	GocItemClass *klass = GOC_ITEM_GET_CLASS (item);
	g_return_if_fail (klass != NULL);

	if (klass->draw)
		klass->draw (item, cr);
}

gboolean
goc_item_draw_region (GocItem const *item, cairo_t *cr,
		      double x0, double y0,
		      double x1, double y1)
{
	GocItemClass *klass = GOC_ITEM_GET_CLASS (item);
	g_return_val_if_fail (klass != NULL, FALSE);

	return (klass->draw_region)?
		klass->draw_region (item, cr, x0, y0, x1, y1): FALSE;
}

cairo_operator_t
goc_item_get_operator (GocItem *item)
{
	GocItemClass *klass;

	g_return_val_if_fail (GOC_IS_ITEM (item), CAIRO_OPERATOR_OVER);
	klass = GOC_ITEM_GET_CLASS (item);
	g_return_val_if_fail (klass != NULL, CAIRO_OPERATOR_OVER);

	return (klass->get_operator)?
		klass->get_operator (item): CAIRO_OPERATOR_OVER;
}

void
goc_item_move (GocItem *item, double x, double y)
{
	GocItemClass *klass = GOC_ITEM_GET_CLASS (item);
	g_return_if_fail (klass != NULL);

	if (klass->move)
		klass->move (item, x, y);
}

void
goc_item_invalidate (GocItem *item)
{
	GocGroup const *parent;
	double x0, y0, x1, y1;

	g_return_if_fail (GOC_IS_ITEM (item));
	if (!GTK_WIDGET_REALIZED (item->canvas))
		return;

	parent = item->parent;
	if (!item->cached_bounds)
		goc_item_update_bounds (GOC_ITEM (item)); /* don't care about const */
	x0 = item->x0;
	y0 = item->y0;
	x1 = item->x1;
	y1 = item->y1;
	while (parent) {
		goc_group_adjust_bounds (parent, &x0, &y0, &x1, &y1);
		parent = parent->base.parent;
	}
	goc_canvas_invalidate (item->canvas, x0, y0, x1, y1);
}

void
goc_item_show (GocItem *item)
{
	g_return_if_fail (GOC_IS_ITEM (item));
	item->visible = TRUE;
	goc_item_invalidate (item);
}

void
goc_item_hide (GocItem *item)
{
	g_return_if_fail (GOC_IS_ITEM (item));
	item->visible = FALSE;
	goc_item_invalidate (item);
}

gboolean
goc_item_is_visible (GocItem *item)
{
	g_return_val_if_fail (GOC_IS_ITEM (item), FALSE);
	return item->visible;
}

void
goc_item_get_bounds (GocItem const *item, double *x0, double *y0, double *x1, double *y1)
{
	g_return_if_fail (GOC_IS_ITEM (item));
	if (!item->cached_bounds) {
		goc_item_update_bounds (GOC_ITEM (item)); /* don't care about const */
	}
	*x0 = item->x0;
	*y0 = item->y0;
	*x1 = item->x1;
	*y1 = item->y1;
}

void
goc_item_update_bounds (GocItem *item)
{
	GocItemClass *klass = GOC_ITEM_GET_CLASS (item);
	g_return_if_fail (klass != NULL);

	if (klass->update_bounds)
		klass->update_bounds (item);
	item->cached_bounds = TRUE;
}

void
goc_item_bounds_changed (GocItem *item)
{
	g_return_if_fail (GOC_IS_ITEM (item));
	item->cached_bounds = FALSE;
	if (item->parent)
		goc_item_bounds_changed (GOC_ITEM (item->parent));
}

void
goc_item_parent_changed (GocItem *item)
{
	GocItemClass *klass = GOC_ITEM_GET_CLASS (item);
	g_return_if_fail (klass != NULL);

	if (klass->parent_changed)
		klass->parent_changed (item);
}

void
goc_item_grab (GocItem *item)
{
	if (item == goc_canvas_get_grabbed_item (item->canvas))
		return;
	g_return_if_fail (GOC_IS_ITEM (item));
	goc_canvas_grab_item (item->canvas, item);
}

void
goc_item_ungrab	(GocItem *item)
{
	g_return_if_fail (item == goc_canvas_get_grabbed_item (item->canvas));
	goc_canvas_ungrab_item (item->canvas);
}

void
goc_item_raise (GocItem *item, int n)
{
	GList *orig = g_list_find (item->parent->children, item);
	GList *dest = g_list_nth (orig, n + 1);
	if (dest)
		item->parent->children = g_list_insert_before (item->parent->children, dest, item);
	else
		item->parent->children = g_list_append (item->parent->children, item);
	item->parent->children = g_list_remove_link (item->parent->children, orig);
}

void
goc_item_lower (GocItem *item, int n)
{
	GList *orig = g_list_find (item->parent->children, item);
	GList *dest = g_list_nth_prev (orig, n);
	if (dest)
		item->parent->children = g_list_insert_before (item->parent->children, dest, item);
	else
		item->parent->children = g_list_prepend (item->parent->children, item);
	item->parent->children = g_list_remove_link (item->parent->children, orig);
}

void
goc_item_lower_to_bottom (GocItem *item)
{
	g_return_if_fail (item->parent != NULL);
	item->parent->children = g_list_remove (item->parent->children, item);
	item->parent->children = g_list_prepend (item->parent->children, item);
}

void
goc_item_raise_to_top (GocItem *item)
{
	g_return_if_fail (item->parent != NULL);
	item->parent->children = g_list_remove (item->parent->children, item);
	item->parent->children = g_list_append (item->parent->children, item);
}

void
_goc_item_realize (GocItem *item)
{
	if (!item->realized) {
		GocItemClass *klass = GOC_ITEM_GET_CLASS (item);
		if (klass->realize)
			klass->realize (item);
		item->realized = TRUE;
	}
}

void
_goc_item_unrealize (GocItem *item)
{
	if (item->realized) {
		GocItemClass *klass = GOC_ITEM_GET_CLASS (item);
		if (klass->unrealize)
			klass->unrealize (item);
		item->realized = FALSE;
	}
}
