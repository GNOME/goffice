/*
 * gog-object.h :
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
#ifndef GOG_OBJECT_H
#define GOG_OBJECT_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

typedef enum {
	GOG_OBJECT_NAME_BY_ROLE	 = 1,
	GOG_OBJECT_NAME_BY_TYPE  = 2,
	GOG_OBJECT_NAME_MANUALLY = 3
} GogObjectNamingConv;

typedef enum {
	GOG_MANUAL_SIZE_AUTO,   /* auto size, can't be changed */
	GOG_MANUAL_SIZE_WIDTH,  /*=1*/
	GOG_MANUAL_SIZE_HEIGHT, /*=2*/
	GOG_MANUAL_SIZE_FULL    /*=3 or GOG_MANUAL_SIZE_WIDTH | GOG_MANUAL_SIZE_HEIGHT */
} GogManualSizeMode;

struct _GogObjectRole {
	char const *id;	/* for persistence */
	char const *is_a_typename;
	unsigned    int priority;

	guint32		  	allowable_positions;
	GogObjectPosition 	default_position;
	GogObjectNamingConv	naming_conv;

	gboolean   (*can_add)	  (GogObject const *parent);
	gboolean   (*can_remove)  (GogObject const *child);
	GogObject *(*allocate)    (GogObject *parent);
	void	   (*post_add)    (GogObject *parent, GogObject *child);
	void       (*pre_remove)  (GogObject *parent, GogObject *child);
	void       (*post_remove) (GogObject *parent, GogObject *child);

	union { /* allow people to tack some useful tidbits on the end */
		int		i;
		gpointer	p;
	} user;
};
GType gog_object_role_get_type (void);

struct _GogObject {
	GObject		 base;

	unsigned int	 id;
	char		*user_name;	/* user assigned, NULL will fall back to system generated */
	char		*auto_name;	/* system generated, in current locale */

	GogObjectRole const *role;

	GogObject	*parent;
	GSList		*children;

	GogObjectPosition  position;
	GogViewAllocation  manual_position;

	unsigned needs_update : 1;
	unsigned being_updated : 1;
	unsigned explicitly_typed_role : 1; /* did we create it automaticly */
	unsigned invisible : 1;

	void		*_priv; /* for future use */
};

typedef struct {
	GObjectClass	base;

	GHashTable *roles;
	GType	    view_type;

	unsigned int use_parent_as_proxy : 1; /* when we change, pretend it was our parent */
	unsigned int roles_allocated:1;
	/*< public >*/

	/* Virtuals */
	void	     (*update)		(GogObject *obj);
	void	     (*parent_changed)	(GogObject *obj, gboolean was_set);
	char const  *(*type_name)	(GogObject const *obj);
	void	     (*populate_editor)	(GogObject *obj,
					 GOEditor *editor,
					 GogDataAllocator *dalloc,
					 GOCmdContext *cc);
	void	     (*document_changed)(GogObject *obj, GODoc *doc);
	GogManualSizeMode (*get_manual_size_mode) (GogObject *obj);
	 /*< private >*/
	void	     (*reserved1)		(GogObject *view);
	void	     (*reserved2)		(GogObject *view);
	/*< public >*/

	/* signals */
	void (*changed)		(GogObject *obj, gboolean size);
	void (*name_changed)	(GogObject *obj);
	void (*possible_additions_changed) (GogObject const *obj);
	void (*child_added)	   (GogObject *parent, GogObject *child);
	void (*child_removed)	   (GogObject *parent, GogObject *child);
	void (*child_name_changed) (GogObject const *obj, GogObject const *child);
	void (*children_reordered) (GogObject *obj);
	void (*update_editor)	   (GogObject *obj);
	 /*< private >*/
	void (*extra_signal1)	   (GogObject *view);
	void (*extra_signal2)	   (GogObject *view);
} GogObjectClass;

#define GOG_TYPE_OBJECT		(gog_object_get_type ())
#define GOG_OBJECT(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_OBJECT, GogObject))
#define GOG_IS_OBJECT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_OBJECT))
#define GOG_OBJECT_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST ((k), GOG_TYPE_OBJECT, GogObjectClass))
#define GOG_IS_OBJECT_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GOG_TYPE_OBJECT))
#define GOG_OBJECT_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GOG_TYPE_OBJECT, GogObjectClass))

