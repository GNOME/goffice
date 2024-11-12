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
#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

#ifdef GOFFICE_WITH_GTK
static void cb_hierarchy_changed (const GocItem *item);
#endif


/**
 * SECTION:goc-item
 * @short_description: Base canvas item.
 *
 * #GocItem is the virtual base object for canvas items.
**/

/**
 * _GocItem:
 * @base: the parent object.
 * @canvas: the canvas in which the item is displayed.
 * @parent: the parent item.
 * @cached_bounds: whether bounds have been cached in @x0, @y0, @x1 and @y1.
 * @visible: whether the item is visible or hidden. A visible item needs to lie
 * in the visible region of the canvas to be really visible.
 * @realized: whether the item is realized.
 * @x0: the lowest horizontal bound of the item.
 * @y0: the lowest vertical bound of the item.
 * @x1: the highest horizontal bound of the item.
 * @y1: the highest vertical bound of the item.
 * @op: the #cairo_operator_t to use when drawing the item.
 * @transform: the #cairo_matrix_t to apply to the item.
 * @transformed: whether @transform is not unity.
 * @priv: private data.
 *
 * <para>
 * #GocItem contains the following fields:
 * <informaltable pgwide="1" frame="none" role="struct">
 * <tgroup cols="2"><colspec colwidth="2*"/><colspec colwidth="8*"/>
 * <tbody>
 * <row>
 * <entry>double x0;</entry>
 * <entry>The lowest horizontal bound of the item.</entry>
 * </row>
 * <row>
 * <entry>double y0;</entry>
 * <entry>The lowest vertical bound of the item.</entry>
 * </row>
 * <row>
 * <entry>double x1;</entry>
 * <entry>The highest horizontal bound of the item.</entry>
 * </row>
 * <row>
 * <entry>double y1;</entry>
 * <entry>The highest vertical bound of the item.</entry>
 * </row>
 * </tbody></tgroup></informaltable>
 * </para>
 *
 * The various fields should not be accessed directly except from children
 * objects which must update @x0, @y0, @x1, and @y1 from their #update_bounds
 * method. All other fields are read-only and not documented.
 **/

enum {
	ITEM_PROP_0,
	ITEM_PROP_CANVAS,
	ITEM_PROP_PARENT
};

/**
 * GocItemClass:
 * @base: the parent class
 * @distance: returns the distance between the item and the point defined by
 * @x and @y. When the distance is larger than a few pixels, the result is not
 * relevant, so an approximate value or even G_MAXDOUBLE might be returned.
 * @draw: draws the item to the cairo context.
 * @draw_region: draws the item in the region defined by @x0, @y0, @x1 and @y1.
 * Should return TRUE when successfull. If FALSE is returned, @draw will be
 * called. There is no need to implement both methods for an item. Large items
 * should implement @draw_region.
 * @update_bounds: updates the bounds stored in #GocItem as fields #x0, #y0,
 * #x1,and #y1.
 * @button_pressed: callback for a button press event.
 * @button2_pressed: callback for a double click event.
 * @button_released: callback for a button release event.
 * @motion: callback for a motion event.
 * @enter_notify: callback for an enter notify event.
 * @leave_notify: callback for a leave notify event.
 * @realize: callback for a realizes event.
 * @unrealize: callback for an unrealize event.
 * @key_pressed: callback for a key press event.
 * @key_released: callback for a key release event.
 * @notify_scrolled: callback for a notify scrolled event. This is useful to
 * reposition children of the GtkLayout parent of the canvas to their new
 * position.
 * @get_window: returns the #GdkWindow for the item if any.
 * @copy: method for copying an item.
 *
 * The base class for all canvas items.
 **/

static GObjectClass *item_parent_class;

static GParamSpecPool *style_property_spec_pool;
static GQuark quark_property_parser;
static GQuark quark_style_context;

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

#ifdef GOFFICE_WITH_GTK
static gboolean
goc_item_key_pressed (GocItem *item, GdkEventKey* ev)
{
	return (item->parent)?
		GOC_ITEM_GET_CLASS (item->parent)->key_pressed (GOC_ITEM (item->parent), ev):
		FALSE;
}

static gboolean
goc_item_key_released (GocItem *item, GdkEventKey* ev)
{
	return (item->parent)?
		GOC_ITEM_GET_CLASS (item->parent)->key_released (GOC_ITEM (item->parent), ev):
		FALSE;
}
#endif

