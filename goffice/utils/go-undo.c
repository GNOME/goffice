/*
 * go-undo.c: low-level undo objects.
 *
 * Authors:
 *   Morten Welinder (terra@gnome.org)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) version 3.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA.
 */

#include <goffice/goffice-config.h>
#include <goffice/utils/go-undo.h>
#include <gsf/gsf-impl-utils.h>

/**
 * GOUndoClass:
 * @base: base class.
 * @undo: undo.
};
 **/

/**
 * GOUndoBinaryClass:
 * @base: base class.
 **/

/**
 * GOUndoGroupClass:
 * @base: base class.
 **/

/**
 * GOUndoUnaryClass:
 * @base: base class.
 **/

/* ------------------------------------------------------------------------- */

GSF_CLASS (GOUndo, go_undo, NULL, NULL, G_TYPE_OBJECT)

/**
 * go_undo_undo_with_data:
 * @u: undo object
 * @data: user data
 *
 * Execute the stored undo operation.  @data is supplied to the undo
 * operation as an extra argument.  It is meant not to affect the undo
 * operation in any way, but rather supply a context through which
 * progress and errors can be reported.
 */
void
go_undo_undo_with_data (GOUndo *u, gpointer data)
{
	GOUndoClass *uc;

	g_return_if_fail (GO_IS_UNDO (u));

	uc = G_TYPE_INSTANCE_GET_CLASS (u, GO_TYPE_UNDO, GOUndoClass);
	uc->undo (u, data);
}

/**
 * go_undo_undo:
 * @u: undo object
 *
 * Execute the stored undo operation.
 */
void
go_undo_undo (GOUndo *u)
{
	go_undo_undo_with_data (u, NULL);
}

/**
 * go_undo_combine:
 * @a: (transfer full) (nullable): first undo operation
 * @b: (transfer full) (nullable): last undo operation
 *
 * This function takes ownership of the argument references and gives ownership
 * of the result to the caller.  Either argument may be %NULL in which case the
 * other is returned.
 *
 * Returns: (transfer full) (nullable): the combination of two undo operations.
 **/
GOUndo *
go_undo_combine (GOUndo *a, GOUndo *b)
{
	g_return_val_if_fail (a == NULL || GO_IS_UNDO (a), NULL);
	g_return_val_if_fail (b == NULL || GO_IS_UNDO (b), NULL);

	if (!a)
		return b;
	else if (!b)
		return a;
	else if (GO_IS_UNDO_GROUP (a)) {
		go_undo_group_add (GO_UNDO_GROUP (a), b);
		return a;
	} else {
		GOUndoGroup *g = go_undo_group_new ();
		go_undo_group_add (g, a);
		go_undo_group_add (g, b);
		return GO_UNDO (g);
	}
}

/* ------------------------------------------------------------------------- */

static GObjectClass *go_undo_group_parent_class;

static void
go_undo_group_init (GObject *o)
{
	GOUndoGroup *ug = (GOUndoGroup *)o;
	ug->undos = g_ptr_array_new ();
}

static void
go_undo_group_finalize (GObject *o)
{
	GOUndoGroup *ug = (GOUndoGroup *)o;
	unsigned i;

	for (i = ug->undos->len; i-- > 0; ) {
		GOUndo *u = g_ptr_array_index (ug->undos, i);
		g_object_unref (u);
	}
	g_ptr_array_free (ug->undos, TRUE);

	go_undo_group_parent_class->finalize (o);
}

static void
go_undo_group_undo (GOUndo *u, gpointer data)
{
	GOUndoGroup *ug = (GOUndoGroup *)u;
	unsigned i;

	for (i = ug->undos->len; i-- > 0; ) {
		GOUndo *u_ = g_ptr_array_index (ug->undos, i);
		go_undo_undo_with_data (u_, data);
	}
}

static void
go_undo_group_class_init (GObjectClass *gobject_class)
{
	GOUndoClass *uclass = (GOUndoClass *)gobject_class;

	go_undo_group_parent_class = g_type_class_peek_parent (gobject_class);

	gobject_class->finalize = go_undo_group_finalize;
	uclass->undo = go_undo_group_undo;
}

GSF_CLASS (GOUndoGroup, go_undo_group,
	   go_undo_group_class_init, go_undo_group_init, GO_TYPE_UNDO)

/**
 * go_undo_group_new:
 *
 * This function creates a new undo group for compounding undo objects.
 *
 * Returns: a new, empty undo group.
 **/
