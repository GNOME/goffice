/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * god-anchor.h: MS Office Graphic Object support
 *
 * Author:
 *    Michael Meeks (michael@ximian.com)
 *    Jody Goldberg (jody@gnome.org)
 *    Christopher James Lahey <clahey@ximian.com>
 *
 * (C) 1998-2003 Michael Meeks, Jody Goldberg, Chris Lahey
 */

#ifndef GOD_ANCHOR_H
#define GOD_ANCHOR_H

#include <glib-object.h>
#include <glib.h>
#include <goffice/utils/go-units.h>

G_BEGIN_DECLS

#define GOD_TYPE_ANCHOR		(god_anchor_get_type ())
#define GOD_ANCHOR(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GOD_TYPE_ANCHOR, GodAnchor))
#define GOD_ANCHOR_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST ((k), GOD_TYPE_ANCHOR, GodAnchorClass))
#define GOD_ANCHOR_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS((o), GOD_TYPE_ANCHOR, GodAnchorClass))
#define GOD_IS_ANCHOR(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOD_TYPE_ANCHOR))
#define GOD_IS_ANCHOR_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GOD_TYPE_ANCHOR))

typedef struct GodAnchorPrivate_ GodAnchorPrivate;

typedef struct {
	GObject parent;
	GodAnchorPrivate *priv;
} GodAnchor;

typedef struct {
	GObjectClass parent_class;

	void (*get_rect)  (GodAnchor *anchor, GORect *rect);
	void (*set_rect)  (GodAnchor *anchor, const GORect *rect);
} GodAnchorClass;

GType      god_anchor_get_type  (void);

GodAnchor *god_anchor_new       (void);
void       god_anchor_get_rect  (GodAnchor    *anchor,
				 GORect       *rect);
void       god_anchor_set_rect  (GodAnchor    *anchor,
				 const GORect *anchor_value);



G_END_DECLS

#endif /* GOD_ANCHOR_H */
