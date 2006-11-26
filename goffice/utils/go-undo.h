#ifndef _GOFFICE_UNDO_H_
#define _GOFFICE_UNDO_H_

#include <glib-object.h>

G_BEGIN_DECLS

/* ------------------------------------------------------------------------- */

#define GO_UNDO_TYPE	    (go_undo_get_type ())
#define GO_UNDO(o)	    (G_TYPE_CHECK_INSTANCE_CAST ((o), GO_UNDO_TYPE, GOUndo))
#define IS_GO_UNDO(o)	    (G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_UNDO_TYPE))

GType go_undo_get_type (void);

typedef struct _GOUndo GOUndo;
typedef struct _GOUndoClass GOUndoClass;

struct _GOUndo {
	GObject base;
};

struct _GOUndoClass {
	GObjectClass	base;

	void (*undo) (GOUndo *u);
};

/* Basic operations */
void go_undo_undo (GOUndo *u);

/* ------------------------------------------------------------------------- */
/* Compound operations.  */

#define GO_UNDO_GROUP_TYPE  (go_undo_group_get_type ())
#define GO_UNDO_GROUP(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GO_UNDO_GROUP_TYPE, GOUndoGroup))
#define IS_GO_UNDO_GROUP(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_UNDO_GROUP_TYPE))

GType go_undo_group_get_type (void);

typedef struct _GOUndoGroup GOUndoGroup;
typedef struct _GOUndoGroupClass GOUndoGroupClass;

struct _GOUndoGroup {
	GOUndo base;
	GPtrArray *undos;
};

struct _GOUndoGroupClass {
	GOUndoClass base;
};

GOUndoGroup *go_undo_group_new (void);
void go_undo_group_add (GOUndoGroup *g, GOUndo *u);

/* ------------------------------------------------------------------------- */

#define GO_UNDO_BINARY_TYPE  (go_undo_binary_get_type ())
#define GO_UNDO_BINARY(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GO_UNDO_BINARY_TYPE, GOUndoBinary))
#define IS_GO_UNDO_BINARY(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_UNDO_BINARY_TYPE))

GType go_undo_binary_get_type (void);

typedef struct _GOUndoBinary GOUndoBinary;
typedef struct _GOUndoBinaryClass GOUndoBinaryClass;

struct _GOUndoBinary {
	GOUndo base;
	gpointer a, b;
	GFunc undo;
	GFreeFunc disposea;
	GFreeFunc disposeb;
};

struct _GOUndoBinaryClass {
	GOUndoClass base;
};

GOUndo *go_undo_binary_new (gpointer a, gpointer b, GFunc undo,
			    GFreeFunc fa, GFreeFunc fb);

GOUndo *go_undo_unary_new (gpointer a, GFreeFunc undo, GFreeFunc fa);

/* ------------------------------------------------------------------------- */

G_END_DECLS

#endif