static void
goc_item_realize (GocItem *item)
{
	if (item->realized)
		g_warning ("Duplicate realize for %p %s\n",
			   item,
			   g_type_name_from_instance ((GTypeInstance*)item));
	else
		item->realized = TRUE;
}

static void
goc_item_unrealize (GocItem *item)
{
	if (item->realized)
		item->realized = FALSE;
	else
		g_warning ("Duplicate unrealize for %p %s\n",
			   item,
			   g_type_name_from_instance ((GTypeInstance*)item));
}

static void
goc_item_dispose (GObject *object)
{
	GocItem *item = GOC_ITEM (object);
#ifdef GOFFICE_WITH_GTK
	GtkStyleContext *context;
#endif

	if (item->canvas) {
		item->cached_bounds = TRUE; /* avoids a call to update_bounds */
		goc_item_invalidate (item);
	}

	if (item->parent != NULL)
		goc_group_remove_child (item->parent, item);

#ifdef GOFFICE_WITH_GTK
	context = g_object_get_qdata (G_OBJECT (item), quark_style_context);
	if (context) {
		g_signal_handlers_disconnect_by_func
			(object, G_CALLBACK (cb_hierarchy_changed), NULL);
		gtk_style_context_set_parent (context, NULL);
		g_object_set_qdata (object, quark_style_context, NULL);
	}
#endif

	item_parent_class->dispose (object);
}

