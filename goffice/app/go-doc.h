/*
 * go-doc.h :  A GOffice document
 *
 * Copyright (C) 2004-2006 Jody Goldberg (jody@gnome.org)
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
#ifndef GO_DOC_H
#define GO_DOC_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

#define GO_TYPE_DOC	    (go_doc_get_type ())
#define GO_DOC(o)	    (G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_DOC, GODoc))
#define GO_IS_DOC(o)	    (G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_DOC))

GType go_doc_get_type (void);

void     go_doc_set_pristine             (GODoc *doc, gboolean pristine);
gboolean go_doc_is_pristine		 (GODoc const *doc);

void	 go_doc_set_dirty		 (GODoc *doc, gboolean is_dirty);
gboolean go_doc_is_dirty		 (GODoc const *doc);

void     go_doc_set_dirty_time           (GODoc *doc, gint64 t);
gint64   go_doc_get_dirty_time           (GODoc const *doc);

void     go_doc_set_modtime              (GODoc *doc, GDateTime *modtime);
GDateTime *go_doc_get_modtime            (GODoc const *doc);

void     go_doc_set_state                (GODoc *doc, guint64 state);
void     go_doc_bump_state               (GODoc *doc);
guint64  go_doc_get_state                (GODoc *doc);

void     go_doc_set_saved_state          (GODoc *doc, guint64 state);
guint64  go_doc_get_saved_state          (GODoc *doc);

gboolean	 go_doc_set_uri		 (GODoc *doc, char const *uri);
char const	*go_doc_get_uri		 (GODoc const *doc);

GsfDocMetaData	*go_doc_get_meta_data	 (GODoc const *doc);
void		 go_doc_set_meta_data	 (GODoc *doc, GsfDocMetaData *data);
void		 go_doc_update_meta_data (GODoc *doc);

/* put into GODoc (as properties) */

/* Images related functions */
GOImage		*go_doc_get_image	(GODoc *doc, char const *id);
GOImage		*go_doc_add_image	(GODoc *doc, char const *id, GOImage *image);
GHashTable	*go_doc_get_images	(GODoc *doc);
void		 go_doc_init_write	(GODoc *doc, GsfXMLOut *output);
void		 go_doc_write		(GODoc *doc, GsfXMLOut *output);
void		 go_doc_save_image	(GODoc *doc, char const *id);
void		 go_doc_init_read	(GODoc *doc, GsfInput *input);
void		 go_doc_read		(GODoc *doc, GsfXMLIn *xin, xmlChar const **attrs);
void		 go_doc_end_read	(GODoc *doc);
GOImage		*go_doc_image_fetch 	(GODoc *doc, char const *id, GType type);
void		 go_doc_save_resource (GODoc *doc, GOPersist const *gp);

G_END_DECLS

#endif /* GO_DOC_H */
