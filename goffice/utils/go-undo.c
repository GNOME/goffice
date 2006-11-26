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

void
go_undo_undo (GOUndo *u)
{
	GOUndoClass *uc;

	g_return_if_fail (IS_GO_UNDO (u));

	uc = G_TYPE_INSTANCE_GET_CLASS (u, GO_UNDO_TYPE, GOUndoClass);
	uc->undo (u);
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
go_undo_group_undo (GOUndo *u)
{
	GOUndoGroup *ug = (GOUndoGroup *)u;
	unsigned i;

	for (i = ug->undos->len; i-- > 0; ) {
		GOUndo *u = g_ptr_array_index (ug->undos, i);
		go_undo_undo (u);
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
go_undo_binary_undo (GOUndo *u)
{
	GOUndoBinary *ua = (GOUndoBinary *)u;

	ua->undo (ua->a, ua->b);
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
go_undo_binary_new (gpointer a, gpointer b, GFunc undo,
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

/* We fake this, but we can fix it later if we need to.  */
GOUndo *
go_undo_unary_new (gpointer a, GFreeFunc undo, GFreeFunc fa)
{
	return go_undo_binary_new (a, NULL, (GFunc)undo, fa, NULL);
}

/* ------------------------------------------------------------------------- */