static void
goc_item_get_property (GObject *gobject, guint param_id,
		       GValue *value, GParamSpec *pspec)
{
	GocItem *item = GOC_ITEM (gobject);

	switch (param_id) {
	case ITEM_PROP_CANVAS:
		g_value_set_object (value, item->canvas);
		break;

	case ITEM_PROP_PARENT:
		g_value_set_object (value, item->parent);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
}

static void
goc_item_class_init (GocItemClass *item_klass)
{
	GObjectClass *obj_klass = (GObjectClass *) item_klass;
	item_parent_class = g_type_class_peek_parent (item_klass);

	obj_klass->dispose = goc_item_dispose;
	obj_klass->get_property = goc_item_get_property;

	g_object_class_install_property (obj_klass, ITEM_PROP_CANVAS,
		g_param_spec_object ("canvas",
			_("Canvas"),
			_("The canvas object on which the item resides"),
			GOC_TYPE_CANVAS,
			GSF_PARAM_STATIC | G_PARAM_READABLE));
	g_object_class_install_property (obj_klass, ITEM_PROP_PARENT,
		g_param_spec_object ("parent",
			_("Parent"),
			_("The group in which the item resides"),
			GOC_TYPE_GROUP,
			GSF_PARAM_STATIC | G_PARAM_READABLE));

	item_klass->realize = goc_item_realize;
	item_klass->unrealize = goc_item_unrealize;
	item_klass->button_pressed = goc_item_button_pressed;
	item_klass->button2_pressed = goc_item_button2_pressed;
	item_klass->button_released = goc_item_button_released;
	item_klass->motion = goc_item_motion;
	item_klass->enter_notify = goc_item_enter_notify;
	item_klass->leave_notify = goc_item_leave_notify;
#ifdef GOFFICE_WITH_GTK
	item_klass->key_pressed = goc_item_key_pressed;
	item_klass->key_released = goc_item_key_released;
#endif

	style_property_spec_pool = g_param_spec_pool_new (FALSE);
	quark_property_parser = g_quark_from_static_string ("gtk-rc-property-parser");
	quark_style_context = g_quark_from_static_string ("GocItem::style-context");
}

static void
goc_item_init (GocItem *item)
{
	item->visible = TRUE;
	cairo_matrix_init_identity (&item->transform);
}

GSF_CLASS_FULL (GocItem, goc_item, NULL, NULL,
	   goc_item_class_init, NULL, goc_item_init,
	   G_TYPE_OBJECT, G_TYPE_FLAG_ABSTRACT, {})

static void
_goc_item_update_bounds (GocItem *item)
{
	GocItemClass *klass = GOC_ITEM_GET_CLASS (item);
	g_return_if_fail (klass != NULL);

	if (klass->update_bounds)
		klass->update_bounds (item);
	item->cached_bounds = TRUE;
}

/**
 * goc_item_new:
 * @parent: parent #GocGroup for the new item
 * @type: #GType of the new item
 * @first_arg_name: property name or %NULL
 * @...: value for the first property, followed optionally by more
 *  name/value pairs, followed by %NULL
 *
 * Creates a new item of type @type in group @group. Properties can be
 * set just the same way they are in #g_object_new.
 * Returns: (transfer none): the newly created #GocItem.
 **/
GocItem*
goc_item_new (GocGroup *parent, GType type, const gchar *first_arg_name, ...)
{
	GocItem *item;
	va_list args;

	g_return_val_if_fail (GOC_IS_GROUP (parent), NULL);

	va_start (args, first_arg_name);
	item = GOC_ITEM (g_object_new_valist (type, first_arg_name, args));
	va_end (args);
	g_return_val_if_fail ((item != NULL), NULL);

	goc_group_add_child (parent, item);
	goc_item_invalidate (item);

	return item;
}

/**
 * goc_item_destroy:
 * @item: #GocItem
 *
 * Destroys @item, removes it from its parent group and updates the canvas
 * accordingly.
 **/
void
goc_item_destroy (GocItem *item)
{
	g_object_run_dispose (G_OBJECT (item));
	g_object_unref (item);
}

/**
 * goc_item_set:
 * @item: #GocItem
 * @first_arg_name: property name or %NULL
 * @...: value for the first property, followed optionally by more
 *  name/value pairs, followed by %NULL
 *
 * Set properties and updates the canvas. Using #g_object_set instead would
 * set the properties, but not update the canvas.
 **/
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

/**
 * goc_item_distance:
 * @item: #GocItem
 * @x: horizontal position
 * @y: vertical position
 * @near_item: where to store the nearest item
 *
 * Evaluates the distance between the point with canvas coordinates @x,@y
 * and the nearest point of @item. @near_item is set with either @item or
 * its appropriate child.
 * Returns: the evaluated distance.
 **/
double
goc_item_distance (GocItem *item, double x, double y, GocItem **near_item)
{
	GocItemClass *klass = GOC_ITEM_GET_CLASS (item);
	g_return_val_if_fail (klass != NULL, G_MAXDOUBLE);

	return (klass->distance)?
		klass->distance (item, x, y, near_item): G_MAXDOUBLE;
}

/**
 * goc_item_draw:
 * @item: #GocItem
 * @cr: #cairo_t
 *
 * Renders @item using @cr. There is no need to call this function directly.
 * Invalidating the item is the way to go.
 **/
void
goc_item_draw (GocItem const *item, cairo_t *cr)
{
	GocItemClass *klass = GOC_ITEM_GET_CLASS (item);
	g_return_if_fail (klass != NULL);

	if (klass->draw != NULL)
		klass->draw (item, cr);
}

/**
 * goc_item_draw_region:
 * @item: #GocItem
 * @cr: #cairo_t
 * @x0: the lowest horizontal bound of the region to draw
 * @y0: the lowest vertical bound of the region to draw
 * @x1: the highest horizontal bound of the region to draw
 * @y1: the highest vertical bound of the region to draw
 *
 * Renders @item using @cr, limiting all drawings to the region limited by
 * @x0, @y0, @x1, and @y1. If this function returns %FALSE, #goc_item_draw
 * should be called. There is no need to call this function directly.
 * Invalidating the item is the way to go.
 * Returns: %TRUE if successful.
 **/
gboolean
goc_item_draw_region (GocItem const *item, cairo_t *cr,
		      double x0, double y0,
		      double x1, double y1)
{
	GocItemClass *klass = GOC_ITEM_GET_CLASS (item);
	g_return_val_if_fail (klass != NULL, FALSE);

	return (klass->draw_region != NULL)?
		klass->draw_region (item, cr, x0, y0, x1, y1):
		FALSE;
}

static void
goc_item_maybe_invalidate (GocItem *item, gboolean ignore_visibility)
{
	GocGroup const *parent;
	double x0, y0, x1, y1;

	parent = item->parent;
	if (!parent)
		return;

	if (!item->canvas || !goc_canvas_get_realized (item->canvas))
		return;

	if (!ignore_visibility && !item->visible)
		return;

	if (!item->cached_bounds)
		_goc_item_update_bounds (GOC_ITEM (item)); /* don't care about const */
	x0 = item->x0;
	y0 = item->y0;
	x1 = item->x1;
	y1 = item->y1;
	goc_group_adjust_bounds (parent, &x0, &y0, &x1, &y1);
	goc_canvas_invalidate (item->canvas, x0, y0, x1, y1);
}

/**
 * goc_item_invalidate:
 * @item: #GocItem
 *
 * Force a redraw of @item bounding region.
 **/
void
goc_item_invalidate (GocItem *item)
{
	g_return_if_fail (GOC_IS_ITEM (item));

	goc_item_maybe_invalidate (item, FALSE);
}

/**
 * goc_item_set_visible:
 * @item: #GocItem
 * @visible: whether the item should be visible
 *
 * Either hides or shows @item as appropriate..
 **/
void
goc_item_set_visible (GocItem *item, gboolean visible)
{
	g_return_if_fail (GOC_IS_ITEM (item));

	visible = (visible != FALSE);
	if (visible != item->visible) {
		item->visible = visible;
		goc_item_maybe_invalidate (item, TRUE);
	}
}

/**
 * goc_item_show:
 * @item: #GocItem
 *
 * Makes @item visible.
 **/
void
goc_item_show (GocItem *item)
{
	goc_item_set_visible (item, TRUE);
}

/**
 * goc_item_hide:
 * @item: #GocItem
 *
 * Hides @item.
 **/
void
goc_item_hide (GocItem *item)
{
	goc_item_set_visible (item, FALSE);
}

/**
 * goc_item_is_visible:
 * @item: #GocItem
 *
 * Returns: %TRUE if @item is visible.
 **/
gboolean
goc_item_is_visible (GocItem *item)
{
	g_return_val_if_fail (GOC_IS_ITEM (item), FALSE);
	return item->visible;
}

/**
 * goc_item_get_bounds:
 * @item: #GocItem
 * @x0: where to store the lowest horizontal bound
 * @y0: where to store the lowest vertical bound
 * @x1: where to store the highest horizontal bound
 * @y1: where to store the highest vertical bound
 *
 * Retrieves the bounds of @item in canvas coordinates.
 **/
void
goc_item_get_bounds (GocItem const *item, double *x0, double *y0, double *x1, double *y1)
{
	g_return_if_fail (GOC_IS_ITEM (item));
	if (!item->cached_bounds) {
		_goc_item_update_bounds (GOC_ITEM (item)); /* don't care about const */
	}
	*x0 = item->x0;
	*y0 = item->y0;
	*x1 = item->x1;
	*y1 = item->y1;
}

/**
 * goc_item_bounds_changed:
 * @item: #GocItem
 *
 * This function needs to be called each time the bounds of @item change. It
 * is normally called from inside the implementation of items derived classes.
 **/
void
goc_item_bounds_changed (GocItem *item)
{
	g_return_if_fail (GOC_IS_ITEM (item));
	if (item->cached_bounds) {
		GocItem *cur = item;
		goc_item_invalidate (item);
		do {
			cur->cached_bounds = FALSE;
			cur = GOC_ITEM (cur->parent);
		} while (cur);
		return;
	}
	goc_item_invalidate (item);
}

/**
 * goc_item_grab:
 * @item: #GocItem
 *
 * Grabs the item. This function will fail if another item is grabbed.
 **/
void
goc_item_grab (GocItem *item)
{
	if (item == goc_canvas_get_grabbed_item (item->canvas))
		return;
	g_return_if_fail (GOC_IS_ITEM (item));
	goc_canvas_grab_item (item->canvas, item);
}

/**
 * goc_item_ungrab:
 * @item: #GocItem
 *
 * Ungrabs the item. This function will fail if @item is not grabbed.
 **/
void
goc_item_ungrab	(GocItem *item)
{
	g_return_if_fail (item == goc_canvas_get_grabbed_item (item->canvas));
	goc_canvas_ungrab_item (item->canvas);
}

static void
goc_item_reordered (GocItem *item, int n)
{
#ifdef GOFFICE_WITH_GTK
	GocGroup *group = item->parent;
	int cur = goc_group_find_child (group, item);
	while (TRUE) {
		GocItem *child = goc_group_get_child (group, cur);
		GdkWindow *window;
		if (!child)
			break;
		window = goc_item_get_window (child);
		if (window) {
			if (n > 0)
				gdk_window_raise (window);
			else
				gdk_window_lower (window);
		}
		cur += (n > 0 ? +1 : -1);
	}
#endif
}

/**
 * goc_item_raise:
 * @item: #GocItem
 * @n: the rank change
 *
 * Raises @item by @n steps inside its parent #GocGroup (or less if the list
 * is too short) in the item list so that it is displayed nearer the top of
 * the items stack.
 **/
void
goc_item_raise (GocItem *item, int n)
{
	GocGroup *group = item->parent;
	GPtrArray *children = goc_group_get_children (group);
	int len = children->len;
	int ix = goc_group_find_child (group, item);
	if (n > len - 1 - ix) n = len - 1 - ix;
	g_ptr_array_remove_index (children, ix);
	g_ptr_array_insert (children, ix + n, item);
	goc_item_invalidate (item);
	goc_item_reordered (item, n);
	g_ptr_array_unref (children);
}

/**
 * goc_item_lower:
 * @item: #GocItem
 * @n: the rank change
 *
 * Lowers @item by @n steps inside its parent #GocGroup (or less if the list
 * is too short) in the item list so that it is displayed more deeply in the
 * items stack.
 **/
void
goc_item_lower (GocItem *item, int n)
{
	GocGroup *group = item->parent;
	GPtrArray *children = goc_group_get_children (group);
	int ix = goc_group_find_child (group, item);
	if (n > ix) n = ix;

	g_ptr_array_remove_index (children, ix);
	g_ptr_array_insert (children, ix - n, item);
	goc_item_invalidate (item);
	goc_item_reordered (item, -n);
	g_ptr_array_unref (children);
}

/**
 * goc_item_lower_to_bottom:
 * @item: #GocItem
 *
 * Lowers @item to bottom inside its parent #GocGroup so that it will be at
 * least partly hidden by any overlapping item.
 **/
void
goc_item_lower_to_bottom (GocItem *item)
{
	goc_item_lower (item, G_MAXINT);
}

/**
 * goc_item_raise_to_top:
 * @item: #GocItem
 *
 * Raises @item to front so that it becomes the toplevel item inside
 * its parent #GocGroup.
 **/
void
goc_item_raise_to_top (GocItem *item)
{
	goc_item_raise (item, G_MAXINT);
}

void
_goc_item_realize (GocItem *item)
{
	if (!item->realized) {
		GocItemClass *klass = GOC_ITEM_GET_CLASS (item);
		klass->realize (item);
	}
}

void
_goc_item_unrealize (GocItem *item)
{
	if (item->realized) {
		GocItemClass *klass = GOC_ITEM_GET_CLASS (item);
		klass->unrealize (item);
	}
}

/**
 * goc_item_get_parent:
 * @item: #GocItem
 *
 * Returns: (transfer none): The item parent #GocGroup.
 **/
GocGroup *
goc_item_get_parent (GocItem *item)
{
	return item->parent;
}

#ifdef GOFFICE_WITH_GTK
/**
 * goc_item_get_window:
 * @item: #GocItem
 *
 * Returns: (transfer none): The #GdkWindow associated with the item if any or
 * NULL. Only #GocWidget owns a #GdkWindow.
 **/
GdkWindow *
goc_item_get_window (GocItem *item)
{
	GocItemClass *klass = GOC_ITEM_GET_CLASS (item);
	return (klass->get_window)? klass->get_window (item): NULL;
}
#endif

/**
 * goc_item_set_operator: (skip)
 * @item: #GocItem
 * @op: #cairo_operator_t
 *
 * Set the operator used when drawing the item.
 */
void
goc_item_set_operator  (GocItem *item, cairo_operator_t op)
{
	item->op = op;
	goc_item_invalidate (item);
}

/**
 * goc_item_get_operator: (skip)
 * @item: #GocItem
 *
 * Returns: the operator used when drawing the item.
 */
cairo_operator_t
goc_item_get_operator  (GocItem *item)
{
	return item->op;
}

#define matrix_epsilon 1e-12
/**
 * goc_item_set_transform: (skip)
 * @item: #GocItem
 * @m: #cairo_matrix_t
 *
 * Set the matrix used to transform the item.
 */
void
goc_item_set_transform (GocItem *item, cairo_matrix_t *m)
{
	item->transformed = fabs (m->xx - 1.) > matrix_epsilon
						|| fabs (m->xy) > matrix_epsilon
						|| fabs (m->xy) > matrix_epsilon
						|| fabs (m->yx) > matrix_epsilon
						|| fabs (m->yy - 1.) > matrix_epsilon
						|| fabs (m->x0) > matrix_epsilon
						|| fabs (m->y0) > matrix_epsilon;
	if (item->transformed)
		item->transform = *m;
	else
		cairo_matrix_init_identity (&item->transform);
}

void
_goc_item_transform (GocItem const *item, cairo_t *cr, gboolean scaled)
{
	double scale = item->canvas? item->canvas->pixels_per_unit: 1.0;
	cairo_matrix_t m = item->transform, buf,
		sc = {scale, 0., 0., scale, 0., 0.};
	while ((item = GOC_ITEM (item->parent)))
		if (item->transformed) {
			cairo_matrix_multiply (&buf, &m, &item->transform);
			m = buf;
		}
	if (scaled) {
		cairo_matrix_multiply (&buf, &m, &sc);
		m = buf;
	}
	cairo_transform (cr, &m);
}

#ifdef GOFFICE_WITH_GTK

static void
cb_hierarchy_changed (const GocItem *item)
{
	GtkStyleContext *context = goc_item_get_style_context (item);
	GtkStyleContext *pcontext;
	GtkWidgetPath *path;

	if (item->parent) {
		pcontext = goc_item_get_style_context (GOC_ITEM (item->parent));
	} else if (item->canvas &&
		   GOC_ITEM (item->canvas->root) == item) {
		pcontext = gtk_widget_get_style_context (GTK_WIDGET (item->canvas));
	} else {
		pcontext = NULL;
	}
	if (pcontext)
		path = gtk_widget_path_copy (gtk_style_context_get_path (pcontext));
	else
		path = gtk_widget_path_new ();

	gtk_widget_path_append_type (path, G_TYPE_FROM_INSTANCE (item));
#if GTK_CHECK_VERSION(3,20,0)
	gtk_widget_path_iter_set_object_name (path, -1, G_OBJECT_TYPE_NAME (item));
#endif
	gtk_style_context_set_path (context, path);
	gtk_widget_path_free (path);
	gtk_style_context_set_parent (context, pcontext);
}


/**
 * goc_item_get_style_context:
 * @item: #GocItem
 *
 * Returns: (transfer none): The style context to use for the item.
 */
GtkStyleContext *
goc_item_get_style_context (const GocItem *item)
{
	GtkStyleContext *context;
	g_return_val_if_fail (GOC_IS_ITEM (item), NULL);

	context = g_object_get_qdata (G_OBJECT (item), quark_style_context);
	if (!context) {
		context = gtk_style_context_new ();
		g_object_set_qdata_full (G_OBJECT (item),
					 quark_style_context,
					 context,
					 g_object_unref);

		g_signal_connect (G_OBJECT (item),
				  "notify::parent",
				  G_CALLBACK (cb_hierarchy_changed),
				  NULL);
		g_signal_connect (G_OBJECT (item),
				  "notify::canvas",
				  G_CALLBACK (cb_hierarchy_changed),
				  NULL);
		cb_hierarchy_changed (item);
	}

	return context;
}
#endif

/**
 * goc_item_duplicate:
 * @item: #GocItem
 * @parent: #GocGroup
 *
 * Creates a new GocItem identical to @item inside the parent GocGroup if not
 * NULL.
 *
 * Returns: (transfer none): The duplicated item or NULL if the duplication was
 * not possible.
 */
GocItem*
goc_item_duplicate (GocItem *item, GocGroup *parent)
{
	GocItemClass *klass;
	GocItem *ret;

	g_return_val_if_fail (GOC_IS_ITEM (item), NULL);

	klass = GOC_ITEM_GET_CLASS (item);
	if (klass->copy == NULL)
		return NULL;

	ret = GOC_ITEM ((parent)? goc_item_new (parent, G_OBJECT_TYPE (item), NULL):
							  g_object_new (G_OBJECT_TYPE (item), NULL));
	
	klass->copy (ret, item);
	return ret;
}

/**
 * goc_item_copy:
 * @source: #GocItem
 * @dest: #GocItem
 *
 * Copies @source properties to @dest. The two items must be of the same type
 * and their common class needs a @copy member.
 **/
void
goc_item_copy (GocItem *dest, GocItem *source)
{
	GocItemClass *klass = GOC_ITEM_GET_CLASS (source);

	g_return_if_fail (GOC_IS_ITEM (source));
	g_return_if_fail (GOC_IS_ITEM (dest));
	g_return_if_fail (klass == GOC_ITEM_GET_CLASS (dest));
	g_return_if_fail (klass->copy);
	dest->visible = source->visible;
	dest->op = source->op;
	dest->transform = source->transform;
	dest->transformed = source->transformed;
	klass->copy (dest, source);
}
