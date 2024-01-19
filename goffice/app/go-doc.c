/*
 * go-doc.c : A GOffice Document
 *
 * Copyright (C) 2004-2006 Jody Goldberg (jody@gnome.org)
 * Copyright (C) 2008-2009 Morten Welinder (terra@gnome.org)
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

#include <goffice/goffice-config.h>
#include <goffice/app/goffice-app.h>
#include <goffice/app/go-doc-impl.h>

#include <gsf/gsf-doc-meta-data.h>
#include <gsf/gsf-impl-utils.h>
#include <gsf/gsf-libxml.h>
#include <glib/gi18n.h>

#include <string.h>

struct _GODocPrivate {
	GHashTable	*imagebuf; /* used when loading/saving images */

	/* color maps */
	GSList	*resourcesbuf; /* used when loading/saving color maps and themes */

	guint64 state, saved_state, last_used_state;
};

/**
 * _GODoc:
 * @base: parent object.
 * @uri: URI.
 * @meta_data: metadata.
 * @modified: whether the document has been modified.
 * @first_modification_time: date of the first modification.
 * @pristine: whether the document is unchanged since it was created.
 * @images: images used inside the document.
**/

enum {
	PROP_0,
	PROP_URI,
	PROP_DIRTY,
	PROP_DIRTY_TIME,
	PROP_PRISTINE,
	PROP_MODTIME,
	PROP_STATE,
	PROP_SAVED_STATE
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

	case PROP_DIRTY_TIME:
		g_value_set_int64 (value, go_doc_get_dirty_time (doc));
		break;

	case PROP_PRISTINE:
		g_value_set_boolean (value, go_doc_is_pristine (doc));
		break;

	case PROP_MODTIME:
		g_value_set_boxed (value, go_doc_get_modtime (doc));
		break;

	case PROP_STATE:
		g_value_set_uint64 (value, go_doc_get_state (doc));
		break;

	case PROP_SAVED_STATE:
		g_value_set_uint64 (value, go_doc_get_saved_state (doc));
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

 	case PROP_DIRTY_TIME:
		go_doc_set_dirty_time (doc, g_value_get_int64 (value));
		break;

 	case PROP_PRISTINE:
		go_doc_set_pristine (doc, g_value_get_boolean (value));
		break;

	case PROP_MODTIME:
		go_doc_set_modtime (doc, g_value_get_boxed (value));
		break;

	case PROP_STATE:
		go_doc_set_state (doc, g_value_get_uint64 (value));
		break;

	case PROP_SAVED_STATE:
		go_doc_set_saved_state (doc, g_value_get_uint64 (value));
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

	go_doc_set_modtime (doc, NULL);

	if (doc->images)
		g_hash_table_destroy (doc->images);
	doc->images = NULL;
	g_free (doc->priv);
	doc->priv = NULL;

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
	doc->priv = g_new0 (struct _GODocPrivate, 1);
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

        g_object_class_install_property (object_class, PROP_DIRTY_TIME,
		 g_param_spec_int64 ("dirty-time", _("Dirty Time"),
			_("When the document was first changed."),
			G_MININT64, G_MAXINT64, 0,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));