#define GOG_PARAM_FORCE_SAVE	(1 << (G_PARAM_USER_SHIFT+1))	/* even if the value == default */
#define GOG_PARAM_POSITION	(1 << (G_PARAM_USER_SHIFT+2))	/* position parameters */

GType gog_object_get_type (void);

typedef void (*GogDataDuplicator) (GogDataset const *src, GogDataset *dst);

GogObject   *gog_object_dup		 (GogObject const *src, GogObject *new_parent, GogDataDuplicator datadup);
GogObject   *gog_object_get_parent	 (GogObject const *obj);
GogObject   *gog_object_get_parent_typed (GogObject const *obj, GType t);
GogGraph    *gog_object_get_graph	 (GogObject const *obj);
GogTheme    *gog_object_get_theme	 (GogObject const *obj);
unsigned     gog_object_get_id		 (GogObject const *obj);
char const  *gog_object_get_name	 (GogObject const *obj);
void	     gog_object_set_name	 (GogObject *obj, char *name, GError **err);
GSList      *gog_object_get_children	 (GogObject const *obj, GogObjectRole const *filter);
GogObject   *gog_object_get_child_by_role(GogObject const *obj, GogObjectRole const *role);
GogObject   *gog_object_get_child_by_name(GogObject const *obj, char const *name);
gpointer     gog_object_get_editor	 (GogObject *obj,
					  GogDataAllocator *dalloc, GOCmdContext *cc);
GogView	  *gog_object_new_view		 (GogObject const *obj, GogView *parent);
gboolean   gog_object_is_deletable	 (GogObject const *obj);
GSList    *gog_object_possible_additions (GogObject const *parent);
GogObject *gog_object_add_by_role	 (GogObject *parent,
					  GogObjectRole const *role, GogObject *child);
GogObject   *gog_object_add_by_name	 (GogObject *parent,
					  char const *role, GogObject *child);
void		  gog_object_can_reorder (GogObject const *obj,
					  gboolean *inc_ok, gboolean *dec_ok);
GogObject	 *gog_object_reorder	 (GogObject const *obj,
					  gboolean inc, gboolean goto_max);

#define	  gog_object_is_visible(obj) (!((GogObject*)obj)->invisible)
void		  gog_object_set_invisible	       (GogObject *obj, gboolean invisible);
GogObjectPosition gog_object_get_position_flags	       (GogObject const *obj, GogObjectPosition mask);
gboolean          gog_object_set_position_flags        (GogObject *obj, GogObjectPosition flags,
							GogObjectPosition mask);
gboolean 	  gog_object_is_default_position_flags (GogObject const *obj, char const *name);
void	          gog_object_get_manual_position       (GogObject *obj, GogViewAllocation *pos);
void 		  gog_object_set_manual_position       (GogObject *obj, GogViewAllocation const *pos);

GogViewAllocation gog_object_get_manual_allocation (GogObject *gobj,
						    GogViewAllocation const *parent_allocation,
						    GogViewRequisition const *requisition);

GogObjectRole const *gog_object_find_role_by_name (GogObject const *obj,
						   char const *role);

/* protected */
void	 gog_object_update		  (GogObject *obj);
gboolean gog_object_request_update	  (GogObject *obj);
void 	 gog_object_emit_changed	  (GogObject *obj, gboolean size);
gboolean gog_object_clear_parent	  (GogObject *obj);
gboolean gog_object_set_parent		  (GogObject *child, GogObject *parent,
					   GogObjectRole const *role, unsigned int id);
void 	 gog_object_register_roles	  (GogObjectClass *klass,
					   GogObjectRole const *roles, unsigned int n_roles);
void 	 gog_object_request_editor_update (GogObject *obj);

void	 gog_object_document_changed	  (GogObject *obj, GODoc *doc);
GogManualSizeMode gog_object_get_manual_size_mode  (GogObject *obj);

G_END_DECLS

#endif /* GOG_OBJECT_H */
