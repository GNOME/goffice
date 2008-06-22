/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-doc.c : A GOffice Document
 *
 * Copyright (C) 2004-2006 Jody Goldberg (jody@gnome.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
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

#include <goffice/goffice-config.h>
#include <goffice/app/goffice-app.h>
#include <goffice/app/go-doc-impl.h>
#include <goffice/utils/go-glib-extras.h>

#include <gsf/gsf-doc-meta-data.h>
#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n.h>

enum {
	PROP_0,
	PROP_URI,
	PROP_DIRTY
};
enum {
	METADATA_CHANGED,
	METADATA_UPDATE,
	LAST_SIGNAL
};
static guint signals [LAST_SIGNAL] = { 0 };
static GObjectClass *go_doc_parent_class;

static void
go_doc_get_property (GObject *obj, guint property_id,
		     GValue *value, GParamSpec *pspec)
{
	GODoc *doc = (GODoc *)obj;

	switch (property_id) {
	case PROP_URI:
		g_value_set_string (value, doc->uri);
		break;

	case PROP_DIRTY:
		g_value_set_boolean (value, go_doc_is_dirty (doc));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
		break;
	}
}

static void
go_doc_set_property (GObject *obj, guint property_id,
		       const GValue *value, GParamSpec *pspec)
{
	GODoc *doc = (GODoc *)obj;

	switch (property_id) {
	case PROP_URI:
		go_doc_set_uri (doc, g_value_get_string (value));
		break;
	case PROP_DIRTY:
		go_doc_set_dirty (doc, g_value_get_boolean (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
		break;
	}
}

static void
go_doc_finalize (GObject *obj)
{
	GODoc *doc = (GODoc *)obj;

	g_object_unref (doc->meta_data);
	doc->meta_data = NULL;
	g_free (doc->uri);
	doc->uri = NULL;

	if (doc->images)
		g_hash_table_destroy (doc->images);
	doc->images = NULL;

	go_doc_parent_class->finalize (obj);
}

static void
go_doc_init (GODoc *obj)
{
	GODoc *doc = (GODoc *)obj;

	doc->meta_data	 = gsf_doc_meta_data_new ();
	doc->uri	 = NULL;
	doc->modified	 = FALSE;
	doc->pristine	 = TRUE;
}

static void
go_doc_class_init (GObjectClass *object_class)
{
	go_doc_parent_class = g_type_class_peek_parent (object_class);

	object_class->set_property = go_doc_set_property;
	object_class->get_property = go_doc_get_property;
	object_class->finalize = go_doc_finalize;

        g_object_class_install_property (object_class, PROP_URI,
		 g_param_spec_string ("uri", _("URI"),
			_("The URI associated with this document."),
			NULL, GSF_PARAM_STATIC | G_PARAM_READWRITE));

        g_object_class_install_property (object_class, PROP_DIRTY,
		 g_param_spec_boolean ("dirty",	_("Dirty"),
			_("Whether the document has been changed."),
			FALSE, GSF_PARAM_STATIC | G_PARAM_READWRITE));

	signals [METADATA_UPDATE] = g_signal_new ("metadata-update",
		GO_DOC_TYPE,	G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GODocClass, meta_data.update),
		NULL, NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE,	0, G_TYPE_NONE);
	signals [METADATA_CHANGED] = g_signal_new ("metadata-changed",
		GO_DOC_TYPE,	G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GODocClass, meta_data.changed),
		NULL, NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0, G_TYPE_NONE);
}

GSF_CLASS (GODoc, go_doc,
	   go_doc_class_init, go_doc_init,
	   G_TYPE_OBJECT)

/**
 * go_doc_set_uri:
 * @doc: the document to modify
 * @uri: the uri for this worksheet.
 *
 * Returns: TRUE if the name was set succesfully.
 **/
gboolean
go_doc_set_uri (GODoc *doc, char const *uri)
{
	char *new_uri;

	g_return_val_if_fail (doc != NULL, FALSE);
	g_return_val_if_fail (uri != NULL, FALSE);

	if (go_str_compare (uri, doc->uri) == 0)
		return TRUE;

	new_uri = g_strdup (uri);
	g_free (doc->uri);
	doc->uri = new_uri;

/* FIXME FIXME FIXME should we allow signal to return FALSE and disable the assignment ? */
	g_object_notify (G_OBJECT (doc), "uri");

	return TRUE;
}

char const *
go_doc_get_uri (GODoc const *doc)
{
	g_return_val_if_fail (IS_GO_DOC (doc), NULL);
	return doc->uri;
}

/**
 * go_doc_set_dirty :
 * @doc : #GODoc
 * @is_dirty : bool
 *
 * Changes the dirty state of @doc to @is_dirty and clears the pristine state
 * no matter what.
 **/
void
go_doc_set_dirty (GODoc *doc, gboolean is_dirty)
{
	g_return_if_fail (IS_GO_DOC (doc));

	is_dirty = !!is_dirty;
	if (is_dirty == doc->modified)
		return;

	/* Dirtiness changed so no longer pristine.  */
	doc->pristine = FALSE;

	doc->modified = is_dirty;
	g_object_notify (G_OBJECT (doc), "dirty");
}

/**
 * go_doc_is_dirty :
 * @doc : #GODoc
 *
 * Returns: TRUE if @doc has been modified.
 **/
gboolean
go_doc_is_dirty (GODoc const *doc)
{
	g_return_val_if_fail (IS_GO_DOC (doc), FALSE);

	return doc->modified;
}

/**
 * go_doc_is_pristine:
 * @doc:
 *
 *   This checks to see if the doc has ever been
 * used ( approximately )
 *
 * Return value: TRUE if we can discard this doc.
 **/
gboolean
go_doc_is_pristine (GODoc const *doc)
{
	g_return_val_if_fail (IS_GO_DOC (doc), FALSE);

#if 0
	if (doc->names ||
	    (doc->file_format_level > FILE_FL_NEW))
		return FALSE;
#endif

	return doc->pristine;
}

GsfDocMetaData *
go_doc_get_meta_data (GODoc const *doc)
{
	g_return_val_if_fail (IS_GO_DOC (doc), NULL);
	return doc->meta_data;
}

/**
 * go_doc_set_meta_data :
 * @doc : #GODoc
 * @data : #GsfDocMetaData
 *
 * Adds a ref to @data.
 **/
void
go_doc_set_meta_data (GODoc *doc, GsfDocMetaData *data)
{
	g_return_if_fail (IS_GO_DOC (doc));

	g_object_ref (data);
	g_object_unref (doc->meta_data);
	doc->meta_data = data;
	g_signal_emit (G_OBJECT (doc), signals [METADATA_CHANGED], 0);
}

/**
 * go_doc_update_meta_data :
 * @doc : #GODoc
 *
 * Signal that @doc's metadata should be updated
 * 	- statistics (sheet count, word count)
 * 	- content (sheet names, bookmarks)
 * 	- reloading linked items
 **/
void
go_doc_update_meta_data (GODoc *doc)
{
	g_return_if_fail (IS_GO_DOC (doc));

	/* update linked properties and automatic content */
	g_signal_emit (G_OBJECT (doc), signals [METADATA_UPDATE], 0);
}

GOImage *
go_doc_get_image (GODoc *doc, char const *id)
{
	return (doc->images != NULL)?
		(GOImage *) g_hash_table_lookup (doc->images, id):
		NULL;
}

GOImage *
go_doc_add_image (GODoc *doc, char const *id, GOImage *image)
{
	GHashTableIter iter;
	char const *key;
	GOImage *img;
	int i = 0;
	char *new_id;

	if (doc->images == NULL)
		doc->images = g_hash_table_new_full (g_str_hash, g_str_equal,
						     g_free, g_object_unref);
	/* check if the image is already there */
	g_hash_table_iter_init (&iter, doc->images);
	while (g_hash_table_iter_next (&iter, (void**) &key, (void**) &img))
		if (go_image_same_pixbuf (image, img))
			return img;
	/* now check if the id is not a duplicate */
	if (g_hash_table_lookup (doc->images, id)) {
		while (1) {
			new_id = g_strdup_printf ("%s(%d)", id, i++);
			if (g_hash_table_lookup (doc->images, new_id))
				g_free (new_id);
			else
				break;
		}
	} else
		new_id = g_strdup (id);
	go_image_set_name (image, new_id),
	g_hash_table_insert (doc->images, new_id, g_object_ref (image));
	return image;
}

GHashTable *
go_doc_get_images (GODoc *doc) {
	g_return_val_if_fail (doc, NULL);
	return doc->images;
}

static void
init_func (gpointer key, gpointer value, gpointer data)
{
	go_image_init_save ((GOImage*) value);
}

void
go_doc_init_write (GODoc *doc)
{
	g_hash_table_foreach (doc->images, init_func, NULL);
}
