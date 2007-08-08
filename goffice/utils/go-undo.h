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

	void (*undo) (GOUndo *u, gpointer data);
};

/* Basic operations */
void go_undo_undo (GOUndo *u);
void go_undo_undo_with_data (GOUndo *u, gpointer data);

GOUndo *go_undo_combine (GOUndo *a, GOUndo *b);

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

typedef void (*GOUndoBinaryFunc) (gpointer a, gpointer b, gpointer data);

struct _GOUndoBinary {
	GOUndo base;
	gpointer a, b;
	GOUndoBinaryFunc undo;
	GFreeFunc disposea;
	GFreeFunc disposeb;
};

struct _GOUndoBinaryClass {
	GOUndoClass base;
};

GOUndo *go_undo_binary_new (gpointer a, gpointer b, GOUndoBinaryFunc undo,
			    GFreeFunc fa, GFreeFunc fb);

/* ------------------------------------------------------------------------- */

#define GO_UNDO_UNARY_TYPE  (go_undo_unary_get_type ())
#define GO_UNDO_UNARY(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GO_UNDO_UNARY_TYPE, GOUndoUnary))
#define IS_GO_UNDO_UNARY(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_UNDO_UNARY_TYPE))

GType go_undo_unary_get_type (void);

typedef struct _GOUndoUnary GOUndoUnary;
typedef struct _GOUndoUnaryClass GOUndoUnaryClass;

typedef void (*GOUndoUnaryFunc) (gpointer a, gpointer data);

struct _GOUndoUnary {
	GOUndo base;
	gpointer a;
	GOUndoUnaryFunc undo;
	GFreeFunc disposea;
};

struct _GOUndoUnaryClass {
	GOUndoClass base;
};

GOUndo *go_undo_unary_new (gpointer a, GOUndoUnaryFunc undo, GFreeFunc fa);

/* ------------------------------------------------------------------------- */

G_END_DECLS

#endif
