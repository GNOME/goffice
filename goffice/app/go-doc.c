/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-doc.c : A GOffice Document
 *
 * Copyright (C) 2004-2006 Jody Goldberg (jody@gnome.org)
 * Copyright (C) 2008-2009 Morten Welinder (terra@gnome.org)
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

#include <gsf/gsf-doc-meta-data.h>
#include <gsf/gsf-impl-utils.h>
#include <gsf/gsf-libxml.h>
#include <glib/gi18n.h>

#include <string.h>

enum {
	PROP_0,
	PROP_URI,
	PROP_DIRTY,
	PROP_PRISTINE
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

	case PROP_PRISTINE:
		g_value_set_boolean (value, go_doc_is_pristine (doc));
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

 	case PROP_PRISTINE:
		go_doc_set_pristine (doc, g_value_get_boolean (value));
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
		 g_param_spec_boolean ("dirty", _("Dirty"),
			_("Whether the document has been changed."),
			FALSE, GSF_PARAM_STATIC | G_PARAM_READWRITE));

        g_object_class_install_property (object_class, PROP_PRISTINE,
		 g_param_spec_boolean ("pristine", _("Pristine"),
			_("Whether the document is unchanged since it was created."),
			FALSE, GSF_PARAM_STATIC | G_PARAM_READWRITE));

	signals [METADATA_UPDATE] = g_signal_new ("metadata-update",
		GO_TYPE_DOC,	G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GODocClass, meta_data.update),
		NULL, NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE,	0, G_TYPE_NONE);
	signals [METADATA_CHANGED] = g_signal_new ("metadata-changed",
		GO_TYPE_DOC,	G_SIGNAL_RUN_LAST,
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
	g_return_val_if_fail (GO_IS_DOC (doc), NULL);
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
	g_return_if_fail (GO_IS_DOC (doc));

	is_dirty = !!is_dirty;
	if (is_dirty == doc->modified)
		return;

	/* Dirtiness changed so no longer pristine.  */
	go_doc_set_pristine (doc, FALSE);

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
	g_return_val_if_fail (GO_IS_DOC (doc), FALSE);

	return doc->modified;
}

/**
 * go_doc_set_pristine:
 * @doc: #GODoc
 * @pristine: a gboolean.
 *
 * Sets the indication of whether this document is unchanged since it was
 * created.  Note: if both "dirty" and "pristine" are being set, set
 * "pristine" last.
 **/
void
go_doc_set_pristine (GODoc *doc, gboolean pristine)
{
	g_return_if_fail (GO_IS_DOC (doc));

	pristine = !!pristine;
	if (pristine == doc->pristine)
		return;

	doc->pristine = pristine;
	g_object_notify (G_OBJECT (doc), "pristine");
}

/**
 * go_doc_is_pristine:
 * @doc: #GODoc
 *
 * This checks to see if the doc has ever been
 * used ( approximately )
 *
 * Return value: %TRUE if we can discard this doc.
 **/
gboolean
go_doc_is_pristine (GODoc const *doc)
{
	g_return_val_if_fail (GO_IS_DOC (doc), FALSE);
	return doc->pristine;
}

GsfDocMetaData *
go_doc_get_meta_data (GODoc const *doc)
{
	g_return_val_if_fail (GO_IS_DOC (doc), NULL);
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
	g_return_if_fail (GO_IS_DOC (doc));

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
	g_return_if_fail (GO_IS_DOC (doc));

	/* update linked properties and automatic content */
	g_signal_emit (G_OBJECT (doc), signals [METADATA_UPDATE], 0);
}

/** go_doc_get_image:
 * @doc: a #GODoc
 * @id: the image name
 *
 * Returns: the #GOImage is one exist with name @id. The caller does not own a
 * reference.
 */
GOImage *
go_doc_get_image (GODoc *doc, char const *id)
{
	return (doc->images != NULL)?
		(GOImage *) g_hash_table_lookup (doc->images, id):
		NULL;
}

#ifndef HAVE_G_HASH_TABLE_ITER_INIT
struct check_for_pixbuf {
	GOImage *src_image;
	GOImage *dst_image;
};

static void
check_for_pixbuf (gpointer key, gpointer img_, gpointer user)
{
	GOImage *img = img_;
	struct check_for_pixbuf *cl = user;

	if (cl->dst_image == NULL) {
		if (go_image_same_pixbuf (cl->src_image, img))
			cl->dst_image = img;
	}
}
#endif

/** go_doc_add_image:
 * @doc: a #GODoc
 * @id: the image name or NULL
 * @image: a #GOImage
 *
 * Adds @image to the document if no such image already exists. The name of
 * the returned image might be different from @id, even if given.
 * Returns: either @image, in which case the document adds a reference on it, or
 * an identical image for which the owner does not own a reference.
 */