GOUndoGroup *
go_undo_group_new (void)
{
	return g_object_new (GO_TYPE_UNDO_GROUP, NULL);
}

/**
 * go_undo_group_add:
 * @g: undo group
 * @u: (transfer full): undo object
 *
 * This function adds @u to @g.
 **/
void
go_undo_group_add (GOUndoGroup *g, GOUndo *u)
{
	g_return_if_fail (GO_IS_UNDO_GROUP (g));
	g_return_if_fail (GO_IS_UNDO (u));
	g_ptr_array_add (g->undos, u);
}

/* ------------------------------------------------------------------------- */

static GObjectClass *go_undo_binary_parent_class;

static void
go_undo_binary_finalize (GObject *o)
{
	GOUndoBinary *ua = (GOUndoBinary *)o;

	if (ua->disposea)
		ua->disposea (ua->a);
	if (ua->disposeb)
		ua->disposeb (ua->b);

	G_OBJECT_CLASS (go_undo_binary_parent_class)->finalize (o);
}

static void
go_undo_binary_undo (GOUndo *u, gpointer data)
{
	GOUndoBinary *ua = (GOUndoBinary *)u;

	ua->undo (ua->a, ua->b, data);
}

static void
go_undo_binary_class_init (GObjectClass *gobject_class)
{
	GOUndoClass *uclass = (GOUndoClass *)gobject_class;

	go_undo_binary_parent_class = g_type_class_peek_parent (gobject_class);

	gobject_class->finalize = go_undo_binary_finalize;
	uclass->undo = go_undo_binary_undo;
}


GSF_CLASS (GOUndoBinary, go_undo_binary,
	   go_undo_binary_class_init, NULL, GO_TYPE_UNDO)

/**
 * go_undo_binary_new:
 * @a: first argument for undo operation
 * @b: second argument for undo operation
 * @undo: (scope async): function to call with arguments @a and @b for undo.
 * @fa: (scope async): optional function to free @a.
 * @fb: (scope async): optional function to free @b.
 *
 * This function creates a new undo object for undo operations of two
 * arguments.  (In addition, an undo-time argument is added for context.)
 *
 * Returns: (transfer full): a new undo object.
 **/
GOUndo *
go_undo_binary_new (gpointer a, gpointer b, GOUndoBinaryFunc undo,
		    GFreeFunc fa, GFreeFunc fb)
{
	GOUndoBinary *ua = g_object_new (GO_TYPE_UNDO_BINARY, NULL);
	ua->a = a;
	ua->b = b;
	ua->undo = undo;
	ua->disposea = fa;
	ua->disposeb = fb;
	return (GOUndo *)ua;
}

/* ------------------------------------------------------------------------- */

static GObjectClass *go_undo_unary_parent_class;

static void
go_undo_unary_finalize (GObject *o)
{
	GOUndoUnary *ua = (GOUndoUnary *)o;

	if (ua->disposea)
		ua->disposea (ua->a);

	G_OBJECT_CLASS (go_undo_unary_parent_class)->finalize (o);
}

static void
go_undo_unary_undo (GOUndo *u, gpointer data)
{
	GOUndoUnary *ua = (GOUndoUnary *)u;

	ua->undo (ua->a, data);
}

static void
go_undo_unary_class_init (GObjectClass *gobject_class)
{
	GOUndoClass *uclass = (GOUndoClass *)gobject_class;

	go_undo_unary_parent_class = g_type_class_peek_parent (gobject_class);

	gobject_class->finalize = go_undo_unary_finalize;
	uclass->undo = go_undo_unary_undo;
}


GSF_CLASS (GOUndoUnary, go_undo_unary,
	   go_undo_unary_class_init, NULL, GO_TYPE_UNDO)

/**
 * go_undo_unary_new:
 * @a: argument for undo operation
 * @undo: (scope async): function to call with argument @a for undo.
 * @fa: (scope async): optional function to free @a.
 *
 * This function creates a new undo object for undo operations of one
 * argument.  (In addition, an undo-time argument is added for context.)
 *
 * Returns: (transfer full): a new undo object.
 **/
GOUndo *
go_undo_unary_new (gpointer a, GOUndoUnaryFunc undo, GFreeFunc fa)
{
	GOUndoUnary *ua = g_object_new (GO_TYPE_UNDO_UNARY, NULL);
	ua->a = a;
	ua->undo = undo;
	ua->disposea = fa;
	return (GOUndo *)ua;
}

/* ------------------------------------------------------------------------- */
