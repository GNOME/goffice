/*
 * go-doc-impl.h : Implementation details of a GOffice Document
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
#ifndef GO_DOC_IMPL_H
#define GO_DOC_IMPL_H

#include <goffice/app/go-doc.h>

G_BEGIN_DECLS

struct _GODoc {
	GObject		 base;

	gchar		*uri;
	GsfDocMetaData	*meta_data;
	gboolean	 modified;
	gint64           first_modification_time;
	gboolean	 pristine;
	GHashTable	*images;
	GDateTime       *modtime;

	/* <private> */
	struct _GODocPrivate *priv;
};

typedef struct {
	GObjectClass	base;

	struct _GOMetaDataIFace {
		/* Reload doc statistics and update linked values */
		void (*update)  (GODoc *doc);
		void (*changed) (GODoc *doc);
	} meta_data;
} GODocClass;

#define GO_DOC_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GO_TYPE_DOC, GODocClass))
#define GO_IS_DOC_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GO_TYPE_DOC))
#define GO_DOC_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GO_TYPE_DOC, GODocClass))

G_END_DECLS

#endif /* GO_DOC_IMPL_H */
