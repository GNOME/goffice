/*
 * go-persist.h :
 *
 * Copyright (C) 2003-2008 Jody Goldberg (jody@gnome.org)
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
#ifndef GO_PERSIST_H
#define GO_PERSIST_H

#include <goffice/goffice.h>
#include <libxml/tree.h>
#include <gsf/gsf-libxml.h>

G_BEGIN_DECLS

typedef struct _GOPersist GOPersist;

typedef struct {
	GTypeInterface base;

	void	 (*prep_sax) (GOPersist *gp, GsfXMLIn *xin, xmlChar const **attrs);
	void     (*sax_save) (GOPersist const *gp, GsfXMLOut *output);
} GOPersistClass;

#define GO_TYPE_PERSIST		 (go_persist_get_type ())
#define GO_PERSIST(o)		 (G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_PERSIST, GOPersist))
#define GO_IS_PERSIST(o)	 (G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_PERSIST))
#define GO_PERSIST_CLASS(k)	 (G_TYPE_CHECK_CLASS_CAST ((k), GO_TYPE_PERSIST, GOPersistClass))
#define GO_IS_PERSIST_CLASS(k)	 (G_TYPE_CHECK_CLASS_TYPE ((k), GO_TYPE_PERSIST))
#define GO_PERSIST_GET_CLASS(o)	 (G_TYPE_INSTANCE_GET_INTERFACE ((o), GO_TYPE_PERSIST, GOPersistClass))

GType go_persist_get_type (void);

void     go_persist_sax_save (GOPersist const *gp, GsfXMLOut *output);
void	 go_persist_prep_sax (GOPersist *gp,
			       GsfXMLIn *xin, xmlChar const **attrs);

#define GO_PARAM_PERSISTENT	(1 << (G_PARAM_USER_SHIFT+0))

G_END_DECLS

#endif /* GO_PERSIST_H */