        g_object_class_install_property (object_class, PROP_PRISTINE,
		 g_param_spec_boolean ("pristine", _("Pristine"),
			_("Whether the document is unchanged since it was created."),
			FALSE, GSF_PARAM_STATIC | G_PARAM_READWRITE));
        g_object_class_install_property (object_class, PROP_MODTIME,
		 g_param_spec_boxed ("modtime",
				     _("Modification time"),
				     _("The known file system modification time"),
				     G_TYPE_DATE_TIME,
				     GSF_PARAM_STATIC |
				     G_PARAM_READWRITE));
        g_object_class_install_property (object_class, PROP_STATE,
		 g_param_spec_uint64 ("state", _("State"),
			_("Current state of document"),
			0, G_MAXUINT64, 0,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
        g_object_class_install_property (object_class, PROP_STATE,
		 g_param_spec_uint64 ("saved-state", _("Saved state"),
			_("State of document when last saved"),
			0, G_MAXUINT64, 0,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));


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
 * Returns: %TRUE if the name was set succesfully.
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
 * go_doc_set_dirty:
 * @doc: #GODoc
 * @is_dirty:bool
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

	doc->modified = is_dirty;
	g_object_notify (G_OBJECT (doc), "dirty");

	go_doc_set_dirty_time (doc, is_dirty ? g_get_real_time () : 0);

	/* Dirtiness changed so no longer pristine.  */
	go_doc_set_pristine (doc, FALSE);
}

/**
 * go_doc_is_dirty:
 * @doc: #GODoc
 *
 * Returns: %TRUE if @doc has been modified.
 **/
gboolean
go_doc_is_dirty (GODoc const *doc)
{
	g_return_val_if_fail (GO_IS_DOC (doc), FALSE);

	return doc->modified;
}

/**
 * go_doc_set_dirty_time:
 * @doc: #GODoc
 * @t: a timestamp from g_get_real_time
 *
 * Changes the dirty time, i.e., the time the document was first marked
 * dirty.
 **/
void
go_doc_set_dirty_time (GODoc *doc, gint64 t)
{
	g_return_if_fail (GO_IS_DOC (doc));

	if (doc->first_modification_time == t)
		return;

	doc->first_modification_time = t;
	g_object_notify (G_OBJECT (doc), "dirty-time");
}

/**
 * go_doc_get_dirty_time:
 * @doc: #GODoc
 *
 * Returns: the time (as in g_get_real_time()) the document was first marked
 * dirty.
 **/
gint64
go_doc_get_dirty_time (GODoc const *doc)
{
	g_return_val_if_fail (GO_IS_DOC (doc), 0);
	return doc->first_modification_time;
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

/**
 * go_doc_get_meta_data:
 * @doc: #GODoc
 *
 * Returns: (transfer none): @doc's metadata
 **/
GsfDocMetaData *
go_doc_get_meta_data (GODoc const *doc)
{
	g_return_val_if_fail (GO_IS_DOC (doc), NULL);
	return doc->meta_data;
}

/**
 * go_doc_set_meta_data:
 * @doc: #GODoc
 * @data: #GsfDocMetaData
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
 * go_doc_update_meta_data:
 * @doc: #GODoc
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

/**
 * go_doc_set_modtime:
 * @doc: #GODoc
 * @modtime: (allow-none): new file system time stamp
 *
 * Sets the last known file system time stamp for the document, %NULL
 * if unknown.
 **/
void
go_doc_set_modtime (GODoc *doc, GDateTime *modtime)
{
	g_return_if_fail (GO_IS_DOC (doc));

	if (modtime == doc->modtime)
		return;

	if (modtime)
		g_date_time_ref (modtime);
	if (doc->modtime)
		g_date_time_unref (doc->modtime);
	doc->modtime = modtime;

	g_object_notify (G_OBJECT (doc), "modtime");
}

/**
 * go_doc_get_modtime:
 * @doc: #GODoc
 *
 * Returns: (transfer none): the last known file system time stamp for
 * the document, or %NULL if unknown.
 **/
GDateTime *
go_doc_get_modtime (GODoc const *doc)
{
	g_return_val_if_fail (GO_IS_DOC (doc), NULL);

	return doc->modtime;
}


/**
 * go_doc_set_state:
 * @doc: #GODoc
 * @state: state of document
 *
 * Sets the current state of the document as a serial number that is intended
 * to be incremented for each document change.
 *
 * Setting the state will set the document's dirty status also, assuming both
 * the state and the saved state are known.
 **/
void
go_doc_set_state (GODoc *doc, guint64 state)
{
	g_return_if_fail (GO_IS_DOC (doc));

	if (doc->priv->state == state)
		return;

	doc->priv->state = state;
	g_object_notify (G_OBJECT (doc), "state");
	go_doc_set_dirty (doc, state != doc->priv->saved_state);
}

/**
 * go_doc_bump_state:
 * @doc: #GODoc
 *
 * Sets the current state of the document to a fresh id.
 **/
void
go_doc_bump_state (GODoc *doc)
{
	g_return_if_fail (GO_IS_DOC (doc));
	go_doc_set_state (doc, ++(doc->priv->last_used_state));
}


/**
 * go_doc_get_state:
 * @doc: #GODoc
 *
 * Returns: the current state of the document.
 **/
guint64
go_doc_get_state (GODoc *doc)
{
	g_return_val_if_fail (GO_IS_DOC (doc), 0);

	return doc->priv->state;
}

/**
 * go_doc_set_saved_state:
 * @doc: #GODoc
 * @state: state at the time of last save
 *
 * Sets the state at the last time the document was saved.
 **/
void
go_doc_set_saved_state (GODoc *doc, guint64 state)
{
	g_return_if_fail (GO_IS_DOC (doc));

	if (doc->priv->saved_state == state)
		return;

	doc->priv->saved_state = state;
	g_object_notify (G_OBJECT (doc), "saved-state");
	go_doc_set_dirty (doc, state != doc->priv->state);
}


/**
 * go_doc_get_saved_state:
 * @doc: #GODoc
 *
 * Returns: the state at the last time the document was saved.
 **/
guint64
go_doc_get_saved_state (GODoc *doc)
{
	g_return_val_if_fail (GO_IS_DOC (doc), 0);

	return doc->priv->saved_state;
}


/**
 * go_doc_get_image:
 * @doc: a #GODoc
 * @id: the image name
 *
 * Return value: (transfer none): the #GOImage is one exist with name @id. The caller does not own a
 * reference.
 **/
GOImage *
go_doc_get_image (GODoc *doc, char const *id)
{
	return (doc->images != NULL)?
		(GOImage *) g_hash_table_lookup (doc->images, id):
		NULL;
}

/**
 * go_doc_add_image:
 * @doc: a #GODoc
 * @id: the image name or %NULL
 * @image: a #GOImage
 *
 * Adds @image to the document if no such image already exists. The name of
 * the returned image might be different from @id, even if given.
 * Returns: (transfer none): either @image, in which case the document adds a reference on it, or
 * an identical image for which the owner does not own a reference.
 **/
GOImage *
go_doc_add_image (GODoc *doc, char const *id, GOImage *image)
{
	GOImage *img;
	int i = 0;
	char *new_id;
	GHashTableIter iter;
	char const *key;

	if (doc->images == NULL)
		doc->images = g_hash_table_new_full (g_str_hash, g_str_equal,
						     g_free, g_object_unref);

	/* check if the image is already there */
	g_hash_table_iter_init (&iter, doc->images);
	while (g_hash_table_iter_next (&iter, (void**) &key, (void**) &img))
		if (!go_image_differ (image, img))
			return img;

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

/**
 * go_doc_get_images:
 * @doc: #GODoc
 *
 * Returns: (transfer none): the images registered inside the document
 **/
GHashTable *
go_doc_get_images (GODoc *doc) {
	g_return_val_if_fail (doc, NULL);
	return doc->images;
}

void
go_doc_init_write (GODoc *doc, GsfXMLOut *output)
{
	g_return_if_fail (GO_IS_DOC (doc));
	g_return_if_fail (doc->priv->imagebuf == NULL);

	doc->priv->imagebuf = g_hash_table_new_full (g_str_hash, g_str_equal,
					       g_free, NULL);
	g_object_set_data (G_OBJECT (gsf_xml_out_get_output (output)),
			   "document", doc);
}

void
go_doc_init_read (GODoc *doc, GsfInput *input)
{
	g_return_if_fail (GO_IS_DOC (doc));
	g_return_if_fail (doc->priv->imagebuf == NULL);

	doc->priv->imagebuf = g_hash_table_new_full (g_str_hash, g_str_equal,
					       g_free, g_object_unref);
	g_object_set_data (G_OBJECT (input), "document", doc);
}

static void
save_image_cb (G_GNUC_UNUSED gpointer key, gpointer img_, gpointer user)
{
	go_image_save ((GOImage *) img_, (GsfXMLOut *) user);
}

void
go_doc_write (GODoc *doc, GsfXMLOut *output)
{
	GSList *ptr;
	if (g_hash_table_size (doc->priv->imagebuf) > 0 ||
	    doc->priv->resourcesbuf != NULL) {
		gsf_xml_out_start_element (output, "GODoc");
		g_hash_table_foreach (doc->priv->imagebuf, save_image_cb, output);
		for (ptr = doc->priv->resourcesbuf; ptr; ptr = ptr->next) {
			gsf_xml_out_start_element (output, G_OBJECT_TYPE_NAME (ptr->data));
			go_persist_sax_save (GO_PERSIST (ptr->data), output);
			gsf_xml_out_end_element (output);
		}
		g_slist_free (doc->priv->resourcesbuf);
		doc->priv->resourcesbuf = NULL;
		gsf_xml_out_end_element (output);
	}
	g_hash_table_destroy (doc->priv->imagebuf);
	doc->priv->imagebuf = NULL;
}

/**
 * go_doc_save_image:
 * @doc: a #GODoc
 * @id: the Id of the #GOImage to save
 *
 * Saves the image with the document. Each image will be saved only
 * once.
 **/
void
go_doc_save_image (GODoc *doc, char const *id)
{
	if (!doc)
		return;
	if (!g_hash_table_lookup (doc->priv->imagebuf, id)) {
		GOImage *image = go_doc_get_image (doc, id);
		if (image)
			g_hash_table_replace (doc->priv->imagebuf,
					      g_strdup (id),
					      image);
	}
}

static void
load_image (GsfXMLIn *xin, xmlChar const **attrs)
{
	GODoc *doc = GO_DOC (xin->user_state);
	GOImage *image = NULL;
	xmlChar const **attr;
	GType type = 0;
	if (!attrs || !*attrs)
		return;
	for (attr = attrs; *attr; attr += 2)
		if (!strcmp (*attr, "name"))
			image = (GOImage *) g_hash_table_lookup (doc->priv->imagebuf, attr[1]);
		else if (!strcmp (*attr, "type"))
			type = g_type_from_name (attr[1]);
	if (!image) /* this should not occur, but if it does, we might want to load the image? */
		return;
	g_return_if_fail (type == 0 || G_OBJECT_TYPE (image) == type);
	go_image_load_attrs (image, xin, attrs);
	g_object_set_data (G_OBJECT (doc), "new image", image);
}

static void
load_image_data (GsfXMLIn *xin, G_GNUC_UNUSED GsfXMLBlob *unknown)
{
	GODoc *doc = GO_DOC (xin->user_state);
	GOImage *image = GO_IMAGE (g_object_get_data (G_OBJECT (doc), "new image")), *real;
	g_return_if_fail (image != NULL);
	go_image_load_data (image, xin);
	real = go_doc_add_image (doc, go_image_get_name (image), image);
	g_hash_table_remove (doc->priv->imagebuf, (gpointer) go_image_get_name (image));
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

static void
load_color_map (GsfXMLIn *xin, xmlChar const **attrs)
{
	GogAxisColorMap *map = NULL;
	for (; attrs && *attrs; attrs +=2)
		if (!strcmp ((char const *) *attrs, "id")) {
			map = GOG_AXIS_COLOR_MAP (gog_axis_color_map_get_from_id ((char const *) attrs[1]));
			break;
		}
	if (map)
		go_persist_prep_sax (GO_PERSIST (map), xin, attrs);
}

static void
load_theme (GsfXMLIn *xin, xmlChar const **attrs)
{
	GogTheme *theme = NULL;
	for (; attrs && *attrs; attrs +=2)
		if (!strcmp ((char const *) *attrs, "id")) {
			theme = GOG_THEME (gog_theme_registry_lookup ((char const *) attrs[1]));
			break;
		}
	if (theme)
		go_persist_prep_sax (GO_PERSIST (theme), xin, attrs);
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
		GSF_XML_IN_NODE 	(DOC, COLOR_MAP,
					 -1, "GogAxisColorMap",
					 GSF_XML_NO_CONTENT,
					 &load_color_map, NULL),
		GSF_XML_IN_NODE 	(DOC, THEME,
					 -1, "GogTheme",
					 GSF_XML_NO_CONTENT,
					 &load_theme, NULL),
		GSF_XML_IN_NODE_END
	};
	static GsfXMLInDoc *xmldoc = NULL;
	if (NULL == xmldoc) {
		xmldoc = gsf_xml_in_doc_new (dtd, NULL);
		go_xml_in_doc_dispose_on_exit (&xmldoc);
	}
	gsf_xml_in_push_state (xin, xmldoc, doc, NULL, attrs);
}

void
go_doc_end_read	(GODoc *doc)
{
	g_hash_table_destroy (doc->priv->imagebuf);
	doc->priv->imagebuf = NULL;
}

/**
 * go_doc_image_fetch:
 * @doc: a #GODoc
 * @id: the name for the new image.
 * @type: the type of the #GOImage to create if needed.
 *
 * Searches for a #GOImage with name @id in the document image buffer and
 * creates one if needed. The caller does not own a reference on the returned
 * #GOImage.
 * This function must be called after a call to go_doc_init_read(), otherwise
 * it will emit a critical and return NULL.
 * Returns: (transfer none): the found or created #GOImage.
 **/
GOImage *
go_doc_image_fetch (GODoc *doc, char const *id, GType type)
{
	GOImage *image;
	g_return_val_if_fail (doc && doc->priv->imagebuf, NULL);
	image = g_hash_table_lookup (doc->priv->imagebuf, id);
	if (!image) {
		g_return_val_if_fail (g_type_is_a (type, GO_TYPE_IMAGE), NULL);
		image = g_object_new (type, NULL);
		if (!GO_IS_IMAGE (image)) {
			if (image)
				g_object_unref (image);
			g_critical ("Invalid image type");
			return NULL;
		}
		go_image_set_name (image, id);
		g_hash_table_replace (doc->priv->imagebuf,
				      g_strdup (go_image_get_name (image)),
				      image);
	}
	return image;
}

/**
 * go_doc_save_resource:
 * @doc: a #GODoc
 * @gp: the #GOPersist to save
 *
 * Saves the resource with the document. Each resource will be saved only
 * once.
 **/
void
go_doc_save_resource (GODoc *doc, GOPersist const *gp)
{
	GSList *ptr;
	for (ptr = doc->priv->resourcesbuf; ptr; ptr = ptr->next)
		if (ptr->data == gp) /* already marked for saving */
			return;
	doc->priv->resourcesbuf = g_slist_prepend (doc->priv->resourcesbuf, (void *) gp);
}
