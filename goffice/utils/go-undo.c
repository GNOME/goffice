/*
 * go-undo.c: low-level undo objects.
 *
 * Authors:
 *   Morten Welinder (terra@gnome.org)
 */

#include <goffice/goffice-config.h>
#include <goffice/utils/go-undo.h>
#include <gsf/gsf-impl-utils.h>

/* ------------------------------------------------------------------------- */

GSF_CLASS (GOUndo, go_undo, NULL, NULL, G_TYPE_OBJECT)

/*
 * Execute the stored undo operation.  Note: the supplied data item is
 * supplied to the undo operation.  It is meant not to affect the undo
 * operation in any way, but rather supply a context through which
 * progress and errors can be reported.
 */
void
go_undo_undo_with_data (GOUndo *u, gpointer data)
{
	GOUndoClass *uc;

	g_return_if_fail (IS_GO_UNDO (u));

	uc = G_TYPE_INSTANCE_GET_CLASS (u, GO_UNDO_TYPE, GOUndoClass);
	uc->undo (u, data);
}

void
go_undo_undo (GOUndo *u)
{
	go_undo_undo_with_data (u, NULL);
}

/**
 * go_undo_combine:
 * @a: optional first undo operation
 * @b: optional last undo operation
 *
 * Combines two undo operations and returns the combination.  This function
 * takes ownership of the argument references and gives ownership of the
 * result to the caller.  Either argument may be NULL in which case the
 * other is returned.
 */
GOUndo *
go_undo_combine (GOUndo *a, GOUndo *b)
{
	g_return_val_if_fail (a == NULL || IS_GO_UNDO (a), NULL);
	g_return_val_if_fail (b == NULL || IS_GO_UNDO (b), NULL);

	if (!a)
		return b;
	else if (!b)
		return a;
	else if (IS_GO_UNDO_GROUP (a)) {
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
		GOUndo *u = g_ptr_array_index (ug->undos, i);
		go_undo_undo_with_data (u, data);
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
	   go_undo_group_class_init, go_undo_group_init, GO_UNDO_TYPE)

GOUndoGroup *
go_undo_group_new (void)
{
	return g_object_new (GO_UNDO_GROUP_TYPE, NULL);
}

/* Takes ownership of u. */
void
go_undo_group_add (GOUndoGroup *ug, GOUndo *u)
{
	g_return_if_fail (IS_GO_UNDO_GROUP (ug));
	g_return_if_fail (IS_GO_UNDO (u));
	g_ptr_array_add (ug->undos, u);
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
	   go_undo_binary_class_init, NULL, GO_UNDO_TYPE)

GOUndo *
go_undo_binary_new (gpointer a, gpointer b, GOUndoBinaryFunc undo,
		    GFreeFunc fa, GFreeFunc fb)
{
	GOUndoBinary *ua = g_object_new (GO_UNDO_BINARY_TYPE, NULL);
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
	   go_undo_unary_class_init, NULL, GO_UNDO_TYPE)

GOUndo *
go_undo_unary_new (gpointer a, GOUndoUnaryFunc undo, GFreeFunc fa)
{
	GOUndoUnary *ua = g_object_new (GO_UNDO_UNARY_TYPE, NULL);
	ua->a = a;
	ua->undo = undo;
	ua->disposea = fa;
	return (GOUndo *)ua;
}

/* ------------------------------------------------------------------------- */