GOImage *
go_doc_add_image (GODoc *doc, char const *id, GOImage *image)
{
	GOImage *img;
	int i = 0;
	char *new_id;
#ifdef HAVE_G_HASH_TABLE_ITER_INIT
	GHashTableIter iter;
	char const *key;
#else
	struct check_for_pixbuf cl;
#endif

	if (doc->images == NULL)
		doc->images = g_hash_table_new_full (g_str_hash, g_str_equal,
						     g_free, g_object_unref);

	/* check if the image is already there */
#ifdef HAVE_G_HASH_TABLE_ITER_INIT
	g_hash_table_iter_init (&iter, doc->images);
	while (g_hash_table_iter_next (&iter, (void**) &key, (void**) &img))
		if (go_image_same_pixbuf (image, img))
			return img;
#else
	cl.src_image = image;
	cl.dst_image = NULL;
	g_hash_table_foreach (doc->images, check_for_pixbuf, &img);
	if (cl.dst_image)
		return cl.dst_image;
#endif

	if (!id || !*id)
		id = _("Image");
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

void
go_doc_init_write (GODoc *doc, GsfXMLOut *output)
{
	g_return_if_fail (GO_IS_DOC (doc));
	g_return_if_fail (doc->imagebuf == NULL);

	doc->imagebuf = g_hash_table_new_full (g_str_hash, g_str_equal,
					       g_free, NULL);
	g_object_set_data (G_OBJECT (gsf_xml_out_get_output (output)),
			   "document", doc);
}

void
go_doc_init_read (GODoc *doc, GsfInput *input)
{
	g_return_if_fail (GO_IS_DOC (doc));
	g_return_if_fail (doc->imagebuf == NULL);

	doc->imagebuf = g_hash_table_new_full (g_str_hash, g_str_equal,
					       g_free, g_object_unref);
	g_object_set_data (G_OBJECT (input), "document", doc);
}

static void
save_image_cb (gpointer key, gpointer img_, gpointer user)
{
	go_image_save ((GOImage *) img_, (GsfXMLOut *) user);
}

void
go_doc_write (GODoc *doc, GsfXMLOut *output)
{
	if (g_hash_table_size (doc->imagebuf) > 0) {
		gsf_xml_out_start_element (output, "GODoc");
		g_hash_table_foreach (doc->imagebuf, save_image_cb, output);
		gsf_xml_out_end_element (output);
	}
	g_hash_table_destroy (doc->imagebuf);
	doc->imagebuf = NULL;
}

void
go_doc_save_image (GODoc *doc, char const *id)
{
	if (!doc)
		return;
	if (!g_hash_table_lookup (doc->imagebuf, id)) {
		GOImage *image = g_hash_table_lookup (doc->images, id);
		if (image)
			g_hash_table_replace (doc->imagebuf,
					      g_strdup (id),
					      image);
	}
}

static void
load_image (GsfXMLIn *xin, xmlChar const **attrs)
{
	GODoc *doc = GO_DOC (xin->user_state);
	GOImage *image;
	xmlChar const **attr = 	attrs;
	if (!*attr)
		return;
	while (*attr && strcmp (*attr, "name"))
		attr += 2;
	image = (GOImage *) g_hash_table_lookup (doc->imagebuf, attr[1]);
	if (!image) /* this should not occur, but if it does, we might want to load the image? */
		return;
	go_image_load_attrs (image, xin, attrs);
	g_object_set_data (G_OBJECT (doc), "new image", image);
}

static void
load_image_data (GsfXMLIn *xin, GsfXMLBlob *unknown)
{
	GODoc *doc = GO_DOC (xin->user_state);
	GOImage *image = GO_IMAGE (g_object_get_data (G_OBJECT (doc), "new image")), *real;
	g_return_if_fail (image != NULL);
	go_image_load_data (image, xin);
	real = go_doc_add_image (doc, go_image_get_name (image), image);
	g_hash_table_remove (doc->imagebuf, (gpointer) go_image_get_name (image));
	/*
	 * We have an issue if the image already existed and this can
	 * happen on pasting or if one day, we implement merging two
	 * documents (import a workbook as new sheets in an existing
	 * workbook). At the moment, I don't see any way to tell the
	 * clients to reference the image stored in the document instead
	 * of the one created by go_doc_image_fetch, so let's just make
	 * certain they share the same id. Anyway this should not happen
	 * very often (may be with a company logo?) and it is not so
	 * harmful since the duplication will not survive
	 * serialization. (Jean)
	 */
	if (real != image) {
		go_image_set_name (image, go_image_get_name (real));
		g_object_unref (image);
	}
	g_object_set_data (G_OBJECT (doc), "new image", NULL);
}

void
go_doc_read (GODoc *doc, GsfXMLIn *xin, xmlChar const **attrs)
{
	static GsfXMLInNode const dtd[] = {
		GSF_XML_IN_NODE 	(DOC, DOC,
					 -1, "GODoc",
					 FALSE, NULL, NULL),
		GSF_XML_IN_NODE 	(DOC, IMAGE,
					 -1, "GOImage",
					 GSF_XML_CONTENT,
					 &load_image, &load_image_data),
		GSF_XML_IN_NODE_END
	};
	static GsfXMLInDoc *xmldoc = NULL;
	if (NULL == xmldoc)
		xmldoc = gsf_xml_in_doc_new (dtd, NULL);
	gsf_xml_in_push_state (xin, xmldoc, doc, NULL, attrs);
}

void
go_doc_end_read	(GODoc *doc)
{
	g_hash_table_destroy (doc->imagebuf);
	doc->imagebuf = NULL;
}

/** go_doc_image_fetch:
 * @doc: a #GODoc
 * @id: the name for the new image.
 *
 * Searches for a #GOImage with name @id in the document image buffer and
 * creates one if needed. The caller does not own a reference on the returned
 * #GOImage.
 * This function must be called after a call to go_doc_init_read(), otherwise
 * it will emit a critical and return NULL.
 * Returns: the found or created #GOImage.
 */
GOImage *
go_doc_image_fetch (GODoc *doc, char const *id)
{
	GOImage *image;
	g_return_val_if_fail (doc && doc->imagebuf, NULL);
	image = g_hash_table_lookup (doc->imagebuf, id);
	if (!image) {
		image = g_object_new (GO_TYPE_IMAGE, NULL);
		go_image_set_name (image, id);
		g_hash_table_replace (doc->imagebuf,
				      g_strdup (go_image_get_name (image)),
				      image);
	}
	return image;
}
